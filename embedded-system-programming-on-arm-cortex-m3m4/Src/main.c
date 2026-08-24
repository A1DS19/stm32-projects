/* Lesson: stack memory; see Src/stackmem.c. `make serial`.
 * Side demos parked in git: oled.c (I2C3 SSD1306), sevenseg.c (raw GPIO).
 * Previous lessons: bitband.c, memmap.c, tbit.c, resetseq.c, inlineasm.c,
 * coreregs.c, access.c, modes.c. */

#include "stackmem.h"

int main(void) {
    playing_with_stack_memory();

    for (;;) {}
}
