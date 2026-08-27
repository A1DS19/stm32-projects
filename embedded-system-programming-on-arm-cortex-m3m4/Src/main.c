/* Lesson: exception entry/exit & faults; see Src/faults.c. `make serial`.
 * Side demos parked in git: oled.c (I2C3 SSD1306), sevenseg.c (raw GPIO).
 * Previous lessons: cmsisnvic.c, nvic.c, stackmem.c, bitband.c, memmap.c,
 * tbit.c, resetseq.c, inlineasm.c, coreregs.c, access.c, modes.c. */

#include "faults.h"

int main(void) {
    playing_with_faults();

    for (;;) {}
}
