/* Lesson: interrupts & the NVIC (slides 118-157).
 *
 * The Cortex-M core ships with its own built-in peripherals, living on
 * the Private Peripheral Bus (PPB region, 0xE000_0000 — the memmap
 * lesson classified CPUID there): the NVIC, the SCB, the SysTick timer,
 * the MPU, the FPU and the debug blocks. This lesson is about the first
 * two, because together they run the EXCEPTION MODEL — the machinery
 * that decides when the CPU drops what it's doing and runs a handler.
 *
 * One model, two families, two register homes:
 *   - SYSTEM EXCEPTIONS (numbers 1-15): reset, NMI, the faults, SVC,
 *     PendSV, SysTick. Born inside the processor. Managed through the
 *     SCB (System Control Block): SHCSR holds their enable bits, ICSR
 *     their software-pend bits.
 *   - INTERRUPTS / IRQs (numbers 16 and up): the vendor's peripherals —
 *     ST wired 82 lines on the L476 (USART3 is line 39, TIM2 line 28...).
 *     Managed through the NVIC (Nested Vectored Interrupt Controller):
 *     ISER enables a line, ISPR pends it, IPR gives it a priority.
 *     "Exception number = IRQ number + 16" — IPSR proves it below.
 *
 * The recipe this lesson exists to teach — wiring ANY peripheral
 * interrupt, on any Cortex-M chip, forever:
 *   1. find the IRQ number in the vendor's vector table (RM0351 here)
 *   2. NVIC side: enable that line (ISER), give it a priority (IPR)
 *   3. peripheral side: tell the peripheral to raise its line (its own
 *      control registers — the I2C3/UART lessons' territory, not this one)
 * Here no peripheral is ever clocked or configured: we set the pending
 * bit OURSELVES with a store to ISPR, and the handler runs exactly as if
 * hardware had done it. (modes.c pended by writing a NUMBER into STIR;
 * ISPR is the other door — one BIT per line, and it also lets you READ
 * what's pending.)
 *
 * Priority, the part everyone trips on:
 *   - LOWER value = MORE urgent. Priority 7 beats priority 8.
 *   - Each IRQ gets one byte in the IPR registers (4 IRQs per 32-bit
 *     word), but ST only wired the TOP 4 BITS of each byte — writes to
 *     the low nibble vanish. Hence "16 priority levels" (0x00, 0x10, ...
 *     0xF0). Proven below by writing 0xFF and reading 0xF0 back.
 *   - A new interrupt PREEMPTS a running handler only if its pre-empt
 *     priority is strictly more urgent; equals wait their turn and run
 *     back-to-back after ("tail-chaining" — the return is skipped and
 *     the next handler entered directly).
 *   - PRIGROUP (a 3-bit field in SCB AIRCR) splits the priority byte
 *     into "pre-empt" bits (left) and "sub-priority" bits (right).
 *     Sub-priority never interrupts anyone — it only orders who goes
 *     first among the waiting. Group 7 = zero pre-empt bits = nothing
 *     preempts anything, proven live in round C.
 *   - All else equal, the LOWER IRQ NUMBER wins the tie.
 *   - System exceptions compete in the SAME contest, but their priority
 *     bytes live in SCB SHPR1-3 (they boot at 0 = most urgent), and
 *     three sit above everything at FIXED levels no register can move:
 *     Reset -3, NMI -2, HardFault -1. An RTOS demotes PendSV to the
 *     bottom so context switches never delay real interrupts.
 *
 * When you'll use this: step 2 of every driver you will ever write —
 * every UART/SPI/timer/DMA setup ends with "enable the line in NVIC,
 * pick a priority", and HAL_NVIC_SetPriority()/HAL_NVIC_EnableIRQ() are
 * exactly the stores below. Priority values are where real systems get
 * their latency bugs: a "slow" interrupt set more urgent than a motor
 * or audio deadline, or an RTOS's "don't call the kernel above this
 * priority" line (FreeRTOS configMAX_SYSCALL_INTERRUPT_PRIORITY) drawn
 * wrong — both are diagnosed by reading IPR/AIRCR the way this lesson
 * writes them. And the scheduler capstone stands on demo 1: PendSV,
 * pended from software, is the context-switch workhorse. */

#include "nvic.h"

#include "uart2.h"

#include <stdint.h>
#include <stdio.h>

/* NVIC — one bit per IRQ line (write 1 to act, 0s are ignored) */
#define NVIC_ISER0 ((volatile uint32_t*)0xE000E100)    /* enable, lines 0-31  */
#define NVIC_ISER1 ((volatile uint32_t*)0xE000E104)    /* enable, lines 32-63 */
#define NVIC_ISPR0 ((volatile uint32_t*)0xE000E200)    /* pend,   lines 0-31  */
#define NVIC_ISPR1 ((volatile uint32_t*)0xE000E204)    /* pend,   lines 32-63 */
#define NVIC_IPR_BASE ((volatile uint32_t*)0xE000E400) /* priorities, 4 IRQs/word */

/* SCB — the system-exception side of the model */
#define SCB_ICSR ((volatile uint32_t*)0xE000ED04)  /* pend/status: PENDSVSET=bit28 */
#define SCB_AIRCR ((volatile uint32_t*)0xE000ED0C) /* PRIGROUP [10:8], key-locked  */
#define SCB_SHCSR ((volatile uint32_t*)0xE000ED24) /* system handler enables       */
#define SCB_SHPR3 ((volatile uint32_t*)0xE000ED20) /* PendSV pri byte 2, SysTick 3 */

#define AIRCR_VECTKEY (0x05FAUL << 16) /* AIRCR ignores writes without this key */

#define IRQ_TIM2 28U    /* TIM2 global interrupt  — exception number 44 */
#define IRQ_I2C1_EV 31U /* I2C1 event interrupt   — exception number 47 */
#define IRQ_USART3 39U  /* USART3 global interrupt — exception number 55 */

/* Which demo round is running, so the shared handlers know their part.
 * 'A' equal priorities, 'B' I2C1 more urgent, 'C' PRIGROUP=7, 'T' tie,
 * 'S' system-exception priority (TIM2 pends a demoted PendSV). */
static volatile char demo_round;

static uint32_t read_ipsr(void) {
    uint32_t value;
    __asm volatile("MRS %0, IPSR" : "=r"(value));
    return value;
}

/* The slide-128 math: one byte per IRQ inside word-sized registers.
 * (Plain = stores would clobber the 3 neighbor bytes, so priorities DO
 * need read-modify-write — unlike ISER/ISPR, see below.) */
/* NOLINTNEXTLINE(bugprone-easily-swappable-parameters) — same shape as CMSIS NVIC_SetPriority */
static void set_irq_priority(uint32_t irq, uint8_t value) {
    volatile uint32_t* ipr = NVIC_IPR_BASE + (irq / 4U); /* which word   */
    uint32_t shift = (irq % 4U) * 8U;                    /* which byte     */
    uint32_t word = *ipr;
    word &= ~(0xFFUL << shift);
    word |= (uint32_t)value << shift;
    *ipr = word;
}

static uint8_t get_irq_priority(uint32_t irq) {
    return (uint8_t)(*(NVIC_IPR_BASE + (irq / 4U)) >> ((irq % 4U) * 8U));
}

/* ---- the handlers (weak Default_Handler in startup, strong here) ---- */

/* PendSV: a system exception, so its pend bit lives in SCB ICSR, not in
 * any NVIC register. The strong PendSV_Handler symbol now belongs to
 * scheduler.c (same single-strong-symbol rule as faulthandler.c's
 * HardFault_Handler); until the scheduler starts, its handler forwards
 * here, so this lesson still runs by pointing main.c at it. */
void nvic_pendsv_demo(void) {
    printf("  [PendSV handler] IPSR=%lu — system exception, no IRQ number at all\r\n",
           (unsigned long)read_ipsr());
}

void USART3_IRQHandler(void) {
    printf("  [USART3 handler] IPSR=%lu = 16+%lu — ran the instant ISER let it\r\n",
           (unsigned long)read_ipsr(), (unsigned long)IRQ_USART3);
}

void TIM2_IRQHandler(void) {
    printf("\r\n  [TIM2 handler %c] enter (IPSR=%lu)\r\n", demo_round, (unsigned long)read_ipsr());
    if (demo_round == 'S') {
        *SCB_ICSR = 1UL << 28; /* pend PendSV from inside a handler — the RTOS offload move */
        printf("  [TIM2 handler %c] PendSV pended from in here — outranked, it waits\r\n",
               demo_round);
    } else if (demo_round != 'T') {
        *NVIC_ISPR0 = 1UL << IRQ_I2C1_EV; /* pend I2C1 from INSIDE a running handler */
        printf("  [TIM2 handler %c] I2C1 pended from in here — is it above this line?\r\n",
               demo_round);
    }
    printf("  [TIM2 handler %c] exit\r\n", demo_round);
}

void I2C1_EV_IRQHandler(void) {
    printf("  [I2C1 handler %c] running (IPSR=%lu)\r\n", demo_round, (unsigned long)read_ipsr());
}

/* --------------------------------------------------------------------- */

void playing_with_nvic(void) {
    uart2_init();

    printf("\r\nLesson: interrupts & the NVIC — the exception model, live\r\n");

    /* 1 — system exceptions live in the SCB, not the NVIC */
    printf("\r\n1) SHCSR = 0x%08lx: usage/bus/memmanage fault enables all 0 (the\r\n",
           (unsigned long)*SCB_SHCSR);
    printf("   'disabled by default' table, live — every fault now escalates to\r\n");
    printf("   HardFault, which is why faulthandler.c catches everything so far)\r\n");
    *SCB_SHCSR |= (1UL << 16) | (1UL << 17) | (1UL << 18);
    printf("   armed all three: SHCSR = 0x%08lx — section 8 will trip them\r\n",
           (unsigned long)*SCB_SHCSR);
    printf("   pending PendSV via ICSR bit 28 (no NVIC register has a bit for it):\r\n");
    *SCB_ICSR = 1UL << 28;

    /* 2 — exercise s128: USART3, pended first, enabled second */
    printf("\r\n2) USART3 (IRQ %lu), the s128 exercise — pend it while DISABLED:\r\n",
           (unsigned long)IRQ_USART3);
    /* ISER/ISPR are write-1-to-set: 0s do nothing, so a plain = store is
     * the honest idiom. |= would read pending state and write it back —
     * a read-modify-write that pretends the 0 bits needed preserving. */
    *NVIC_ISPR1 = 1UL << (IRQ_USART3 % 32U);
    printf("   ISPR1 bit %lu reads %lu — pended, nothing ran. ICSR VECTPENDING=%lu:\r\n",
           (unsigned long)(IRQ_USART3 % 32U),
           (unsigned long)((*NVIC_ISPR1 >> (IRQ_USART3 % 32U)) & 1U),
           (unsigned long)((*SCB_ICSR >> 12) & 0x1FFU));
    printf("   0, not 55 — that field only reports pending lines that are ENABLED,\r\n");
    printf("   so a disabled line's pend is visible in ISPR alone (finding)\r\n");
    printf("   the peripheral itself: untouched, not even clocked. Now enable:\r\n");
    *NVIC_ISER1 = 1UL << (IRQ_USART3 % 32U);
    printf("   that's the slide's promise: pending survives disable, fires on enable\r\n");

    /* 3 — how wide is a priority byte really? */
    set_irq_priority(IRQ_TIM2, 0xFF);
    printf("\r\n3) wrote priority 0xFF for TIM2, read back 0x%02X — ST wired only\r\n",
           get_irq_priority(IRQ_TIM2));
    printf("   the top 4 bits of each byte: 16 levels (0x00 most urgent...0xF0)\r\n");

    /* 4 — round A: equal priorities never preempt */
    printf("\r\n4) round A: TIM2 and I2C1 both at 0x80. TIM2 pends I2C1 mid-handler\r\n");
    printf("   — equals must WAIT, so expect it AFTER TIM2's exit (tail-chained):\r\n");
    demo_round = 'A';
    set_irq_priority(IRQ_TIM2, 0x80);
    set_irq_priority(IRQ_I2C1_EV, 0x80);
    *NVIC_ISER0 = (1UL << IRQ_TIM2) | (1UL << IRQ_I2C1_EV);
    *NVIC_ISPR0 = 1UL << IRQ_TIM2;

    /* 5 — round B: a more urgent value preempts, mid-handler */
    printf("\r\n5) round B: I2C1 promoted to 0x70 (lower value = MORE urgent).\r\n");
    printf("   same dance — now it should barge in BETWEEN TIM2's two prints:\r\n");
    demo_round = 'B';
    set_irq_priority(IRQ_I2C1_EV, 0x70);
    *NVIC_ISPR0 = 1UL << IRQ_TIM2;

    /* 6 — round C: PRIGROUP decides what counts as "more urgent" */
    printf("\r\n6) round C: same 0x70-vs-0x80, but AIRCR PRIGROUP=7: all 8 bits are\r\n");
    printf("   now SUB-priority, pre-empt field is 0 bits wide — nobody preempts\r\n");
    printf("   anybody. Expect round A's order back:\r\n");
    demo_round = 'C';
    *SCB_AIRCR = AIRCR_VECTKEY | (7UL << 8);
    printf("   (AIRCR PRIGROUP now reads %lu)\r\n", (unsigned long)((*SCB_AIRCR >> 8) & 7U));
    *NVIC_ISPR0 = 1UL << IRQ_TIM2;
    *SCB_AIRCR = AIRCR_VECTKEY; /* restore group 0 for whoever runs next */

    /* 7 — the tie-break: lower IRQ number goes first */
    printf("\r\n7) tie-break: both back to 0x80, both pended in ONE ISPR0 store —\r\n");
    printf("   identical urgency, so the lower line number (TIM2, %lu < %lu) wins:\r\n",
           (unsigned long)IRQ_TIM2, (unsigned long)IRQ_I2C1_EV);
    demo_round = 'T';
    set_irq_priority(IRQ_I2C1_EV, 0x80);
    *NVIC_ISPR0 = (1UL << IRQ_TIM2) | (1UL << IRQ_I2C1_EV);

    /* 8 — system exceptions have priorities too: SCB SHPR, not NVIC IPR */
    printf("\r\n8) SHPR3 = 0x%08lx — every configurable system exception boots at\r\n",
           (unsigned long)*SCB_SHPR3);
    printf("   priority 0, the most urgent programmable level. That's why demo 1's\r\n");
    printf("   PendSV fired mid-thread instantly. Demote it: wrote 0xFF into its\r\n");
    *SCB_SHPR3 |= 0xFFUL << 16; /* RMW: SysTick's byte lives in this word too */
    printf("   SHPR3 byte, reads back 0x%02X — same 4-bit rule as the IPRs.\r\n",
           (unsigned)((*SCB_SHPR3 >> 16) & 0xFFU));
    printf("   round S: TIM2 (0x80) pends the now-0xF0 PendSV from inside its\r\n");
    printf("   handler — the RTOS offload pattern: the context switch politely\r\n");
    printf("   runs only after every real interrupt is done:\r\n");
    demo_round = 'S';
    *NVIC_ISPR0 = 1UL << IRQ_TIM2;
    printf("   (Reset/NMI/HardFault sit above ALL of this at fixed -3/-2/-1 —\r\n");
    printf("   no register exists to touch them)\r\n");

    printf("\r\ndone — 3 IRQ lines and PendSV all ran with zero peripherals configured:\r\n");
    printf("the NVIC neither knows nor cares who set the pending bit\r\n");
}
