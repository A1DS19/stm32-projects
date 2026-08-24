/* Side demo: 128x32 SSD1306 OLED over I2C3; see Src/oled.c (4-pin wiring
 * there). `make serial`, type text. Sibling demo: sevenseg.c (raw GPIO).
 * Course lessons so far: memmap.c, tbit.c, resetseq.c, inlineasm.c,
 * coreregs.c, access.c, modes.c. */

#include "oled.h"

int main(void) {
    playing_with_the_oled();

    for (;;) {}
}
