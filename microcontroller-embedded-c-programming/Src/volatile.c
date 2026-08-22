/* Lesson: volatile — why every hardware-register access must be volatile.
 *
 * `volatile` is a promise to the compiler: "the value at this address can
 * change on its own, and every access I wrote matters — do each read and
 * each write for real, in the order I wrote them, no skipping, no caching."
 *
 * Without it, the compiler assumes it is the only one touching the data,
 * and it "optimizes": a value read inside a loop gets read once, the copy
 * is kept in a CPU register, and the copy is reused. A loop that then
 * looks useless gets deleted completely. That is great for normal
 * variables — and wrong for hardware, because hardware changes values
 * behind the compiler's back.
 *
 * Demo: watch the blue button (B1) with two loops whose source code is
 * identical. B1 sits on pin PC13: it reads 1 when idle, 0 while pressed.
 * ONLY THIS FILE is compiled with optimization on (-O2, set in
 * CMakeLists.txt). The rest of the build uses -O0 (no optimization),
 * where even the buggy loop happens to work — which is exactly why this
 * kind of bug shows up only when optimization is switched on.
 *
 *   round 1: the pointer's volatile is removed with a cast -> the
 *            compiler reads the button register once, decides the loop
 *            can never change, and deletes the whole 5-second wait.
 *            Your press is invisible; the poll returns in ~0 ms.
 *   round 2: a proper `volatile uint32_t *` -> one fresh read on every
 *            loop pass, so the press is caught at human speed.
 *
 * CMSIS marks every register `__IO` (which simply means volatile) for
 * this exact reason; round 1 has to strip it on purpose to show the bug.
 * (Side note: a per-function optimize("O2") attribute was tried first
 * and silently did nothing — GCC documents it as unreliable. That is why
 * the -O2 lives in CMakeLists.txt instead.) */

#include "volatile.h"
#include "stm32l476xx.h"
#include "uart2.h"
#include <stdint.h>
#include <stdio.h>

/* CPU speed after reset (the MSI clock); SystemInit() does not change it. */
#define CPU_HZ 4000000u

/* How many loop passes round 2 waits before giving up — about 5 seconds.
 * (Round 1 needs no budget at all: its loop gets deleted.) */
#define POLL_BUDGET 3000000u

/* The DWT is a small debug unit inside the CPU. It has a counter that
 * goes up by 1 every clock cycle — a free stopwatch for us. */
static void cyccnt_init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; /* switch the DWT on */
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void spin_cycles(uint32_t cycles)
{
  uint32_t start = DWT->CYCCNT;
  while ((DWT->CYCCNT - start) < cycles) {
  }
}

/* THE BUG. The cast throws away the volatile that CMSIS put on IDR. The
 * compiler now treats the register like a normal variable: it reads it
 * once before the loop starts. The loop body is then empty, so the
 * compiler deletes the countdown as well. A press that arrives after
 * that single read can never be seen. */
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

/* THE FIX. Same loop, but the pointer keeps volatile: every pass does a
 * real read of the register at 0x48000810 (GPIOC base + IDR offset), so
 * the press is seen. */
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

  /* If B1 is still held down from the last round, even the buggy loop's
   * single read would see it and confuse the result — so wait here until
   * the button is released. */
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
