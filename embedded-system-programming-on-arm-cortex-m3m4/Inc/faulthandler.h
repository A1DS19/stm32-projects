#ifndef FAULTHANDLER_H
#define FAULTHANDLER_H

#include <stdint.h>

/* The eight words the core pushes on every exception entry, lowest
 * address first — the "stacked frame" the faults lesson reads back.
 * (A floating-point-active frame appends S0-S15 + FPSCR after these,
 * so this prefix view is safe for either layout.) */
struct FaultFrame {
    uint32_t r0, r1, r2, r3, r12, lr, pc, xpsr;
};

/* Shared reporters in faulthandler.c — used by HardFault_Handler there
 * and by the configurable-fault handlers in faults.c. */
void fault_print_frame(const struct FaultFrame* frame, uint32_t exc_return);
void fault_print_cfsr(void);

#endif
