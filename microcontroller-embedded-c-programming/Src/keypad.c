/* Lesson: 4x4 matrix keypad — scanning, debouncing, pin tables.
 *
 * The module is nothing but 16 switches. Pressing a key connects one of
 * the 4 ROW wires to one of the 4 COLUMN wires — that's all. 8 pins for
 * 16 keys, no power, no ground, no external resistors: the chip's
 * internal pull-ups do the electrical work.
 *
 * How the scan works:
 *   - Columns are inputs with pull-ups -> they read 1 when nothing pulls
 *     them down.
 *   - Rows sit disconnected (high-impedance inputs) when idle.
 *   - To scan: make ONE row an output driving 0, then read the columns.
 *     A column reading 0 means the key at (that row, that column) is
 *     down — the switch connected the column to our low row.
 *   - Repeat for each of the 4 rows: 16 keys checked in 4 steps.
 * Idle rows stay high-impedance (not driven high!) so two pressed keys
 * on the same column can never short a high row into a low row.
 *
 * Debouncing — the real kind, at last. The blue B1 button never bounced
 * because the Nucleo hides a 100 nF capacitor on it. These membrane keys
 * are honest: the contact flickers for a few milliseconds on press and
 * release. Fix in software: poll every few ms and accept a new state
 * only after it has read the same for several polls in a row.
 *
 * Ghosting, the classic matrix flaw: press three keys forming an L
 * (say 1, 2, 4) and the keypad seems to press the fourth corner (5) by
 * itself — current sneaks from the scanned row through the three
 * switches into a column we then misread. Real keyboards fix it with a
 * diode per key; membrane pads just live with it. Try it and watch.
 *
 * The wiring (Arduino header; D0/D1 are the serial port, D13 the LED,
 * so they are avoided):
 *   rows 1..4 -> D2 PA10, D4 PB5, D5 PB4, D6 PB10
 *   cols 1..4 -> D7 PA8,  D8 PA9, D9 PC7, D10 PB6
 * The pins span three ports, so the driver uses const pin-descriptor
 * tables (struct lessons put to work) instead of hard-coded registers. */

#include "keypad.h"
#include "stm32l476xx.h"
#include "uart2.h"
#include <stdint.h>
#include <stdio.h>

struct pin {
  GPIO_TypeDef *port;
  uint8_t pin;
};

/* const -> these tables live in flash, not RAM. */
static const struct pin rows[4] = {
    {GPIOA, 10}, {GPIOB, 5}, {GPIOB, 4}, {GPIOB, 10}};
static const struct pin cols[4] = {
    {GPIOA, 8}, {GPIOA, 9}, {GPIOC, 7}, {GPIOB, 6}};

static const char keymap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'},
};

/* Small register helpers over the descriptor — each is one MODER/PUPDR/
 * IDR access, pin number scaled to its 2-bit or 1-bit slice. */
static void pin_input(const struct pin *p)
{
  p->port->MODER &= ~(3u << (p->pin * 2u));
}

static void pin_output(const struct pin *p)
{
  p->port->MODER = (p->port->MODER & ~(3u << (p->pin * 2u)))
                 | (1u << (p->pin * 2u));
}

static void pin_pullup(const struct pin *p)
{
  p->port->PUPDR = (p->port->PUPDR & ~(3u << (p->pin * 2u)))
                 | (1u << (p->pin * 2u));
}

static uint32_t pin_read(const struct pin *p)
{
  return (p->port->IDR >> p->pin) & 1u;
}

static void spin(volatile uint32_t n)
{
  while (n--) {
  }
}

static void keypad_init(void)
{
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN
                | RCC_AHB2ENR_GPIOCEN;

  for (int i = 0; i < 4; i++) {
    /* Rows: output LEVEL is preloaded to 0 once; scanning then only
     * flips the pin between high-impedance input (idle) and output
     * (driving that preloaded 0). */
    rows[i].port->ODR &= ~(1u << rows[i].pin);
    pin_input(&rows[i]);

    pin_input(&cols[i]);
    pin_pullup(&cols[i]);
  }
}

/* One full scan. Returns the key character, or 0 if nothing is pressed.
 * (First key found wins — this simple driver reports one key at a time.) */
static char keypad_scan(void)
{
  char found = 0;
  for (int r = 0; r < 4 && !found; r++) {
    pin_output(&rows[r]); /* drive this row low */
    spin(20);             /* let the column wires settle (~10 us) */
    for (int c = 0; c < 4; c++) {
      if (pin_read(&cols[c]) == 0) {
        found = keymap[r][c];
        break;
      }
    }
    pin_input(&rows[r]); /* release the row again */
  }
  return found;
}

void playing_with_keypad(void)
{
  uart2_init();
  keypad_init();

  /* LD2 for the 'A' key, set up the usual CMSIS way. */
  GPIOA->MODER &= ~GPIO_MODER_MODE5;
  GPIOA->MODER |= GPIO_MODER_MODE5_0;

  printf("\r\n== 4x4 keypad: press keys ==\r\n");
  printf("(A toggles LD2; digits collect; # prints and clears them)\r\n");

  char digits[13];
  uint32_t ndigits = 0;

  /* Debounce: accept a new state only after it has read identically for
   * DEBOUNCE_POLLS polls in a row (~15 ms of stability). `stable` is the
   * accepted state; a press is the moment it changes to a new key. */
  enum { DEBOUNCE_POLLS = 3 };
  char stable = 0, last_raw = 0;
  uint32_t same = 0;

  for (;;) {
    char raw = keypad_scan();
    if (raw == last_raw) {
      if (same < DEBOUNCE_POLLS) {
        same++;
      }
    } else {
      last_raw = raw;
      same = 0;
    }

    if (same == DEBOUNCE_POLLS && raw != stable) {
      stable = raw;
      if (stable != 0) { /* a debounced PRESS (releases stay silent) */
        printf("key: %c\r\n", stable);
        if (stable == 'A') {
          GPIOA->ODR ^= GPIO_ODR_OD5;
        } else if (stable == '#') {
          digits[ndigits] = '\0';
          printf("entered: %s\r\n", ndigits ? digits : "(nothing)");
          ndigits = 0;
        } else if (stable >= '0' && stable <= '9' &&
                   ndigits < sizeof(digits) - 1) {
          digits[ndigits++] = stable;
        }
      }
    }

    spin(2500); /* ~5 ms between polls at this clock */
  }
}
