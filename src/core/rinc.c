#include <stdio.h>
#include <string.h>
#include "rinc.h"

_Alignas(4) uint8_t rinc_screen[rinc_H][rinc_W]; /* (aligned for DMA) */
uint16_t rinc_palette[256];
float (*rinc_time)(void);
uint16_t rinc_buttons;
int16_t rinc_axes[6];
static void (*rinc_put)(const char *s, int n);

static uint16_t rinc_rgb565(int r, int g, int b) { return (uint16_t)((r >> 3) << 11 | (g >> 2) << 5 | b >> 3); }

static void rinc_initPalette(void) {
  static const uint32_t pico8[16] = {0x000000, 0x1D2B53, 0x7E2553, 0x008751, 0xAB5236, 0x5F574F, 0xC2C3C7, 0xFFF1E8,
                                     0xFF004D, 0xFFA300, 0xFFEC27, 0x00E436, 0x29ADFF, 0x83769C, 0xFF77A8, 0xFFCCAA};
  static const uint32_t secret[16] = {0x291814, 0x111D35, 0x422136, 0x125359, 0x742F29, 0x49333B, 0xA28879, 0xF3EF7D,
                                      0xBE1250, 0xFF6C24, 0xA8E72E, 0x00B543, 0x065AB5, 0x754665, 0xFF6E59, 0xFF9D81}; /* (PICO-8's 128-143) */
  static const int level[6] = {0, 95, 135, 175, 215, 255};
  int i;
  for (i = 0; i < 16; i++) rinc_palette[i] = rinc_rgb565((int)(pico8[i] >> 16), (int)(pico8[i] >> 8 & 255), (int)(pico8[i] & 255));
  for (i = 0; i < 16; i++) rinc_palette[16 + i] = rinc_rgb565((int)(secret[i] >> 16), (int)(secret[i] >> 8 & 255), (int)(secret[i] & 255));
  for (i = 0; i < 216; i++) rinc_palette[32 + i] = rinc_rgb565(level[i / 36], level[i / 6 % 6], level[i % 6]);
  for (i = 0; i < 8; i++) rinc_palette[248 + i] = rinc_rgb565(8 + 32 * i, 8 + 32 * i, 8 + 32 * i);
}

/* host procs: arguments arrive in a[], a result goes in a[0]. Drawing outside the screen is clipped */
static void rinc_hostCls(rio_Vm *v, rio_Val *a) { (void)v; memset(rinc_screen, (uint8_t)a[0].i, sizeof rinc_screen); }
static void rinc_hostPset(rio_Vm *v, rio_Val *a) {
  uint32_t x = a[0].u, y = a[1].u;
  (void)v;
  if (x < rinc_W && y < rinc_H) rinc_screen[y][x] = (uint8_t)a[2].i;
}
static void rinc_hostPget(rio_Vm *v, rio_Val *a) {
  uint32_t x = a[0].u, y = a[1].u;
  (void)v;
  a[0].i = x < rinc_W && y < rinc_H ? rinc_screen[y][x] : 0;
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
   Enter, Backspace, a letter for Ctrl+letter) with modifiers m. ESC [ Z is Shift+Tab */
void rinc_textPush(int c) {
  rinc_Text *t = &rinc_typed;
  if (t->esc == 1) {
    if (c == '[' || c == 'O') { t->esc = 2; t->escn = 0; t->escp[0] = t->escp[1] = t->escp[2] = 0; return; }
    t->esc = 0; rinc_queue(t, 27); /* (a lone ESC: Esc itself, then c) */
  } else if (t->esc == 2) {
    int n = t->escp[0], m = t->escp[1] > 1 ? t->escp[1] - 1 : 0, mods = rinc_textMods(m & 1, m & 4, m & 2), k = t->escp[2];
    if (c >= '0' && c <= '9') { if (t->escp[t->escn] < 10000) t->escp[t->escn] = t->escp[t->escn] * 10 + c - '0'; return; }
    if (c == ';') { if (t->escn < 2) t->escn++; return; }
    t->esc = 0;
    if (c == '~' && n == 27) { /* (xterm's modifyOtherKeys) */
      if (k >= 'A' && k <= 'Z') k += 32;
      c = k == 9 || k == 13 ? k : k == 8 || k == 127 ? 8 : k >= 'a' && k <= 'z' && (mods & rinc_Mod_ctrl) ? k - 'a' + 1 : 0;
      if (c >= 1 && c <= 26 && c != 8 && c != 9 && c != 13) mods &= ~rinc_Mod_ctrl; /* (Ctrl+letter says Ctrl itself) */
    } else
      c = c == 'A' ? rinc_Key_up : c == 'B' ? rinc_Key_down : c == 'C' ? rinc_Key_right : c == 'D' ? rinc_Key_left :
          c == 'H' ? rinc_Key_home : c == 'F' ? rinc_Key_end : c == 'Z' ? 9 | rinc_Mod_shift : c != '~' ? 0 :
          n == 1 || n == 7 ? rinc_Key_home : n == 4 || n == 8 ? rinc_Key_end : n == 3 ? rinc_Key_delete :
          n == 5 ? rinc_Key_pageUp : n == 6 ? rinc_Key_pageDown : 0;
    if (c) rinc_queue(t, c | mods);
    return;
  }
  if (c == 27) { t->esc = 1; return; }
  if (c == '\n') c = 13;
  if (c == 127) c = 8;
  if ((c >= 1 && c <= 26) || (c >= 32 && c < 127)) rinc_queue(t, c);
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
static char rinc_appRef[rinc_REF] = "app"; /* and as it was started: chord, or its project, chord* */
static int rinc_appProject; /* (it's a project: its own save space) */
void rinc_setAppName(const char *name) {
  size_t n = strlen(name);
  if (n >= sizeof rinc_appRef) n = sizeof rinc_appRef - 1;
  memcpy(rinc_appRef, name, n); rinc_appRef[n] = 0;
  rinc_appProject = n > 1 && name[n - 1] == '*';
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
/* an app's name, or its project's (a star after it) */
static int rinc_refOk(const char *s, int n) { return n > 1 && s[n - 1] == '*' ? rinc_nameOk(s, n - 1, 0) : rinc_nameOk(s, n, 0); }
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
  snprintf(ref, sizeof ref, "%s%s", name, ud ? "*" : ""); /* (ud: projects) */
  rinc_appAdd(ref);
}
static int rinc_lowerCmp(const char *a, const char *b) {
  for (;; a++, b++) {
    int x = *a >= 'A' && *a <= 'Z' ? *a + 32 : *a, y = *b >= 'A' && *b <= 'Z' ? *b + 32 : *b;
    if (x != y || !x) return x - y;
  }
}
/* the apps, in storage and in the bundle, and the projects (chord*), in order of name (ignoring case,
   each project after its app): how many */
static int rinc_findApps(void) {
  int i, j;
  rinc_appCount = 0;
  snprintf(rinc_appHome, sizeof rinc_appHome, "%s", rinc_homeName()); /* (read first: storage has one file open at a time) */
  if (rinc_fs) { rinc_fs->list("/rinc/apps", rinc_appFound, 0); rinc_fs->list("/rinc/projects", rinc_appFound, (void *)1); }
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
static int rinc_readFile(const char *path, char *buf, uint32_t cap, uint32_t *total) {
  char chunk[256]; int got;
  *total = 0;
  if (!rinc_fs || rinc_fs->open(path, 0)) return -1;
  while ((got = rinc_fs->read(chunk, sizeof chunk)) > 0) {
    if (buf && *total < cap) memcpy(buf + *total, chunk, (size_t)got < cap - *total ? (size_t)got : cap - *total);
    *total += (uint32_t)got;
  }
  rinc_fs->close();
  return got < 0 ? -1 : 0;
}
/* an app's source, or its project's (chord*) */
static int rinc_appPath(const char *name, char *path, size_t cap) {
  int n = (int)strlen(name), project = n > 1 && name[n - 1] == '*';
  if (!rinc_refOk(name, n)) return 0;
  if (project) n--;
  return snprintf(path, cap, "/rinc/%s/%.*s/%.*s.rio", project ? "projects" : "apps", n, name, n, name) < (int)cap;
}
int rinc_readApp(const char *name, char *buf, uint32_t cap) {
  char path[96]; uint32_t total;
  if (!rinc_appPath(name, path, sizeof path) || rinc_readFile(path, buf, cap, &total)) return -1;
  return total > cap ? -2 : (int)total;
}
void rinc_cacheDrop(const char *name); /* (compiled apps: below) */
static void rinc_hostApps(rio_Vm *v, rio_Val *a) { (void)v; a[0].i = rinc_findApps(); }
static int rinc_copyOut(rio_Vm *v, rio_Val *out, const char *s, int n) {
  char *p = (char *)rio_sliceBytes(v, out); int cap = (int)out[1].u;
  if (p) memcpy(p, s, (size_t)(n < cap ? n : cap));
  return n;
}
static void rinc_hostAppName(rio_Vm *v, rio_Val *a) {
  a[0].i = a[0].u < (uint32_t)rinc_appCount ? rinc_copyOut(v, a + 1, rinc_appNames[a[0].i], (int)strlen(rinc_appNames[a[0].i])) : 0;
}
static void rinc_hostAppInfo(rio_Vm *v, rio_Val *a) {
  char path[96], line[128]; const char *t = line; uint32_t n = 0; int i, k = a[0].i; const rinc_App *b;
  a[0].i = 0;
  if (k < 0 || k >= rinc_appCount) return;
  if (rinc_appPath(rinc_appNames[k], path, sizeof path) && !rinc_readFile(path, line, sizeof line, &n)) {
    if (n > sizeof line) n = sizeof line;
  } else if ((b = rinc_bundled(rinc_appNames[k])) && b->image) {
    a[0].i = rinc_copyOut(v, a + 1, b->info, (int)strlen(b->info));
    return;
  } else if (b) { t = b->data; n = rinc_appLen(b); }
  if (n == 0 || t[0] != '#') return;
  for (i = 1; i < (int)n && t[i] == ' '; i++) {}
  for (k = i; k < (int)n && t[k] != '\n' && t[k] != '\r'; k++) {}
  a[0].i = rinc_copyOut(v, a + 1, t + i, k - i);
}
/* help texts, for a help app: each app's own, in its help folder (/rinc/apps/<app>/help/..., named
   <app>/...), and those that are no app's (Rinc's and Rio's docs), in /rinc/help (named by their path
   there), and each project's (/rinc/projects/<app>/help/..., named with the star, chord* then /...); in storage, and in the
   bundle the same way (apps/<app>/help/..., help/...). helps(out) -> Int looks for them, and puts their
   names in out (.md left off: rinc/spec/apps), in order, each with a 10 after it, as many as fit: how
   many; helpRead(name, out) -> Int copies a text (from storage, else the bundle) into out, as much as
   fits: its whole length, or -1 */
static char *rinc_helpOut; static int rinc_helpCap, rinc_helpUsed, rinc_helpCount; /* (the names, in the app's own memory) */
static char rinc_helpDirs[16][96]; static int rinc_helpDirCount, rinc_helpDirNow; /* (folders to look through, in storage) */
/* a help text's name: letters, digits, _, - and dots, in parts split by /, none starting with a dot (and
   a project's star, chord*, ending the first) */
static int rinc_helpNameOk(const char *s, int n) {
  int i;
  if (!s || n < 1 || n > 47 || s[0] == '/' || s[n - 1] == '/') return 0;
  for (i = 0; i < n; i++) {
    char c = s[i];
    if (c == '.' && (i == 0 || s[i - 1] == '/')) return 0;
    if (c == '/' && s[i - 1] == '/') return 0;
    if (c == '*' && (i == 0 || i + 1 >= n || s[i + 1] != '/' || memchr(s, '/', (size_t)i))) return 0;
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' || c == '/' || c == '*')) return 0;
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
  else if (!strncmp(dir, "/rinc/projects/", 15) && (h = strstr(dir + 15, "/help"))) /* (a project's: its star, then /a) */
    snprintf(out, cap, "%.*s*%s", (int)(h - dir - 15), dir + 15, h + 5);
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
    const char *a = i < apps ? rinc_appNames[i] : rinc_appHome; int n = (int)strlen(a), project = n && a[n - 1] == '*';
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
static void rinc_hostHelps(rio_Vm *v, rio_Val *a) { char *out = (char *)rio_sliceBytes(v, a); a[0].i = rinc_findHelps(out, out ? (int)a[1].u : 0); }
static void rinc_hostHelpRead(rio_Vm *v, rio_Val *a) {
  const char *s = (const char *)rio_sliceBytes(v, a), *slash; int n = (int)a[1].u, app = 0, k; char name[64], path[128];
  uint8_t *out = (uint8_t *)rio_sliceBytes(v, a + 2); uint32_t cap = a[3].u, total; const rinc_File *f = 0;
  a[0].i = -1; /* (after the name's read: it was there) */
  if (!rinc_helpNameOk(s, n)) return;
  memcpy(name, s, (size_t)n); name[n] = 0;
  if ((slash = strchr(name, '/'))) app = (int)(slash - name); /* (app/rest: maybe an app's own) */
  if (app > 1 && name[app - 1] == '*') { /* (a project's: in storage only) */
    snprintf(path, sizeof path, "/rinc/projects/%.*s/help/%s.md", app - 1, name, slash + 1);
    if (rinc_nameOk(name, app - 1, 0) && !rinc_readFile(path, (char *)out, out ? cap : 0, &total)) a[0].i = (int32_t)total;
    return;
  }
  for (k = 0; k < 4 && !f; k++) { /* storage: the app's, then help's; the bundle: the same */
    if ((k == 0 || k == 2) && !(app && rinc_nameOk(name, app, 0))) continue;
    if (k == 0) snprintf(path, sizeof path, "/rinc/apps/%.*s/help/%s.md", app, name, slash + 1);
    else if (k == 1) snprintf(path, sizeof path, "/rinc/help/%s.md", name);
    else if (k == 2) snprintf(path, sizeof path, "apps/%.*s/help/%s.md", app, name, slash + 1);
    else snprintf(path, sizeof path, "help/%s.md", name);
    if (k < 2) {
      if (!rinc_readFile(path, (char *)out, out ? cap : 0, &total)) { a[0].i = (int32_t)total; return; }
    } else if ((f = rinc_bundleFile(path))) {
      if (out) memcpy(out, f->data, f->len < cap ? f->len : cap);
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
  a[0].i = rinc_copyOut(v, a, rinc_helpTopicNow, (int)strlen(rinc_helpTopicNow));
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
static void rinc_hostArgs(rio_Vm *v, rio_Val *a) { a[0].i = rinc_copyOut(v, a, rinc_callArgs, rinc_callArgsLen); }
static void rinc_hostResult(rio_Vm *v, rio_Val *a) {
  int n = rinc_callResultLen;
  a[0].i = n < 0 ? -1 : rinc_copyOut(v, a, rinc_callResult, n);
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
static void rinc_hostKey(rio_Vm *v, rio_Val *a) { a[0].i = rinc_dequeue(v == &rinc_nest ? &rinc_nestTyped : &rinc_typed); }

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
/* an app's name, or its project's (chord*), into out (rinc_REF) */
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
static int rinc_savePath(rio_Vm *v, rio_Val *file, char *path, size_t cap) {
  char own[33], space[64];
  return rinc_argName(v, file, own, 1) && rinc_saveSpace(space, sizeof space) && snprintf(path, cap, "%s/%s", space, own) < (int)cap;
}
/* every file in folder from, copied to the same places in folder to (but not a top-level folder named
   skip): 0, or -1 if any couldn't be. Names gather in rinc_tree as each folder's listed */
static char rinc_tree[512]; static int rinc_treeUsed;
static void rinc_treeFound(const char *name, int dir, void *ud) {
  size_t n = strlen(name);
  (void)ud;
  if (n > 47 || rinc_treeUsed + (int)n + 2 > (int)sizeof rinc_tree) return;
  rinc_tree[rinc_treeUsed++] = dir ? 'd' : 'f';
  memcpy(rinc_tree + rinc_treeUsed, name, n + 1); rinc_treeUsed += (int)n + 1;
}
static int rinc_copyTree(const char *from, const char *to, const char *skip, int depth) {
  int start = rinc_treeUsed, end, at, bad = 0; char a[128], b[128];
  if (!rinc_fs || depth > 4 || rinc_fs->list(from, rinc_treeFound, 0)) { rinc_treeUsed = start; return -1; }
  end = rinc_treeUsed;
  for (at = start; at < end; at += (int)strlen(rinc_tree + at + 1) + 2) {
    const char *name = rinc_tree + at + 1;
    if (snprintf(a, sizeof a, "%s/%s", from, name) >= (int)sizeof a || snprintf(b, sizeof b, "%s/%s", to, name) >= (int)sizeof b) { bad = 1; continue; }
    if (rinc_tree[at] == 'd') { if (!(skip && !strcmp(name, skip)) && rinc_copyTree(a, b, 0, depth + 1)) bad = 1; }
    else if (rinc_fs->copy(a, b)) bad = 1;
    rinc_treeUsed = end;
  }
  rinc_treeUsed = start;
  return bad ? -1 : 0;
}
/* every file and folder in folder dir, and it: 0, or -1 if any couldn't be removed */
static int rinc_removeTree(const char *dir, int depth) {
  int start = rinc_treeUsed, end, at, bad = 0; char a[128];
  if (!rinc_fs || depth > 4) return -1;
  if (!rinc_fs->list(dir, rinc_treeFound, 0)) {
    end = rinc_treeUsed;
    for (at = start; at < end; at += (int)strlen(rinc_tree + at + 1) + 2) {
      if (snprintf(a, sizeof a, "%s/%s", dir, rinc_tree + at + 1) >= (int)sizeof a) { bad = 1; continue; }
      if (rinc_tree[at] == 'd' ? rinc_removeTree(a, depth + 1) : rinc_fs->remove(a)) bad = 1;
      rinc_treeUsed = end;
    }
  }
  rinc_treeUsed = start;
  return rinc_fs->remove(dir) || bad ? -1 : 0;
}
/* an app's folder, or its project's (chord*): 0 if the name's no good */
static int rinc_appFolder(const char *name, char *out, size_t cap) {
  int n = (int)strlen(name), project = n > 1 && name[n - 1] == '*';
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
/* appCopy(from, to) -> Int: app or project from, copied as new project to (a star after it): its
   files, its main one renamed (its own saves aside). appRename(from, to) -> Int: project from as new
   project to. Each 0, or -1 (to there already, say) */
static void rinc_hostAppCopy(rio_Vm *v, rio_Val *a) {
  char from[rinc_REF], to[rinc_REF], fromDir[64], toDir[64], path[96], main[96]; uint32_t total; int n, k;
  int named = rinc_argRef(v, a, from) && rinc_argRef(v, a + 2, to);
  a[0].i = -1;
  if (!named || !rinc_fs || (n = (int)strlen(to)) < 2 || to[n - 1] != '*' || !rinc_appFolder(from, fromDir, sizeof fromDir) || !rinc_appFolder(to, toDir, sizeof toDir)) return;
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
  k = (int)strlen(from) - (from[strlen(from) - 1] == '*');
  snprintf(main, sizeof main, "%s/%.*s.rio", toDir, k, from); /* (its main file, by its old name: renamed) */
  a[0].i = rinc_fs->rename(main, path) && rinc_readApp(to, 0, 0) < 0 ? -1 : 0;
}
static void rinc_hostAppRename(rio_Vm *v, rio_Val *a) {
  char from[rinc_REF], to[rinc_REF], fromDir[64], toDir[64], path[96], main[96], active[rinc_REF]; uint32_t total; int n, m;
  int named = rinc_argRef(v, a, from) && rinc_argRef(v, a + 2, to);
  a[0].i = -1;
  if (!named || !rinc_fs || (n = (int)strlen(from)) < 2 || from[n - 1] != '*' || (m = (int)strlen(to)) < 2 || to[m - 1] != '*' ||
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
static void rinc_hostDir(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_argPath(v, a, path, 0);
  rinc_dirOut = (char *)rio_sliceBytes(v, a + 2); rinc_dirCap = rinc_dirOut ? (int)a[3].u : 0; rinc_dirUsed = rinc_dirCount = 0;
  a[0].i = named && rinc_fs && !rinc_fs->list(path, rinc_dirFound, 0) ? rinc_dirCount : -1;
  rinc_dirOut = 0;
}
static void rinc_hostFileRead(rio_Vm *v, rio_Val *a) {
  char path[96]; uint8_t *out = (uint8_t *)rio_sliceBytes(v, a + 2); uint32_t cap = a[3].u, total; int named = rinc_argPath(v, a, path, 0);
  a[0].i = named && !rinc_readFile(path, (char *)out, out ? cap : 0, &total) ? (int32_t)total : -1;
}
static void rinc_hostFileWrite(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_argPath(v, a, path, 1);
  if (named) rinc_writeFile(v, a, path); else a[0].i = -1;
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
  a[0].i = named && rinc_fs && !rinc_exists(to) && rinc_exists(from) && !rinc_fs->rename(from, to) ? 0 : -1;
}
/* a project's own save space, the first time it runs: the app's saves, copied in */
static void rinc_projectSaves(void) {
  char space[64], app[64];
  if (!rinc_appProject || !rinc_fs || !rinc_saveSpace(space, sizeof space) || !rinc_fs->list(space, rinc_treeFound, 0)) { rinc_treeUsed = 0; return; }
  rinc_treeUsed = 0;
  snprintf(app, sizeof app, "/rinc/save/%s", rinc_appNow);
  rinc_copyTree(app, space, 0, 0); /* (none: nothing to copy) */
}
/* appInstall(name) -> Int: project name* installed as the app name: its files copied over the app's
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
  char name[rinc_REF], path[96]; uint8_t *out = (uint8_t *)rio_sliceBytes(v, a + 2); uint32_t cap = a[3].u, total; const rinc_App *b;
  int named = rinc_argRef(v, a, name);
  a[0].i = -1;
  if (!named) return;
  if (rinc_appPath(name, path, sizeof path) && !rinc_readFile(path, (char *)out, out ? cap : 0, &total)) a[0].i = (int32_t)total;
  else if ((b = rinc_bundled(name)) && !b->image) {
    total = rinc_appLen(b);
    if (out) memcpy(out, b->data, total < cap ? total : cap);
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
  char path[96]; uint8_t *out = (uint8_t *)rio_sliceBytes(v, a + 2); uint32_t cap = a[3].u, total; int named = rinc_savePath(v, a, path, sizeof path);
  a[0].i = named && !rinc_readFile(path, (char *)out, out ? cap : 0, &total) ? (int32_t)total : -1;
}
static void rinc_hostSaveWrite(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_savePath(v, a, path, sizeof path);
  if (named) rinc_writeFile(v, a, path); else a[0].i = -1;
}
static void rinc_hostSaveDelete(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_savePath(v, a, path, sizeof path);
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
  char path[96]; uint8_t *out = (uint8_t *)rio_sliceBytes(v, a + 2); uint32_t cap = a[3].u, total; int named = rinc_stashPath(v, a, path, sizeof path);
  a[0].i = named && !rinc_readFile(path, (char *)out, out ? cap : 0, &total) ? (int32_t)total : -1;
}
static void rinc_hostStashDelete(rio_Vm *v, rio_Val *a) {
  char path[96]; int named = rinc_stashPath(v, a, path, sizeof path);
  a[0].i = named && rinc_fs && !rinc_fs->remove(path) ? 0 : -1;
}
/* the settings, /rinc/settings/<key>: the whole Rinc's, not an app's, shared by every app (such as
   "active": the app or project being worked on, chord or chord*). settingRead(key, out) -> Int: as much
   as fits, and its whole length, or -1 if there's none; settingWrite(key, data) -> Int: 0, or -1. Keys
   are named as save files are */
static int rinc_settingPath(rio_Vm *v, rio_Val *key, char *path, size_t cap) {
  char own[33];
  return rinc_argName(v, key, own, 1) && snprintf(path, cap, "/rinc/settings/%s", own) < (int)cap;
}
static void rinc_hostSettingRead(rio_Vm *v, rio_Val *a) {
  char path[64]; uint8_t *out = (uint8_t *)rio_sliceBytes(v, a + 2); uint32_t cap = a[3].u, total; int named = rinc_settingPath(v, a, path, sizeof path);
  a[0].i = named && !rinc_readFile(path, (char *)out, out ? cap : 0, &total) ? (int32_t)total : -1;
}
static void rinc_hostSettingWrite(rio_Vm *v, rio_Val *a) {
  char path[64]; int named = rinc_settingPath(v, a, path, sizeof path);
  if (named) rinc_writeFile(v, a, path); else a[0].i = -1;
}
/* the theme: a color for each role the system apps draw with (pc.Role, in this order), so they look
   alike and a theme changes them all. theme(role) -> Int. The theme setting names one, owner/id: a
   file of an app's, owner/themes/id.theme.rio, in its folder (or the bundle's) or its save space, or a
   project's (owner*). The file is Rio's form of constants, and only that is read, never run: lines of
   name :: number (a star after the name, or not), # comments, and the first line its title. Roles it
   leaves out, or with no theme, are Rinc's own colors, these */
static const char *const rinc_roleNames[] = {"bg", "bar", "text", "soft", "dim", "faint", "accent", "heading", "pick", "select", "link",
                                             "error", "ok", "codeBg", "code", "punct", "keyword", "type", "call", "number", "string", "comment"};
#define rinc_ROLES ((int)(sizeof rinc_roleNames / sizeof rinc_roleNames[0]))
static const uint8_t rinc_roleDefaults[rinc_ROLES] = {1, 0, 7, 6, 13, 5, 10, 9, 12, 2, 12, 8, 11, 17, 7, 6, 14, 15, 12, 9, 11, 13};
static uint8_t rinc_theme[rinc_ROLES]; static int rinc_themeOn; /* (loaded once: before, the defaults) */
/* a theme's file, read into the roles: 0, or -1 (no such file) */
static int rinc_themeRead(const char *owner, int n, const char *id, int m) {
  static char text[2048]; char path[112]; uint32_t len = 0; int project = owner[n - 1] == '*', i, found = 0;
  const rinc_File *f;
  snprintf(path, sizeof path, "/rinc/%s/%.*s/themes/%.*s.theme.rio", project ? "projects" : "apps", project ? n - 1 : n, owner, m, id);
  if (!rinc_readFile(path, text, sizeof text - 1, &len)) found = 1;
  if (!found && !project) {
    snprintf(path, sizeof path, "/rinc/save/%.*s/themes/%.*s.theme.rio", n, owner, m, id);
    if (!rinc_readFile(path, text, sizeof text - 1, &len)) found = 1;
  }
  if (!found && !project) {
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
  char name[80]; uint32_t len = 0; const char *slash; int n;
  memcpy(rinc_theme, rinc_roleDefaults, sizeof rinc_theme);
  rinc_themeOn = 1;
  rinc_textFg = rinc_theme[2]; /* (the screen console's text) */
  if (rinc_readFile("/rinc/settings/theme", name, sizeof name - 1, &len) || len >= sizeof name) return;
  while (len && (name[len - 1] == '\n' || name[len - 1] == '\r' || name[len - 1] == ' ')) len--;
  name[len] = 0;
  if (!(slash = strchr(name, '/')) || !rinc_refOk(name, n = (int)(slash - name)) || !rinc_nameOk(slash + 1, (int)strlen(slash + 1), 0)) return;
  rinc_themeRead(name, n, slash + 1, (int)strlen(slash + 1));
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
static int rinc_copyText(rio_Vm *v, rio_Val *out, const char *s, int n) {
  char *p = (char *)rio_sliceBytes(v, out); int cap = (int)out[1].u;
  if (n > cap) n = cap;
  if (p && n > 0) memcpy(p, s, (size_t)n);
  return p ? n : 0;
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
  a[0].i = v == &rinc_nest ? 0 : rinc_copyText(v, a, e, (int)strlen(e));
}
static void rinc_hostRioLog(rio_Vm *v, rio_Val *a) {
  int n = v == &rinc_nest ? 0 : rinc_copyText(v, a, rinc_nestLog, rinc_nestLogLen);
  memmove(rinc_nestLog, rinc_nestLog + n, (size_t)(rinc_nestLogLen - n)); rinc_nestLogLen -= n;
  a[0].i = n;
}
static void rinc_hostRioIndent(rio_Vm *v, rio_Val *a) { const char *s = (const char *)rio_sliceBytes(v, a); a[0].i = s ? rio_replIndent(s, a[1].u) : 0; }
static void rinc_hostBtn(rio_Vm *v, rio_Val *a) { (void)v; a[0].i = a[0].u < 16 ? rinc_buttons >> a[0].i & 1 : 0; }
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
   line breaks, is skipped) into out, until it's full. Returns how many. For binary data typed into an
   app: sound, pictures */
static int rinc_hexDigit(int c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1; }
static void rinc_hostFromhex(rio_Vm *v, rio_Val *a) {
  uint8_t *out = (uint8_t *)rio_sliceBytes(v, a); const char *t = (const char *)rio_sliceBytes(v, a + 2);
  uint32_t cap = a[1].u, n = a[3].u, i, k = 0; int hi = -1;
  for (i = 0; out && t && i < n && k < cap; i++) {
    int d = rinc_hexDigit(t[i]);
    if (d < 0) continue;
    if (hi < 0) hi = d; else { out[k++] = (uint8_t)(hi << 4 | d); hi = -1; }
  }
  a[0].u = k;
}

int rinc_game, rinc_showLog;
/* what an app logs, and what Rinc says about it: to the platform, and on the screen while rinc_showLog */
static void rinc_out(const char *s, int n) { rinc_put(s, n); if (rinc_showLog) rinc_consolePut(s, n); }
void rinc_say(const char *s) { rinc_out(s, (int)strlen(s)); }
static void rinc_logLine(void *ud, const char *s, int n) { (void)ud; rinc_out(s, n); rinc_out("\r\n", 2); }
/* the way home: on the screen (it's for whoever's looking at it), not in what's printed */
static void rinc_back(void) { static const char s[] = "\r\n(Sys+Esc: back home)\r\n"; if (rinc_showLog) rinc_consolePut(s, (int)sizeof s - 1); }

/* the modules and files Rinc has for apps (docs/spec/apps.md#more-files): pc, from pc.rio, built in (its
   host procs, as declared, and its types); an app's own files, in its folder, for include "parts/a.rio"
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
static int rinc_srcCount, rinc_srcLost; /* (lost: more than were noted) */
#ifdef rio_NO_COMPILER
static int rinc_loader(void *ud, const char *path, int kind, rio_Source *out) { (void)ud; (void)path; (void)kind; (void)out; return 1; } /* (images only) */
#else
extern const char rinc_pcSource[];
extern const int rinc_pcSource_len;
int (*rinc_hostRead)(const char *path, char *buf, uint32_t at, uint32_t n);
char rinc_hostFolder[512];
static int rinc_readAt(void *ud, char *buf, uint32_t at, uint32_t n) {
  rinc_Reader *r = (rinc_Reader *)ud; int got = 0, k;
  if (r->host) {
    char full[640];
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
  char base[64], bundleBase[64], tryPath[104], name[640]; const char *last; const rinc_File *f; int n = (int)strlen(rinc_loadApp), project, k, res;
  int host = !(kind & rio_LoadKind_package) && rinc_hostFolder[0]; /* (an app run from the host's files: its own, from there) */
  (void)ud;
  if ((kind & rio_LoadKind_package) && !strcmp(path, "pc")) {
    out->src = rinc_pcSource; out->len = (uint32_t)rinc_pcSource_len; out->isDir = 0; out->name = "pc.rio";
    return 0;
  }
  if (!rinc_srcOn || strstr(path, "..") || strlen(path) > 48) return 1;
  project = n > 1 && rinc_loadApp[n - 1] == '*';
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

static int rinc_switching, rinc_pick; /* the switcher (Sys+Tab) is up, and which recent app it's on */

static int rinc_sysLock; /* Ctrl+Shift tapped: the system key, for the next combination */
int rinc_fullscreen;      /* Sys+Enter asked for the screen to fill the display, or not (rinc.h) */

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
    rinc_overlayText("Esc home Tab switch Q back R redo H help P perf", 2, 7);
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
  char path[96]; uint32_t total; int n = (int)strlen(app), project = n > 1 && app[n - 1] == '*';
  if (!rinc_bundled("help") && rinc_readApp("help", 0, 0) < 0) return 0;
  snprintf(path, sizeof path, "/rinc/%s/%.*s/help/README.md", project ? "projects" : "apps", project ? n - 1 : n, app);
  if (!rinc_readFile(path, 0, 0, &total)) return 1;
  if (project) return 0;
  snprintf(path, sizeof path, "apps/%.32s/help/README.md", app);
  return rinc_bundleFile(path) != 0;
}
int rinc_systemKey(int sysDown, int mods, int otherDown, int keys) {
  static int now, down, seen, spoiled, startAt, lastTap, lastTapAt, again, wasKeys, wasOther, held;
  int pressed = keys & ~wasKeys, otherPressed = otherDown && !wasOther, go = 0, strip;
  now++;
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
    else if ((pressed & rinc_SysKey_enter) || (held && !sysDown)) {
      rinc_switching = 0;
      if (rinc_pick > 0 && rinc_pick < n) { snprintf(rinc_launchName, sizeof rinc_launchName, "%s", rinc_recent[rinc_pick]); go = 1; }
    }
  } else if ((sysDown || rinc_sysLock) && pressed && !(pressed & ~rinc_SysKey_pointer)) {
    /* the pointer's keys only: they move and click it (rinc_pointerFrame), and don't use up a locked system key */
  } else if ((sysDown || rinc_sysLock) && pressed) { /* (alone, the system key does nothing, so it isn't pressed by accident) */
    if (pressed & rinc_SysKey_esc) { snprintf(rinc_launchName, sizeof rinc_launchName, "home"); go = 1; }
    else if ((pressed & (rinc_SysKey_tab | rinc_SysKey_next | rinc_SysKey_prev)) && rinc_recentCount > 1) {
      int back = (pressed & rinc_SysKey_prev) || ((pressed & rinc_SysKey_tab) && (mods & 1));
      rinc_switching = 1; rinc_pick = back ? rinc_recentCount - 1 : 1;
      held = sysDown;
    } else if (pressed & rinc_SysKey_q) { /* back to the app before, this one forgotten, its stash too */
      rinc_stashDrop(rinc_appNow);
      rinc_recentDrop(0);
      snprintf(rinc_launchName, sizeof rinc_launchName, "%s", rinc_recentCount ? rinc_recent[0] : "home");
      go = 1;
    } else if (pressed & rinc_SysKey_r) { /* this app again, fresh: its stash dropped, and its compiled image */
      rinc_stashDrop(rinc_appNow);
      rinc_cacheDrop(rinc_appRef);
      snprintf(rinc_launchName, sizeof rinc_launchName, "%s", rinc_appRef);
      go = 1;
    } else if (pressed & rinc_SysKey_h) { /* this app's help, if it has any */
      if (rinc_hasHelp(rinc_appRef)) { /* (a project's own: its name with the star, then /README) */
        snprintf(rinc_helpTopicNow, sizeof rinc_helpTopicNow, "%.33s/README", rinc_appRef);
        snprintf(rinc_launchName, sizeof rinc_launchName, "help");
        go = 1;
      } else rinc_noticeSay("No help for ", rinc_appRef);
    } else if (pressed & rinc_SysKey_p) rinc_perfOn = !rinc_perfOn;
    else if (pressed & rinc_SysKey_enter) rinc_fullscreen = !rinc_fullscreen;
    rinc_sysLock = 0; /* (used up) */
  } else if (rinc_sysLock && otherPressed && !down) rinc_sysLock = 0; /* (another key: no combination) */
  rinc_systemDown = sysDown || rinc_sysLock || rinc_switching;
  rinc_sysPointer = rinc_systemDown && !rinc_switching ? keys & rinc_SysKey_pointer : 0;
  strip = !go && (rinc_systemDown || rinc_perfOn);
  rinc_overlayOn = 0; /* (while it's redrawn) */
  if (strip || rinc_locks || rinc_sysLock || rinc_noticeOn()) { rinc_overlayDraw(strip); rinc_overlayOn = 1; }
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
static void rinc_hostPointerY(rio_Vm *v, rio_Val *a) { (void)v; a[0].i = (int)rinc_pointerY; }
static void rinc_hostPointerDown(rio_Vm *v, rio_Val *a) { (void)v; a[0].i = a[0].u < 3 ? rinc_pointerHeld >> a[0].i & 1 : 0; }
static void rinc_hostPointerShown(rio_Vm *v, rio_Val *a) { (void)v; a[0].i = rinc_pointerOnScreen; }
static void rinc_hostScroll(rio_Vm *v, rio_Val *a) {
  int k;
  (void)v;
  if (a[0].u > 1) { a[0].i = 0; return; }
  k = (int)rinc_scrolled[a[0].i];
  rinc_scrolled[a[0].i] -= (float)k;
  a[0].i = k;
}

int rinc_hostProcs(rio_Vm *vm) {
  return rio_addFfi(vm, "pc.cls", "i", rinc_hostCls) || rio_addFfi(vm, "pc.pset", "iii", rinc_hostPset) || rio_addFfi(vm, "pc.pget", "ii>i", rinc_hostPget) ||
         rio_addFfi(vm, "pc.rect", "iiiii", rinc_hostRect) || rio_addFfi(vm, "pc.print", "siii", rinc_hostPrint) || rio_addFfi(vm, "pc.btn", "i>i", rinc_hostBtn) || rinc_audioProcs(vm) || rio_addFfi(vm, "pc.time", ">f", rinc_hostTime) ||
         rio_addFfi(vm, "pc.fromHex", "bs>i", rinc_hostFromhex) || rio_addFfi(vm, "pc.stick", "i>f", rinc_hostStick) || rio_addFfi(vm, "pc.trigger", "i>f", rinc_hostTrigger) ||
         rio_addFfi(vm, "pc.key", ">i", rinc_hostKey) || rio_addFfi(vm, "pc.apps", ">i", rinc_hostApps) || rio_addFfi(vm, "pc.appName", "ib>i", rinc_hostAppName) ||
         rio_addFfi(vm, "pc.appInfo", "ib>i", rinc_hostAppInfo) || rio_addFfi(vm, "pc.launch", "s", rinc_hostLaunch) ||
         rio_addFfi(vm, "pc.appSource", "sb>i", rinc_hostAppSource) || rio_addFfi(vm, "pc.appSave", "ss>i", rinc_hostAppSave) ||
         rio_addFfi(vm, "pc.appDelete", "s>i", rinc_hostAppDelete) ||
         rio_addFfi(vm, "pc.helps", "b>i", rinc_hostHelps) ||
         rio_addFfi(vm, "pc.helpRead", "sb>i", rinc_hostHelpRead) || rio_addFfi(vm, "pc.help", "s", rinc_hostHelp) ||
         rio_addFfi(vm, "pc.helpTopic", "b>i", rinc_hostHelpTopic) ||
         rio_addFfi(vm, "pc.stashWrite", "ss>i", rinc_hostStashWrite) || rio_addFfi(vm, "pc.stashRead", "sb>i", rinc_hostStashRead) ||
         rio_addFfi(vm, "pc.stashDelete", "s>i", rinc_hostStashDelete) || rio_addFfi(vm, "pc.boot", ">i", rinc_hostBoot) ||
         rio_addFfi(vm, "pc.settingRead", "sb>i", rinc_hostSettingRead) || rio_addFfi(vm, "pc.settingWrite", "ss>i", rinc_hostSettingWrite) || rio_addFfi(vm, "pc.theme", "i>i", rinc_hostTheme) ||
         rio_addFfi(vm, "pc.pointer", "i", rinc_hostPointer) || rio_addFfi(vm, "pc.appInstall", "s>i", rinc_hostAppInstall) ||
         rio_addFfi(vm, "pc.appCopy", "ss>i", rinc_hostAppCopy) ||
         rio_addFfi(vm, "pc.call", "ss", rinc_hostCall) || rio_addFfi(vm, "pc.args", "b>i", rinc_hostArgs) ||
         rio_addFfi(vm, "pc.done", "s", rinc_hostDone) || rio_addFfi(vm, "pc.result", "b>i", rinc_hostResult) ||
         rio_addFfi(vm, "pc.dir", "sb>i", rinc_hostDir) || rio_addFfi(vm, "pc.fileRead", "sb>i", rinc_hostFileRead) ||
         rio_addFfi(vm, "pc.fileWrite", "ss>i", rinc_hostFileWrite) || rio_addFfi(vm, "pc.fileDelete", "s>i", rinc_hostFileDelete) ||
         rio_addFfi(vm, "pc.fileCopy", "ss>i", rinc_hostFileCopy) || rio_addFfi(vm, "pc.dirMake", "s>i", rinc_hostDirMake) || rio_addFfi(vm, "pc.fileRename", "ss>i", rinc_hostFileRename) || rio_addFfi(vm, "pc.appRename", "ss>i", rinc_hostAppRename) || rio_addFfi(vm, "pc.pointerX", ">i", rinc_hostPointerX) ||
         rio_addFfi(vm, "pc.pointerY", ">i", rinc_hostPointerY) || rio_addFfi(vm, "pc.pointerDown", "i>i", rinc_hostPointerDown) ||
         rio_addFfi(vm, "pc.pointerShown", ">i", rinc_hostPointerShown) || rio_addFfi(vm, "pc.scroll", "i>i", rinc_hostScroll) ||
         rio_addFfi(vm, "pc.saveRead", "sb>i", rinc_hostSaveRead) || rio_addFfi(vm, "pc.saveWrite", "ss>i", rinc_hostSaveWrite) ||
         rio_addFfi(vm, "pc.saveDelete", "s>i", rinc_hostSaveDelete) || rio_addFfi(vm, "pc.rioStart", "bb>i", rinc_hostRioStart) ||
         rio_addFfi(vm, "pc.rioEval", "s>i", rinc_hostRioEval) || rio_addFfi(vm, "pc.rioFrame", ">i", rinc_hostRioFrame) ||
         rio_addFfi(vm, "pc.rioType", "i", rinc_hostRioType) || rio_addFfi(vm, "pc.rioError", "b>i", rinc_hostRioError) ||
         rio_addFfi(vm, "pc.rioLog", "b>i", rinc_hostRioLog) || rio_addFfi(vm, "pc.rioIndent", "s>i", rinc_hostRioIndent);
}

/* a game's frame: update then draw */
/* starting an app, the same on every Rinc: rinc_appBegin, then rinc_appRun (its source compiled, read a
   part at a time from its file, or its image loaded). Whatever goes wrong goes on the screen, with the way home
   (and, when it ran out of something, how near each limit it came) */
static rio_Usage rinc_usage;
int rinc_appBegin(rio_Vm *vm, void *space, uint32_t size, const char *name, void (*put)(const char *s, int n)) {
  rinc_game = 0;
  rinc_showLog = 1;
  rinc_put = put;
  rinc_themeLoad();
  rinc_consoleClear(rinc_themeColor(0));
  if (rinc_start(vm, space, size, put)) { rinc_say("can't start rio"); rinc_back(); return -1; }
  rinc_callStart(name);
  rinc_setAppName(name);
  snprintf(rinc_callee, sizeof rinc_callee, "%s", name); /* (who'd be the caller, if this calls) */
  rinc_projectSaves();
  rinc_recentAdd(name);
  rinc_pointerMode = 0; rinc_scrolled[0] = rinc_scrolled[1] = 0; /* (a game's, until it says) */
  return 0;
}
void rinc_appFail(const char *name, const char *why) {
  rinc_say(name); rinc_say(why); rinc_back();
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
    rinc_say(name); rinc_say(image ? ": " : ".rio:"); rinc_say(rio_error(vm));
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
    rinc_say(name); rinc_say(".rio:"); rinc_say(rio_error(vm)); rinc_back();
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
  int n = (int)strlen(name), project = n > 1 && name[n - 1] == '*';
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
  if (!rinc_game) return 1;
  t0 = rinc_time ? rinc_time() : 0;
  res = rinc_frame(vm);
  rinc_perfNote(rinc_time ? (rinc_time() - t0) * 1000.0f : 0); /* (for the performance strip) */
  if (res != 0) { /* an error, or no update or draw any more */
    rinc_game = 0; rinc_showLog = 1; rinc_audioReset(); rinc_consoleClear(rinc_themeColor(0));
    rinc_say(res < 0 ? rio_error(vm) : "nothing to run: define update* or draw*");
    rinc_back();
  }
  return res;
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
    "import", "include", "enum", "union", "switch", "case", "is", "nil", "and", "or", "not", "true", "false"};
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
static int rinc_replAppPath(char *path, size_t cap, const char *name, int n) { /* (an app's, or a project's: chord*) */
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
  if (n > w + 1 && w == 7 && !memcmp(s, "install", 7)) { /* project name* as the app name */
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
