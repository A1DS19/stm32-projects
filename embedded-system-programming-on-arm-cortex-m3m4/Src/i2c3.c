/* I2C3 as a register-level bus master, for talking to chips OUTSIDE the
 * microcontroller. Same three-step dance as every peripheral so far:
 * clock it, route pins to it, configure its registers — I2C3's own
 * registers sit at 0x4000_5C00 (APB1, behind the AHB->APB bridge).
 *
 * Pins: PC0 = SCL, PC1 = SDA (alternate function 4). On the Nucleo those
 * are the Arduino A5 and A4 pins — the classic I2C spot. I2C lines are
 * open-drain: a pin only ever pulls the wire LOW or lets go; resistors
 * pull it high when everyone lets go. That's what lets two chips share
 * the same two wires without fighting.
 *
 * Timing: the boot clock is MSI 4 MHz and I2C3 runs from PCLK1 (also
 * 4 MHz). TIMINGR carves that into a ~100 kHz SCL: PRESC=0 keeps the
 * 250 ns tick, SCLL=0x13 (20 ticks = 5 us low), SCLH=0x0F (16 ticks =
 * 4 us high), plus SDADEL/SCLDEL data setup/hold — the reference manual
 * RM0351 has the recipe. */

#include "i2c3.h"

#include "stm32l476xx.h"

#include <stdint.h>

#define I2C_TIMEOUT_SPINS 100000U /* give up rather than hang the demo */

void i2c3_init(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C3EN;

    /* PC0/PC1: open-drain, internal pull-ups (most modules add their own),
     * then alternate-function mode routed to AF4 = I2C3 */
    GPIOC->OTYPER |= GPIO_OTYPER_OT0 | GPIO_OTYPER_OT1;
    GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD0 | GPIO_PUPDR_PUPD1);
    GPIOC->PUPDR |= GPIO_PUPDR_PUPD0_0 | GPIO_PUPDR_PUPD1_0;
    GPIOC->AFR[0] &= ~(GPIO_AFRL_AFSEL0 | GPIO_AFRL_AFSEL1);
    GPIOC->AFR[0] |= (4U << GPIO_AFRL_AFSEL0_Pos) | (4U << GPIO_AFRL_AFSEL1_Pos);
    GPIOC->MODER &= ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE1);
    GPIOC->MODER |= GPIO_MODER_MODE0_1 | GPIO_MODER_MODE1_1;

    I2C3->TIMINGR = 0x00420F13U; /* ~100 kHz from the 4 MHz kernel clock */
    I2C3->CR1 = I2C_CR1_PE;
}

/* Write `len` bytes to the 7-bit address. len 0 = just the address probe:
 * a chip that exists answers the address byte with an ACK.
 * Returns 0 = acknowledged, -1 = NACK (nobody home), -2 = bus timeout. */
int i2c3_write(uint8_t addr7, const uint8_t* data, uint32_t len) {
    uint32_t spins = 0;

    while (I2C3->ISR & I2C_ISR_BUSY) {
        if (++spins > I2C_TIMEOUT_SPINS) {
            return -2;
        }
    }

    /* one transfer, hardware-managed: address, len bytes, STOP (AUTOEND) */
    I2C3->CR2 =
        ((uint32_t)addr7 << 1U) | (len << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND | I2C_CR2_START;

    for (uint32_t sent = 0; sent < len; ++sent) {
        spins = 0;
        while (!(I2C3->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF))) {
            if (++spins > I2C_TIMEOUT_SPINS) {
                return -2;
            }
        }
        if (I2C3->ISR & I2C_ISR_NACKF) {
            break;
        }
        I2C3->TXDR = data[sent];
    }

    spins = 0;
    while (!(I2C3->ISR & I2C_ISR_STOPF)) {
        if (++spins > I2C_TIMEOUT_SPINS) {
            return -2;
        }
    }

    int nacked = (I2C3->ISR & I2C_ISR_NACKF) ? 1 : 0;
    I2C3->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF;
    return nacked ? -1 : 0;
}
