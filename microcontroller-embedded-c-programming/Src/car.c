/* Lesson: modeling real data with structs — a car.
 *
 * The method: for each field, first pick the UNIT and the RANGE that unit
 * must cover, then choose the smallest type that fits with some headroom.
 *
 *   max speed  -> km/h fits uint8_t (0..255) for these cars. Know the
 *                 limit: a Bugatti does 490 km/h and would wrap around to
 *                 a wrong small number. If the data could ever include
 *                 one, uint16_t is the honest choice.
 *   max revs   -> rpm goes up to ~9000, too big for a raw uint8_t. So
 *                 store a SCALED unit: rpm/100 (75 means 7500). The
 *                 display multiplies back. Why /100 and not /1000: a
 *                 7500 rpm redline exists, and thousands can only say
 *                 7000 or 8000.
 *   model name -> a small code that points into a fixed table — exactly
 *                 what enum + an array of strings is for. The code costs
 *                 1 byte per car; the text is stored once, in flash.
 *   vin        -> NOT a number. A real VIN is 17 letters-and-digits
 *                 ("1HGES16564L012345"), so it is text: char[17+1], the
 *                 +1 for the NUL byte that ends every C string. A
 *                 uint16_t stops at 65535 — five digits — and no integer
 *                 type can hold letters at all.
 *
 * Layout note (last lesson): every member here is 1 byte wide, so there
 * is ZERO padding whatever the order — sizeof = sum of the members. */

#include "car.h"
#include "uart2.h"
#include <stdint.h>
#include <stdio.h>

enum car_model {
  MODEL_CIVIC,
  MODEL_GOLF_GTI,
  MODEL_MX5,
  MODEL_COUNT,
};

/* The "predefined set of names" the 1-byte code indexes into. */
static const char *const model_names[MODEL_COUNT] = {
    [MODEL_CIVIC] = "Honda Civic",
    [MODEL_GOLF_GTI] = "VW Golf GTI",
    [MODEL_MX5] = "Mazda MX-5",
};

struct car {
  char vin[18];            /* 17 chars + the ending NUL */
  uint8_t max_speed_kmh;   /* km/h, honest range 0..255 */
  uint8_t max_revs_100rpm; /* rpm / 100: 66 means 6600 rpm */
  uint8_t model;           /* enum car_model, kept to one byte */
};

/* Pass structs to functions by POINTER (4 bytes), not by value — by
 * value would copy all 21 bytes on every call. const says: read only. */
static void print_car(const struct car *c)
{
  printf("  %-11s  VIN %s  top %u km/h  redline %u rpm (stored: %u)\r\n",
         model_names[c->model], c->vin, c->max_speed_kmh,
         c->max_revs_100rpm * 100u, c->max_revs_100rpm);
}

void playing_with_car(void)
{
  uart2_init();

  static const struct car garage[] = {
      {.vin = "1HGES16564L012345",
       .max_speed_kmh = 200,
       .max_revs_100rpm = 66,
       .model = MODEL_CIVIC},
      {.vin = "WVWZZZ1KZ6W123456",
       .max_speed_kmh = 250,
       .max_revs_100rpm = 65,
       .model = MODEL_GOLF_GTI},
      {.vin = "JM1NDAB75H0123456",
       .max_speed_kmh = 219,
       .max_revs_100rpm = 75, /* 7500 — the value /1000 scaling can't say */
       .model = MODEL_MX5},
  };

  printf("\r\n== struct modeling: the garage ==\r\n");
  printf("sizeof(struct car) = %lu (18+1+1+1, zero padding), garage of %lu = %lu bytes\r\n",
         (unsigned long)sizeof(struct car),
         (unsigned long)(sizeof(garage) / sizeof(garage[0])),
         (unsigned long)sizeof(garage));

  for (uint32_t i = 0; i < sizeof(garage) / sizeof(garage[0]); i++) {
    print_car(&garage[i]);
  }

  printf("done\r\n");
}
