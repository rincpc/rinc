/* Caps Lock on the board (keyboard.c, through rinc_hidShifted): a letter typed with it is a capital, with
   Shift too it's small again, and other keys (digits, punctuation) aren't changed by it, as the desktop's
   and the browser's typing has it */
#include <stdio.h>
#include "rinc.h"

static int failed;
static void check(int ok, const char *what) { printf("%s%s\n", ok ? "ok   " : "FAIL ", what); failed += !ok; }

int main(void) {
  int a = 4, z = 29, one = 30, slash = 56;
  check(!rinc_hidShifted(a, 0, 0) && rinc_hidShifted(a, 1, 0), "a, A: Shift alone");
  check(rinc_hidShifted(a, 0, 1) && rinc_hidShifted(z, 0, 1), "Caps Lock: A, Z");
  check(!rinc_hidShifted(a, 1, 1), "Caps Lock and Shift: a");
  check(!rinc_hidShifted(one, 0, 1) && rinc_hidShifted(one, 1, 1), "Caps Lock leaves 1 (Shift still !)");
  check(!rinc_hidShifted(slash, 0, 1), "Caps Lock leaves /");
  return failed != 0;
}
