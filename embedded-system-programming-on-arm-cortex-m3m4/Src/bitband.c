/* Lesson: bit-banding (slides 70-78, exercise s76).
 *
 * Last lesson's map had a footnote: the FIRST 1 MB of the SRAM region and
 * the FIRST 1 MB of the peripheral region each get a 32 MB shadow zone —
 * the "bit-band alias":
 *
 *   SRAM       0x2000_0000..0x200F_FFFF  -> alias 0x2200_0000..0x23FF_FFFF
 *   peripheral 0x4000_0000..0x400F_FFFF  -> alias 0x4200_0000..0x43FF_FFFF
 *
 * In the alias, every 32-bit WORD stands for one single BIT of the real
 * memory (1 MB of bits = 8 million bits = 32 MB of words — that's where
 * the 32x blow-up comes from). Write 0 or 1 to the alias word and the
 * hardware flips just that bit underneath; read it and you get that bit
 * as 0 or 1. The address math:
 *
 *   alias = alias_base + (byte_offset * 32) + (bit * 4)
 *
 * Why care? The usual way to clear one bit is three instructions:
 * LOAD the whole byte, AND away the bit, STORE it back ("read-modify-
 * write"). If an interrupt fires between those steps and touches the
 * same byte, its change gets overwritten by our stale STORE. The alias
 * write is ONE store — the hardware does the read-modify-write inside
 * the bus, nothing can wedge in between ("atomic"). Same medicine as
 * GPIO's BSRR register in the 7-segment demo, but generalized to every
 * bit of low SRAM and low peripheral space.
 *
 * The slide exercise says "use address 0x2000_0200" — but the memory-map
 * lesson showed our own .data/.bss occupy that area (a .data variable
 * printed @0x2000_0548). Poking a raw address would stomp whatever the
 * linker put there, so we run the exercise on a variable we own and let
 * the linker pick its address — the same dodge the inline-assembly
 * lesson used. The demo peeks at 0x2000_0200 read-only to prove the
 * point.
 *
 * Bounds fine print, both ends anchored in earlier lessons: SRAM2 at
 * 0x1000_0000 (CODE region) is OUTSIDE the bandable megabyte, and so is
 * every GPIO port — ST parked them at 0x4800_0000 (AHB2), past the
 * peripheral megabyte. USART2 at 0x4000_4400 (APB1) is inside, so the
 * demo reads one of its status bits through the alias too. */

#include "bitband.h"

#include "uart2.h"

#include <stdint.h>
#include <stdio.h>

#define SRAM_ALIAS(addr, bit) (0x22000000U + (((addr) - 0x20000000U) * 32U) + ((bit) * 4U))
#define PERIPH_ALIAS(addr, bit) (0x42000000U + (((addr) - 0x40000000U) * 32U) + ((bit) * 4U))

static volatile uint8_t bucket; /* the exercise target, placed by the linker */

void playing_with_bit_banding(void) {
    uint32_t bucket_addr = (uint32_t)(uintptr_t)&bucket;
    uint32_t alias_addr = SRAM_ALIAS(bucket_addr, 7);
    volatile uint32_t* alias_bit7 = (uint32_t*)alias_addr;
    volatile uint32_t* alias_bit0 = (uint32_t*)SRAM_ALIAS(bucket_addr, 0);

    uart2_init();

    printf("\r\nLesson: bit-banding — a 32MB shadow where every word is one bit\r\n");

    /* why not the slide's literal 0x2000_0200: someone already lives there */
    printf("slide says poke 0x20000200 raw; it holds 0x%08lx — that's OUR\r\n",
           (unsigned long)*(volatile uint32_t*)0x20000200U);
    printf("  .data/.bss (memmap lesson), so the exercise runs on &bucket instead\r\n");

    printf("\r\nexercise s76, victim @0x%08lx:\r\n", (unsigned long)bucket_addr);

    /* round 1: the usual read-modify-write */
    bucket = 0xFF;
    printf("stored 0xFF, the usual way: bucket &= ~(1<<7)\r\n");
    bucket &= (uint8_t)~(1U << 7U);
    printf("  bucket = 0x%02X — but that was LDRB, BIC, STRB: three steps\r\n", bucket);
    printf("  an interrupt between them could get its own write overwritten\r\n");

    /* round 2: same job through the alias */
    bucket = 0xFF;
    printf("stored 0xFF again, now via the alias:\r\n");
    printf("  alias = 0x22000000 + (0x%08lx - 0x20000000)*32 + 7*4 = 0x%08lx\r\n",
           (unsigned long)bucket_addr, (unsigned long)alias_addr);
    *alias_bit7 = 0;
    printf("  wrote 0 there -> bucket = 0x%02X — ONE store, atomic\r\n", bucket);

    /* the alias reads back single bits too */
    printf("reading the alias: bit0 word = %lu, bit7 word = %lu (0x7F = 0111 1111)\r\n",
           (unsigned long)*alias_bit0, (unsigned long)*alias_bit7);

    /* peripheral flavor: USART2 is inside the bandable megabyte. CR1 bit 0
     * is UE, the enable bit our own uart2_init() set — must read 1 */
    volatile uint32_t* ue_alias = (uint32_t*)PERIPH_ALIAS(0x40004400U, 0);
    printf("peripheral alias: USART2 CR1 bit0 (UE) via 0x%08lx reads %lu —\r\n",
           (unsigned long)(uintptr_t)ue_alias, (unsigned long)*ue_alias);
    printf("  the very enable bit uart2_init set, seen one bit at a time\r\n");
    printf("GPIO can't play: ST put it at 0x48000000, past the bandable MB —\r\n");
    printf("which is exactly why BSRR exists (the 7-segment demo's workhorse)\r\n");
}
