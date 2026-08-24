#include "uart2.h"

#include "stm32l476xx.h"

void uart2_init(void) {
    /* Enable clocks: GPIOA (AHB2) and USART2 (APB1) */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;

    /* PA2 (TX) / PA3 (RX) -> alternate function mode, AF7 = USART2 */
    GPIOA->MODER &= ~(GPIO_MODER_MODE2 | GPIO_MODER_MODE3);
    GPIOA->MODER |= GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1;
    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3);
    GPIOA->AFR[0] |= (7U << GPIO_AFRL_AFSEL2_Pos) | (7U << GPIO_AFRL_AFSEL3_Pos);

    /* Baud: after reset the system runs on MSI at 4 MHz and USART2 is clocked
     * from PCLK1 (also 4 MHz). BRR = 4000000 / 115200 ~= 35 (0.8% error). */
    USART2->BRR = 35U;
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

/* Strong definition of the weak hook syscall.c's _write() calls — this is what
 * connects printf to the VCP. */
int __io_putchar(int ch) {
    while (!(USART2->ISR & USART_ISR_TXE)) {}
    USART2->TDR = (uint8_t)ch;
    return ch;
}

/* Non-blocking receive: return the waiting byte, or -1 if none has arrived
 * (RXNE = "receive register not empty"; reading RDR also clears it).
 * If bytes ever arrived faster than we picked them up, the overrun flag (ORE)
 * sets and reception stalls until it is cleared — clear it so input keeps
 * working after a hiccup. */
int uart2_poll(void) {
    if (USART2->ISR & USART_ISR_ORE) {
        USART2->ICR = USART_ICR_ORECF;
    }
    if (USART2->ISR & USART_ISR_RXNE) {
        return (int)(USART2->RDR & 0xFFU);
    }
    return -1;
}

/* Blocking receive: poll until a byte shows up. */
int uart2_getc(void) {
    int byte;
    while ((byte = uart2_poll()) < 0) {}
    return byte;
}
