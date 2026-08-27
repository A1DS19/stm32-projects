/* Lesson: SVC, the system-call door; see Src/svc.c. `make serial`.
 * Side demos parked in git: oled.c (I2C3 SSD1306), sevenseg.c (raw GPIO).
 * Previous lessons: faults.c, cmsisnvic.c, nvic.c, stackmem.c, bitband.c,
 * memmap.c, tbit.c, resetseq.c, inlineasm.c, coreregs.c, access.c, modes.c. */

#include "svc.h"

int main(void) {
    playing_with_svc();

    for (;;) {}
}
