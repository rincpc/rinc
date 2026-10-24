/* Rinc on the RP2350 (Adafruit Fruit Jam): it runs one app at a time, starting at home. Every
   app, home and the console included, is an app on the SD card, compiled whole into the app space
   (docs/spec/running.md); the system key, or Ctrl-C over USB serial, switches. With no home or
   console app there, the firmware's own console (rinc_fwBegin) takes their place.

   Memory, of the RP2350's 520 KB of SRAM:
     app space            256 KB  (Rio: an app's code and data; an app's source, read from the SD card,
                                   sits at its top while it compiles, where its data will be zero)
     screen               150 KB  (drawn into, and the copy being shown)
     native loops          16 KB  (rio_jit: loops compiled to Thumb-2)
     rio's interpreter     10 KB  (its loop runs from SRAM: see RINC_RIO_IN_RAM)
     USB, console, stacks  the rest */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "rinc.h"
#include "video.h"
#include "keyboard.h"
#include "gamepad.h"
void rinc_sdSetup(void); /* sd.c: the SD card, as rinc_fs */
extern const uint8_t rinc_firmwareBundle[]; extern const uint32_t rinc_firmwareBundleSize; /* (bundle.c, from RINC_BUNDLE) */
#include "rinc_audio.h"
#include "tusb.h" /* (HID key codes) */

#define rinc_APP_SPACE (256 << 10) /* the app space */

static _Alignas(16) uint8_t rinc_appSpace[rinc_APP_SPACE];
static _Alignas(8) uint8_t rinc_jitCode[16 << 10]; /* simple loops, compiled to machine code (SRAM runs code) */
static rio_Vm rinc_vm;
/* log output: to the serial port (and, while an app starts and once it's stopped, the screen: rinc_say) */
static void rinc_put(const char *s, int n) { fwrite(s, 1, (size_t)n, stdout); fflush(stdout); }
#define rinc_DOWN(k) (rinc_keysDown[(k) >> 3] >> ((k) & 7) & 1)

/* the system key (rinc_systemKey): the Windows key, a controller's Xbox button, or Ctrl and Shift tapped
   together; and the locks. With Esc (or the controller's Back), Tab, Q, P or Enter: 1 to start
   rinc_launchName */
static int rinc_systemKeys(void) {
  int k, other = rinc_padButtons != 0, m = rinc_keysDown[0xE0 >> 3]; /* (the modifiers: Ctrl Shift Alt GUI, left then right) */
  int mods = (m & 0x22 ? 1 : 0) | (m & 0x11 ? 2 : 0) | (m & 0x44 ? 4 : 0);
  int keys = (rinc_DOWN(HID_KEY_ESCAPE) || (rinc_padButtons & rinc_Button_back) ? rinc_SysKey_esc : 0) | (rinc_DOWN(HID_KEY_TAB) ? rinc_SysKey_tab : 0) |
             (rinc_DOWN(HID_KEY_Q) ? rinc_SysKey_q : 0) | (rinc_DOWN(HID_KEY_P) ? rinc_SysKey_p : 0) | (rinc_DOWN(HID_KEY_R) ? rinc_SysKey_r : 0) | (rinc_DOWN(HID_KEY_H) ? rinc_SysKey_h : 0) |
             (rinc_DOWN(HID_KEY_ENTER) || rinc_DOWN(HID_KEY_KEYPAD_ENTER) ? rinc_SysKey_enter : 0) |
             (rinc_padButtons & rinc_Button_rb ? rinc_SysKey_next : 0) | (rinc_padButtons & rinc_Button_lb ? rinc_SysKey_prev : 0) |
             rinc_pointerKeys(rinc_padButtons | rinc_DOWN(HID_KEY_ARROW_LEFT) * rinc_Button_left | rinc_DOWN(HID_KEY_ARROW_RIGHT) * rinc_Button_right |
                              rinc_DOWN(HID_KEY_ARROW_UP) * rinc_Button_up | rinc_DOWN(HID_KEY_ARROW_DOWN) * rinc_Button_down |
                              rinc_DOWN(HID_KEY_V) * rinc_Button_a | rinc_DOWN(HID_KEY_C) * rinc_Button_b); /* (the pointer's) */
  for (k = 0; k < 32; k++) if (k != (0xE0 >> 3)) other |= rinc_keysDown[k]; /* (all but the modifiers) */
  return rinc_systemKey(rinc_DOWN(HID_KEY_GUI_LEFT) || rinc_DOWN(HID_KEY_GUI_RIGHT) || rinc_padGuide, mods, other, keys);
}

/* an app's native code, once it's loaded: its simple loops compiled to Thumb-2 (after its image is
   cached, which keeps to bytecode) */
static int rinc_jitApp(rio_Vm *vm) { return rio_jit(vm, rinc_jitCode, sizeof rinc_jitCode); }

/* an app, by name, started (rinc_appBegin, rinc_appStart): from the SD card, else the firmware's
   bundle, compiled whole (or its cached image, while it's still its source's); its top-level code,
   then main; then, if it's a game, its frames. What goes wrong stays on the screen */
static void rinc_fwBegin(const char *why);
static rinc_Repl *rinc_fw; /* the firmware's own console, while it's on (rinc_fwBegin) */
static void rinc_open(const char *name) {
  rinc_fw = 0; /* (its memory is the app's now) */
  name = rinc_appFor(name); /* (home: the card's, else the bundle's) */
  if ((!strcmp(name, "home") || !strcmp(name, "console")) && rinc_readApp(name, 0, 0) == -1 && !rinc_bundled(name)) {
    rinc_fwBegin(!strcmp(name, "home") ? "No home app on the SD card (/rinc/apps/home).\r\n"
                                         "uv run tools/install.py puts Rinc's apps there.\r\n\r\n"
                                       : "No console app on the SD card: Rinc's own console.\r\n\r\n");
    return;
  }
  if (rinc_appBegin(&rinc_vm, rinc_appSpace, rinc_APP_SPACE, name, rinc_put)) return;
  rinc_appStart(&rinc_vm, rinc_appSpace, rinc_APP_SPACE, name);
}

/* the firmware's own console (rinc_Repl, as the desktop's terminal has): when the card has no home or
   no console app, or on Ctrl-B over serial. It runs Rio, and it puts files on the card (put: how
   tools/install.py fills it). On the screen and over serial, typed at from either. It and its rio
   session live in the app space, since no app runs meanwhile */
#define rinc_FW_MEM (128 << 10)
static void rinc_fwPut(const char *s, int n) { rinc_put(s, n); rinc_consolePut(s, n); }
static void rinc_fwBegin(const char *why) {
  uint32_t k = (sizeof(rinc_Repl) + 15) & ~15u;
  rinc_game = 0; rinc_showLog = 1;
  rinc_consoleClear(1);
  rinc_fw = (rinc_Repl *)(void *)rinc_appSpace;
  rinc_fwPut(why, (int)strlen(why));
  if (rinc_start(&rinc_vm, rinc_appSpace + k, rinc_FW_MEM, rinc_fwPut) ||
      rinc_replBegin(rinc_fw, &rinc_vm, rinc_appSpace + k + rinc_FW_MEM, rinc_APP_SPACE - k - rinc_FW_MEM)) {
    const char *e = rio_error(&rinc_vm);
    rinc_fwPut(e, (int)strlen(e));
    rinc_fw = 0;
    return;
  }
  rinc_fw->put = rinc_fwPut;
}

/* typing, from the USB keyboard and the serial port, for pc.key() (or the firmware's console, while
   it's on); over serial (a developer's tools, such as send.py; not the keyboard's), Ctrl-C opens the
   console app, in place of whatever's running: 1; and Ctrl-B the firmware's own console: 2 */
static int rinc_type(void) {
  int c;
  if (rinc_fw) {
    while ((c = rinc_keyboardChar()) >= 0) if (!rinc_systemHeld()) rinc_replChar(rinc_fw, c);
    while (rinc_fw && (c = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {
      if (c == 2) return 2; /* (a fresh one) */
      if (c == 3 && (rinc_readApp("console", 0, 0) != -1 || rinc_bundled("console"))) return 1;
      rinc_replChar(rinc_fw, c);
    }
    if (rinc_fw && rinc_fw->run) rinc_replStop(rinc_fw, "(run is for the desktop's terminal: here, apps run from home)");
    return 0;
  }
  while (rinc_textRoom() > 8 && (c = rinc_keyboardChar()) >= 0) {
    if (rinc_systemHeld()) continue; /* (a combination with the system key: not the app's typing) */
    if (c == 27) { /* (Esc itself, or the start of an editing key's escape: ESC [ ...) */
      int d = rinc_keyboardChar();
      if (d == '[') { rinc_textPush(27); rinc_textPush('['); continue; }
      rinc_textEsc();
      c = d;
      if (c < 0) continue;
    }
    rinc_textPush(c);
  }
  while (rinc_textRoom() > 8 && (c = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {
    if (c == 3) return 1;
    if (c == 2) return 2;
    rinc_textPush(c);
  }
  return 0;
}

/* a frame, at each vertical blank, right after the screen was copied for showing */
static void rinc_frameNow(void) {
  static uint32_t last; static int wasConnected; int stop = 0, connected;
  while (rinc_frames == last) { rinc_keyboardPoll(); stop |= rinc_type(); } /* (the USB host runs all the while) */
  last = rinc_frames;
  stop |= rinc_type();
  if ((connected = stdio_usb_connected()) && !wasConnected) printf("%s", rinc_keyboardEvents()); /* (a terminal arrived: what's plugged in) */
  wasConnected = connected;
  rinc_padRead();
  if (stop & 2) { rinc_fwBegin("Rinc's own console (Ctrl-B).\r\n\r\n"); rinc_launch = 0; return; }
  if (stop & 1) { strcpy(rinc_launchName, "console"); rinc_launch = 1; }
  if (rinc_systemKeys()) rinc_launch = 1; /* (the one key no app gets: to rinc_launchName) */
  if (rinc_launch) { rinc_launch = 0; rinc_open(rinc_launchName); return; }
  if (!rinc_game) return;
  rinc_buttons = rinc_padButtons | (uint16_t)(rinc_DOWN(HID_KEY_ARROW_LEFT) * rinc_Button_left | rinc_DOWN(HID_KEY_ARROW_RIGHT) * rinc_Button_right |
    rinc_DOWN(HID_KEY_ARROW_UP) * rinc_Button_up | rinc_DOWN(HID_KEY_ARROW_DOWN) * rinc_Button_down |
    (rinc_lettersAsButtons() ? rinc_DOWN(HID_KEY_V) * rinc_Button_a | rinc_DOWN(HID_KEY_C) * rinc_Button_b | rinc_DOWN(HID_KEY_X) * rinc_Button_x |
                               rinc_DOWN(HID_KEY_Z) * rinc_Button_y : 0)); /* (a game's: a pointer app's keyboard types) */
  if (rinc_systemHeld()) rinc_buttons = 0; /* (a system combination: not the app's) */
  rinc_gameFrame(&rinc_vm); /* (an error goes on the screen) */
  if (rinc_launch) { rinc_launch = 0; rinc_open(rinc_launchName); } /* (pc.launch: after the frame) */
}

static float rinc_now(void) { return (float)((double)time_us_64() / 1e6); }

int main(void) {
  set_sys_clock_khz(252000, true); /* 10x the 25.2 MHz pixel clock of 640x480 DVI; fine at the default core voltage */
  rinc_keyboardSetup();
  stdio_init_all();
  rinc_time = rinc_now;
  rinc_consoleClear(1);
  rinc_consolePut("Rinc\r\n\r\n", 8);
  rinc_audioSetup();
  rinc_videoStart(); /* (core 1: video, then sound) */
  rinc_keyboardStart();
  rinc_sdSetup();
  rinc_native = rinc_jitApp;
  rinc_cacheTrust = 1; /* (the card changes only through Rinc while it runs: a remount drops what's checked) */
  rinc_bundleRead(rinc_firmwareBundle, rinc_firmwareBundleSize); /* (none, by default: the apps are on the card; a bundled one gives way to the card's) */
  rinc_open("home"); /* (the board starts at home) */
  for (;;) rinc_frameNow();
}
