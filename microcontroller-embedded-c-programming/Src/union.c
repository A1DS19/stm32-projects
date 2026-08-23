/* Lesson: unions — one storage, many names.
 *
 * A union is one box of bytes with several names on it. All members start
 * at offset 0 and share the bytes; sizeof = the largest member. Three
 * things this lesson shows that the earlier ones did not:
 *
 * 1) Members really do overwrite each other. A union holds ONE value at a
 *    time; writing through one name disturbs what the other names read.
 *    (A struct holds all its members at once — that is the difference.)
 *
 * 2) A union is a keyhole into how types are stored. We put a float in
 *    and read the raw bits out: the IEEE-754 format (sign, exponent,
 *    mantissa) with the bit-field tools from the last lessons. First
 *    float on this board — the FPU that SystemInit switches on at boot
 *    (see system.c) is what makes this and printf("%f") work.
 *
 * 3) The pattern real firmware uses daily: the TAGGED union. When several
 *    kinds of data are mutually exclusive (an event is a temperature OR a
 *    button press OR an error — never two at once), a struct would pay
 *    for all of them; a union pays only for the biggest, plus one small
 *    "tag" that records which member is the live one. Event queues and
 *    protocol messages are built exactly like this.
 *
 * Golden rule for every union: read only the member you last wrote — for
 * a tagged union, only the member the tag names. Anything else is bytes
 * reinterpreted as something they are not. */

#include "union.h"
#include "uart2.h"
#include <stdint.h>
#include <stdio.h>

/* --- 1) overlap ------------------------------------------------------ */

union three_views {
  uint8_t byte;  /* bits [7:0]            */
  uint16_t half; /* bits [15:0]           */
  uint32_t word; /* all 32                */
};

/* --- 2) float bits ---------------------------------------------------- */

/* IEEE-754 single precision: 1 sign bit, 8 exponent bits (stored with a
 * +127 offset, so 127 means 2^0), 23 mantissa bits (the digits after the
 * leading 1). The union lets us see all of it. */
union float_bits {
  float value;
  uint32_t bits;
  struct {
    uint32_t mantissa : 23;
    uint32_t exponent : 8;
    uint32_t sign : 1;
  } f;
};

/* --- 3) tagged union: an event queue --------------------------------- */

enum event_type { EVENT_TEMPERATURE, EVENT_BUTTON, EVENT_ERROR };

/* One event is exactly one of these three — never two at once. The union
 * sizes the storage for the biggest variant only; `type` is the tag that
 * says which name is real. */
struct event {
  uint8_t type; /* enum event_type */
  union {
    struct {
      int16_t celsius_x10; /* scaled unit, like the car lesson: 235 = 23.5 C */
      uint8_t sensor_id;
    } temperature;
    struct {
      uint8_t id;
      uint8_t long_press;
    } button;
    struct {
      uint16_t code;
    } error;
  } as;
};

/* For comparison only: the same variants as a plain struct — every event
 * would carry all three payloads, two of them always dead weight. */
struct event_wasteful {
  uint8_t type;
  struct { int16_t celsius_x10; uint8_t sensor_id; } temperature;
  struct { uint8_t id; uint8_t long_press; } button;
  struct { uint16_t code; } error;
};

static void print_float(const char *label, float value)
{
  union float_bits u = {.value = value};
  printf("   %s = %.2f -> bits 0x%08lX (sign %lu, exponent %lu, mantissa 0x%06lX)\r\n",
         label, value, (unsigned long)u.bits, (unsigned long)u.f.sign,
         (unsigned long)u.f.exponent, (unsigned long)u.f.mantissa);
}

void playing_with_union(void)
{
  uart2_init();

  printf("\r\n== union: one storage, many names ==\r\n");

  printf("1) members overlap — a union holds ONE value at a time\r\n");
  union three_views v;
  printf("   sizeof = %lu (the largest member, not the sum)\r\n",
         (unsigned long)sizeof(v));
  v.word = 0xAABBCCDD;
  printf("   word = 0xAABBCCDD -> byte reads 0x%02X, half reads 0x%04X"
         " (low bytes first: little-endian)\r\n", v.byte, v.half);
  v.byte = 0x11;
  printf("   write byte = 0x11 -> word is now 0x%08lX — same bytes!\r\n",
         (unsigned long)v.word);

  printf("2) a float through the keyhole (IEEE-754)\r\n");
  print_float(" 1.00", 1.0f);   /* 2^0 * 1.0   -> exponent 127+0  */
  print_float("-6.25", -6.25f); /* -2^2 * 1.5625 -> exponent 127+2 */

  printf("3) tagged union — the event queue pattern\r\n");
  printf("   biggest variant %lu, tagged event %lu; all-variants struct"
         " would be %lu\r\n",
         (unsigned long)sizeof(((struct event *)0)->as),
         (unsigned long)sizeof(struct event),
         (unsigned long)sizeof(struct event_wasteful));

  const struct event queue[] = {
      {.type = EVENT_TEMPERATURE,
       .as.temperature = {.celsius_x10 = 235, .sensor_id = 2}},
      {.type = EVENT_BUTTON, .as.button = {.id = 1, .long_press = 1}},
      {.type = EVENT_ERROR, .as.error = {.code = 0x0404}},
  };

  for (uint32_t i = 0; i < sizeof(queue) / sizeof(queue[0]); i++) {
    const struct event *e = &queue[i];
    /* The switch on the tag is the whole discipline: each case touches
     * only the member the tag names. */
    switch (e->type) {
    case EVENT_TEMPERATURE:
      printf("   [temperature] sensor %u: %d.%d C\r\n",
             e->as.temperature.sensor_id, e->as.temperature.celsius_x10 / 10,
             e->as.temperature.celsius_x10 % 10);
      break;
    case EVENT_BUTTON:
      printf("   [button] id %u, %s press\r\n", e->as.button.id,
             e->as.button.long_press ? "long" : "short");
      break;
    case EVENT_ERROR:
      printf("   [error] code 0x%04X\r\n", e->as.error.code);
      break;
    }
  }

  printf("done\r\n");
}
