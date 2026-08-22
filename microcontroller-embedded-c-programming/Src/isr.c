/* Lesson: interrupts (ISR) + volatile.
 *
 * Last lesson we POLLED the button: main sat in a loop reading the pin.
 * Polling only works while the CPU has nothing else to do — look away for
 * a moment and the press is missed. An interrupt flips the roles: the
 * hardware watches the pin and CALLS YOUR FUNCTION the instant something
 * happens, no matter what main was doing. That function is the ISR
 * (Interrupt Service Routine). The path on an STM32:
 *
 *   PC13 edge -> SYSCFG  (a selector: which port's pin 13 feeds line 13)
 *             -> EXTI    (edge detector; latches a "pending" flag)
 *             -> NVIC    (the interrupt controller inside the CPU)
 *             -> CPU saves its place and jumps, via the VECTOR TABLE,
 *                into the ISR
 *
 * Where does the name EXTI15_10_IRQHandler come from? The vector table in
 * startup_stm32l476xx.S lists one function name per interrupt. Every entry
 * starts as "weak" — a placeholder that is just an endless loop. Define a
 * function with the EXACT same name and yours replaces the placeholder
 * when the program is linked. The name itself IS the registration; there
 * is no "attach my callback" call anywhere.
 *
 * And volatile? Same bug as last lesson, new cause. main's wait loop keeps
 * reading tick_ms and blink_interval_ms. No line in main ever writes them,
 * so with optimization on the compiler would read each once and reuse the
 * copies — the loop would never end and presses would change nothing. The
 * compiler cannot know the ISR runs: the ISR is invisible to it, exactly
 * like the hardware was. Rule: any variable shared between an ISR and
 * main must be volatile. (This file is compiled -O2 like volatile.c, so
 * deleting a volatile below really does break it — try it.)
 *
 * Demo: LD2 toggles every 2 s. Each press of blue B1 doubles the blink
 * speed; past the 100 ms floor it resets to 2 s. So:
 * 2000 -> 1000 -> 500 -> 250 -> 125 -> 2000 -> ...
 */

#include "isr.h"
#include "stm32l476xx.h"
#include "uart2.h"
#include <stdint.h>
#include <stdio.h>

#define CPU_HZ 4000000u /* CPU speed after reset; SystemInit leaves it alone */

#define BLINK_START_MS 2000u
#define SPEEDUP 2u      /* each press divides the interval by this */
#define BLINK_MIN_MS 100u

/* Data shared between the ISR and main. The ISR writes, main reads:
 * volatile is required (see the top comment). One more worry with shared
 * data: could main catch a value half-written? Not here — the CPU reads
 * or writes an aligned 32-bit variable in a single step (the access is
 * "atomic"). Anything bigger — a struct, a 64-bit counter — is moved in
 * pieces, and then you must switch the interrupt off around the access. */
static volatile uint32_t tick_ms;
static volatile uint32_t blink_interval_ms = BLINK_START_MS;

/* SysTick is a simple timer built into the CPU core itself (not a
 * peripheral on a bus). Set up below to call this every 1 ms — it is
 * the clock all our timing counts on. */
void SysTick_Handler(void)
{
  tick_ms++;
}

/* One handler serves EXTI lines 10..15 together; with several lines
 * enabled you would check PR1 to see which one fired. Keep ISRs short:
 * update your data, get out. printf does NOT belong in here — it is slow,
 * and it breaks if an interrupt re-enters it halfway. main does the
 * talking. (No debounce code needed: the board has a 100 nF capacitor on
 * B1 that cleans up the press.) */
void EXTI15_10_IRQHandler(void)
{
  /* PR1 is write-1-to-clear: writing a 1 to a bit ACKNOWLEDGES that
   * interrupt. Classic bug: forget this line and the ISR runs again the
   * moment it returns, forever — main looks frozen. (|= would be wrong
   * too: it writes back a 1 to every pending bit it read, clearing OTHER
   * lines by accident.) */
  EXTI->PR1 = EXTI_PR1_PIF13;

  uint32_t next = blink_interval_ms / SPEEDUP;
  blink_interval_ms = (next < BLINK_MIN_MS) ? BLINK_START_MS : next;
}

void playing_with_isr(void)
{
  uart2_init();

  /* SysTick takes 3 registers: LOAD = where the countdown starts (it
   * counts LOAD..0, hence the -1), VAL = clear the current count,
   * CTRL = use the CPU clock, raise the interrupt, run. */
  SysTick->LOAD = CPU_HZ / 1000u - 1u; /* 4000 ticks = 1 ms at 4 MHz */
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk /* clock = CPU, not CPU/8 */
                | SysTick_CTRL_TICKINT_Msk   /* call SysTick_Handler */
                | SysTick_CTRL_ENABLE_Msk;

  /* LD2 on PA5 as output, B1 on PC13 as input (the board already has a
   * pull-up resistor on B1). */
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN;
  GPIOA->MODER &= ~GPIO_MODER_MODE5;
  GPIOA->MODER |= GPIO_MODER_MODE5_0;
  GPIOC->MODER &= ~GPIO_MODER_MODE13;

  /* Connect PC13 to EXTI line 13. Several ports have a pin 13 (PA13,
   * PB13, ...) and only one may drive the line — SYSCFG is the selector
   * that picks which. It sits on the APB2 bus, so clock it first.
   * EXTICR[3] (called EXTICR4 in the manual) covers pins 12..15. */
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
  SYSCFG->EXTICR[3] &= ~SYSCFG_EXTICR4_EXTI13;
  SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI13_PC;

  /* Pressed = pin pulled to GND = a falling edge. Unmask line 13 in
   * EXTI, then let the NVIC deliver it (NVIC_EnableIRQ sets one enable
   * bit inside the NVIC). */
  EXTI->FTSR1 |= EXTI_FTSR1_FT13;
  EXTI->IMR1 |= EXTI_IMR1_IM13;
  NVIC_EnableIRQ(EXTI15_10_IRQn);

  printf("\r\n== ISR demo: LD2 toggles, B1 speeds it up ==\r\n");
  printf("toggling every %lu ms — press the blue button\r\n",
         (unsigned long)BLINK_START_MS);

  uint32_t shown = BLINK_START_MS;
  for (;;) {
    GPIOA->ODR ^= GPIO_ODR_OD5;

    /* Wait one interval. The subtraction is unsigned, so it stays right
     * even when tick_ms wraps around to 0 (after ~49 days). Both reads
     * are volatile, so a press DURING the wait counts: the very next
     * pass compares against the new interval. */
    uint32_t start = tick_ms;
    while ((tick_ms - start) < blink_interval_ms) {
      if (blink_interval_ms != shown) { /* the ISR changed it — report */
        shown = blink_interval_ms;
        printf("interval now %lu ms\r\n", (unsigned long)shown);
      }
    }
  }
}
