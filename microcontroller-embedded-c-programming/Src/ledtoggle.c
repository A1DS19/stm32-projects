/* Lesson: LED toggle through bit-field register structs — everything
 * from the last lessons in one build:
 *
 *   struct   -> the register map: members at the hardware's offsets
 *   union    -> each register readable two ways: named bits or whole word
 *   bit-field-> every pin is a named slice (pin5, not "bits [11:10]")
 *   volatile -> on every register, as always (lesson 1)
 *
 * So instead of CMSIS masks:
 *     GPIOA->MODER &= ~GPIO_MODER_MODE5; GPIOA->MODER |= GPIO_MODER_MODE5_0;
 * the same hardware write becomes:
 *     LED_PORT->MODER.f.pin5 = 1;
 *
 * New tool: _Static_assert — a check the COMPILER runs. If a slice count
 * or a member order is wrong, the build fails with our message instead of
 * the board misbehaving. A register map you can't get silently wrong.
 *
 * Honest reminder from the bitfield lesson: every bit-field write here is
 * a hidden read-modify-write of the whole register. That is fine in this
 * program — boot-time setup plus a single toggle loop, no interrupts
 * touching these registers, no write-1-to-clear bits anywhere in sight.
 * Know why it's safe here, and when it wouldn't be. */

#include "ledtoggle.h"
#include "stm32l476xx.h"
#include "uart2.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* RCC AHB2ENR: one enable bit per GPIO port (bits 8+ serve other
 * peripherals — left as one reserved block, we don't use them). */
typedef union {
  struct {
    uint32_t gpioa : 1, gpiob : 1, gpioc : 1, gpiod : 1;
    uint32_t gpioe : 1, gpiof : 1, gpiog : 1, gpioh : 1;
    uint32_t rest : 24;
  } f;
  uint32_t word;
} ahb2enr_t;

/* GPIO MODER: sixteen 2-bit slices, one per pin (00 input, 01 output,
 * 10 alternate, 11 analog). 16 x 2 = exactly 32 bits. */
typedef union {
  struct {
    uint32_t pin0 : 2, pin1 : 2, pin2 : 2, pin3 : 2;
    uint32_t pin4 : 2, pin5 : 2, pin6 : 2, pin7 : 2;
    uint32_t pin8 : 2, pin9 : 2, pin10 : 2, pin11 : 2;
    uint32_t pin12 : 2, pin13 : 2, pin14 : 2, pin15 : 2;
  } f;
  uint32_t word;
} moder_t;

/* GPIO ODR: one output bit per pin, upper half reserved. */
typedef union {
  struct {
    uint32_t pin0 : 1, pin1 : 1, pin2 : 1, pin3 : 1;
    uint32_t pin4 : 1, pin5 : 1, pin6 : 1, pin7 : 1;
    uint32_t pin8 : 1, pin9 : 1, pin10 : 1, pin11 : 1;
    uint32_t pin12 : 1, pin13 : 1, pin14 : 1, pin15 : 1;
    uint32_t rest : 16;
  } f;
  uint32_t word;
} odr_t;

/* The port map, offsets from the reference manual — same shape as the
 * struct lesson, but now the interesting registers carry named bits. */
struct gpio_port {
  volatile moder_t MODER;    /* 0x00 */
  volatile uint32_t OTYPER;  /* 0x04 */
  volatile uint32_t OSPEEDR; /* 0x08 */
  volatile uint32_t PUPDR;   /* 0x0C */
  volatile uint32_t IDR;     /* 0x10 */
  volatile odr_t ODR;        /* 0x14 */
};

/* The compiler checks the map. Wrong slice widths or a missing member
 * and the build stops here — the board never sees the mistake. */
_Static_assert(sizeof(ahb2enr_t) == 4, "AHB2ENR view must be one word");
_Static_assert(sizeof(moder_t) == 4, "MODER view must be one word");
_Static_assert(sizeof(odr_t) == 4, "ODR view must be one word");
_Static_assert(offsetof(struct gpio_port, IDR) == 0x10, "IDR belongs at 0x10");
_Static_assert(offsetof(struct gpio_port, ODR) == 0x14, "ODR belongs at 0x14");

#define MY_AHB2ENR ((volatile ahb2enr_t *)(0x40021000u + 0x4Cu))
#define LED_PORT ((struct gpio_port *)0x48000000u) /* GPIOA; LD2 = pin 5 */

static void spin(volatile uint32_t n)
{
  while (n--) {
  }
}

void playing_with_ledtoggle(void)
{
  uart2_init();

  printf("\r\n== LED toggle through bit-field register structs ==\r\n");
  printf("register map verified at compile time (_Static_assert)\r\n");

  /* Every hardware touch below goes through OUR types; each step is
   * cross-checked by reading back through CMSIS — two views, same
   * silicon, like the struct lesson proved. */
  MY_AHB2ENR->f.gpioa = 1;
  printf("clock:  AHB2ENR.gpioa = 1 -> CMSIS reads bit0 = %lu\r\n",
         (unsigned long)(RCC->AHB2ENR & 1u));

  LED_PORT->MODER.f.pin5 = 1; /* 01 = output */
  printf("mode:   MODER.pin5 = 1   -> CMSIS reads MODER[11:10] = %lu\r\n",
         (unsigned long)((GPIOA->MODER >> 10) & 3u));

  for (int i = 1; i <= 6; i++) {
    LED_PORT->ODR.f.pin5 ^= 1; /* the toggle, one named bit */
    printf("toggle %d -> CMSIS reads IDR bit5 = %lu\r\n", i,
           (unsigned long)((GPIOA->IDR >> 5) & 1u));
    spin(200000);
  }

  printf("blinking forever — watch LD2\r\n");
  for (;;) {
    LED_PORT->ODR.f.pin5 ^= 1;
    spin(200000);
  }
}
