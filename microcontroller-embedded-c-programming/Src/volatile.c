/* Lesson: volatile — why every hardware-register access must be volatile.
 *
 * `volatile` tells the compiler: this object can change behind your back (or
 * the access itself matters), so every read/write written in the source must
 * become a real load/store on the bus, in program order — no caching in a
 * register, no merging, no deleting "useless" reads.
 *
 * Without it the optimizer assumes plain RAM that only this code touches:
 * a read in a loop is loop-invariant, so it is hoisted out and done once;
 * a loop left with no visible effect can be removed entirely.
 *
 * Demo: poll the blue user button B1. On the Nucleo it sits on PC13 with a
 * pull-up and closes to GND, so IDR bit 13 reads 1 idle, 0 pressed. Two
 * polling loops with identical source. THIS FILE ALONE is compiled -O2
 * (set_source_files_properties in CMakeLists.txt) while the rest of the
 * Debug build stays -O0, where even non-volatile code happens to work —
 * which is exactly why this bug class only bites when optimization is
 * turned on. (A per-function optimize("O2") attribute was tried first and
 * silently did nothing: GCC documents it as unreliable from -O0.)
 *
 *   round 1: the pointer's volatile is cast away -> the compiler reads IDR
 *            once, and the now-empty countdown loop is optimized away too.
 *            Your press is invisible; the "5 s poll" returns in ~0 ms.
 *   round 2: proper `volatile uint32_t *` -> one fresh load per iteration,
 *            the press is caught at human reaction speed.
 *
 * CMSIS declares every peripheral register `__IO` (= volatile) for exactly
 * this reason; round 1 has to strip it deliberately to reproduce the bug. */

#include "volatile.h"
#include "stm32l476xx.h"
#include "uart2.h"
#include <stdint.h>
#include <stdio.h>

/* MSI reset default; SystemInit() doesn't touch the clock tree. */
#define CPU_HZ 4000000u

/* Poll budget: at -O2 the volatile loop is ~7 cycles/pass -> roughly 5 s.
 * (The non-volatile round would need no budget at all — it vanishes.) */
#define POLL_BUDGET 3000000u

/* DWT (Data Watchpoint & Trace) has a free-running CPU cycle counter —
 * a debug peripheral on the core itself, handy as a stopwatch. */
static void cyccnt_init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; /* unlock DWT */
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void spin_cycles(uint32_t cycles)
{
  uint32_t start = DWT->CYCCNT;
  while ((DWT->CYCCNT - start) < cycles) {
  }
}

/* THE BUG. Casting &GPIOC->IDR to a plain pointer strips the volatile that
 * CMSIS put there. At -O2 the compiler sees a loop-invariant load: it reads
 * IDR once before the loop, and since the loop body is then empty it deletes
 * the countdown as well. A press after entry can never be seen. */
static uint32_t wait_for_press_plain(uint32_t budget)
{
  const uint32_t *idr = (const uint32_t *)&GPIOC->IDR;
  while (budget--) {
    if ((*idr & GPIO_IDR_ID13) == 0) {
      return 1;
    }
  }
  return 0;
}

/* THE FIX. Same loop, pointer kept volatile: every pass performs a real
 * load from 0x48000810 (GPIOC base + IDR offset), so the press is seen. */
static uint32_t wait_for_press_volatile(uint32_t budget)
{
  const volatile uint32_t *idr = &GPIOC->IDR;
  while (budget--) {
    if ((*idr & GPIO_IDR_ID13) == 0) {
      return 1;
    }
  }
  return 0;
}

/* Prompt, give the human 2 s to get ready, then time one polling round. */
static void run_round(const char *label, uint32_t (*wait_for_press)(uint32_t))
{
  printf("%s: when GO prints, press the blue B1 button\r\n", label);

  /* If B1 is still held from the previous round, even the buggy loop's one
   * hoisted read would see it and muddy the lesson — wait for release. */
  while ((GPIOC->IDR & GPIO_IDR_ID13) == 0) {
  }

  spin_cycles(2 * CPU_HZ);
  printf("GO\r\n");

  uint32_t t0 = DWT->CYCCNT;
  uint32_t seen = wait_for_press(POLL_BUDGET);
  uint32_t ms = (DWT->CYCCNT - t0) / (CPU_HZ / 1000u);

  printf("  press seen: %s — poll loop lasted %lu ms\r\n",
         seen ? "YES" : "no", (unsigned long)ms);
}

void playing_with_volatile(void)
{
  uart2_init();
  cyccnt_init();

  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
  GPIOC->MODER &= ~GPIO_MODER_MODE13; /* 00 = input (reset state is analog) */

  printf("\r\n== volatile: polling B1 (PC13) at -O2 ==\r\n");

  run_round("round 1, volatile CAST AWAY", wait_for_press_plain);
  run_round("round 2, volatile pointer  ", wait_for_press_volatile);

  printf("done — press the black RESET button to run it again\r\n");
}
