/* Lesson: interrupts & the NVIC; see Src/nvic.c. `make serial`.
 * Side demos parked in git: oled.c (I2C3 SSD1306), sevenseg.c (raw GPIO).
 * Previous lessons: stackmem.c, bitband.c, memmap.c, tbit.c, resetseq.c,
 * inlineasm.c, coreregs.c, access.c, modes.c. */

#include "nvic.h"

int main(void) {
    playing_with_nvic();

    for (;;) {}
}
