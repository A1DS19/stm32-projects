/* startup.c — the code that runs before main(), written by us
 * (slides 306-320). Until now ST's startup_stm32l476xx.S did this in
 * assembly behind our backs; this file is the same job in C.
 *
 * Three things a startup file must do:
 *   1. Put the vector table at the start of FLASH: word 0 = the initial
 *      stack pointer, word 1 = the reset handler, then one address per
 *      exception and interrupt (resetseq.c proved the core reads words
 *      0 and 1 from 0x08000000 and nothing else).
 *   2. Give every exception a handler, even the ones the program never
 *      uses — a "weak alias" makes each name a second name for
 *      Default_Handler until somebody defines the real one (then the
 *      strong definition wins, as faulthandler.c's HardFault_Handler
 *      does here).
 *   3. In Reset_Handler: build the world C expects — copy .data's initial
 *      values from FLASH to SRAM, zero .bss, turn on the FPU (system.c;
 *      printf hard-faults without it), run the C library's constructor
 *      table, call main().
 *
 * Every address this file needs comes from the linker script as a
 * symbol; `extern uint32_t _sdata;` declares one, `&_sdata` uses it.
 * Never read the value of such a symbol — there is nothing there but
 * whatever the section holds. */

#include <stdint.h>
#include <stdio.h>

/* ---- the linker script's symbols: names for addresses ---- */
extern uint32_t _estack; /* top of SRAM1 */
extern uint32_t _sidata; /* FLASH copy of .data's initial values */
extern uint32_t _sdata;  /* .data in SRAM1, start */
extern uint32_t _edata;  /* .data in SRAM1, end */
extern uint32_t _sbss;   /* .bss, start */
extern uint32_t _ebss;   /* .bss, end */

void SystemInit(void);        /* ../Src/system.c: FPU on */
void __libc_init_array(void); /* newlib: run the .init_array constructors */
int main(void);

void Reset_Handler(void);
void Default_Handler(void);

/* -nostartfiles left out crti.o/crtn.o, the objects that normally define
 * _init and _fini — and __libc_init_array calls _init. Empty ones will do. */
void _init(void) {}
void _fini(void) {}

/* ---- every handler, weak, aliased to Default_Handler ---- */
#define WEAK_DEFAULT __attribute__((weak, alias("Default_Handler")))

void NMI_Handler(void) WEAK_DEFAULT;
void HardFault_Handler(void) WEAK_DEFAULT;
void MemManage_Handler(void) WEAK_DEFAULT;
void BusFault_Handler(void) WEAK_DEFAULT;
void UsageFault_Handler(void) WEAK_DEFAULT;
void SVC_Handler(void) WEAK_DEFAULT;
void DebugMon_Handler(void) WEAK_DEFAULT;
void PendSV_Handler(void) WEAK_DEFAULT;
void SysTick_Handler(void) WEAK_DEFAULT;

/* the STM32L476's 83 IRQ slots (0-82, one reserved) in reference-manual order */
void WWDG_IRQHandler(void) WEAK_DEFAULT;
void PVD_PVM_IRQHandler(void) WEAK_DEFAULT;
void TAMP_STAMP_IRQHandler(void) WEAK_DEFAULT;
void RTC_WKUP_IRQHandler(void) WEAK_DEFAULT;
void FLASH_IRQHandler(void) WEAK_DEFAULT;
void RCC_IRQHandler(void) WEAK_DEFAULT;
void EXTI0_IRQHandler(void) WEAK_DEFAULT;
void EXTI1_IRQHandler(void) WEAK_DEFAULT;
void EXTI2_IRQHandler(void) WEAK_DEFAULT;
void EXTI3_IRQHandler(void) WEAK_DEFAULT;
void EXTI4_IRQHandler(void) WEAK_DEFAULT;
void DMA1_CH1_IRQHandler(void) WEAK_DEFAULT;
void DMA1_CH2_IRQHandler(void) WEAK_DEFAULT;
void DMA1_CH3_IRQHandler(void) WEAK_DEFAULT;
void DMA1_CH4_IRQHandler(void) WEAK_DEFAULT;
void DMA1_CH5_IRQHandler(void) WEAK_DEFAULT;
void DMA1_CH6_IRQHandler(void) WEAK_DEFAULT;
void DMA1_CH7_IRQHandler(void) WEAK_DEFAULT;
void ADC1_2_IRQHandler(void) WEAK_DEFAULT;
void CAN1_TX_IRQHandler(void) WEAK_DEFAULT;
void CAN1_RX0_IRQHandler(void) WEAK_DEFAULT;
void CAN1_RX1_IRQHandler(void) WEAK_DEFAULT;
void CAN1_SCE_IRQHandler(void) WEAK_DEFAULT;
void EXTI9_5_IRQHandler(void) WEAK_DEFAULT;
void TIM1_BRK_TIM15_IRQHandler(void) WEAK_DEFAULT;
void TIM1_UP_TIM16_IRQHandler(void) WEAK_DEFAULT;
void TIM1_TRG_COM_TIM17_IRQHandler(void) WEAK_DEFAULT;
void TIM1_CC_IRQHandler(void) WEAK_DEFAULT;
void TIM2_IRQHandler(void) WEAK_DEFAULT;
void TIM3_IRQHandler(void) WEAK_DEFAULT;
void TIM4_IRQHandler(void) WEAK_DEFAULT;
void I2C1_EV_IRQHandler(void) WEAK_DEFAULT;
void I2C1_ER_IRQHandler(void) WEAK_DEFAULT;
void I2C2_EV_IRQHandler(void) WEAK_DEFAULT;
void I2C2_ER_IRQHandler(void) WEAK_DEFAULT;
void SPI1_IRQHandler(void) WEAK_DEFAULT;
void SPI2_IRQHandler(void) WEAK_DEFAULT;
void USART1_IRQHandler(void) WEAK_DEFAULT;
void USART2_IRQHandler(void) WEAK_DEFAULT;
void USART3_IRQHandler(void) WEAK_DEFAULT;
void EXTI15_10_IRQHandler(void) WEAK_DEFAULT;
void RTC_ALARM_IRQHandler(void) WEAK_DEFAULT;
void DFSDM1_FLT3_IRQHandler(void) WEAK_DEFAULT;
void TIM8_BRK_IRQHandler(void) WEAK_DEFAULT;
void TIM8_UP_IRQHandler(void) WEAK_DEFAULT;
void TIM8_TRG_COM_IRQHandler(void) WEAK_DEFAULT;
void TIM8_CC_IRQHandler(void) WEAK_DEFAULT;
void ADC3_IRQHandler(void) WEAK_DEFAULT;
void FMC_IRQHandler(void) WEAK_DEFAULT;
void SDMMC1_IRQHandler(void) WEAK_DEFAULT;
void TIM5_IRQHandler(void) WEAK_DEFAULT;
void SPI3_IRQHandler(void) WEAK_DEFAULT;
void UART4_IRQHandler(void) WEAK_DEFAULT;
void UART5_IRQHandler(void) WEAK_DEFAULT;
void TIM6_DACUNDER_IRQHandler(void) WEAK_DEFAULT;
void TIM7_IRQHandler(void) WEAK_DEFAULT;
void DMA2_CH1_IRQHandler(void) WEAK_DEFAULT;
void DMA2_CH2_IRQHandler(void) WEAK_DEFAULT;
void DMA2_CH3_IRQHandler(void) WEAK_DEFAULT;
void DMA2_CH4_IRQHandler(void) WEAK_DEFAULT;
void DMA2_CH5_IRQHandler(void) WEAK_DEFAULT;
void DFSDM1_FLT0_IRQHandler(void) WEAK_DEFAULT;
void DFSDM1_FLT1_IRQHandler(void) WEAK_DEFAULT;
void DFSDM1_FLT2_IRQHandler(void) WEAK_DEFAULT;
void COMP_IRQHandler(void) WEAK_DEFAULT;
void LPTIM1_IRQHandler(void) WEAK_DEFAULT;
void LPTIM2_IRQHandler(void) WEAK_DEFAULT;
void OTG_FS_IRQHandler(void) WEAK_DEFAULT;
void DMA2_CH6_IRQHandler(void) WEAK_DEFAULT;
void DMA2_CH7_IRQHandler(void) WEAK_DEFAULT;
void LPUART1_IRQHandler(void) WEAK_DEFAULT;
void QUADSPI_IRQHandler(void) WEAK_DEFAULT;
void I2C3_EV_IRQHandler(void) WEAK_DEFAULT;
void I2C3_ER_IRQHandler(void) WEAK_DEFAULT;
void SAI1_IRQHandler(void) WEAK_DEFAULT;
void SAI2_IRQHandler(void) WEAK_DEFAULT;
void SWPMI1_IRQHandler(void) WEAK_DEFAULT;
void TSC_IRQHandler(void) WEAK_DEFAULT;
void LCD_IRQHandler(void) WEAK_DEFAULT;
void RNG_IRQHandler(void) WEAK_DEFAULT;
void FPU_IRQHandler(void) WEAK_DEFAULT;
void CRS_IRQHandler(void) WEAK_DEFAULT;

/* ---- the vector table ----
 * A plain array of addresses. The section attribute keeps it out of
 * .rodata and puts it in a section of its own, .isr_vector, which the
 * linker script places first in FLASH. 16 system slots + 83 IRQ slots =
 * 99 words = 396 bytes. Reserved slots hold 0. */
/* clang-format off */
const uint32_t vectors[] __attribute__((section(".isr_vector"))) = {
    (uint32_t)&_estack,
    (uint32_t)Reset_Handler,
    (uint32_t)NMI_Handler,
    (uint32_t)HardFault_Handler,
    (uint32_t)MemManage_Handler,
    (uint32_t)BusFault_Handler,
    (uint32_t)UsageFault_Handler,
    0,
    0,
    0,
    0,
    (uint32_t)SVC_Handler,
    (uint32_t)DebugMon_Handler,
    0,
    (uint32_t)PendSV_Handler,
    (uint32_t)SysTick_Handler,
    (uint32_t)WWDG_IRQHandler, /* IRQ 0 */
    (uint32_t)PVD_PVM_IRQHandler,
    (uint32_t)TAMP_STAMP_IRQHandler,
    (uint32_t)RTC_WKUP_IRQHandler,
    (uint32_t)FLASH_IRQHandler,
    (uint32_t)RCC_IRQHandler, /* IRQ 5 — main.c pends this one on purpose */
    (uint32_t)EXTI0_IRQHandler,
    (uint32_t)EXTI1_IRQHandler,
    (uint32_t)EXTI2_IRQHandler,
    (uint32_t)EXTI3_IRQHandler,
    (uint32_t)EXTI4_IRQHandler,
    (uint32_t)DMA1_CH1_IRQHandler,
    (uint32_t)DMA1_CH2_IRQHandler,
    (uint32_t)DMA1_CH3_IRQHandler,
    (uint32_t)DMA1_CH4_IRQHandler,
    (uint32_t)DMA1_CH5_IRQHandler,
    (uint32_t)DMA1_CH6_IRQHandler,
    (uint32_t)DMA1_CH7_IRQHandler,
    (uint32_t)ADC1_2_IRQHandler,
    (uint32_t)CAN1_TX_IRQHandler,
    (uint32_t)CAN1_RX0_IRQHandler,
    (uint32_t)CAN1_RX1_IRQHandler,
    (uint32_t)CAN1_SCE_IRQHandler,
    (uint32_t)EXTI9_5_IRQHandler,
    (uint32_t)TIM1_BRK_TIM15_IRQHandler,
    (uint32_t)TIM1_UP_TIM16_IRQHandler,
    (uint32_t)TIM1_TRG_COM_TIM17_IRQHandler,
    (uint32_t)TIM1_CC_IRQHandler,
    (uint32_t)TIM2_IRQHandler,
    (uint32_t)TIM3_IRQHandler,
    (uint32_t)TIM4_IRQHandler,
    (uint32_t)I2C1_EV_IRQHandler,
    (uint32_t)I2C1_ER_IRQHandler,
    (uint32_t)I2C2_EV_IRQHandler,
    (uint32_t)I2C2_ER_IRQHandler,
    (uint32_t)SPI1_IRQHandler,
    (uint32_t)SPI2_IRQHandler,
    (uint32_t)USART1_IRQHandler,
    (uint32_t)USART2_IRQHandler,
    (uint32_t)USART3_IRQHandler,
    (uint32_t)EXTI15_10_IRQHandler,
    (uint32_t)RTC_ALARM_IRQHandler,
    (uint32_t)DFSDM1_FLT3_IRQHandler,
    (uint32_t)TIM8_BRK_IRQHandler,
    (uint32_t)TIM8_UP_IRQHandler,
    (uint32_t)TIM8_TRG_COM_IRQHandler,
    (uint32_t)TIM8_CC_IRQHandler,
    (uint32_t)ADC3_IRQHandler,
    (uint32_t)FMC_IRQHandler,
    (uint32_t)SDMMC1_IRQHandler,
    (uint32_t)TIM5_IRQHandler,
    (uint32_t)SPI3_IRQHandler,
    (uint32_t)UART4_IRQHandler,
    (uint32_t)UART5_IRQHandler,
    (uint32_t)TIM6_DACUNDER_IRQHandler,
    (uint32_t)TIM7_IRQHandler,
    (uint32_t)DMA2_CH1_IRQHandler,
    (uint32_t)DMA2_CH2_IRQHandler,
    (uint32_t)DMA2_CH3_IRQHandler,
    (uint32_t)DMA2_CH4_IRQHandler,
    (uint32_t)DMA2_CH5_IRQHandler,
    (uint32_t)DFSDM1_FLT0_IRQHandler,
    (uint32_t)DFSDM1_FLT1_IRQHandler,
    (uint32_t)DFSDM1_FLT2_IRQHandler,
    (uint32_t)COMP_IRQHandler,
    (uint32_t)LPTIM1_IRQHandler,
    (uint32_t)LPTIM2_IRQHandler,
    (uint32_t)OTG_FS_IRQHandler,
    (uint32_t)DMA2_CH6_IRQHandler,
    (uint32_t)DMA2_CH7_IRQHandler,
    (uint32_t)LPUART1_IRQHandler,
    (uint32_t)QUADSPI_IRQHandler,
    (uint32_t)I2C3_EV_IRQHandler,
    (uint32_t)I2C3_ER_IRQHandler,
    (uint32_t)SAI1_IRQHandler,
    (uint32_t)SAI2_IRQHandler,
    (uint32_t)SWPMI1_IRQHandler,
    (uint32_t)TSC_IRQHandler,
    (uint32_t)LCD_IRQHandler,
    0, /* reserved */
    (uint32_t)RNG_IRQHandler,
    (uint32_t)FPU_IRQHandler,
    (uint32_t)CRS_IRQHandler, /* IRQ 82 */
};
/* clang-format on */

/* ---- reset: the first code that runs ----
 * The core has already loaded MSP from vector[0], so this is ordinary C
 * with a working stack. Nothing else works yet: .data holds garbage and
 * .bss holds whatever SRAM woke up with. Word loops, no memcpy — at
 * -O0 the compiler emits exactly the loop you read. */
void Reset_Handler(void) {
    /* 1. .data: initial values wait in FLASH (_sidata); the variables
     *    live in SRAM1 (_sdata.._edata). Copy word by word. */
    uint32_t* src = &_sidata;
    for (uint32_t* dst = &_sdata; dst < &_edata; dst++) {
        *dst = *src++;
    }

    /* 2. .bss: no FLASH image at all — just clear the SRAM1 range */
    for (uint32_t* p = &_sbss; p < &_ebss; p++) {
        *p = 0;
    }

    /* 3. the rest of the world C expects */
    SystemInit();        /* FPU on before any printf */
    __libc_init_array(); /* constructor tables (empty in C — but newlib insists) */
    main();

    for (;;) {} /* main must never return; if it does, park here */
}

/* Where every unwritten handler lands. Real firmware does exactly this
 * — prints which exception it was and stops — instead of the slides'
 * bare while(1), so a crash into a missing handler explains itself. */
void Default_Handler(void) {
    uint32_t ipsr;
    __asm volatile("MRS %0, IPSR" : "=r"(ipsr));

    printf("\r\n[Default_Handler] unhandled exception: IPSR=%lu", (unsigned long)ipsr);
    if (ipsr >= 16U) {
        printf(" = IRQ %lu", (unsigned long)(ipsr - 16U));
    }
    printf(" — every vector nobody wrote a handler for points here\r\n");

    for (;;) {}
}
