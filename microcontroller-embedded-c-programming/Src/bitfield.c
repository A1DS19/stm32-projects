/* Lesson: bit-fields & unions, the full story.
 *
 * A bit-field is a named slice of an integer: `uint32_t sample : 12` IS
 * "bits [x+11:x] of the word", and every read/write of it compiles to the
 * shift-and-mask you'd otherwise write by hand (section 2 proves they're
 * identical). Always declare them unsigned: a plain `int flag : 1` holds
 * 0 and -1, not 0 and 1.
 *
 * A union is overlapping storage: all members start at offset 0 and share
 * the bytes; sizeof = the largest member. Reading a different member than
 * you last wrote reinterprets the same bytes (legal in C, and the idiom
 * embedded lives by): word-view of packed fields, byte-view of a word for
 * UART/CAN transmission — which also exposes the CPU's endianness.
 *
 * Section 4 does what CMSIS deliberately doesn't: overlays bit-fields on a
 * live GPIO register and blinks the LED by assigning to a 1-bit name. It
 * works — and the comments there spell out the three reasons production
 * register code still uses masks (RMW hazard, layout portability, access
 * width), so you know exactly what you're trading. */

#include "bitfield.h"
#include "stm32l476xx.h"
#include "uart2.h"
#include <stdint.h>
#include <stdio.h>

/* --- sections 1 & 2: slices in RAM ---------------------------------- */

union packet {
  struct {
    uint32_t addr : 5;    /* bits [4:0]  : 0..31   */
    uint32_t rw : 1;      /* bit  [5]              */
    uint32_t len : 10;    /* bits [15:6] : 0..1023 */
    uint32_t reserved : 16;
  } f;
  uint32_t word;
};

/* --- section 3: byte view ------------------------------------------- */

union word_bytes {
  uint32_t value;
  uint8_t bytes[4]; /* same 4 bytes, addressable one at a time */
};

/* --- section 4: register overlay ------------------------------------ */

/* MODER is sixteen 2-bit slices; we only name what we use. The container
 * must stay volatile — the union changes the NAMES, not lesson 1's rules. */
typedef union {
  struct {
    uint32_t pin0 : 2, pin1 : 2, pin2 : 2, pin3 : 2, pin4 : 2, pin5 : 2;
    uint32_t rest : 20;
  } f;
  uint32_t word;
} moder_bits_t;

typedef union {
  struct {
    uint32_t od0 : 1, od1 : 1, od2 : 1, od3 : 1, od4 : 1, od5 : 1;
    uint32_t rest : 26;
  } f;
  uint32_t word;
} odr_bits_t;

#define MODER_A ((volatile moder_bits_t *)&GPIOA->MODER)
#define ODR_A ((volatile odr_bits_t *)&GPIOA->ODR)

static void spin(volatile uint32_t n)
{
  while (n--) {
  }
}

void playing_with_bitfield(void)
{
  uart2_init();

  printf("\r\n== bit-fields & unions ==\r\n");

  /* 1) a slice can only hold its width: extra high bits are dropped. */
  printf("1) truncation\r\n");
  union packet p = {.word = 0};
  /* via variables so the compiler can't warn at the assignment itself —
   * with literals GCC flags the overflow, which is the lesson in itself */
  uint32_t nine = 9, twohundred = 200;
  p.f.rw = nine;       /* 1-bit slice keeps 9 & 0x1 */
  p.f.addr = twohundred; /* 5-bit slice keeps 200 & 0x1F = 8 */
  printf("   rw = 9  -> stored %lu;  addr = 200 -> stored %lu\r\n",
         (unsigned long)p.f.rw, (unsigned long)p.f.addr);

  /* 2) bit-fields are compiled shift-and-mask — prove it by hand. */
  printf("2) bit-field vs manual shift+mask\r\n");
  p.word = 0;
  p.f.addr = 19;
  p.f.rw = 1;
  p.f.len = 300;
  uint32_t manual = (19u << 0) | (1u << 5) | (300u << 6);
  printf("   fields word = 0x%08lx, manual = 0x%08lx [%s]\r\n",
         (unsigned long)p.word, (unsigned long)manual,
         p.word == manual ? "identical" : "MISMATCH");

  /* 3) byte view: how a word actually sits in memory. Little-endian ARM
   * stores the LOW byte first — what a byte-at-a-time UART send emits. */
  printf("3) union byte view of 0x12345678\r\n");
  union word_bytes w = {.value = 0x12345678};
  printf("   bytes[0..3] = %02x %02x %02x %02x -> little-endian\r\n",
         w.bytes[0], w.bytes[1], w.bytes[2], w.bytes[3]);

  /* 4) the forbidden fruit: bit-fields on a live register. */
  printf("4) GPIO through bit-field names\r\n");
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

  /* One assignment writes the 2-bit slice: 01 = output. What it compiles
   * to is LDR whole MODER, mask, ORR, STR whole MODER — a hidden
   * read-modify-write of all 16 pins' modes. That expansion is why this
   * style is fine to *understand* and risky to *ship*:
   *   - not atomic: an ISR flipping another pin's bit between the LDR and
   *     STR gets overwritten (the isr.c lesson's BSRR exists for this)
   *   - on write-1-to-clear registers it's destructive: `pr.pif13 = 1` on
   *     EXTI->PR1 would read every pending line back and ack them ALL —
   *     the exact |= bug the ISR lesson warns about, dressed up nicer
   *   - which bit each field lands on, and the bus access width used,
   *     are the compiler's choice, not yours — CMSIS masks make both
   *     explicit and portable */
  MODER_A->f.pin5 = 1;
  printf("   moder.pin5 = 1 -> CMSIS reads MODER[11:10] = %lu\r\n",
         (unsigned long)((GPIOA->MODER >> 10) & 3u));

  for (int i = 0; i < 3; i++) {
    ODR_A->f.od5 = 1;
    printf("   od5 = 1 -> CMSIS reads IDR bit5 = %lu\r\n",
           (unsigned long)((GPIOA->IDR >> 5) & 1u));
    spin(150000);
    ODR_A->f.od5 = 0;
    printf("   od5 = 0 -> CMSIS reads IDR bit5 = %lu\r\n",
           (unsigned long)((GPIOA->IDR >> 5) & 1u));
    spin(150000);
  }

  printf("done\r\n");
}
