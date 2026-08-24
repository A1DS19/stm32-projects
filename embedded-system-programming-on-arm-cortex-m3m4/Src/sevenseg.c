/* Side demo: a 3461AS 4-digit 7-segment LED display driven by nothing but
 * the register model — peripherals have fixed addresses, addresses hold
 * registers, firmware wakes them up and pokes bits. No controller chip on
 * the display: every lit segment IS a GPIO output bit, made visible.
 *
 * Wiring (display pin 1 = bottom-left; bottom row 1-6 left to right, top
 * row 7-12 right to left). Segments all on GPIOB, digit commons all on
 * GPIOA, so each half of the display is one port = one register write.
 * Every wire lands on a silkscreened Arduino-header pin:
 *
 *   display 11  seg A   -> PB0   (A3)   through 1k resistor
 *   display  7  seg B   -> PB3   (D3)   through 1k
 *   display  4  seg C   -> PB4   (D5)   through 1k
 *   display  2  seg D   -> PB5   (D4)   through 1k
 *   display  1  seg E   -> PB6   (D10)  through 1k
 *   display 10  seg F   -> PB8   (D14)  through 1k
 *   display  5  seg G   -> PB9   (D15)  through 1k
 *   display  3  seg DP  -> PB10  (D6)   through 1k
 *   display 12  DIG1    -> PA0   (A0)   direct
 *   display  9  DIG2    -> PA1   (A1)   direct
 *   display  8  DIG3    -> PA4   (A2)   direct
 *   display  6  DIG4    -> PA10  (D2)   direct
 *
 * The 3461AS is common-cathode: each digit's 8 LEDs share one cathode
 * (DIGn). A segment lights when its GPIOB pin drives HIGH and the digit's
 * GPIOA pin drives LOW to sink the current. Only one digit is on at any
 * instant — the loop lights digit 1, 2, 3, 4 for ~2 ms each, over and
 * over (~125 full frames a second), and your eye blends the flicker into
 * four steady digits ("multiplexing"). That's why 12 pins drive 32 LEDs.
 * A 3461BS is the same part with every diode flipped (common-anode):
 * build with SEVENSEG_COMMON_ANODE=1 and both polarities invert.
 *
 * The register work is exactly the uart2_init() dance on a new peripheral:
 *   1. RCC AHB2ENR    — clock GPIOA+GPIOB (wake the buildings)
 *   2. GPIOx MODER    — two bits per pin, 01 = output; read-modify-write
 *                       only our pins' slices, the neighbours keep theirs
 *   3. GPIOx BSRR     — the workhorse. Write-only: low half sets pins
 *                       high, top half sets them low, all in ONE bus
 *                       write. No read-mask-shift-write dance, and no
 *                       chance of tearing — this register is the ancestor
 *                       of the bit-banding idea coming next lesson.
 * The demo also reads USART2 without blocking, so typing a digit in the
 * serial terminal pushes it onto the display: byte arrives at USART2 RDR
 * (APB1, behind the bridge), CPU carries it across, writes GPIOB BSRR
 * (AHB2), photons. The whole bus discussion in one keypress. */

#include "sevenseg.h"

#include "stm32l476xx.h"
#include "uart2.h"

#include <stdint.h>
#include <stdio.h>

#ifndef SEVENSEG_COMMON_ANODE
    #define SEVENSEG_COMMON_ANODE 0 /* 0 = 3461AS (common cathode), 1 = 3461BS */
#endif

/* GPIOB bit for each segment, in order A B C D E F G DP */
static const uint8_t SEG_BITS[8] = {0, 3, 4, 5, 6, 8, 9, 10};
/* GPIOA bit for each digit common, DIG1..DIG4 (left to right) */
static const uint8_t DIG_BITS[4] = {0, 1, 4, 10};

/* which segments light per numeral: bit 0=A .. bit 6=G (bit 7 = DP) */
static const uint8_t FONT[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

#define DIGIT_HOLD_LOOPS 700U /* ~2 ms at the 4 MHz boot clock; not critical */
#define FRAMES_PER_SECOND 125U

/* one BSRR word that drives all 8 segment pins at once: set the lit ones,
 * reset the dark ones (swapped for common-anode) */
static uint32_t segment_bsrr(uint8_t pattern) {
    uint32_t word = 0;
    for (uint32_t i = 0; i < 8U; ++i) {
        uint32_t set_high = 1U << SEG_BITS[i];
        uint32_t set_low = set_high << 16U;
        uint32_t lit = (pattern >> i) & 1U;
#if SEVENSEG_COMMON_ANODE
        lit = !lit;
#endif
        word |= lit ? set_high : set_low;
    }
    return word;
}

/* one BSRR word that selects a single digit common (or none, pass -1):
 * common-cathode selects by driving LOW */
static uint32_t digit_bsrr(int active) {
    uint32_t word = 0;
    for (int i = 0; i < 4; ++i) {
        uint32_t set_high = 1U << DIG_BITS[i];
        uint32_t set_low = set_high << 16U;
        uint32_t selected = (i == active) ? 1U : 0U;
#if SEVENSEG_COMMON_ANODE
        selected = !selected;
#endif
        word |= selected ? set_low : set_high;
    }
    return word;
}

static void pin_to_output(GPIO_TypeDef* port, uint32_t pin) {
    port->MODER &= ~(3U << (pin * 2U)); /* clear this pin's 2-bit slice */
    port->MODER |= 1U << (pin * 2U);    /* 01 = general-purpose output */
}

static void sevenseg_init(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;

    /* park every line in the "all dark" state BEFORE the pins become
     * outputs, so the display doesn't flash garbage for an instant */
    GPIOB->BSRR = segment_bsrr(0);
    GPIOA->BSRR = digit_bsrr(-1);

    for (uint32_t i = 0; i < 8U; ++i) {
        pin_to_output(GPIOB, SEG_BITS[i]);
    }
    for (uint32_t i = 0; i < 4U; ++i) {
        pin_to_output(GPIOA, DIG_BITS[i]);
    }
}

static void hold_a_moment(void) {
    for (volatile uint32_t i = 0; i < DIGIT_HOLD_LOOPS; ++i) {}
}

void playing_with_seven_segments(void) {
    uint8_t shown[4] = {0, 0, 0, 0}; /* the four numerals, left to right */
    uint32_t seconds = 0;
    uint32_t frames = 0;
    int counting = 1; /* until the first typed digit takes over */

    uart2_init();
    sevenseg_init();

    printf("\r\nSide demo: 3461AS on the register model — no driver chip, no library\r\n");
    printf("segments = GPIOB, one write to BSRR @0x%08lx moves all 8\r\n",
           (unsigned long)(uintptr_t)&GPIOB->BSRR);
    printf("digits   = GPIOA, one write to BSRR @0x%08lx picks 1 of 4\r\n",
           (unsigned long)(uintptr_t)&GPIOA->BSRR);
    printf("counting seconds; type digits here and they take over the display\r\n");

    for (;;) {
        /* one frame: each digit gets ~2 ms in the spotlight */
        for (int pos = 0; pos < 4; ++pos) {
            GPIOB->BSRR = segment_bsrr(FONT[shown[pos]]);
            GPIOA->BSRR = digit_bsrr(pos);
            hold_a_moment();
        }

        int key = uart2_poll();
        if (key >= '0' && key <= '9') {
            counting = 0;
            shown[0] = shown[1];
            shown[1] = shown[2];
            shown[2] = shown[3];
            shown[3] = (uint8_t)(key - '0');
            printf("'%c' came in over APB1, went out over AHB2\r\n", key);
        }

        if (counting && ++frames >= FRAMES_PER_SECOND) {
            frames = 0;
            seconds = (seconds + 1U) % 10000U;
            shown[0] = (uint8_t)(seconds / 1000U % 10U);
            shown[1] = (uint8_t)(seconds / 100U % 10U);
            shown[2] = (uint8_t)(seconds / 10U % 10U);
            shown[3] = (uint8_t)(seconds % 10U);
        }
    }
}
