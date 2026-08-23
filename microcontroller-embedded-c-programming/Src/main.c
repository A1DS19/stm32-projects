/* Lesson: 4x4 matrix keypad — scan, debounce, pin-descriptor tables.
 * See Src/keypad.c for wiring and theory. Keys print on `make serial`;
 * A toggles LD2, digits collect, # prints them.
 * (Src/keydiag.c holds the wire-finder that debugged the wiring — point
 * main at playing_with_keydiag() to run it again.) */

#include "keypad.h"

int main(void)
{
  playing_with_keypad();

  for (;;) {
  }
}
