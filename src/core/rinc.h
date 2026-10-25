/* Rinc: what every platform shares. The screen is plain memory that apps draw into through the host
   procs registered here; a platform shows it (the Fruit Jam over DVI, the desktop in a window or a
   file) and feeds keys to the REPL. No allocation: the platform hands over every buffer. */
#ifndef rinc_CORE_H
#define rinc_CORE_H
#include <stdint.h>
#include "rio.h"

#define rinc_W 320
#define rinc_H 240

extern uint8_t rinc_screen[rinc_H][rinc_W]; /* 8 bits per pixel: an index into rinc_palette */
extern uint16_t rinc_palette[256];        /* RGB565: PICO-8's 32, a 6x6x6 color cube, 8 grays */
uint32_t rinc_rgb(int i);                 /* the same, as 0xRRGGBB */
/* pictures as PNG files (png.c), in storage: indexed, with the palette as their colors and 0 see-through.
   pngWrite: w * h pixels, a byte each, row by row: 0, or -1. pngRead: into out (cap bytes), w and h (2
   bytes each, low first), then the pixels, each the palette's nearest color (exactly the same, for a PNG
   of the palette's colors), its size; or -1: no such file, a PNG it can't read, or out too small (it
   needs room for the PNG's rows as they come, too: w * h + 2 * h + 4 bytes for one of the palette's).
   notes: an app's text about the picture (a sprite sheet's sprite size, say), kept in the PNG in a tEXt
   chunk named Rinc (up to rinc_PNG_NOTES bytes; n 0: none), which other programs keep or ignore;
   pngNotes reads them back into out: how many bytes, or -1 (no such file, not a PNG, or none) */
#define rinc_PNG_NOTES 512
int rinc_pngWrite(const char *path, const uint8_t *pixels, int w, int h, const char *notes, int n);
int rinc_pngRead(const char *path, uint8_t *out, uint32_t cap);
int rinc_pngNotes(const char *path, char *out, uint32_t cap);
/* raw deflate data (a zip's file, size bytes of it) from read, inflated into out: its length, or -1 */
int rinc_inflateRaw(int (*read)(char *buf, int n), uint32_t size, uint8_t *out, uint32_t cap);
extern float (*rinc_time)(void);          /* seconds since start, from the platform */
/* buttons held: bit 0 left, 1 right, 2 up, 3 down, then the face buttons, named as on an Xbox
   controller and in order of importance: 4 A (V key), 5 B (C), 6 X (X), 7 Y (Z), then the
   controller's others: 8 LB, 9 RB, 10 left stick, 11 right stick (pressed in), 12 Start, 13 Back.
   Each platform sets them from its keys and controllers before a frame */
extern uint16_t rinc_buttons;
/* whether a keyboard's V, C, X and Z are A, B, X and Y too (as its arrows are the d-pad): for a game, not
   for a pointer app (pc.Pointer.ui), whose keyboard types only. There a controller's A and B, with the
   pointer hidden, are Enter and Esc too */
int rinc_lettersAsButtons(void);
/* a controller's sticks and triggers, also set by the platform: 0 left stick x, 1 its y, 2 and 3 the
   right stick's (-32767 to 32767; y is + down, as on the screen), 4 left trigger, 5 right (0 to 32767) */
extern int16_t rinc_axes[6];
/* typed text, for an app's key() (docs/spec/input.md): the platform passes what's typed, as a terminal
   sends it: printable ASCII, 13 Enter (or \n), 8 Backspace (or 127), 9 Tab, Ctrl+letter as 1-26, and
   editing keys as ESC [ sequences, which become the key codes below. A key that isn't a character
   (these, Tab, Enter, Backspace, Ctrl+letter) carries the modifiers held with it, as bits: as a
   terminal sends them, ESC [ 1 ; 5 C for Ctrl+right, ESC [ Z for Shift+Tab, ESC [ 27 ; m ; key ~
   for the rest */
enum { rinc_Key_left = 128, rinc_Key_right, rinc_Key_up, rinc_Key_down, rinc_Key_home, rinc_Key_end, rinc_Key_delete,
       rinc_Key_pageUp, rinc_Key_pageDown };
enum { rinc_Mod_shift = 256, rinc_Mod_ctrl = 512, rinc_Mod_alt = 1024 };
void rinc_textPush(int c);
void rinc_textEsc(void);    /* the Esc key itself, for key() (the platform knows it from a terminal escape) */
void rinc_textKey(int key); /* a key code itself (rinc_Key_*, with rinc_Mod_*), from a platform that knows the key */
int rinc_textMods(int shift, int ctrl, int alt); /* the rinc_Mod_* bits for these */
int rinc_textRoom(void);    /* how many more can wait: a platform reads its serial port no faster */
enum { rinc_Button_left = 1, rinc_Button_right = 2, rinc_Button_up = 4, rinc_Button_down = 8, rinc_Button_a = 16, rinc_Button_b = 32, rinc_Button_x = 64, rinc_Button_y = 128,
       rinc_Button_lb = 256, rinc_Button_rb = 512, rinc_Button_ls = 1024, rinc_Button_rs = 2048, rinc_Button_start = 4096, rinc_Button_back = 8192 };

/* set up rio in the given memory (the app space: code and data both live there), with log output
   going to put and the host procs apps call, in module pc (auto-imported; src/core/pc.rio declares
   them): cls(c), pset(x, y, c), pget(x, y) -> c, rect(x, y, w, h, c), print(s, x, y, c),
   btn(i) -> Int (1 held), time() -> Float */
int rinc_start(rio_Vm *vm, void *mem, uint32_t memsize, void (*put)(const char *s, int n));
/* just the host procs, registered as pc.cls and so on (rinc_start does this, and auto-imports pc) */
int rinc_hostProcs(rio_Vm *vm);
/* an app compiled as every Rinc compiles one (docs/spec/memory.md), after rinc_start, the compiler in
   the app space (where the program's memory is still zero): its source read a part at a time, from
   path in storage (or, host, path in rinc_hostFolder), as its other files are; or (rinc_compile) a
   source in memory, outside the app space, as a bundle's is. 0, or -1 (rio_error says why) */
int rinc_compileFile(rio_Vm *vm, void *space, uint32_t size, const char *path, int host);
const char *rinc_errorPart(rio_Vm *vm); /* the file an error's in, if not the app's own (else 0) */
int rinc_compile(rio_Vm *vm, void *space, uint32_t size, const char *src, uint32_t len);
int rinc_atLimit(const char *error); /* is this compile error about running out of something? */
/* the system key (docs/spec/apps.md): the Windows key or a controller's guide button, held (sysDown), or
   Ctrl and Shift tapped together, for the next combination. With it: Esc (or a controller's Back) home,
   Tab the switcher of recent apps (Tab and Shift+Tab move, and a controller's RB and LB; letting go,
   or Enter switches, Esc doesn't; no time limits), Q back to the app before (this one forgotten, its
   stash dropped), R this app again (its stash dropped), P
   the performance strip, Enter the whole display (rinc_fullscreen). And the locks: Shift, Ctrl or Alt
   tapped twice locks it, for every key typed, until it's tapped again. Call each frame with what's down
   (mods: 1 Shift, 2 Ctrl, 4 Alt; otherDown: any other key, the system key's own aside; keys:
   rinc_SysKey_* bits): 1 to start rinc_launchName now. While the system key is on (rinc_systemHeld),
   what's typed isn't the app's */
enum { rinc_SysKey_esc = 1, rinc_SysKey_tab = 2, rinc_SysKey_q = 4, rinc_SysKey_p = 8, rinc_SysKey_enter = 16,
       rinc_SysKey_next = 32, rinc_SysKey_prev = 64, rinc_SysKey_r = 128, /* (next, prev: a controller's RB and LB, as Tab and Shift+Tab) */
       rinc_SysKey_left = 256, rinc_SysKey_right = 512, rinc_SysKey_up = 1024, rinc_SysKey_down = 2048, /* the pointer's: the arrows (the d-pad) move it, */
       rinc_SysKey_a = 4096, rinc_SysKey_b = 8192, /* V and C (A and B) its buttons */
       rinc_SysKey_pointer = 0x3f00, rinc_SysKey_h = 0x4000 /* H: the app's help */ };
/* the pointer's system keys from buttons held (rinc_Button_*: the arrows, V and C, a controller's d-pad, A and B) */
static inline int rinc_pointerKeys(int buttons) {
  return (buttons & 1 ? rinc_SysKey_left : 0) | (buttons & 2 ? rinc_SysKey_right : 0) | (buttons & 4 ? rinc_SysKey_up : 0) |
         (buttons & 8 ? rinc_SysKey_down : 0) | (buttons & 16 ? rinc_SysKey_a : 0) | (buttons & 32 ? rinc_SysKey_b : 0);
}
int rinc_systemKey(int sysDown, int mods, int otherDown, int keys);
/* a mouse's, from the platform: where it is in screen pixels (fractions as they come), the buttons held
   (1 primary, 2 secondary, 4 middle), and its wheel (steps, + right and up) */
void rinc_mouseMove(float x, float y);
void rinc_mouseButtons(int held);
void rinc_mouseWheel(float dx, float dy);
void rinc_mouseGone(void); /* it left the window: the pointer hides, unless a button's held (dragging) */
int rinc_systemHeld(void);
extern int rinc_fullscreen; /* whether the screen should fill the display (Sys+Enter): a platform that can, follows */
/* the overlay: Rinc's own over the screen's bottom rows, apart from the app's screen: a strip while the
   system key is on (what it does; the switcher) or the performance strip is, and the locks' badges.
   Where it's rinc_SEE_THROUGH, the screen shows; a platform shows each pixel as rinc_shownAt gives it */
#define rinc_OVERLAY_H 11
#define rinc_SEE_THROUGH 255
extern uint8_t rinc_overlay[rinc_OVERLAY_H][rinc_W];
extern volatile int rinc_overlayOn;
/* the pointer, over all that while it's shown: its image's top left corner is its point */
extern volatile int rinc_pointerOnScreen;
extern int rinc_pointerAtX, rinc_pointerAtY;
extern uint8_t rinc_pointerImage[8][8];
static inline uint8_t rinc_shownAt(int x, int y) {
  uint8_t o;
  if (rinc_pointerOnScreen && (unsigned)(x - rinc_pointerAtX) < 8 && (unsigned)(y - rinc_pointerAtY) < 8 &&
      (o = rinc_pointerImage[y - rinc_pointerAtY][x - rinc_pointerAtX]) != rinc_SEE_THROUGH) return o;
  if (!rinc_overlayOn || y < rinc_H - rinc_OVERLAY_H) return rinc_screen[y][x];
  o = rinc_overlay[y - (rinc_H - rinc_OVERLAY_H)][x];
  return o == rinc_SEE_THROUGH ? rinc_screen[y][x] : o;
}
/* one frame of a game: the app's update* and then draw* (each optional). 0: ran, 1: the app has
   neither (it isn't a game), -1: a runtime error (rio_error says) */
int rinc_frame(rio_Vm *vm);
/* starting an app, the same on every Rinc: rinc_appBegin (0, or -1), then rinc_appRun with its source
   in memory or its image (or rinc_appRunHost, its main file in rinc_hostFolder; 0: running, or done; -1). rinc_appFail is a reason it can't start (name, then why). What goes wrong goes on the screen
   (rinc_say: and to put), with the way home. Each frame after, rinc_gameFrame: a game's frame, and its
   error on the screen if it fails (0: ran, 1 nothing running, -1 it failed) */
extern int rinc_game;    /* the app has frames to run (update*, draw*) */
extern int rinc_showLog; /* what's logged goes on the screen too: while an app starts, and once it's stopped */
int rinc_appBegin(rio_Vm *vm, void *space, uint32_t size, const char *name, void (*put)(const char *s, int n));
int rinc_appRun(rio_Vm *vm, void *space, uint32_t size, const char *name, const char *src, uint32_t len, const void *image);
int rinc_appRunHost(rio_Vm *vm, void *space, uint32_t size, const char *name, const char *path);
/* or, for an app by name (after rinc_appBegin): from storage, else the bundle, through its cached
   compiled image while that's still its source's (rinc_appRun's the rest). 0, or -1 */
int rinc_appStart(rio_Vm *vm, void *space, uint32_t size, const char *name);
/* the platform's: native code for an app, once it's loaded (the Fruit Jam's: rio_jit), or NULL. 0 or
   more, or -1 if it can't */
extern int (*rinc_native)(rio_Vm *vm);
/* cached images: rinc_cacheTrust 1 on a Rinc whose storage changes only through Rinc while it runs
   (then an app's checked once a start, not each time it's opened); the platform calls
   rinc_storageChanged(path) for each file it writes or removes; rinc_cacheDrop drops an app's image.
   rinc_buildId: this build's (build_id.c): another build's images are compiled again */
extern int rinc_cacheTrust;
/* an app run from the host's own files (the desktop's rinc game/): its files come from this folder,
   through rinc_hostRead (path in it: up to n bytes from byte at, how many, 0 at its end, or -1 if it
   isn't there), not storage */
extern int (*rinc_hostRead)(const char *path, char *buf, uint32_t at, uint32_t n);
extern char rinc_hostFolder[512];
void rinc_storageChanged(const char *path);
void rinc_cacheDrop(const char *name);
extern const char rinc_buildId[];
void rinc_appFail(const char *name, const char *why);
void rinc_say(const char *s);
/* what every Rinc says with no home app to start, into out: how, the platform's way to install apps */
void rinc_noHome(char *out, int size, const char *how);
int rinc_gameFrame(rio_Vm *vm);

/* sound (audio.c): rinc_VOICES voices mixed to rinc_RATE frames a second of 16-bit stereo. Host procs,
   in pc: sound(voice, data, bits, channels, rate), soundRate(voice, rate), soundVolume(voice, 0..1),
   soundPan(voice, -1..1), soundLoop(voice, start, end), soundStop(voice), soundPlaying(voice) -> Int.
   A platform pulls samples with rinc_audioMix, and sets rinc_audioLock when that runs on another
   thread or in an interrupt (called with 1 before a host proc changes a voice, 0 after) */
#define rinc_RATE 48000
#define rinc_VOICES 8
#ifdef rinc_HOT_SECTION /* where code that runs all the time goes: the RP2350 puts it in RAM, not flash */
#define rinc_HOT __attribute__((section(rinc_HOT_SECTION)))
#else
#define rinc_HOT
#endif
#define rinc_ADPCM_BLOCK 256 /* sound(..., 4, ...): ADPCM blocks of this many bytes a channel (505 frames) */
void rinc_audioMix(int16_t *stereo, int frames);
void rinc_audioReset(void); /* every voice stops */
#define rinc_VOLUME_STEPS 16
void rinc_audioVolume(int step); /* the system volume: +1 or -1 steps (2.5 dB), 0 to mute or unmute */
int rinc_audioLevel(void);       /* 0 to rinc_VOLUME_STEPS (full), or -1 muted */
extern void (*rinc_audioLock)(int on);
int rinc_audioProcs(rio_Vm *vm);

/* text: the 5x8 font (ASCII 32-126, bit 0 leftmost) in 6x9 cells, and a console of 53x26 of them
   that scrolls the screen (console.c) */
extern const uint8_t rinc_font[95][8];
extern uint8_t rinc_textFg, rinc_textBg;
void rinc_char(int c, int x, int y, int fg, int bg); /* bg < 0: transparent */
void rinc_consolePut(const char *s, int n);
void rinc_consoleClear(int bg);
/* the theme (pc.theme): read from the theme setting, and a role's color (pc.Role's order: 0 bg, 2 text) */
void rinc_themeLoad(void);
int rinc_themeColor(int role);

/* files, where the platform has storage (else rinc_fs is NULL): one open at a time. open: 0 or -1
   (writing makes the directories along the path, and replaces the file); read: bytes read, 0 at its
   end, -1 failed; write: n, or -1; seek: reading on from byte at (its end, at most): 0 or -1; list: each
   entry of a directory, with whether it's one too.
   Apps live in /rinc/apps/<name>/<name>.rio */
/* an app's name, 1 to 32 letters, digits, _ and -: the installed app, /rinc/apps/<name>; with a +
   after it (chord+), its project, /rinc/projects/<name>, the app in work. rinc_REF holds either */
#define rinc_REF 34
#define rinc_PROJECT '+' /* what marks a project's name: chord+ */
typedef struct {
  int (*open)(const char *path, int write);
  int (*read)(char *buf, int n);
  int (*write)(const char *buf, int n);
  int (*close)(void);
  int (*list)(const char *dir, void (*each)(const char *name, int dir, void *ud), void *ud);
  int (*remove)(const char *path); /* a file, or an empty directory: 0 or -1 */
  int (*copy)(const char *from, const char *to); /* a file, whole (to's directories made, it replaced): 0 or -1 */
  int (*rename)(const char *from, const char *to); /* a file or a directory, to a name not taken (in a directory there): 0 or -1 */
  int (*mkdir)(const char *path); /* a directory, the ones along the way too: 0 or -1 */
  int (*seek)(uint32_t at);
} rinc_Fs;
extern const rinc_Fs *rinc_fs;
/* an app asked to switch (pc.launch): to the app named rinc_launchName, which may be a built-in one,
   "home" or "console". The platform does it, after the frame, and sets rinc_launch back to 0 */
extern int rinc_launch;
extern char rinc_launchName[rinc_REF];
/* the apps a Rinc carries with it, its bundle (docs/spec/apps.md#bundles), in one format everywhere:
   the Fruit Jam's firmware has one built in (RINC_BUNDLE: home and the console, by default), an exported
   desktop program has one at its end, and a web page has one in it. Storage, where there is some, comes
   first, by name. Each app is a source, or (image: 1) a compiled Rio image; info is its description
   (for a source, its first line, without the #) */
#define rinc_BUNDLE_MAGIC "RINCBUN1"
#define rinc_BUNDLE_MOST 256 /* (files) */
/* a bundle is files, by path as in storage without /rinc/ (apps/chord/chord.rio, apps/chord/assets/...):
   an app is apps/name/name.rio (a source), or apps/name/name.rioi (a compiled image, its description in
   apps/name/name.info) */
typedef struct { const char *path, *data; uint32_t len; } rinc_File;
typedef struct { char name[33], info[97]; const char *data; uint32_t len; int image; } rinc_App;
/* takes a bundle, which stays where it is: 0, or -1 if it isn't one */
int rinc_bundleRead(const void *bundle, uint32_t size);
/* writes one: name, its storage folder's name ("" for Rinc's own); home, its home app's name. Its size,
   or 0 if it doesn't fit in cap */
uint32_t rinc_bundleWrite(uint8_t *out, uint32_t cap, const char *name, const char *home, const rinc_File *files, int n);
const rinc_File *rinc_bundleFile(const char *path); /* NULL if the bundle has no such file */
const rinc_App *rinc_bundled(const char *name); /* NULL if the bundle has no such app */
const char *rinc_bundleName(void); /* the bundle's storage folder's name ("": Rinc's own) */
/* the app "home" means (rinc_homeName): the session's home, if the platform set one (rinc_setHome: the
   desktop's rinc whatever), else a bundle's home app, if the bundle carries apps, else the home
   setting (/rinc/settings/home), else home; and any other name, itself */
const char *rinc_appFor(const char *name);
const char *rinc_homeName(void);
void rinc_setHome(const char *name); /* for this session only (NULL: none) */
/* an app's source from storage, read into buf: its length, -1 if storage has no such app, or -2 if it
   doesn't fit */
int rinc_readApp(const char *name, char *buf, uint32_t cap);
/* the app running now, by name: its save space is /rinc/save/<name> (pc.saveRead and so on) */
void rinc_setAppName(const char *name);

/* a REPL over any text stream: feed it characters, it echoes them and runs each complete input.
   Besides rio, it knows a few commands, each a line on its own: run (the game loop, for what's been
   typed), home (back home, where there is one), and with storage: ls (the apps), load name (an app's source, as if typed, into a fresh
   session: restart gives one), run name (load, then run), save name (what's typed next, until
   Ctrl-D, becomes the app's source; Ctrl-C gives up), put path (any file under /rinc, such as
   apps/chord/parts/sound.rio or help/rinc/README.md: hex digits until Ctrl-D, any bytes, anything else
   between the digits ignored; for tools/install.py), delete name (its source and folder, if that's
   all the folder holds) */
typedef struct {
  rio_Vm *vm;
  void (*put)(const char *s, int n);
  char acc[4096];
  int n, line0; /* acc holds the unfinished input; the current line starts at line0 */
  int echo;      /* echo typed characters (serial terminals don't; a desktop terminal already did) */
  int run;       /* the line "run" asks for the game loop: the platform runs frames while it's set */
  /* editing: the cursor (in acc), where the screen's cursor is (from line0), a key's escape sequence
     arriving, the color state at the line's end (mode, and where its unfinished word starts) */
  int cur, scr, esc, escn, mode, word, numdot; const char *wp; /* wp: the unfinished word's color now; numdot: a number's dot, for now */
  int autoind; /* spaces of the current line's auto indent that typed spaces can still take the place of */
  char hist[1024], draft[256]; int hlen, back, draftn; /* lines typed before, and the one being typed */
  int (*restart)(void); /* a fresh rio session, for load (0: done) */
  int loading, errors, saving, saved, cr; char out[256]; int nout; /* load and save as they go */
  int hex, half; /* put: the file comes as hex digits; half, a byte's first digit (or -1) */
} rinc_Repl;
int rinc_replBegin(rinc_Repl *r, rio_Vm *vm, void *scratch, uint32_t size);
void rinc_replStop(rinc_Repl *r, const char *why); /* the game loop ended (why: NULL, or a message) */
void rinc_replChar(rinc_Repl *r, int c);
/* a fresh session with an app from storage, or with source from memory (home), running:
   0, or -1 (what went wrong is said) */
int rinc_replApp(rinc_Repl *r, const char *name, int run);
int rinc_replSource(rinc_Repl *r, const char *src, int len);

#endif
