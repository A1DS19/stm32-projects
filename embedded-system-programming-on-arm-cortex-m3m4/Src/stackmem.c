/* Lesson: stack memory (slides 79-117).
 *
 * The stack is plain RAM used through one pointer (SP, alias R13) with a
 * last-in-first-out discipline: PUSH stores and moves SP, POP undoes it.
 * ARM Cortex-M is a FULL DESCENDING stack — "descending": pushes move SP
 * toward LOWER addresses; "full": SP points AT the last pushed item, not
 * at the next free slot. (The other three models — full ascending, empty
 * ascending, empty descending — exist on other CPUs; slide 85 draws all
 * four.) Our linker script uses the classic placement: stack starts at
 * the very top of SRAM1 (_estack = 0x2001_8000) and burrows downward,
 * heap grows up from below — they meet only in an out-of-memory crash.
 *
 * The banked pointer, finally cashed in: R13 is really TWO registers.
 *   MSP — main stack pointer. Everyone's default from reset (the core
 *         loads it from vector[0] — reset-sequence lesson), and the ONLY
 *         stack handler mode ever uses.
 *   PSP — process stack pointer. Parked at 0 since reset (the coreregs
 *         lesson printed it) until somebody banks thread mode onto it:
 *         load PSP via MSR, then set CONTROL bit 1 (SPSEL).
 * The switch must happen in a "naked" function — one the compiler is
 * told not to give a prologue/epilogue, because those touch the stack
 * through the very pointer we're swapping mid-function.
 *
 * Why a second pointer at all: an RTOS gives every task its own little
 * stack (PSP points into the current task's), while the kernel and every
 * interrupt run on MSP. Corrupt a task stack and the kernel still stands.
 * That is the seed of the scheduler capstone at the end of this course.
 *
 * AAPCS — the function-call rulebook the compiler follows (Procedure
 * Call Standard for the Arm Architecture):
 *   R0-R3   carry the first four arguments in; R0 (R1 for 64-bit)
 *           carries the result back
 *   R0-R3, R12, LR, PSR — "caller-saved": a function may trash them
 *           freely; if the caller still wants them, saving is its problem
 *   R4-R11  "callee-saved": a function that borrows them must put the
 *           original values back before returning — proven live below
 * When an interrupt fires, hardware itself pushes the caller-saved set
 * (R0-R3, R12, LR, PC, xPSR — the 8-word "stack frame") onto the ACTIVE
 * stack, which is why a plain C function works as a handler. The full
 * frame anatomy is section 8's story.
 *
 * When you'll use this: the moment firmware grows beyond one loop. The
 * split proven here — tasks on PSP, kernel and interrupts on MSP — is
 * the load-bearing wall of FreeRTOS, Zephyr, and the scheduler capstone;
 * their port.c is this lesson's code with the serial numbers filed off.
 * It's daily bread for debugging too: sizing task stacks, reading
 * overflows, and walking fault frames all start from the full-descending
 * model proven at the top of this file. */

#include "stackmem.h"

#include "uart2.h"

#include <stdint.h>
#include <stdio.h>

extern uint32_t _estack; /* top of SRAM1, from stm32l476xg_flash.ld */

/* the process stack we hand to PSP: 2 KB of our own .bss, 8-byte aligned
 * as AAPCS demands (printf with hard-float FPU state is stack-hungry) */
static uint8_t process_stack[2048] __attribute__((aligned(8)));

static volatile int handler_report_done;

#define DEFINE_SPECIAL_READER(fn, reg)                                                             \
    static uint32_t fn(void) {                                                                     \
        uint32_t value;                                                                            \
        __asm volatile("MRS %0, " reg : "=r"(value));                                              \
        return value;                                                                              \
    }

DEFINE_SPECIAL_READER(read_msp, "MSP")
DEFINE_SPECIAL_READER(read_psp, "PSP")
DEFINE_SPECIAL_READER(read_control, "CONTROL")

/* Naked = no compiler prologue/epilogue: nothing may touch the stack
 * while we swap the pointer under our own feet. The new stack top
 * arrives in R0 (AAPCS argument rule, used deliberately). ISB flushes
 * the pipeline so everything after runs with the new SP selection. */
__attribute__((naked)) static void change_sp_to_psp(uint32_t top __attribute__((unused))) {
    /* `top` arrives as R0 and is consumed by the first instruction */
    __asm volatile("MSR PSP, R0        \n"
                   "MRS R1, CONTROL    \n"
                   "ORR R1, R1, #2     \n" /* CONTROL.SPSEL = 1 */
                   "MSR CONTROL, R1    \n"
                   "ISB                \n"
                   "BX  LR             \n");
}

/* full-descending, proven: push one word inside a single asm block and
 * look at where SP went and what it points at */
static void prove_full_descending(void) {
    uint32_t sentinel = 0xF00DF00D;
    uint32_t sp_before;
    uint32_t sp_after;
    uint32_t slot;
    uint32_t popped;

    __asm volatile("MRS  %0, MSP     \n"
                   "PUSH {%4}        \n"
                   "MRS  %1, MSP     \n"
                   "LDR  %2, [%1]    \n"
                   "POP  {%3}        \n"
                   : "=&r"(sp_before), "=&r"(sp_after), "=&r"(slot), "=&r"(popped)
                   : "r"(sentinel));

    printf("\r\nfull descending, live: SP 0x%08lx --PUSH--> 0x%08lx (down 4)\r\n",
           (unsigned long)sp_before, (unsigned long)sp_after);
    printf("  [SP] holds 0x%08lx — SP aims AT the pushed word (\"full\");\r\n",
           (unsigned long)slot);
    printf("  POP handed back 0x%08lx and SP returned home\r\n", (unsigned long)popped);
}

static uint32_t local_address_one_call_down(void) {
    uint32_t probe = 0;
    return (uint32_t)(uintptr_t)&probe + probe;
}

/* runs after the switch: its stack frame must land inside process_stack */
static void living_on_the_process_stack(void) {
    uint32_t local = 0;
    uint32_t local_addr = (uint32_t)(uintptr_t)&local;
    uint32_t start = (uint32_t)(uintptr_t)&process_stack[0];
    uint32_t top = start + sizeof(process_stack);

    printf("\r\non the process stack now: this local lives @0x%08lx,\r\n",
           (unsigned long)local_addr);
    printf("  inside OUR array [0x%08lx..0x%08lx) — the stack is wherever\r\n",
           (unsigned long)start, (unsigned long)top);
    printf("  PSP points. MSP sits frozen at 0x%08lx, untouched by calls\r\n",
           (unsigned long)read_msp());
}

/* IRQ 6 (EXTI0), pended from software: handler mode ignores SPSEL and
 * always runs on MSP — its local proves it by address */
void EXTI0_IRQHandler(void) {
    uint32_t handler_local = 0;
    uint32_t addr = (uint32_t)(uintptr_t)&handler_local + handler_local;

    printf("\r\n[EXTI0 handler] my local @0x%08lx — MSP territory, not the array\r\n",
           (unsigned long)addr);
    printf("[EXTI0 handler] CONTROL reads %lu: SPSEL shows 0 here — handler mode\r\n",
           (unsigned long)read_control());
    printf("[EXTI0 handler] rides MSP no matter what thread mode selected\r\n");
    handler_report_done = 1;
}

/* AAPCS callee-saved, proven live: plant a value in R4, make printf do
 * heavy work (it certainly borrows R4-R11 internally), read R4 after */
static void prove_callee_saved(void) {
    register uint32_t keeper __asm("r4") = 0xCAFED00D;

    printf("\r\nAAPCS: planted 0x%08lx in R4, then made this very printf run\r\n",
           (unsigned long)keeper);
    printf("R4 afterwards = 0x%08lx — printf borrowed it but restored it,\r\n",
           (unsigned long)keeper);
    printf("  as callee-saved rules demand. R0-R3/R12/LR get no such mercy\r\n");
}

void playing_with_stack_memory(void) {
    volatile uint32_t* iser0 = (uint32_t*)0xE000E100; /* NVIC enable    */
    volatile uint32_t* stir = (uint32_t*)0xE000EF00;  /* pend-by-number */
    uint32_t here = (uint32_t)(uintptr_t)&here;

    uart2_init();

    printf("\r\nLesson: stack memory — one pointer, two registers behind it\r\n");
    printf("_estack = 0x%08lx (top of SRAM1; linker script), stack digs down\r\n",
           (unsigned long)(uintptr_t)&_estack);
    printf("MSP = 0x%08lx  PSP = 0x%08lx  CONTROL = %lu\r\n", (unsigned long)read_msp(),
           (unsigned long)read_psp(), (unsigned long)read_control());
    printf("  PSP still 0 since reset — the coreregs lesson's parked register\r\n");

    prove_full_descending();

    printf("\r\ndescending frames: a local here @0x%08lx, one call deeper @0x%08lx\r\n",
           (unsigned long)here, (unsigned long)local_address_one_call_down());

    printf("\r\nbanking thread mode onto the process stack (naked MSR PSP + SPSEL)...\r\n");
    change_sp_to_psp((uint32_t)(uintptr_t)&process_stack[0] + sizeof(process_stack));
    printf("done: CONTROL = %lu (bit1 SPSEL=1; bit2 FPCA is printf's FPU flag),\r\n",
           (unsigned long)read_control());
    printf("  PSP = 0x%08lx and falling with every call; MSP parked\r\n",
           (unsigned long)read_psp());

    living_on_the_process_stack();

    *iser0 |= 1U << 6U; /* unmask IRQ 6 (EXTI0)              */
    *stir = 6;          /* pend it right now, from software  */
    while (!handler_report_done) {}

    prove_callee_saved();

    printf("\r\nthread on PSP + handlers on MSP = every RTOS's floor plan;\r\n");
    printf("the scheduler capstone builds exactly on this switch\r\n");
}
