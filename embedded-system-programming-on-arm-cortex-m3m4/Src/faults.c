/* Lesson: exception entry/exit & faults (slides 158-202).
 *
 * When an exception fires, the hardware itself — before one line of
 * handler code runs — pushes eight registers onto the stack: r0-r3,
 * r12, lr, pc, xpsr. That is the STACKED FRAME. It exists so the
 * interrupted code can be resumed exactly where it left off, but it has
 * a second career: the pc word in the frame is the address of the
 * instruction that faulted. Every crash dump, RTOS fault screen and
 * post-mortem log in embedded is code reading its own stacked frame,
 * exactly like this file does.
 *
 * The return address does NOT go to LR the way a function call's would.
 * LR instead receives EXC_RETURN, a magic 0xFFFFFFxx value; writing it
 * to PC is what triggers exception return (unstacking). Its low bits
 * describe the frame: bit 2 = which stack holds it (MSP or PSP), bit
 * 3 = thread or handler mode interrupted, bit 4 = whether the FPU's
 * S0-S15 were stacked too. Our build runs hard-float, so bit 4 is live
 * here — something the course's F407 project never shows.
 *
 * FAULTS are the exceptions the core raises at its own program:
 *   - UsageFault: a broken instruction stream — undefined opcode, T bit
 *     dropped (tbit.c), divide by zero if trapping is enabled (demo 3:
 *     by default the core quietly answers 0!).
 *   - BusFault: the bus said no — nothing answers at that address. It
 *     comes PRECISE (stacked pc = the guilty instruction) or IMPRECISE
 *     (the store sat in a write buffer while the core moved on, so the
 *     frame points somewhere LATER — the famous lying crash log).
 *   - MemManage: a memory-protection violation. No MPU is configured
 *     here, but executing from an eXecute-Never (XN) region raises it
 *     anyway — that rule is the architecture's, not the MPU's.
 *   - HardFault: the boss, fixed priority -1, always on. Any fault
 *     above ESCALATES to it when disabled or outranked — HFSR's FORCED
 *     bit says so. Ours lives in faulthandler.c, and parks.
 * The three configurable ones boot DISABLED; SHCSR bits 16/17/18 arm
 * them, per boot (demo 1).
 *
 * The trick the whole lesson runs on: a handler that WRITES to the
 * stacked frame edits the past. Change the stacked pc and "resume"
 * resumes somewhere else. Two recovery recipes below:
 *   - return-to-caller: pc := stacked lr & ~1 — the stacked lr still
 *     holds the doomed call's return address. For jumps into garbage.
 *   - skip-the-instruction: pc += 2 or 4 (a first halfword of
 *     0b11101/0b11110/0b11111 opens a 32-bit Thumb opcode; anything
 *     else is 16-bit). For div-by-zero and precise bus errors.
 * The scheduler capstone is this trick industrialized: forge whole
 * frames from scratch and "return" into functions nobody called.
 *
 * When you'll use this: the day your product crashes in the field. A
 * handler that logs CFSR + the stacked frame is standard equipment in
 * every shipping firmware; these are the canonical shape (TST LR,#4
 * prologue and all — FreeRTOS's looks the same). Reading one: stacked
 * pc = the crime scene, CFSR = the charge sheet, addr2line on the pc =
 * the guilty source line. And know the one lie: IMPRECISERR means the
 * pc drifted past the culprit — look BEHIND it for a store to a bad
 * address. */

#include "faults.h"

#include "faulthandler.h"
#include "uart2.h"

#include <stdint.h>
#include <stdio.h>

/* SCB — the fault plumbing (CMSIS dialect: SCB->SHCSR, SCB->CCR, SCB->CFSR) */
#define SCB_SHCSR (*(volatile uint32_t*)0xE000ED24) /* system handler enables  */
#define SCB_CCR (*(volatile uint32_t*)0xE000ED14)   /* configuration & control */
#define SCB_CFSR (*(volatile uint32_t*)0xE000ED28)  /* UFSR | BFSR | MMFSR     */

#define SHCSR_MEMFAULTENA (1UL << 16)
#define SHCSR_BUSFAULTENA (1UL << 17)
#define SHCSR_USGFAULTENA (1UL << 18)
#define CCR_DIV_0_TRP (1UL << 4)

#define UFSR_UNDEFINSTR (1UL << 16) /* positions within the whole CFSR word */
#define BFSR_PRECISERR (1UL << 9)

/* How long is the Thumb instruction at addr? First halfword
 * 0b11101/0b11110/0b11111 opens a 32-bit encoding, the rest are 16-bit. */
static uint32_t instruction_size(uint32_t addr) {
    uint16_t first = *(const uint16_t*)addr;
    return ((first >> 11) >= 0x1DU) ? 4U : 2U;
}

/* ---- the three configurable handlers (weak in startup, strong here;
 * HardFault stays in faulthandler.c — single-strong-symbol rule) ----
 *
 * Each is naked — the compiler must not grow a prologue that moves SP
 * before we capture it — and each opens with the canonical four lines:
 * EXC_RETURN bit 2 picks the stack pointer that holds the frame, then
 * frame + EXC_RETURN travel to a C body as r0/r1 = the two arguments.
 * The C body RETURNS, unlike HardFault: LR still holds EXC_RETURN, so
 * the body's ordinary function return IS the exception return. */

void usagefault_body(struct FaultFrame* frame, uint32_t exc_return);
void busfault_body(struct FaultFrame* frame, uint32_t exc_return);
void memfault_body(struct FaultFrame* frame, uint32_t exc_return);

__attribute__((naked)) void UsageFault_Handler(void) {
    __asm volatile("TST LR, #4         \n"
                   "ITE EQ             \n"
                   "MRSEQ R0, MSP      \n"
                   "MRSNE R0, PSP      \n"
                   "MOV R1, LR         \n"
                   "B usagefault_body  \n");
}

__attribute__((naked)) void BusFault_Handler(void) {
    __asm volatile("TST LR, #4         \n"
                   "ITE EQ             \n"
                   "MRSEQ R0, MSP      \n"
                   "MRSNE R0, PSP      \n"
                   "MOV R1, LR         \n"
                   "B busfault_body    \n");
}

__attribute__((naked)) void MemManage_Handler(void) {
    __asm volatile("TST LR, #4         \n"
                   "ITE EQ             \n"
                   "MRSEQ R0, MSP      \n"
                   "MRSNE R0, PSP      \n"
                   "MOV R1, LR         \n"
                   "B memfault_body    \n");
}

void usagefault_body(struct FaultFrame* frame, uint32_t exc_return) {
    uint32_t cause = SCB_CFSR;

    printf("  [UsageFault handler]\r\n");
    fault_print_cfsr();
    fault_print_frame(frame, exc_return);
    SCB_CFSR = 0xFFFFUL << 16; /* sticky bits: write 1 to clear our UFSR byte */

    if (cause & UFSR_UNDEFINSTR) {
        /* pc points INTO the garbage — skipping would fault again */
        frame->pc = frame->lr & ~1UL;
        printf("  recovery: pc := stacked lr & ~1 — back to the caller\r\n");
    } else {
        uint32_t size = instruction_size(frame->pc);
        frame->pc += size;
        printf("  recovery: skip the %lu-byte instruction at the stacked pc\r\n",
               (unsigned long)size);
    }
}

void busfault_body(struct FaultFrame* frame, uint32_t exc_return) {
    uint32_t cause = SCB_CFSR;

    printf("  [BusFault handler]\r\n");
    fault_print_cfsr();
    fault_print_frame(frame, exc_return);
    SCB_CFSR = 0xFFUL << 8;

    if (cause & BFSR_PRECISERR) {
        frame->pc += instruction_size(frame->pc);
        printf("  recovery: precise — skip the faulting access\r\n");
    } else {
        /* imprecise: the pc names no culprit; touching it would frame
         * an innocent instruction. Just resume. */
        printf("  recovery: imprecise — pc left alone, just resume\r\n");
    }
}

void memfault_body(struct FaultFrame* frame, uint32_t exc_return) {
    printf("  [MemManage handler]\r\n");
    fault_print_cfsr();
    fault_print_frame(frame, exc_return);
    SCB_CFSR = 0xFFUL;

    /* we jumped into an XN region: abandon the doomed call */
    frame->pc = frame->lr & ~1UL;
    printf("  recovery: pc := stacked lr & ~1 — back to the caller\r\n");
}

/* --------------------------------------------------------------------- */

void playing_with_faults(void) {
    uart2_init();

    printf("\r\nLesson: exception entry/exit & faults — reading the crash back\r\n");

    /* 1 — arm the three configurable faults (disabled every boot) */
    printf("\r\n1) SHCSR before: 0x%08lx — MemManage/BusFault/UsageFault boot\r\n",
           (unsigned long)SCB_SHCSR);
    SCB_SHCSR |= SHCSR_USGFAULTENA | SHCSR_BUSFAULTENA | SHCSR_MEMFAULTENA;
    printf("   disabled (nvic.c armed them too — per boot, gone at reset).\r\n");
    printf("   After setting bits 16/17/18: 0x%08lx. Until now every fault\r\n",
           (unsigned long)SCB_SHCSR);
    printf("   escalated to HardFault; from here each goes to its own handler\r\n");

    /* 2 — usage fault: undefined instruction (exercise s186, cause 1) */
    static uint32_t garbage[2] = {0xFFFFFFFFUL, 0xFFFFFFFFUL}; /* .data → SRAM */
    void (*bad_code)(void) = (void (*)(void))((uintptr_t)garbage | 1UL);
    printf("\r\n2) calling into an SRAM array of 0xFFFFFFFF at %p (bit 0 set, so\r\n",
           (void*)garbage);
    printf("   the T bit survives — no INVSTATE; the core fetches from SRAM\r\n");
    printf("   fine, then meets an opcode it doesn't know):\r\n");
    bad_code();
    printf("   ...and we're BACK — the handler rewrote the stacked pc\r\n");

    /* 3 — usage fault: divide by zero (exercise s186, cause 2) */
    volatile int32_t num = 10;
    volatile int32_t den = 0;
    volatile int32_t quotient = 1234;
    printf("\r\n3) divide by zero, two personalities. Untrapped: 10/0 = ");
    quotient = num / den;
    printf("%ld — the core QUIETLY answers zero, no fault. Now set\r\n", (long)quotient);
    printf("   CCR.DIV_0_TRP and divide again:\r\n");
    SCB_CCR |= CCR_DIV_0_TRP;
    quotient = num / den;
    printf("   back; quotient=%ld — the skipped SDIV never ran, the result\r\n", (long)quotient);
    printf("   register kept whatever it held (garbage in, honestly reported)\r\n");

    /* 4 — mem manage fault: execute from the peripheral region (cause 3) */
    void (*evil)(void) = (void (*)(void))(uintptr_t)0x40000001UL;
    printf("\r\n4) calling 0x40000000 — the peripheral region is eXecute-Never by\r\n");
    printf("   architecture (no MPU here): registers must never become code\r\n");
    printf("   (that's how code-injection through a peripheral is shut out):\r\n");
    evil();
    printf("   ...back again. MMARVALID stayed 0 — for instruction-side\r\n");
    printf("   violations the stacked pc is the witness, not MMFAR\r\n");

    /* 5 — bus fault, precise: a load past the end of SRAM1. (Two
     * gentler candidates DON'T error on the L476 — FMC space at
     * 0x60000000 and un-clocked TIM2 both quietly read 0. Past the
     * last real SRAM word, the bus matrix has no slave to pick.) */
    printf("\r\n5) READING 0x20018000 — one word past SRAM1's 96K: no memory\r\n");
    printf("   answers, the bus errors. A load must stall until its data\r\n");
    printf("   arrives, so the fault is PRECISE and BFAR names the address:\r\n");
    volatile uint32_t loot = *(volatile uint32_t*)0x20018000UL;
    printf("   back; the \"loaded\" value %08lx is junk — the skipped LDR never\r\n",
           (unsigned long)loot);
    printf("   delivered\r\n");

    /* 6 — bus fault, imprecise: a store to the same nowhere */
    uint32_t store_site;
    printf("\r\n6) WRITING 0x20018000 — stores go through a WRITE BUFFER: the core\r\n");
    printf("   moves on before the bus answers, so the fault lands LATE:\r\n");
    __asm volatile("MOV %0, PC" : "=r"(store_site));
    *(volatile uint32_t*)0x20018000UL = 0xDEADBEEFUL;
    printf("   the store sat near 0x%08lx — the frame's pc above is PAST it,\r\n",
           (unsigned long)store_site);
    printf("   and BFARVALID stayed 0 (the buffer lost the address). This is\r\n");
    printf("   why crash logs lie: IMPRECISERR = look BEHIND the stacked pc\r\n");

    /* 7 — escalation: disarm UsageFault and fault again (slides 170-172) */
    printf("\r\n7) escalation: clear USGFAULTENA and divide by zero once more —\r\n");
    printf("   the fault still happens, but its handler is off, so it has\r\n");
    printf("   nowhere to go but UP. HFSR.FORCED will say exactly that:\r\n");
    SCB_SHCSR &= ~SHCSR_USGFAULTENA;
    quotient = num / den;

    /* never reached — the escalated HardFault parks in faulthandler.c */
    printf("   (never printed: %ld)\r\n", (long)quotient);
}
