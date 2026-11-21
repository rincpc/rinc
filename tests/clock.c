/* frame times measured on a clock that's been running long: a frame's 10.5 ms is still 10.5 ms after a
   day up (as a Float of seconds, it had gone to steps of 8 ms) */
#include <math.h>
#include <stdio.h>
#include "rinc.h"

static double now;
static double clock(void) { return now; }
static int failed;
static void check(int ok, const char *what) { printf("%s%s\n", ok ? "ok   " : "FAIL ", what); failed += !ok; }

int main(void) {
  double t0; float ms;
  rinc_time = clock;
  now = 1; t0 = now; now += 0.0105; ms = rinc_msSince(t0);
  check(fabsf(ms - 10.5f) < 0.01f, "a second up: 10.5 ms");
  now = 86400; t0 = now; now += 0.0105; ms = rinc_msSince(t0);
  printf("     (a day up: %.3f ms)\n", ms);
  check(fabsf(ms - 10.5f) < 0.01f, "a day up: 10.5 ms");
  now = 400 * 86400.0; t0 = now; now += 0.0105; ms = rinc_msSince(t0);
  check(fabsf(ms - 10.5f) < 0.01f, "400 days up: 10.5 ms");
  return failed != 0;
}
