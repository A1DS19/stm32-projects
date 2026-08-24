/* Lesson: access levels — PAL vs nPAL (slide 32, instructor's
 * 005_access_levels).
 *
 * Orthogonal to thread/handler mode, code runs at one of two access levels:
 *   PAL  (privileged)     — can touch everything, including the core's own
 *                           registers in the System Control Space (NVIC,
 *                           SCB, SysTick — the 0xE000_xxxx block)
 *   nPAL (non-privileged) — locked out of that block; a touch there comes
 *                           back as a bus error, which escalates to a
 *                           HARD FAULT because we haven't enabled the bus
 *                           fault handler yet (that's the faults section)
 *
 * The CONTROL core register decides: bit 0 (nPRIV) = 0 privileged,
 * 1 unprivileged. main() starts privileged. The catch that makes this a
 * security boundary: writing CONTROL is itself privileged. Once thread
 * mode drops to nPAL it CANNOT lift itself back — MSR CONTROL from nPAL
 * is silently ignored. The only road back is an exception: handler mode
 * is ALWAYS privileged, whatever CONTROL says. That one-way door is the
 * seed of every OS's user-space/kernel split (and of the SVC lesson).
 *
 * The demo, in order:
 *   1. privileged: pend IRQ 4 through the NVIC — works, ISR prints
 *   2. drop to nPAL (MRS/modify/MSR on CONTROL + ISB)
 *   3. same NVIC touch again — hard fault; the fault handler proves it
 *      runs privileged by reading the NVIC register nPAL couldn't, then
 *      parks the core (returning would just re-run the faulting access)
 *
 * modes.c already owns the IRQ 3 line (RTC_WKUP), so this lesson borrows
 * the next line of the vector table: IRQ 4, the flash controller. Same
 * trick — pend it by number, never touch the actual flash peripheral.
 *
 * Seen on hardware: after the drop CONTROL prints 5, not 1. That's bit 2
 * (FPCA, "floating-point context active") riding along — this project
 * compiles hard-float, and the first printf touched FPU registers, so the
 * core flagged an FP context. 5 = FPCA + nPRIV; bit 1 (SPSEL) is still 0
 * because thread mode still runs on MSP — SPSEL is the bit the stack
 * lesson flips, and FPCA is why exception stack frames come in two sizes.
 *
 * Expected serial tail: the ISR line, CONTROL=5, the hard fault report —
 * and "after" never prints. Reset the board when done.
 *
 * When you'll use this: building the user/kernel wall. An RTOS drops its
 * tasks to nPAL so a buggy task can't reprogram the NVIC, the clocks, or
 * another task out of existence — containment instead of chaos. It also
 * decodes a whole family of mystery crashes: "worked in main(), faults
 * inside my task" is usually privilege, and this lesson is the
 * diagnosis. */

#include "access.h"

#include "uart2.h"

#include <stdint.h>
#include <stdio.h>

#define ISER0 ((volatile uint32_t*)0xE000E100)

static uint32_t read_control(void) {
    uint32_t control;
    __asm volatile("MRS %0, CONTROL" : "=r"(control));
    return control;
}

/* Privileged-only write. The ISB afterwards is required by ARM: it makes
 * sure no later instruction slips through under the old access level. */
static void drop_to_unprivileged(void) {
    uint32_t control = read_control();
    control |= 1U; /* nPRIV = 1 */
    __asm volatile("MSR CONTROL, %0" : : "r"(control) : "memory");
    __asm volatile("ISB");
}

static void pend_flash_irq(void) {
    volatile uint32_t* stir = (uint32_t*)0xE000EF00;

    *ISER0 |= (1U << 4); /* unmask IRQ 4 — faults here once in nPAL */
    *stir = (4U & 0x1FFU);
}

/* THREAD mode, starts privileged. */
void playing_with_access_levels(void) {
    uart2_init();

    printf("\r\nLesson: access levels (PAL vs nPAL)\r\n");
    printf("CONTROL=%lu -> privileged thread mode\r\n", (unsigned long)read_control());

    printf("privileged: pending IRQ 4 through the NVIC...\r\n");
    pend_flash_irq();

    drop_to_unprivileged();
    printf("CONTROL=%lu -> unprivileged thread mode\r\n", (unsigned long)read_control());

    printf("unprivileged: same NVIC touch again...\r\n");
    pend_flash_irq();

    printf("after — never prints, the fault handler parked the core\r\n");
}

/* HANDLER mode. */
void FLASH_IRQHandler(void) {
    printf("handler mode: FLASH ISR ran\r\n");
}

/* The hard-fault reporter this lesson relied on now lives in
 * Src/faulthandler.c — one strong HardFault_Handler for all lessons. Its
 * SCS reads (CFSR, ISER0) carry this lesson's proof: handler mode stays
 * privileged even with CONTROL.nPRIV=1. */
