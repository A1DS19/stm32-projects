/* Lesson: reset sequence; see Src/resetseq.c. `make serial`.
 * Previous: inlineasm.c, coreregs.c, access.c, modes.c. */

#include "resetseq.h"

int main(void) {
    playing_with_reset_sequence();

    for (;;) {}
}
