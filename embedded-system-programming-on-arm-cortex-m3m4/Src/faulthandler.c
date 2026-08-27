/* Shared fault reporting. Only ONE strong HardFault_Handler may exist
 * in the build (it overrides the startup file's weak default), and
 * several lessons crash on purpose — so the reporter lives here and
 * everyone shares it. The faults lesson (Src/faults.c) upgraded this
 * file from a two-constant stub into the real thing: a naked handler
 * that finds the stacked frame, plus the named-bit CFSR decode below,
 * both exported through Inc/faulthandler.h so the configurable-fault
 * handlers can reuse them.
 *
 * CFSR, the Configurable Fault Status Register, is the core's own note
 * about WHY it faulted: three status bytes packed into one word — usage
 * (UFSR, top half), bus (BFSR, byte 1), memory management (MMFSR,
 * byte 0). Every bit is sticky, "write 1 to clear": it stays set until
 * software clears it, so stale causes pile up across faults unless each
 * handler cleans its byte after decoding.
 *
 * Reading CFSR and ISER0 from inside the handler is itself the
 * access-levels proof: these are System Control Space reads, and they
 * work here even when CONTROL.nPRIV=1 — handler mode is always
 * privileged.
 *
 * HardFault still parks the core on purpose: returning would mostly
 * re-run the instruction that faulted. Reset the board to move on.
 * (The configurable handlers in faults.c DO return — by editing the
 * stacked frame first. That's that lesson's whole trick.) */

#include "faulthandler.h"

#include <stdint.h>
#include <stdio.h>

#define SCB_CFSR (*(volatile uint32_t*)0xE000ED28)  /* UFSR | BFSR | MMFSR        */
#define SCB_HFSR (*(volatile uint32_t*)0xE000ED2C)  /* hard fault: FORCED/VECTTBL */
#define SCB_MMFAR (*(volatile uint32_t*)0xE000ED34) /* faulting DATA address (mem) */
#define SCB_BFAR (*(volatile uint32_t*)0xE000ED38)  /* faulting DATA address (bus) */

#define CFSR_MMARVALID (1UL << 7)
#define CFSR_BFARVALID (1UL << 15)

/* Every architected CFSR bit by name — the crash-log decoder ring. */
static const struct {
    uint32_t bit;
    const char* name;
    const char* meaning;
} CFSR_BITS[] = {
    /* MMFSR — memory management faults, bits 0-7 */
    {1UL << 0, "IACCVIOL", "instruction fetch from a no-execute region"},
    {1UL << 1, "DACCVIOL", "data access the MPU forbids"},
    {1UL << 3, "MUNSTKERR", "MPU violation while unstacking on exception exit"},
    {1UL << 4, "MSTKERR", "MPU violation while stacking on exception entry"},
    {1UL << 5, "MLSPERR", "MPU violation during lazy FPU state save"},
    {CFSR_MMARVALID, "MMARVALID", "MMFAR holds the faulting address"},
    /* BFSR — bus faults, bits 8-15 */
    {1UL << 8, "IBUSERR", "bus error on an instruction fetch"},
    {1UL << 9, "PRECISERR", "bus error, PRECISE — the stacked pc is the culprit"},
    {1UL << 10, "IMPRECISERR", "bus error, IMPRECISE — the stacked pc has moved on"},
    {1UL << 11, "UNSTKERR", "bus error while unstacking on exception exit"},
    {1UL << 12, "STKERR", "bus error while stacking on exception entry"},
    {1UL << 13, "LSPERR", "bus error during lazy FPU state save"},
    {CFSR_BFARVALID, "BFARVALID", "BFAR holds the faulting address"},
    /* UFSR — usage faults, bits 16-31 */
    {1UL << 16, "UNDEFINSTR", "opcode the core doesn't know"},
    {1UL << 17, "INVSTATE", "T bit went 0 (tbit.c's whole lesson)"},
    {1UL << 18, "INVPC", "broken EXC_RETURN / forged frame on exception return"},
    {1UL << 19, "NOCP", "coprocessor (FPU) instruction while the FPU is off"},
    {1UL << 24, "UNALIGNED", "unaligned access with trapping enabled"},
    {1UL << 25, "DIVBYZERO", "divide by zero with CCR.DIV_0_TRP enabled"},
};

void fault_print_cfsr(void) {
    uint32_t cfsr = SCB_CFSR;

    printf("  CFSR=0x%08lx (UFSR|BFSR|MMFSR):\r\n", (unsigned long)cfsr);
    for (unsigned i = 0; i < sizeof(CFSR_BITS) / sizeof(CFSR_BITS[0]); i++) {
        if (cfsr & CFSR_BITS[i].bit) {
            printf("    %s — %s\r\n", CFSR_BITS[i].name, CFSR_BITS[i].meaning);
        }
    }
    /* The address registers only mean something when their VALID bit
     * says so — reading them blind is how crash logs grow red herrings. */
    if (cfsr & CFSR_MMARVALID) {
        printf("    MMFAR=0x%08lx\r\n", (unsigned long)SCB_MMFAR);
    }
    if (cfsr & CFSR_BFARVALID) {
        printf("    BFAR=0x%08lx\r\n", (unsigned long)SCB_BFAR);
    }
}

void fault_print_frame(const struct FaultFrame* frame, uint32_t exc_return) {
    printf("  stacked frame @ %p:\r\n", (const void*)frame);
    printf("    r0=%08lx r1=%08lx r2=%08lx r3=%08lx\r\n", (unsigned long)frame->r0,
           (unsigned long)frame->r1, (unsigned long)frame->r2, (unsigned long)frame->r3);
    printf("    r12=%08lx lr=%08lx pc=%08lx xpsr=%08lx\r\n", (unsigned long)frame->r12,
           (unsigned long)frame->lr, (unsigned long)frame->pc, (unsigned long)frame->xpsr);
    printf("  EXC_RETURN=0x%08lx: %s frame, was in %s mode on %s\r\n", (unsigned long)exc_return,
           (exc_return & (1UL << 4)) ? "basic 8-word" : "extended (FPU regs too)",
           (exc_return & (1UL << 3)) ? "thread" : "handler",
           (exc_return & (1UL << 2)) ? "PSP" : "MSP");
}

void hardfault_report(const struct FaultFrame* frame, uint32_t exc_return);

/* Naked: the compiler must not touch SP before we capture it.
 * EXC_RETURN bit 2 says which stack pointer holds the frame — the same
 * four-line prologue every RTOS fault handler opens with. */
__attribute__((naked)) void HardFault_Handler(void) {
    __asm volatile("TST LR, #4          \n"
                   "ITE EQ              \n"
                   "MRSEQ R0, MSP       \n"
                   "MRSNE R0, PSP       \n"
                   "MOV R1, LR          \n"
                   "B hardfault_report  \n");
}

void hardfault_report(const struct FaultFrame* frame, uint32_t exc_return) {
    uint32_t hfsr = SCB_HFSR;

    printf("\r\nHARD FAULT — HFSR=0x%08lx\r\n", (unsigned long)hfsr);
    if (hfsr & (1UL << 30)) {
        printf("    FORCED — a disabled/outranked configurable fault escalated here\r\n");
    }
    if (hfsr & (1UL << 1)) {
        printf("    VECTTBL — bus error fetching a vector\r\n");
    }
    fault_print_cfsr();
    fault_print_frame(frame, exc_return);
    printf("  handler mode is privileged anyway: ISER0=0x%lx (read OK)\r\n",
           (unsigned long)*(volatile uint32_t*)0xE000E100);
    printf("parked — reset the board\r\n");
    for (;;) {}
}
