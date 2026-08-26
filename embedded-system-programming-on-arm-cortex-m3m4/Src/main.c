/* Lesson: the NVIC through CMSIS; see Src/cmsisnvic.c. `make serial`.
 * Side demos parked in git: oled.c (I2C3 SSD1306), sevenseg.c (raw GPIO).
 * Previous lessons: nvic.c, stackmem.c, bitband.c, memmap.c, tbit.c,
 * resetseq.c, inlineasm.c, coreregs.c, access.c, modes.c. */

#include "cmsisnvic.h"

int main(void) {
    playing_with_cmsis_nvic();

    for (;;) {}
}
