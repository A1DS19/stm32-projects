/* Lesson: the T bit (instructor's 006_t-bit; closes the slide 32-48 block).
 *
 * The Cortex-M4 executes exactly one instruction set: Thumb. The T bit
 * (EPSR bit 24) says "Thumb state", and on this core it must ALWAYS be 1.
 * We've brushed it three times already:
 *   - gdb showed xPSR = 0x01000000 — that 1 is T
 *   - coreregs: MRS reads the EPSR slice as zero, software can't see T
 *   - resetseq: vector[1] and &Reset_Handler were ODD addresses
 * That oddness is the mechanism: branch-to-register instructions (BX/BLX)
 * copy bit 0 of the target address into T. Bit 0 = 1 means "stay Thumb";
 * the address really used is the even one. The compiler quietly sets
 * bit 0 on every function address for exactly this reason.
 *
 * So: take a working function, clear bit 0 of its address, call it. BLX
 * copies the 0 into T, the core leaves its only legal state, and the
 * very next instruction fetch raises a usage fault (INVSTATE) — which
 * escalates to hard fault because usage faults aren't enabled yet (the
 * faults section changes that). The shared reporter in faulthandler.c
 * prints CFSR = 0x00020000: INVSTATE, exactly as decoded there.
 *
 * The instructor hardcodes a random even address (0x080001e8); we clear
 * bit 0 of a real function instead, so the demo works on any build.
 *
 * When you'll use this: any time an address is treated as code — function
 * pointers, jump tables, a bootloader's leap into the application, and
 * the forged task frames of the scheduler capstone (their fake xPSR must
 * carry T=1). Knowing this turns an INVSTATE fault from an afternoon of
 * confusion into a ten-second diagnosis. */

#include "tbit.h"

#include "uart2.h"

#include <stdint.h>
#include <stdio.h>

static void innocent_function(void) {
    printf("innocent_function ran — bit0 was 1, T stayed 1\r\n");
}

void playing_with_t_bit(void) {
    uart2_init();

    printf("\r\nLesson: the T bit\r\n");

    uintptr_t addr = (uintptr_t)&innocent_function;
    printf("&innocent_function = 0x%08lx  bit0=%lu, like every function\r\n", (unsigned long)addr,
           (unsigned long)(addr & 1U));

    void (*good_call)(void) = &innocent_function;
    good_call();

    /* same code, address with bit 0 forced to 0. volatile so the compiler
     * can't notice what we did and "fix" it */
    void (*volatile bad_call)(void) = (void (*)(void))(addr & ~(uintptr_t)1);
    printf("calling 0x%08lx — same code, bit0 cleared...\r\n",
           (unsigned long)(addr & ~(uintptr_t)1));
    bad_call();

    printf("never prints — the core never came back\r\n");
}
