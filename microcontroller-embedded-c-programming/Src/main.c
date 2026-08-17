/* Lesson zero: prove the bare-metal toolchain end to end.
 * LD2 (PA5) blinks via direct register access; printf goes out the ST-LINK VCP.
 * Course lessons overwrite this file — old ones live in git history. */

#include <stdio.h>
#include "stm32l476xx.h"
#include "uart2.h"

static void delay(volatile uint32_t count)
{
  while (count--) {}
}

int main(void)
{
  uart2_init();

  /* LD2 = PA5: enable GPIOA clock, set PA5 to output mode (01) */
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
  GPIOA->MODER &= ~GPIO_MODER_MODE5;
  GPIOA->MODER |= GPIO_MODER_MODE5_0;

  printf("microcontroller-embedded-c-programming: lesson zero\r\n");

  uint32_t n = 0;
  for (;;) {
    GPIOA->ODR ^= GPIO_ODR_OD5;
    printf("blink %lu\r\n", (unsigned long)n++);
    delay(400000);
  }
}
