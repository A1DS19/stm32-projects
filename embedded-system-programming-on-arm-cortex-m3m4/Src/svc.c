/* Lesson: SVC & PendSV — exceptions for system-level services
 * (slides 203-220).
 *
 * SVC (SuperVisor Call) is a Thumb instruction whose whole job is to
 * raise exception 11. It is THE SYSTEM-CALL DOOR: unprivileged thread
 * code executes `SVC #n`, the core switches to handler mode (always
 * privileged — the access lesson), and kernel code decides what service
 * number n meant. Every OS you have ever used works this way: a Linux
 * syscall on ARM is the same instruction, and an RTOS with the MPU on
 * (FreeRTOS MPU ports) wraps every kernel entry in exactly the little
 * functions below.
 *
 * The odd part, and the point of exercise s210: the NUMBER LIVES IN THE
 * OPCODE, not in a register. `SVC #25` assembles to the halfword 0xDF19
 * (0xDF = SVC, 0x19 = 25). The hardware does NOT hand the handler that
 * number — the handler must go dig it out of program memory:
 *     stacked pc            = the instruction AFTER the SVC
 *     stacked pc - 2        = the SVC opcode itself
 *     low byte of that      = the service number
 * Which is why the stacked frame matters again: the same eight words
 * the faults lesson read for forensics are here the SYSTEM CALL ABI —
 * arguments arrive in the stacked r0/r1 (AAPCS put them there before
 * the exception), and the handler returns a value by WRITING the
 * stacked r0: unstacking pops it straight into the caller's register.
 * faults.c edited the frame's pc to move the program; this lesson edits
 * the frame's r0 to answer it.
 *
 * Thread-side wrappers here use the register-pinned asm idiom
 * (`register int32_t a __asm("r0")`) that real syscall shims use —
 * newlib, musl, the Linux EABI all look like demo 2's functions. The
 * slides' bare `MOV %0,R0` after the SVC also "works", but only because
 * -O0 happens not to touch r0 in between; pinning makes the contract
 * explicit instead of lucky.
 *
 * One hard rule, proven at the end: SVC is SYNCHRONOUS and cannot wait.
 * Executing SVC while its own exception is active (inside the handler,
 * or in any handler at the same or lower priority) cannot preempt — so
 * the core escalates straight to HardFault. That closes the two s185
 * fault causes section 8 left open.
 *
 * PendSV, the other system-level service exception, has no instruction
 * at all — only the ICSR pend bit — and exists so an OS can say "do the
 * context switch LATER, when no interrupt is active". We already proved
 * it live: nvic.c round S demoted PendSV to 0xF0, pended it from inside
 * TIM2, and watched it tail-chain after — the offload/bottom-half
 * pattern. Its full context-switch job is the section 10 capstone,
 * which is also when PendSV_Handler's symbol moves to the scheduler.
 *
 * When you'll use this: any time firmware has a privilege wall — an
 * RTOS with the MPU on, a bootloader offering services to an app, a
 * TrustZone gateway — the call through the wall is this lesson's shape:
 * pin the arguments, SVC #n, kernel switches on n, answer comes back
 * through the stacked r0. And the crash you will one day debug: a
 * driver or RTOS API called from inside an interrupt handler that
 * hard-faults instantly — that is demo 4, the SVC-can't-preempt rule,
 * recognized on sight. */

#include "svc.h"

#include "faulthandler.h" /* struct FaultFrame + fault_print_frame */
#include "uart2.h"

#include <stdint.h>
#include <stdio.h>

#define SCB_SHCSR (*(volatile uint32_t*)0xE000ED24) /* CMSIS: SCB->SHCSR */
#define SHCSR_SVCALLPENDED (1UL << 15)

static uint32_t read_ipsr(void) {
    uint32_t value;
    __asm volatile("MRS %0, IPSR" : "=r"(value));
    return value;
}

/* ---- thread-side wrappers: the real syscall-shim idiom ----
 * Arguments pinned to r0/r1 so exception entry stacks them where the
 * handler expects; "+r"(a) says r0 is also the result — which the
 * handler plants in the frame and unstacking delivers. */

/* NOLINTBEGIN(bugprone-easily-swappable-parameters) — binary math ops,
 * same shape as every libc's add/sub prototypes */
static int32_t svc_add(int32_t lhs, int32_t rhs) {
    register int32_t reg0 __asm("r0") = lhs;
    register int32_t reg1 __asm("r1") = rhs;
    __asm volatile("SVC #36" : "+r"(reg0) : "r"(reg1) : "memory");
    return reg0;
}

static int32_t svc_sub(int32_t lhs, int32_t rhs) {
    register int32_t reg0 __asm("r0") = lhs;
    register int32_t reg1 __asm("r1") = rhs;
    __asm volatile("SVC #37" : "+r"(reg0) : "r"(reg1) : "memory");
    return reg0;
}

static int32_t svc_mul(int32_t lhs, int32_t rhs) {
    register int32_t reg0 __asm("r0") = lhs;
    register int32_t reg1 __asm("r1") = rhs;
    __asm volatile("SVC #38" : "+r"(reg0) : "r"(reg1) : "memory");
    return reg0;
}

static int32_t svc_div(int32_t lhs, int32_t rhs) {
    register int32_t reg0 __asm("r0") = lhs;
    register int32_t reg1 __asm("r1") = rhs;
    __asm volatile("SVC #39" : "+r"(reg0) : "r"(reg1) : "memory");
    return reg0;
}
/* NOLINTEND(bugprone-easily-swappable-parameters) */

/* ---- the handler (weak in startup, strong here) ---- */

void svc_body(struct FaultFrame* frame, uint32_t exc_return);

/* Same naked prologue as faults.c — the frame is the interface. */
__attribute__((naked)) void SVC_Handler(void) {
    __asm volatile("TST LR, #4    \n"
                   "ITE EQ        \n"
                   "MRSEQ R0, MSP \n"
                   "MRSNE R0, PSP \n"
                   "MOV R1, LR    \n"
                   "B svc_body    \n");
}

void svc_body(struct FaultFrame* frame, uint32_t exc_return) {
    /* Dig the opcode out of program memory, 2 bytes behind the
     * return address the frame carries. */
    uint16_t opcode = *(const uint16_t*)(frame->pc - 2U);

    if ((opcode >> 8) != 0xDFU) {
        /* demo 3 lands here: SVC pended by a register write, so the
         * instruction behind the stacked pc is NOT an SVC opcode */
        printf("  [SVC handler] ran, but opcode at pc-2 is 0x%04X — not 0xDFxx!\r\n",
               (unsigned)opcode);
        printf("  no instruction, no number: the contract needs `SVC #n`\r\n");
        return;
    }

    uint8_t number = (uint8_t)(opcode & 0xFFU);

    switch (number) {
    case 25: /* demo 1 — the verbose walk-through */
        printf("  [SVC handler] IPSR=%lu (exception 11)\r\n", (unsigned long)read_ipsr());
        fault_print_frame(frame, exc_return);
        printf("  opcode at stacked pc-2 (0x%08lx): 0x%04X → number %u\r\n",
               (unsigned long)(frame->pc - 2U), (unsigned)opcode, (unsigned)number);
        frame->r0 = number + 4U; /* the answer rides home in the frame */
        printf("  wrote %u into the frame's r0 — unstacking will deliver it\r\n",
               (unsigned)(number + 4U));
        break;
    case 36:
    case 37:
    case 38:
    case 39: { /* demo 2 — the calculator kernel */
        int32_t lhs = (int32_t)frame->r0;
        int32_t rhs = (int32_t)frame->r1;
        int32_t result = 0;
        switch (number) {
        case 36: result = lhs + rhs; break;
        case 37: result = lhs - rhs; break;
        case 38: result = lhs * rhs; break;
        case 39: result = lhs / rhs; break;
        default: break; /* unreachable — the outer case pinned 36..39 */
        }
        printf("  [SVC #%u] operands from the frame: %ld, %ld → %ld\r\n", (unsigned)number,
               (long)lhs, (long)rhs, (long)result);
        frame->r0 = (uint32_t)result;
        break;
    }
    case 99: /* demo 4 — break the rule on purpose */
        printf("  [SVC #99] now executing SVC #100 from INSIDE this handler...\r\n");
        __asm volatile("SVC #100");
        printf("  (never printed)\r\n");
        break;
    default:
        printf("  [SVC handler] unknown service %u — a real kernel returns -ENOSYS\r\n",
               (unsigned)number);
        break;
    }
}

/* --------------------------------------------------------------------- */

void playing_with_svc(void) {
    uart2_init();

    printf("\r\nLesson: SVC — the system-call door, opened from thread mode\r\n");

    /* 1 — exercise s210: number in, number+4 back */
    printf("\r\n1) SVC #25 — the handler must dig 25 out of the opcode, then\r\n");
    printf("   answer 29 through the stacked r0:\r\n");
    register uint32_t answer __asm("r0");
    __asm volatile("SVC #25" : "=r"(answer) : : "memory");
    printf("   thread mode received %lu — planted in the frame, popped into r0\r\n",
           (unsigned long)answer);

    /* 2 — exercise s211: a 4-function calculator kernel */
    printf("\r\n2) calculator by service number (36..39), operands in stacked\r\n");
    printf("   r0/r1 — the wrappers are the same shape as real syscall shims:\r\n");
    printf("   add(40,-90)  = %ld\r\n", (long)svc_add(40, -90));
    printf("   sub(25,150)  = %ld\r\n", (long)svc_sub(25, 150));
    printf("   mul(374,890) = %ld\r\n", (long)svc_mul(374, 890));
    printf("   div(67,-3)   = %ld\r\n", (long)svc_div(67, -3));

    /* 3 — slide 207's "uncommon method": pend SVC by register write */
    printf("\r\n3) the other trigger: SHCSR bit 15 (SVCALLPENDED) set by hand —\r\n");
    printf("   the handler runs, but behind its stacked pc sits no SVC opcode:\r\n");
    SCB_SHCSR |= SHCSR_SVCALLPENDED;
    printf("   that's why nobody uses it: the number travels IN the instruction\r\n");

    /* 4 — s185's leftovers: SVC cannot preempt itself */
    printf("\r\n4) the rule: SVC is synchronous and can't wait. SVC #99 asks the\r\n");
    printf("   handler to execute SVC #100 inside itself — same priority, can't\r\n");
    printf("   preempt, nowhere to go but HardFault (same fate for SVC in any\r\n");
    printf("   equal-or-lower-priority interrupt handler):\r\n");
    __asm volatile("SVC #99");

    /* never reached — the escalated HardFault parks in faulthandler.c */
    printf("   (never printed)\r\n");
}
