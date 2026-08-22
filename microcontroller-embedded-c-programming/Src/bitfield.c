/* Lesson: bit-fields & unions, the full story.
 *
 * A bit-field is a named slice of an integer. `uint32_t len : 10` means
 * "10 bits of the word", and every read or write of it compiles into the
 * same shift-and-mask you could write by hand (section 2 proves it).
 * Always declare them unsigned: a plain `int flag : 1` can hold 0 and
 * -1, not 0 and 1.
 *
 * A union is one box of bytes with several names on it. All members
 * start at offset 0 and SHARE the same storage; sizeof = the largest
 * member. Write through one name, read through another, and you see the
 * same bytes reinterpreted. Embedded code uses this constantly: a
 * word-view of packed fields, or a byte-view of a word to send it over
 * UART/CAN — which also reveals the CPU's byte order (endianness).
 *
 * Section 4 does what CMSIS on purpose does not: it lays bit-field names
 * over a live GPIO register and blinks the LED by assigning to a 1-bit
 * name. It works — and the comments there give the three reasons real
 * register code still uses masks, so you know the trade. */

#include "bitfield.h"
#include "stm32l476xx.h"
#include "uart2.h"
#include <stdint.h>
#include <stdio.h>

/* --- sections 1 & 2: slices in RAM ---------------------------------- */

/* One 4-byte box, two names for its contents:
 *   .f    = the labeled view — each member is a named slice of the bits
 *   .word = the unlabeled view — all 32 bits as one plain integer
 * These are NOT two variables. There is one storage; writing through
 * either name changes what the other name reads, instantly. Use .f when
 * you care about one field, .word when you handle the whole thing at
 * once: copy it, clear it, compare it, print it, send it. */
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
  uint8_t bytes[4]; /* the same 4 bytes, reachable one at a time */
};

/* --- section 4: register overlay ------------------------------------ */

/* MODER holds sixteen 2-bit slices, one per pin; we name only the ones
 * we use. The pointer must stay volatile — the union changes the NAMES
 * on the register, not lesson 1's rules. */
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

  /* 1) a slice can only hold as many bits as it is wide — the extra
   * high bits are simply dropped. */
  printf("1) truncation\r\n");
  union packet p = {.word = 0};
  /* through variables so the compiler can't warn at the assignment —
   * with plain literals GCC flags the overflow at compile time, which
   * is a lesson of its own */
  uint32_t nine = 9, twohundred = 200;
  p.f.rw = nine;         /* 1-bit slice keeps 9 & 0x1 */
  p.f.addr = twohundred; /* 5-bit slice keeps 200 & 0x1F = 8 */
  printf("   rw = 9  -> stored %lu;  addr = 200 -> stored %lu\r\n",
         (unsigned long)p.f.rw, (unsigned long)p.f.addr);

  /* 2) bit-fields ARE shift-and-mask — prove it by building the same
   * word by hand. */
  printf("2) bit-field vs manual shift+mask\r\n");
  p.word = 0;
  p.f.addr = 19;
  p.f.rw = 1;
  p.f.len = 300;
  uint32_t manual = (19u << 0) | (1u << 5) | (300u << 6);
  printf("   fields word = 0x%08lx, manual = 0x%08lx [%s]\r\n",
         (unsigned long)p.word, (unsigned long)manual,
         p.word == manual ? "identical" : "MISMATCH");

  /* 3) byte view: how the word really sits in memory. Little-endian ARM
   * stores the LOWEST byte first — the order a byte-by-byte UART send
   * would put on the wire. */
  printf("3) union byte view of 0x12345678\r\n");
  union word_bytes w = {.value = 0x12345678};
  printf("   bytes[0..3] = %02x %02x %02x %02x -> little-endian\r\n",
         w.bytes[0], w.bytes[1], w.bytes[2], w.bytes[3]);

  /* 4) bit-field names over a live register. */
  printf("4) GPIO through bit-field names\r\n");
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

  /* One assignment writes the 2-bit slice: 01 = output. But look at
   * what it compiles into: read the WHOLE 32-bit MODER, change 2 bits,
   * write the WHOLE register back — a hidden read-modify-write. That
   * expansion is why this style is good to understand and risky to
   * ship:
   *   - not atomic: if an interrupt changes another pin's bits between
   *     the read and the write-back, that change is overwritten — lost
   *     (this is why BSRR exists, see the struct lesson)
   *   - on write-1-to-clear registers it is destructive: `pr.pif13 = 1`
   *     on EXTI->PR1 would read ALL the pending flags back and
   *     acknowledge every one of them — the exact |= bug the ISR lesson
   *     warns about, in nicer clothes
   *   - the compiler decides which bit each field lands on and how wide
   *     the bus access is — not you. CMSIS masks state both explicitly,
   *     the same on every compiler. */
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
