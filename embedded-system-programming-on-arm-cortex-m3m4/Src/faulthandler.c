/* Shared hard-fault reporter. Only ONE strong HardFault_Handler may exist
 * in the build (it overrides the startup file's weak default), and several
 * lessons want to crash on purpose — so the reporter lives here, and every
 * lesson shares it.
 *
 * It prints CFSR, the Configurable Fault Status Register — the core's own
 * note about WHY it faulted (the faults section reads it properly; here we
 * decode just our two lessons' causes). Reading CFSR and ISER0 from inside
 * the handler is itself the access-levels proof: these are System Control
 * Space reads, and they work here even when CONTROL.nPRIV=1 — handler mode
 * is always privileged.
 *
 * It parks the core on purpose: returning from a hard fault would mostly
 * re-run the instruction that faulted. Reset the board to move on. */

#include <stdint.h>
#include <stdio.h>

void HardFault_Handler(void) {
    volatile uint32_t* cfsr = (uint32_t*)0xE000ED28;
    volatile uint32_t* iser0 = (uint32_t*)0xE000E100;

    printf("\r\nHARD FAULT (escalated) — CFSR=0x%08lx\r\n", (unsigned long)*cfsr);
    printf("  0x00020000 = INVSTATE: T bit went 0\r\n");
    printf("  0x00008200 = precise bus error: nPAL touched the SCS\r\n");
    printf("handler mode is privileged anyway: ISER0=0x%lx (read OK)\r\n", (unsigned long)*iser0);
    printf("parked — reset the board\r\n");
    for (;;) {}
}
