/* Lesson: structs — from memory layout to memory-mapped peripherals.
 *
 * A struct is a layout contract: members sit one after another at growing
 * offsets. The compiler also inserts invisible filler bytes, called
 * PADDING, so each member starts at an address it likes: a uint32_t on a
 * multiple of 4, a uint16_t on a multiple of 2. That is "natural
 * alignment" — the memory bus can fetch an aligned value in one go, so
 * padding trades wasted bytes for fast access. Shown below:
 *   - member order matters: a bad order can nearly double sizeof
 *   - sizeof is also rounded up so arrays of the struct stay aligned
 *   - __attribute__((packed)) removes the padding — and the speed with it
 *
 * Bit-fields carve an integer into named slices a few bits wide — good
 * for squeezing a message or a settings blob into little RAM. But the C
 * standard does not fix WHICH bit each slice lands on (compilers may
 * differ), so CMSIS never uses them for registers: register code uses
 * masks and shifts (GPIO_MODER_MODE5_0 and friends) instead.
 *
 * The embedded payoff: point a struct at a hardware address and every
 * member becomes a register. That is the whole trick behind CMSIS —
 *   #define GPIOA ((GPIO_TypeDef *) 0x48000000)
 * GPIO_TypeDef has only uint32_t members, so no padding is possible and
 * each member's offset matches the reference manual table exactly. Every
 * member is __IO, which is just volatile (lesson 1!). Below we build our
 * own mini GPIO_TypeDef, check its offsets against CMSIS, and blink LD2
 * with it — writes go through OUR struct, read-back through CMSIS, which
 * proves both names point at the same silicon. */

#include "struct.h"
#include "stm32l476xx.h"
#include "uart2.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* --- 1) padding ---------------------------------------------------- */

struct naive_order {
  uint8_t flag;   /* offset 0 — 3 filler bytes follow so count can align */
  uint32_t count; /* offset 4 */
  uint16_t id;    /* offset 8 — 2 filler bytes at the end round sizeof to 12 */
};

struct sorted_order { /* same members, widest first: no gaps, 1 end filler */
  uint32_t count;
  uint16_t id;
  uint8_t flag;
};

struct __attribute__((packed)) packed_order { /* no filler at all */
  uint8_t flag;
  uint32_t count; /* now at offset 1 — every access to it is unaligned */
  uint16_t id;
};

/* --- 2) bit-fields -------------------------------------------------- */

union sensor_report {
  struct {
    uint32_t sensor_id : 4; /* 0..15  */
    uint32_t channel : 3;   /* 0..7   */
    uint32_t overrun : 1;
    uint32_t sample : 12;   /* 0..4095 */
    uint32_t reserved : 12; /* name the leftover bits, like the reference
                             * manual does. Unnamed leftover bits are NOT
                             * filled in by an initializer (we watched
                             * 0x08 of stack garbage land there) — a
                             * named member is set to zero. */
  } f;                      /* 20 bits used + 12 reserved = one uint32_t */
  uint32_t word;            /* union: the same bytes under a second name —
                             * lets us look at the raw packed value */
};

/* --- 3) a register map, hand-rolled --------------------------------- */

/* Offsets copied from the GPIO register table in RM0351 (the reference
 * manual). All members are uint32_t, each naturally aligned, so the
 * compiler CANNOT insert padding — the offset inside the struct equals
 * the address offset in the hardware. volatile is what CMSIS's __IO
 * expands to. (The real GPIO_TypeDef continues with LCKR, AFR[2] — yes,
 * arrays work in register maps too — and BRR; left out, we don't use
 * them here.) */
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

  /* Blink LD2 using only the homemade struct; read the pin back through
   * CMSIS. (The clock enable stays CMSIS: RCC would need a whole
   * register map of its own.) */
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
  MY_GPIOA->MODER &= ~GPIO_MODER_MODE5;
  MY_GPIOA->MODER |= GPIO_MODER_MODE5_0;

  for (int i = 0; i < 3; i++) {
    /* BSRR = Bit Set/Reset Register: write-only. A 1 in the low half
     * sets that pin, a 1 in the high half clears it. One plain write —
     * no need to read the old value, change it, and write it back the
     * way ODR requires. */
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
