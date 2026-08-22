/* Lesson: interrupts + volatile — see Src/isr.c for the full story.
 * LD2 toggles every 2 s; each B1 press makes it 10x faster (100 ms floor
 * resets to 2 s). Watch interval changes on `make serial`. */

#include "isr.h"

int main(void)
{
  playing_with_isr();

  for (;;) {
  }
}
