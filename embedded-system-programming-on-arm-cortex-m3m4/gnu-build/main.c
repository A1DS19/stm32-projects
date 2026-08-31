/* Lesson: the bare-metal build — our own startup file, our own linker
 * script, and a Makefile that shows every command (slides 277-344).
 *
 * Every lesson so far was built by CMake with ST's startup_stm32l476xx.S
 * and stm32l476xg_flash.ld doing the invisible part: putting the vector
 * table at 0x08000000, copying initialized globals into RAM, zeroing the
 * rest, calling main. This folder does all of it by hand:
 *
 *   Makefile        — the build, stage by stage, every flag explained
 *   startup.c       — the vector table and Reset_Handler, in C
 *   stm32l476rg.ld  — the linker script, from scratch, every command explained
 *   main.c          — this program: proves on the board that it all worked
 *
 * The idea underneath: a program is made of SECTIONS. The compiler sorts
 * each .c file's contents into .text (code), .rodata (constants), .data
 * (initialized globals) and .bss (uninitialized globals), each starting
 * at address 0 in its .o file. The linker merges the sections of every
 * .o and gives them real addresses using the linker script. FLASH keeps
 * its contents through a power cycle, SRAM does not — so .data has TWO
 * addresses: where its initial values are stored (FLASH, the "load
 * address") and where the program uses it (SRAM, the "run address"), and
 * somebody has to copy between them before main. That somebody is our
 * Reset_Handler. .bss never touches FLASH at all: a range of SRAM zeroed
 * at reset.
 *
 * `make analyze` shows the same story from the host side: objdump on a
 * .o (sections at address 0, calls as relocations), readelf on the ELF
 * (sections placed, LOAD segments), nm for the symbols, and the map.
 *
 * Section 12 (slides 345-378) reuses this folder: the SAME program linked
 * four ways — full newlib, newlib-nano (`make nano`), with dead code
 * removed (`make gc`), and printing through the debugger instead of the
 * UART (`make semi`, semihosting) — `make compare` puts the four sizes
 * side by side. printf is the thread that ties them: it ends in a tiny
 * function called _write(), and WHO provides _write() decides where the
 * text goes. ../Src/syscall.c hands it to uart2 (USART2); librdimon's
 * version executes BKPT 0xAB and lets OpenOCD print it on the host. Two
 * scripted OpenOCD runs close the course: `make inspect` reads our memory
 * through the debug port (the AHB-AP) and watches Reset_Handler overwrite
 * garbage the debugger planted; `make run-semi` shows the semihosting
 * trap stopping the core dead when nobody is listening, then the same
 * text flowing once `arm semihosting enable` is on.
 *
 * When you'll use this: every board bring-up starts with a startup file
 * and a linker script — a new chip, a new memory layout, a bootloader
 * that must leave room for the application, code that has to run from
 * RAM, a variable pinned to a fixed address the debugger or another core
 * can find. And the bugs: "my global has garbage in it" (.data never
 * copied), "my table of zeros costs 8 KB of flash" (it was initialized
 * to 0 explicitly and became .data instead of .bss), "it links but
 * crashes before main" (vector[0] or [1] wrong, or _estack in the wrong
 * memory), "undefined reference to _init" (the -nostartfiles trap), "my
 * float prints as nothing" (newlib-nano without -u _printf_float), "it
 * hangs at the first printf in the field" (semihosting left on with no
 * debugger attached), the 30 KB nobody knew --gc-sections would save.
 * All of them are in this folder, on purpose or by explanation. */

#include <stdint.h>
#include <stdio.h>

/* ---- section 12: who answers printf's _write()? ----
 * SEMIHOSTING builds link librdimon (--specs=rdimon.specs): its _write()
 * is a BKPT 0xAB the debugger services, and it must be told to open
 * stdout first. Normal builds use ../Src/syscall.c → uart2. */
#ifdef SEMIHOSTING
extern void initialise_monitor_handles(void);
static const char console_name[] = "semihosting: printf → librdimon _write → BKPT 0xAB → OpenOCD";
#else
    #include "uart2.h"
static const char console_name[] = "USART2: printf → syscall.c _write → uart2 __io_putchar";
#endif

static void console_init(void) {
#ifdef SEMIHOSTING
    initialise_monitor_handles();
#else
    uart2_init();
#endif
}

/* ---- the linker script's symbols (names for addresses) ---- */
extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss, _end, _etext;
extern const uint32_t vectors[]; /* startup.c's table */

#define NVIC_ISER0 (*(volatile uint32_t*)0xE000E100)
#define NVIC_ISPR0 (*(volatile uint32_t*)0xE000E200)
#define IRQ_RCC 5U /* has no handler in this program — main.c pends it on purpose */

/* ---- the slides' variable table, as real variables ---- */
int global_init = 42;             /* .data: initialized global */
int global_uninit[64];            /* .bss: uninitialized global, 256 bytes of SRAM, 0 of FLASH */
const char rodata_text[] = "hi!"; /* .rodata: constant, stays in FLASH */

/* ---- section 12: is __libc_init_array real? ----
 * A function marked constructor is listed in .init_array — the table our
 * linker script collects between __init_array_start and __init_array_end
 * — and __libc_init_array, called from Reset_Handler, runs it before
 * main. The mark lives in .bss: zeroed by our loop first, then set here. */
static uint32_t constructor_mark;

__attribute__((constructor)) static void before_main(void) {
    constructor_mark = 0xC0FFEEU;
}

/* ---- section 12: bait for --gc-sections ----
 * Nobody calls or reads these. A plain link keeps them anyway: the linker
 * works in whole input sections and main.o's .text is ONE section. With
 * -ffunction-sections/-fdata-sections each gets a section of its own and
 * --gc-sections drops them (`make analyze-gc`). Not static, so the
 * compiler emits them without complaint. */
void unused_helper(void);
void unused_helper(void) {
    global_uninit[0]++;
}
const uint32_t unused_table[256] = {[0] = 1, [255] = 256}; /* 1 KB of .rodata */

static const char* where(const void* addr) {
    uint32_t a = (uint32_t)addr;
    if (a >= 0x08000000U && a < 0x08100000U) {
        return "FLASH";
    }
    if (a >= 0x20000000U && a < 0x20018000U) {
        return "SRAM1";
    }
    return "?";
}

static uint32_t read_msp(void) {
    uint32_t value;
    __asm volatile("MRS %0, MSP" : "=r"(value));
    return value;
}

static void show_map(void) {
    printf("\r\n1) the map, from the symbols our linker script defined:\r\n");
    printf(
        "   .text  0x%08lx..0x%08lx  %-5s %5lu bytes — vector table first, then code + rodata\r\n",
        (unsigned long)(uintptr_t)vectors, (unsigned long)(uintptr_t)&_etext, where(vectors),
        (unsigned long)((uintptr_t)&_etext - (uintptr_t)vectors));
    printf("   .data  load 0x%08lx (FLASH) -> run 0x%08lx..0x%08lx (%s) %lu bytes, copied by "
           "Reset_Handler\r\n",
           (unsigned long)(uintptr_t)&_sidata, (unsigned long)(uintptr_t)&_sdata,
           (unsigned long)(uintptr_t)&_edata, where(&_sdata),
           (unsigned long)((uintptr_t)&_edata - (uintptr_t)&_sdata));
    printf("   .bss   0x%08lx..0x%08lx  %-5s %5lu bytes, zeroed by Reset_Handler, 0 bytes of "
           "FLASH\r\n",
           (unsigned long)(uintptr_t)&_sbss, (unsigned long)(uintptr_t)&_ebss, where(&_sbss),
           (unsigned long)((uintptr_t)&_ebss - (uintptr_t)&_sbss));
    printf("   heap   from 0x%08lx (_end) — malloc grows upward from here (sysmem.c)\r\n",
           (unsigned long)(uintptr_t)&_end);
    printf("   stack  top 0x%08lx (_estack = vector[0]); MSP right now 0x%08lx, growing down\r\n",
           (unsigned long)(uintptr_t)&_estack, (unsigned long)read_msp());
}

static void show_variables(void) {
    static int local_static = 9; /* static storage: .data, not the stack */
    int local = 5;               /* the stack: born when this function runs */

    printf("\r\n2) the slides' variable table, live (address -> memory -> section):\r\n");
    printf("   int global_init = 42         @0x%08lx %s  .data   (value came from FLASH)\r\n",
           (unsigned long)(uintptr_t)&global_init, where(&global_init));
    printf("   int global_uninit[64]        @0x%08lx %s  .bss    (no FLASH image, zeroed)\r\n",
           (unsigned long)(uintptr_t)global_uninit, where(global_uninit));
    printf("   const char rodata_text[]     @0x%08lx %s  .rodata (read in place, never copied)\r\n",
           (unsigned long)(uintptr_t)rodata_text, where(rodata_text));
    printf("   static int local_static = 9  @0x%08lx %s  .data   (static = same as a global)\r\n",
           (unsigned long)(uintptr_t)&local_static, where(&local_static));
    printf(
        "   int local = 5                @0x%08lx %s  stack   (below _estack, gone on return)\r\n",
        (unsigned long)(uintptr_t)&local, where(&local));
}

static void show_data_and_bss_proof(void) {
    /* the FLASH image of global_init sits at the same offset inside the
     * load copy as the variable inside the run copy */
    const uint32_t* flash_image = &_sidata + (&global_init - (int*)&_sdata);

    printf("\r\n3) proof the copy happened: global_init reads %d at 0x%08lx (SRAM1)\r\n",
           global_init, (unsigned long)(uintptr_t)&global_init);
    printf("   and its FLASH image at 0x%08lx (_sidata + same offset) reads %lu — the same\r\n",
           (unsigned long)(uintptr_t)flash_image, (unsigned long)*flash_image);
    printf("   42 in two places, one of them put there by our Reset_Handler\r\n");
    printf("   .bss: global_uninit[63] = %d — zeroed by our loop, not a byte of FLASH spent\r\n",
           global_uninit[63]);
}

static void show_vector_table(void) {
    const volatile uint32_t* flash = (const uint32_t*)0x08000000;

    printf("\r\n4) the vector table at 0x08000000 is OUR array (&vectors = 0x%08lx):\r\n",
           (unsigned long)(uintptr_t)vectors);
    printf("   [0] = 0x%08lx = &_estack   [1] = 0x%08lx = Reset_Handler with the T bit\r\n",
           (unsigned long)flash[0], (unsigned long)flash[1]);
    printf("   the core loaded [0] into MSP and jumped to [1] — resetseq.c's protocol,\r\n");
    printf("   served this time by a C array with a section attribute\r\n");
}

static void show_constructor(void) {
    printf("\r\n6) __libc_init_array ran the .init_array table before main: constructor_mark = "
           "0x%06lX\r\n",
           (unsigned long)constructor_mark);
    printf("   (a .bss word: zeroed by Reset_Handler's loop, set by a constructor, read here)\r\n");
}

int main(void) {
    console_init();

    printf("\r\nLesson: the bare-metal build — our own startup file and linker script, no IDE, no "
           "CMake\r\n");
    printf("console: %s\r\n", console_name);

    show_map();
    show_variables();
    show_data_and_bss_proof();
    show_vector_table();
    show_constructor();

    printf("\r\n5) an interrupt nobody wrote a handler for: enable + pend IRQ %lu (RCC).\r\n",
           (unsigned long)IRQ_RCC);
    printf("   Its vector is a weak alias of Default_Handler, which reports and parks:\r\n");
    NVIC_ISER0 = 1UL << IRQ_RCC;
    NVIC_ISPR0 = 1UL << IRQ_RCC;

    for (;;) {} /* never reached: Default_Handler parks first */
}
