/* png.c - pictures as PNG files: written indexed, with the palette as their colors (color 0 see-through,
   in tRNS), so other programs show them as Rinc does, and they read back exactly; read from any 8-bit
   PNG (indexed at 1, 2, 4 or 8 bits too), each color the palette's nearest. Small enough for the Fruit
   Jam: 1.2 KB, the inflater's Huffman trees, a read buffer and a color map, which writing (its 512-byte
   buffer) shares, since a picture is read or written at a time.
   Reading inflates into the caller's own memory (out), past where the pixels go, and reverses PNG's
   filters there, a row at a time; writing compresses as it goes, matching runs (the pixel before) and
   rows (the pixel above), which is most of what pixel art repeats, in IDAT chunks of 1 KB. */
#include <string.h>
#include "rinc.h"

/* a Huffman tree, as counts of codes of each length and the symbols in code order (literals and lengths:
   288 of them; distances 30, and the code lengths' code 19) */
typedef struct { uint16_t counts[16], symbols[288]; } rinc_Tree;
typedef struct { uint16_t counts[16], symbols[32]; } rinc_SmallTree;
static union {
  uint8_t out[512]; /* writing: an IDAT chunk's data, filling */
  struct { rinc_Tree lit; rinc_SmallTree dist; uint8_t in[256], map[256]; } r; /* reading */
} rinc_png;
#define rinc_pngOut rinc_png.out
#define rinc_pngIn rinc_png.r.in

/* ------------------------------------------------------------------ writing */

static uint32_t rinc_pngOutLen, rinc_pngBits, rinc_pngBitCount, rinc_pngAdlerA, rinc_pngAdlerB;
static int rinc_pngBad;

static uint32_t rinc_crc(uint32_t crc, const uint8_t *p, uint32_t n) {
  int k;
  crc = ~crc;
  while (n--) {
    crc ^= *p++;
    for (k = 0; k < 8; k++) crc = crc >> 1 ^ (0xEDB88320u & (0u - (crc & 1)));
  }
  return ~crc;
}
static void rinc_put32be(uint8_t *p, uint32_t v) { p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v; }
/* a chunk: its length, type, data and CRC */
static void rinc_pngChunk(const char *type, const uint8_t *data, uint32_t n) {
  uint8_t head[8], tail[4];
  rinc_put32be(head, n);
  memcpy(head + 4, type, 4);
  rinc_put32be(tail, rinc_crc(rinc_crc(0, head + 4, 4), data, n));
  if (rinc_fs->write((const char *)head, 8) != 8 || (n && rinc_fs->write((const char *)data, (int)n) != (int)n) || rinc_fs->write((const char *)tail, 4) != 4)
    rinc_pngBad = 1;
}
static void rinc_pngByte(uint8_t b) {
  rinc_pngOut[rinc_pngOutLen++] = b;
  if (rinc_pngOutLen == sizeof rinc_pngOut) { rinc_pngChunk("IDAT", rinc_pngOut, rinc_pngOutLen); rinc_pngOutLen = 0; }
}
/* n bits of v, low first (deflate's order) */
static void rinc_pngPut(uint32_t v, int n) {
  rinc_pngBits |= v << rinc_pngBitCount;
  rinc_pngBitCount += (uint32_t)n;
  while (rinc_pngBitCount >= 8) { rinc_pngByte((uint8_t)rinc_pngBits); rinc_pngBits >>= 8; rinc_pngBitCount -= 8; }
}
/* a Huffman code of n bits, written high first */
static void rinc_pngCode(uint32_t code, int n) {
  uint32_t r = 0;
  int i;
  for (i = 0; i < n; i++) r |= (code >> i & 1) << (n - 1 - i);
  rinc_pngPut(r, n);
}
/* a literal or length symbol, in deflate's fixed codes */
static void rinc_pngSymbol(int s) {
  if (s < 144) rinc_pngCode((uint32_t)(0x30 + s), 8);
  else if (s < 256) rinc_pngCode((uint32_t)(0x190 + s - 144), 9);
  else if (s < 280) rinc_pngCode((uint32_t)(s - 256), 7);
  else rinc_pngCode((uint32_t)(0xC0 + s - 280), 8);
}
static const uint16_t rinc_lenBase[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
static const uint8_t rinc_lenExtra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
static const uint16_t rinc_distBase[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073,
                                           4097, 6145, 8193, 12289, 16385, 24577};
static const uint8_t rinc_distExtra[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
static void rinc_pngMatch(int len, int dist) {
  int i = 28, j = 29;
  while (rinc_lenBase[i] > len) i--;
  rinc_pngSymbol(257 + i);
  rinc_pngPut((uint32_t)(len - rinc_lenBase[i]), rinc_lenExtra[i]);
  while (rinc_distBase[j] > dist) j--;
  rinc_pngCode((uint32_t)j, 5);
  rinc_pngPut((uint32_t)(dist - rinc_distBase[j]), rinc_distExtra[j]);
}

/* the data PNG compresses: each row, a filter byte (0: none) before it */
static const uint8_t *rinc_pngPixels;
static uint32_t rinc_pngW;
static int rinc_pngData(uint32_t k) { uint32_t col = k % (rinc_pngW + 1); return col ? rinc_pngPixels[k / (rinc_pngW + 1) * rinc_pngW + col - 1] : 0; }

int rinc_pngWrite(const char *path, const uint8_t *pixels, int w, int h) {
  static const uint8_t signature[8] = {137, 'P', 'N', 'G', 13, 10, 26, 10};
  uint8_t head[13], plte[768], trns[1] = {0};
  uint32_t n, k, d, i;
  if (!rinc_fs || w < 1 || h < 1 || w > 4096 || h > 4096 || rinc_fs->open(path, 1)) return -1;
  rinc_pngBad = rinc_fs->write((const char *)signature, 8) != 8;
  rinc_put32be(head, (uint32_t)w);
  rinc_put32be(head + 4, (uint32_t)h);
  head[8] = 8; head[9] = 3; head[10] = head[11] = head[12] = 0; /* (8 bits a pixel, indexed, not interlaced) */
  rinc_pngChunk("IHDR", head, 13);
  for (i = 0; i < 256; i++) { uint32_t c = rinc_rgb((int)i); plte[i * 3] = (uint8_t)(c >> 16); plte[i * 3 + 1] = (uint8_t)(c >> 8); plte[i * 3 + 2] = (uint8_t)c; }
  rinc_pngChunk("PLTE", plte, 768);
  rinc_pngChunk("tRNS", trns, 1); /* (color 0: see-through) */
  rinc_pngPixels = pixels; rinc_pngW = (uint32_t)w;
  rinc_pngOutLen = rinc_pngBits = rinc_pngBitCount = 0; rinc_pngAdlerA = 1; rinc_pngAdlerB = 0;
  rinc_pngByte(0x78); rinc_pngByte(0x01); /* (zlib: deflate, a 32 KB window) */
  rinc_pngPut(1, 1); rinc_pngPut(1, 2); /* (the last block, with fixed codes) */
  n = (uint32_t)h * ((uint32_t)w + 1);
  for (k = 0; k < n;) {
    int best = 0, bestDist = 0, c = rinc_pngData(k), di;
    for (di = 0; di < 2; di++) { /* (the byte before, a run; or the row above) */
      int len = 0;
      d = di ? (uint32_t)w + 1 : 1;
      if (k < d || d > 32768) continue;
      while (k + (uint32_t)len < n && len < 258 && rinc_pngData(k + (uint32_t)len) == rinc_pngData(k + (uint32_t)len - d)) len++;
      if (len > best) { best = len; bestDist = (int)d; }
    }
    if (best >= 3) {
      rinc_pngMatch(best, bestDist);
      for (i = 0; i < (uint32_t)best; i++) {
        rinc_pngAdlerA = (rinc_pngAdlerA + (uint32_t)rinc_pngData(k + i)) % 65521; rinc_pngAdlerB = (rinc_pngAdlerB + rinc_pngAdlerA) % 65521;
      }
      k += (uint32_t)best;
    } else {
      rinc_pngSymbol(c);
      rinc_pngAdlerA = (rinc_pngAdlerA + (uint32_t)c) % 65521; rinc_pngAdlerB = (rinc_pngAdlerB + rinc_pngAdlerA) % 65521;
      k++;
    }
  }
  rinc_pngSymbol(256); /* (the block's end) */
  if (rinc_pngBitCount) rinc_pngPut(0, 8 - (int)rinc_pngBitCount);
  d = rinc_pngAdlerB << 16 | rinc_pngAdlerA;
  rinc_pngByte((uint8_t)(d >> 24)); rinc_pngByte((uint8_t)(d >> 16)); rinc_pngByte((uint8_t)(d >> 8)); rinc_pngByte((uint8_t)d);
  if (rinc_pngOutLen) rinc_pngChunk("IDAT", rinc_pngOut, rinc_pngOutLen);
  rinc_pngChunk("IEND", 0, 0);
  if (rinc_fs->close()) rinc_pngBad = 1;
  return rinc_pngBad ? -1 : 0;
}

/* ------------------------------------------------------------------ reading */

static int rinc_pngInLen, rinc_pngInAt;
static uint32_t rinc_pngLeft; /* bytes left of the IDAT chunk being read */
static int rinc_pngByteIn(void) {
  if (rinc_pngInAt == rinc_pngInLen) {
    rinc_pngInLen = rinc_fs->read((char *)rinc_pngIn, (int)sizeof rinc_pngIn);
    rinc_pngInAt = 0;
    if (rinc_pngInLen <= 0) { rinc_pngInLen = 0; return -1; }
  }
  return rinc_pngIn[rinc_pngInAt++];
}
static uint32_t rinc_png32(void) {
  uint32_t v = 0;
  int i, b;
  for (i = 0; i < 4; i++) { if ((b = rinc_pngByteIn()) < 0) rinc_pngBad = 1; v = v << 8 | (uint32_t)(b & 255); }
  return v;
}
/* the next byte of the compressed data, across IDAT chunks (-1 at their end) */
static int rinc_pngData8(void) {
  while (!rinc_pngLeft) {
    uint32_t len, type;
    rinc_png32(); /* (the CRC of the chunk before) */
    len = rinc_png32(); type = rinc_png32();
    if (rinc_pngBad || type != 0x49444154u) return -1; /* IDAT */
    rinc_pngLeft = len;
  }
  rinc_pngLeft--;
  return rinc_pngByteIn();
}
static uint32_t rinc_inBits, rinc_inCount;
static int rinc_bit(void) {
  int b;
  if (!rinc_inCount) {
    if ((b = rinc_pngData8()) < 0) { rinc_pngBad = 1; b = 0; }
    rinc_inBits = (uint32_t)b; rinc_inCount = 8;
  }
  rinc_inCount--;
  b = (int)(rinc_inBits & 1); rinc_inBits >>= 1;
  return b;
}
static uint32_t rinc_bits(int n) { uint32_t v = 0; int i; for (i = 0; i < n; i++) v |= (uint32_t)rinc_bit() << i; return v; }

/* a tree from its codes' lengths (counts, then symbols: a small tree has the same layout, shorter) */
static void rinc_tree(rinc_Tree *t, const uint8_t *lengths, int n) {
  uint16_t at[16];
  int i, sum = 0;
  memset(t->counts, 0, sizeof t->counts);
  for (i = 0; i < n; i++) t->counts[lengths[i]]++;
  t->counts[0] = 0;
  for (i = 0; i < 16; i++) { at[i] = (uint16_t)sum; sum += t->counts[i]; }
  for (i = 0; i < n; i++) if (lengths[i]) t->symbols[at[lengths[i]]++] = (uint16_t)i;
}
static int rinc_symbol(const rinc_Tree *t) {
  int code = 0, first = 0, index = 0, len;
  for (len = 1; len < 16; len++) {
    int count = t->counts[len];
    code |= rinc_bit();
    if (code - count < first) return t->symbols[index + (code - first)];
    index += count; first += count; first <<= 1; code <<= 1;
  }
  rinc_pngBad = 1;
  return 0;
}
/* the zlib stream, inflated into out from at (back-references reach only what's written from start) */
static int rinc_inflate(uint8_t *out, uint32_t start, uint32_t cap, uint32_t *end) {
  static const uint8_t order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
  uint32_t at = start;
  int last = 0;
  rinc_inCount = 0;
  rinc_bits(16); /* (the zlib header: deflate, its window) */
  while (!last && !rinc_pngBad) {
    uint32_t type;
    last = (int)rinc_bits(1);
    type = rinc_bits(2);
    if (type == 0) { /* stored */
      uint32_t len, i;
      rinc_inCount = 0;
      len = rinc_bits(16); rinc_bits(16);
      for (i = 0; i < len; i++) { int b = rinc_pngData8(); if (b < 0 || at >= cap) return -1; out[at++] = (uint8_t)b; }
      continue;
    }
    if (type == 1) { /* fixed codes */
      uint8_t lengths[288];
      int i;
      for (i = 0; i < 288; i++) lengths[i] = (uint8_t)(i < 144 ? 8 : i < 256 ? 9 : i < 280 ? 7 : 8);
      rinc_tree(&rinc_png.r.lit, lengths, 288);
      for (i = 0; i < 30; i++) lengths[i] = 5;
      rinc_tree((rinc_Tree *)&rinc_png.r.dist, lengths, 30);
    } else if (type == 2) { /* its own codes: their lengths first, in codes of their own */
      uint8_t lengths[320];
      int nlit = (int)rinc_bits(5) + 257, ndist = (int)rinc_bits(5) + 1, ncode = (int)rinc_bits(4) + 4, i;
      memset(lengths, 0, 19);
      for (i = 0; i < ncode; i++) lengths[order[i]] = (uint8_t)rinc_bits(3);
      rinc_tree((rinc_Tree *)&rinc_png.r.dist, lengths, 19);
      for (i = 0; i < nlit + ndist && !rinc_pngBad;) {
        int s = rinc_symbol((const rinc_Tree *)&rinc_png.r.dist), repeat = 0, value = 0;
        if (s < 16) { lengths[i++] = (uint8_t)s; continue; }
        if (s == 16) { if (!i) return -1; value = lengths[i - 1]; repeat = 3 + (int)rinc_bits(2); }
        else if (s == 17) repeat = 3 + (int)rinc_bits(3);
        else repeat = 11 + (int)rinc_bits(7);
        if (i + repeat > nlit + ndist) return -1;
        while (repeat--) lengths[i++] = (uint8_t)value;
      }
      rinc_tree(&rinc_png.r.lit, lengths, nlit);
      rinc_tree((rinc_Tree *)&rinc_png.r.dist, lengths + nlit, ndist);
    } else return -1;
    for (;;) {
      int s = rinc_symbol(&rinc_png.r.lit);
      if (rinc_pngBad) return -1;
      if (s < 256) { if (at >= cap) return -1; out[at++] = (uint8_t)s; }
      else if (s == 256) break;
      else {
        uint32_t len, dist, i;
        s -= 257;
        if (s >= 29) return -1;
        len = rinc_lenBase[s] + rinc_bits(rinc_lenExtra[s]);
        s = rinc_symbol((const rinc_Tree *)&rinc_png.r.dist);
        if (s >= 30) return -1;
        dist = rinc_distBase[s] + rinc_bits(rinc_distExtra[s]);
        if (dist > at - start || len > cap - at) return -1;
        for (i = 0; i < len; i++, at++) out[at] = out[at - dist];
      }
    }
  }
  *end = at;
  return rinc_pngBad ? -1 : 0;
}

/* the palette's nearest color to an RGB one (exactly i, for the palette's own color i, if i is given) */
static uint8_t rinc_nearest(uint32_t c, int i) {
  int k, best = 0;
  uint32_t bestD = 0xFFFFFFFFu;
  if (i >= 0 && rinc_rgb(i) == c) return (uint8_t)i;
  for (k = 0; k < 256; k++) {
    uint32_t p = rinc_rgb(k);
    int dr = (int)(p >> 16) - (int)(c >> 16), dg = (int)(p >> 8 & 255) - (int)(c >> 8 & 255), db = (int)(p & 255) - (int)(c & 255);
    uint32_t d = (uint32_t)(dr * dr * 3 + dg * dg * 4 + db * db * 2);
    if (k && d < bestD) { bestD = d; best = k; } /* (0 is see-through: a color's never 0) */
  }
  return (uint8_t)best;
}
static int rinc_paeth(int a, int b, int c) {
  int p = a + b - c, pa = p > a ? p - a : a - p, pb = p > b ? p - b : b - p, pc = p > c ? p - c : c - p;
  return pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
}

int rinc_pngRead(const char *path, uint8_t *out, uint32_t cap) {
  uint8_t *map = rinc_png.r.map; /* (an indexed PNG's colors, as the palette's) */
  uint32_t w = 0, h = 0, rowBytes, base, end = 0, x, y, len, type;
  int depth = 0, kind = -1, channels, pixelBytes, i, b;
  uint8_t sig[8];
  if (!rinc_fs || !out || rinc_fs->open(path, 0)) return -1;
  rinc_pngBad = 0; rinc_pngInLen = rinc_pngInAt = 0; rinc_pngLeft = 0;
  for (i = 0; i < 8; i++) sig[i] = (uint8_t)rinc_pngByteIn();
  if (memcmp(sig, "\211PNG\r\n\032\n", 8)) { rinc_fs->close(); return -1; }
  for (i = 0; i < 256; i++) map[i] = (uint8_t)i;
  for (;;) { /* chunks, up to the first IDAT */
    len = rinc_png32(); type = rinc_png32();
    if (rinc_pngBad) { rinc_fs->close(); return -1; }
    if (type == 0x49484452u) { /* IHDR */
      uint8_t hd[13];
      for (i = 0; i < 13; i++) hd[i] = (uint8_t)rinc_pngByteIn();
      w = (uint32_t)hd[0] << 24 | (uint32_t)hd[1] << 16 | (uint32_t)hd[2] << 8 | hd[3];
      h = (uint32_t)hd[4] << 24 | (uint32_t)hd[5] << 16 | (uint32_t)hd[6] << 8 | hd[7];
      depth = hd[8]; kind = hd[9];
      if (hd[12] || !w || !h || w > 4096 || h > 4096 || (kind == 3 ? depth > 8 : depth != 8) || (kind != 0 && kind != 2 && kind != 3 && kind != 4 && kind != 6)) {
        rinc_fs->close(); return -1; /* (interlaced, 16 bits, or the like: not read) */
      }
      rinc_png32();
    } else if (type == 0x504C5445u) { /* PLTE: each color, the palette's nearest */
      for (i = 0; i < (int)(len / 3); i++) {
        uint32_t c = (uint32_t)rinc_pngByteIn() << 16; c |= (uint32_t)rinc_pngByteIn() << 8; c |= (uint32_t)rinc_pngByteIn();
        if (i < 256) map[i] = rinc_nearest(c, i);
      }
      for (i = (int)(len / 3) * 3; i < (int)len; i++) rinc_pngByteIn();
      rinc_png32();
    } else if (type == 0x74524E53u && kind == 3) { /* tRNS: see-through colors are 0 */
      for (i = 0; i < (int)len; i++) { b = rinc_pngByteIn(); if (i < 256 && b < 128) map[i] = 0; }
      rinc_png32();
    } else if (type == 0x49444154u) { rinc_pngLeft = len; break; } /* IDAT */
    else { for (x = 0; x < len + 4; x++) rinc_pngByteIn(); } /* (any other: skipped, its CRC too) */
    if (rinc_pngBad || (!len && type == 0x49454E44u)) { rinc_fs->close(); return -1; } /* (IEND, before any IDAT) */
  }
  if (kind < 0) { rinc_fs->close(); return -1; }
  channels = kind == 2 ? 3 : kind == 4 ? 2 : kind == 6 ? 4 : 1;
  rowBytes = (w * (uint32_t)channels * (uint32_t)depth + 7) / 8;
  pixelBytes = (channels * depth + 7) / 8;
  base = rowBytes >= w ? 4 + h : 4 + w * h; /* (the rows as they come, from here: past where pixels go, row for row) */
  if ((uint64_t)base + (uint64_t)h * (rowBytes + 1) > cap) { rinc_fs->close(); return -1; }
  if (rinc_inflate(out, base, cap, &end) || end - base < h * (rowBytes + 1)) { rinc_fs->close(); return -1; }
  rinc_fs->close();
  for (y = 0; y < h; y++) { /* the filters, undone, in place */
    uint8_t *row = out + base + y * (rowBytes + 1), *up = y ? row - (rowBytes + 1) : 0;
    int f = row[0];
    row++;
    if (up) up++;
    for (x = 0; x < rowBytes; x++) {
      int a = x >= (uint32_t)pixelBytes ? row[x - (uint32_t)pixelBytes] : 0, u = up ? up[x] : 0, c = up && x >= (uint32_t)pixelBytes ? up[x - (uint32_t)pixelBytes] : 0;
      row[x] = (uint8_t)(row[x] + (f == 1 ? a : f == 2 ? u : f == 3 ? (a + u) / 2 : f == 4 ? rinc_paeth(a, u, c) : 0));
    }
  }
  for (y = 0; y < h; y++) { /* each row's pixels, as the palette's colors, where they go */
    const uint8_t *row = out + base + y * (rowBytes + 1) + 1;
    uint8_t *to = out + 4 + y * w;
    uint32_t last = 0xFFFFFFFFu; uint8_t lastColor = 0; /* (the color before, often the same) */
    for (x = 0; x < w; x++) {
      uint32_t c;
      if (kind == 3) { uint32_t bit = x * (uint32_t)depth; to[x] = map[row[bit / 8] >> (8 - depth - (int)(bit % 8)) & ((1 << depth) - 1)]; continue; }
      if (kind == 0 || kind == 4) { if (kind == 4 && row[x * 2 + 1] < 128) { to[x] = 0; continue; } c = row[x * (uint32_t)channels] * 0x010101u; }
      else { if (kind == 6 && row[x * 4 + 3] < 128) { to[x] = 0; continue; } c = (uint32_t)row[x * (uint32_t)channels] << 16 | (uint32_t)row[x * (uint32_t)channels + 1] << 8 | row[x * (uint32_t)channels + 2]; }
      if (c != last) { last = c; lastColor = rinc_nearest(c, -1); }
      to[x] = lastColor;
    }
  }
  out[0] = (uint8_t)w; out[1] = (uint8_t)(w >> 8); out[2] = (uint8_t)h; out[3] = (uint8_t)(h >> 8);
  return (int)(4 + w * h);
}
