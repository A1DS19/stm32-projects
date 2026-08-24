/* Lesson: memory map & bus interfaces (slides 54-69).
 *
 * The Cortex-M address bus is 32 bits wide, so the processor can point at
 * 2^32 = 4 GB of addresses. ARM fixes, for every Cortex-M ever made, what
 * each slice of that range is FOR — the "memory map":
 *
 *   0x0000_0000  CODE          512 MB  program memory (flash/ROM); the
 *                                      vector table is fetched here at reset
 *   0x2000_0000  SRAM          512 MB  RAM; first 1 MB bit-addressable
 *                                      (bit-banding — next lesson)
 *   0x4000_0000  Peripheral    512 MB  on-chip peripheral registers; first
 *                                      1 MB bit-addressable; eXecute Never
 *   0x6000_0000  External RAM    1 GB  off-chip memory you can run code from
 *   0xA000_0000  External dev    1 GB  off-chip devices; eXecute Never
 *   0xE000_0000  PPB             1 MB  Private Peripheral Bus: the core's
 *                                      own registers — NVIC, SysTick, SCB
 *   0xE010_0000  Vendor        511 MB  whatever the chip maker wants
 *
 * "eXecute Never" (XN) means the core refuses to run instructions fetched
 * from there — jumping into the peripheral region raises a fault (the
 * faults section will do it on purpose, exercise s186).
 *
 * ARM only draws the map. The MCU vendor — ST here — decides what
 * actually sits at which address, and publishes that in the reference
 * manual (RM0351 for our L476). The addresses this demo prints come from
 * ST's CMSIS header, stm32l476xx.h, which is that manual in #define form.
 *
 * The buses that carry all this traffic follow ARM's AMBA spec
 * ("Advanced Microcontroller Bus Architecture") in two flavors:
 *   AHB — the fast bus ("Advanced High-performance Bus"); memories and
 *         speed-hungry peripherals (GPIO ports) sit on it
 *   APB — the slower, simpler cousin ("Advanced Peripheral Bus"), reached
 *         through an AHB-to-APB bridge; most peripherals (USART, timers,
 *         I2C...) live here
 * The core itself has four bus ports, split by address range: I-CODE
 * (instruction fetches from the CODE region), D-CODE (data reads from the
 * CODE region — constants, the vector table), System (everything from
 * 0x2000_0000 up: SRAM, peripherals, external), and the internal PPB port.
 * Software can't watch those ports directly, but the address of every item
 * below tells you exactly which port and bus its access rode on.
 *
 * When you'll use this: constantly, silently. Every register address you
 * type, every linker-script region, every "why is this peripheral on
 * APB1" datasheet moment is a walk on this map. It's also crash triage:
 * a fault at 0x0800_xxxx is code, 0x2001_xxxx the stack, 0x4000_xxxx a
 * driver — the address alone points at the suspect. */

#include "memmap.h"

#include "stm32l476xx.h"
#include "uart2.h"

#include <stdint.h>
#include <stdio.h>

static uint32_t data_var = 0x11223344; /* has an initial value -> .data */
static uint32_t bss_var;               /* starts as zero -> .bss */

/* the top bits of any address name its ARM region — that's the whole map */
static const char* region_name(uint32_t addr) {
    if (addr < 0x20000000U) {
        return "CODE";
    }
    if (addr < 0x40000000U) {
        return "SRAM";
    }
    if (addr < 0x60000000U) {
        return "Peripheral";
    }
    if (addr < 0xA0000000U) {
        return "External RAM";
    }
    if (addr < 0xE0000000U) {
        return "External device";
    }
    if (addr < 0xE0100000U) {
        return "PPB";
    }
    return "Vendor";
}

/* inside the peripheral region, the address also names the ST bus it
 * hangs on — the boundaries are straight from stm32l476xx.h */
static const char* bus_name(uint32_t addr) {
    if (addr >= AHB2PERIPH_BASE) {
        return "AHB2";
    }
    if (addr >= AHB1PERIPH_BASE) {
        return "AHB1";
    }
    if (addr >= APB2PERIPH_BASE) {
        return "APB2";
    }
    return "APB1";
}

void exploring_the_memory_map(void) {
    uint32_t stack_local = 0;
    uint32_t code_addr = (uint32_t)(uintptr_t)&exploring_the_memory_map;
    volatile uint32_t* boot_alias = (uint32_t*)0x00000000;
    volatile uint32_t* cpuid = (uint32_t*)0xE000ED00; /* SCB CPUID */

    uart2_init();

    printf("\r\nLesson: memory map — ARM fixes the 4GB map, ST picks the tenants\r\n");

    printf("\r\nthis very program, located (region computed from each address):\r\n");
    printf("this function's code  @0x%08lx  %s — flash; bit0 is the Thumb bit\r\n",
           (unsigned long)code_addr, region_name(code_addr));
    printf("boot alias word[0]    @0x00000000  %s — = 0x%08lx, the initial MSP:\r\n",
           region_name(0x00000000U), (unsigned long)boot_alias[0]);
    printf("                      a CODE address holding an SRAM address (reset lesson)\r\n");
    printf(".data variable        @0x%08lx  %s — copied flash->RAM at boot\r\n",
           (unsigned long)(uintptr_t)&data_var, region_name((uint32_t)(uintptr_t)&data_var));
    printf(".bss variable         @0x%08lx  %s — zeroed at boot\r\n",
           (unsigned long)(uintptr_t)&bss_var, region_name((uint32_t)(uintptr_t)&bss_var));
    printf("a stack local         @0x%08lx  %s — near the top; stacks grow down\r\n",
           (unsigned long)(uintptr_t)&stack_local, region_name((uint32_t)(uintptr_t)&stack_local));

    printf("\r\nperipherals — the address alone names the bus:\r\n");
    printf("USART2 (printf's UART)@0x%08lx  %s, %s — behind the AHB->APB bridge\r\n",
           (unsigned long)USART2_BASE, region_name(USART2_BASE), bus_name(USART2_BASE));
    printf("RCC (clock switches)  @0x%08lx  %s, %s — fast bus\r\n", (unsigned long)RCC_BASE,
           region_name(RCC_BASE), bus_name(RCC_BASE));
    printf("GPIOA (PA2/PA3 pins)  @0x%08lx  %s, %s — fast bus\r\n", (unsigned long)GPIOA_BASE,
           region_name(GPIOA_BASE), bus_name(GPIOA_BASE));
    printf("SCB CPUID             @0xE000ED00  %s — the core's own block, = 0x%08lx\r\n",
           region_name(0xE000ED00U), (unsigned long)*cpuid);

    /* ST placed SRAM2, a second 32 KB RAM, at 0x1000_0000 — inside ARM's
     * CODE region. Our linker script knows it but places nothing there, so
     * poking it is safe. If the write reads back, it's RAM no matter what
     * the region is called: the map is zoning, not the building. */
    volatile uint32_t* sram2 = (uint32_t*)SRAM2_BASE;
    *sram2 = 0xCAFEBABEU;
    printf("\r\nST's twist: SRAM2 (32K)@0x%08lx  %s region — yet it's RAM:\r\n",
           (unsigned long)SRAM2_BASE, region_name(SRAM2_BASE));
    printf("wrote 0xcafebabe, read back 0x%08lx — regions say what MAY connect,\r\n",
           (unsigned long)*sram2);
    printf("the vendor decides what DOES. (CODE-region RAM can even run code.)\r\n");
}
