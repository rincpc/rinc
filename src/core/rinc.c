#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "rinc.h"
#include "pc_host.h" /* (pc.rio as C, its source, structs and sigs: rinc -H src/core/pc_host.h src/core/pc.rio) */
#include "core_host.h" /* (Rio's core.rio, its source, for pc.moduleSource: rinc -H src/core/core_host.h lib/rio/src/core.rio) */

_Alignas(4) uint8_t rinc_screen[rinc_H][rinc_W]; /* (aligned for DMA) */
uint16_t rinc_palette[256];
float (*rinc_time)(void);
uint16_t rinc_buttons;
int16_t rinc_axes[6];
static void (*rinc_put)(const char *s, int n);

static uint16_t rinc_rgb565(int r, int g, int b) { return (uint16_t)((r >> 3) << 11 | (g >> 2) << 5 | b >> 3); }

/* color i of the palette as 0xRRGGBB (docs/spec/screen.md#the-palette): shown at 16 bits a pixel, RGB565,
   and written as it is in PNG files. palette.h is tools/palette.py's */
#include "palette.h"
uint32_t rinc_rgb(int i) { return rinc_colors[i & 255]; }
static void rinc_initPalette(void) {
  int i;
  for (i = 0; i < 256; i++) {
    uint32_t c = rinc_rgb(i);
    rinc_palette[i] = rinc_rgb565((int)(c >> 16), (int)(c >> 8 & 255), (int)(c & 255));
  }
}

/* host procs: arguments arrive in a[], a result goes in a[0]. Drawing outside the screen is clipped */
static void rinc_hostCls(rio_Vm *v, rio_Val *a) { (void)v; memset(rinc_screen, (uint8_t)a[0].i, sizeof rinc_screen); }
static void rinc_hostPset(rio_Vm *v, rio_Val *a) {
  uint32_t x = a[0].u, y = a[1].u;
  (void)v;
  if (x < rinc_W && y < rinc_H) rinc_screen[y][x] = (uint8_t)a[2].i;
}
static void rinc_hostRgb(rio_Vm *v, rio_Val *a) {
  (void)v;
  a[0].i = (int32_t)rinc_rgb((int)a[0].i);
}
static void rinc_hostPget(rio_Vm *v, rio_Val *a) {
  uint32_t x = a[0].u, y = a[1].u;
  (void)v;
  a[0].i = x < rinc_W && y < rinc_H ? rinc_screen[y][x] : 0;
}
/* blit(image, at, b: Blit): a picture (pc.Image: its pixels, a byte each, row by row, and its width and
   height), or b.size of it from b.from (a 0 in size: the rest of it that way),
   drawn at at, each pixel b.scale wide and high (0: 1; negative: mirrored that way); color 0 is
   see-through. b left out: all zeros */
static void rinc_hostBlit(rio_Vm *v, rio_Val *a) {
  pc_Image im; pc_Blit b; const uint8_t *p; uint32_t n;
  int pw, ph, sx, sy, w, h, x0, y0, kx, ky, flipX, flipY, i, j;
  memset(&im, 0, sizeof im); memset(&b, 0, sizeof b);
  rio_getStruct(v, a, &im, sizeof im);
  rio_getStruct(v, a + 4, &b, sizeof b);
  p = (const uint8_t *)rio_viewBytes(v, im.pixels); n = im.pixels.len; pw = im.size[0]; ph = im.size[1];
  sx = b.from[0]; sy = b.from[1]; w = b.size[0]; h = b.size[1]; x0 = a[2].i; y0 = a[3].i;
  flipX = b.scale[0] < 0; flipY = b.scale[1] < 0;
  kx = flipX ? -b.scale[0] : b.scale[0] ? b.scale[0] : 1; ky = flipY ? -b.scale[1] : b.scale[1] ? b.scale[1] : 1;
  if (!p || pw <= 0 || ph <= 0 || (uint64_t)pw * (uint32_t)ph > n) return;
  if (w <= 0) w = pw - (sx > 0 ? sx : 0); /* (0: the rest of the picture) */
  if (h <= 0) h = ph - (sy > 0 ? sy : 0);
  if (sx < 0) { w += sx; x0 -= sx * kx; sx = 0; } /* (the part that's in the picture) */
  if (sy < 0) { h += sy; y0 -= sy * ky; sy = 0; }
  if (w > pw - sx) w = pw - sx;
  if (h > ph - sy) h = ph - sy;
  if (kx == 1 && ky == 1 && !flipX) { /* (the usual: a pixel for a pixel, a row at a time; a map's tiles are many) */
    int i0 = x0 < 0 ? -x0 : 0, i1 = x0 + w > rinc_W ? rinc_W - x0 : w;
    for (j = 0; j < h; j++) {
      int y = y0 + j;
      const uint8_t *row = p + (sy + (flipY ? h - 1 - j : j)) * pw + sx;
      uint8_t *out;
      if (y < 0 || y >= rinc_H) continue;
      out = rinc_screen[y];
      for (i = i0; i < i1; i++)
        if (row[i]) out[x0 + i] = row[i];
    }
    return;
  }
  for (j = 0; j < h; j++) {
    const uint8_t *row = p + (sy + (flipY ? h - 1 - j : j)) * pw + sx; /* (mirrored: the rows from the bottom) */
    int y = y0 + j * ky, ya = y < 0 ? 0 : y, yb = y + ky > rinc_H ? rinc_H : y + ky;
    if (ya >= yb) continue;
    for (i = 0; i < w; i++) {
      int x = x0 + i * kx, xa = x < 0 ? 0 : x, xb = x + kx > rinc_W ? rinc_W : x + kx, yy;
      uint8_t c = row[flipX ? w - 1 - i : i];
      if (!c || xa >= xb) continue;
      for (yy = ya; yy < yb; yy++) memset(&rinc_screen[yy][xa], c, (size_t)(xb - xa));
    }
  }
}
static void rinc_hostRect(rio_Vm *v, rio_Val *a) {
  int x0 = a[0].i, y0 = a[1].i, x1 = x0 + a[2].i, y1 = y0 + a[3].i, y;
  (void)v;
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > rinc_W) x1 = rinc_W;
  if (y1 > rinc_H) y1 = rinc_H;
  for (y = y0; y < y1 && x0 < x1; y++) memset(&rinc_screen[y][x0], (uint8_t)a[4].i, (size_t)(x1 - x0));
}
static void rinc_hostPrint(rio_Vm *v, rio_Val *a) { /* print(s, x, y, c): text at pixel x, y */
  const char *s = (const char *)rio_sliceBytes(v, a); int i, x = a[2].i, y = a[3].i;
  if (!s) return;
  for (i = 0; i < a[1].i; i++, x += 6) rinc_char((unsigned char)s[i], x, y, a[4].i & 255, -1);
}
/* key() -> Int: the next key typed, or -1 when there's none waiting (docs/spec/input.md#typed-text):
   printable ASCII, 13 Enter, 8 Backspace, 9 Tab, 27 Esc, Ctrl+letter as 1-26, and editing keys as
   rinc_Key_*; the keys that aren't characters with the modifiers held (rinc_Mod_*). The keys also press
   buttons as they're held: an app reads whichever it wants. A nested session (pc.rioStart) has a queue
   of its own, which its app fills with pc.rioType */
#define rinc_TEXT_CAP 64
typedef struct { uint16_t q[rinc_TEXT_CAP]; int head, tail, esc, escn, escp[3]; } rinc_Text;
static rinc_Text rinc_typed, rinc_nestTyped;
/* the modifiers locked (tapped twice: rinc_systemKey), as rinc_Mod_* bits; what they do to a key typed:
   Shift capitalizes letters, Ctrl makes a letter Ctrl+letter, and on a key that isn't a character each
   adds its bit (as held ones do: Enter and Backspace take no Shift, Esc nothing) */
static int rinc_locks;
static int rinc_locked(int c) {
  int lowered = c >= 'A' && c <= 'Z' ? c + 32 : c;
  if (!rinc_locks || c == 27) return c;
  if (lowered >= 'a' && lowered <= 'z' && c < 128) {
    if (rinc_locks & rinc_Mod_ctrl) return (lowered - 'a' + 1) | (rinc_locks & (rinc_Mod_shift | rinc_Mod_alt));
    return rinc_locks & rinc_Mod_shift ? lowered - 32 : c;
  }
  if (c >= 32 && c < 127) return c; /* (other characters: as typed) */
  if ((c & 255) == 13 || (c & 255) == 8) return c | (rinc_locks & ~rinc_Mod_shift);
  if ((c & 255) >= 1 && (c & 255) <= 26 && (c & 255) != 9) return c | (rinc_locks & ~rinc_Mod_ctrl); /* (Ctrl+letter says Ctrl) */
  return c | rinc_locks;
}
static int rinc_pointerOn; /* (the pointer's, below) */
static void rinc_queue(rinc_Text *t, int c) {
  int next = (t->head + 1) % rinc_TEXT_CAP;
  if (t == &rinc_typed) { c = rinc_locked(c); rinc_pointerOn = rinc_pointerOnScreen = 0; } /* (typing hides the pointer) */
  if (next != t->tail) { t->q[t->head] = (uint16_t)c; t->head = next; }
}
static int rinc_dequeue(rinc_Text *t) {
  int c;
  if (t->tail == t->head) return -1;
  c = t->q[t->tail]; t->tail = (t->tail + 1) % rinc_TEXT_CAP;
  return c;
}
int rinc_textMods(int shift, int ctrl, int alt) { return (shift ? rinc_Mod_shift : 0) | (ctrl ? rinc_Mod_ctrl : 0) | (alt ? rinc_Mod_alt : 0); }
/* a terminal's bytes: ESC [ (or ESC O) and a letter, or numbers and ~, is an editing key. Numbers are
   separated by ; and a key's modifiers are its second (1 + 1 Shift, 2 Alt, 4 Ctrl), as xterm sends
   them: ESC [ 1 ; 5 C is Ctrl+right, ESC [ 3 ; 2 ~ Shift+Delete, and ESC [ 27 ; m ; k ~ key k (Tab,
   Enter, Backspace, a letter for Ctrl+letter, a space or / for Ctrl+Space or Ctrl+/) with modifiers m.
   ESC [ Z is Shift+Tab. A NUL byte (a terminal's Ctrl+Space) is Ctrl+Space, 32 with rinc_Mod_ctrl, and
   31 (its Ctrl+/, as Ctrl+_) is Ctrl+/, 47 with it */
void rinc_textPush(int c) {
  rinc_Text *t = &rinc_typed;
  if (t->esc == 1) {
    if (c == '[' || c == 'O') { t->esc = 2; t->escn = 0; t->escp[0] = t->escp[1] = t->escp[2] = 0; return; }
    t->esc = 0; rinc_queue(t, 27); /* (a lone ESC: Esc itself, then c) */
  } else if (t->esc == 2) {
    int n = t->escp[0], m = t->escp[1] > 1 ? t->escp[1] - 1 : 0, mods = rinc_textMods(m & 1, m & 4, m & 2), k = t->escp[2], c0;
    if (c >= '0' && c <= '9') { if (t->escp[t->escn] < 10000) t->escp[t->escn] = t->escp[t->escn] * 10 + c - '0'; return; }
    if (c == ';') { if (t->escn < 2) t->escn++; return; }
    t->esc = 0;
    c0 = c;
    if (c == '~' && n == 27) { /* (xterm's modifyOtherKeys) */
      if (k >= 'A' && k <= 'Z') k += 32;
      c = k == 9 || k == 13 || k == 32 || k == '/' ? k : k == 8 || k == 127 ? 8 : k >= 'a' && k <= 'z' && (mods & rinc_Mod_ctrl) ? k - 'a' + 1 : 0;
      if ((c == 32 || c == '/') && !(mods & rinc_Mod_ctrl)) c = 0; /* (Ctrl+Space and Ctrl+/: 32 or 47 with Ctrl's bit, nothing else) */
      if (c >= 1 && c <= 26 && c != 8 && c != 9 && c != 13) mods &= ~rinc_Mod_ctrl; /* (Ctrl+letter says Ctrl itself) */
    } else
      c = c == 'A' ? rinc_Key_up : c == 'B' ? rinc_Key_down : c == 'C' ? rinc_Key_right : c == 'D' ? rinc_Key_left :
          c == 'H' ? rinc_Key_home : c == 'F' ? rinc_Key_end : c == 'Z' ? 9 | rinc_Mod_shift : c != '~' ? 0 :
          n == 1 || n == 7 ? rinc_Key_home : n == 4 || n == 8 ? rinc_Key_end : n == 3 ? rinc_Key_delete :
          n == 5 ? rinc_Key_pageUp : n == 6 ? rinc_Key_pageDown : 0;
    if (c0 >= 'P' && c0 <= 'S') c = rinc_Key_f1 + c0 - 'P'; /* (F1 to F4: ESC O P to S, or ESC [ 1 ; m P with modifiers) */
    if (c0 == '~' && n >= 11 && n <= 24 && n != 16 && n != 22) c = rinc_Key_f1 + (n <= 15 ? n - 11 : n <= 21 ? n - 12 : n - 13); /* (F1 to F12: n ~, as xterm and the console send them) */
    if (c) rinc_queue(t, c | mods);
    return;
  }
  if (c == 27) { t->esc = 1; return; }
  if (c == '\n') c = 13;
  if (c == 127) c = 8;
  if (c == 0) c = 32 | rinc_Mod_ctrl; /* (a terminal's Ctrl+Space) */
  if (c == 31) c = '/' | rinc_Mod_ctrl; /* (and its Ctrl+/, as Ctrl+_) */
  if ((c >= 1 && c <= 26) || (c >= 32 && c < 127) || c == (32 | rinc_Mod_ctrl) || c == ('/' | rinc_Mod_ctrl)) rinc_queue(t, c);
}
void rinc_textKey(int key) { rinc_typed.esc = 0; if (key > 0 && key < 2048) rinc_queue(&rinc_typed, key); }
int rinc_textRoom(void) { return rinc_TEXT_CAP - 1 - (rinc_typed.head - rinc_typed.tail + rinc_TEXT_CAP) % rinc_TEXT_CAP; }
/* apps, for home: apps() -> Int looks again, in storage and in the bundle, and says how many (home
   aside); appName(i, out) -> Int and appInfo(i, out) -> Int copy app i's name, or its description (its
   source's first line, the comment marker left off: "Title: what it does"), into out, and say how long
   it is. launch(name) asks the platform to switch to an app, after this frame */
const rinc_Fs *rinc_fs;
static const uint8_t *rinc_bundle; static int rinc_bundleCount; /* (read where it is: on the Fruit Jam, flash) */
static char rinc_bundleStorage[33], rinc_bundleHome[33] = "home";
int rinc_launch; char rinc_launchName[rinc_REF];
static int rinc_cmdWanted, rinc_cmdSettle; /* the command list: asked for (Sys+P, pc.showCommands); just closed, the buttons and pointer held then not the app's (below) */
/* the bundle format (little-endian): "RINCBUN1", the file count (4 bytes), the storage folder's name and
   the home app's name (32 bytes each, 0-padded), then each file's entry: its path (96 bytes, 0-padded,
   as in storage without /rinc/: apps/chord/chord.rio), and where its bytes are (4, from the bundle's
   start) and how many (4); then the bytes */
#define rinc_BUNDLE_HEAD 76
#define rinc_BUNDLE_ENTRY 104
static uint32_t rinc_le32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static void rinc_put32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static void rinc_text(char *out, const uint8_t *in, int n) { memcpy(out, in, (size_t)n); out[n] = 0; }
int rinc_bundleRead(const void *bundle, uint32_t size) {
  const uint8_t *b = (const uint8_t *)bundle; uint32_t n, i;
  if (size < rinc_BUNDLE_HEAD || memcmp(b, rinc_BUNDLE_MAGIC, 8) || (n = rinc_le32(b + 8)) > rinc_BUNDLE_MOST || rinc_BUNDLE_HEAD + n * rinc_BUNDLE_ENTRY > size) return -1;
  for (i = 0; i < n; i++) {
    const uint8_t *e = b + rinc_BUNDLE_HEAD + i * rinc_BUNDLE_ENTRY; uint32_t at = rinc_le32(e + 96), len = rinc_le32(e + 100);
    if (at > size || len > size - at) return -1;
  }
  rinc_text(rinc_bundleStorage, b + 12, 32);
  rinc_text(rinc_bundleHome, b + 44, 32);
  rinc_bundle = b;
  rinc_bundleCount = (int)n;
  return 0;
}
static void rinc_fill(uint8_t *out, const char *s, size_t n) { size_t k = strlen(s); memcpy(out, s, k < n ? k : n); } /* (the rest stays 0) */
uint32_t rinc_bundleWrite(uint8_t *out, uint32_t cap, const char *name, const char *home, const rinc_File *files, int n) {
  uint32_t at = rinc_BUNDLE_HEAD + (uint32_t)n * rinc_BUNDLE_ENTRY; int i;
  if (n > rinc_BUNDLE_MOST || at > cap) return 0;
  memset(out, 0, at);
  memcpy(out, rinc_BUNDLE_MAGIC, 8); rinc_put32(out + 8, (uint32_t)n);
  rinc_fill(out + 12, name, 32); rinc_fill(out + 44, home, 32);
  for (i = 0; i < n; i++) {
    uint8_t *e = out + rinc_BUNDLE_HEAD + i * rinc_BUNDLE_ENTRY;
    if (files[i].len > cap - at) return 0;
    if (strlen(files[i].path) >= 96) return 0; /* (paths end with a 0 in the bundle) */
    rinc_fill(e, files[i].path, 96);
    rinc_put32(e + 96, at); rinc_put32(e + 100, files[i].len);
    memcpy(out + at, files[i].data, files[i].len);
    at += files[i].len;
  }
  return at;
}
const char *rinc_bundleName(void) { return rinc_bundleStorage; }
/* the app home means: the session's (rinc_setHome: the desktop's rinc whatever), else a bundle's home
   app (one that carries apps: an exported game, a web page), else the home setting, else home */
static char rinc_homeOver[rinc_REF], rinc_homeSet[rinc_REF];
static int rinc_readFile(const char *path, char *buf, uint32_t cap, uint32_t *total);
/* a builder argument, Blob[..] (where its length is, where its bytes start, its capacity): rinc_into finds
   its end, where more go, and the room past it (0 for a bad one); rinc_outAdd counts n more bytes in it */
uint8_t *rinc_into(rio_Vm *v, const rio_Val *b, rinc_Out *o) {
  rio_Val at[2]; const uint8_t *l; uint32_t n;
  o->len[0] = b[0]; o->len[1].u = 4; o->end = 0; o->room = 0;
  if (!(l = (const uint8_t *)rio_sliceBytes(v, o->len))) return 0;
  memcpy(&n, l, 4);
  at[0] = b[1]; at[1] = b[2];
  if (n > b[2].u || !(o->end = (uint8_t *)rio_sliceBytes(v, at))) return o->end = 0;
  o->end += n; o->room = b[2].u - n;
  return o->end;
}
void rinc_outAdd(rio_Vm *v, rinc_Out *o, uint32_t n) {
  uint8_t *l = (uint8_t *)rio_sliceBytes(v, o->len); uint32_t k;
  if (!l || !o->end || n > o->room) return;
  memcpy(&k, l, 4); k += n; memcpy(l, &k, 4);
  o->end += n; o->room -= n;
}
/* n bytes of s added to a builder, as many as fit: n */
static int rinc_addOut(rio_Vm *v, const rio_Val *b, const char *s, int n) {
  rinc_Out o; uint32_t k;
  if (n > 0 && rinc_into(v, b, &o)) { k = (uint32_t)n < o.room ? (uint32_t)n : o.room; memcpy(o.end, s, k); rinc_outAdd(v, &o, k); }
  return n;
}
/* a read into a builder: as much as fits, added; its whole length, or -1 */
static int32_t rinc_readOut(rio_Vm *v, const rio_Val *b, const char *path) {
  rinc_Out o; uint32_t total;
  if (!rinc_into(v, b, &o) || rinc_readFile(path, (char *)o.end, o.room, &total)) return -1;
  rinc_outAdd(v, &o, total < o.room ? total : o.room);
  return (int32_t)total;
}
static int rinc_refOk(const char *s, int n);
void rinc_setHome(const char *name) { snprintf(rinc_homeOver, sizeof rinc_homeOver, "%s", name ? name : ""); }
const char *rinc_homeName(void) {
  uint32_t n;
  if (rinc_homeOver[0]) return rinc_homeOver;
  if (rinc_bundleCount) return rinc_bundleHome;
  if (!rinc_readFile("/rinc/settings/home", rinc_homeSet, sizeof rinc_homeSet - 1, &n) && n < sizeof rinc_homeSet) {
    while (n && (rinc_homeSet[n - 1] == '\n' || rinc_homeSet[n - 1] == '\r' || rinc_homeSet[n - 1] == ' ')) n--; /* (as an editor saves it) */
    rinc_homeSet[n] = 0;
    if (rinc_refOk(rinc_homeSet, (int)n)) return rinc_homeSet;
  }
  return "home";
}
const char *rinc_appFor(const char *name) { return strcmp(name, "home") ? name : rinc_homeName(); }
static char rinc_appNow[33] = "app"; /* the app running, for its stash (a project's is its app's) */
static char rinc_appRef[rinc_REF] = "app"; /* and as it was started: chord, or its project, chord+ */
static int rinc_appProject; /* (it's a project: its own save space) */
void rinc_setAppName(const char *name) {
  size_t n = strlen(name);
  if (n >= sizeof rinc_appRef) n = sizeof rinc_appRef - 1;
  memcpy(rinc_appRef, name, n); rinc_appRef[n] = 0;
  if (strchr(rinc_appRef, '/')) n = (size_t)(strchr(rinc_appRef, '/') - rinc_appRef); /* (another of its files, run as it: edit/test is edit) */
  rinc_appProject = n > 1 && name[n - 1] == rinc_PROJECT;
  if (rinc_appProject) n--;
  if (n >= sizeof rinc_appNow) n = sizeof rinc_appNow - 1;
  memcpy(rinc_appNow, name, n); rinc_appNow[n] = 0;
}
static const char *rinc_entryPath(int i) { return (const char *)rinc_bundle + rinc_BUNDLE_HEAD + i * rinc_BUNDLE_ENTRY; }
const rinc_File *rinc_bundleFile(const char *path) {
  static rinc_File f; int i;
  for (i = 0; i < rinc_bundleCount; i++)
    if (!strncmp(rinc_entryPath(i), path, 96)) {
      const uint8_t *e = (const uint8_t *)rinc_entryPath(i);
      f.path = (const char *)e; f.data = (const char *)rinc_bundle + rinc_le32(e + 96); f.len = rinc_le32(e + 100);
      return &f;
    }
  return 0;
}
/* app name in the bundle: apps/name/name.rio (a source), or apps/name/name.rioi (an image, its
   description in apps/name/name.info) */
const rinc_App *rinc_bundled(const char *name) {
  static rinc_App a; char path[96]; const rinc_File *f, *info; uint32_t k; const char *data; uint32_t len;
  if (strlen(name) > 32) return 0;
  snprintf(path, sizeof path, "apps/%s/%s.rio", name, name);
  a.image = !(f = rinc_bundleFile(path));
  if (a.image) {
    snprintf(path, sizeof path, "apps/%s/%s.rioi", name, name);
    if (!(f = rinc_bundleFile(path))) return 0;
  }
  data = f->data; len = f->len; /* (before the next lookup, which reuses f) */
  snprintf(a.name, sizeof a.name, "%s", name);
  a.data = data; a.len = len; a.info[0] = 0;
  snprintf(path, sizeof path, "apps/%s/%s.info", name, name);
  if (a.image && (info = rinc_bundleFile(path))) {
    for (k = 0; k < info->len && k < sizeof a.info - 1 && info->data[k] != '\n' && info->data[k] != '\r'; k++) a.info[k] = info->data[k];
    a.info[k] = 0;
  }
  return &a;
}
static uint32_t rinc_appLen(const rinc_App *a) { return a->len; }
/* a name for an app (letters, digits, _ and -) or, with dots, for a file in a save space (not starting
   with one): 1 to 32 of them */
static int rinc_nameOk(const char *s, int n, int dots) {
  int i;
  if (!s || n < 1 || n > 32 || (dots && s[0] == '.')) return 0;
  for (i = 0; i < n; i++)
    if (!((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= '0' && s[i] <= '9') || s[i] == '_' || s[i] == '-' || (dots && s[i] == '.'))) return 0;
  return 1;
}
/* an app's name, or its project's (a + after it) */
static int rinc_refOk(const char *s, int n) { return n > 1 && s[n - 1] == rinc_PROJECT ? rinc_nameOk(s, n - 1, 0) : rinc_nameOk(s, n, 0); }
static char rinc_appNames[64][rinc_REF], rinc_appHome[rinc_REF]; static int rinc_appCount;
static void rinc_appAdd(const char *name) {
  size_t n = strlen(name); int i;
  if (!rinc_refOk(name, (int)n) || !strcmp(name, rinc_appHome) || rinc_appCount >= 64) return; /* (home: where this list is shown) */
  for (i = 0; i < rinc_appCount; i++) if (!strcmp(rinc_appNames[i], name)) return;
  memcpy(rinc_appNames[rinc_appCount++], name, n + 1);
}
static void rinc_appFound(const char *name, int dir, void *ud) {
  char ref[rinc_REF];
  if (!dir || strlen(name) > 32) return;
  snprintf(ref, sizeof ref, "%s%s", name, ud ? "+" : ""); /* (ud: projects) */
  rinc_appAdd(ref);
}
static int rinc_lowerCmp(const char *a, const char *b) {
  for (;; a++, b++) {
    int x = *a >= 'A' && *a <= 'Z' ? *a + 32 : *a, y = *b >= 'A' && *b <= 'Z' ? *b + 32 : *b;
    if (x != y || !x) return x - y;
  }
}
/* the apps, in storage and in the bundle, and the projects (chord+), in order of name (ignoring case,
   each project after its app): how many */
static int rinc_exists(const char *path);
static int rinc_findApps(void) {
  int i, j;
  rinc_appCount = 0;
  snprintf(rinc_appHome, sizeof rinc_appHome, "%s", rinc_homeName()); /* (read first: storage has one file open at a time) */
  if (rinc_fs) { rinc_fs->list("/rinc/apps", rinc_appFound, 0); rinc_fs->list("/rinc/projects", rinc_appFound, (void *)1); }
  for (i = j = 0; i < rinc_appCount; i++) { /* (a folder is an app if its main file is there, and it isn't Rinc's own, which holds themes) */
    char path[112], *dot; int n = (int)strlen(rinc_appNames[i]), project = rinc_appNames[i][n - 1] == rinc_PROJECT;
    if (project) n--;
    snprintf(path, sizeof path, "/rinc/%s/%.*s/%.*s.rio", project ? "projects" : "apps", n, rinc_appNames[i], n, rinc_appNames[i]);
    dot = path + strlen(path) - 4;
    if (!project && !strcmp(rinc_appNames[i], *rinc_bundleStorage ? rinc_bundleStorage : "rinc")) continue; /* (the system's own: its themes) */
    if (!rinc_exists(path)) { memcpy(dot, ".rioi", 6); if (!rinc_exists(path)) continue; }
    if (i != j) memcpy(rinc_appNames[j], rinc_appNames[i], sizeof rinc_appNames[0]);
    j++;
  }
  rinc_appCount = j;
  for (i = 0; i < rinc_bundleCount; i++) { /* (apps/name/name.rio or .rioi) */
    const char *q = rinc_entryPath(i), *slash; char name[33]; size_t k;
    if (strncmp(q, "apps/", 5) || !(slash = strchr(q + 5, '/')) || (k = (size_t)(slash - q - 5)) > 32) continue;
    memcpy(name, q + 5, k); name[k] = 0;
    if (!strncmp(slash + 1, name, k) && (!strcmp(slash + 1 + k, ".rio") || !strcmp(slash + 1 + k, ".rioi"))) rinc_appAdd(name);
  }
  for (i = 1; i < rinc_appCount; i++) /* (a few dozen at most: insertion) */
    for (j = i; j > 0 && rinc_lowerCmp(rinc_appNames[j - 1], rinc_appNames[j]) > 0; j--) {
      char t[sizeof rinc_appNames[0]]; memcpy(t, rinc_appNames[j], sizeof t); memcpy(rinc_appNames[j], rinc_appNames[j - 1], sizeof t); memcpy(rinc_appNames[j - 1], t, sizeof t);
    }
  return rinc_appCount;
}
/* a file in storage, into buf as far as it fits: 0 (*total: its whole length), or -1 */
/* a file the bundle carries, by its storage path (/rinc/apps/...): as rinc_readFile reads one */
static int rinc_readBundled(const char *path, char *buf, uint32_t cap, uint32_t *total) {
  const rinc_File *f = strncmp(path, "/rinc/", 6) ? 0 : rinc_bundleFile(path + 6);
  if (!f) return -1;
  if (buf) memcpy(buf, f->data, f->len < cap ? f->len : cap);
  *total = f->len;
  return 0;
}
/* a file, from storage, else the bundle: as much as fits in buf (cap), and its whole length (total) */
static int rinc_readFile(const char *path, char *buf, uint32_t cap, uint32_t *total) {
  char chunk[256]; int got;
  *total = 0;
  if (!rinc_fs || rinc_fs->open(path, 0)) return rinc_readBundled(path, buf, cap, total);
  while ((got = rinc_fs->read(chunk, sizeof chunk)) > 0) {
    if (buf && *total < cap) memcpy(buf + *total, chunk, (size_t)got < cap - *total ? (size_t)got : cap - *total);
    *total += (uint32_t)got;
  }
  rinc_fs->close();
  return got < 0 ? -1 : 0;
}
/* a file from byte at on, from storage, else the bundle: as much as fits in buf (cap), how much that was
   (got), and its whole length, or -1 */
static int32_t rinc_fileFrom(const char *path, uint32_t at, char *buf, uint32_t cap, uint32_t *got) {
  int32_t size; int k;
  *got = 0;
  if (!rinc_fs || rinc_fs->open(path, rinc_READ)) {
    const rinc_File *f = strncmp(path, "/rinc/", 6) ? 0 : rinc_bundleFile(path + 6);
    if (!f) return -1;
    if (at < f->len) { *got = f->len - at < cap ? f->len - at : cap; memcpy(buf, f->data + at, *got); }
    return (int32_t)f->len;
  }
  if ((size = rinc_fs->size()) >= 0 && at < (uint32_t)size && cap) {
    if (rinc_fs->seek(at)) size = -1;
    while (size >= 0 && *got < cap && (k = rinc_fs->read(buf + *got, (int)(cap - *got))) != 0) {
      if (k < 0) size = -1; else *got += (uint32_t)k;
    }
  }
  rinc_fs->close();
  return size;
}
/* n bytes written over a file from byte at on (-1: at its end; past its end, no), the file made if it
   isn't there (its folders too), and closed again, so it's all on the card: where they went, or -1 */
static int32_t rinc_fileOver(const char *path, int32_t at, const char *data, int n) {
  int32_t size; int bad;
  if (!rinc_fs || n < 0 || (n && !data) || rinc_fs->open(path, rinc_UPDATE)) return -1;
  size = rinc_fs->size();
  if (at == -1) at = size;
  bad = size < 0 || at < 0 || at > size || rinc_fs->seek((uint32_t)at) || (n && rinc_fs->write(data, n) != n);
  return rinc_fs->close() || bad ? -1 : at;
}
/* an app's source, or its project's (chord+) */
static int rinc_appPath(const char *name, char *path, size_t cap) {
  int n = (int)strlen(name), project = n > 1 && name[n - 1] == rinc_PROJECT;
  if (!rinc_refOk(name, n)) return 0;
  if (project) n--;
  return snprintf(path, cap, "/rinc/%s/%.*s/%.*s.rio", project ? "projects" : "apps", n, name, n, name) < (int)cap;
}
/* what name runs: an app's main file, or app/file, another file in its folder run as the app (edit/test:
   /rinc/apps/edit/test.rio, its tests, its own files included from beside it); app: the app's name */
static int rinc_runPath(const char *name, char *app, size_t appCap, char *path, size_t cap) {
  const char *slash = strchr(name, '/'); int n, project;
  if (!slash) return snprintf(app, appCap, "%s", name) < (int)appCap && rinc_appPath(name, path, cap);
  n = (int)(slash - name);
  if (!rinc_refOk(name, n) || !rinc_nameOk(slash + 1, (int)strlen(slash + 1), 0) || n >= (int)appCap) return 0;
  memcpy(app, name, (size_t)n); app[n] = 0;
  project = name[n - 1] == rinc_PROJECT;
  return snprintf(path, cap, "/rinc/%s/%.*s/%s.rio", project ? "projects" : "apps", project ? n - 1 : n, name, slash + 1) < (int)cap;
}
int rinc_readApp(const char *name, char *buf, uint32_t cap) {
  char path[96], app[rinc_REF]; uint32_t total;
  if (!rinc_runPath(name, app, sizeof app, path, sizeof path) || rinc_readFile(path, buf, cap, &total)) return -1;
  return total > cap ? -2 : (int)total;
}
void rinc_cacheDrop(const char *name); /* (compiled apps: below) */
static void rinc_hostApps(rio_Vm *v, rio_Val *a) { (void)v; a[0].i = rinc_findApps(); }
static void rinc_hostAppName(rio_Vm *v, rio_Val *a) {
  a[0].i = a[0].u < (uint32_t)rinc_appCount ? rinc_addOut(v, a + 1, rinc_appNames[a[0].i], (int)strlen(rinc_appNames[a[0].i])) : 0;
}
static void rinc_hostAppInfo(rio_Vm *v, rio_Val *a) {
  char path[96], line[128]; const char *t = line; uint32_t n = 0; int i, k = a[0].i; const rinc_App *b;
  a[0].i = 0;
  if (k < 0 || k >= rinc_appCount) return;
  if (rinc_appPath(rinc_appNames[k], path, sizeof path) && !rinc_readFile(path, line, sizeof line, &n)) {
    if (n > sizeof line) n = sizeof line;
  } else if ((b = rinc_bundled(rinc_appNames[k])) && b->image) {
    a[0].i = rinc_addOut(v, a + 1, b->info, (int)strlen(b->info));
    return;
  } else if (b) { t = b->data; n = rinc_appLen(b); }
  if (n == 0 || t[0] != '#') return;
  for (i = 1; i < (int)n && t[i] == ' '; i++) {}
  for (k = i; k < (int)n && t[k] != '\n' && t[k] != '\r'; k++) {}
  a[0].i = rinc_addOut(v, a + 1, t + i, k - i);
}
/* help texts, for a help app: each app's own, in its help folder (/rinc/apps/<app>/help/..., named
   <app>/...), and those that are no app's (Rinc's and Rio's docs), in /rinc/help (named by their path
   there), and each project's (/rinc/projects/<app>/help/..., named with the +, chord+ then /...); in storage, and in the
   bundle the same way (apps/<app>/help/..., help/...). helps(out) -> Int looks for them, and puts their
   names in out (.md left off: rinc/spec/apps), in order, each with a 10 after it, as many as fit: how
   many; helpRead(name, out) -> Int copies a text (from storage, else the bundle) into out, as much as
   fits: its whole length, or -1 */
static char *rinc_helpOut; static int rinc_helpCap, rinc_helpUsed, rinc_helpCount; /* (the names, in the app's own memory) */
static char rinc_helpDirs[16][96]; static int rinc_helpDirCount, rinc_helpDirNow; /* (folders to look through, in storage) */
/* a help text's name: letters, digits, _, - and dots, in parts split by /, none starting with a dot (and
   a project's +, chord+, ending the first) */
static int rinc_helpNameOk(const char *s, int n) {
  int i;
  if (!s || n < 1 || n > 47 || s[0] == '/' || s[n - 1] == '/') return 0;
  for (i = 0; i < n; i++) {
    char c = s[i];
    if (c == '.' && (i == 0 || s[i - 1] == '/')) return 0;
    if (c == '/' && s[i - 1] == '/') return 0;
    if (c == rinc_PROJECT && (i == 0 || i + 1 >= n || s[i + 1] != '/' || memchr(s, '/', (size_t)i))) return 0;
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' || c == '/' || c == rinc_PROJECT)) return 0;
  }
  return 1;
}
/* a name, into its place in order among those in rinc_helpOut (once) */
static void rinc_helpAdd(const char *name, int n) {
  char *p = rinc_helpOut, *e = rinc_helpOut + rinc_helpUsed, held[48];
  if (!rinc_helpNameOk(name, n) || !p) return;
  memcpy(held, name, (size_t)n); held[n] = 0;
  while (p < e) { /* (each name ends with 10) */
    char *end = memchr(p, 10, (size_t)(e - p)), was[48]; int k = (int)(end - p), c;
    memcpy(was, p, (size_t)(k < 47 ? k : 47)); was[k < 47 ? k : 47] = 0;
    if (!(c = strcmp(was, held))) return;
    if (rinc_lowerCmp(was, held) > 0 || (!rinc_lowerCmp(was, held) && c > 0)) break;
    p = end + 1;
  }
  if (rinc_helpUsed + n + 1 > rinc_helpCap) return;
  memmove(p + n + 1, p, (size_t)(e - p));
  memcpy(p, name, (size_t)n); p[n] = 10;
  rinc_helpUsed += n + 1; rinc_helpCount++;
}
/* a folder of help texts to look through, in storage */
static void rinc_helpDir(const char *path) {
  if (rinc_helpDirCount < 16 && strlen(path) < sizeof rinc_helpDirs[0]) memcpy(rinc_helpDirs[rinc_helpDirCount++], path, strlen(path) + 1);
}
/* the names a folder's texts start with: /rinc/help/a/b is a/b, and /rinc/apps/chord/help/a is chord/a */
static void rinc_helpPrefix(const char *dir, char *out, size_t cap) {
  const char *h;
  if (!strncmp(dir, "/rinc/help", 10)) snprintf(out, cap, "%s", dir[10] == '/' ? dir + 11 : "");
  else if (!strncmp(dir, "/rinc/apps/", 11) && (h = strstr(dir + 11, "/help")))
    snprintf(out, cap, "%.*s%s", (int)(h - dir - 11), dir + 11, h + 5);
  else if (!strncmp(dir, "/rinc/projects/", 15) && (h = strstr(dir + 15, "/help"))) /* (a project's: its +, then /a) */
    snprintf(out, cap, "%.*s+%s", (int)(h - dir - 15), dir + 15, h + 5);
  else out[0] = 0;
}
static void rinc_helpFound(const char *found, int dir, void *ud) {
  char in[96], name[64], path[160]; int n;
  (void)ud;
  if (found[0] == '.') return;
  rinc_helpPrefix(rinc_helpDirs[rinc_helpDirNow], in, sizeof in);
  n = snprintf(name, sizeof name, "%s%s%s", in, *in ? "/" : "", found);
  if (n < 0 || n >= (int)sizeof name) return;
  if (dir) {
    snprintf(path, sizeof path, "%.95s/%.63s", rinc_helpDirs[rinc_helpDirNow], found);
    rinc_helpDir(path);
  } else if (n > 3 && !strcmp(name + n - 3, ".md")) rinc_helpAdd(name, n - 3);
}
static int rinc_findHelps(char *out, int cap) {
  int i, apps = rinc_findApps(); char path[96];
  rinc_helpOut = out; rinc_helpCap = cap; rinc_helpUsed = rinc_helpCount = rinc_helpDirCount = 0;
  rinc_helpDir("/rinc/help");
  for (i = 0; i <= apps; i++) { /* (each app's help folder, and each project's; and, last, home's, which the list leaves out) */
    const char *a = i < apps ? rinc_appNames[i] : rinc_appHome; int n = (int)strlen(a), project = n && a[n - 1] == rinc_PROJECT;
    if (!n) continue;
    snprintf(path, sizeof path, "/rinc/%s/%.*s/help", project ? "projects" : "apps", project ? n - 1 : n, a);
    rinc_helpDir(path);
  }
  for (rinc_helpDirNow = 0; rinc_fs && rinc_helpDirNow < rinc_helpDirCount; rinc_helpDirNow++) /* (storage: a folder at a time) */
    rinc_fs->list(rinc_helpDirs[rinc_helpDirNow], rinc_helpFound, 0);
  for (i = 0; i < rinc_bundleCount; i++) { /* (help/name.md, apps/app/help/name.md) */
    const char *q = rinc_entryPath(i), *h; int n = (int)strnlen(q, 96); char name[64];
    if (n < 4 || strcmp(q + n - 3, ".md")) continue;
    if (!strncmp(q, "help/", 5) && n > 8) rinc_helpAdd(q + 5, n - 8);
    else if (!strncmp(q, "apps/", 5) && (h = strstr(q + 5, "/help/")) && h - q - 5 <= 32 &&
             snprintf(name, sizeof name, "%.*s/%.*s", (int)(h - q - 5), q + 5, (int)(q + n - 3 - (h + 6)), h + 6) < (int)sizeof name)
      rinc_helpAdd(name, (int)strlen(name));
  }
  rinc_helpOut = 0;
  return rinc_helpCount;
}
static void rinc_hostHelps(rio_Vm *v, rio_Val *a) {
  rinc_Out o; int n = rinc_into(v, a, &o) ? rinc_findHelps((char *)o.end, (int)o.room) : 0;
  rinc_outAdd(v, &o, (uint32_t)rinc_helpUsed);
  a[0].i = n;
}
static void rinc_hostHelpRead(rio_Vm *v, rio_Val *a) {
  const char *s = (const char *)rio_sliceBytes(v, a), *slash; int n = (int)a[1].u, app = 0, k; char name[64], path[128];
  rinc_Out o; const rinc_File *f = 0;
  a[0].i = -1; /* (after the name's read: it was there) */
  if (!rinc_into(v, a + 2, &o)) return;
  if (!rinc_helpNameOk(s, n)) return;
  memcpy(name, s, (size_t)n); name[n] = 0;
  if ((slash = strchr(name, '/'))) app = (int)(slash - name); /* (app/rest: maybe an app's own) */
  if (app > 1 && name[app - 1] == rinc_PROJECT) { /* (a project's: in storage only) */
    snprintf(path, sizeof path, "/rinc/projects/%.*s/help/%s.md", app - 1, name, slash + 1);
    if (rinc_nameOk(name, app - 1, 0)) a[0].i = rinc_readOut(v, a + 2, path);
    return;
  }
  for (k = 0; k < 4 && !f; k++) { /* storage: the app's, then help's; the bundle: the same */
    if ((k == 0 || k == 2) && !(app && rinc_nameOk(name, app, 0))) continue;
    if (k == 0) snprintf(path, sizeof path, "/rinc/apps/%.*s/help/%s.md", app, name, slash + 1);
    else if (k == 1) snprintf(path, sizeof path, "/rinc/help/%s.md", name);
    else if (k == 2) snprintf(path, sizeof path, "apps/%.*s/help/%s.md", app, name, slash + 1);
    else snprintf(path, sizeof path, "help/%s.md", name);
    if (k < 2) {
      if (rinc_exists(path)) { a[0].i = rinc_readOut(v, a + 2, path); return; }
    } else if ((f = rinc_bundleFile(path))) {
      uint32_t k = f->len < o.room ? f->len : o.room;
      memcpy(o.end, f->data, k); rinc_outAdd(v, &o, k);
      a[0].i = (int32_t)f->len;
    }
  }
}
/* help(topic): the help app, at that help text (and #heading), after this frame; helpTopic(out) -> Int,
   for the help app: the topic it was opened at, taken (how long; 0 for none) */
static char rinc_helpTopicNow[96];
static void rinc_hostHelp(rio_Vm *v, rio_Val *a) {
  const char *s = (const char *)rio_sliceBytes(v, a); int n = (int)a[1].u;
  if (!s || n < 0 || n >= (int)sizeof rinc_helpTopicNow) return;
  memcpy(rinc_helpTopicNow, s, (size_t)n); rinc_helpTopicNow[n] = 0;
  snprintf(rinc_launchName, sizeof rinc_launchName, "help"); rinc_launch = 1;
}
static void rinc_hostHelpTopic(rio_Vm *v, rio_Val *a) {
  a[0].i = rinc_addOut(v, a, rinc_helpTopicNow, (int)strlen(rinc_helpTopicNow));
  rinc_helpTopicNow[0] = 0;
}
/* one app calling another (docs/spec/apps.md#calling-an-app): call(name, args) starts app name (after
   this frame) with args, which it reads with args(out) -> Int (how long: 0 for none), and remembers who
   called; done(result) goes back to the caller (or the app before, if there's none), this app left out
   of the recent apps, and the caller, starting again, reads result(out) -> Int (how long, or -1 for none;
   taken) */
static char rinc_callArgs[256], rinc_callResult[256], rinc_caller[rinc_REF], rinc_callee[rinc_REF];
static int rinc_callArgsLen, rinc_callResultLen = -1, rinc_calling;
static void rinc_hostCall(rio_Vm *v, rio_Val *a) {
  const char *s = (const char *)rio_sliceBytes(v, a), *t = (const char *)rio_sliceBytes(v, a + 2); int n = (int)a[1].u, m = (int)a[3].u;
  if (!s || n < 1 || n >= (int)sizeof rinc_launchName || m < 0 || m > (int)sizeof rinc_callArgs) return;
  memcpy(rinc_launchName, s, (size_t)n); rinc_launchName[n] = 0;
  if (m) memcpy(rinc_callArgs, t, (size_t)m);
  rinc_callArgsLen = m;
  rinc_calling = 1; rinc_launch = 1;
}
static void rinc_hostArgs(rio_Vm *v, rio_Val *a) { a[0].i = rinc_addOut(v, a, rinc_callArgs, rinc_callArgsLen); }
static void rinc_hostResult(rio_Vm *v, rio_Val *a) {
  int n = rinc_callResultLen;
  a[0].i = n < 0 ? -1 : rinc_addOut(v, a, rinc_callResult, n);
  rinc_callResultLen = -1;
}
static const char *rinc_recentBefore(void); /* (the recent apps: below) */
static void rinc_hostDone(rio_Vm *v, rio_Val *a) {
  const char *s = (const char *)rio_sliceBytes(v, a), *before = rinc_recentBefore(); int n = (int)a[1].u;
  if (n < 0 || n > (int)sizeof rinc_callResult) n = 0;
  if (rinc_caller[0]) {
    if (s && n) memcpy(rinc_callResult, s, (size_t)n);
    rinc_callResultLen = s ? n : 0;
    snprintf(rinc_launchName, sizeof rinc_launchName, "%s", rinc_caller);
  } else snprintf(rinc_launchName, sizeof rinc_launchName, "%s", before);
  rinc_launch = 1;
}
/* each app's start: a call's args and its caller kept for it; anything else starts without */
static void rinc_callStart(const char *name) {
  if (rinc_calling && !strcmp(name, rinc_launchName)) {
    snprintf(rinc_caller, sizeof rinc_caller, "%s", rinc_callee[0] ? rinc_callee : "home");
  } else if (strcmp(name, rinc_caller)) { rinc_caller[0] = 0; rinc_callArgsLen = 0; rinc_callResultLen = -1; } /* (the caller, starting again, keeps its result) */
  else { rinc_caller[0] = 0; rinc_callArgsLen = 0; }
  rinc_calling = 0;
}
static void rinc_hostLaunch(rio_Vm *v, rio_Val *a) {
  const char *s = (const char *)rio_sliceBytes(v, a); int n = (int)a[1].u;
  if (!s || n < 1 || n >= (int)sizeof rinc_launchName) return;
  memcpy(rinc_launchName, s, (size_t)n); rinc_launchName[n] = 0; rinc_launch = 1;
}
void rinc_textEsc(void) { rinc_typed.esc = 0; rinc_queue(&rinc_typed, 27); }
static rio_Vm rinc_nest; static int rinc_nestOn, rinc_nestTried; /* (below: Rio inside an app) */
static void rinc_hostKey(rio_Vm *v, rio_Val *a) { a[0].i = v == &rinc_nest ? rinc_dequeue(&rinc_nestTyped) : rinc_cmdWanted ? -1 : rinc_dequeue(&rinc_typed); } /* (the command list asked for: the rest is its) */
extern int rinc_pointerMode; /* (below) */
uint8_t rinc_keysHeld[32];
int rinc_keysKnown;
void rinc_keyHold(int k, int down) {
  if (k < 0) { memset(rinc_keysHeld, 0, sizeof rinc_keysHeld); return; }
  if (k >= 'A' && k <= 'Z') k += 32;
  if (k > 255) return;
  if (down) rinc_keysHeld[k >> 3] |= (uint8_t)(1 << (k & 7));
  else rinc_keysHeld[k >> 3] &= (uint8_t)~(1 << (k & 7));
}
/* pc.keyHeld: 1 while key k is held, 0 if not (nor while a system combination's held, or a command settles), -1: it can't tell */
static void rinc_hostKeyHeld(rio_Vm *v, rio_Val *a) {
  int k = a[0].i;
  (void)v;
  if (k >= 'A' && k <= 'Z') k += 32;
  a[0].i = !rinc_keysKnown ? -1 : k < 0 || k > 255 || rinc_cmdSettle || rinc_systemHeld() ? 0 : rinc_keysHeld[k >> 3] >> (k & 7) & 1;
}
static void rinc_hostModsHeld(rio_Vm *v, rio_Val *a) { /* (as key() adds them: 256 Shift, 512 Ctrl, 1024 Alt, a locked one too; -1: it can't tell) */
  int m = rinc_modsHeld, pad = rinc_pointerMode == 1 ? (rinc_axes[4] > 16000 ? 256 : 0) | (rinc_axes[5] > 16000 ? 512 : 0) : 0; /* (in a pointer app, a controller's LT is Shift, RT Ctrl) */
  (void)v;
  a[0].i = m < 0 && !pad ? -1 : (m > 0 ? (m & 1 ? 256 : 0) | (m & 2 ? 512 : 0) | (m & 4 ? 1024 : 0) : 0) | pad | rinc_locks;
}

/* apps' sources, for a console or an editor: appSource(name, out) -> Int copies app name's source (from
   storage, else the bundle) into out, as much as fits, and gives its whole length (-1: no such source);
   appSave(name, text) -> Int and appDelete(name) -> Int change storage: 0, or -1. And an app's own save
   space, /rinc/save/<app>: saveRead(file, out) -> Int (as appSource), saveWrite(file, data) -> Int and
   saveDelete(file) -> Int (0, or -1). Each reads its name first: a[0], the name's address, takes the
   result */
static int rinc_argName(rio_Vm *v, rio_Val *name, char *out, int dots) {
  const char *s = (const char *)rio_sliceBytes(v, name); int n = (int)name[1].u;
  if (!rinc_nameOk(s, n, dots)) return 0;
  memcpy(out, s, (size_t)n); out[n] = 0;
  return 1;
}
/* an app's name, or its project's (chord+), into out (rinc_REF) */
static int rinc_argRef(rio_Vm *v, rio_Val *name, char *out) {
  const char *s = (const char *)rio_sliceBytes(v, name); int n = (int)name[1].u;
  if (!s || !rinc_refOk(s, n)) return 0;
  memcpy(out, s, (size_t)n); out[n] = 0;
  return 1;
}
/* the save space: the app's, /rinc/save/<app>, or a project's own, in its folder (its first run copies
   the app's in: rinc_projectSaves), so work on it can't spoil the app's */
static int rinc_saveSpace(char *path, size_t cap) {
  return snprintf(path, cap, rinc_appProject ? "/rinc/projects/%s/save" : "/rinc/save/%s", rinc_appNow) < (int)cap;
}
/* an app's cache space, /rinc/cache/<app> (a project's: <app>+, its own): what it can make again, which
   may go at any time; Rinc keeps the app's log there (log, below). cacheRead, cacheWrite and cacheDelete
   are as saves are */
static int rinc_cacheSpace(char *path, size_t cap) {
  return snprintf(path, cap, "/rinc/cache/%s%s", rinc_appNow, rinc_appProject ? "+" : "") < (int)cap;
}
static int rinc_cachePathOf(rio_Vm *v, rio_Val *file, char *path, size_t cap) {
  char own[33], space[64];
  return rinc_argName(v, file, own, 1) && rinc_cacheSpace(space, sizeof space) && snprintf(path, cap, "%s/%s", space, own) < (int)cap;
}
static int rinc_savePath(rio_Vm *v, rio_Val *file, char *path, size_t cap) {
  char own[33], space[64];
  return rinc_argName(v, file, own, 1) && rinc_saveSpace(space, sizeof space) && snprintf(path, cap, "%s/%s", space, own) < (int)cap;
}
/* every file in folder from, copied to the same places in folder to (but not a top-level folder named
   skip): 0, or -1 if any couldn't be. Names gather in rinc_tree as each folder's listed */
/* a folder's entries, a run of them at a time (the room's shared by the folders inside it, as they're
   walked): 'd' or 'f' and each name, or 'x' for one whose name's too long to have a path. A walk skips
   the first skip it's given, and says if there were more than there was room for (full) */
static char rinc_tree[512]; static int rinc_treeUsed, rinc_treeCap = (int)sizeof rinc_tree; /* (cap: where this level's run stops, below) */
typedef struct { int skip, seen, full; } rinc_TreeWalk;
static void rinc_treeFound(const char *name, int dir, void *ud) {
  rinc_TreeWalk *w = (rinc_TreeWalk *)ud; size_t n = strlen(name);
  if (w && (w->seen++ < w->skip || w->full)) return; /* (once one doesn't fit, none after it: a run is unbroken, so the next run starts right after it) */
  if (n > 95) n = 0, dir = 'x'; /* (no path for it: counted, and it can't be done) */
  if (rinc_treeUsed + (int)n + 2 > rinc_treeCap) { if (w) w->full = 1; return; }
  rinc_tree[rinc_treeUsed++] = (char)(dir == 'x' ? 'x' : dir ? 'd' : 'f');
  memcpy(rinc_tree + rinc_treeUsed, name, n); rinc_tree[rinc_treeUsed + (int)n] = 0; rinc_treeUsed += (int)n + 1;
}
#define rinc_TREE_DEEP 12
/* a level's room: half of what's left, so the folders in it have the rest (but one long name, at least) */
static void rinc_treeRoom(int start) {
  int left = (int)sizeof rinc_tree - start, half = left / 2;
  rinc_treeCap = start + (half > 98 ? half : left < 98 ? left : 98);
}
/* the paths being walked, each level's /name added on, and taken off after: here, not on the stack, which
   on a board is small (4 KB, the interpreter's calls too), so a folder 12 deep is no danger */
static char rinc_treeFrom[128], rinc_treeTo[128];
static int rinc_treeAdd(char *path, int len, const char *name) { /* (path/name: its length, or -1 if it's too long) */
  int n = (int)strlen(name);
  if (len + 1 + n >= 128) return -1;
  path[len] = '/'; memcpy(path + len + 1, name, (size_t)n + 1);
  return len + 1 + n;
}
static int rinc_copyAt(int fromLen, int toLen, const char *skip, int depth) {
  int start = rinc_treeUsed, end, at, bad = 0, done = 0; rinc_TreeWalk w;
  if (!rinc_fs || depth > rinc_TREE_DEEP) return -1;
  for (;;) { /* (a run of its entries at a time: those before done, skipped) */
    w.skip = done; w.seen = w.full = 0; rinc_treeUsed = start;
    rinc_treeRoom(start);
    rinc_treeFrom[fromLen] = 0;
    if (rinc_fs->list(rinc_treeFrom, rinc_treeFound, &w)) { rinc_treeUsed = start; rinc_treeCap = (int)sizeof rinc_tree; return -1; }
    end = rinc_treeUsed;
    if (end == start) { bad |= w.full; break; } /* (none, or not even one fits) */
    for (at = start; at < end; at += (int)strlen(rinc_tree + at + 1) + 2, done++) {
      const char *name = rinc_tree + at + 1;
      int f = rinc_tree[at] == 'x' ? -1 : rinc_treeAdd(rinc_treeFrom, fromLen, name), t = f < 0 ? -1 : rinc_treeAdd(rinc_treeTo, toLen, name);
      if (f < 0 || t < 0) bad = 1;
      else if (rinc_tree[at] == 'd') { if (!(skip && !strcmp(name, skip)) && rinc_copyAt(f, t, 0, depth + 1)) bad = 1; }
      else if (rinc_fs->copy(rinc_treeFrom, rinc_treeTo)) bad = 1;
      rinc_treeFrom[fromLen] = rinc_treeTo[toLen] = 0;
      rinc_treeUsed = end;
    }
    if (!w.full) break;
  }
  rinc_treeUsed = start; rinc_treeCap = (int)sizeof rinc_tree;
  return bad ? -1 : 0;
}
static int rinc_copyTree(const char *from, const char *to, const char *skip, int depth) {
  int f = snprintf(rinc_treeFrom, sizeof rinc_treeFrom, "%s", from), t = snprintf(rinc_treeTo, sizeof rinc_treeTo, "%s", to);
  return f < 0 || t < 0 || f >= (int)sizeof rinc_treeFrom || t >= (int)sizeof rinc_treeTo ? -1 : rinc_copyAt(f, t, skip, depth);
}
/* every file and folder in folder dir, and it: 0, or -1 if any couldn't be removed. What's removed is
   gone from the next run's listing, so each run skips just what couldn't be */
static int rinc_removeAt(int len, int depth) {
  int start = rinc_treeUsed, end, at, kept = 0, r; rinc_TreeWalk w;
  if (!rinc_fs || depth > rinc_TREE_DEEP) return -1;
  for (;;) {
    w.skip = kept; w.seen = w.full = 0; rinc_treeUsed = start;
    rinc_treeRoom(start);
    rinc_treeFrom[len] = 0;
    if (rinc_fs->list(rinc_treeFrom, rinc_treeFound, &w)) break;
    end = rinc_treeUsed;
    if (end == start) { kept += w.full; break; }
    for (at = start; at < end; at += (int)strlen(rinc_tree + at + 1) + 2) {
      int f = rinc_tree[at] == 'x' ? -1 : rinc_treeAdd(rinc_treeFrom, len, rinc_tree + at + 1);
      if (f < 0) { kept++; continue; }
      r = rinc_tree[at] == 'd' ? rinc_removeAt(f, depth + 1) : rinc_fs->remove(rinc_treeFrom);
      rinc_treeFrom[len] = 0;
      if (r) kept++;
      rinc_treeUsed = end;
    }
    if (!w.full) break;
  }
  rinc_treeUsed = start; rinc_treeCap = (int)sizeof rinc_tree;
  rinc_treeFrom[len] = 0;
  return rinc_fs->remove(rinc_treeFrom) || kept ? -1 : 0;
}
static int rinc_removeTree(const char *dir, int depth) {
  int n = snprintf(rinc_treeFrom, sizeof rinc_treeFrom, "%s", dir);
  return n < 0 || n >= (int)sizeof rinc_treeFrom ? -1 : rinc_removeAt(n, depth);
}
/* an app's folder, or its project's (chord+): 0 if the name's no good */
static int rinc_appFolder(const char *name, char *out, size_t cap) {
  int n = (int)strlen(name), project = n > 1 && name[n - 1] == rinc_PROJECT;
  if (!rinc_refOk(name, n)) return 0;
  return snprintf(out, cap, "/rinc/%s/%.*s", project ? "projects" : "apps", project ? n - 1 : n, name) < (int)cap;
}
void rinc_cacheDrop(const char *name);
/* app or project name gone: its folder, all of it (its own saves, for a project), and its compiled
   image; the active one no more, if it was. 0, or -1 if there's no such app */
static int rinc_appRemove(const char *name) {
  char path[96], folder[64], active[rinc_REF]; uint32_t total;
  if (!rinc_fs || !rinc_appPath(name, path, sizeof path) || !rinc_appFolder(name, folder, sizeof folder) || rinc_readFile(path, 0, 0, &total)) return -1;
  rinc_removeTree(folder, 0);
  rinc_cacheDrop(name);
  if (!rinc_readFile("/rinc/settings/active", active, sizeof active - 1, &total) && total < sizeof active && (active[total] = 0, !strcmp(active, name)))
    rinc_fs->remove("/rinc/settings/active");
  return 0;
}
/* appCopy(from, to) -> Int: app or project from, copied as new project to (a + after it): its
   files, its main one renamed (its own saves aside). appRename(from, to) -> Int: project from as new
   project to. Each 0, or -1 (to there already, say) */
static void rinc_hostAppCopy(rio_Vm *v, rio_Val *a) {
  char from[rinc_REF], to[rinc_REF], fromDir[64], toDir[64], path[96], main[96]; uint32_t total; int n, k;
  int named = rinc_argRef(v, a, from) && rinc_argRef(v, a + 2, to);
  a[0].i = -1;
  if (!named || !rinc_fs || (n = (int)strlen(to)) < 2 || to[n - 1] != rinc_PROJECT || !rinc_appFolder(from, fromDir, sizeof fromDir) || !rinc_appFolder(to, toDir, sizeof toDir)) return;
  if (!rinc_appPath(to, path, sizeof path) || !rinc_readFile(path, 0, 0, &total) || !rinc_fs->list(toDir, rinc_treeFound, 0)) { rinc_treeUsed = 0; return; } /* (there already) */
  rinc_treeUsed = 0;
  if (rinc_copyTree(fromDir, toDir, "save", 0) && rinc_readApp(to, 0, 0) < 0) { /* (an app in the bundle: its source, from there) */
    const rinc_App *b = rinc_bundled(from);
    if (!b || b->image || rinc_fs->open(path, 1)) return;
    k = rinc_fs->write(b->data, (int)b->len) != (int)b->len;
    if (rinc_fs->close() || k) return;
    a[0].i = 0;
    return;
  }
  k = (int)strlen(from) - (from[strlen(from) - 1] == rinc_PROJECT);
  snprintf(main, sizeof main, "%s/%.*s.rio", toDir, k, from); /* (its main file, by its old name: renamed) */
  a[0].i = rinc_fs->rename(main, path) && rinc_readApp(to, 0, 0) < 0 ? -1 : 0;
}
static void rinc_hostAppRename(rio_Vm *v, rio_Val *a) {
  char from[rinc_REF], to[rinc_REF], fromDir[64], toDir[64], path[96], main[96], active[rinc_REF]; uint32_t total; int n, m;
  int named = rinc_argRef(v, a, from) && rinc_argRef(v, a + 2, to);
  a[0].i = -1;
  if (!named || !rinc_fs || (n = (int)strlen(from)) < 2 || from[n - 1] != rinc_PROJECT || (m = (int)strlen(to)) < 2 || to[m - 1] != rinc_PROJECT ||
      !rinc_appFolder(from, fromDir, sizeof fromDir) || !rinc_appFolder(to, toDir, sizeof toDir) || !rinc_appPath(to, path, sizeof path)) return;
  if (!rinc_fs->list(toDir, rinc_treeFound, 0)) { rinc_treeUsed = 0; return; } /* (there already) */
  rinc_treeUsed = 0;
  if (rinc_fs->rename(fromDir, toDir)) return;
  snprintf(main, sizeof main, "%s/%.*s.rio", toDir, n - 1, from);
  rinc_fs->rename(main, path);
  rinc_cacheDrop(from);
  if (!rinc_readFile("/rinc/settings/active", active, sizeof active - 1, &total) && total < sizeof active && (active[total] = 0, !strcmp(active, from)) &&
      !rinc_fs->open("/rinc/settings/active", 1)) { rinc_fs->write(to, m); rinc_fs->close(); } /* (the active one: by its new name) */
  a[0].i = 0;
}
/* files by path, for a file browser (docs/spec/apps.md#files): a path is absolute, /rinc/save/chord/best,
   with no . or .. parts. Looking (dir, fileRead) goes anywhere in /rinc, the root (for now); changing
   (fileWrite, fileDelete, fileCopy's to, fileRename) only inside it, never /rinc itself. dir(path, out) -> Int: its entries into out, folders first, then by name, each a line,
   "d name" or "f name", as many as fit: how many, or -1 if it's no folder. fileRead(path, out) -> Int: as
   much as fits, its whole length, or -1; fileWrite(path, data) -> Int; fileDelete(path) -> Int (a folder:
   all of it); fileCopy(from, to) -> Int (a folder: all of it) and fileRename(from, to) -> Int, to a
   name not taken: each 0, or -1 */
static void rinc_writeFile(rio_Vm *v, rio_Val *a, const char *path); /* (below) */
static int rinc_argPath(rio_Vm *v, rio_Val *a, char *out, int change) {
  const char *s = (const char *)rio_sliceBytes(v, a); int n = (int)a[1].u, i;
  char own[96];
  if (s && n >= 1 && s[0] != '/') { /* (one of the app's own files, by its path in its folder: its project's, chord+, or as installed) */
    int k = snprintf(own, sizeof own, "/rinc/%s/%s/%.*s", rinc_appProject ? "projects" : "apps", rinc_appNow, n, s);
    if (change || k < 0 || k >= (int)sizeof own) return 0; /* (read only: an installed app's folder isn't for writing) */
    s = own;
    n = k;
  }
  if (!s || n < 1 || n > 95 || s[0] != '/' || (n > 1 && s[n - 1] == '/')) return 0;
  for (i = 0; i < n; i++) {
    char c = s[i];
    if (c < 32 || c > 126 || c == '\\' || c == ':') return 0;
    if (c == '/' && i + 1 < n && (s[i + 1] == '/' || (s[i + 1] == '.' && (i + 2 == n || s[i + 2] == '/' || (s[i + 2] == '.' && (i + 3 == n || s[i + 3] == '/')))))) return 0;
  }
  if (change ? n <= 6 || strncmp(s, "/rinc/", 6) : strncmp(s, "/rinc", 5) || (n > 5 && s[5] != '/')) return 0; /* (/rinc is the root: looking goes only there for now) */
  memcpy(out, s, (size_t)n); out[n] = 0;
  return 1;
}
static int rinc_exists(const char *path) {
  if (!rinc_fs) return 0;
  if (!rinc_fs->open(path, 0)) { rinc_fs->close(); return 1; }
  if (!rinc_fs->list(path, rinc_treeFound, 0)) { rinc_treeUsed = 0; return 1; }
  rinc_treeUsed = 0;
  return 0;
}
static char *rinc_dirOut; static int rinc_dirCap, rinc_dirUsed, rinc_dirCount;
static void rinc_dirFound(const char *name, int dir, void *ud) { /* (into its place: folders, then names, ignoring case) */
  char *p = rinc_dirOut, *e = rinc_dirOut + rinc_dirUsed; int n = (int)strlen(name);
  (void)ud;
  if (!p || n > 60 || name[0] == 0 || name[0] == '.') return; /* (a name starting with a dot: hidden, a web folder's .folder say) */
  while (p < e) {
    char *end = memchr(p, 10, (size_t)(e - p)), was[64]; int k = (int)(end - p) - 2;
    memcpy(was, p + 2, (size_t)k); was[k] = 0;
    if ((p[0] == 'f' && dir) || ((p[0] == 'd') == (dir != 0) && rinc_lowerCmp(was, name) > 0)) break;
    p = end + 1;
  }
  if (rinc_dirUsed + n + 3 > rinc_dirCap) return;
  memmove(p + n + 3, p, (size_t)(e - p));
  p[0] = dir ? 'd' : 'f'; p[1] = ' '; memcpy(p + 2, name, (size_t)n); p[n + 2] = 10;
  rinc_dirUsed += n + 3; rinc_dirCount++;
}
/* whether the entries found so far have this one */
static int rinc_dirHas(const char *name, int dir) {
  char *p = rinc_dirOut, *e = rinc_dirOut + rinc_dirUsed; size_t n = strlen(name);
  while (p && p < e) {
    char *end = memchr(p, 10, (size_t)(e - p));
    if ((p[0] == 'd') == (dir != 0) && (size_t)(end - p) == n + 2 && !memcmp(p + 2, name, n)) return 1;
    p = end + 1;
  }
  return 0;
}
/* a folder's entries in the bundle (path: /rinc/...), each once, after storage's: whether it has any */
static int rinc_dirBundled(const char *path) {
  const char *rel = !strcmp(path, "/rinc") ? "" : !strncmp(path, "/rinc/", 6) ? path + 6 : 0;
  size_t n; int i, any = 0;
  if (!rel) return 0;
  for (n = strlen(rel); n && rel[n - 1] == '/'; n--) {}
  for (i = 0; i < rinc_bundleCount; i++) {
    const char *e = rinc_entryPath(i), *rest, *slash; char name[64]; size_t k, len = strnlen(e, 96);
    if (n && (len <= n || strncmp(e, rel, n) || e[n] != '/')) continue;
    rest = e + (n ? n + 1 : 0);
    slash = memchr(rest, '/', len - (size_t)(rest - e));
    k = slash ? (size_t)(slash - rest) : len - (size_t)(rest - e);
    if (!k || k > 60) continue;
    memcpy(name, rest, k); name[k] = 0;
    any = 1;
    if (!rinc_dirHas(name, slash != 0)) rinc_dirFound(name, slash != 0, 0);
  }
  return any;
}
static void rinc_hostDir(rio_Vm *v, rio_Val *a) { /* (storage's entries, then the bundle's) */
  char path[96]; int named = rinc_argPath(v, a, path, 0), listed, bundled;
  rinc_Out o;
  if (!rinc_into(v, a + 2, &o)) { a[0].i = -1; return; }
  rinc_dirOut = (char *)o.end; rinc_dirCap = (int)o.room; rinc_dirUsed = rinc_dirCount = 0;
  listed = named && rinc_fs && !rinc_fs->list(path, rinc_dirFound, 0);
  bundled = named && rinc_dirBundled(path);
  a[0].i = listed || bundled ? rinc_dirCount : -1;
  rinc_outAdd(v, &o, (uint32_t)rinc_dirUsed);
  rinc_dirOut = 0;
}
static void rinc_hostFileRead(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_argPath(v, a, path, 0);
  a[0].i = named ? rinc_readOut(v, a + 2, path) : -1;
}
static void rinc_hostFileWrite(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_argPath(v, a, path, 1);
  if (named) rinc_writeFile(v, a, path); else a[0].i = -1;
}
/* fileReadAt(path, at, out) -> Int: from byte at on, added to out, as much as fits: its whole length,
   or -1; fileWriteAt(path, at, data) -> Int: data over the file from byte at on (-1: at its end), the
   file made if it isn't there: where it went, or -1 (at past its end, say) */
static void rinc_readAtOut(rio_Vm *v, rio_Val *a, int named, const char *path) { /* (a[2] at, a + 3 out) */
  rinc_Out o; uint32_t got;
  if (!named || a[2].i < 0 || !rinc_into(v, a + 3, &o)) { a[0].i = -1; return; }
  a[0].i = rinc_fileFrom(path, (uint32_t)a[2].i, (char *)o.end, o.room, &got);
  rinc_outAdd(v, &o, got);
}
static void rinc_hostFileReadAt(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_argPath(v, a, path, 0);
  rinc_readAtOut(v, a, named, path);
}
static void rinc_hostFileWriteAt(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_argPath(v, a, path, 1);
  a[0].i = named ? rinc_fileOver(path, a[2].i, (const char *)rio_sliceBytes(v, a + 3), (int)a[4].u) : -1;
}
/* pictures in files (png.c; only PNGs so far): imageLoad(path, pixels) -> [2]Int (pc.imageRead's): the
   picture's pixels, a byte each, row by row, added to pixels; its width and height, or {0, 0} (pixels needs
   room past its length for the PNG's rows as they come: (w + 1) * h for one of the palette's; without it,
   just its size, and a line in the log saying so).
   imageWrite(path, image, o: ImageWrite) -> Int: one, as an indexed PNG (o.format fromName: the path ends
   .png), with the app's notes, o.notes (text; "": none): 0, or -1. imageNotes(path, out) -> Int: those
   notes, added to out: how many bytes, or -1 */
static void rinc_hostImageLoad(rio_Vm *v, rio_Val *a) {
  char path[96], say[160]; rinc_Out o; uint32_t size[2] = {0, 0}, need = 0;
  int named = rinc_argPath(v, a, path, 0), r = named && rinc_into(v, a + 2, &o) ? rinc_pngRead(path, o.end, o.room, size, &need) : -1;
  a[0].i = (int32_t)size[0]; a[1].i = (int32_t)size[1];
  if (r == -2) {
    snprintf(say, sizeof say, "imageRead: %.96s needs %u bytes of room, and has %u\r\n", path, (unsigned)need, (unsigned)o.room);
    rinc_say(say);
  }
  if (!r) rinc_outAdd(v, &o, size[0] * size[1]);
}
static void rinc_hostImageWrite(rio_Vm *v, rio_Val *a) {
  char path[96]; pc_Image im; pc_ImageWrite o; const uint8_t *p; int w, h, named = rinc_argPath(v, a, path, 1);
  size_t k = strlen(path);
  memset(&im, 0, sizeof im); memset(&o, 0, sizeof o);
  rio_getStruct(v, a + 2, &im, sizeof im);
  rio_getStruct(v, a + 4, &o, sizeof o);
  p = (const uint8_t *)rio_viewBytes(v, im.pixels); w = im.size[0]; h = im.size[1];
  a[0].i = -1;
  if (!named || !p || w <= 0 || h <= 0 || (uint64_t)w * (uint32_t)h > im.pixels.len || o.notes.len > rinc_PNG_NOTES) return;
  if (!o.format && !(k > 4 && path[k - 4] == '.' && (path[k - 3] | 32) == 'p' && (path[k - 2] | 32) == 'n' && (path[k - 1] | 32) == 'g')) return; /* (fromName) */
  a[0].i = rinc_pngWrite(path, p, w, h, (const char *)rio_viewBytes(v, o.notes), (int)o.notes.len);
}
static void rinc_hostImageNotes(rio_Vm *v, rio_Val *a) {
  char path[96]; rinc_Out o; int named = rinc_argPath(v, a, path, 0), n = named && rinc_into(v, a + 2, &o) ? rinc_pngNotes(path, (char *)o.end, o.room) : -1;
  if (n > 0) rinc_outAdd(v, &o, (uint32_t)n < o.room ? (uint32_t)n : o.room);
  a[0].i = n;
}
static void rinc_hostFileDelete(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_argPath(v, a, path, 1);
  a[0].i = -1;
  if (!named || !rinc_fs || !rinc_exists(path)) return;
  a[0].i = rinc_fs->remove(path) && rinc_removeTree(path, 0) ? -1 : 0;
}
static void rinc_hostFileCopy(rio_Vm *v, rio_Val *a) {
  char from[96], to[96]; int named = rinc_argPath(v, a, from, 0) && rinc_argPath(v, a + 2, to, 1);
  a[0].i = -1;
  if (!named || !rinc_fs || rinc_exists(to) || (!strncmp(to, from, strlen(from)) && (to[strlen(from)] == '/' || !to[strlen(from)]))) return; /* (not into itself) */
  if (!rinc_fs->list(from, rinc_treeFound, 0)) { rinc_treeUsed = 0; a[0].i = rinc_copyTree(from, to, 0, 0); return; } /* (a folder: everything in it; a file can't tell, everywhere) */
  rinc_treeUsed = 0;
  a[0].i = rinc_fs->copy(from, to) ? -1 : 0;
}
static void rinc_hostDirMake(rio_Vm *v, rio_Val *a) { /* dirMake(path) -> Int: a folder, at a path not taken: 0, or -1 */
  char path[96]; int named = rinc_argPath(v, a, path, 1);
  a[0].i = named && rinc_fs && !rinc_exists(path) && !rinc_fs->mkdir(path) ? 0 : -1;
}
static void rinc_hostFileRename(rio_Vm *v, rio_Val *a) {
  char from[96], to[96]; int named = rinc_argPath(v, a, from, 1) && rinc_argPath(v, a + 2, to, 1);
  int into = named && !strncmp(to, from, strlen(from)) && to[strlen(from)] == '/'; /* (not into itself) */
  a[0].i = named && !into && rinc_fs && !rinc_exists(to) && rinc_exists(from) && !rinc_fs->rename(from, to) ? 0 : -1;
}
/* a project's own save space, the first time it runs: the app's saves, copied in */
static void rinc_projectSaves(void) {
  char space[64], app[64];
  if (!rinc_appProject || !rinc_fs || !rinc_saveSpace(space, sizeof space) || !rinc_fs->list(space, rinc_treeFound, 0)) { rinc_treeUsed = 0; return; }
  rinc_treeUsed = 0;
  snprintf(app, sizeof app, "/rinc/save/%s", rinc_appNow);
  rinc_copyTree(app, space, 0, 0); /* (none: nothing to copy) */
}
/* appInstall(name) -> Int: project name+ installed as the app name: its files copied over the app's
   (its own save space aside): 0, or -1 */
static void rinc_hostAppInstall(rio_Vm *v, rio_Val *a) {
  char name[33], from[64], to[64];
  int named = rinc_argName(v, a, name, 0);
  a[0].i = -1;
  if (!named || !rinc_fs) return;
  snprintf(from, sizeof from, "/rinc/projects/%s", name);
  snprintf(to, sizeof to, "/rinc/apps/%s", name);
  a[0].i = rinc_copyTree(from, to, "save", 0);
}
static void rinc_hostAppSource(rio_Vm *v, rio_Val *a) {
  char name[rinc_REF], path[96]; const rinc_App *b;
  int named = rinc_argRef(v, a, name);
  a[0].i = -1;
  if (!named) return;
  if (rinc_appPath(name, path, sizeof path) && rinc_exists(path)) a[0].i = rinc_readOut(v, a + 2, path);
  else if ((b = rinc_bundled(name)) && !b->image) {
    uint32_t total = rinc_appLen(b);
    rinc_addOut(v, a + 2, (const char *)b->data, (int)total);
    a[0].i = (int32_t)total;
  }
}
static void rinc_writeFile(rio_Vm *v, rio_Val *a, const char *path) {
  const char *text = (const char *)rio_sliceBytes(v, a + 2); int n = (int)a[3].u, bad;
  a[0].i = -1;
  if (!rinc_fs || rinc_fs->open(path, 1)) return;
  bad = n > 0 && (!text || rinc_fs->write(text, n) != n);
  a[0].i = rinc_fs->close() || bad ? -1 : 0;
}
static void rinc_hostAppSave(rio_Vm *v, rio_Val *a) {
  char name[rinc_REF], path[96]; int named = rinc_argRef(v, a, name) && rinc_appPath(name, path, sizeof path);
  if (named) rinc_writeFile(v, a, path); else a[0].i = -1;
}
static int rinc_appRemove(const char *name);
static void rinc_hostAppDelete(rio_Vm *v, rio_Val *a) { /* (all of it: rinc_appRemove) */
  char name[rinc_REF]; int named = rinc_argRef(v, a, name);
  a[0].i = named ? rinc_appRemove(name) : -1;
}
static void rinc_hostSaveRead(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_savePath(v, a, path, sizeof path);
  a[0].i = named ? rinc_readOut(v, a + 2, path) : -1;
}
static void rinc_hostSaveWrite(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_savePath(v, a, path, sizeof path);
  if (named) rinc_writeFile(v, a, path); else a[0].i = -1;
}
static void rinc_hostSaveDelete(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_savePath(v, a, path, sizeof path);
  a[0].i = named && rinc_fs && !rinc_fs->remove(path) ? 0 : -1;
}
static void rinc_hostCacheRead(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_cachePathOf(v, a, path, sizeof path);
  a[0].i = named ? rinc_readOut(v, a + 2, path) : -1;
}
static void rinc_hostCacheWrite(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_cachePathOf(v, a, path, sizeof path);
  if (named) rinc_writeFile(v, a, path); else a[0].i = -1;
}
static void rinc_hostCacheDelete(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_cachePathOf(v, a, path, sizeof path);
  a[0].i = named && rinc_fs && !rinc_fs->remove(path) ? 0 : -1;
}
/* an app's stash, /rinc/stash/<app>/<key>: what it keeps of where it was, for its next start (a place,
   a draft, something slow to make again), under keys of its own, written as it goes. Rinc drops it all,
   where its save space only the app changes (Sys+Q, Sys+R). stashWrite(key, data) -> Int: 0, or -1;
   stashRead(key, out) -> Int: as much as fits, and its whole length, or -1 if there's none;
   stashDelete(key) -> Int: 0, or -1. Keys are named as save files are */
static int rinc_stashPath(rio_Vm *v, rio_Val *key, char *path, size_t cap) {
  char own[33];
  return rinc_argName(v, key, own, 1) && snprintf(path, cap, "/rinc/stash/%s/%s", rinc_appNow, own) < (int)cap;
}
static char rinc_stashGone[96];
static void rinc_stashFound(const char *name, int dir, void *ud) {
  char path[160];
  (void)ud;
  if (dir) return;
  snprintf(path, sizeof path, "%s/%.63s", rinc_stashGone, name);
  rinc_fs->remove(path);
}
void rinc_stashDrop(const char *app) {
  if (!rinc_fs) return;
  snprintf(rinc_stashGone, sizeof rinc_stashGone, "/rinc/stash/%.32s", app);
  rinc_fs->list(rinc_stashGone, rinc_stashFound, 0);
  rinc_fs->remove(rinc_stashGone); /* (the folder, empty now) */
}
static void rinc_hostStashWrite(rio_Vm *v, rio_Val *a) {
  char path[96], folder[64]; int named = rinc_stashPath(v, a, path, sizeof path);
  rio_Val keep[4];
  if (!named) { a[0].i = -1; return; }
  memcpy(keep, a, sizeof keep); /* (a[0] takes the result: for trying again) */
  rinc_writeFile(v, a, path);
  if (a[0].i < 0 && rinc_fs) { /* (a stash from before keys, a file where its folder goes: dropped, and again) */
    snprintf(folder, sizeof folder, "/rinc/stash/%.32s", rinc_appNow);
    rinc_fs->remove(folder);
    memcpy(a, keep, sizeof keep);
    rinc_writeFile(v, a, path);
  }
}
static void rinc_hostStashRead(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_stashPath(v, a, path, sizeof path);
  a[0].i = named ? rinc_readOut(v, a + 2, path) : -1;
}
/* stashReadAt(key, at, out) -> Int and stashWriteAt(key, at, data) -> Int: as fileReadAt and
   fileWriteAt, in the stash */
static void rinc_hostStashReadAt(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_stashPath(v, a, path, sizeof path);
  rinc_readAtOut(v, a, named, path);
}
static void rinc_hostStashWriteAt(rio_Vm *v, rio_Val *a) {
  char path[96], folder[64]; int named = rinc_stashPath(v, a, path, sizeof path), n = (int)a[4].u; int32_t at = a[2].i;
  const char *data = (const char *)rio_sliceBytes(v, a + 3);
  if (!named) { a[0].i = -1; return; }
  if ((a[0].i = rinc_fileOver(path, at, data, n)) < 0 && rinc_fs) { /* (a stash from before keys, a file where its folder goes: dropped, and again) */
    snprintf(folder, sizeof folder, "/rinc/stash/%.32s", rinc_appNow);
    if (!rinc_fs->open(folder, rinc_READ)) { rinc_fs->close(); rinc_fs->remove(folder); a[0].i = rinc_fileOver(path, at, data, n); }
  }
}
static void rinc_hostStashDelete(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_stashPath(v, a, path, sizeof path);
  a[0].i = named && rinc_fs && !rinc_fs->remove(path) ? 0 : -1;
}
/* the settings, /rinc/settings/<key>: the whole Rinc's, not an app's, shared by every app (such as
   "active": the app or project being worked on, chord or chord+). settingRead(key, out) -> Int: as much
   as fits, and its whole length, or -1 if there's none; settingWrite(key, data) -> Int: 0, or -1. Keys
   are named as save files are */
/* a setting's file: its key a name, or names with / between (files/appreg/png), each as a save file's is */
static int rinc_settingPath(rio_Vm *v, rio_Val *key, char *path, size_t cap) {
  const char *s = (const char *)rio_sliceBytes(v, key); int n = (int)key[1].u, at = 0, i;
  if (!s || n < 1 || n > 48) return 0;
  for (i = 0; i <= n; i++)
    if (i == n || s[i] == '/') {
      if (!rinc_nameOk(s + at, i - at, 1)) return 0;
      at = i + 1;
    }
  return snprintf(path, cap, "/rinc/settings/%.*s", n, s) < (int)cap;
}
/* appsFor(ext, out) -> Int: the apps that open files ending .ext, as their opens.txt says (a line each:
   the extension, then what such a file is), into out a line each, the app's name and what, in order of
   name: how many. The apps' own folders say, so installing one is all that registers it */
#define rinc_opens rinc_tree /* (an opens.txt, read: into the folder walks' room, which no walk is using now) */
static void rinc_hostAppsFor(rio_Vm *v, rio_Val *a) {
  const char *ext = (const char *)rio_sliceBytes(v, a); int n = (int)a[1].u, i, found = 0; rinc_Out o;
  a[0].i = 0;
  if (!ext || n < 1 || n > 16 || !rinc_into(v, a + 2, &o)) return;
  rinc_findApps();
  for (i = 0; i < rinc_appCount; i++) {
    char path[112]; uint32_t total; const char *p, *end;
    if (strchr(rinc_appNames[i], rinc_PROJECT)) continue; /* (installed apps: a project opens as its app does, once it's installed) */
    snprintf(path, sizeof path, "/rinc/apps/%s/opens.txt", rinc_appNames[i]);
    if (rinc_readFile(path, rinc_opens, sizeof rinc_opens - 1, &total)) continue;
    rinc_opens[total < sizeof rinc_opens - 1 ? total : sizeof rinc_opens - 1] = 0;
    for (p = rinc_opens; *p; p = *end ? end + 1 : end) {
      const char *w = p, *what; int k;
      end = strchr(p, '\n'); if (!end) end = p + strlen(p);
      while (w < end && (*w == ' ' || *w == '.')) w++;
      for (k = 0; k < n && w + k < end && tolower((unsigned char)w[k]) == tolower((unsigned char)ext[k]); k++) {}
      if (k < n || (w + k < end && w[k] != ' ' && w[k] != '\t' && w[k] != '\r')) continue;
      for (what = w + k; what < end && (*what == ' ' || *what == '\t'); what++) {}
      { char line[96]; int len = snprintf(line, sizeof line, "%s %.*s\n", rinc_appNames[i], (int)(end - what - (end > what && end[-1] == '\r')), what);
        if (len > 0 && len < (int)sizeof line && (uint32_t)len <= o.room) { memcpy(o.end, line, (size_t)len); rinc_outAdd(v, &o, (uint32_t)len); } } /* (outAdd moves on past it) */
      found++;
      break;
    }
  }
  a[0].i = found;
}
static void rinc_hostSettingRead(rio_Vm *v, rio_Val *a) {
  char path[64]; int named = rinc_settingPath(v, a, path, sizeof path);
  a[0].i = named ? rinc_readOut(v, a + 2, path) : -1;
}
static void rinc_hostSettingWrite(rio_Vm *v, rio_Val *a) {
  char path[64]; int named = rinc_settingPath(v, a, path, sizeof path);
  if (named) rinc_writeFile(v, a, path); else a[0].i = -1;
}
/* the theme: a color for each role the system apps draw with (pc.Role, in this order), so they look
   alike and a theme changes them all. theme(role) -> Int. The theme setting names one, owner/id: a
   file of an app's, owner/themes/id.theme.rio, in its folder (or the bundle's) or its save space, or a
   project's (owner+); owner/id+ is only the save space's, your copy of a locked one (Rinc's own are
   rinc/slate and so on, in /rinc/apps/rinc/themes). The file is Rio's form of constants, and only that is read, never run: lines of
   name :: number (a star after the name, or not), # comments, and the first line its title. Roles it
   leaves out, or with no theme, are Rinc's own colors, these */
static const char *const rinc_roleNames[] = {"bg", "bar", "text", "soft", "dim", "faint", "accent", "heading", "pick", "select", "link",
                                             "error", "ok", "codeBg", "code", "punct", "keyword", "type", "call", "number", "string", "comment"};
#define rinc_ROLES ((int)(sizeof rinc_roleNames / sizeof rinc_roleNames[0]))
static const uint8_t rinc_roleDefaults[rinc_ROLES] = {1, 0, 7, 6, 13, 5, 10, 9, 12, 2, 12, 8, 11, 17, 7, 6, 14, 15, 12, 9, 11, 13};
static uint8_t rinc_theme[rinc_ROLES]; static int rinc_themeOn; /* (loaded once: before, the defaults) */
/* a theme's file, read into the roles: 0, or -1 (no such file) */
static int rinc_themeRead(const char *owner, int n, const char *id, int m, int copy) {
  static char text[1024]; char path[112]; uint32_t len = 0; int project = owner[n - 1] == rinc_PROJECT, i, found = 0; /* (a theme file is about 640 bytes) */
  const rinc_File *f;
  if (copy && project) return -1;
  snprintf(path, sizeof path, "/rinc/%s/%.*s/themes/%.*s.theme.rio", project ? "projects" : "apps", project ? n - 1 : n, owner, m, id);
  if (!copy && !rinc_readFile(path, text, sizeof text - 1, &len)) found = 1;
  if (!found && !project) {
    snprintf(path, sizeof path, "/rinc/save/%.*s/themes/%.*s.theme.rio", n, owner, m, id);
    if (!rinc_readFile(path, text, sizeof text - 1, &len)) found = 1;
  }
  if (!found && !project && !copy) {
    snprintf(path, sizeof path, "apps/%.*s/themes/%.*s.theme.rio", n, owner, m, id);
    if ((f = rinc_bundleFile(path))) { len = f->len < sizeof text - 1 ? f->len : sizeof text - 1; memcpy(text, f->data, len); found = 1; }
  }
  if (!found) return -1;
  if (len > sizeof text - 1) len = sizeof text - 1;
  text[len] = 0;
  for (i = 0; text[i]; ) { /* (a line: name, a star or not, ::, a number) */
    int a, b, r, v = 0, digits = 0;
    while (text[i] == ' ' || text[i] == '\t') i++;
    a = i;
    while ((text[i] >= 'a' && text[i] <= 'z') || (text[i] >= 'A' && text[i] <= 'Z') || (text[i] >= '0' && text[i] <= '9') || text[i] == '_') i++;
    b = i;
    if (text[i] == '*') i++;
    while (text[i] == ' ' || text[i] == '\t') i++;
    if (b > a && text[i] == ':' && text[i + 1] == ':') {
      i += 2;
      while (text[i] == ' ' || text[i] == '\t') i++;
      while (text[i] >= '0' && text[i] <= '9' && digits < 4) { v = v * 10 + text[i++] - '0'; digits++; }
      for (r = 0; digits && v < 256 && r < rinc_ROLES; r++)
        if ((int)strlen(rinc_roleNames[r]) == b - a && !memcmp(rinc_roleNames[r], text + a, (size_t)(b - a))) rinc_theme[r] = (uint8_t)v;
    }
    while (text[i] && text[i] != '\n') i++;
    if (text[i]) i++;
  }
  return 0;
}
/* the theme the setting names (each app's start: a change shows in the next app) */
void rinc_themeLoad(void) {
  char name[80]; uint32_t len = 0; const char *slash; int n, copy;
  memcpy(rinc_theme, rinc_roleDefaults, sizeof rinc_theme);
  rinc_themeOn = 1;
  rinc_textFg = rinc_theme[2]; /* (the screen console's text) */
  if (rinc_readFile("/rinc/settings/theme", name, sizeof name - 1, &len) || len >= sizeof name) return;
  while (len && (name[len - 1] == '\n' || name[len - 1] == '\r' || name[len - 1] == ' ')) len--;
  name[len] = 0;
  copy = len > 1 && name[len - 1] == '+'; /* (rinc/slate+: your copy of a locked theme, in the save space) */
  if (copy) name[--len] = 0;
  if (!(slash = strchr(name, '/')) || !rinc_refOk(name, n = (int)(slash - name)) || !rinc_nameOk(slash + 1, (int)strlen(slash + 1), 0)) return;
  rinc_themeRead(name, n, slash + 1, (int)strlen(slash + 1), copy);
  rinc_textFg = rinc_theme[2];
}
static void rinc_hostTheme(rio_Vm *v, rio_Val *a) { (void)v; a[0].i = a[0].u < (uint32_t)rinc_ROLES ? rinc_theme[a[0].i] : rinc_theme[2]; }
int rinc_themeColor(int role) { return role < 0 || role >= rinc_ROLES ? rinc_roleDefaults[2] : rinc_themeOn ? rinc_theme[role] : rinc_roleDefaults[role]; }

/* boot() -> Int: how many times this Rinc has started (kept in /rinc/boot), counted the first time an
   app asks after each start: so an app can tell a restart from just being opened again (0: no storage) */
static int rinc_bootNow = -1;
static void rinc_hostBoot(rio_Vm *v, rio_Val *a) {
  (void)v;
  if (rinc_bootNow < 0) {
    char text[16]; uint32_t total; int k;
    rinc_bootNow = 0;
    if (rinc_fs) {
      if (!rinc_readFile("/rinc/boot", text, sizeof text - 1, &total) && total < sizeof text) {
        text[total] = 0;
        for (k = 0; text[k] >= '0' && text[k] <= '9'; k++) rinc_bootNow = rinc_bootNow * 10 + text[k] - '0';
      }
      rinc_bootNow++;
      k = snprintf(text, sizeof text, "%d", rinc_bootNow);
      if (rinc_fs->open("/rinc/boot", 1) || rinc_fs->write(text, k) != k || rinc_fs->close()) rinc_bootNow = 0;
    }
  }
  a[0].i = rinc_bootNow;
}
/* Rio inside an app (docs/spec/rio.md): one session at a time, a REPL in memory the app hands over.
   rioStart(mem, tables) -> Int: a fresh one, its code and data in mem and its compiler's tables in
   tables (0, or -1); rioEval(text) -> Int: compile and run a piece (0 done, 1 unfinished: it needs more,
   -1 an error); rioFrame() -> Int: its update then draw (0, 1 if it has neither, -1 an error);
   rioType(key): a key for its pc.key(); rioError(out) -> Int and rioLog(out) -> Int: the last error, and
   what it has logged since (taken), as text; rioIndent(text) -> Int: the indent steps an unfinished
   input's next line takes. The session has pc too, but not a session of its own */
static char rinc_nestLog[2048]; static int rinc_nestLogLen;
static void rinc_nestLogLine(void *ud, const char *s, int n) {
  (void)ud;
  if (n > (int)sizeof rinc_nestLog - 1 - rinc_nestLogLen) n = (int)sizeof rinc_nestLog - 1 - rinc_nestLogLen; /* (full: the rest is lost) */
  if (n < 0) return;
  memcpy(rinc_nestLog + rinc_nestLogLen, s, (size_t)n); rinc_nestLogLen += n;
  rinc_nestLog[rinc_nestLogLen++] = '\n';
}
#ifdef rio_NO_COMPILER /* (a vm that only runs images, as the web player's: no sessions, nothing to compile) */
static int rinc_noCompiler(void) { return -1; }
#define rio_replBegin(vm, scratch, size, lim) rinc_noCompiler()
#define rio_replEval(vm, src, len) rinc_noCompiler()
#define rio_replIndent(src, len) 0
#define rio_compile(vm, src, len) rinc_noCompiler()
#endif
static int rinc_loader(void *ud, const char *path, int kind, rio_Source *out); /* (pc.rio: below) */
static void rinc_hostRioStart(rio_Vm *v, rio_Val *a) {
  uint8_t *mem = (uint8_t *)rio_sliceBytes(v, a), *tables = (uint8_t *)rio_sliceBytes(v, a + 2);
  a[0].i = -1;
  rinc_nestOn = rinc_nestLogLen = 0; rinc_nestTried = 1;
  memset(&rinc_nestTyped, 0, sizeof rinc_nestTyped);
  if (v == &rinc_nest || !mem || !tables || rio_init(&rinc_nest, mem, a[1].u, 0, 0)) return;
  rio_setLog(&rinc_nest, rinc_nestLogLine, 0);
  rio_setLoader(&rinc_nest, rinc_loader, 0); /* (pc as in an app: its types too) */
  if (rinc_hostProcs(&rinc_nest) || rio_autoImport(&rinc_nest, "pc") || rio_replBegin(&rinc_nest, tables, a[3].u, 0)) return;
  rinc_nestOn = 1; a[0].i = 0;
}
static void rinc_hostRioEval(rio_Vm *v, rio_Val *a) {
  const char *s = (const char *)rio_sliceBytes(v, a);
  a[0].i = v != &rinc_nest && rinc_nestOn && s ? rio_replEval(&rinc_nest, s, a[1].u) : -1;
}
static void rinc_hostRioFrame(rio_Vm *v, rio_Val *a) { a[0].i = v != &rinc_nest && rinc_nestOn ? rinc_frame(&rinc_nest) : -1; }
static void rinc_hostRioType(rio_Vm *v, rio_Val *a) { if (v != &rinc_nest && a[0].i > 0 && a[0].i < 2048) rinc_queue(&rinc_nestTyped, a[0].i); }
static void rinc_hostRioError(rio_Vm *v, rio_Val *a) {
  const char *e = rinc_nestOn || rinc_nestTried ? rio_error(&rinc_nest) : "no session (pc.rioStart starts one)";
  a[0].i = v == &rinc_nest ? 0 : rinc_addOut(v, a, e, (int)strlen(e));
}
static void rinc_hostRioLog(rio_Vm *v, rio_Val *a) {
  rinc_Out o; int n = v == &rinc_nest || !rinc_into(v, a, &o) ? 0 : rinc_nestLogLen < (int)o.room ? rinc_nestLogLen : (int)o.room;
  memcpy(o.end, rinc_nestLog, (size_t)n); if (n) rinc_outAdd(v, &o, (uint32_t)n);
  memmove(rinc_nestLog, rinc_nestLog + n, (size_t)(rinc_nestLogLen - n)); rinc_nestLogLen -= n;
  a[0].i = n;
}
static void rinc_hostRioIndent(rio_Vm *v, rio_Val *a) { const char *s = (const char *)rio_sliceBytes(v, a); a[0].i = s ? rio_replIndent(s, a[1].u) : 0; }
static void rinc_hostBtn(rio_Vm *v, rio_Val *a) { (void)v; a[0].i = a[0].u < 16 && !rinc_cmdSettle ? rinc_buttons >> a[0].i & 1 : 0; }
static void rinc_hostShowCommands(rio_Vm *v, rio_Val *a) { (void)a; if (v != &rinc_nest) rinc_cmdWanted = 1; }
/* stick(i) -> Float: 0 the left stick's x, 1 its y, 2 and 3 the right one's; -1 to 1, + right and down,
   0 when it's near the middle (it gets there on its own only roughly). trigger(i) -> Float: 0 left, 1
   right; 0 to 1 */
#define rinc_DEAD 0.15f
static float rinc_stickAt(int i) {
  float v = (float)rinc_axes[i] / 32767.0f, m = v < 0 ? -v : v;
  if (m < rinc_DEAD) return 0;
  m = (m - rinc_DEAD) / (1 - rinc_DEAD);
  return v < 0 ? -(m > 1 ? 1 : m) : (m > 1 ? 1 : m);
}
static void rinc_hostStick(rio_Vm *v, rio_Val *a) { (void)v; a[0].f = a[0].u < 4 ? rinc_stickAt(a[0].i) : 0; }
static void rinc_hostTrigger(rio_Vm *v, rio_Val *a) { (void)v; a[0].f = a[0].u < 2 ? (float)rinc_axes[4 + a[0].i] / 32767.0f : 0; }
static void rinc_hostTime(rio_Vm *v, rio_Val *a) { (void)v; a[0].f = rinc_time ? rinc_time() : 0; }
/* fromHex(out, text): bytes from hex digits (pairs; anything else between them, such as spaces and
   line breaks, is skipped) added to out, until it's full. Returns how many. For binary data typed into an
   app: sound, pictures */
static int rinc_hexDigit(int c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1; }
static void rinc_hostFromhex(rio_Vm *v, rio_Val *a) {
  rinc_Out o; uint8_t *out = rinc_into(v, a, &o); const char *t = (const char *)rio_sliceBytes(v, a + 3);
  uint32_t cap = o.room, n = a[4].u, i, k = 0; int hi = -1;
  for (i = 0; out && t && i < n && k < cap; i++) {
    int d = rinc_hexDigit(t[i]);
    if (d < 0) continue;
    if (hi < 0) hi = d; else { out[k++] = (uint8_t)(hi << 4 | d); hi = -1; }
  }
  if (k) rinc_outAdd(v, &o, k);
  a[0].u = k;
}
/* moduleSource(name, out): pc's or core's source, added to out, as much as fits, and its whole length (-1: neither) */
static void rinc_hostModulesource(rio_Vm *v, rio_Val *a) {
  const char *name = (const char *)rio_sliceBytes(v, a), *s = 0;
  uint32_t n = 0;
  if (name && a[1].u == 2 && !memcmp(name, "pc", 2)) s = pc_SOURCE, n = sizeof pc_SOURCE - 1;
  if (name && a[1].u == 4 && !memcmp(name, "core", 4)) s = core_SOURCE, n = sizeof core_SOURCE - 1;
  a[0].i = s ? rinc_addOut(v, a + 2, s, (int)n) : -1;
}
/* formatRio(out, code): rio_format's (rio fmt's), added to out (nothing, unless it all fits); -2 (left as
   it is) in a build without the compiler, which has no rio_format */
static void rinc_hostFormatrio(rio_Vm *v, rio_Val *a) {
#ifdef rio_NO_COMPILER
  (void)v;
  a[0].i = -2;
#else
  rinc_Out o; const char *code = (const char *)rio_sliceBytes(v, a + 3);
  a[0].i = rinc_into(v, a, &o) && o.room ? rio_format(code ? code : "", code ? a[4].u : 0, (char *)o.end, o.room) : -1;
  if (a[0].i > 0) rinc_outAdd(v, &o, (uint32_t)a[0].i);
#endif
}

int rinc_game, rinc_showLog;
/* what an app logs, and what Rinc says about it: to the platform, and on the screen while rinc_showLog */
/* each app's log: what it logs, and what Rinc says about it, kept in its cache space as log, the newest
   rinc_LOG bytes (whole lines). Held here as it comes, and written at most a second apart while it does
   (rinc_logTick, each frame), and when the app stops or another starts (rinc_logStart): the log app shows
   it (Sys+O) */
#define rinc_LOG 4096
static char rinc_log[rinc_LOG], rinc_logApp[40];
static int rinc_logLen, rinc_logDirty;
static float rinc_logAt;
static void rinc_readerDrop(void); /* (below: a source file being read, let go of, as storage has one file open at a time) */
static void rinc_logFlush(void) {
  char path[64];
  if (!rinc_logDirty || !rinc_logApp[0] || !rinc_fs || snprintf(path, sizeof path, "/rinc/cache/%s/log", rinc_logApp) >= (int)sizeof path) return;
  rinc_logDirty = 0;
  rinc_readerDrop();
  if (rinc_fs->open(path, 1)) return;
  rinc_fs->write(rinc_log, rinc_logLen);
  rinc_fs->close();
}
static void rinc_logAdd(const char *s, int n) {
  int i;
  if (!rinc_logApp[0]) return;
  for (i = 0; i < n; i++) {
    if (s[i] == '\r') continue;
    if (rinc_logLen == rinc_LOG) { /* (full: the oldest quarter goes, to a line's end) */
      int cut = rinc_LOG / 4;
      while (cut < rinc_LOG && rinc_log[cut - 1] != '\n') cut++;
      memmove(rinc_log, rinc_log + cut, (size_t)(rinc_LOG - cut));
      rinc_logLen -= cut;
    }
    rinc_log[rinc_logLen++] = s[i];
  }
  rinc_logDirty = 1;
}
void rinc_logSave(void) { rinc_logFlush(); }
static void rinc_logTick(void) {
  float t = rinc_time ? rinc_time() : 0;
  if (rinc_logDirty && t - rinc_logAt >= 1) { rinc_logFlush(); rinc_logAt = t; }
}
static void rinc_logStart(const char *app) { /* (the last app's written; this one's read back, then a line saying it's started) */
  char path[64]; uint32_t total = 0;
  rinc_logFlush();
  snprintf(rinc_logApp, sizeof rinc_logApp, "%s", app);
  rinc_logLen = 0;
  if (snprintf(path, sizeof path, "/rinc/cache/%s/log", app) < (int)sizeof path && !rinc_readFile(path, rinc_log, rinc_LOG, &total))
    rinc_logLen = total < rinc_LOG ? (int)total : rinc_LOG;
  rinc_logAdd("-- started\n", 11);
}
static void rinc_out(const char *s, int n) { rinc_put(s, n); if (rinc_showLog) rinc_consolePut(s, n); rinc_logAdd(s, n); }
void rinc_say(const char *s) { rinc_out(s, (int)strlen(s)); }
void rinc_noHome(char *out, int size, const char *how) { /* (wrapped between words, for the screen's width) */
  char text[400]; const char *s = text; int n = 0, col = 0;
  snprintf(text, sizeof text, "No apps here yet: no home app (/rinc/apps/%s).\n\n%s\n\n", rinc_appFor("home"), how);
  while (*s && n < size - 3) {
    int word = 0;
    if (*s == '\n') { out[n++] = '\r'; out[n++] = '\n'; col = 0; s++; continue; }
    while (s[word] && s[word] != ' ' && s[word] != '\n') word++;
    if (col && col + word > rinc_W / 6 - 1) { out[n++] = '\r'; out[n++] = '\n'; col = 0; }
    for (; word && n < size - 3; word--, col++) out[n++] = *s++;
    if (*s == ' ') { if (col) { out[n++] = ' '; col++; } s++; }
  }
  out[n] = 0;
}
int rinc_hasHome(void) { return rinc_readApp(rinc_appFor("home"), 0, 0) != -1 || rinc_bundled(rinc_appFor("home")); }
void rinc_noHomeShow(void (*put)(const char *s, int n), const char *how) {
  char text[600];
  rinc_game = 0; rinc_showLog = 1; rinc_put = put;
  rinc_initPalette(); /* (with no app started yet, it's all black) */
  rinc_themeLoad();
  rinc_consoleClear(rinc_themeColor(0));
  rinc_noHome(text, sizeof text, how);
  rinc_say(text);
}
static void rinc_logLine(void *ud, const char *s, int n) { (void)ud; rinc_out(s, n); rinc_out("\r\n", 2); }
/* the way home: on the screen (it's for whoever's looking at it), not in what's printed */
static void rinc_back(void) { static const char s[] = "\r\n(Sys+Esc: back home)\r\n"; if (rinc_showLog) rinc_consolePut(s, (int)sizeof s - 1); }

/* the modules and files Rinc has for apps (docs/spec/apps.md#more-files): (pc is pc.rio, registered with
   the host procs: rinc_hostProcs) an app's own files, in its folder, for include "parts/a.rio"
   and import .shapes; and the library's, /rinc/lib, for import tween. Storage's first, else the bundle's
   (apps/<app>/..., lib/...). Those from storage (and the host's) are read a part at a time, as the
   compiler wants them, and each from storage is noted, for the app's cached image */
static char rinc_loadApp[rinc_REF]; /* (the app compiling: its folder) */
static int rinc_srcOn; /* (compiling an app: its other files can be read) */
#define rinc_SRC_FILES 16
/* a file read a part at a time: from storage (one file open at a time: this one, opened again whenever
   another was), or the host's (host: path in rinc_hostFolder); hashed as it's read, in order, for the
   cached image (to its end: ended) */
typedef struct { char path[100]; uint8_t host, ended; uint32_t hashed; uint64_t x; } rinc_Reader;
static rinc_Reader rinc_srcFiles[rinc_SRC_FILES + 1], *rinc_readerOpen; /* (the last: the main file) */
static void rinc_readerDrop(void) { if (rinc_readerOpen) { rinc_fs->close(); rinc_readerOpen = 0; } } /* (it opens again where it was, as it's next read) */
static int rinc_srcCount, rinc_srcLost; /* (lost: more than were noted) */
#ifdef rio_NO_COMPILER
static int rinc_loader(void *ud, const char *path, int kind, rio_Source *out) { (void)ud; (void)path; (void)kind; (void)out; return 1; } /* (images only) */
#else
int (*rinc_hostRead)(const char *path, char *buf, uint32_t at, uint32_t n);
char rinc_hostFolder[512];
static int rinc_readAt(void *ud, char *buf, uint32_t at, uint32_t n) {
  rinc_Reader *r = (rinc_Reader *)ud; int got = 0, k;
  if (r->host) {
    static char full[640]; /* (static, as are the loader's: off the stack, small on a board, which an app's check also loads from: pc.checkRio) */
    snprintf(full, sizeof full, "%s/%s", rinc_hostFolder, r->path);
    if (!rinc_hostRead || (got = rinc_hostRead(full, buf, at, n)) < 0) return -1;
  } else {
    if (rinc_readerOpen != r) {
      rinc_readerOpen = 0;
      if (!rinc_fs || rinc_fs->open(r->path, 0)) return -1;
      rinc_readerOpen = r;
    }
    if (rinc_fs->seek(at)) return -1;
    for (; (uint32_t)got < n; got += k) {
      if ((k = rinc_fs->read(buf + got, (int)(n - (uint32_t)got))) < 0) return -1;
      if (!k) break;
    }
  }
  if (at <= r->hashed && at + (uint32_t)got > r->hashed) { /* (what's new: hashed, FNV-1a, as rinc_hash) */
    uint32_t i;
    for (i = r->hashed - at; i < (uint32_t)got; i++) r->x = (r->x ^ (uint8_t)buf[i]) * 0x100000001b3ull;
    r->hashed = at + (uint32_t)got;
  }
  if (!got && at == r->hashed) r->ended = 1;
  return got;
}
/* a reader, set going on path (from storage, else host's): whether the file's there */
static int rinc_readerStart(rinc_Reader *r, const char *path, int host) {
  char probe;
  if (strlen(path) >= sizeof r->path) return 0;
  memset(r, 0, sizeof *r);
  memcpy(r->path, path, strlen(path) + 1); /* (it fits: just checked) */
  r->host = (uint8_t)host;
  r->x = 0xcbf29ce484222325ull;
  if (rinc_readAt(r, &probe, 0, 0) < 0) return 0; /* (there: open, to be read) */
  return 1;
}
/* a file from storage (or, host, the host's), read a part at a time: 0, or 1 if it isn't there, or 2 if
   there are too many */
static int rinc_loadFile(const char *path, rio_Source *out, int host) {
  rinc_Reader *r = &rinc_srcFiles[rinc_srcCount];
  if (rinc_srcCount == rinc_SRC_FILES) return 2;
  if (!rinc_readerStart(r, path, host)) return 1;
  rinc_srcCount++;
  out->src = 0; out->len = 0; out->read = rinc_readAt; out->readData = r;
  out->name = host ? r->path : r->path + 6;
  return 0;
}
static int rinc_loader(void *ud, const char *path, int kind, rio_Source *out) {
  static char base[64], bundleBase[64], tryPath[104], name[640]; const char *last; const rinc_File *f; int n = (int)strlen(rinc_loadApp), project, k, res;
  int host = !(kind & rio_LoadKind_package) && rinc_hostFolder[0]; /* (an app run from the host's files: its own, from there) */
  (void)ud;
  if (!rinc_srcOn || strstr(path, "..") || strlen(path) > 48) return 1;
  project = n > 1 && rinc_loadApp[n - 1] == rinc_PROJECT;
  if (kind & rio_LoadKind_package) { snprintf(base, sizeof base, "/rinc/lib/"); snprintf(bundleBase, sizeof bundleBase, "lib/"); }
  else if (host) base[0] = bundleBase[0] = 0;
  else if (n) {
    snprintf(base, sizeof base, "/rinc/%s/%.*s/", project ? "projects" : "apps", project ? n - 1 : n, rinc_loadApp);
    if (project) bundleBase[0] = 0; /* (a project's own files: storage's only) */
    else snprintf(bundleBase, sizeof bundleBase, "apps/%s/", rinc_loadApp);
  } else return 1;
  last = strrchr(path, '/') ? strrchr(path, '/') + 1 : path;
  for (k = 0; k < 2; k++) { /* (a file: that path; a module: path.rio, else its folder's own, path/<last>.rio) */
    if (kind & rio_LoadKind_file) { if (k) break; snprintf(tryPath, sizeof tryPath, "%s", path); }
    else if (!k) snprintf(tryPath, sizeof tryPath, "%s.rio", path);
    else snprintf(tryPath, sizeof tryPath, "%s/%s.rio", path, last);
    snprintf(name, sizeof name, "%s%s", base, tryPath);
    if (!(res = rinc_loadFile(name, out, host))) { out->isDir = k; return 0; }
    if (res == 2) return 1; /* (too many files: as if it isn't there) */
    if (host) continue;
    snprintf(name, sizeof name, "%s%s", bundleBase, tryPath);
    if (*bundleBase && (f = rinc_bundleFile(name))) { out->src = f->data; out->len = f->len; out->isDir = k; out->name = f->path; return 0; }
    if (kind & rio_LoadKind_package) { /* (and the library built in: a card's own, or the bundle's, first) */
      int j;
      for (j = 0; rinc_library[j].path; j++)
        if (!strcmp(rinc_library[j].path, tryPath)) { out->src = rinc_library[j].data; out->len = rinc_library[j].len; out->isDir = k; out->name = rinc_library[j].path; return 0; }
    }
  }
  return 1;
}
#endif

int rinc_start(rio_Vm *vm, void *mem, uint32_t memsize, void (*put)(const char *s, int n)) {
  rinc_put = put;
  rinc_initPalette();
  rinc_themeLoad(); /* (each start: a change of theme shows in the next app) */
  if (rio_init(vm, mem, memsize, 0, 0)) return -1; /* no code buffer: the code goes in mem too */
  rinc_audioReset();
  rio_setLog(vm, rinc_logLine, 0);
  rio_setLoader(vm, rinc_loader, 0);
  if (rinc_hostProcs(vm) || rio_autoImport(vm, "pc")) return -1; /* (pc.cls and so on, everywhere) */
  if (!rinc_calling) memset(&rinc_typed, 0, sizeof rinc_typed); /* (typed ahead for an app called: its) */
  rinc_nestOn = rinc_nestTried = 0; /* (a session lives in an app's memory, which is starting over) */
  return 0;
}

/* an app's source compiled (as rinc.h says): in memory (src), or read a part at a time from path (as are
   its other files) */
static int rinc_compileSource(rio_Vm *vm, const char *src, uint32_t len, const char *path, int host) {
  int res = -1;
#ifdef rio_NO_COMPILER
  (void)vm; (void)src; (void)len; (void)path; (void)host;
#else
  rinc_Reader *top = &rinc_srcFiles[rinc_SRC_FILES]; /* (the main file) */
  rinc_srcCount = rinc_srcLost = 0;
  rinc_srcOn = 1;
  rio_setSourceWindow(vm, 2048); /* (every app's compile alike: the window, whether its main file's read so or not) */
  if (!path) res = rio_compile(vm, src, len) ? -1 : 0;
  else {
    if (!rinc_readerStart(top, path, host)) top->path[0] = 0, top->host = 0; /* (not there: Rio says it can't read it) */
    res = rio_compileStream(vm, rinc_readAt, top) ? -1 : 0;
  }
  rinc_srcOn = 0;
  if (rinc_readerOpen) { rinc_fs->close(); rinc_readerOpen = 0; }
#endif
  return res;
}
int rinc_compile(rio_Vm *vm, void *space, uint32_t size, const char *src, uint32_t len) {
  (void)space; (void)size; /* (the app space: rinc_start's already) */
  return rinc_compileSource(vm, src, len, 0, 0);
}
int rinc_compileFile(rio_Vm *vm, void *space, uint32_t size, const char *path, int host) {
  (void)space; (void)size;
  return rinc_compileSource(vm, 0, 0, path, host);
}

/* An editor's completion (pc.checkRio, pc.completeRio): its app's main file checked (rio_check) in the
   session's vm, up to the file being edited, whose text (cut at the cursor's line) the editor gives;
   then the names that fit there. The tables stay in the app's memory (room), lent: hashed after the
   check, so a room the app has written over since is seen, and not read */
static int rinc_cplOn; static const uint8_t *rinc_checkRoom; static uint32_t rinc_checkSize, rinc_checkHash;
static const char *rinc_checkPath, *rinc_checkCode; static uint32_t rinc_checkLen;
static uint32_t rinc_roomHash(const uint8_t *p, uint32_t n) { uint32_t h = 2166136261u; while (n--) h = (h ^ *p++) * 16777619u; return h; }
#ifndef rio_NO_COMPILER
static int rinc_checkLoader(void *ud, const char *path, int kind, rio_Source *out) { /* (the file being edited: the editor's text) */
  int res = rinc_loader(ud, path, kind, out);
  const char *name = out->name;
  if (res || !name) return res;
  if (out->read == rinc_readAt) name = ((rinc_Reader *)out->readData)->path; /* (storage's: /rinc/apps/...) */
  if (!strcmp(name, rinc_checkPath) || (!strncmp(rinc_checkPath, "/rinc/", 6) && !strcmp(name, rinc_checkPath + 6))) {
    out->src = rinc_checkCode; out->len = rinc_checkLen; out->read = 0; out->readData = 0;
  }
  return 0;
}
#endif
/* the check itself: main compiled, from storage, unless it's the file being edited (the editor's text). Deep:
   the compiler, and under it the loader, the SD card's reader and its file system, from inside an app's
   call, so the board runs it on a stack of its own (rinc_onBigStack) */
void (*rinc_onBigStack)(void (*fn)(void *), void *arg);
static struct { const char *main; uint8_t *room; uint32_t size; int result; } rinc_checking;
static struct { char path[128], message[rio_ERROR_BUF_SIZE]; int line, col, len; } rinc_checkError; /* (the last check's) */
static void rinc_runCheck(void *arg) {
  (void)arg;
  rinc_checking.result = -1;
#ifndef rio_NO_COMPILER
  if (!strcmp(rinc_checking.main, rinc_checkPath))
    rinc_checking.result = rio_check(&rinc_nest, rinc_checkCode, rinc_checkLen, 0, 0, rinc_checking.room, rinc_checking.size);
  else {
    rinc_Reader *top = &rinc_srcFiles[rinc_SRC_FILES];
    if (!rinc_readerStart(top, rinc_checking.main, 0)) top->path[0] = 0;
    rinc_checking.result = rio_check(&rinc_nest, 0, 0, rinc_readAt, top, rinc_checking.room, rinc_checking.size);
  }
#endif
}
static void rinc_hostCheckrio(rio_Vm *v, rio_Val *a) {
  const char *main = (const char *)rio_sliceBytes(v, a), *path = (const char *)rio_sliceBytes(v, a + 2), *code = (const char *)rio_sliceBytes(v, a + 4);
  uint8_t *room = (uint8_t *)rio_sliceBytes(v, a + 6);
  uint32_t size = a[7].u, constRoom = size / 16 < 4096 ? 4096 : size / 16; /* (the constants: ~1000 in 4 KB) */
  static char mainPath[100], editPath[100]; /* (off the stack: see rinc_readAt) */
  char host;
  a[0].i = -1;
  rinc_cplOn = 0;
  if (v == &rinc_nest || !main || !path || !room || a[1].u >= sizeof mainPath || a[3].u >= sizeof editPath || size < constRoom + 4096) return;
#ifdef rio_NO_COMPILER
  (void)code;
#else
  memcpy(mainPath, main, a[1].u); mainPath[a[1].u] = 0;
  memcpy(editPath, path, a[3].u); editPath[a[3].u] = 0;
  rinc_nestOn = 0; rinc_nestTried = 1; /* (the session's vm: a session there is over) */
  if (rio_init(&rinc_nest, room, constRoom, 0, 0)) return;
  rio_setLoader(&rinc_nest, rinc_checkLoader, 0);
  if (rinc_hostProcs(&rinc_nest) || rio_autoImport(&rinc_nest, "pc")) return;
  rinc_checkPath = editPath; rinc_checkCode = code ? code : ""; rinc_checkLen = code ? a[5].u : 0;
  { /* (its app: the folder its own files come from, /rinc/apps/name/... or /rinc/projects/name/..., as name+) */
    const char *p = mainPath, *e; int project = !strncmp(p, "/rinc/projects/", 15);
    rinc_loadApp[0] = 0;
    if (project || !strncmp(p, "/rinc/apps/", 11)) {
      p += project ? 15 : 11;
      if ((e = strchr(p, '/')) && e - p < (int)sizeof rinc_loadApp - 1) snprintf(rinc_loadApp, sizeof rinc_loadApp, "%.*s%s", (int)(e - p), p, project ? "+" : "");
    }
  }
  rinc_srcCount = rinc_srcLost = 0;
  rinc_srcOn = 1;
  host = rinc_hostFolder[0];
  rinc_hostFolder[0] = 0; /* (storage's files, even for an app run from the host's) */
  rio_setSourceWindow(&rinc_nest, 2048);
  rinc_checking.main = mainPath; rinc_checking.room = room + constRoom; rinc_checking.size = size - constRoom;
  if (rinc_onBigStack) rinc_onBigStack(rinc_runCheck, 0); /* (the board's: a stack of its own) */
  else rinc_runCheck(0);
  a[0].i = rinc_checking.result;
  { /* (its error, kept for pc.checkError: in which file, /rinc/... or main's own) */
    rio_Error e = rio_errorInfo(&rinc_nest);
    rinc_checkError.line = a[0].i < 0 && e.kind == rio_ErrorKind_compile ? e.line : 0;
    rinc_checkError.col = e.col; rinc_checkError.len = e.len;
    if (!e.file) snprintf(rinc_checkError.path, sizeof rinc_checkError.path, "%s", mainPath);
    else snprintf(rinc_checkError.path, sizeof rinc_checkError.path, "%s%s", e.file[0] == '/' ? "" : "/rinc/", e.file);
    snprintf(rinc_checkError.message, sizeof rinc_checkError.message, "%s", e.msg ? e.msg : "");
  }
  rinc_srcOn = 0;
  rinc_hostFolder[0] = host;
  if (rinc_readerOpen) { rinc_fs->close(); rinc_readerOpen = 0; }
  rinc_checkPath = rinc_checkCode = 0;
  if (!rinc_nest.compiler) return;
  rinc_cplOn = 1; rinc_checkRoom = room; rinc_checkSize = size; rinc_checkHash = rinc_roomHash(room, size);
#endif
}
/* checkError(path, message) -> pc.AppError: the last check's error (line 0: none), its file and what it
   says, added to path and message */
static void rinc_hostCheckerror(rio_Vm *v, rio_Val *a) {
  pc_AppError r;
  memset(&r, 0, sizeof r);
  if (rinc_checkError.line > 0) {
    r.line = rinc_checkError.line; r.col = rinc_checkError.col; r.len = rinc_checkError.len;
    rinc_addOut(v, a + 2, rinc_checkError.path, (int)strlen(rinc_checkError.path));
    rinc_addOut(v, a + 5, rinc_checkError.message, (int)strlen(rinc_checkError.message));
  }
  rio_putStruct(v, a, &r, sizeof r);
}
typedef struct { char *out; uint32_t cap, n; } rinc_Offers;
static void rinc_offer(void *ud, const char *name, int nameLen, int kind, const char *detail, int detailLen) {
  rinc_Offers *o = (rinc_Offers *)ud;
  if (o->n + (uint32_t)nameLen + (uint32_t)detailLen + 4 > o->cap) return;
  memcpy(o->out + o->n, name, (size_t)nameLen); o->n += (uint32_t)nameLen;
  o->out[o->n++] = '\t'; o->out[o->n++] = "kvctpmfMe"[kind < 9 ? kind : 0]; o->out[o->n++] = '\t';
  memcpy(o->out + o->n, detail, (size_t)detailLen); o->n += (uint32_t)detailLen;
  o->out[o->n++] = '\n';
}
static void rinc_hostCompleterio(rio_Vm *v, rio_Val *a) {
  const char *line = (const char *)rio_sliceBytes(v, a);
  rinc_Offers o; rinc_Out into;
  o.out = (char *)rinc_into(v, a + 2, &into); o.cap = into.room; o.n = 0;
  a[0].i = 0;
  if (v == &rinc_nest || !o.out) return;
#ifdef rio_NO_COMPILER
  (void)line;
#else
  { /* (the check's tables, if the room's as the check left it; else just the words, and the line's own names) */
    const uint8_t *c = (const uint8_t *)rinc_nest.compiler; /* (still the check's: not a session's since) */
    int ok = rinc_cplOn && c >= rinc_checkRoom && c < rinc_checkRoom + rinc_checkSize && rinc_roomHash(rinc_checkRoom, rinc_checkSize) == rinc_checkHash;
    rio_complete(ok ? &rinc_nest : 0, line ? line : "", line ? a[1].u : 0, rinc_offer, &o);
  }
#endif
  rinc_outAdd(v, &into, o.n);
  a[0].i = (int32_t)o.n;
}

/* where an app's error is: the file it included or imported, if it was in one (else 0: the app's own) */
const char *rinc_errorPart(rio_Vm *vm) { return rio_errorInfo(vm).file; }
/* where the last app to stop with an error stopped, until an editor takes it (pc.appError): its file
   (the app's main file, else a part's, under /rinc unless it's the host's), and what was wrong */
static struct { char path[128], message[rio_ERROR_BUF_SIZE]; int line, col, len, runtime; } rinc_lastError;
static void rinc_keepError(rio_Vm *vm, const char *name) {
  rio_Error e = rio_errorInfo(vm);
  char app[rinc_REF];
  rinc_lastError.line = 0;
  if (e.kind == rio_ErrorKind_none || e.line <= 0) return;
  if (!e.file) { if (!rinc_runPath(name, app, sizeof app, rinc_lastError.path, sizeof rinc_lastError.path)) return; }
  else if (!strncmp(e.file, "apps/", 5) || !strncmp(e.file, "projects/", 9) || !strncmp(e.file, "lib/", 4))
    snprintf(rinc_lastError.path, sizeof rinc_lastError.path, "/rinc/%s", e.file);
  else snprintf(rinc_lastError.path, sizeof rinc_lastError.path, "%s", e.file);
  snprintf(rinc_lastError.message, sizeof rinc_lastError.message, "%s", e.msg ? e.msg : rio_error(vm));
  rinc_lastError.line = e.line; rinc_lastError.col = e.col; rinc_lastError.len = e.len;
  rinc_lastError.runtime = e.kind == rio_ErrorKind_runtime;
}
static void rinc_hostAppError(rio_Vm *v, rio_Val *a) { /* (-> pc.AppError: where it goes in a[0], a[1]; path, message after, added to) */
  pc_AppError r;
  memset(&r, 0, sizeof r);
  if (rinc_lastError.line > 0) {
    r.line = rinc_lastError.line; r.col = rinc_lastError.col; r.len = rinc_lastError.len; r.runtime = rinc_lastError.runtime;
    rinc_addOut(v, a + 2, rinc_lastError.path, (int)strlen(rinc_lastError.path));
    rinc_addOut(v, a + 5, rinc_lastError.message, (int)strlen(rinc_lastError.message));
  }
  rinc_lastError.line = 0;
  rio_putStruct(v, a, &r, sizeof r);
}

/* an app's error, after where it is (and kept, for an editor) */
static void rinc_sayError(rio_Vm *vm, const char *name, int image) {
  const char *part = image ? 0 : rinc_errorPart(vm);
  rinc_keepError(vm, name);
  if (part) { rinc_say(part); rinc_say(":"); }
  else { rinc_say(name); rinc_say(image ? ": " : ".rio:"); }
  rinc_say(rio_error(vm));
  rinc_logFlush(); /* (now: it's stopped) */
}

int rinc_atLimit(const char *e) { return strstr(e, "out of") || strstr(e, "too many") || strstr(e, "too complex") || strstr(e, "too deep"); }

static int rinc_systemDown;
int rinc_systemHeld(void) { return rinc_systemDown; }
uint8_t rinc_overlay[rinc_OVERLAY_H][rinc_W];
volatile int rinc_overlayOn;
/* text into the overlay, at pixel x on its row */
static int rinc_overlayText(const char *s, int x, int fg) {
  for (; *s; s++, x += 6) {
    const uint8_t *g = *s > 32 && *s < 127 ? rinc_font[*s - 32] : rinc_font[0];
    int r, k;
    for (r = 0; r < 8; r++)
      for (k = 0; k < 6 && x + k < rinc_W; k++) if (g[r] >> k & 1) rinc_overlay[2 + r][x + k] = (uint8_t)fg;
  }
  return x;
}
static void rinc_overlayDot(int x, int y, int c) { if (x >= 0 && x < rinc_W && y >= 0 && y < rinc_OVERLAY_H) rinc_overlay[y][x] = (uint8_t)c; }

/* recent apps, the latest first (each app's start puts it there): for Sys+Tab and Sys+Q */
#define rinc_RECENT 8
static char rinc_recent[rinc_RECENT][rinc_REF];
static int rinc_recentCount;
static void rinc_recentDrop(int i);
/* this app left out of the recent apps: the one before it (else home) */
static const char *rinc_recentBefore(void) {
  rinc_recentDrop(0);
  return rinc_recentCount ? rinc_recent[0] : "home";
}
static void rinc_recentDrop(int i) {
  for (; i + 1 < rinc_recentCount; i++) memcpy(rinc_recent[i], rinc_recent[i + 1], sizeof rinc_recent[0]);
  if (rinc_recentCount > 0) rinc_recentCount--;
}
static void rinc_recentAdd(const char *name) {
  int i;
  if (strchr(name, '/')) return; /* (an app's other file, its tests say: not one to go back to) */
  for (i = 0; i < rinc_recentCount && strcmp(rinc_recent[i], name); i++) {}
  if (i < rinc_recentCount) rinc_recentDrop(i);
  else if (rinc_recentCount == rinc_RECENT) rinc_recentCount--;
  memmove(rinc_recent[1], rinc_recent[0], sizeof rinc_recent[0] * (size_t)rinc_recentCount);
  snprintf(rinc_recent[0], sizeof rinc_recent[0], "%s", name);
  rinc_recentCount++;
}

/* the performance strip (Sys+P): each frame's time, against the 1/60 s a frame has */
static uint16_t rinc_frameMs[256]; /* (in hundredths of a millisecond: 655 ms at most) */
static int rinc_frameAt, rinc_perfOn;
static void rinc_perfNote(float ms) { rinc_frameMs[rinc_frameAt++ & 255] = (uint16_t)(ms < 0 ? 0 : ms > 655 ? 65500 : ms * 100.0f); }

static int rinc_switching, rinc_pick, rinc_switchHeld; /* the switcher (Sys+Tab) is up, which recent app it's on, and whether the system key's held (letting go switches) */

static int rinc_sysLock; /* Ctrl+Shift tapped: the system key, for the next combination */
int rinc_fullscreen;      /* Sys+Enter asked for the screen to fill the display, or not (rinc.h) */
int rinc_sleeping;        /* Sys+L (rinc.h) */
static int rinc_waking;   /* woken, until every key's let go: what's still down isn't the app's */

/* the locks on, as badges at the bottom right */
/* a notice at the bottom left, over the screen, for a few seconds (no help for this app, say) */
static char rinc_notice[48];
static float rinc_noticeUntil;
static void rinc_noticeSay(const char *what, const char *name) {
  snprintf(rinc_notice, sizeof rinc_notice, "%s%.32s", what, name);
  rinc_noticeUntil = (rinc_time ? rinc_time() : 0) + 3;
}
static int rinc_noticeOn(void) { return rinc_notice[0] && (!rinc_time || rinc_time() < rinc_noticeUntil); }
static void rinc_badges(void) {
  static const char *const names[4] = {"SYS", "ALT", "CTRL", "SHIFT"};
  int on[4], i, x = rinc_W - 2, y;
  on[0] = rinc_sysLock; on[1] = rinc_locks & rinc_Mod_alt; on[2] = rinc_locks & rinc_Mod_ctrl; on[3] = rinc_locks & rinc_Mod_shift;
  for (i = 0; i < 4; i++) {
    int w = (int)strlen(names[i]) * 6 + 3;
    if (!on[i]) continue;
    x -= w;
    for (y = 1; y < rinc_OVERLAY_H; y++) memset(&rinc_overlay[y][x], i ? 2 : 10, (size_t)w);
    rinc_overlayText(names[i], x + 2, i ? 7 : 0);
    x -= 2;
  }
}

/* the overlay: the switcher, else the system key's combinations, else the performance strip (strip: one
   of those, across the bottom), and any locks; where there's no strip, the screen shows through */
static void rinc_overlayDraw(int strip) {
  int x, i;
  memset(rinc_overlay, strip ? 0 : rinc_SEE_THROUGH, sizeof rinc_overlay);
  if (!strip) {
    if (rinc_noticeOn()) { /* (on a box of its own) */
      int w = (int)strlen(rinc_notice) * 6 + 3, y;
      for (y = 1; y < rinc_OVERLAY_H; y++) memset(rinc_overlay[y], 2, (size_t)(w < rinc_W ? w : rinc_W));
      rinc_overlayText(rinc_notice, 2, 7);
    }
  } else if (rinc_switching) {
    memset(rinc_overlay[0], 12, rinc_W); /* (a blue edge) */
    x = rinc_overlayText("SWITCH", 4, 12) + 12;
    for (i = 0; i < rinc_recentCount && x < rinc_W; i++) {
      int end = x + (int)strlen(rinc_recent[i]) * 6, y;
      if (i == rinc_pick)
        for (y = 1; y < rinc_OVERLAY_H; y++) memset(&rinc_overlay[y][x - 2 < 0 ? 0 : x - 2], 2, (size_t)((end + 2 > rinc_W ? rinc_W : end + 2) - (x - 2 < 0 ? 0 : x - 2)));
      x = rinc_overlayText(rinc_recent[i], x, i == rinc_pick ? 7 : 6) + 12;
    }
  } else if (rinc_systemDown) {
    memset(rinc_overlay[0], 10, rinc_W); /* (a yellow edge) */
    rinc_overlayText("Esc home Tab apps P commands H help O log L sleep", 2, 7);
  } else { /* the performance strip: the latest frame's time, then a bar a frame, the newest at the right: 8 high is 1/60 s */
    char text[24]; float ms = (float)rinc_frameMs[(rinc_frameAt - 1) & 255] / 100.0f;
    int pct = (int)(ms * 6.0f + 0.5f); /* (of 16.67 ms) */
    memset(rinc_overlay[0], 5, rinc_W);
    if (ms < 10) snprintf(text, sizeof text, "%d.%02dms %d%%", (int)ms, (int)(ms * 100.0f) % 100, pct);
    else snprintf(text, sizeof text, "%d.%dms %d%%", (int)ms, (int)(ms * 10.0f) % 10, pct);
    rinc_overlayText(text, 2, pct < 50 ? 11 : pct <= 100 ? 10 : 8);
    for (x = 90; x < rinc_W - 2; x++) {
      int age = rinc_W - 3 - x, h, y; float f;
      if (age >= rinc_frameAt || age >= 256) continue;
      f = (float)rinc_frameMs[(rinc_frameAt - 1 - age) & 255] * 0.06f; /* (percent of the budget) */
      h = (int)(f * 8.0f / 100.0f + 0.5f);
      if (h > 9) h = 9;
      if (f > 0 && h < 1) h = 1;
      for (y = 0; y < h; y++) rinc_overlayDot(x, rinc_OVERLAY_H - 1 - y, f < 50 ? 11 : f <= 100 ? 10 : 8);
      if (h < 8 && x % 2 == 0) rinc_overlayDot(x, rinc_OVERLAY_H - 9, 5); /* (the budget's line) */
    }
  }
  rinc_badges();
}

/* the system key's combinations and the locks, each frame (rinc.h). A press of modifiers alone (nothing
   else down in it), let go within 0.4 s, is a tap: Shift, Ctrl or Alt tapped twice in a row locks it,
   and a tap unlocks it; Ctrl and Shift tapped together lock the system key, for the next combination */
static int rinc_sysPointer; /* the pointer's system keys held (rinc_SysKey_pointer), for its next frame */
/* an app's own help: its help/README, in storage or the bundle (a help app to show it, too) */
static int rinc_hasHelp(const char *app) {
  char path[96]; uint32_t total; int n = (int)strlen(app), project = n > 1 && app[n - 1] == rinc_PROJECT;
  if (!rinc_bundled("help") && rinc_readApp("help", 0, 0) < 0) return 0;
  snprintf(path, sizeof path, "/rinc/%s/%.*s/help/README.md", project ? "projects" : "apps", project ? n - 1 : n, app);
  if (!rinc_readFile(path, 0, 0, &total)) return 1;
  if (project) return 0;
  snprintf(path, sizeof path, "apps/%.32s/help/README.md", app);
  return rinc_bundleFile(path) != 0;
}
int rinc_modsHeld = -1;
static int rinc_sysDo(int pressed, int mods, int held); /* (below) */
static int rinc_forgetting; /* (this app, forgotten as it gives way: no leaving*, rinc_appLeave) */
int rinc_systemKey(int sysDown, int mods, int otherDown, int keys) {
  static int now, down, seen, spoiled, startAt, lastTap, lastTapAt, again, wasKeys, wasOther;
  int pressed = keys & ~wasKeys, otherPressed = otherDown && !wasOther, go = 0, strip;
  now++;
  rinc_modsHeld = mods;
  { /* asleep: once nothing's down, anything wakes it; and until it's all let go, nothing's the app's */
    static int armed;
    int any = sysDown || otherDown || mods || keys;
    if (rinc_sleeping && !any) armed = 1;
    else if (rinc_sleeping && armed) { rinc_sleeping = 0; rinc_waking = 1; armed = 0; }
    else if (rinc_waking && !any) rinc_waking = 0;
    if (rinc_sleeping || rinc_waking) {
      wasKeys = keys; wasOther = otherDown;
      rinc_systemDown = 1;
      return 0;
    }
  }
  wasKeys = keys;
  wasOther = otherDown;
  mods &= 7; /* (1 Shift, 2 Ctrl, 4 Alt) */
  if (mods && !down) { down = 1; seen = spoiled = 0; startAt = now; again = lastTap && now - lastTapAt <= 24; }
  if (down) {
    seen |= mods;
    if (otherDown || keys) spoiled = 1;
    if (!mods) { /* let go: a tap? */
      int bit = seen == 1 ? rinc_Mod_shift : seen == 2 ? rinc_Mod_ctrl : seen == 4 ? rinc_Mod_alt : 0;
      down = 0;
      if (spoiled || now - startAt > 24) lastTap = 0;
      else if (seen == 3) { rinc_sysLock = !rinc_sysLock; lastTap = 0; }
      else if (bit && (rinc_locks & bit)) { rinc_locks &= ~bit; lastTap = 0; }
      else if (bit && again && lastTap == seen) { rinc_locks |= bit; lastTap = 0; }
      else { lastTap = bit ? seen : 0; lastTapAt = now; }
    }
  } else if (otherDown || keys) lastTap = 0; /* (typing between taps) */
  if (rinc_switching) { /* Tab and Shift+Tab move; letting go of a held system key, or Enter, switches; Esc doesn't (no time limits) */
    int n = rinc_recentCount;
    if ((pressed & (rinc_SysKey_tab | rinc_SysKey_next | rinc_SysKey_prev)) && n > 0) {
      int back = (pressed & rinc_SysKey_prev) || ((pressed & rinc_SysKey_tab) && (mods & 1));
      rinc_pick = (rinc_pick + (back ? n - 1 : 1)) % n;
    }
    if (pressed & rinc_SysKey_esc) rinc_switching = 0;
    else if ((pressed & rinc_SysKey_enter) || (rinc_switchHeld && !sysDown)) {
      rinc_switching = 0;
      if (rinc_pick > 0 && rinc_pick < n) { snprintf(rinc_launchName, sizeof rinc_launchName, "%s", rinc_recent[rinc_pick]); go = 1; }
    }
  } else if ((sysDown || rinc_sysLock) && pressed && !(pressed & ~rinc_SysKey_pointer)) {
    /* the pointer's keys only: they move and click it (rinc_pointerFrame), and don't use up a locked system key */
  } else if ((sysDown || rinc_sysLock) && pressed) { /* (alone, the system key does nothing, so it isn't pressed by accident) */
    go = rinc_sysDo(pressed, mods, sysDown);
    rinc_sysLock = 0; /* (used up) */
  } else if (rinc_sysLock && otherPressed && !down) rinc_sysLock = 0; /* (another key: no combination) */
  rinc_systemDown = sysDown || rinc_sysLock || rinc_switching;
  rinc_sysPointer = rinc_systemDown && !rinc_switching ? keys & rinc_SysKey_pointer : 0;
  strip = !go && (rinc_systemDown || rinc_perfOn);
  rinc_overlayOn = 0; /* (while it's redrawn) */
  if (strip || rinc_locks || rinc_sysLock || rinc_noticeOn()) { rinc_overlayDraw(strip); rinc_overlayOn = 1; }
  return go;
}
/* what the system key does with keys pressed (rinc_SysKey_* bits; mods: 1 Shift; held: the system key
   is, so letting go of it ends the switcher): 1 to start rinc_launchName. The command list's own
   entries do the same */
static int rinc_sysDo(int pressed, int mods, int held) {
  int go = 0;
  {
    if (pressed & rinc_SysKey_esc) { snprintf(rinc_launchName, sizeof rinc_launchName, "home"); go = 1; }
    else if ((pressed & (rinc_SysKey_tab | rinc_SysKey_next | rinc_SysKey_prev)) && rinc_recentCount > 1) {
      int back = (pressed & rinc_SysKey_prev) || ((pressed & rinc_SysKey_tab) && (mods & 1));
      rinc_switching = 1; rinc_pick = back ? rinc_recentCount - 1 : 1;
      rinc_switchHeld = held;
    } else if (pressed & rinc_SysKey_q) { /* back to the app before, this one forgotten, its stash too */
      rinc_stashDrop(rinc_appNow);
      rinc_forgetting = 1;
      rinc_recentDrop(0);
      snprintf(rinc_launchName, sizeof rinc_launchName, "%s", rinc_recentCount ? rinc_recent[0] : "home");
      go = 1;
    } else if (pressed & rinc_SysKey_r) { /* this app again, fresh: its stash dropped, and its compiled image */
      rinc_stashDrop(rinc_appNow);
      rinc_forgetting = 1;
      rinc_cacheDrop(rinc_appRef);
      snprintf(rinc_launchName, sizeof rinc_launchName, "%s", rinc_appRef);
      go = 1;
    } else if (pressed & rinc_SysKey_o) { /* this app's log, in the log app, which comes back here */
      if (strcmp(rinc_appRef, "log")) {
        rinc_logFlush();
        snprintf(rinc_launchName, sizeof rinc_launchName, "log");
        rinc_callArgsLen = snprintf(rinc_callArgs, sizeof rinc_callArgs, "%s", rinc_appRef);
        rinc_calling = 1;
        go = 1;
      }
    } else if (pressed & rinc_SysKey_h) { /* this app's help, if it has any */
      if (rinc_hasHelp(rinc_appRef)) { /* (a project's own: its name with the +, then /README) */
        snprintf(rinc_helpTopicNow, sizeof rinc_helpTopicNow, "%.33s/README", rinc_appRef);
        snprintf(rinc_launchName, sizeof rinc_launchName, "help");
        go = 1;
      } else rinc_noticeSay("No help for ", rinc_appRef);
    } else if (pressed & rinc_SysKey_p) rinc_cmdWanted = 1; /* (the command list: once the app's frame can ask it for its own) */
    else if (pressed & rinc_SysKey_m) rinc_perfOn = !rinc_perfOn;
    else if (pressed & rinc_SysKey_l) rinc_sleeping = 1; /* (asleep: the display rests, until a key) */
    else if (pressed & rinc_SysKey_enter) rinc_fullscreen = !rinc_fullscreen;
  }
  return go;
}

/* the pointer (docs/spec/input.md#the-pointer): a mouse's, and in pointer mode a controller's too (its left
   stick moves it, its right one scrolls, A and B click while it's shown), drawn by Rinc over the screen while it's shown:
   moving it shows it, typing hides it. Modes: 0 (the default, for games) a mouse's only, the left stick
   still the arrows; 1 a pointer app's; 2 none */
int rinc_pointerMode, rinc_pointerAtX = 160, rinc_pointerAtY = 120;
volatile int rinc_pointerOnScreen;
uint8_t rinc_pointerImage[8][8];
static float rinc_pointerX = 160, rinc_pointerY = 120, rinc_scrolled[2];
static int rinc_mouseHeld, rinc_pointerHeld;
static void rinc_pointerShow(int on) {
  static const char *const arrow[8] = {"K.......", "KK......", "KWK.....", "KWWK....", "KWWWK...", "KWWWWK..", "KWWKKKK.", "KKK....."};
  int x, y;
  if (!rinc_pointerImage[0][7]) /* (the arrow, the first time: K its edge, W its fill) */
    for (y = 0; y < 8; y++) for (x = 0; x < 8; x++) rinc_pointerImage[y][x] = arrow[y][x] == 'K' ? 0 : arrow[y][x] == 'W' ? 7 : rinc_SEE_THROUGH;
  rinc_pointerOn = on && rinc_pointerMode != 2;
  rinc_pointerAtX = (int)rinc_pointerX; rinc_pointerAtY = (int)rinc_pointerY;
  rinc_pointerOnScreen = rinc_pointerOn;
}
void rinc_mouseMove(float x, float y) {
  rinc_pointerX = x < 0 ? 0 : x > rinc_W - 1 ? rinc_W - 1 : x;
  rinc_pointerY = y < 0 ? 0 : y > rinc_H - 1 ? rinc_H - 1 : y;
  rinc_pointerShow(1);
}
void rinc_mouseButtons(int held) { if (held & ~rinc_mouseHeld) rinc_pointerShow(1); rinc_mouseHeld = held; } /* (pressing one shows it) */
void rinc_mouseGone(void) { if (!rinc_mouseHeld) rinc_pointerOn = rinc_pointerOnScreen = 0; } /* (not while dragging) */
void rinc_mouseWheel(float dx, float dy) { rinc_scrolled[0] += dx; rinc_scrolled[1] += dy; }
int rinc_lettersAsButtons(void) { return rinc_pointerMode != 1; }
/* each frame: in pointer mode, the controller moves it, scrolls, and clicks */
static void rinc_pointerFrame(void) {
  static int arrows, keyFrames;
  int keyed = rinc_pointerMode == 2 ? 0 : rinc_sysPointer, held = rinc_mouseHeld | (keyed & rinc_SysKey_a ? 1 : 0) | (keyed & rinc_SysKey_b ? 2 : 0), now = rinc_buttons & (rinc_Button_left | rinc_Button_right | rinc_Button_up | rinc_Button_down |
                                                  rinc_Button_start | rinc_Button_back);
  now |= rinc_systemHeld() ? 0x10000 : 0; /* (and the system key: a controller's guide button, Ctrl and Shift) */
  if (now & ~arrows) rinc_pointerOn = rinc_pointerOnScreen = 0; /* (an arrow pressed, the d-pad's too, or a center button: as typing, it hides the pointer) */
  arrows = now;
  if (keyed & (rinc_SysKey_left | rinc_SysKey_right | rinc_SysKey_up | rinc_SysKey_down)) { /* (1 pixel a frame, up to 6 after a second) */
    float step = (float)(1 + (keyFrames < 60 ? keyFrames / 12 : 5));
    keyFrames++;
    rinc_mouseMove(rinc_pointerX + (keyed & rinc_SysKey_right ? step : 0) - (keyed & rinc_SysKey_left ? step : 0),
                   rinc_pointerY + (keyed & rinc_SysKey_down ? step : 0) - (keyed & rinc_SysKey_up ? step : 0));
  } else keyFrames = 0;
  if (held & ~rinc_pointerHeld) rinc_pointerShow(1);
  if (rinc_pointerMode == 1) {
    float lx = rinc_stickAt(0), ly = rinc_stickAt(1), rx = rinc_stickAt(2), ry = rinc_stickAt(3);
    if (lx || ly) { /* (faster the further: up to 5 pixels a frame) */
      rinc_pointerX += lx * (lx < 0 ? -lx : lx) * 5.0f; rinc_pointerY += ly * (ly < 0 ? -ly : ly) * 5.0f;
      rinc_mouseMove(rinc_pointerX, rinc_pointerY);
    }
    rinc_scrolled[0] += rx * 0.3f; rinc_scrolled[1] -= ry * 0.3f; /* (up: scrolled up, as a wheel) */
    /* (A and B click only while the pointer's shown, and a click goes on till it's let go. Hidden, they're
       the controller's buttons, and Enter and Esc too, pressed: a pointer app's keyboard types only, so
       here they're a controller's, rinc_lettersAsButtons) */
    static uint16_t was;
    uint16_t pressed = (uint16_t)(rinc_buttons & ~was);
    if ((rinc_buttons & rinc_Button_a) && (rinc_pointerOn || (rinc_pointerHeld & 1))) held |= 1;
    else if (pressed & rinc_Button_a) rinc_textKey(13);
    if ((rinc_buttons & rinc_Button_b) && (rinc_pointerOn || (rinc_pointerHeld & 2))) held |= 2;
    else if (pressed & rinc_Button_b) rinc_textEsc();
    was = rinc_buttons;
    if (held & ~rinc_pointerHeld) rinc_pointerShow(1);
  }
  rinc_pointerHeld = held;
}
/* pointer(mode); pointerX() -> Int, pointerY() -> Int; pointerDown(i) -> Int (0 primary, 1 secondary, 2
   middle); pointerShown() -> Int; scroll(i) -> Int: the whole steps scrolled since (0 across, + right; 1
   up and down, + up), taken */
static void rinc_hostPointer(rio_Vm *v, rio_Val *a) { (void)v; rinc_pointerMode = a[0].i < 0 || a[0].i > 2 ? 0 : a[0].i; rinc_pointerShow(rinc_pointerOn); }
static void rinc_hostPointerX(rio_Vm *v, rio_Val *a) { (void)v; a[0].i = (int)rinc_pointerX; }
static void rinc_hostPointerAt(rio_Vm *v, rio_Val *a) { (void)v; a[0].i = (int)rinc_pointerX; a[1].i = (int)rinc_pointerY; } /* ({x, y}) */
static void rinc_hostPointerY(rio_Vm *v, rio_Val *a) { (void)v; a[0].i = (int)rinc_pointerY; }
static void rinc_hostPointerDown(rio_Vm *v, rio_Val *a) { (void)v; a[0].i = a[0].u < 3 && !rinc_cmdSettle ? rinc_pointerHeld >> a[0].i & 1 : 0; }
static void rinc_hostPointerShown(rio_Vm *v, rio_Val *a) { (void)v; a[0].i = rinc_pointerOnScreen; }
static void rinc_hostScroll(rio_Vm *v, rio_Val *a) {
  int k;
  (void)v;
  if (a[0].u > 1) { a[0].i = 0; return; }
  k = (int)rinc_scrolled[a[0].i];
  rinc_scrolled[a[0].i] -= (float)k;
  a[0].i = k;
}

/* pc's source, pc.rio, as pc_host.h has it (rio -H: with the sigs above), for compiling apps against */
static int rinc_pcSource(rio_Vm *vm) {
#ifdef rio_NO_COMPILER
  (void)vm;
  return 0;
#else
  return rio_addSource(vm, "pc", pc_SOURCE, sizeof pc_SOURCE - 1);
#endif
}
int rinc_hostProcs(rio_Vm *vm) {
  return rio_addFfi(vm, "pc.cls", pc_cls_SIG, rinc_hostCls) || rio_addFfi(vm, "pc.pset", pc_pset_SIG, rinc_hostPset) || rio_addFfi(vm, "pc.pget", pc_pget_SIG, rinc_hostPget) || rio_addFfi(vm, "pc.appsFor", pc_appsFor_SIG, rinc_hostAppsFor) || rio_addFfi(vm, "pc.showCommands", pc_showCommands_SIG, rinc_hostShowCommands) || rio_addFfi(vm, "pc.rgb", pc_rgb_SIG, rinc_hostRgb) ||
         rio_addFfi(vm, "pc.rect", pc_rect_SIG, rinc_hostRect) || rio_addFfi(vm, "pc.blit", pc_blit_SIG, rinc_hostBlit) || rio_addFfi(vm, "pc.print", pc_print_SIG, rinc_hostPrint) || rio_addFfi(vm, "pc.btn", pc_btn_SIG, rinc_hostBtn) || rinc_audioProcs(vm) || rio_addFfi(vm, "pc.time", pc_time_SIG, rinc_hostTime) ||
         rio_addFfi(vm, "pc.fromHex", pc_fromHex_SIG, rinc_hostFromhex) || rio_addFfi(vm, "pc.formatRio", pc_formatRio_SIG, rinc_hostFormatrio) || rio_addFfi(vm, "pc.moduleSource", pc_moduleSource_SIG, rinc_hostModulesource) || rio_addFfi(vm, "pc.stick", pc_stick_SIG, rinc_hostStick) || rio_addFfi(vm, "pc.trigger", pc_trigger_SIG, rinc_hostTrigger) ||
         rio_addFfi(vm, "pc.key", pc_key_SIG, rinc_hostKey) || rio_addFfi(vm, "pc.modsHeld", pc_modsHeld_SIG, rinc_hostModsHeld) || rio_addFfi(vm, "pc.keyHeld", pc_keyHeld_SIG, rinc_hostKeyHeld) || rio_addFfi(vm, "pc.apps", pc_apps_SIG, rinc_hostApps) || rio_addFfi(vm, "pc.appName", pc_appName_SIG, rinc_hostAppName) ||
         rio_addFfi(vm, "pc.appInfo", pc_appInfo_SIG, rinc_hostAppInfo) || rio_addFfi(vm, "pc.launch", pc_launch_SIG, rinc_hostLaunch) ||
         rio_addFfi(vm, "pc.appSource", pc_appSource_SIG, rinc_hostAppSource) || rio_addFfi(vm, "pc.appSave", pc_appSave_SIG, rinc_hostAppSave) ||
         rio_addFfi(vm, "pc.appDelete", pc_appDelete_SIG, rinc_hostAppDelete) ||
         rio_addFfi(vm, "pc.helps", pc_helps_SIG, rinc_hostHelps) ||
         rio_addFfi(vm, "pc.helpRead", pc_helpRead_SIG, rinc_hostHelpRead) || rio_addFfi(vm, "pc.help", pc_help_SIG, rinc_hostHelp) ||
         rio_addFfi(vm, "pc.helpTopic", pc_helpTopic_SIG, rinc_hostHelpTopic) ||
         rio_addFfi(vm, "pc.stashWrite", pc_stashWrite_SIG, rinc_hostStashWrite) || rio_addFfi(vm, "pc.stashRead", pc_stashRead_SIG, rinc_hostStashRead) || rio_addFfi(vm, "pc.stashWriteAt", pc_stashWriteAt_SIG, rinc_hostStashWriteAt) || rio_addFfi(vm, "pc.stashReadAt", pc_stashReadAt_SIG, rinc_hostStashReadAt) ||
         rio_addFfi(vm, "pc.stashDelete", pc_stashDelete_SIG, rinc_hostStashDelete) ||
         rio_addFfi(vm, "pc.cacheRead", pc_cacheRead_SIG, rinc_hostCacheRead) || rio_addFfi(vm, "pc.cacheWrite", pc_cacheWrite_SIG, rinc_hostCacheWrite) ||
         rio_addFfi(vm, "pc.cacheDelete", pc_cacheDelete_SIG, rinc_hostCacheDelete) || rio_addFfi(vm, "pc.boot", pc_boot_SIG, rinc_hostBoot) ||
         rio_addFfi(vm, "pc.settingRead", pc_settingRead_SIG, rinc_hostSettingRead) || rio_addFfi(vm, "pc.settingWrite", pc_settingWrite_SIG, rinc_hostSettingWrite) || rio_addFfi(vm, "pc.theme", pc_theme_SIG, rinc_hostTheme) ||
         rio_addFfi(vm, "pc.pointer", pc_pointer_SIG, rinc_hostPointer) || rio_addFfi(vm, "pc.appInstall", pc_appInstall_SIG, rinc_hostAppInstall) ||
         rio_addFfi(vm, "pc.appCopy", pc_appCopy_SIG, rinc_hostAppCopy) ||
         rio_addFfi(vm, "pc.call", pc_call_SIG, rinc_hostCall) || rio_addFfi(vm, "pc.args", pc_args_SIG, rinc_hostArgs) ||
         rio_addFfi(vm, "pc.done", pc_done_SIG, rinc_hostDone) || rio_addFfi(vm, "pc.result", pc_result_SIG, rinc_hostResult) ||
         rio_addFfi(vm, "pc.appError", pc_appError_SIG, rinc_hostAppError) ||
         rio_addFfi(vm, "pc.dir", pc_dir_SIG, rinc_hostDir) || rio_addFfi(vm, "pc.fileRead", pc_fileRead_SIG, rinc_hostFileRead) ||
         rio_addFfi(vm, "pc.fileWrite", pc_fileWrite_SIG, rinc_hostFileWrite) || rio_addFfi(vm, "pc.fileReadAt", pc_fileReadAt_SIG, rinc_hostFileReadAt) || rio_addFfi(vm, "pc.fileWriteAt", pc_fileWriteAt_SIG, rinc_hostFileWriteAt) || rio_addFfi(vm, "pc.fileDelete", pc_fileDelete_SIG, rinc_hostFileDelete) || rio_addFfi(vm, "pc.imageLoad", pc_imageLoad_SIG, rinc_hostImageLoad) || rio_addFfi(vm, "pc.imageWrite", pc_imageWrite_SIG, rinc_hostImageWrite) || rio_addFfi(vm, "pc.imageNotes", pc_imageNotes_SIG, rinc_hostImageNotes) ||
         rio_addFfi(vm, "pc.fileCopy", pc_fileCopy_SIG, rinc_hostFileCopy) || rio_addFfi(vm, "pc.dirMake", pc_dirMake_SIG, rinc_hostDirMake) || rio_addFfi(vm, "pc.fileRename", pc_fileRename_SIG, rinc_hostFileRename) || rio_addFfi(vm, "pc.appRename", pc_appRename_SIG, rinc_hostAppRename) || rio_addFfi(vm, "pc.pointerX", pc_pointerX_SIG, rinc_hostPointerX) || rio_addFfi(vm, "pc.pointerAt", pc_pointerAt_SIG, rinc_hostPointerAt) ||
         rio_addFfi(vm, "pc.pointerY", pc_pointerY_SIG, rinc_hostPointerY) || rio_addFfi(vm, "pc.pointerDown", pc_pointerDown_SIG, rinc_hostPointerDown) ||
         rio_addFfi(vm, "pc.pointerShown", pc_pointerShown_SIG, rinc_hostPointerShown) || rio_addFfi(vm, "pc.scroll", pc_scroll_SIG, rinc_hostScroll) ||
         rio_addFfi(vm, "pc.saveRead", pc_saveRead_SIG, rinc_hostSaveRead) || rio_addFfi(vm, "pc.saveWrite", pc_saveWrite_SIG, rinc_hostSaveWrite) ||
         rio_addFfi(vm, "pc.saveDelete", pc_saveDelete_SIG, rinc_hostSaveDelete) || rio_addFfi(vm, "pc.rioStart", pc_rioStart_SIG, rinc_hostRioStart) ||
         rio_addFfi(vm, "pc.rioEval", pc_rioEval_SIG, rinc_hostRioEval) || rio_addFfi(vm, "pc.rioFrame", pc_rioFrame_SIG, rinc_hostRioFrame) ||
         rio_addFfi(vm, "pc.rioType", pc_rioType_SIG, rinc_hostRioType) || rio_addFfi(vm, "pc.rioError", pc_rioError_SIG, rinc_hostRioError) ||
         rio_addFfi(vm, "pc.rioLog", pc_rioLog_SIG, rinc_hostRioLog) || rio_addFfi(vm, "pc.rioIndent", pc_rioIndent_SIG, rinc_hostRioIndent) || rio_addFfi(vm, "pc.checkRio", pc_checkRio_SIG, rinc_hostCheckrio) || rio_addFfi(vm, "pc.completeRio", pc_completeRio_SIG, rinc_hostCompleterio) || rio_addFfi(vm, "pc.checkError", pc_checkError_SIG, rinc_hostCheckerror) ||
         rinc_pcSource(vm);
}

/* a game's frame: update then draw */
/* starting an app, the same on every Rinc: rinc_appBegin, then rinc_appRun (its source compiled, read a
   part at a time from its file, or its image loaded). Whatever goes wrong goes on the screen, with the way home
   (and, when it ran out of something, how near each limit it came) */
static rio_Usage rinc_usage;
/* the app running, about to give way to another (docs/spec/apps.md#leaving): its leaving*, if it has
   one, to keep where it is in its stash. Not when it's being forgotten (the system key with Q or R: its
   stash dropped, and staying so) */

void rinc_appLeave(rio_Vm *vm) {
  int fn = rinc_forgetting || vm == &rinc_nest ? -1 : rio_findProc(vm, "leaving");
  rinc_forgetting = 0;
  if (fn >= 0) (void)rio_call(vm, fn); /* (what goes wrong there, it's too late to show: the next app starts) */
}
int rinc_appBegin(rio_Vm *vm, void *space, uint32_t size, const char *name, void (*put)(const char *s, int n)) {
  rinc_appLeave(vm);
  rinc_game = 0;
  rinc_showLog = 1;
  rinc_put = put;
  rinc_themeLoad();
  rinc_consoleClear(rinc_themeColor(0));
  if (rinc_start(vm, space, size, put)) { rinc_say("can't start rio"); rinc_back(); return -1; }
  rinc_logStart(name);
  rinc_callStart(name);
  rinc_setAppName(name);
  snprintf(rinc_callee, sizeof rinc_callee, "%s", name); /* (who'd be the caller, if this calls) */
  rinc_projectSaves();
  rinc_recentAdd(name);
  rinc_pointerMode = 0; rinc_scrolled[0] = rinc_scrolled[1] = 0; /* (a game's, until it says) */
  return 0;
}
void rinc_appFail(const char *name, const char *why) {
  rinc_say(name); rinc_say(why); rinc_logFlush(); rinc_back();
}
int (*rinc_native)(rio_Vm *vm);
/* an app compiled (its source in memory, else read from path: host, the host's), or its image loaded:
   0, or -1 (said why) */
static int rinc_appLoad(rio_Vm *vm, void *space, uint32_t size, const char *name, const char *src, uint32_t len, const void *image, const char *path, int host) {
  int res;
  if (image) res = rio_loadImage(vm, image, len);
  else {
#ifdef rio_NO_COMPILER
    (void)space; (void)size; (void)src; (void)path; (void)host;
    rinc_appFail(name, ": this Rinc runs compiled apps only");
    return -1;
#else
    rio_trackUsage(vm, &rinc_usage);
    res = path ? rinc_compileFile(vm, space, size, path, host) : rinc_compile(vm, space, size, src, len);
#endif
  }
  if (res) {
    rinc_sayError(vm, name, image != 0);
#ifndef rio_NO_COMPILER
    if (!image && rinc_atLimit(rio_error(vm))) {
      char *text = (char *)space; /* (the app's memory: nothing's in it now) */
      int k, m = rio_usageText(&rinc_usage, text, 1024 < size ? 1024 : (int)size);
      rinc_say("\r\n");
      for (k = 0; k < m; k++) rinc_out(text[k] == '\n' ? "\r\n" : text + k, text[k] == '\n' ? 2 : 1);
    }
#endif
    rinc_back();
    return -1;
  }
  return 0;
}
/* an app loaded: its native code made, then its top-level code run, then main; a game from there on */
static int rinc_appGo(rio_Vm *vm, const char *name) {
  int fn;
  if (rinc_native && rinc_native(vm) < 0) { rinc_appFail(name, ": can't set up native code"); return -1; }
  if (rio_run(vm) || ((fn = rio_findProc(vm, "main")) >= 0 && rio_call(vm, fn))) {
    rinc_sayError(vm, name, 0); rinc_back();
    return -1;
  }
  if (rio_findProc(vm, "update") >= 0 || rio_findProc(vm, "draw") >= 0) { rinc_game = 1; rinc_showLog = 0; }
  else { rinc_consolePut("\r\n(done)", 8); rinc_back(); }
  return 0;
}
int rinc_appRun(rio_Vm *vm, void *space, uint32_t size, const char *name, const char *src, uint32_t len, const void *image) {
  return rinc_appLoad(vm, space, size, name, src, len, image, 0, 0) || rinc_appGo(vm, name);
}
int rinc_appRunHost(rio_Vm *vm, void *space, uint32_t size, const char *name, const char *path) {
  return rinc_appLoad(vm, space, size, name, 0, 0, 0, path, 1) || rinc_appGo(vm, name);
}

/* compiled apps, cached (docs/spec/apps.md#compiled-apps): /rinc/cache/rinc/apps/<app>.rioi, Rinc's
   header (which build made it, and the length and hash of the source it's from), then Rio's image.
   It's used only while it's still its source's: the source is read and hashed to tell (cheaper by far
   than compiling it), except on a Rinc that trusts what it's checked (rinc_cacheTrust: one where
   storage changes only through Rinc while it runs), which does that once an app, until something
   under /rinc/apps or /rinc/lib changes (rinc_storageChanged) or it starts again */
#define rinc_CACHE_MAGIC "RINCIMG2"
enum { rinc_CACHE_HEAD = 8 + 32 + 4 * 5 }; /* magic, build, source length, its hash (2 words), the image's length, how many files more */
/* After the header, each other file the compile read from storage (the app's own, the library's), to
   check too: its length and hash (3 words), then its path (/rinc/ left off), a byte for its length then
   the path, padded to whole words. Then Rio's image */
int rinc_cacheTrust;
static struct { char name[rinc_REF]; uint32_t len, hash[2]; } rinc_checked[8];
static int rinc_checkedCount, rinc_checkedNext;
void rinc_storageChanged(const char *path) {
  if (!strncmp(path, "/rinc/apps/", 11) || !strncmp(path, "/rinc/projects/", 15) || !strncmp(path, "/rinc/lib/", 10)) rinc_checkedCount = 0;
}
static void rinc_hash(const char *s, uint32_t n, uint32_t *h) { /* (FNV-1a, 64 bits) */
  uint64_t x = 0xcbf29ce484222325ull; uint32_t i;
  for (i = 0; i < n; i++) x = (x ^ (uint8_t)s[i]) * 0x100000001b3ull;
  h[0] = (uint32_t)x; h[1] = (uint32_t)(x >> 32);
}
static int rinc_cachePath(const char *name, char *path, size_t cap) { /* (a project's: /rinc/cache/rinc/projects/chord.rioi) */
  int n = (int)strlen(name), project = n > 1 && name[n - 1] == rinc_PROJECT;
  return snprintf(path, cap, "/rinc/cache/rinc/%s/%.*s.rioi", project ? "projects" : "apps", project ? n - 1 : n, name) < (int)cap;
}
void rinc_cacheDrop(const char *name) {
  char path[96]; int i;
  for (i = 0; i < rinc_checkedCount && i < (int)(sizeof rinc_checked / sizeof *rinc_checked); i++) if (!strcmp(rinc_checked[i].name, name)) rinc_checked[i].name[0] = 0;
  if (rinc_fs && rinc_cachePath(name, path, sizeof path)) rinc_fs->remove(path);
}
static int rinc_readAll(void *ud, void *bytes, uint32_t n) {
  char *p = (char *)bytes; int got;
  (void)ud;
  while (n) {
    if ((got = rinc_fs->read(p, n > 0x4000 ? 0x4000 : (int)n)) <= 0) return 1;
    p += got; n -= (uint32_t)got;
  }
  return 0;
}
static int rinc_writeAll(void *ud, const void *bytes, uint32_t n) { (void)ud; return n && rinc_fs->write((const char *)bytes, (int)n) != (int)n; }
static void rinc_cacheHead(uint8_t *head, uint32_t len, const uint32_t *hash, uint32_t imageLen) {
  uint32_t w[5]; w[0] = len; w[1] = hash[0]; w[2] = hash[1]; w[3] = imageLen; w[4] = (uint32_t)rinc_srcCount;
  memset(head, 0, rinc_CACHE_HEAD);
  memcpy(head, rinc_CACHE_MAGIC, 8);
  memcpy(head + 8, rinc_buildId, strlen(rinc_buildId) < 32 ? strlen(rinc_buildId) : 32);
  memcpy(head + 40, w, sizeof w);
}
/* a file's length and hash, read a piece at a time: 0, or -1 if it isn't there */
static int rinc_hashFile(const char *path, uint32_t *len, uint32_t *h) {
  uint64_t x = 0xcbf29ce484222325ull; char chunk[128]; int got, i; uint32_t n = 0;
  if (!rinc_fs || rinc_fs->open(path, 0)) return -1;
  while ((got = rinc_fs->read(chunk, sizeof chunk)) > 0) {
    for (i = 0; i < got; i++) x = (x ^ (uint8_t)chunk[i]) * 0x100000001b3ull;
    n += (uint32_t)got;
  }
  rinc_fs->close();
  *len = n; h[0] = (uint32_t)x; h[1] = (uint32_t)(x >> 32);
  return got < 0 ? -1 : 0;
}
/* the app's cached image, if it's this source's (len, hash), loaded: 0; else -1 (nothing loaded, or a
   failed load: the vm needs starting again) or -2 (no image of it: nothing touched). Its other files
   are checked too (into scratch, the app space, which is free: the main source is at its top), unless
   trusted (checked already) */
static int rinc_cacheLoad(rio_Vm *vm, const char *name, uint32_t len, const uint32_t *hash, uint8_t *scratch, uint32_t room, int trusted) {
  char path[96], file[64]; uint8_t want[rinc_CACHE_HEAD], head[rinc_CACHE_HEAD]; uint32_t imageLen, files, i, at = 0, w[3], flen, fh[2]; int res;
  if (!rinc_fs || !rinc_cachePath(name, path, sizeof path) || rinc_fs->open(path, 0)) return -2;
  rinc_srcCount = 0;
  rinc_cacheHead(want, len, hash, 0);
  if (rinc_readAll(0, head, sizeof head) || memcmp(head, want, rinc_CACHE_HEAD - 8)) { rinc_fs->close(); return -2; }
  memcpy(&imageLen, head + rinc_CACHE_HEAD - 8, 4);
  memcpy(&files, head + rinc_CACHE_HEAD - 4, 4);
  for (i = 0; i < files; i++) { /* (each other file: noted, in scratch) */
    uint8_t n;
    if (at + 12 + 64 > room || rinc_readAll(0, scratch + at, 12) || rinc_readAll(0, &n, 1) || n > 51 || rinc_readAll(0, scratch + at + 12, (uint32_t)((n + 4) & ~3) - 1)) { rinc_fs->close(); return -2; }
    scratch[at + 12 + n] = 0;
    at += 12 + 64;
  }
  if (!trusted && files) { /* (checked, each against what's there now: one file open at a time) */
    rinc_fs->close();
    for (i = 0; i < files; i++) {
      memcpy(w, scratch + i * 76, 12);
      snprintf(file, sizeof file, "/rinc/%s", (char *)scratch + i * 76 + 12);
      if (rinc_hashFile(file, &flen, fh) || flen != w[0] || fh[0] != w[1] || fh[1] != w[2]) return -2;
    }
    if (rinc_fs->open(path, 0) || rinc_readAll(0, head, sizeof head)) return -2;
    for (i = 0; i < files; i++) { uint8_t n; if (rinc_readAll(0, scratch, 13) || rinc_readAll(0, scratch, (uint32_t)((scratch[12] + 4) & ~3) - 1)) { rinc_fs->close(); return -2; } (void)n; }
  }
  res = rio_readImage(vm, rinc_readAll, 0, imageLen);
  rinc_fs->close();
  return res ? -1 : 0;
}
/* the app just compiled, its image cached: or none (a half-written one taken away) */
static void rinc_cacheSave(rio_Vm *vm, const char *name, uint32_t len, const uint32_t *hash) {
#ifdef rio_NO_COMPILER
  (void)vm; (void)name; (void)len; (void)hash;
#else
  char path[96]; uint8_t head[rinc_CACHE_HEAD], entry[64]; uint32_t imageLen = rio_saveImage(vm, 0, 0); int bad, i;
  for (i = 0; i < rinc_srcCount; i++) /* (each other file: read to its end, hashed, and its path fits) */
    if (rinc_srcFiles[i].host || !rinc_srcFiles[i].ended || strlen(rinc_srcFiles[i].path + 6) > 51) rinc_srcLost = 1;
  if (!rinc_fs || !imageLen || rinc_srcLost || !rinc_cachePath(name, path, sizeof path) || rinc_fs->open(path, 1)) return; /* (lost: not all its files noted, so none) */
  rinc_cacheHead(head, len, hash, imageLen);
  bad = rinc_writeAll(0, head, sizeof head);
  for (i = 0; i < rinc_srcCount && !bad; i++) { /* (the other files read: /rinc/ left off) */
    const rinc_Reader *r = &rinc_srcFiles[i]; uint32_t w[3]; uint8_t n = (uint8_t)strlen(r->path + 6);
    w[0] = r->hashed; w[1] = (uint32_t)r->x; w[2] = (uint32_t)(r->x >> 32);
    memset(entry, 0, sizeof entry); entry[0] = n; memcpy(entry + 1, r->path + 6, n);
    bad = rinc_writeAll(0, w, sizeof w) || rinc_writeAll(0, entry, (uint32_t)((n + 4) & ~3));
  }
  bad = bad || rio_writeImage(vm, rinc_writeAll, 0) != imageLen;
  if (rinc_fs->close() || bad) rinc_fs->remove(path);
#endif
}
static void rinc_checkedAdd(const char *name, uint32_t len, const uint32_t *hash) {
  int i;
  for (i = 0; i < rinc_checkedCount && strcmp(rinc_checked[i].name, name); i++) {}
  if (i == rinc_checkedCount) {
    if (rinc_checkedCount < (int)(sizeof rinc_checked / sizeof *rinc_checked)) rinc_checkedCount++;
    else { i = rinc_checkedNext; rinc_checkedNext = (rinc_checkedNext + 1) % rinc_checkedCount; }
  }
  snprintf(rinc_checked[i].name, sizeof rinc_checked[i].name, "%s", name);
  rinc_checked[i].len = len; rinc_checked[i].hash[0] = hash[0]; rinc_checked[i].hash[1] = hash[1];
}

int rinc_appStart(rio_Vm *vm, void *space, uint32_t size, const char *name) {
  char path[96]; const char *src = 0; const rinc_App *b = 0;
  uint32_t hash[2], len = 0; int i, res, stored = 0;
#ifndef rio_NO_COMPILER
  rinc_hostFolder[0] = 0; /* (an app from storage or the bundle: its files from there, not the host's, after an app from the host's) */
  if (strchr(name, '/')) { /* (app/file: its app's files beside it; compiled each time, not cached) */
    if (!rinc_runPath(name, rinc_loadApp, sizeof rinc_loadApp, path, sizeof path) || rinc_readFile(path, 0, 0, &len)) { rinc_appFail(name, ": no file of this name in its app"); return -1; }
    return rinc_appLoad(vm, space, size, name, 0, 0, 0, path, 0) || rinc_appGo(vm, name);
  }
#endif
  snprintf(rinc_loadApp, sizeof rinc_loadApp, "%s", name); /* (its own files: in its folder) */
  if (rinc_cacheTrust)
    for (i = 0; i < rinc_checkedCount; i++)
      if (!strcmp(rinc_checked[i].name, name)) {
        if (!(res = rinc_cacheLoad(vm, name, rinc_checked[i].len, rinc_checked[i].hash, (uint8_t *)space, size, 1))) return rinc_appGo(vm, name);
        if (res == -1 && rinc_start(vm, space, size, rinc_put)) { rinc_appFail(name, ": can't start rio"); return -1; }
        break;
      }
#ifndef rio_NO_COMPILER /* (no compiler: bundled images only) */
  stored = rinc_appPath(name, path, sizeof path) && !rinc_hashFile(path, &len, hash); /* (its source: read through once, to tell its image) */
#endif
  if (!stored && !(b = rinc_bundled(name))) { rinc_appFail(name, ": no app of this name"); return -1; }
  if (b && b->image) return rinc_appRun(vm, space, size, name, b->data, b->len, b->data);
  if (b) { src = b->data; len = rinc_appLen(b); rinc_hash(src, len, hash); }
  if (!(res = rinc_cacheLoad(vm, name, len, hash, (uint8_t *)space, size, 0))) {
    rinc_checkedAdd(name, len, hash);
    return rinc_appGo(vm, name);
  }
  if (res == -1 && rinc_start(vm, space, size, rinc_put)) { rinc_appFail(name, ": can't start rio"); return -1; } /* (a load that failed part way) */
  if (rinc_appLoad(vm, space, size, name, src, len, 0, b ? 0 : path, 0)) return -1;
  if (!b) { /* (its source changed while it compiled: no image of it) */
    const rinc_Reader *m = &rinc_srcFiles[rinc_SRC_FILES];
    if (!m->ended || m->hashed != len || (uint32_t)m->x != hash[0] || (uint32_t)(m->x >> 32) != hash[1]) return rinc_appGo(vm, name);
  }
  rinc_cacheSave(vm, name, len, hash);
  rinc_checkedAdd(name, len, hash);
  return rinc_appGo(vm, name);
}
int rinc_gameFrame(rio_Vm *vm) {
  static uint16_t wasButtons;
  int res;
  float t0;
  uint16_t pressed = (uint16_t)(rinc_buttons & ~wasButtons);
  wasButtons = rinc_buttons;
  if (pressed & rinc_Button_rb) rinc_textKey(9); /* (a controller's RB and LB type Tab and Shift+Tab too) */
  if (pressed & rinc_Button_lb) rinc_textKey(9 | rinc_Mod_shift);
  rinc_pointerFrame();
  rinc_logTick();
  if (!rinc_game) return 1;
  t0 = rinc_time ? rinc_time() : 0;
  res = rinc_frame(vm);
  rinc_perfNote(rinc_time ? (rinc_time() - t0) * 1000.0f : 0); /* (for the performance strip) */
  if (res != 0) { /* an error, or no update or draw any more */
    rinc_game = 0; rinc_showLog = 1; rinc_audioReset(); rinc_consoleClear(rinc_themeColor(0));
    if (res < 0) rinc_keepError(vm, rinc_appRef);
    rinc_say(res < 0 ? rio_error(vm) : "nothing to run: define update* or draw*");
    rinc_back();
  }
  return res;
}

/* the command list (docs/spec/apps.md#its-commands): the app's (its commandList*, pc.Command each), then
   Rinc's own, narrowed by what's typed; a pick types its key for the app (or does what the system key
   would). Up and down (and the d-pad, Page Up and Down) move, Enter picks (a controller's A), Esc
   closes (B), a click picks one, and one outside closes it. While it's up, the app waits */
typedef struct { rio_View label, keys; int32_t key; } rinc_Command; /* (pc.Command's layout) */
typedef char rinc_Command_size[sizeof(rinc_Command) == 20 ? 1 : -1];
static const struct { const char *label, *keys; int bit; } rinc_sysCommands[] = {
  {"Home", "Sys+Esc", rinc_SysKey_esc}, {"Switch apps", "Sys+Tab", rinc_SysKey_tab}, {"This app's help", "Sys+H", rinc_SysKey_h},
  {"This app's log", "Sys+O", rinc_SysKey_o}, {"Back to the app before", "Sys+Q", rinc_SysKey_q},
  {"This app again, from the start", "Sys+R", rinc_SysKey_r}, {"Performance strip, shown or not", "Sys+M", rinc_SysKey_m},
  {"Sleep: the display rests", "Sys+L", rinc_SysKey_l}, {"The whole display, or not", "Sys+Enter", rinc_SysKey_enter}};
#define rinc_SYS_COMMANDS ((int)(sizeof rinc_sysCommands / sizeof rinc_sysCommands[0]))
#define rinc_APP_COMMANDS 56
#define rinc_CMD_ROWS 16
/* the app's, as it gave them: its text stays where it is in the app's memory (the app waits, so it's as
   it was), Rinc's own after them */
static rinc_Command rinc_cmdApp[rinc_APP_COMMANDS];
static rio_Vm *rinc_cmdVm;
static uint8_t rinc_cmdShown[rinc_APP_COMMANDS + rinc_SYS_COMMANDS];
static int rinc_cmdAppCount, rinc_cmdCount, rinc_cmdOn, rinc_cmdNShown, rinc_cmdPick, rinc_cmdFilterLen, rinc_cmdButtons, rinc_cmdRepeat, rinc_cmdClicked;
static char rinc_cmdFilter[24];
/* command i's label or keys (keys: 1), as text and its length, as much as shows (most) */
static const char *rinc_cmdText(int i, int keys, int most, int *len) {
  const char *s; int n;
  if (i < rinc_cmdAppCount) {
    rio_View v = keys ? rinc_cmdApp[i].keys : rinc_cmdApp[i].label;
    s = (const char *)rio_viewBytes(rinc_cmdVm, v); n = s ? (int)v.len : 0;
  } else {
    s = keys ? rinc_sysCommands[i - rinc_cmdAppCount].keys : rinc_sysCommands[i - rinc_cmdAppCount].label; n = (int)strlen(s);
  }
  *len = n < most ? n : most;
  return s;
}
static int rinc_cmdKeyOf(int i) { return i < rinc_cmdAppCount ? (rinc_cmdApp[i].key < 0 ? 0 : rinc_cmdApp[i].key) : -rinc_sysCommands[i - rinc_cmdAppCount].bit; }
static int rinc_cmdHas(int i) { /* (what's typed, anywhere in its label, any case) */
  int n, k, j; const char *s = rinc_cmdText(i, 0, 64, &n);
  for (j = 0; j + rinc_cmdFilterLen <= n; j++) {
    for (k = 0; k < rinc_cmdFilterLen && tolower((unsigned char)s[j + k]) == tolower((unsigned char)rinc_cmdFilter[k]); k++) {}
    if (k == rinc_cmdFilterLen) return 1;
  }
  return 0;
}
static void rinc_cmdNarrow(void) {
  int i;
  rinc_cmdNShown = 0;
  for (i = 0; i < rinc_cmdCount; i++) if (rinc_cmdHas(i)) rinc_cmdShown[rinc_cmdNShown++] = (uint8_t)i;
  if (rinc_cmdPick >= rinc_cmdNShown) rinc_cmdPick = rinc_cmdNShown - 1;
  if (rinc_cmdPick < 0) rinc_cmdPick = 0;
}
static int rinc_cmdOpen(rio_Vm *vm) { /* (the app's, asked now: -1 if that stopped with an error) */
  int fn = rio_findProc(vm, "commandList");
  rinc_cmdAppCount = 0; rinc_cmdVm = vm;
  if (fn >= 0) {
    rio_Val *r; uint32_t at, n;
    if (rio_call(vm, fn)) return -1;
    r = rio_procResult(vm, fn); at = r[0].u; n = r[1].u;
    if (n > rinc_APP_COMMANDS) n = rinc_APP_COMMANDS;
    if (at <= vm->memSize && n * sizeof(rinc_Command) <= vm->memSize - at) {
      memcpy(rinc_cmdApp, vm->mem + at, n * sizeof(rinc_Command));
      rinc_cmdAppCount = (int)n;
    }
  }
  rinc_cmdCount = rinc_cmdAppCount + rinc_SYS_COMMANDS;
  rinc_cmdFilterLen = rinc_cmdPick = 0;
  rinc_cmdNarrow();
  rinc_cmdButtons = rinc_buttons; rinc_cmdClicked = rinc_pointerHeld; /* (held already: not presses) */
  rinc_cmdOn = 1;
  return 0;
}
static void rinc_cmdRun(int i) { /* (a pick: its key typed for the app, or the system key's) */
  int key = rinc_cmdKeyOf(i);
  rinc_cmdOn = 0; rinc_cmdSettle = 1;
  if (key > 0) { /* (first, before anything typed after it; as it is: no lock adds to it) */
    int before = (rinc_typed.tail + rinc_TEXT_CAP - 1) % rinc_TEXT_CAP;
    if (before != rinc_typed.head) { rinc_typed.q[before] = (uint16_t)key; rinc_typed.tail = before; }
  } else if (key < 0 && rinc_sysDo(-key, 0, 0)) rinc_launch = 1;
}
static void rinc_cmdMove(int by) {
  rinc_cmdPick += by;
  if (rinc_cmdPick >= rinc_cmdNShown) rinc_cmdPick = rinc_cmdNShown - 1;
  if (rinc_cmdPick < 0) rinc_cmdPick = 0;
}
static void rinc_cmdKey(int c) {
  int k = c & 255;
  if (c == 27) rinc_cmdOn = 0, rinc_cmdSettle = 1;
  else if (k == 13) { if (rinc_cmdNShown) rinc_cmdRun(rinc_cmdShown[rinc_cmdPick]); }
  else if (k == 8) { if (rinc_cmdFilterLen) rinc_cmdFilterLen--; rinc_cmdNarrow(); }
  else if (c >= 32 && c < 127) { if (rinc_cmdFilterLen < (int)sizeof rinc_cmdFilter - 1) rinc_cmdFilter[rinc_cmdFilterLen++] = (char)c; rinc_cmdPick = 0; rinc_cmdNarrow(); }
  else if (c < 1024 && k == 130 && !(rinc_buttons & rinc_Button_up)) rinc_cmdMove(-1); /* (an arrow typed, not held: held, it's a button, below) */
  else if (c < 1024 && k == 131 && !(rinc_buttons & rinc_Button_down)) rinc_cmdMove(1);
  else if (k == 9) rinc_cmdMove(c & 256 ? -1 : 1);
  else if (k == 135) rinc_cmdMove(-(rinc_CMD_ROWS - 1));
  else if (k == 136) rinc_cmdMove(rinc_CMD_ROWS - 1);
  else if (k == 132) rinc_cmdMove(-rinc_cmdNShown);
  else if (k == 133) rinc_cmdMove(rinc_cmdNShown);
}
static void rinc_cmdBox(int x, int y, int w, int h, int c) {
  int j;
  for (j = y; j < y + h && j < rinc_H; j++) if (j >= 0) memset(&rinc_screen[j][x], c, (size_t)w);
}
static void rinc_cmdPrint(const char *s, int n, int x, int y, int c) { /* (n characters of s: what isn't text, as ?) */
  for (; n > 0; s++, n--, x += 6) rinc_char(*s >= 32 && *s < 127 ? (unsigned char)*s : '?', x, y, c, -1);
}
#define rinc_CMD_X 30
#define rinc_CMD_Y 20
#define rinc_CMD_W 260
static void rinc_cmdFrame(void) {
  int c, i, step = 0, from, held = rinc_buttons & (rinc_Button_up | rinc_Button_down), click = rinc_pointerHeld & ~rinc_cmdClicked & 1;
  while (rinc_cmdOn && (c = rinc_dequeue(&rinc_typed)) >= 0) rinc_cmdKey(c);
  if (!rinc_cmdOn) return;
  /* up and down held (the d-pad's too): a step a press, then more while it's held */
  if (held != (rinc_cmdButtons & (rinc_Button_up | rinc_Button_down))) { rinc_cmdRepeat = 15; step = held ? 1 : 0; }
  else if (held && --rinc_cmdRepeat == 0) { rinc_cmdRepeat = 4; step = 1; }
  if (step) rinc_cmdMove(held & rinc_Button_up ? -1 : 1);
  rinc_cmdButtons = rinc_buttons;
  rinc_cmdClicked = rinc_pointerHeld;
  from = rinc_cmdPick - rinc_CMD_ROWS + 1 > 0 ? rinc_cmdPick - rinc_CMD_ROWS + 1 : 0;
  if (click) { /* (a row: picked; outside the box: closed) */
    int px = (int)rinc_pointerX, py = (int)rinc_pointerY, row = (py - (rinc_CMD_Y + 32)) / 10;
    if (px < rinc_CMD_X || px >= rinc_CMD_X + rinc_CMD_W || py < rinc_CMD_Y || py >= rinc_CMD_Y + 36 + rinc_CMD_ROWS * 10) { rinc_cmdOn = 0; rinc_cmdSettle = 1; return; }
    if (py >= rinc_CMD_Y + 32 && from + row < rinc_cmdNShown) { rinc_cmdRun(rinc_cmdShown[from + row]); return; }
  }
  { /* drawn over the app's screen, as it was */
    int bar = rinc_themeColor(1), text = rinc_themeColor(2), soft = rinc_themeColor(3), dim = rinc_themeColor(4), accent = rinc_themeColor(6), pick = rinc_themeColor(8), back = rinc_themeColor(0);
    rinc_cmdBox(rinc_CMD_X, rinc_CMD_Y, rinc_CMD_W, 36 + rinc_CMD_ROWS * 10, bar);
    rinc_cmdPrint("Commands (type to narrow):", 26, rinc_CMD_X + 6, rinc_CMD_Y + 4, accent);
    rinc_cmdBox(rinc_CMD_X + 4, rinc_CMD_Y + 14, rinc_CMD_W - 8, 11, back);
    rinc_cmdPrint(rinc_cmdFilter, rinc_cmdFilterLen, rinc_CMD_X + 6, rinc_CMD_Y + 16, text);
    rinc_cmdBox(rinc_CMD_X + 6 + rinc_cmdFilterLen * 6, rinc_CMD_Y + 15, 1, 9, accent);
    for (i = 0; i < rinc_CMD_ROWS && from + i < rinc_cmdNShown; i++) {
      int k = rinc_cmdShown[from + i], y = rinc_CMD_Y + 32 + i * 10, kn, ln, on = from + i == rinc_cmdPick;
      const char *keys = rinc_cmdText(k, 1, 11, &kn), *label = rinc_cmdText(k, 0, (rinc_CMD_W - 16 - (kn ? kn * 6 + 6 : 0)) / 6, &ln);
      if (on) rinc_cmdBox(rinc_CMD_X + 4, y - 2, rinc_CMD_W - 8, 10, pick);
      rinc_cmdPrint(label, ln, rinc_CMD_X + 8, y, on ? text : soft);
      rinc_cmdPrint(keys, kn, rinc_CMD_X + rinc_CMD_W - 8 - kn * 6, y, on ? text : dim);
    }
    if (!rinc_cmdNShown) rinc_cmdPrint("No such command", 15, rinc_CMD_X + 8, rinc_CMD_Y + 32, rinc_themeColor(5));
  }
}

int rinc_frame(rio_Vm *vm) {
  int u = rio_findProc(vm, "update"), d = rio_findProc(vm, "draw");
  /* the left stick pushed well over also counts as the arrows, as on PICO-8 (but not in pointer mode:
     there it moves the pointer) */
  if (rinc_pointerMode != 1) {
    if (rinc_axes[0] < -16000) rinc_buttons |= rinc_Button_left;
    if (rinc_axes[0] > 16000) rinc_buttons |= rinc_Button_right;
    if (rinc_axes[1] < -16000) rinc_buttons |= rinc_Button_up;
    if (rinc_axes[1] > 16000) rinc_buttons |= rinc_Button_down;
  }
  if (u < 0 && d < 0) return 1;
  if (vm != &rinc_nest) { /* (the command list: while it's up, the app waits) */
    if (rinc_cmdWanted) { rinc_cmdWanted = 0; if (rinc_cmdOpen(vm)) return -1; }
    if (rinc_cmdOn) { rinc_cmdFrame(); return 0; }
    if (rinc_cmdSettle && !rinc_buttons && !rinc_pointerHeld) rinc_cmdSettle = 0;
  }
  if (u >= 0 && rio_call(vm, u)) return -1;
  if (d >= 0 && rio_call(vm, d)) return -1;
  return 0;
}

#ifndef rio_NO_COMPILER /* (a vm that only runs images has no REPL) */
/* the REPL: lines gather until rio has a complete input; a blank line gives up on an unfinished one.
   The line being typed can be edited anywhere: left and right, Home and End (or Ctrl-A, Ctrl-E),
   Backspace and Delete, and up and down go back through the lines typed before. It's in a little
   color as it's typed: keywords, constants (numbers and strings), comments. Each next line of an unfinished input starts
   indented (two spaces a step, as rio_replIndent says), end, else and case go a step back out once
   typed, and Backspace in the indent takes a step.
   Keys arrive as a terminal sends them (ESC [ A and so on: the keyboard sends the same), and the echo
   is in terminal escapes too, which the console understands, so a serial terminal and the screen show
   the same. Typing at the end of the line, the usual case (and all of a pasted app), sends little
   more than what's typed: a word takes its color as soon as it is one (the end of the line ends it,
   for now), recolored as it changes. Edits elsewhere redraw the line. */
/* code's colors, the theme's (pc.Role: keyword, number, string, comment), as palette escapes: a buffer
   each, so a word's color can be told by its pointer (r->wp) */
static const char *rinc_paintOf(int role) {
  static char paints[4][16]; char *p = paints[role == 16 ? 0 : role == 19 ? 1 : role == 20 ? 2 : 3];
  snprintf(p, 16, "\033[38;5;%dm", rinc_themeColor(role));
  return p;
}
#define rinc_PAINT_KEYWORD rinc_paintOf(16)
#define rinc_PAINT_NUMBER rinc_paintOf(19)
#define rinc_PAINT_STRING rinc_paintOf(20)
#define rinc_PAINT_COMMENT rinc_paintOf(21)
#define rinc_PAINT_PLAIN "\033[0m"
enum { rinc_Mode_plain, rinc_Mode_string, rinc_Mode_comment };
static void rinc_puts(rinc_Repl *r, const char *s) { r->put(s, (int)strlen(s)); }
static void rinc_prompt(rinc_Repl *r) { if (!r->loading) r->put(r->n ? ". " : "> ", 2); } /* (as wide: lines line up) */
static int rinc_blank(const rinc_Repl *r) { int i; for (i = r->line0; i < r->n; i++) if (r->acc[i] != ' ') return 0; return 1; }
static int rinc_wordChar(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'; }
/* a word's color, or 0 for none (types too: plain) */
static const char *rinc_wordPaint(const char *w, int n) {
  static const char *const keywords[] = {"proc", "struct", "if", "else", "for", "in", "end", "return", "break", "continue",
    "import", "include", "enum", "union", "switch", "case", "is", "nil", "and", "or", "not", "do", "true", "false"};
  size_t i;
  if (n > 0 && w[0] >= '0' && w[0] <= '9') return rinc_PAINT_NUMBER;
  for (i = 0; i < sizeof keywords / sizeof keywords[0]; i++) if ((int)strlen(keywords[i]) == n && !memcmp(keywords[i], w, (size_t)n)) return rinc_PAINT_KEYWORD;
  return 0;
}
static void rinc_moveLeft(rinc_Repl *r, int k) { char b[16]; if (k > 0) r->put(b, snprintf(b, sizeof b, "\033[%dD", k)); }
/* the line from its start, in color, leaving the paint state as at its end */
static void rinc_paintLine(rinc_Repl *r) {
  int i = r->line0, j;
  rinc_puts(r, rinc_PAINT_PLAIN); /* (from whatever color the cursor was last in: a comment's, say) */
  r->mode = rinc_Mode_plain; r->word = -1; r->numdot = -1; /* (a string ends with its line: none carries over) */
  while (i < r->n) {
    char c = r->acc[i];
    if (r->mode == rinc_Mode_string) {
      for (j = i; j < r->n && r->acc[j] != '"'; j++) if (r->acc[j] == '\\' && j + 1 < r->n) j++;
      if (j < r->n) { r->put(r->acc + i, j + 1 - i); rinc_puts(r, rinc_PAINT_PLAIN); r->mode = rinc_Mode_plain; i = j + 1; }
      else { r->put(r->acc + i, r->n - i); i = r->n; }
    } else if (c == '#') { rinc_puts(r, rinc_PAINT_COMMENT); r->put(r->acc + i, r->n - i); r->mode = rinc_Mode_comment; i = r->n; }
    else if (c == '"') { rinc_puts(r, rinc_PAINT_STRING); r->put(&c, 1); r->mode = rinc_Mode_string; i++; }
    else if (rinc_wordChar(c)) {
      const char *p;
      int dot = 0; /* (a number's one dot, if a digit follows it: 1.5, but 0..<n) */
      for (j = i; j < r->n && (rinc_wordChar(r->acc[j]) || (r->acc[i] >= '0' && r->acc[i] <= '9' && r->acc[j] == '.' && !dot++ &&
                               j + 1 < r->n && r->acc[j + 1] >= '0' && r->acc[j + 1] <= '9')); j++) {}
      if ((p = rinc_wordPaint(r->acc + i, j - i))) { rinc_puts(r, p); r->put(r->acc + i, j - i); rinc_puts(r, rinc_PAINT_PLAIN); }
      else r->put(r->acc + i, j - i);
      if (j == r->n) { r->word = i; r->wp = p; r->numdot = -1; } /* (the last word: still being typed) */
      i = j;
    } else { r->put(&c, 1); i++; }
  }
}
/* redraw the line: back to its start, paint it, blank what's left of the old one, and the cursor back */
static void rinc_redraw(rinc_Repl *r) {
  if (!r->echo) return;
  rinc_moveLeft(r, r->scr);
  rinc_paintLine(r); rinc_puts(r, "\033[J");
  rinc_moveLeft(r, r->n - r->cur); r->scr = r->cur - r->line0;
}
/* the word at the end of the line ended (it's already in its color) */
static void rinc_finishWord(rinc_Repl *r) { r->word = -1; r->wp = 0; r->numdot = -1; }
static void rinc_appendTail(rinc_Repl *r, char c);
/* c typed at the end of the line */
static void rinc_append(rinc_Repl *r, char c) {
  r->acc[r->n++] = c; r->cur = r->n;
  if (!r->echo) return;
  r->scr = r->cur - r->line0;
  if (r->mode == rinc_Mode_comment) r->put(&c, 1);
  else if (r->mode == rinc_Mode_string) {
    int k = 0, i; /* (a quote ends it unless an odd number of backslashes come before) */
    for (i = r->n - 2; i >= r->line0 && r->acc[i] == '\\'; i--) k++;
    r->put(&c, 1);
    if (c == '"' && !(k & 1)) { rinc_puts(r, rinc_PAINT_PLAIN); r->mode = rinc_Mode_plain; }
  } else if (r->numdot >= 0 && r->numdot == r->n - 2 && !(c >= '0' && c <= '9')) { /* (a number's dot, but no digit after: not its dot) */
    rinc_moveLeft(r, 1); rinc_puts(r, rinc_PAINT_PLAIN "."); rinc_finishWord(r);
    r->n--; rinc_appendTail(r, c);
  } else if (rinc_wordChar(c) || (c == '.' && r->word >= 0 && r->acc[r->word] >= '0' && r->acc[r->word] <= '9' && r->numdot < r->word)) {
    const char *p;
    if (c == '.') r->numdot = r->n - 1; /* (for now: if a digit comes next, it's the number's) */
    if (r->word < 0) { r->word = r->n - 1; r->wp = 0; }
    p = rinc_wordPaint(r->acc + r->word, r->n - r->word);
    if (p != r->wp) { /* (its color changed: the whole word again) */
      rinc_moveLeft(r, r->n - 1 - r->word);
      if (p) rinc_puts(r, p);
      r->put(r->acc + r->word, r->n - r->word);
      if (p) rinc_puts(r, rinc_PAINT_PLAIN);
      r->wp = p;
    } else if (p) { rinc_puts(r, p); r->put(&c, 1); rinc_puts(r, rinc_PAINT_PLAIN); }
    else r->put(&c, 1);
  } else {
    r->n--; rinc_finishWord(r); r->n++; /* (the word ended before c) */
    if (c == '"') { rinc_puts(r, rinc_PAINT_STRING); r->mode = rinc_Mode_string; }
    else if (c == '#') { rinc_puts(r, rinc_PAINT_COMMENT); r->mode = rinc_Mode_comment; }
    r->put(&c, 1);
  }
}
static void rinc_appendTail(rinc_Repl *r, char c) { rinc_append(r, c); } /* (append again, after taking back a dot) */
/* the next line's indent, two spaces a step. Spaces typed at its start take its place first (so
   typed or pasted indenting doesn't double it) */
static void rinc_indent(rinc_Repl *r) {
  int k = rio_replIndent(r->acc, (uint32_t)r->n) * 2;
  r->autoind = 0;
  while (k-- > 0 && r->n < (int)sizeof r->acc - 2) { rinc_append(r, ' '); r->autoind++; }
}
/* the line so far is its indent and one word: end, else or case go a step out (if the indent is still
   all there: not when Backspace already took the step) */
static void rinc_outdent(rinc_Repl *r) {
  int s = r->line0, w, i;
  if (r->cur != r->n) return;
  while (s < r->n && r->acc[s] == ' ') s++;
  w = r->n - s;
  for (i = s; i < r->n; i++) if (r->acc[i] == ' ') return;
  if (s - r->line0 < 2 || s - r->line0 < rio_replIndent(r->acc, (uint32_t)r->line0) * 2) return;
  if (!w || !rio_replOutdent(r->acc + s, (uint32_t)w)) return;
  memmove(r->acc + s - 2, r->acc + s, (size_t)w); r->n -= 2; r->cur = r->n;
  rinc_redraw(r);
}
/* the line becomes text (from history): replacing what's typed on it */
static void rinc_setLine(rinc_Repl *r, const char *text, int n) {
  if (n > (int)sizeof r->acc - 2 - r->line0) n = (int)sizeof r->acc - 2 - r->line0;
  memcpy(r->acc + r->line0, text, (size_t)n); r->n = r->cur = r->line0 + n;
  rinc_redraw(r);
}
/* history: past lines, oldest first, each ending in \n; back counts how far up the list we've gone */
static void rinc_remember(rinc_Repl *r) {
  int n = r->n - r->line0, last;
  if (r->loading || rinc_blank(r) || n + 1 > (int)sizeof r->hist) return;
  for (last = r->hlen - 1; last > 0 && r->hist[last - 1] != '\n'; last--) {} /* (the newest, to skip repeats) */
  if (r->hlen && r->hlen - 1 - last == n && !memcmp(r->hist + last, r->acc + r->line0, (size_t)n)) return;
  while (r->hlen + n + 1 > (int)sizeof r->hist) { /* (room: the oldest go) */
    int k = 0; while (k < r->hlen && r->hist[k] != '\n') k++;
    memmove(r->hist, r->hist + k + 1, (size_t)(r->hlen - k - 1)); r->hlen -= k + 1;
  }
  memcpy(r->hist + r->hlen, r->acc + r->line0, (size_t)n); r->hlen += n; r->hist[r->hlen++] = '\n';
}
static void rinc_recall(rinc_Repl *r, int dir) { /* dir 1: older, -1: newer */
  int count = 0, i, k, back = r->back + dir, end;
  for (i = 0; i < r->hlen; i++) count += r->hist[i] == '\n';
  if (back < 0 || back > count) return;
  if (r->back == 0) { r->draftn = r->n - r->line0; if (r->draftn > (int)sizeof r->draft) r->draftn = (int)sizeof r->draft; memcpy(r->draft, r->acc + r->line0, (size_t)r->draftn); }
  r->back = back;
  if (!back) { rinc_setLine(r, r->draft, r->draftn); return; }
  for (end = r->hlen, k = 0; end > 0; ) { /* the back-th line from the end */
    int start = end - 1; while (start > 0 && r->hist[start - 1] != '\n') start--;
    if (++k == back) { rinc_setLine(r, r->hist + start, end - 1 - start); return; }
    end = start;
  }
}

int rinc_replBegin(rinc_Repl *r, rio_Vm *vm, void *scratch, uint32_t size) {
  memset(r, 0, sizeof *r);
  r->vm = vm; r->put = rinc_put; r->echo = 1; r->word = -1; r->numdot = -1;
  if (rio_replBegin(vm, scratch, size, 0)) return -1;
  r->put("Rinc: rio REPL\r\n", 16);
  rinc_prompt(r);
  return 0;
}

void rinc_replStop(rinc_Repl *r, const char *why) {
  r->run = 0;
  rinc_audioReset(); /* a game's sounds stop with it */
  if (why) { r->put(why, (int)strlen(why)); r->put("\r\n", 2); }
  rinc_prompt(r);
}

/* the commands for apps on storage */
static void rinc_replSay(rinc_Repl *r, const char *s) { r->put(s, (int)strlen(s)); }
static int rinc_replAppPath(char *path, size_t cap, const char *name, int n) { /* (an app's, or a project's: chord+) */
  char ref[rinc_REF];
  if (n < 1 || n >= (int)sizeof ref) return 0;
  memcpy(ref, name, (size_t)n); ref[n] = 0;
  return rinc_appPath(ref, path, cap);
}

/* source, fed in as if typed, into a fresh session: from storage (src NULL) or from memory */
static int rinc_feed(rinc_Repl *r, const char *src, int len) {
  char buf[256]; int got, i, echo = r->echo, at = 0;
  if (r->restart && r->restart()) { rinc_replSay(r, "can't start a fresh session\r\n"); return -1; }
  r->loading = 1; r->errors = 0; r->echo = 0; r->n = 0; r->line0 = r->cur = r->scr = 0; r->run = 0;
  r->esc = 0; r->saving = 0; r->hex = 0; /* (nothing half typed carries over: an Esc that began a key's escape, say) */
  for (;;) {
    if (src) { got = len - at < (int)sizeof buf ? len - at : (int)sizeof buf; memcpy(buf, src + at, (size_t)got); at += got; }
    else got = rinc_fs->read(buf, sizeof buf);
    if (got <= 0) break;
    for (i = 0; i < got; i++) rinc_replChar(r, buf[i] == '\n' ? '\r' : buf[i] == '\r' ? -1 : (unsigned char)buf[i]);
  }
  if (r->n) rinc_replChar(r, '\r'); /* (an unfinished last line) */
  r->loading = 0; r->echo = echo; r->n = r->line0 = r->cur = r->scr = 0; r->run = 0;
  return r->errors;
}
static void rinc_load(rinc_Repl *r, const char *name, int n, int run) {
  char path[96]; int errors;
  if (!rinc_replAppPath(path, sizeof path, name, n)) { rinc_replSay(r, "names are letters, digits, _ and -\r\n"); return; }
  if (!rinc_fs || rinc_fs->open(path, 0)) { rinc_replSay(r, rinc_fs ? "can't open " : "no storage: "); rinc_replSay(r, path); rinc_replSay(r, "\r\n"); return; }
  errors = rinc_feed(r, 0, 0);
  rinc_fs->close();
  if (errors < 0) return;
  rinc_replSay(r, errors ? "loaded, with errors: " : "loaded "); rinc_replSay(r, path); rinc_replSay(r, "\r\n");
  if (run && !errors) r->run = 1;
}
int rinc_replApp(rinc_Repl *r, const char *name, int run) { rinc_load(r, name, (int)strlen(name), run); return r->run ? 0 : -1; }
int rinc_replSource(rinc_Repl *r, const char *src, int len) {
  int errors = rinc_feed(r, src, len);
  if (!errors) r->run = 1;
  return errors ? -1 : 0;
}
static void rinc_saveFlush(rinc_Repl *r) { if (r->nout && rinc_fs->write(r->out, r->nout) < 0) r->saving = -1; r->saved += r->nout; r->nout = 0; }
static void rinc_saveChar(rinc_Repl *r, int c) { /* while saving: everything typed goes to the file, until Ctrl-D */
  if (c == 4 || c == 3) {
    rinc_saveFlush(r);
    rinc_replSay(r, rinc_PAINT_PLAIN "\r\n");
    if (rinc_fs->close() || r->saving < 0) rinc_replSay(r, "couldn't save it all\r\n");
    else if (c == 4) { char b[48]; rinc_replSay(r, "saved "); r->put(b, snprintf(b, sizeof b, "%d bytes\r\n", r->saved)); }
    else rinc_replSay(r, "gave up (the file has what came before)\r\n");
    r->saving = 0; r->hex = 0; rinc_prompt(r); return;
  }
  if (r->hex) { /* (put: two hex digits a byte, anything else skipped) */
    int v = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
    if (v < 0) return;
    if (r->half < 0) { r->half = v; return; }
    r->out[r->nout++] = (char)(r->half << 4 | v); r->half = -1;
    if (r->nout == (int)sizeof r->out) rinc_saveFlush(r);
    return;
  }
  if (c == '\n' && r->cr) { r->cr = 0; return; } /* (\r\n: one line break) */
  r->cr = c == '\r';
  if (c == '\r') c = '\n';
  if (c != '\n' && c != '\t' && (c < 32 || c > 126)) return;
  r->out[r->nout++] = (char)c;
  if (r->nout == (int)sizeof r->out) rinc_saveFlush(r);
  if (r->echo) { if (c == '\n') r->put("\r\n", 2); else { char ch = (char)c; r->put(&ch, 1); } }
}
static void rinc_save(rinc_Repl *r, const char *name, int n) {
  char path[96];
  if (!rinc_replAppPath(path, sizeof path, name, n)) { rinc_replSay(r, "names are letters, digits, _ and -\r\n"); return; }
  if (!rinc_fs || rinc_fs->open(path, 1)) { rinc_replSay(r, rinc_fs ? "can't write " : "no storage: "); rinc_replSay(r, path); rinc_replSay(r, "\r\n"); return; }
  rinc_replSay(r, "saving "); rinc_replSay(r, path); rinc_replSay(r, ": type or paste it, then Ctrl-D (Ctrl-C gives up)\r\n");
  r->saving = 1; r->saved = 0; r->nout = 0; r->cr = 0;
}
/* put path: a file anywhere under /rinc (each part of its path a name, dots allowed, not starting
   with one), its bytes as hex */
static void rinc_putFile(rinc_Repl *r, const char *s, int n) {
  char path[112]; int i = 0, j;
  if (n < 1 || n > 100 || s[n - 1] == '/') i = -1;
  while (i >= 0 && i < n) { for (j = i; j < n && s[j] != '/'; j++) {} i = rinc_nameOk(s + i, j - i, 1) ? j + 1 : -1; }
  if (i < 0) { rinc_replSay(r, "a path under /rinc: names of letters, digits, _, - and ., parted by /\r\n"); return; }
  snprintf(path, sizeof path, "/rinc/%.*s", n, s);
  if (!rinc_fs || rinc_fs->open(path, 1)) { rinc_replSay(r, rinc_fs ? "can't write " : "no storage: "); rinc_replSay(r, path); rinc_replSay(r, "\r\n"); return; }
  rinc_replSay(r, "putting "); rinc_replSay(r, path); rinc_replSay(r, ": hex digits, then Ctrl-D (Ctrl-C gives up)\r\n");
  r->saving = 1; r->hex = 1; r->half = -1; r->saved = 0; r->nout = 0;
}
/* a command line? (only as a whole input of its own) */
static int rinc_command(rinc_Repl *r) {
  const char *s = r->acc; int n = r->n, w;
  if (r->line0 || r->loading) return 0;
  while (n && s[n - 1] == ' ') n--;
  while (n && *s == ' ') { s++; n--; }
  for (w = 0; w < n && s[w] != ' '; w++) {}
  if (w == 4 && n == 4 && !memcmp(s, "home", 4)) { memcpy(rinc_launchName, "home", 5); rinc_launch = 1; return 1; } /* (the platform goes back to it) */
  if (w == 2 && n == 2 && !memcmp(s, "ls", 2)) {
    if (!rinc_fs) rinc_replSay(r, "no storage\r\n");
    else if (rinc_findApps() < 0) rinc_replSay(r, "no apps (or no card): /rinc/apps\r\n");
    else { int i; for (i = 0; i < rinc_appCount; i++) { rinc_replSay(r, rinc_appNames[i]); rinc_replSay(r, "  "); } rinc_replSay(r, rinc_appCount ? "\r\n" : "(none yet)\r\n"); }
    return 1;
  }
  if (n > w + 1 && ((w == 4 && !memcmp(s, "load", 4)) || (w == 3 && !memcmp(s, "run", 3)))) { rinc_load(r, s + w + 1, n - w - 1, w == 3); return 1; }
  if (n > w + 1 && w == 4 && !memcmp(s, "save", 4)) { rinc_save(r, s + w + 1, n - w - 1); return 1; }
  if (n > w + 1 && w == 3 && !memcmp(s, "put", 3)) { rinc_putFile(r, s + w + 1, n - w - 1); return 1; }
  if (n > w + 1 && w == 7 && !memcmp(s, "install", 7)) { /* project name+ as the app name */
    char name[33], from[64], to[64]; int k = n - w - 1;
    if (!rinc_nameOk(s + w + 1, k, 0)) { rinc_replSay(r, "names are letters, digits, _ and -\r\n"); return 1; }
    memcpy(name, s + w + 1, (size_t)k); name[k] = 0;
    snprintf(from, sizeof from, "/rinc/projects/%s", name); snprintf(to, sizeof to, "/rinc/apps/%s", name);
    rinc_replSay(r, rinc_copyTree(from, to, "save", 0) ? "couldn't install " : "installed ");
    r->put(name, k); rinc_replSay(r, "\r\n");
    return 1;
  }
  if (n > w + 1 && w == 6 && !memcmp(s, "delete", 6)) {
    char path[96]; const char *name = s + w + 1; int k = n - w - 1;
    if (!rinc_replAppPath(path, sizeof path, name, k)) rinc_replSay(r, "names are letters, digits, _ and -\r\n");
    else { char ref[rinc_REF]; snprintf(ref, sizeof ref, "%.*s", k, name);
      if (rinc_appRemove(ref)) { rinc_replSay(r, "can't delete "); rinc_replSay(r, path); rinc_replSay(r, "\r\n"); return 1; } rinc_replSay(r, "deleted "); r->put(name, k); rinc_replSay(r, "\r\n"); } /* (all of it) */
    return 1;
  }
  return 0;
}

static void rinc_newLine(rinc_Repl *r) { r->line0 = r->n; r->cur = r->n; r->scr = 0; r->mode = rinc_Mode_plain; r->word = -1; r->numdot = -1; r->back = 0; r->autoind = 0; }
static void rinc_key(rinc_Repl *r, int k) { /* an editing key: A up, B down, C right, D left, H home, F end, 3 delete */
  if (k == 'A' || k == 'B') { rinc_recall(r, k == 'A' ? 1 : -1); return; }
  if (k == 'C' && r->cur < r->n) r->cur++;
  else if (k == 'D' && r->cur > r->line0) r->cur--;
  else if (k == 'H') r->cur = r->line0;
  else if (k == 'F') r->cur = r->n;
  else if (k == '3' && r->cur < r->n) { memmove(r->acc + r->cur, r->acc + r->cur + 1, (size_t)(r->n - r->cur - 1)); r->n--; rinc_redraw(r); return; }
  else return;
  if (r->echo) { /* (just the cursor) */
    char b[16]; int d = r->cur - r->line0 - r->scr;
    if (d < 0) rinc_moveLeft(r, -d); else if (d > 0) r->put(b, snprintf(b, sizeof b, "\033[%dC", d));
    r->scr = r->cur - r->line0;
  }
}

void rinc_replChar(rinc_Repl *r, int c) {
  if (c < 0) return;
  if (r->saving) { rinc_saveChar(r, c); return; }
  if (r->esc == 1) { r->esc = c == '[' || c == 'O' ? 2 : 0; r->escn = 0; if (r->esc) return; } /* (else: a lone ESC, then c) */
  else if (r->esc == 2) {
    if (c >= '0' && c <= '9') { r->escn = r->escn * 10 + c - '0'; return; }
    r->esc = 0;
    if (c == '~') rinc_key(r, r->escn == 1 || r->escn == 7 ? 'H' : r->escn == 4 || r->escn == 8 ? 'F' : r->escn == 3 ? '3' : 0);
    else rinc_key(r, c);
    return;
  }
  if (c == 27) { r->esc = 1; return; }
  if (c == '\r' || c == '\n') {
    int res;
    rinc_outdent(r);
    rinc_key(r, 'F'); /* (to the end, then the last word's color, if it's done typing one) */
    if (r->echo) { rinc_finishWord(r); rinc_puts(r, rinc_PAINT_PLAIN "\r\n"); }
    if (r->line0 && rinc_blank(r)) { r->put("(unfinished input dropped)\r\n", 28); r->n = 0; rinc_newLine(r); rinc_prompt(r); return; }
    rinc_remember(r);
    if (!r->line0 && r->n == 3 && !memcmp(r->acc, "run", 3)) { r->n = 0; rinc_newLine(r); r->run = 1; return; } /* the platform takes it from here */
    if (rinc_command(r)) { r->n = 0; rinc_newLine(r); if (!r->saving && !r->run) rinc_prompt(r); return; }
    if (r->n >= (int)sizeof r->acc - 1) { r->put("(input too long)\r\n", 18); r->n = 0; rinc_newLine(r); rinc_prompt(r); return; }
    r->acc[r->n++] = '\n';
    res = rio_replEval(r->vm, r->acc, (uint32_t)r->n);
    if (res == rio_ReplResult_more) { rinc_newLine(r); rinc_prompt(r); rinc_indent(r); return; }
    if (res < 0) { const char *e = rio_error(r->vm); r->errors++; r->put(e, (int)strlen(e)); r->put("\r\n", 2); }
    r->n = 0; rinc_newLine(r);
    rinc_prompt(r);
  } else if (c == 8 || c == 127) { /* backspace (in the indent, a step) */
    r->autoind = 0;
    if (r->cur == r->n) {
      int k = r->n > r->line0 && rinc_blank(r) && r->n - r->line0 >= 2 ? 2 - (r->n - r->line0) % 2 : 1;
      while (k-- > 0 && r->n > r->line0) r->n--;
      r->cur = r->n;
    } else if (r->cur > r->line0) { memmove(r->acc + r->cur - 1, r->acc + r->cur, (size_t)(r->n - r->cur)); r->n--; r->cur--; }
    rinc_redraw(r);
  } else if (c == 1) rinc_key(r, 'H'); /* Ctrl-A, Ctrl-E: the start and end of the line */
  else if (c == 5) rinc_key(r, 'F');
  else if (c == 3) { /* Ctrl-C: drop what's typed */
    r->put(rinc_PAINT_PLAIN "^C\r\n", 8); r->n = 0; rinc_newLine(r); rinc_prompt(r);
  } else if (c >= 32 && c < 127 && r->n < (int)sizeof r->acc - 2) {
    if (c == ' ' && r->autoind > 0 && r->cur == r->n && rinc_blank(r)) { r->autoind--; return; } /* (in place of the auto indent) */
    if (c != ' ') r->autoind = 0;
    if (c == ' ') rinc_outdent(r);
    if (r->cur == r->n) rinc_append(r, (char)c);
    else { memmove(r->acc + r->cur + 1, r->acc + r->cur, (size_t)(r->n - r->cur)); r->acc[r->cur++] = (char)c; r->n++; rinc_redraw(r); }
  }
}
#endif
