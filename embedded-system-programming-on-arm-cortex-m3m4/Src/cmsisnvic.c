/* Lesson: the NVIC again, through CMSIS (companion to nvic.c).
 *
 * CMSIS — Cortex Microcontroller Software Interface Standard — is ARM's
 * header layer, shipped identically by every Cortex-M vendor. Two files
 * matter here: core_cm4.h (ARM's: the NVIC/SCB structs and the
 * NVIC_* functions) and stm32l476xx.h (ST's: IRQn names, peripheral
 * addresses). The functions are static inline — they COMPILE AWAY into
 * the very stores nvic.c wrote by hand. Nobody types 0xE000E100 at a
 * job; everybody calls NVIC_EnableIRQ. This lesson proves, on hardware,
 * that both dialects land on the same silicon — and finds the one spot
 * where the library's version is genuinely better than ours.
 *
 * That spot: priority writes. nvic.c set a priority the slides' way —
 * load the 32-bit IPR word, mask the byte, OR the new value, store the
 * word back (LDR/BIC/ORR/STR, four instructions). CMSIS instead does
 * NVIC->IP[n] = value: the IPR registers are BYTE-ADDRESSABLE, so one
 * STRB writes exactly one IRQ's byte and cannot touch its three
 * word-neighbors. Shorter, and it deletes a real race: our word
 * read-modify-write could clobber a neighbor's priority if an interrupt
 * changed it between the load and the store. Demo 2 proves the byte
 * store works on live hardware. Good libraries are made of accumulated
 * hardware folklore like this.
 *
 * The dialect trap (memorize this one): CMSIS priorities are 0..15 —
 * NVIC_SetPriority(irq, 5) shifts left by 4 internally and the hardware
 * byte reads 0x50. Same level, two spellings. FreeRTOS configs mix
 * pre-shifted (0x50-style) and unshifted (5-style) values depending on
 * the setting, and putting an ISR on the wrong side of
 * configMAX_SYSCALL_INTERRUPT_PRIORITY this way is the classic
 * lose-a-week embedded bug. Demo 1 shows both spellings side by side.
 *
 * One more kindness: NVIC_SetPriority takes NEGATIVE numbers for system
 * exceptions (SysTick is -1, PendSV -2, SVC -5) and quietly redirects
 * them to SCB->SHP — the SHPR bytes nvic.c's demo 8 poked raw. One
 * function spans both register files the exception model splits across
 * NVIC and SCB.
 *
 * When you'll use this: daily, verbatim — the two lines of demo 3 ARE
 * the interrupt setup of every driver you'll ever ship, and vendor HAL
 * code (HAL_NVIC_*) is a thin coat of paint over these same calls. The
 * working rule: WRITE CMSIS, THINK REGISTERS — type NVIC_SetPriority,
 * but when the system misbehaves, read ISER/IPR/SHPR in the debugger
 * and know (from nvic.c) exactly what the bits mean. This file is the
 * bridge between the course's register truth and the code you'll
 * actually be paid to write. */

#include "cmsisnvic.h"

#include "stm32l476xx.h" /* pulls in core_cm4.h: NVIC_*, SCB, IRQn names */
#include "uart2.h"

#include <stdint.h>
#include <stdio.h>

/* TIM3 (IRQ 29, exception 45) — this lesson's line; nvic.c owns
 * TIM2/I2C1/USART3/PendSV, and all lesson files compile together. */

void TIM3_IRQHandler(void) {
    printf("  [TIM3 handler] IPSR=%lu — pended and enabled purely via CMSIS calls\r\n",
           (unsigned long)__get_IPSR());
}

void playing_with_cmsis_nvic(void) {
    uart2_init();

    printf("\r\nLesson: the NVIC again — same silicon, CMSIS dialect\r\n");
    printf("(companion to nvic.c: every demo names the raw move it replaces)\r\n");

    /* 1 — two spellings of one level */
    NVIC_SetPriority(TIM3_IRQn, 5);
    printf("\r\n1) NVIC_SetPriority(TIM3_IRQn, 5) — CMSIS speaks 0..15:\r\n");
    printf("   raw IPR byte reads 0x%02X (the <<4 shift, done for us),\r\n",
           (unsigned)NVIC->IP[TIM3_IRQn]);
    printf("   NVIC_GetPriority reads %lu — same level, two dialects. Mixing\r\n",
           (unsigned long)NVIC_GetPriority(TIM3_IRQn));
    printf("   them is THE classic FreeRTOS priority-config bug\r\n");

    /* 2 — why CMSIS compiles to one STRB: IPR is byte-addressable */
    printf("\r\n2) IPR word for IRQs 28-31 before: 0x%08lx\r\n",
           (unsigned long)*(volatile uint32_t*)&NVIC->IP[28]);
    NVIC->IP[TIM3_IRQn] = 0xA0; /* ONE byte store — no read-modify-write */
    printf("   after NVIC->IP[29] = 0xA0:  0x%08lx — one byte changed, three\r\n",
           (unsigned long)*(volatile uint32_t*)&NVIC->IP[28]);
    printf("   neighbors untouched, and no LDR/BIC/ORR/STR window for an\r\n");
    printf("   interrupt to race through. nvic.c's word math never knew\r\n");

    /* 3 — the day-job idiom: the two lines every driver ends with */
    printf("\r\n3) NVIC_SetPendingIRQ(TIM3_IRQn) while disabled (nvic.c's ISPR0\r\n");
    NVIC_SetPendingIRQ(TIM3_IRQn);
    printf("   store): NVIC_GetPendingIRQ=%lu. Now NVIC_EnableIRQ (the ISER0\r\n",
           (unsigned long)NVIC_GetPendingIRQ(TIM3_IRQn));
    printf("   store — identical disassembly, we checked):\r\n");
    NVIC_EnableIRQ(TIM3_IRQn);

    /* 4 — negative IRQn: one function, both register files */
    NVIC_SetPriority(SysTick_IRQn, 15);
    printf("\r\n4) NVIC_SetPriority(SysTick_IRQn, 15) — IRQn is NEGATIVE (-1), so\r\n");
    printf("   CMSIS routes it to SCB->SHP, not NVIC->IP: SHPR3 = 0x%08lx\r\n",
           (unsigned long)*(volatile uint32_t*)&SCB->SHP[8]);
    printf("   (top byte 0xF0 — demo 8 of nvic.c poked PendSV's byte raw;\r\n");
    printf("   same file of registers, one API for both families)\r\n");

    /* 5 — round C's AIRCR dance, wrapped */
    NVIC_SetPriorityGrouping(7);
    printf("\r\n5) NVIC_SetPriorityGrouping(7): GetPriorityGrouping=%lu — the\r\n",
           (unsigned long)NVIC_GetPriorityGrouping());
    printf("   VECTKEY-guarded AIRCR read-modify-write from round C, one call\r\n");
    NVIC_SetPriorityGrouping(0); /* restore for whoever runs next */

    printf("\r\ndone — same NVIC as yesterday, zero addresses typed. Write CMSIS,\r\n");
    printf("think registers: NVIC->ISER[0] sits at %p, exactly where we left it\r\n",
           (void*)&NVIC->ISER[0]);
}
