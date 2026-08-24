/* Lesson: operation modes — thread mode vs handler mode (slides 32-33,
 * instructor's 003_operation_modes).
 *
 * The Cortex-M4 core is always in one of two modes:
 *   thread mode  — where normal code runs; main() starts here
 *   handler mode — where the core jumps when an interrupt or exception
 *                  fires; interrupt service routines (ISRs) run here
 *
 * You can't call your way into handler mode — only an interrupt gets you
 * there, and returning from the ISR puts you back. To show the switch we
 * fire an interrupt from software: no timer, no button, we just ask the
 * NVIC (the core's interrupt controller) to pretend IRQ 3 happened.
 *
 * Two NVIC registers do it, both in the core's private peripheral region
 * (0xE000_0000 up — same block the memory-map lesson calls PPB):
 *   ISER0 (0xE000_E100) — interrupt set-enable: bit n = 1 unmasks IRQ n
 *   STIR  (0xE000_EF00) — software trigger: writing n pends IRQ n
 * Raw addresses on purpose: this section teaches where these live. The
 * CMSIS names for them come later in the course.
 *
 * How we SEE the mode: the IPSR core register holds the number of the
 * exception being served — 0 in thread mode, 16+IRQ in handler mode
 * (IRQ 3 prints as 19, because the first 16 slots belong to system
 * exceptions like reset and faults). IPSR isn't memory-mapped, so no
 * pointer can reach it; the MRS instruction below is a one-line preview
 * of the next lesson (inline assembly).
 *
 * Expected serial output (make serial):
 *   thread mode: IPSR=0 (before interrupt)
 *   handler mode: IPSR=19 (inside RTC_WKUP ISR)
 *   thread mode: IPSR=0 (after interrupt)
 *
 * On the L476, IRQ 3 is the RTC wake-up line — we never touch the RTC
 * itself, we only borrow its interrupt number, so the ISR fires exactly
 * once per STIR write. Same IRQ number as the course's F407. */

#include "uart2.h"
#include <stdint.h>
#include <stdio.h>

static uint32_t read_ipsr(void)
{
  uint32_t ipsr;
  __asm volatile("MRS %0, IPSR" : "=r"(ipsr)); /* next lesson explains this */
  return ipsr;
}

/* Runs in THREAD mode. */
static void generate_interrupt(void)
{
  volatile uint32_t *pISER0 = (uint32_t *)0xE000E100;
  volatile uint32_t *pSTIR = (uint32_t *)0xE000EF00;

  *pISER0 |= (1U << 3);  /* unmask IRQ 3 in the NVIC */
  *pSTIR = (3U & 0x1FFU); /* pend IRQ 3 from software — core vectors to the ISR */
}

/* Runs in THREAD mode. */
int main(void)
{
  uart2_init();

  printf("\r\nLesson: operation modes\r\n");
  printf("thread mode: IPSR=%lu (before interrupt)\r\n",
         (unsigned long)read_ipsr());

  generate_interrupt();

  printf("thread mode: IPSR=%lu (after interrupt)\r\n",
         (unsigned long)read_ipsr());

  for (;;) {
  }
}

/* Runs in HANDLER mode — the startup file's vector table points IRQ 3
 * here, overriding its weak do-nothing default. */
void RTC_WKUP_IRQHandler(void)
{
  printf("handler mode: IPSR=%lu (inside RTC_WKUP ISR)\r\n",
         (unsigned long)read_ipsr());
}
