/* typed keys, as a terminal sends them, become pc.key()'s codes (docs/spec/input.md#typed-text): editing
   keys, their modifiers (xterm's ESC [ 1 ; m X, ESC [ n ; m ~ and ESC [ 27 ; m ; k ~), Shift+Tab, a
   lone Esc, Ctrl+letter and plain characters */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "rinc.h"

static _Alignas(16) uint8_t space[256 << 10];
static rio_Vm vm;
static char out[1024];
static int outLen;

static void put(const char *s, int n) {
  if (n > (int)sizeof out - 1 - outLen) n = (int)sizeof out - 1 - outLen;
  memcpy(out + outLen, s, (size_t)n);
  outLen += n;
  out[outLen] = 0;
}

int main(void) {
  static const char typed[] =
    "\033[C"        /* right: 129 */
    "\033[1;5C"     /* Ctrl+right: 129 + 512 */
    "\033[1;2D"     /* Shift+left: 128 + 256 */
    "\033[3;6~"     /* Ctrl+Shift+Delete: 134 + 256 + 512 */
    "\033[Z"        /* Shift+Tab: 9 + 256 */
    "\033[27;5;8~"  /* Ctrl+Backspace: 8 + 512 */
    "\033[27;6;122~" /* Ctrl+Shift+Z: 26 + 256 */
    "\032"          /* Ctrl+Z: 26 */
    "a\r";          /* a, Enter */
  static const char app[] = "c := pc.key()\nfor c >= 0\n  log(c)\n  c = pc.key()\nend\n";
  static const char want[] = "129\n641\n384\n902\n265\n520\n282\n26\n97\n13\n";
  const char *p;
  if (rinc_start(&vm, space, sizeof space, put) || rinc_compile(&vm, space, sizeof space, app, sizeof app - 1)) {
    printf("FAIL: %s\n", rio_error(&vm));
    return 1;
  }
  for (p = typed; *p; p++) rinc_textPush((unsigned char)*p); /* (after the start, which empties the queue) */
  if (rio_run(&vm)) {
    printf("FAIL: %s\n", rio_error(&vm));
    return 1;
  }
  for (p = out; *p; p++) if (*p == '\r') memmove((char *)p, p + 1, strlen(p)); /* (lines as \n) */
  if (strcmp(out, want)) { printf("FAIL: got\n%swanted\n%s", out, want); return 1; }
  { /* the system key and the locks, a frame a letter: s Shift, c Ctrl, a Alt, C Ctrl and Shift, X Ctrl and
       Shift with another key, o another key, e Esc, . nothing; then 1 for home on the last frame, if wanted */
    static const struct { const char *frames; int home; } cases[] = {
      {"C.e", 1},      /* Ctrl and Shift tapped together, then Esc */
      {"cC.e", 1},     /* one, then the other, let go together */
      {"CX.e", 0},     /* another key while they're down */
      {"C.oe", 0},     /* another key before Esc */
      {"C.C.e", 0},    /* tapped twice: off again */
      {"s.s.e", 0},    /* Shift tapped twice: Shift's lock, not the system key */
      {"..e", 0},      /* Esc alone */
    };
    int i, f;
    for (i = 0; i < (int)(sizeof cases / sizeof cases[0]); i++) {
      const char *fr = cases[i].frames; int home = 0;
      for (f = 0; f < 60; f++) rinc_systemKey(0, 0, 0, 0); /* (settled) */
      for (f = 0; fr[f]; f++) {
        char c = fr[f];
        int mods = c == 's' ? 1 : c == 'c' ? 2 : c == 'a' ? 4 : c == 'C' || c == 'X' ? 3 : 0;
        home = rinc_systemKey(0, mods, c == 'o' || c == 'X', c == 'e');
      }
      if (home != cases[i].home) { printf("FAIL: system key %s gave %d\n", fr, home); return 1; }
      if (rinc_systemHeld()) { rinc_systemKey(0, 3, 0, 0); rinc_systemKey(0, 0, 0, 0); } /* (off again) */
      if (fr[0] == 's') { rinc_systemKey(0, 1, 0, 0); rinc_systemKey(0, 0, 0, 0); } /* (Shift unlocked again) */
    }
    /* the overlay: the strip while the system key is on, its edge drawn; down after home */
    rinc_systemKey(0, 3, 0, 0); rinc_systemKey(0, 0, 0, 0);
    if (!rinc_overlayOn || rinc_overlay[0][0] != 10 || rinc_shownAt(0, rinc_H - rinc_OVERLAY_H) != 10) {
      printf("FAIL: no overlay while the system key is on\n");
      return 1;
    }
    if (!rinc_systemKey(0, 0, 0, 1) || rinc_overlayOn || rinc_shownAt(5, rinc_H - 1) != rinc_screen[rinc_H - 1][5]) {
      printf("FAIL: the overlay stays after going home\n");
      return 1;
    }
  }
  { /* the locks: Shift tapped twice capitalizes, Ctrl tapped twice makes Ctrl+letter (with Shift's bit too,
       both locked), a tap unlocks, and a capital typed isn't a tap; the badges show, the rest of the strip
       see-through */
    static const char app2[] = "c := pc.key()\nfor c >= 0\n  log(c)\n  c = pc.key()\nend\n";
    int f;
    rinc_systemKey(0, 1, 0, 0); rinc_systemKey(0, 0, 0, 0); rinc_systemKey(0, 1, 0, 0); rinc_systemKey(0, 0, 0, 0);
    if (!rinc_overlayOn || rinc_overlay[5][0] != rinc_SEE_THROUGH || rinc_overlay[5][rinc_W - 3] == rinc_SEE_THROUGH ||
        rinc_shownAt(0, rinc_H - 5) != rinc_screen[rinc_H - 5][0]) {
      printf("FAIL: no Shift badge, or the strip isn't see-through\n");
      return 1;
    }
    outLen = 0; out[0] = 0;
    if (rinc_start(&vm, space, sizeof space, put) || rinc_compile(&vm, space, sizeof space, app2, sizeof app2 - 1)) { printf("FAIL: %s\n", rio_error(&vm)); return 1; }
    rinc_textPush('a');
    rinc_systemKey(0, 2, 0, 0); rinc_systemKey(0, 0, 0, 0); rinc_systemKey(0, 2, 0, 0); rinc_systemKey(0, 0, 0, 0);
    rinc_textPush('b');
    rinc_textKey(rinc_Key_left);
    rinc_systemKey(0, 1, 0, 0); rinc_systemKey(0, 0, 0, 0); /* (Shift unlocked) */
    rinc_systemKey(0, 2, 0, 0); rinc_systemKey(0, 0, 0, 0); /* (Ctrl unlocked) */
    rinc_systemKey(0, 1, 1, 0); rinc_systemKey(0, 0, 0, 0); rinc_systemKey(0, 1, 0, 0); rinc_systemKey(0, 0, 0, 0); /* (a capital typed: no lock) */
    rinc_textPush('c');
    for (f = 0; f < 60; f++) rinc_systemKey(0, 0, 0, 0);
    if (rio_run(&vm)) { printf("FAIL: %s\n", rio_error(&vm)); return 1; }
    for (p = out; *p; p++) if (*p == '\r') memmove((char *)p, p + 1, strlen(p));
    if (strcmp(out, "65\n258\n896\n99\n")) { printf("FAIL: locked keys gave\n%s", out); return 1; }
    if (rinc_overlayOn) { printf("FAIL: the overlay stays with no locks\n"); return 1; }
  }
  { /* recent apps: started a, then b. Sys+Tab, then Enter: a. Sys+Q (in b): back to a, b forgotten. Sys+P:
       the performance strip, up while nothing's held. Sys+Enter: the whole display */
    int f, go = 0;
    rinc_appBegin(&vm, space, sizeof space, "a", put);
    rinc_appBegin(&vm, space, sizeof space, "b", put);
    for (f = 0; f < 60; f++) rinc_systemKey(0, 0, 0, 0);
    rinc_systemKey(0, 3, 0, 0); rinc_systemKey(0, 0, 0, 0);
    go |= rinc_systemKey(0, 0, 1, rinc_SysKey_tab); /* (the switcher, on a) */
    go |= rinc_systemKey(0, 0, 0, 0);
    if (go || !rinc_systemHeld()) { printf("FAIL: no switcher\n"); return 1; }
    if (!rinc_systemKey(0, 0, 1, rinc_SysKey_enter) || strcmp(rinc_launchName, "a")) { printf("FAIL: the switcher didn't go to a\n"); return 1; }
    rinc_appBegin(&vm, space, sizeof space, "b", put); /* (b again, the latest) */
    rinc_systemKey(0, 0, 0, 0); rinc_systemKey(0, 3, 0, 0); rinc_systemKey(0, 0, 0, 0);
    if (!rinc_systemKey(0, 0, 1, rinc_SysKey_q) || strcmp(rinc_launchName, "a")) { printf("FAIL: Sys+Q didn't go back to a\n"); return 1; }
    rinc_appBegin(&vm, space, sizeof space, "a", put);
    rinc_systemKey(0, 0, 0, 0); rinc_systemKey(0, 3, 0, 0); rinc_systemKey(0, 0, 0, 0);
    if (rinc_systemKey(0, 0, 1, rinc_SysKey_tab) || rinc_systemHeld()) { printf("FAIL: a switcher with b forgotten\n"); return 1; }
    rinc_systemKey(0, 0, 0, 0); rinc_systemKey(0, 3, 0, 0); rinc_systemKey(0, 0, 0, 0);
    rinc_systemKey(0, 0, 1, rinc_SysKey_p);
    for (f = 0; f < 60; f++) rinc_systemKey(0, 0, 0, 0);
    if (!rinc_overlayOn || rinc_overlay[0][0] != 5) { printf("FAIL: no performance strip\n"); return 1; }
    rinc_systemKey(0, 3, 0, 0); rinc_systemKey(0, 0, 0, 0);
    rinc_systemKey(0, 0, 1, rinc_SysKey_enter);
    if (!rinc_fullscreen) { printf("FAIL: Sys+Enter didn't ask for the whole display\n"); return 1; }
    /* a controller: guide held, RB opens the switcher on the app before, and letting go of guide switches;
       LB starts from the other end (with c, b, a recent: a) */
    rinc_appBegin(&vm, space, sizeof space, "b", put);
    rinc_appBegin(&vm, space, sizeof space, "c", put);
    rinc_systemKey(1, 0, 0, 0);
    if (rinc_systemKey(1, 0, 0, rinc_SysKey_next) || rinc_systemKey(1, 0, 0, 0) || !rinc_systemKey(0, 0, 0, 0) || strcmp(rinc_launchName, "b")) {
      printf("FAIL: guide and RB didn't switch to b\n");
      return 1;
    }
    rinc_appBegin(&vm, space, sizeof space, "b", put);
    rinc_systemKey(1, 0, 0, 0);
    if (rinc_systemKey(1, 0, 0, rinc_SysKey_prev) || rinc_systemKey(1, 0, 0, 0) || !rinc_systemKey(0, 0, 0, 0) || strcmp(rinc_launchName, "a")) {
      printf("FAIL: guide and LB didn't switch to a\n");
      return 1;
    }
    /* Sys+R: this app (b, the latest started) again */
    rinc_systemKey(0, 3, 0, 0); rinc_systemKey(0, 0, 0, 0);
    if (!rinc_systemKey(0, 0, 1, rinc_SysKey_r) || strcmp(rinc_launchName, "b")) { printf("FAIL: Sys+R didn't start b again\n"); return 1; }
  }
  { /* a controller's RB and LB type Tab and Shift+Tab as they're pressed, to a game */
    static const char game[] = "update* :: proc()\n  c := pc.key()\n  for c >= 0\n    log(c)\n    c = pc.key()\n  end\nend\n";
    outLen = 0; out[0] = 0;
    if (rinc_appBegin(&vm, space, sizeof space, "pad", put) || rinc_appRun(&vm, space, sizeof space, "pad", game, sizeof game - 1, 0) || !rinc_game) {
      printf("FAIL: %s\n", rio_error(&vm));
      return 1;
    }
    outLen = 0; out[0] = 0;
    rinc_buttons = rinc_Button_rb; rinc_gameFrame(&vm);
    rinc_buttons = rinc_Button_rb; rinc_gameFrame(&vm); /* (held: nothing more) */
    rinc_buttons = 0; rinc_gameFrame(&vm);
    rinc_buttons = rinc_Button_lb; rinc_gameFrame(&vm);
    for (p = out; *p; p++) if (*p == '\r') memmove((char *)p, p + 1, strlen(p));
    if (strcmp(out, "9\n265\n")) { printf("FAIL: RB and LB typed\n%s", out); return 1; }
  }
  { /* the pointer: a mouse moves and shows it, typing hides it; in pointer mode, the left stick moves it
       (and isn't the arrows), A clicks, and the wheel's steps add up; the d-pad hides it, as typing does */
    static const char game[] =
      "update* :: proc()\n  log(pc.pointerX())\n  log(pc.pointerY())\n  log(pc.pointerDown(0))\n  log(pc.pointerShown())\n"
      "  log(pc.scroll(1))\n  log(pc.btn(1))\n  for pc.key() >= 0\n  end\n  if pc.btn(13) == 1\n    pc.pointer(pc.Pointer.ui)\n  end\nend\n";
    static const char want[] = "10\n20\n0\n1\n0\n0\n" "10\n20\n0\n0\n1\n0\n" "10\n20\n0\n0\n0\n1\n" "15\n20\n1\n1\n0\n0\n" "15\n20\n0\n0\n0\n0\n";
    outLen = 0; out[0] = 0;
    if (rinc_appBegin(&vm, space, sizeof space, "point", put) || rinc_appRun(&vm, space, sizeof space, "point", game, sizeof game - 1, 0) || !rinc_game) {
      printf("FAIL: %s\n", rio_error(&vm));
      return 1;
    }
    outLen = 0; out[0] = 0;
    rinc_mouseMove(10.5f, 20); rinc_gameFrame(&vm);
    rinc_textPush('a'); rinc_mouseWheel(0, 1.5f); rinc_gameFrame(&vm);
    rinc_axes[0] = 32767; rinc_buttons = rinc_Button_back; rinc_gameFrame(&vm); /* (mode 0 still: the stick's the arrows; then mode 1) */
    rinc_buttons = rinc_Button_a; rinc_gameFrame(&vm);
    rinc_axes[0] = 0; rinc_buttons = rinc_Button_left; rinc_gameFrame(&vm); /* (the d-pad hides it) */
    rinc_buttons = 0;
    for (p = out; *p; p++) if (*p == '\r') memmove((char *)p, p + 1, strlen(p));
    if (strcmp(out, want)) { printf("FAIL: the pointer gave\n%swanted\n%s", out, want); return 1; }
  }
  printf("all keys came through\n");
  return 0;
}
