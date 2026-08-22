/* Lesson: interrupts (ISR) + volatile.
 *
 * Last lesson we POLLED the button: main sat in a loop reading IDR. Polling
 * only works while the CPU has nothing better to do — miss the window, miss
 * the press. An interrupt inverts that: the hardware watches the pin and
 * CALLS YOUR FUNCTION the instant the edge happens, no matter what main was
 * doing. The chain on an STM32:
 *
 *   PC13 edge -> SYSCFG (routes pin 13 of port C onto EXTI line 13)
 *             -> EXTI (edge detector, latches a PENDING bit)
 *             -> NVIC (the Cortex-M interrupt controller, if line enabled)
 *             -> CPU pushes registers, jumps via the VECTOR TABLE to the ISR
 *
 * Where does the name EXTI15_10_IRQHandler come from? The vector table in
 * startup_stm32l476xx.S. Every entry is declared .weak and aliased to
 * Default_Handler (an infinite loop); defining a function with the EXACT
 * same name overrides the weak alias at link time. The name IS the
 * registration — there is no "attach callback" call anywhere.
 *
 * And volatile? Same bug as last lesson, new culprit. main's wait loop reads
 * tick_ms and blink_interval_ms over and over; the compiler sees no code
 * path in main that writes them, so at -O2 it would cache both in CPU
 * registers — the loop would never end and button presses would never take
 * effect. The ISR is invisible to the optimizer's reasoning, exactly like
 * the peripheral was. Any variable shared between an ISR and main must be
 * volatile. (This file is compiled -O2 like volatile.c, so deleting a
 * volatile below really does break it — try it.)
 *
 * Demo: LD2 toggles every 2 s. Each press of blue B1 makes the blink 10x
 * faster; past the 100 ms floor it resets to 2 s. So: 2000 -> 200 -> 2000...
 */

#include "isr.h"
#include "stm32l476xx.h"
#include "uart2.h"
#include <stdint.h>
#include <stdio.h>

#define CPU_HZ 4000000u /* MSI reset default, SystemInit leaves it alone */

#define BLINK_START_MS 2000u
#define SPEEDUP 10u     /* each press divides the interval by this */
#define BLINK_MIN_MS 100u

/* Shared ISR <-> main data. Written in interrupt context, read in main:
 * volatile is mandatory (see header comment). A single aligned 32-bit
 * load/store is atomic on Cortex-M, so for these two words that is all the
 * protection needed — anything wider (a struct, a 64-bit counter) would
 * additionally need the interrupt masked around the access. */
static volatile uint32_t tick_ms;
static volatile uint32_t blink_interval_ms = BLINK_START_MS;

/* SysTick is a timer inside the core itself (not on an RCC bus). Configured
 * below to fire this every 1 ms — our clock source for all timing. */
void SysTick_Handler(void)
{
  tick_ms++;
}

/* One handler covers EXTI lines 10..15; with several lines enabled you'd
 * check PR1 to see which fired. Keep ISRs short: update state, get out.
 * printf does NOT belong here — it's slow and not reentrant; main does the
 * talking. (No software debounce: the Nucleo wires a 100 nF cap on B1.) */
void EXTI15_10_IRQHandler(void)
{
  /* PR1 is write-1-to-CLEAR: writing the bit acknowledges the interrupt.
   * Classic bug: forget this line and the ISR re-enters forever — main
   * appears to freeze while the CPU spins in here. (|= would be wrong too:
   * it writes 1 to EVERY pending bit read back, clearing other lines.) */
  EXTI->PR1 = EXTI_PR1_PIF13;

  uint32_t next = blink_interval_ms / SPEEDUP;
  blink_interval_ms = (next < BLINK_MIN_MS) ? BLINK_START_MS : next;
}

void playing_with_isr(void)
{
  uart2_init();

  /* SysTick, 3 registers and done: reload value (counts LOAD..0, so -1),
   * clear current count, then enable with CPU clock + interrupt. */
  SysTick->LOAD = CPU_HZ / 1000u - 1u; /* 4000 ticks = 1 ms at 4 MHz */
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk /* clock = CPU, not /8 */
                | SysTick_CTRL_TICKINT_Msk   /* fire the exception */
                | SysTick_CTRL_ENABLE_Msk;

  /* LD2 on PA5 output, B1 on PC13 input (external pull-up on the board). */
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN;
  GPIOA->MODER &= ~GPIO_MODER_MODE5;
  GPIOA->MODER |= GPIO_MODER_MODE5_0;
  GPIOC->MODER &= ~GPIO_MODER_MODE13;

  /* Route PC13 onto EXTI line 13. SYSCFG is the mux that picks WHICH port's
   * pin 13 feeds the line (only one can). It sits on APB2 — clock it first.
   * EXTICR[3] = EXTICR4 covers pins 12..15. */
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
  SYSCFG->EXTICR[3] &= ~SYSCFG_EXTICR4_EXTI13;
  SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI13_PC;

  /* Pressed = wired to GND = falling edge. Unmask the line, then tell the
   * NVIC to deliver it (NVIC_EnableIRQ writes one bit in NVIC->ISER). */
  EXTI->FTSR1 |= EXTI_FTSR1_FT13;
  EXTI->IMR1 |= EXTI_IMR1_IM13;
  NVIC_EnableIRQ(EXTI15_10_IRQn);

  printf("\r\n== ISR demo: LD2 toggles, B1 speeds it up ==\r\n");
  printf("toggling every %lu ms — press the blue button\r\n",
         (unsigned long)BLINK_START_MS);

  uint32_t shown = BLINK_START_MS;
  for (;;) {
    GPIOA->ODR ^= GPIO_ODR_OD5;

    /* Wait one interval. Unsigned subtraction survives tick_ms wrapping.
     * Both reads are volatile, so a press DURING the wait is honored: the
     * new interval is compared on the very next pass. */
    uint32_t start = tick_ms;
    while ((tick_ms - start) < blink_interval_ms) {
      if (blink_interval_ms != shown) { /* ISR changed it — report */
        shown = blink_interval_ms;
        printf("interval now %lu ms\r\n", (unsigned long)shown);
      }
    }
  }
}
