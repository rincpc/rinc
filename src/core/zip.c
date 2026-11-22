/* Bundles of apps, as zips (a release's rinc-system-apps.zip), installed into storage: rinc_zipInstall.
   The zip is read where the platform has it (a file on the desktop, the page's memory on the web), a
   piece at a time, and each file's inflated into the caller's room (work), so nothing's held but the
   file in hand: no zip library, and no memory of its own. And the stick: a zip mounted, at /stick (below). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rinc.h"

static uint32_t rinc_le16(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8; }
static uint32_t rinc_le32(const uint8_t *p) { return rinc_le16(p) | rinc_le16(p + 2) << 16; }
static uint32_t rinc_crc32(const uint8_t *p, uint32_t n) {
  uint32_t c = 0xFFFFFFFFu, i; int k;
  for (i = 0; i < n; i++) { c ^= p[i]; for (k = 0; k < 8; k++) c = c >> 1 ^ (0xEDB88320u & (0u - (c & 1))); }
  return ~c;
}
static void rinc_zipTell(rinc_Zip *z, const char *format, const char *a) {
  char text[600];
  snprintf(text, sizeof text, format, z->name, a);
  z->tell(1, text);
}
/* (the file in hand, read on from where it's got to, for rinc_inflateRaw) */
static rinc_Zip *rinc_zipNow;
static uint32_t rinc_zipAt;
static int rinc_zipRead(char *buf, int n) {
  int got = rinc_zipNow->read(rinc_zipAt, (uint8_t *)buf, (uint32_t)n);
  if (got > 0) rinc_zipAt += (uint32_t)got;
  return got;
}

static int rinc_zipFiles(rinc_Zip *z);
int rinc_zipInstall(rinc_Zip *z) { /* (with no room given, each file's malloc'd: rinc_zipFiles) */
  int bad;
  if (z->work) return rinc_zipFiles(z);
#ifdef rio_METER
  if (!(z->work = (uint8_t *)malloc(65557))) return 1;
  z->cap = 0; /* (0 with room: each file malloc'd) */
  bad = rinc_zipFiles(z);
  free(z->work); z->work = 0;
#else
  bad = 1;
#endif
  return bad;
}
static int rinc_zipFiles(rinc_Zip *z) {
  uint8_t head[46]; uint32_t back, entries = 0, dirSize = 0, dirAt = 0, end, i, at; int found = 0, pass, bad = 0, k, each = !z->cap;
  z->files = 0; z->apps[0] = 0;
  back = z->size < 65557 ? z->size : 65557; /* (its directory's end, in its last 64 KB: the longest comment) */
  if (!each && back > z->cap) back = z->cap;
  if (back < 22 || z->read(z->size - back, z->work, back) != (int)back) { rinc_zipTell(z, "rinc: %s isn't a zip%s\n", ""); return 1; }
  for (k = (int)back - 22; k >= 0 && !found; k--) if (rinc_le32(z->work + k) == 0x06054B50u) {
    found = 1; entries = rinc_le16(z->work + k + 10); dirSize = rinc_le32(z->work + k + 12); dirAt = rinc_le32(z->work + k + 16);
  }
  if (!found || dirAt > z->size || dirSize > z->size - dirAt) { rinc_zipTell(z, "rinc: %s isn't a zip%s\n", ""); return 1; }
  end = dirAt + dirSize;
  for (pass = 0; pass < 2 && !bad; pass++) /* (names checked, then files written) */
  for (i = 0, at = dirAt; i < entries && !bad; i++) {
    uint8_t local[30]; char name[260], to[300]; const char *rel; uint32_t method, csize, usize, crc, nameLen, skip, local0, got;
    if (at + 46 > end || z->read(at, head, 46) != 46 || rinc_le32(head) != 0x02014B50u) { rinc_zipTell(z, "rinc: %s's directory is damaged%s\n", ""); bad = 1; break; }
    method = rinc_le16(head + 10); crc = rinc_le32(head + 16); csize = rinc_le32(head + 20); usize = rinc_le32(head + 24);
    nameLen = rinc_le16(head + 28); skip = rinc_le16(head + 30) + rinc_le16(head + 32); local0 = rinc_le32(head + 42);
    if (at + 46 + nameLen > end || nameLen > 200 || z->read(at + 46, (uint8_t *)name, nameLen) != (int)nameLen) {
      rinc_zipTell(z, "rinc: %s's directory is damaged (or names a file at length)%s\n", ""); bad = 1; break;
    }
    name[nameLen] = 0;
    at += 46 + nameLen + skip;
    rel = z->into || strncmp(name, "rinc/", 5) ? name : name + 5;
    if (!*rel || rel[strlen(rel) - 1] == '/' || (!z->into && strncmp(rel, "apps/", 5) && strncmp(rel, "help/", 5) && strncmp(rel, "lib/", 4))) continue; /* (folders, and but for a folder of its own, anything but apps, help and the library) */
    if (strstr(rel, "..") || strchr(rel, '\\') || strchr(rel, ':') || strlen(name) != nameLen) { rinc_zipTell(z, "rinc: %s names %s: not installed\n", name); bad = 1; break; }
    if (method != 0 && method != 8) { rinc_zipTell(z, "rinc: %s: %s is packed a way Rinc doesn't read (stored or deflated only)\n", name); bad = 1; break; }
    if ((!each && usize > z->cap) || (!method && csize != usize)) { rinc_zipTell(z, "rinc: %s: %s is too big to install here\n", name); bad = 1; break; }
    if (!pass) continue;
    { uint8_t *room = z->work; /* (each file's own room, malloc'd, or the room given) */
#ifdef rio_METER
    if (each && !(room = (uint8_t *)malloc(usize ? usize : 1))) { rinc_zipTell(z, "rinc: %s: %s is too big to install here\n", name); bad = 1; break; }
#endif
    if (z->read(local0, local, 30) != 30 || rinc_le32(local) != 0x04034B50u) { rinc_zipTell(z, "rinc: %s: can't read %s\n", name); bad = 1; break; }
    rinc_zipNow = z; rinc_zipAt = local0 + 30 + rinc_le16(local + 26) + rinc_le16(local + 28);
    got = method ? (uint32_t)rinc_inflateRaw(rinc_zipRead, csize, room, usize) : (uint32_t)rinc_zipRead((char *)room, (int)usize);
    if (got != usize || rinc_crc32(room, usize) != crc) { rinc_zipTell(z, "rinc: %s: %s is damaged\n", name); bad = 1; }
    else {
      snprintf(to, sizeof to, "%s/%s", z->into ? z->into : "/rinc", rel);
      if (rinc_fs->open(to, 1) || (usize && rinc_fs->write((const char *)room, (int)usize) != (int)usize) || rinc_fs->close()) { rinc_zipTell(z, "rinc: %s: can't write %s\n", to); bad = 1; }
    }
    if (room != z->work) free(room);
    if (bad) break; }
    z->files++;
    if (!z->into && !strncmp(rel, "apps/", 5)) { /* (each app's name, once, to say what came) */
      char app[48], key[52]; const char *slash = strchr(rel + 5, '/');
      snprintf(app, sizeof app, "%.*s", slash ? (int)(slash - rel - 5) : 40, rel + 5);
      snprintf(key, sizeof key, " %s ", app);
      size_t n = strlen(z->apps);
      if (!strstr(z->apps, key) && n + strlen(key) < sizeof z->apps) snprintf(z->apps + n, sizeof z->apps - n, "%s", n ? key + 1 : key);
    }
  }
  return bad;
}

/* after a drop (bad: nothing installed): home, if there is one now (rinc_launch), else what to do */
void rinc_dropped(int bad) {
  if (bad) rinc_say("(nothing installed: drop a bundle of apps, a .zip such as a release's rinc-system-apps.zip)\r\n");
  else if (rinc_hasHome()) {
    snprintf(rinc_launchName, sizeof rinc_launchName, "home");
    rinc_launch = 1;
  } else rinc_say("(no home app yet: the home app's in rinc-system-apps.zip, from a release)\r\n");
}

#ifdef rio_METER
/* a folder, all of it, through rinc_fs (the desktop and the web: the stick's commands): its files' paths
   under it (and folders with nothing in them, ending in /), gathered a folder at a time */
typedef struct { char **names; int n, cap; const char *rel; int any; } rinc_Tree;
static int rinc_treeAdd(rinc_Tree *t, const char *name) {
  if (t->n == t->cap) {
    int cap = t->cap ? t->cap * 2 : 64; char **g = (char **)realloc(t->names, (size_t)cap * sizeof *g);
    if (!g) return -1;
    t->names = g; t->cap = cap;
  }
  if (!(t->names[t->n] = (char *)malloc(strlen(name) + 1))) return -1;
  memcpy(t->names[t->n++], name, strlen(name) + 1);
  return 0;
}
static void rinc_treeFound(const char *name, int dir, void *ud) {
  rinc_Tree *t = (rinc_Tree *)ud; char rel[300];
  if (!strcmp(name, ".folder")) return; /* (the web's mark of a folder) */
  t->any = 1;
  snprintf(rel, sizeof rel, "%s%s%s%s", t->rel, *t->rel ? "/" : "", name, dir ? "/" : "");
  rinc_treeAdd(t, rel);
}
static void rinc_treeFree(rinc_Tree *t) { int i; for (i = 0; i < t->n; i++) free(t->names[i]); free(t->names); }
/* all under path: files, and folders (ending /) to look in, looked in till none's left */
static int rinc_treeGather(const char *path, rinc_Tree *t) {
  int i; char full[600];
  memset(t, 0, sizeof *t); t->rel = "";
  if (rinc_fs->list(path, rinc_treeFound, t)) return -1;
  for (i = 0; i < t->n; i++) {
    size_t k = strlen(t->names[i]);
    if (!k || t->names[i][k - 1] != '/') continue;
    t->names[i][k - 1] = 0; t->rel = t->names[i]; t->any = 0;
    snprintf(full, sizeof full, "%s/%s", path, t->names[i]);
    rinc_fs->list(full, rinc_treeFound, t);
    t->names[i][k - 1] = '/';
    if (t->any) t->names[i][0] = 0; /* (it has things: not kept as a folder of its own) */
    t->rel = "";
  }
  return 0;
}
typedef struct { char **names; int *dirs, n, cap; } rinc_Level;
static void rinc_levelFound(const char *name, int dir, void *ud) {
  rinc_Level *l = (rinc_Level *)ud;
  if (l->n == l->cap) {
    int cap = l->cap ? l->cap * 2 : 16; char **g = (char **)realloc(l->names, (size_t)cap * sizeof *g); int *d;
    if (!g) return;
    l->names = g;
    if (!(d = (int *)realloc(l->dirs, (size_t)cap * sizeof *d))) return;
    l->dirs = d; l->cap = cap;
  }
  if ((l->names[l->n] = (char *)malloc(strlen(name) + 1))) { memcpy(l->names[l->n], name, strlen(name) + 1); l->dirs[l->n++] = dir; }
}
int rinc_treeDelete(const char *path) { /* (what's in it, each folder's after its own, then it: the web's .folder marks too) */
  rinc_Level l; int i; char full[600];
  memset(&l, 0, sizeof l);
  if (rinc_fs->list(path, rinc_levelFound, &l)) return -1;
  for (i = 0; i < l.n; i++) {
    snprintf(full, sizeof full, "%s/%s", path, l.names[i]);
    if (l.dirs[i]) rinc_treeDelete(full); else rinc_fs->remove(full);
    free(l.names[i]);
  }
  free(l.names); free(l.dirs);
  snprintf(full, sizeof full, "%s/.folder", path); rinc_fs->remove(full);
  rinc_fs->remove(path);
  return 0;
}
static void rinc_put16(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void rinc_put32le(uint8_t *p, uint32_t v) { rinc_put16(p, v & 0xFFFF); rinc_put16(p + 2, v >> 16); }
uint8_t *rinc_treeZip(const char *path, uint32_t *size) {
  rinc_Tree t; uint8_t **data = 0, *z = 0, *p; uint32_t *lens = 0, *where = 0, at = 0, dir, n = 0, kept = 0; uint64_t total = 22; int i, bad = 0;
  char full[600];
  if (rinc_treeGather(path, &t)) return 0;
  data = (uint8_t **)calloc((size_t)t.n + 1, sizeof *data); lens = (uint32_t *)calloc((size_t)t.n + 1, sizeof *lens); where = (uint32_t *)calloc((size_t)t.n + 1, sizeof *where);
  if (!data || !lens || !where) bad = 1;
  for (i = 0; i < t.n && !bad; i++) { /* (each file read whole) */
    size_t k = strlen(t.names[i]); int32_t sz;
    if (!k) continue;
    kept++;
    total += 30 + 46 + 2ull * k;
    if (t.names[i][k - 1] == '/') continue;
    snprintf(full, sizeof full, "%s/%s", path, t.names[i]);
    if (rinc_fs->open(full, rinc_READ) || (sz = rinc_fs->size()) < 0 || !(data[i] = (uint8_t *)malloc(sz ? (size_t)sz : 1)) ||
        (sz && rinc_fs->read((char *)data[i], (int)sz) != (int)sz)) bad = 1;
    rinc_fs->close();
    lens[i] = sz < 0 ? 0 : (uint32_t)sz; total += lens[i];
  }
  if (!bad && total <= 0xFFFFFFFFu && kept < 65535 && (z = (uint8_t *)malloc((size_t)total))) {
    for (i = 0; i < t.n; i++) { /* (each: its local header, then its bytes, stored) */
      size_t k = strlen(t.names[i]); uint32_t crc;
      if (!k) continue;
      crc = rinc_crc32(data[i] ? data[i] : (const uint8_t *)"", lens[i]);
      p = z + at; memset(p, 0, 30); where[i] = at;
      rinc_put32le(p, 0x04034B50u); rinc_put16(p + 4, 20); rinc_put32le(p + 14, crc); rinc_put32le(p + 18, lens[i]); rinc_put32le(p + 22, lens[i]); rinc_put16(p + 26, (uint32_t)k);
      memcpy(p + 30, t.names[i], k);
      if (lens[i]) memcpy(p + 30 + k, data[i], lens[i]);
      at += 30 + (uint32_t)k + lens[i];
    }
    dir = at;
    for (i = 0; i < t.n; i++) { /* (the directory: each again, where it is) */
      size_t k = strlen(t.names[i]); uint8_t *local = z + where[i];
      if (!k) continue;
      p = z + dir + n; memset(p, 0, 46);
      rinc_put32le(p, 0x02014B50u); rinc_put16(p + 4, 20); rinc_put16(p + 6, 20);
      memcpy(p + 16, local + 14, 12); rinc_put16(p + 28, (uint32_t)k);
      if (t.names[i][k - 1] == '/') rinc_put32le(p + 38, 0x10); /* (a folder) */
      rinc_put32le(p + 42, where[i]);
      memcpy(p + 46, t.names[i], k);
      n += 46 + (uint32_t)k;
    }
    p = z + dir + n; memset(p, 0, 22);
    rinc_put32le(p, 0x06054B50u); rinc_put16(p + 8, kept); rinc_put16(p + 10, kept); rinc_put32le(p + 12, n); rinc_put32le(p + 16, dir);
    *size = dir + n + 22;
  }
  for (i = 0; i < t.n; i++) if (data) free(data[i]);
  free(data); free(lens); free(where); rinc_treeFree(&t);
  return z;
}
#endif

#if rinc_ZIP_STICK
/* the stick (plans/drives.md), on the desktop and the web: a zip dropped (or rinc -stick), or a new empty
   one, held in memory and written to there, at /stick: its files and folders, as storage's are. It's put
   in front of the platform's storage (rinc_fs): /stick's paths go to it, the rest through to the
   platform's, and copying between the two reads one and writes the other. "/" lists rinc, and stick while
   one's in. A file from the zip stays as it came (deflated, read where it is) till it's changed; one
   written is kept whole. rinc_stickZipped writes it all out as a zip again (to download) */
typedef struct {
  char *name; uint32_t nameLen;
  const uint8_t *packed; uint32_t csize, usize, crc; int deflated; /* (from the zip: where it is in it) */
  uint8_t *own; int folder; /* (written here: its bytes; or an empty folder made here) */
} rinc_StickFile;
static uint8_t *rinc_stickZip; static rinc_StickFile *rinc_stickFiles; static int rinc_stickCount, rinc_stickCap, rinc_stickIsIn;
static char rinc_stickName[64];
static const rinc_Fs *rinc_card; /* (the platform's storage, behind) */
/* the file open on it: read whole into data (or written into it, till it's closed: then kept as name) */
static uint8_t *rinc_stickData; static uint32_t rinc_stickLen, rinc_stickPos, rinc_stickRoom;
static int rinc_stickOpen, rinc_stickOwned, rinc_stickWriting; static char rinc_stickWrite[300];
static char rinc_stickDir[512], rinc_folderName[64]; /* (a folder as the stick: its paths are the platform's, files.c's) */
/* a path on the zip stick: the rest of it ("" for /stick itself), or 0 (and with a folder in, the platform's) */
static const char *rinc_onStick(const char *path) {
  if (!rinc_stickIsIn || strncmp(path, "/stick", 6) || (path[6] && path[6] != '/')) return 0;
  return path[6] ? path + 7 : path + 6;
}
static int rinc_stickFind(const char *rel, int folder) { /* (a file, or an empty folder made here) */
  int i; size_t n = strlen(rel);
  for (i = 0; i < rinc_stickCount; i++) if (rinc_stickFiles[i].folder == folder && rinc_stickFiles[i].nameLen == n && !memcmp(rinc_stickFiles[i].name, rel, n)) return i;
  return -1;
}
static int rinc_stickAdd(const char *rel, size_t n) { /* (a new entry, its name copied: its index, or -1) */
  rinc_StickFile *f;
  if (rinc_stickCount == rinc_stickCap) {
    int cap = rinc_stickCap ? rinc_stickCap * 2 : 32;
    rinc_StickFile *g = (rinc_StickFile *)realloc(rinc_stickFiles, (size_t)cap * sizeof *g);
    if (!g) return -1;
    rinc_stickFiles = g; rinc_stickCap = cap;
  }
  f = &rinc_stickFiles[rinc_stickCount];
  memset(f, 0, sizeof *f);
  if (!(f->name = (char *)malloc(n + 1))) return -1;
  memcpy(f->name, rel, n); f->name[n] = 0; f->nameLen = (uint32_t)n;
  return rinc_stickCount++;
}
static void rinc_stickDrop(int i) { /* (entry i gone) */
  free(rinc_stickFiles[i].name); free(rinc_stickFiles[i].own);
  rinc_stickFiles[i] = rinc_stickFiles[--rinc_stickCount];
}
static const uint8_t *rinc_inflateFrom; static uint32_t rinc_inflateLeft;
static int rinc_inflateRead(char *buf, int n) {
  uint32_t k = (uint32_t)n < rinc_inflateLeft ? (uint32_t)n : rinc_inflateLeft;
  memcpy(buf, rinc_inflateFrom, k); rinc_inflateFrom += k; rinc_inflateLeft -= k;
  return (int)k;
}
static void rinc_stickShut(void) {
  if (rinc_stickOwned) free(rinc_stickData);
  rinc_stickData = 0; rinc_stickOpen = rinc_stickOwned = rinc_stickWriting = 0;
}
/* file i's bytes into rinc_stickData (whole): 0, or -1 */
static int rinc_stickLoad(int i) {
  rinc_StickFile *f = &rinc_stickFiles[i];
  rinc_stickShut();
  if (f->own || !f->deflated) rinc_stickData = f->own ? f->own : (uint8_t *)f->packed;
  else {
    if (!(rinc_stickData = (uint8_t *)malloc(f->usize ? f->usize : 1))) return -1;
    rinc_stickOwned = 1;
    rinc_inflateFrom = f->packed; rinc_inflateLeft = f->csize;
    if ((uint32_t)rinc_inflateRaw(rinc_inflateRead, f->csize, rinc_stickData, f->usize) != f->usize || rinc_crc32(rinc_stickData, f->usize) != f->crc) { rinc_stickShut(); return -1; } /* (damaged: not read) */
  }
  rinc_stickLen = f->usize; rinc_stickPos = 0; rinc_stickOpen = 1;
  return 0;
}
static int rinc_vOpen(const char *path, int how) {
  const char *rel = rinc_onStick(path); int i;
  rinc_stickShut();
  if (!rel) return rinc_card->open(path, how);
  if (!rinc_stickIsIn || !*rel) return -1;
  i = rinc_stickFind(rel, 0);
  if (how == rinc_READ) return i < 0 ? -1 : rinc_stickLoad(i);
  if (rinc_stickFind(rel, 1) >= 0 || strlen(rel) >= sizeof rinc_stickWrite) return -1; /* (a folder's name) */
  if (how == rinc_UPDATE && i >= 0) { /* (its bytes so far, to write over) */
    uint8_t *copy;
    if (rinc_stickLoad(i) || !(copy = (uint8_t *)malloc(rinc_stickLen ? rinc_stickLen : 1))) { rinc_stickShut(); return -1; }
    memcpy(copy, rinc_stickData, rinc_stickLen);
    if (rinc_stickOwned) free(rinc_stickData);
    rinc_stickData = copy; rinc_stickRoom = rinc_stickLen ? rinc_stickLen : 1;
  } else {
    if (!(rinc_stickData = (uint8_t *)malloc(256))) return -1;
    rinc_stickLen = 0; rinc_stickRoom = 256;
  }
  rinc_stickOwned = rinc_stickOpen = rinc_stickWriting = 1; rinc_stickPos = 0;
  snprintf(rinc_stickWrite, sizeof rinc_stickWrite, "%s", rel);
  return 0;
}
static int rinc_vRead(char *buf, int n) {
  uint32_t k;
  if (!rinc_stickOpen) return rinc_card->read(buf, n);
  k = n < 0 ? 0 : (uint32_t)n < rinc_stickLen - rinc_stickPos ? (uint32_t)n : rinc_stickLen - rinc_stickPos;
  memcpy(buf, rinc_stickData + rinc_stickPos, k); rinc_stickPos += k;
  return (int)k;
}
static int rinc_vWrite(const char *buf, int n) {
  uint32_t end;
  if (!rinc_stickOpen) return rinc_card->write(buf, n);
  if (!rinc_stickWriting || n < 0) return -1;
  end = rinc_stickPos + (uint32_t)n;
  if (end > rinc_stickRoom) {
    uint32_t room = rinc_stickRoom; uint8_t *more;
    while (room < end) room *= 2;
    if (!(more = (uint8_t *)realloc(rinc_stickData, room))) return -1;
    rinc_stickData = more; rinc_stickRoom = room;
  }
  memcpy(rinc_stickData + rinc_stickPos, buf, (size_t)n);
  rinc_stickPos = end;
  if (end > rinc_stickLen) rinc_stickLen = end;
  return n;
}
static int rinc_vClose(void) {
  int i;
  if (!rinc_stickOpen) return rinc_card->close();
  if (rinc_stickWriting) { /* (kept: as name, over what was there) */
    if ((i = rinc_stickFind(rinc_stickWrite, 0)) < 0 && (i = rinc_stickAdd(rinc_stickWrite, strlen(rinc_stickWrite))) < 0) { rinc_stickShut(); return -1; }
    free(rinc_stickFiles[i].own);
    rinc_stickFiles[i].own = rinc_stickData; rinc_stickFiles[i].usize = rinc_stickLen;
    rinc_stickFiles[i].packed = 0; rinc_stickFiles[i].deflated = 0;
    rinc_stickOwned = 0; /* (the entry has it now) */
  }
  rinc_stickShut();
  return 0;
}
static int rinc_vSeek(uint32_t at) {
  if (!rinc_stickOpen) return rinc_card->seek(at);
  if (at > rinc_stickLen) return -1;
  rinc_stickPos = at;
  return 0;
}
static int32_t rinc_vSize(void) { return rinc_stickOpen ? (int32_t)rinc_stickLen : rinc_card->size(); }
/* entry f's name in folder rel (n long, "" for /stick): the part after it (*part, *k long), and whether
   that's a folder of it; 0 if f isn't in rel */
static int rinc_stickPart(const rinc_StickFile *f, const char *rel, size_t n, const char **part, uint32_t *k, int *dir) {
  const char *slash; uint32_t left;
  if (n && (f->nameLen <= n || memcmp(f->name, rel, n) || f->name[n] != '/')) return 0;
  *part = f->name + (n ? n + 1 : 0); left = f->nameLen - (uint32_t)(*part - f->name);
  slash = memchr(*part, '/', left);
  *k = slash ? (uint32_t)(slash - *part) : left; *dir = slash != 0 || f->folder;
  return *k > 0;
}
static int rinc_stickHas(const char *rel) { /* (a folder: a file in it, or made here) */
  int i; size_t n = strlen(rel);
  if (!n) return 1;
  for (i = 0; i < rinc_stickCount; i++) {
    const rinc_StickFile *f = &rinc_stickFiles[i];
    if ((f->folder && f->nameLen == n && !memcmp(f->name, rel, n)) || (f->nameLen > n && !memcmp(f->name, rel, n) && f->name[n] == '/')) return 1;
  }
  return 0;
}
static int rinc_vList(const char *dir, void (*each)(const char *name, int dir, void *ud), void *ud) {
  const char *rel = rinc_onStick(dir), *part, *other; size_t n; int i, j, isDir, otherDir; uint32_t k, ok;
  if (!strcmp(dir, "/")) { each("rinc", 1, ud); if (rinc_stickIsIn || rinc_stickDir[0]) each("stick", 1, ud); return 0; }
  if (!rel) return rinc_card->list(dir, each, ud);
  if (!rinc_stickIsIn || !rinc_stickHas(rel)) return -1;
  n = strlen(rel);
  for (i = 0; i < rinc_stickCount; i++) { /* (each name in it once: a file, or a folder) */
    char name[64];
    if (!rinc_stickPart(&rinc_stickFiles[i], rel, n, &part, &k, &isDir)) continue;
    for (j = 0; j < i; j++) /* (named already, by one before it?) */
      if (rinc_stickPart(&rinc_stickFiles[j], rel, n, &other, &ok, &otherDir) && ok == k && otherDir == isDir && !memcmp(other, part, k)) break;
    if (j < i || k >= sizeof name) continue;
    memcpy(name, part, k); name[k] = 0;
    each(name, isDir, ud);
  }
  return 0;
}
/* a folder left empty (its last file removed, or moved away) stays, as on a drive: kept as one made here */
static void rinc_stickKeepFolder(const char *rel) {
  const char *slash = strrchr(rel, '/'); char dir[300]; int i;
  if (!slash || (size_t)(slash - rel) >= sizeof dir) return;
  memcpy(dir, rel, (size_t)(slash - rel)); dir[slash - rel] = 0;
  if (!rinc_stickHas(dir) && (i = rinc_stickAdd(dir, strlen(dir))) >= 0) rinc_stickFiles[i].folder = 1;
}
static int rinc_vRemove(const char *path) { /* (a file, or an empty folder) */
  const char *rel = rinc_onStick(path); int i;
  if (!rel) return rinc_card->remove(path);
  rinc_stickShut();
  if ((i = rinc_stickFind(rel, 0)) >= 0) { rinc_stickDrop(i); rinc_stickKeepFolder(rel); return 0; }
  if ((i = rinc_stickFind(rel, 1)) < 0) return -1;
  { int k; size_t n = strlen(rel); for (k = 0; k < rinc_stickCount; k++) if (rinc_stickFiles[k].nameLen > n && !memcmp(rinc_stickFiles[k].name, rel, n) && rinc_stickFiles[k].name[n] == '/') return -1; } /* (not empty) */
  rinc_stickDrop(i);
  rinc_stickKeepFolder(rel);
  return 0;
}
static int rinc_vMkdir(const char *path) {
  const char *rel = rinc_onStick(path);
  if (!rel) return rinc_card->mkdir(path);
  if (!rinc_stickIsIn) return -1;
  if (!*rel || rinc_stickHas(rel)) return 0;
  if (rinc_stickFind(rel, 0) >= 0) return -1; /* (a file's name) */
  { int i = rinc_stickAdd(rel, strlen(rel)); if (i < 0) return -1; rinc_stickFiles[i].folder = 1; }
  return 0;
}
static int rinc_vRename(const char *from, const char *to) {
  const char *a = rinc_onStick(from), *b = rinc_onStick(to); size_t n, m; int i;
  if (!a && !b) return rinc_card->rename(from, to);
  if (!a || !b || !*a || !*b || !rinc_stickIsIn || rinc_stickFind(b, 0) >= 0 || rinc_stickHas(b)) return -1; /* (on the stick, to a name not taken) */
  rinc_stickShut();
  n = strlen(a); m = strlen(b);
  if (m + 1 >= 300) return -1;
  for (i = 0; i < rinc_stickCount; i++) { /* (it, or all in it) */
    rinc_StickFile *f = &rinc_stickFiles[i]; char *name;
    if (!(f->nameLen == n && !memcmp(f->name, a, n)) && !(f->nameLen > n && !memcmp(f->name, a, n) && f->name[n] == '/')) continue;
    if (!(name = (char *)malloc(m + f->nameLen - n + 1))) return -1;
    memcpy(name, b, m); memcpy(name + m, f->name + n, f->nameLen - n + 1);
    free(f->name); f->name = name; f->nameLen = (uint32_t)(m + f->nameLen - n);
  }
  rinc_stickKeepFolder(a); /* (where it was: still there, if that's all it held) */
  return 0;
}
static int rinc_vCopy(const char *from, const char *to) { /* (a file, whole: to's folders made, it replaced) */
  char dir[300]; const char *slash; uint8_t *bytes = 0; uint32_t n = 0; int bad = 0, i;
  if (!rinc_onStick(from) && !rinc_onStick(to)) return rinc_card->copy(from, to);
  if (rinc_onStick(from)) { /* (its bytes, from the stick) */
    if ((i = rinc_stickFind(rinc_onStick(from), 0)) < 0 || rinc_stickLoad(i)) return -1;
    n = rinc_stickLen;
    if (!(bytes = (uint8_t *)malloc(n ? n : 1))) { rinc_stickShut(); return -1; }
    memcpy(bytes, rinc_stickData, n); rinc_stickShut();
  } else { /* (or from the card) */
    int32_t size;
    if (rinc_card->open(from, rinc_READ) || (size = rinc_card->size()) < 0 || !(bytes = (uint8_t *)malloc(size ? (size_t)size : 1))) { rinc_card->close(); return -1; }
    n = (uint32_t)size;
    bad = n && rinc_card->read((char *)bytes, (int)n) != (int)n;
    rinc_card->close();
    if (bad) { free(bytes); return -1; }
  }
  slash = strrchr(to, '/');
  if (slash && slash > to) { snprintf(dir, sizeof dir, "%.*s", (int)(slash - to), to); rinc_vMkdir(dir); }
  bad = rinc_vOpen(to, rinc_REPLACE) || (n && rinc_vWrite((const char *)bytes, (int)n) != (int)n);
  bad |= rinc_vClose() != 0;
  free(bytes);
  return bad ? -1 : 0;
}
static const rinc_Fs rinc_volumes = {rinc_vOpen, rinc_vRead, rinc_vWrite, rinc_vClose, rinc_vList, rinc_vRemove, rinc_vCopy, rinc_vRename, rinc_vMkdir, rinc_vSeek, rinc_vSize};

static void rinc_stickUse(void) { if (rinc_fs != &rinc_volumes) { rinc_card = rinc_fs; rinc_fs = &rinc_volumes; } }
static void rinc_stickClear(void) {
  rinc_stickDir[0] = rinc_folderName[0] = 0;
  rinc_stickShut();
  while (rinc_stickCount) rinc_stickDrop(rinc_stickCount - 1);
  free(rinc_stickZip); rinc_stickZip = 0; rinc_stickIsIn = 0; rinc_stickName[0] = 0;
}
int rinc_stickMount(uint8_t *zip, uint32_t size, const char *name) {
  uint32_t entries = 0, dirSize = 0, dirAt = 0, end, i, at, back; int k, found = 0;
  if (!zip) { rinc_stickClear(); return 0; } /* (taken out) */
  back = size < 65557 ? size : 65557;
  for (k = (int)back - 22; k >= 0 && !found; k--) {
    const uint8_t *e = zip + size - back + k;
    if (rinc_le32(e) == 0x06054B50u) { found = 1; entries = rinc_le16(e + 10); dirSize = rinc_le32(e + 12); dirAt = rinc_le32(e + 16); }
  }
  if (!found || dirAt > size || dirSize > size - dirAt) return -1;
  end = dirAt + dirSize;
  for (i = 0, at = dirAt; i < entries; i++) { /* (checked whole first: nothing changes for one that isn't a zip) */
    const uint8_t *h = zip + at; uint32_t nameLen, local;
    if (at + 46 > end || rinc_le32(h) != 0x02014B50u) return -1;
    nameLen = rinc_le16(h + 28); local = rinc_le32(h + 42);
    if (at + 46 + nameLen > end || local + 30 > size || rinc_le32(zip + local) != 0x04034B50u) return -1;
    at += 46 + nameLen + rinc_le16(h + 30) + rinc_le16(h + 32);
  }
  rinc_stickClear();
  for (i = 0, at = dirAt; i < entries; i++) {
    const uint8_t *h = zip + at; uint32_t nameLen = rinc_le16(h + 28), local = rinc_le32(h + 42), data, j, method = rinc_le16(h + 10), csize, usize, len;
    const char *name0 = (const char *)h + 46; int e;
    data = local + 30 + rinc_le16(zip + local + 26) + rinc_le16(zip + local + 28);
    at += 46 + nameLen + rinc_le16(h + 30) + rinc_le16(h + 32);
    csize = rinc_le32(h + 20); usize = rinc_le32(h + 24);
    for (len = nameLen; len && name0[len - 1] == '/'; len--) {}
    if (!len || nameLen > 200 || (method != 0 && method != 8) || data > size || csize > size - data || (!method && csize != usize)) continue;
    for (j = 0; j < len && name0[j] != '\\' && name0[j] != ':' && (unsigned char)name0[j] >= 32; j++) {}
    if (j < len || (len >= 2 && !memcmp(name0, "..", 2))) continue; /* (names Rinc wouldn't give a file) */
    for (j = 0; j + 2 < len && !(name0[j] == '/' && name0[j + 1] == '.' && name0[j + 2] == '.'); j++) {}
    if (j + 2 < len) continue;
    if ((e = rinc_stickAdd(name0, len)) < 0) break;
    if (len != nameLen) { rinc_stickFiles[e].folder = 1; continue; } /* (a folder's own entry: kept, in case it's empty) */
    rinc_stickFiles[e].packed = zip + data; rinc_stickFiles[e].csize = csize; rinc_stickFiles[e].usize = usize;
    rinc_stickFiles[e].crc = rinc_le32(h + 16); rinc_stickFiles[e].deflated = method == 8;
  }
  rinc_stickZip = zip; rinc_stickIsIn = 1;
  snprintf(rinc_stickName, sizeof rinc_stickName, "%s", name ? name : "");
  rinc_stickUse();
  return 0;
}
void rinc_stickNew(const char *name) {
  rinc_stickClear();
  rinc_stickIsIn = 1;
  snprintf(rinc_stickName, sizeof rinc_stickName, "%s", name ? name : "stick.zip");
  rinc_stickUse();
}
const char *rinc_stickIn(void) { return rinc_stickIsIn ? rinc_stickName : rinc_stickDir[0] ? rinc_folderName : 0; }
void rinc_stickFolderSet(const char *dir, const char *name) {
  rinc_stickClear();
  snprintf(rinc_stickDir, sizeof rinc_stickDir, "%s", dir);
  snprintf(rinc_folderName, sizeof rinc_folderName, "%s.zip", name); /* (downloaded: its name, as a zip) */
}
const char *rinc_stickFolder(void) { return rinc_stickDir[0] ? rinc_stickDir : 0; }
uint8_t *rinc_stickZipped(uint32_t *size) { return rinc_stickIn() ? rinc_treeZip("/stick", size) : 0; }
#elif !defined(__wasm__) /* (no zip stick: the board's is a thumb drive, sd.c's; the web's is in its storage, main.c's) */
int rinc_stickMount(uint8_t *zip, uint32_t size, const char *name) { (void)zip; (void)size; (void)name; return -1; }
void rinc_stickNew(const char *name) { (void)name; }
const char *rinc_stickIn(void) { return 0; }
uint8_t *rinc_stickZipped(uint32_t *size) { *size = 0; return 0; }
#endif
