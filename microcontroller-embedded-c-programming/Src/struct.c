/* Lesson: structs — from memory layout to memory-mapped peripherals.
 *
 * A struct is a layout contract: members sit at increasing offsets, and the
 * compiler inserts PADDING so each member lands on its natural alignment
 * (a uint32_t on an address divisible by 4, uint16_t by 2...). Aligned
 * accesses are what the bus does in one cycle; the padding buys speed with
 * wasted bytes. Rules of thumb shown below:
 *   - member order matters: worst-first ordering can nearly double sizeof
 *   - sizeof is also rounded up to the widest alignment (arrays must tile)
 *   - __attribute__((packed)) removes padding — pay with unaligned access
 *
 * Bit-fields carve a member into sub-byte slices — great for squeezing a
 * protocol packet or config blob into RAM. But WHICH bit each field lands in
 * is implementation-defined, so CMSIS never uses them for registers: real
 * register code uses masks and shifts (GPIO_MODER_MODE5_0 & friends).
 *
 * The embedded payoff: point a struct at a hardware address and every member
 * becomes a register. That is the whole trick behind CMSIS —
 *   #define GPIOA ((GPIO_TypeDef *) 0x48000000)
 * GPIO_TypeDef is only uint32_t members (no padding possible, offsets match
 * the reference manual table exactly), each declared __IO = volatile
 * (lesson 1!). Below we roll our own mini GPIO_TypeDef, verify member
 * offsets against CMSIS, and blink LD2 through it — writes go through OUR
 * struct, read-back through CMSIS, proving both name the same silicon. */

#include "struct.h"
#include "stm32l476xx.h"
#include "uart2.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* --- 1) padding ---------------------------------------------------- */

struct naive_order {
  uint8_t flag;   /* offset 0 — compiler pads 3 bytes so count aligns */
  uint32_t count; /* offset 4 */
  uint16_t id;    /* offset 8 — 2 tail pad bytes round sizeof to 12 */
};

struct sorted_order { /* same members, widest first: no gaps, tail pad 1 */
  uint32_t count;
  uint16_t id;
  uint8_t flag;
};

struct __attribute__((packed)) packed_order { /* no padding at all */
  uint8_t flag;
  uint32_t count; /* now at offset 1 — every access is unaligned */
  uint16_t id;
};

/* --- 2) bit-fields -------------------------------------------------- */

union sensor_report {
  struct {
    uint32_t sensor_id : 4; /* 0..15  */
    uint32_t channel : 3;   /* 0..7   */
    uint32_t overrun : 1;
    uint32_t sample : 12;   /* 0..4095 */
    uint32_t reserved : 12; /* name the leftover bits, like RM tables do:
                             * unnamed padding bits are UNINITIALIZED (we
                             * watched 0x08 stack garbage land there) —
                             * a named member zero-fills with {.f = ...} */
  } f;                      /* 20 bits used + 12 reserved = one uint32_t */
  uint32_t word;            /* union: same bytes, second name — lets us
                             * peek at the raw packed representation */
};

/* --- 3) a register map, hand-rolled --------------------------------- */

/* Offsets straight from RM0351's GPIO register table. All members are
 * uint32_t: naturally aligned, so the compiler CANNOT pad — offset in the
 * struct == address offset in hardware. volatile is what CMSIS's __IO
 * expands to. (Real GPIO_TypeDef continues: LCKR, AFR[2] — yes, arrays
 * work in register structs too — and BRR; omitted, we don't use them.) */
struct my_gpio {
  volatile uint32_t MODER;   /* 0x00 */
  volatile uint32_t OTYPER;  /* 0x04 */
  volatile uint32_t OSPEEDR; /* 0x08 */
  volatile uint32_t PUPDR;   /* 0x0C */
  volatile uint32_t IDR;     /* 0x10 */
  volatile uint32_t ODR;     /* 0x14 */
  volatile uint32_t BSRR;    /* 0x18 */
};

#define MY_GPIOA ((struct my_gpio *)0x48000000u)

static void spin(volatile uint32_t n)
{
  while (n--) {
  }
}

void playing_with_struct(void)
{
  uart2_init();

  printf("\r\n== struct: layout, padding, and register maps ==\r\n");

  printf("1) padding\r\n");
  printf("   naive {u8,u32,u16}: sizeof = %lu (7 data + 5 pad)\r\n",
         (unsigned long)sizeof(struct naive_order));
  printf("     flag  @ %lu, count @ %lu, id @ %lu\r\n",
         (unsigned long)offsetof(struct naive_order, flag),
         (unsigned long)offsetof(struct naive_order, count),
         (unsigned long)offsetof(struct naive_order, id));
  printf("   widest-first:       sizeof = %lu\r\n",
         (unsigned long)sizeof(struct sorted_order));
  printf("   packed:             sizeof = %lu (unaligned members)\r\n",
         (unsigned long)sizeof(struct packed_order));

  printf("2) bit-fields\r\n");
  union sensor_report r = {.f = {.sensor_id = 9,
                                 .channel = 5,
                                 .overrun = 1,
                                 .sample = 1234}};
  printf("   4+3+1+12 = 20 bits -> sizeof = %lu\r\n",
         (unsigned long)sizeof(union sensor_report));
  printf("   id=9 ch=5 ovr=1 sample=1234 packs to word = 0x%08lx\r\n",
         (unsigned long)r.word);

  printf("3) my_gpio vs CMSIS GPIO_TypeDef\r\n");
  printf("   offsets mine/CMSIS: IDR 0x%02lx/0x%02lx  ODR 0x%02lx/0x%02lx  "
         "BSRR 0x%02lx/0x%02lx\r\n",
         (unsigned long)offsetof(struct my_gpio, IDR),
         (unsigned long)offsetof(GPIO_TypeDef, IDR),
         (unsigned long)offsetof(struct my_gpio, ODR),
         (unsigned long)offsetof(GPIO_TypeDef, ODR),
         (unsigned long)offsetof(struct my_gpio, BSRR),
         (unsigned long)offsetof(GPIO_TypeDef, BSRR));
  printf("   &MY_GPIOA->ODR = 0x%08lx, &GPIOA->ODR = 0x%08lx [%s]\r\n",
         (unsigned long)(uintptr_t)&MY_GPIOA->ODR,
         (unsigned long)(uintptr_t)&GPIOA->ODR,
         (uintptr_t)&MY_GPIOA->ODR == (uintptr_t)&GPIOA->ODR ? "identical"
                                                             : "MISMATCH");

  /* Blink LD2 entirely through the homemade struct; read the pin back
   * through CMSIS. Clock gating stays CMSIS — RCC would be its own map. */
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
  MY_GPIOA->MODER &= ~GPIO_MODER_MODE5;
  MY_GPIOA->MODER |= GPIO_MODER_MODE5_0;

  for (int i = 0; i < 3; i++) {
    /* BSRR = Bit Set/Reset Register: write-only, low half sets pins, high
     * half resets them — atomic, no read-modify-write like ODR needs. */
    MY_GPIOA->BSRR = GPIO_BSRR_BS5;
    printf("   set via my struct   -> CMSIS reads IDR bit5 = %lu\r\n",
           (unsigned long)((GPIOA->IDR >> 5) & 1u));
    spin(150000);
    MY_GPIOA->BSRR = GPIO_BSRR_BR5;
    printf("   clear via my struct -> CMSIS reads IDR bit5 = %lu\r\n",
           (unsigned long)((GPIOA->IDR >> 5) & 1u));
    spin(150000);
  }

  printf("done\r\n");
}
