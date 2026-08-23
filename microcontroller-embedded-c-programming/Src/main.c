/* Lesson: LED toggle through bit-field register structs — struct + union
 * + bit-fields + volatile in one register map, checked by _Static_assert.
 * See Src/ledtoggle.c. LD2 blinks; readbacks on `make serial`. */

#include "ledtoggle.h"

int main(void)
{
  playing_with_ledtoggle();

  for (;;) {
  }
}
