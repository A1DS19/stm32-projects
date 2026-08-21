/* Lesson: read PA0 (digital input) and mirror it on LD2 (PA5).
 * Jumper PA0 (Arduino A0) to 3.3V -> LED on; unplug (pull-down wins) -> LED off.
 *
 * Where the names come from: stm32l476xx.h (CMSIS device header) maps every
 * peripheral as a struct sitting at its bus address. GPIOA is
 *   #define GPIOA ((GPIO_TypeDef *) 0x48000000)
 * and GPIO_TypeDef is just the register layout from the reference manual:
 * each member (MODER, PUPDR, IDR, ODR, ...) is one 32-bit hardware register,
 * declared volatile so every access really hits the bus. So GPIOA->MODER
 * is nothing more than "the 32-bit word at address 0x48000000 + 0x00".
 * The GPIO_MODER_MODE5* macros are plain bit masks for slices of that word. */

#include "stm32l476xx.h"
#include "uart2.h"

int main(void)
{
  uart2_init();

  /* Peripherals are dead until their bus clock runs. RCC (Reset & Clock
   * Control) gates every clock; AHB2ENR bit 0 feeds port A. Without this,
   * writes to any GPIOA register are silently ignored. */
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

  /* MODER = MODE Register: 2 bits per pin (32 bits / 16 pins).
   * Pin 5 owns bits [11:10]. The 2-bit values:
   *   00 input, 01 output, 10 alternate function, 11 analog.
   * GPIO_MODER_MODE5   = 0x3 << 10 (mask of both bits)
   * GPIO_MODER_MODE5_0 = 0x1 << 10 (just the low bit)
   * Clear-then-set writes 01 = output no matter what was there before. */
  GPIOA->MODER &= ~GPIO_MODER_MODE5;
  GPIOA->MODER |= GPIO_MODER_MODE5_0;

  /* PA0 as input: 00 in MODER bits [1:0], so clearing is enough. */
  GPIOA->MODER &= ~GPIO_MODER_MODE0;

  /* PUPDR = Pull-Up/Pull-Down Register, same 2-bits-per-pin layout:
   *   00 none, 01 pull-up, 10 pull-down.
   * Pull-down = a weak internal resistor to GND, so a disconnected PA0
   * reads a clean 0 instead of floating and picking up noise.
   * PUPD0_1 is the high bit of pin 0's pair -> value 10. */
  GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD0;
  GPIOA->PUPDR |=  GPIO_PUPDR_PUPD0_1;

  for (;;) {
    /* IDR = Input Data Register (read-only): 1 bit per pin, the live
     * voltage on the pin. GPIO_IDR_ID0 = 1 << 0. Non-zero AND = pin high. */
    if (GPIOA->IDR & GPIO_IDR_ID0) {
      /* ODR = Output Data Register: 1 bit per pin, the level we drive.
       * Read-modify-write bit 5 to switch the LED without touching others. */
      GPIOA->ODR |= GPIO_ODR_OD5;
    } else {
      GPIOA->ODR &= ~GPIO_ODR_OD5;
    }
  }
}
