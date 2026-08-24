/* Lesson: core registers + memory-mapped vs not; see Src/coreregs.c.
 * `make serial`. Previous: access.c (PAL/nPAL), modes.c (thread/handler). */

#include "coreregs.h"

int main(void) {
    playing_with_core_registers();

    for (;;) {}
}
