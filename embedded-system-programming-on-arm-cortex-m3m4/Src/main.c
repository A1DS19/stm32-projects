/* Lesson: the T bit; see Src/tbit.c. `make serial`. Ends parked in the
 * hard-fault handler on purpose — reset the board to rerun.
 * Previous: resetseq.c, inlineasm.c, coreregs.c, access.c, modes.c. */

#include "tbit.h"

int main(void) {
    playing_with_t_bit();

    for (;;) {}
}
