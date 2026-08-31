/* Lesson: the task scheduler capstone; see Src/scheduler.c. `make serial`.
 * SCHED_BLOCKING_DELAYS is the finished scheduler; flip to
 * SCHED_SPIN_DELAYS to watch the slides' first build share the CPU.
 * Side demos parked in git: oled.c (I2C3 SSD1306), sevenseg.c (raw GPIO).
 * Previous lessons: svc.c, faults.c, cmsisnvic.c, nvic.c, stackmem.c,
 * bitband.c, memmap.c, tbit.c, resetseq.c, inlineasm.c, coreregs.c,
 * access.c, modes.c. */

#include "scheduler.h"

int main(void) {
    playing_with_scheduler(SCHED_BLOCKING_DELAYS);

    for (;;) {}
}
