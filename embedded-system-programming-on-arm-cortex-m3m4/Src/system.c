#include "stm32l476xx.h"

/* Called by the startup code before main. The startup file only declares a weak
 * empty SystemInit, but this project compiles hard-float (-mfloat-abi=hard), so
 * CP10/CP11 must be granted before any FPU instruction executes — newlib's
 * printf pushes FPU registers and hard-faults otherwise. */
void SystemInit(void) {
    SCB->CPACR |= (3U << 20) | (3U << 22); /* CP10 + CP11 full access */
    __DSB();
    __ISB();
}
