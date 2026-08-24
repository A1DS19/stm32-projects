/* Lesson: processor reset sequence (slides 49-53).
 *
 * What the core does on reset, in hardware, before one line of anyone's
 * code runs:
 *   1. reads the word at address 0x0000_0000 -> loads it into MSP
 *   2. reads the word at address 0x0000_0004 -> loads it into PC
 * That's the whole boot protocol. It's why a Cortex-M vector table does
 * NOT start with code: entry 0 is a stack pointer VALUE, entry 1 is the
 * address of the reset handler (with bit 0 set — the Thumb bit again).
 * The stack comes first because even a fault on the very first
 * instruction would need somewhere to push a stack frame.
 *
 * But our program is linked at 0x0800_0000 (flash), not 0. The STM32
 * fills the gap with boot aliasing: with the BOOT0 pin low (Nucleo
 * default), flash is mirrored at address 0, so the two fetches above land
 * in OUR vector table. This demo reads both copies to prove they match.
 *
 * From there Reset_Handler — real code, in our own
 * startup_stm32l476xx.S, finally explained — walks to main:
 *   sp = _estack          reload stack top (belt and braces)
 *   bl SystemInit         our Src/system.c: switch on the FPU
 *   copy .data            initialized globals, flash -> RAM
 *   zero .bss             uninitialized globals
 *   bl __libc_init_array  C library constructors
 *   bl main
 * The copy/zero steps get their own lessons in the linker-script section.
 *
 * Every printed pair below must agree: the vector table read as plain
 * memory vs the linker/startup symbols the build placed there.
 *
 * When you'll use this: bootloaders and firmware update. Jumping to an
 * application IS a hand-made reset sequence — read the app's vector[0]
 * into SP, branch to its vector[1], move VTOR. Board bring-up too: when
 * a fresh design sits dead, checking these two words is step one. */

#include "resetseq.h"

#include "uart2.h"

#include <stdint.h>
#include <stdio.h>

extern uint32_t _estack;         /* defined by stm32l476xg_flash.ld */
extern void Reset_Handler(void); /* defined by startup_stm32l476xx.S */

void playing_with_reset_sequence(void) {
    volatile uint32_t* flash_vectors = (uint32_t*)0x08000000;
    volatile uint32_t* boot_alias = (uint32_t*)0x00000000;
    volatile uint32_t* vtor = (uint32_t*)0xE000ED08; /* SCB VTOR */

    uart2_init();

    printf("\r\nLesson: reset sequence\r\n");
    printf("vector[0] @0x08000000 = 0x%08lx  initial MSP the core loads\r\n",
           (unsigned long)flash_vectors[0]);
    printf("  &_estack from the .ld = 0x%08lx  top of RAM — must match\r\n",
           (unsigned long)(uintptr_t)&_estack);
    printf("vector[1] @0x08000004 = 0x%08lx  initial PC the core loads\r\n",
           (unsigned long)flash_vectors[1]);
    printf("  &Reset_Handler        = 0x%08lx  bit0=1: the Thumb bit\r\n",
           (unsigned long)(uintptr_t)&Reset_Handler);

    printf("boot alias @0x00000000 = 0x%08lx / 0x%08lx\r\n", (unsigned long)boot_alias[0],
           (unsigned long)boot_alias[1]);
    printf("  BOOT0=0 mirrors flash at 0 — THIS is where the core fetched\r\n");

    printf("SCB VTOR   @0xE000ED08 = 0x%08lx  exception table base — still 0!\r\n",
           (unsigned long)*vtor);
    printf("  nobody set it; every ISR so far was fetched via the alias.\r\n");
    printf("  fine while BOOT0=0 — a bootloader would have to move it\r\n");

    printf("then Reset_Handler ran: sp=_estack, SystemInit (FPU on),\r\n");
    printf(".data copied, .bss zeroed, __libc_init_array, main()\r\n");
}
