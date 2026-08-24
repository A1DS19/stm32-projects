/* Side demo: 3461AS 7-segment display on raw GPIO registers; see
 * Src/sevenseg.c (wiring table there). `make serial`, type digits.
 * Course lessons so far: memmap.c, tbit.c, resetseq.c, inlineasm.c,
 * coreregs.c, access.c, modes.c. */

#include "sevenseg.h"

int main(void) {
    playing_with_seven_segments();

    for (;;) {}
}
