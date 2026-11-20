/* text on the screen: 53 columns by 26 rows of 6x9 cells in Rinc's font, compact (5x11 glyphs, their top 3 rows left out), a pixel in from the left
   and top (320 = 53 x 6 + 2: a pixel each side; 240 = 26 x 9 + 6: the bottom gets the other 5).
   Writing past the last row scrolls the whole screen up a row, drawings included, as on PICO-8. A
   line cursor marks where the next character goes: a pixel wide, in the gap just before it (each
   cell is 5 pixels of glyph and 1 of gap; at a line's start, the padding).
   It understands the few terminal escapes the REPL's line editor uses, as serial terminals do, so the
   same output works for both: ESC [ n m colors (30-37 and 90-97 as below; 38;5;n palette color n,
   so the PICO-8 colors by number; 0 or 39 back to the text color), ESC [ n D and C cursor left and right (left wraps back a row: long lines wrap), ESC [ K
   blanks the rest of the row, ESC [ J that and the rows below. Others are ignored. */
#include <string.h>
#include "rinc.h"

#define rinc_CW 6
#define rinc_CH 9
#define rinc_COLS (rinc_W / rinc_CW)
#define rinc_ROWS (rinc_H / rinc_CH)
#define rinc_X0 ((rinc_W - rinc_COLS * rinc_CW) / 2) /* (1: the padding) */
#define rinc_Y0 rinc_X0
#define rinc_CURSOR 10 /* yellow: clear on the dark blue, and apart from the text and highlighting colors */

static int rinc_cx, rinc_cy, rinc_shown; /* shown: the cursor is drawn */
uint8_t rinc_textFg = 7, rinc_textBg = 0;
static uint8_t rinc_ink = 7, rinc_under[rinc_CH]; /* ink: color now; under: the pixels beneath the cursor */
static int rinc_esc; /* escapes: 0 none, 1 ESC seen, 2 in ESC [ */
/* ANSI colors 0-7 (30-37) and bright (90-97), as their nearest PICO-8 colors: black, red, green,
   yellow, blue, pink (for magenta), blue (for cyan), light gray; bright: dark gray, red, green,
   yellow, blue, pink, blue, white */
static const uint8_t rinc_ansi[16] = {0, 8, 11, 10, 12, 14, 12, 6, 5, 8, 11, 10, 12, 14, 12, 7};
static int rinc_escp[3], rinc_nescp; /* an escape's numbers so far */

/* Rinc's own font: a sheet like any user font (font.c: rinc-font.png's bytes, 1 bit, its rows uncompressed),
   read where it lies: its first row's pixels, a row's step (its pixels, and the next row's stored block header
   and filter byte), and its cells as its notes say ("sprite 5x11", "font 32 255", "across 16"). Fixed width:
   a glyph and a gap column, rinc_CW. Its top: the rows above every ASCII glyph (the accents' room, over
   capitals), which a compact line leaves out: 11 tall, its top 3, a line of 9 (rinc_CH, the console's) */
static const uint8_t *rinc_fontRows;
static uint32_t rinc_fontStep;
static int rinc_fontW = 5, rinc_fontH = 8, rinc_fontFirst = 32, rinc_fontLast = 126, rinc_fontAcross = 16, rinc_fontTop;
static int rinc_noted(const uint8_t *t, uint32_t n, const char *key, int *a, int *b) { /* "key A" or "key AxB", "key A B" */
  uint32_t i, k = (uint32_t)strlen(key);
  for (i = 0; i + k < n; i++) if ((i == 0 || t[i - 1] == '\n' || t[i - 1] == 0) && !memcmp(t + i, key, k) && t[i + k] == ' ') {
    int v = 0, w = 0, two = 0;
    for (i += k + 1; i < n && t[i] != '\n'; i++) {
      if (t[i] >= '0' && t[i] <= '9') { if (two) w = w * 10 + t[i] - '0'; else v = v * 10 + t[i] - '0'; }
      else two = 1;
    }
    *a = v; if (b) *b = w;
    return 1;
  }
  return 0;
}
static int rinc_fontRowAt(int c, int y, int x) { /* (its glyph's pixel x, y: c a character it has) */
  int g = c - rinc_fontFirst, sx = g % rinc_fontAcross * rinc_fontW + x;
  return rinc_fontRows[(uint32_t)(g / rinc_fontAcross * rinc_fontH + y) * rinc_fontStep + (uint32_t)(sx >> 3)] >> (7 - (sx & 7)) & 1;
}
static void rinc_fontFind(void) {
  uint32_t at = 8, w = 0;
  int c, y, x, top;
  while (at + 8 <= rinc_fontSheetLen) {
    uint32_t len = (uint32_t)rinc_fontSheet[at] << 24 | (uint32_t)rinc_fontSheet[at + 1] << 16 | (uint32_t)rinc_fontSheet[at + 2] << 8 | rinc_fontSheet[at + 3];
    const uint8_t *t = rinc_fontSheet + at + 4;
    if (!memcmp(t, "IHDR", 4)) w = (uint32_t)t[4] << 24 | (uint32_t)t[5] << 16 | (uint32_t)t[6] << 8 | t[7];
    if (!memcmp(t, "tEXt", 4)) {
      rinc_noted(t + 4, len, "sprite", &rinc_fontW, &rinc_fontH);
      rinc_noted(t + 4, len, "font", &rinc_fontFirst, &rinc_fontLast);
      rinc_noted(t + 4, len, "across", &rinc_fontAcross, 0);
    }
    if (!memcmp(t, "IDAT", 4)) { rinc_fontStep = (w + 7) / 8 + 6; rinc_fontRows = t + 4 + 2 + 6; break; } /* (zlib's header; a row's block header and filter) */
    at += 12 + len;
  }
  if (rinc_fontW < 1 || rinc_fontW > rinc_CW - 1) rinc_fontW = rinc_CW - 1; /* (to 9: one 16-bit window a row) */
  if (rinc_fontAcross < 1) rinc_fontAcross = 16;
  for (top = rinc_fontH, c = 33; rinc_fontRows && c < 127; c++)
    for (y = 0; y < top && c >= rinc_fontFirst && c <= rinc_fontLast; y++)
      for (x = 0; x < rinc_fontW; x++) if (rinc_fontRowAt(c, y, x)) { top = y; break; }
  rinc_fontTop = top < rinc_fontH ? top : 0;
}
/* its glyphs' height and the rows above the ASCII ones (print: a line shorter than the full one, height + 1,
   leaves out as many of those as it needs) */
int rinc_fontHeight(void) { if (!rinc_fontRows) rinc_fontFind(); return rinc_fontH; }
int rinc_fontTopRows(void) { if (!rinc_fontRows) rinc_fontFind(); return rinc_fontTop; }
/* pixel x, y of character c's cell, compact (from its top), in Rinc's own font: 1 if it's drawn (a character it
   hasn't: a space) */
int rinc_fontBit(int c, int x, int y) {
  if (!rinc_fontRows) rinc_fontFind();
  if (c <= rinc_fontFirst || c > rinc_fontLast) c = rinc_fontFirst;
  if (!rinc_fontRows || x < 0 || x >= rinc_fontW || y < 0 || y + rinc_fontTop >= rinc_fontH) return 0;
  return rinc_fontRowAt(c, y + rinc_fontTop, x);
}
/* character c (a code point: one the font hasn't, a space) at pixel x, y, in fg: rows of its cell from row
   top, its glyph's, then gap; bg < 0 leaves the cell's other pixels alone. A row's cell is read as one 16-bit
   window (a row's next bytes are still the PNG's), as fast as the old table: a pixel at a time was 4x slower */
void rinc_charRows(int c, int x, int y, int fg, int bg, int top, int rows) {
  int r, k, g, cx, sh;
  const uint8_t *row;
  if (!rinc_fontRows) rinc_fontFind();
  if (!rinc_fontRows) return;
  if (c <= rinc_fontFirst || c > rinc_fontLast) c = rinc_fontFirst;
  g = c - rinc_fontFirst; cx = g % rinc_fontAcross * rinc_fontW; sh = 16 - (cx & 7) - rinc_fontW;
  row = rinc_fontRows + (uint32_t)(g / rinc_fontAcross * rinc_fontH + top) * rinc_fontStep + (uint32_t)(cx >> 3);
  for (r = 0; r < rows; r++, row += rinc_fontStep) {
    int py = y + r, bits = top + r < rinc_fontH ? (row[0] << 8 | row[1]) >> sh : 0;
    if (py < 0 || py >= rinc_appH) continue;
    for (k = 0; k < rinc_CW; k++) {
      int px = x + k;
      if (px < 0 || px >= rinc_appW) continue;
      if (k < rinc_fontW && bits >> (rinc_fontW - 1 - k) & 1) rinc_screen[py][px] = (uint8_t)fg;
      else if (bg >= 0) rinc_screen[py][px] = (uint8_t)bg;
    }
  }
}
/* the console's: compact, a line of rinc_CH */
void rinc_char(int c, int x, int y, int fg, int bg) {
  if (!rinc_fontRows) rinc_fontFind();
  rinc_charRows(c, x, y, fg, bg, rinc_fontTop, rinc_CH);
}

static void rinc_cursor(int on) { /* a line in its own color, with the pixels beneath kept */
  int r, x = rinc_X0 + rinc_cx * rinc_CW - 1;
  if (on == rinc_shown) return;
  for (r = 0; r < rinc_CH - 1 && x >= 0; r++) { /* (as tall as the glyphs: the cell's last row is the gap below) */
    uint8_t *p = &rinc_screen[rinc_Y0 + rinc_cy * rinc_CH + r][x];
    if (on) { rinc_under[r] = *p; *p = rinc_CURSOR; } else *p = rinc_under[r];
  }
  rinc_shown = on;
}
static void rinc_blankRowFrom(int x, int y) { int r; for (r = 0; r < rinc_CH; r++) memset(&rinc_screen[rinc_Y0 + y * rinc_CH + r][rinc_X0 + x * rinc_CW], rinc_textBg, (size_t)(rinc_COLS - x) * rinc_CW); }
static void rinc_escape(int c) { /* the end of ESC [ n ; ... c */
  int n = rinc_escp[0], y;
  if (c == 'm') {
    if (n == 38 && rinc_nescp == 3 && rinc_escp[1] == 5) rinc_ink = (uint8_t)rinc_escp[2];
    else if (n == 0 || n == 39) rinc_ink = rinc_textFg;
    else if (n >= 30 && n <= 37) rinc_ink = rinc_ansi[n - 30];
    else if (n >= 90 && n <= 97) rinc_ink = rinc_ansi[n - 90 + 8];
  } else if (c == 'D') {
    for (n = n ? n : 1; n > 0; n--) { if (rinc_cx > 0) rinc_cx--; else if (rinc_cy > 0) { rinc_cy--; rinc_cx = rinc_COLS - 1; } }
  } else if (c == 'C') {
    for (n = n ? n : 1; n > 0; n--) { if (rinc_cx < rinc_COLS - 1) rinc_cx++; else if (rinc_cy < rinc_ROWS - 1) { rinc_cy++; rinc_cx = 0; } }
  } else if (c == 'K') rinc_blankRowFrom(rinc_cx, rinc_cy);
  else if (c == 'J') { rinc_blankRowFrom(rinc_cx, rinc_cy); for (y = rinc_cy + 1; y < rinc_ROWS; y++) rinc_blankRowFrom(0, y); }
}

static void rinc_newline(void) {
  rinc_cx = 0;
  if (++rinc_cy < rinc_ROWS) return;
  rinc_cy = rinc_ROWS - 1;
  memmove(rinc_screen[0], rinc_screen[rinc_CH], (size_t)(rinc_H - rinc_CH) * rinc_W);
  memset(rinc_screen[rinc_Y0 + (rinc_ROWS - 1) * rinc_CH], rinc_textBg, (size_t)(rinc_H - rinc_Y0 - (rinc_ROWS - 1) * rinc_CH) * rinc_W); /* (the last row, and the padding below) */
}

void rinc_consolePut(const char *s, int n) {
  rinc_cursor(0);
  while (n-- > 0) {
    int c = (unsigned char)*s++;
    if (rinc_esc == 1) { rinc_esc = c == '[' ? 2 : 0; rinc_escp[0] = rinc_escp[1] = rinc_escp[2] = 0; rinc_nescp = 1; continue; }
    if (rinc_esc == 2) {
      if (c >= '0' && c <= '9') rinc_escp[rinc_nescp - 1] = rinc_escp[rinc_nescp - 1] * 10 + c - '0';
      else if (c == ';') { if (rinc_nescp < 3) rinc_nescp++; }
      else { rinc_escape(c); rinc_esc = 0; }
      continue;
    }
    if (c == 27) { rinc_esc = 1; continue; }
    if (c == '\n') rinc_newline();
    else if (c == '\r') rinc_cx = 0;
    else if (c == '\b') { if (rinc_cx > 0) rinc_cx--; }
    else if (c >= 32) {
      if (rinc_cx >= rinc_COLS) rinc_newline();
      rinc_char(c, rinc_X0 + rinc_cx * rinc_CW, rinc_Y0 + rinc_cy * rinc_CH, rinc_ink, rinc_textBg);
      rinc_cx++;
    }
  }
  if (rinc_cx >= rinc_COLS) rinc_newline();
  rinc_cursor(1);
}

void rinc_consoleClear(int bg) {
  rinc_setZoom(0); /* (the console's text, full size: and each app starts so) */
  rinc_textBg = (uint8_t)bg; rinc_ink = rinc_textFg; rinc_esc = 0;
  memset(rinc_screen, bg, sizeof rinc_screen);
  rinc_cx = rinc_cy = 0; rinc_shown = 0;
  rinc_cursor(1);
}
