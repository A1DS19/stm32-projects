/* Diagnostic: the MCU as a continuity tester for the keypad wiring.
 *
 * Idea: a pressed keypad key connects exactly two wires. So we take every
 * header pin the wires COULD be on (the intended eight plus their likely
 * neighbors D3, D11, D12), give them all pull-ups, then drive each one
 * low in turn and check which of the others reads low with it: those two
 * pins are electrically connected right now. Hold a key -> its row and
 * column wires name the holes they are actually plugged into.
 *
 * D13/PA5 is left out on purpose: the LED circuit on that pin pulls it
 * around and would fake connections. D0/D1 are the serial port. */

#include "keydiag.h"
#include "stm32l476xx.h"
#include "uart2.h"
#include <stdint.h>
#include <stdio.h>

struct dpin {
  GPIO_TypeDef *port;
  uint8_t pin;
  const char *name;
};

static const struct dpin pins[] = {
    {GPIOA, 10, "D2/PA10"}, {GPIOB, 3, "D3/PB3"},   {GPIOB, 5, "D4/PB5"},
    {GPIOB, 4, "D5/PB4"},   {GPIOB, 10, "D6/PB10"}, {GPIOA, 8, "D7/PA8"},
    {GPIOA, 9, "D8/PA9"},   {GPIOC, 7, "D9/PC7"},   {GPIOB, 6, "D10/PB6"},
    {GPIOA, 7, "D11/PA7"},  {GPIOA, 6, "D12/PA6"},
};
#define NPINS (sizeof(pins) / sizeof(pins[0]))

static void pin_input(const struct dpin *p)
{
  p->port->MODER &= ~(3u << (p->pin * 2u));
}

static void pin_output_low(const struct dpin *p)
{
  p->port->ODR &= ~(1u << p->pin);
  p->port->MODER = (p->port->MODER & ~(3u << (p->pin * 2u)))
                 | (1u << (p->pin * 2u));
}

static void pin_pullup(const struct dpin *p)
{
  p->port->PUPDR = (p->port->PUPDR & ~(3u << (p->pin * 2u)))
                 | (1u << (p->pin * 2u));
}

static uint32_t pin_read(const struct dpin *p)
{
  return (p->port->IDR >> p->pin) & 1u;
}

static void spin(volatile uint32_t n)
{
  while (n--) {
  }
}

void playing_with_keydiag(void)
{
  uart2_init();

  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN
                | RCC_AHB2ENR_GPIOCEN;
  for (uint32_t i = 0; i < NPINS; i++) {
    pin_input(&pins[i]);
    pin_pullup(&pins[i]);
  }

  printf("\r\n== keypad wire finder ==\r\n");
  printf("hold one key at a time; connected header pins are named below\r\n");

  /* One bitmask per pin: which HIGHER-numbered pins it connects to.
   * Printed only when the picture changes. */
  uint16_t seen[NPINS] = {0};

  for (;;) {
    uint16_t now[NPINS] = {0};
    for (uint32_t i = 0; i < NPINS; i++) {
      pin_output_low(&pins[i]);
      spin(20);
      for (uint32_t j = i + 1; j < NPINS; j++) {
        if (pin_read(&pins[j]) == 0) {
          now[i] |= (uint16_t)(1u << j);
        }
      }
      pin_input(&pins[i]);
    }

    int changed = 0;
    for (uint32_t i = 0; i < NPINS; i++) {
      if (now[i] != seen[i]) {
        changed = 1;
        seen[i] = now[i];
      }
    }

    if (changed) {
      int any = 0;
      for (uint32_t i = 0; i < NPINS; i++) {
        for (uint32_t j = i + 1; j < NPINS; j++) {
          if (now[i] & (1u << j)) {
            printf("connected: %s <-> %s\r\n", pins[i].name, pins[j].name);
            any = 1;
          }
        }
      }
      if (!any) {
        printf("(no connections)\r\n");
      }
    }

    spin(40000); /* ~10 checks per second */
  }
}
