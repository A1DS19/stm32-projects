/* Course scaffold: Embedded System Programming on ARM Cortex-M3/M4.
 * Proves the project builds, flashes, and prints before lessons start:
 * banner once, then LD2 (the green LED on pin PA5) toggles and a counter
 * line prints about once a second. Watch it with `make serial`.
 * Lesson code replaces this file; git history is the archive. */

#include "stm32l476xx.h"
#include "uart2.h"
#include <stdint.h>
#include <stdio.h>

/* The chip boots on its 4 MHz internal clock (MSI). At -O0 one pass of
 * this loop costs roughly 10 cycles, so 400000 passes is about a second
 * of spinning. Crude on purpose — SysTick lessons come later. */
static void delay_about_1s(void)
{
  for (volatile uint32_t i = 0; i < 400000U; i++) {
  }
}

int main(void)
{
  uart2_init();

  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; /* power on the GPIOA block */
  GPIOA->MODER &= ~GPIO_MODER_MODE5;   /* clear PA5's two mode bits */
  GPIOA->MODER |= GPIO_MODER_MODE5_0;  /* 01 = general purpose output */

  printf("\r\nEmbedded System Programming on ARM Cortex-M3/M4\r\n");
  printf("Nucleo-L476RG scaffold ready — see SYLLABUS.md for the course map.\r\n\r\n");

  uint32_t seconds = 0;
  for (;;) {
    GPIOA->ODR ^= GPIO_ODR_OD5; /* toggle LD2 */
    printf("alive %lu\r\n", (unsigned long)seconds++);
    delay_about_1s();
  }
}
