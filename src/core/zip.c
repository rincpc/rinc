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

int rinc_zipInstall(rinc_Zip *z) {
  uint8_t head[46]; uint32_t back, entries = 0, dirSize = 0, dirAt = 0, end, i, at; int found = 0, pass, bad = 0, k;
  z->files = 0; z->apps[0] = 0;
  back = z->size < 65557 ? z->size : 65557; /* (its directory's end, in its last 64 KB: the longest comment) */
  if (back > z->cap) back = z->cap;
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
    rel = strncmp(name, "rinc/", 5) ? name : name + 5;
    if (!*rel || rel[strlen(rel) - 1] == '/' || (strncmp(rel, "apps/", 5) && strncmp(rel, "help/", 5) && strncmp(rel, "lib/", 4))) continue; /* (folders, and anything but apps, help and the library) */
    if (strstr(rel, "..") || strchr(rel, '\\') || strchr(rel, ':') || strlen(name) != nameLen) { rinc_zipTell(z, "rinc: %s names %s: not installed\n", name); bad = 1; break; }
    if (method != 0 && method != 8) { rinc_zipTell(z, "rinc: %s: %s is packed a way Rinc doesn't read (stored or deflated only)\n", name); bad = 1; break; }
    if (usize > z->cap || (!method && csize != usize)) { rinc_zipTell(z, "rinc: %s: %s is too big to install here\n", name); bad = 1; break; }
    if (!pass) continue;
    if (z->read(local0, local, 30) != 30 || rinc_le32(local) != 0x04034B50u) { rinc_zipTell(z, "rinc: %s: can't read %s\n", name); bad = 1; break; }
    rinc_zipNow = z; rinc_zipAt = local0 + 30 + rinc_le16(local + 26) + rinc_le16(local + 28);
    got = method ? (uint32_t)rinc_inflateRaw(rinc_zipRead, csize, z->work, usize) : (uint32_t)rinc_zipRead((char *)z->work, (int)usize);
    if (got != usize || rinc_crc32(z->work, usize) != crc) { rinc_zipTell(z, "rinc: %s: %s is damaged\n", name); bad = 1; break; }
    snprintf(to, sizeof to, "/rinc/%s", rel);
    if (rinc_fs->open(to, 1) || (usize && rinc_fs->write((const char *)z->work, (int)usize) != (int)usize) || rinc_fs->close()) { rinc_zipTell(z, "rinc: %s: can't write %s\n", to); bad = 1; break; }
    z->files++;
    if (!strncmp(rel, "apps/", 5)) { /* (each app's name, once, to say what came) */
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

/* the stick (plans/drives.md): a zip held in memory (dropped on the desktop or the web, or rinc -stick),
   read only, at /stick: its files and folders, as storage's are. It's put in front of the platform's
   storage (rinc_fs): /stick's paths go to it, the rest through to the platform's, and copying off it
   (Files' copy: pc.fileCopy) reads from one and writes the other. "/" lists rinc, and stick while one's in */
typedef struct { const char *name; uint32_t nameLen, at, csize, usize, crc; int deflated; } rinc_StickFile;
static uint8_t *rinc_stickZip; static rinc_StickFile *rinc_stickFiles; static int rinc_stickCount;
static char rinc_stickName[64];
static const rinc_Fs *rinc_card; /* (the platform's storage, behind) */
static uint8_t *rinc_stickData; static uint32_t rinc_stickLen, rinc_stickPos; static int rinc_stickOpen, rinc_stickOwned; /* (the file open on it, whole) */
/* a path on the stick: the rest of it ("" for /stick itself), or 0 */
static const char *rinc_onStick(const char *path) {
  if (strncmp(path, "/stick", 6) || (path[6] && path[6] != '/')) return 0;
  return path[6] ? path + 7 : path + 6;
}
static int rinc_stickFind(const char *rel) {
  int i; size_t n = strlen(rel);
  for (i = 0; i < rinc_stickCount; i++) if (rinc_stickFiles[i].nameLen == n && !memcmp(rinc_stickFiles[i].name, rel, n)) return i;
  return -1;
}
static const uint8_t *rinc_inflateFrom; static uint32_t rinc_inflateLeft;
static int rinc_inflateRead(char *buf, int n) {
  uint32_t k = (uint32_t)n < rinc_inflateLeft ? (uint32_t)n : rinc_inflateLeft;
  memcpy(buf, rinc_inflateFrom, k); rinc_inflateFrom += k; rinc_inflateLeft -= k;
  return (int)k;
}
static void rinc_stickShut(void) {
  if (rinc_stickOwned) free(rinc_stickData);
  rinc_stickData = 0; rinc_stickOpen = rinc_stickOwned = 0;
}
static int rinc_vOpen(const char *path, int how) {
  const char *rel = rinc_onStick(path); int i; rinc_StickFile *f;
  rinc_stickShut();
  if (!rel) return rinc_card->open(path, how);
  if (how != rinc_READ || (i = rinc_stickFind(rel)) < 0) return -1;
  f = &rinc_stickFiles[i];
  if (!f->deflated) rinc_stickData = rinc_stickZip + f->at;
  else {
    if (!(rinc_stickData = (uint8_t *)malloc(f->usize ? f->usize : 1))) return -1;
    rinc_stickOwned = 1;
    rinc_inflateFrom = rinc_stickZip + f->at; rinc_inflateLeft = f->csize;
    if ((uint32_t)rinc_inflateRaw(rinc_inflateRead, f->csize, rinc_stickData, f->usize) != f->usize || rinc_crc32(rinc_stickData, f->usize) != f->crc) { rinc_stickShut(); return -1; } /* (damaged: not read) */
  }
  rinc_stickLen = f->usize; rinc_stickPos = 0; rinc_stickOpen = 1;
  return 0;
}
static int rinc_vRead(char *buf, int n) {
  uint32_t k;
  if (!rinc_stickOpen) return rinc_card->read(buf, n);
  k = n < 0 ? 0 : (uint32_t)n < rinc_stickLen - rinc_stickPos ? (uint32_t)n : rinc_stickLen - rinc_stickPos;
  memcpy(buf, rinc_stickData + rinc_stickPos, k); rinc_stickPos += k;
  return (int)k;
}
static int rinc_vWrite(const char *buf, int n) { return rinc_stickOpen ? -1 : rinc_card->write(buf, n); }
static int rinc_vClose(void) {
  if (!rinc_stickOpen) return rinc_card->close();
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
/* file f's name in folder rel (n long, "" for /stick): the part after it (*part, *k long), and whether
   that's a folder of it; 0 if f isn't in rel */
static int rinc_stickPart(const rinc_StickFile *f, const char *rel, size_t n, const char **part, uint32_t *k, int *dir) {
  const char *slash; uint32_t left;
  if (n && (f->nameLen <= n || memcmp(f->name, rel, n) || f->name[n] != '/')) return 0;
  *part = f->name + (n ? n + 1 : 0); left = f->nameLen - (uint32_t)(*part - f->name);
  slash = memchr(*part, '/', left);
  *k = slash ? (uint32_t)(slash - *part) : left; *dir = slash != 0;
  return *k > 0;
}
static int rinc_vList(const char *dir, void (*each)(const char *name, int dir, void *ud), void *ud) {
  const char *rel = rinc_onStick(dir), *part, *other; size_t n; int i, j, any = 0, isDir, otherDir; uint32_t k, ok;
  if (!strcmp(dir, "/")) { each("rinc", 1, ud); if (rinc_stickZip) each("stick", 1, ud); return 0; }
  if (!rel) return rinc_card->list(dir, each, ud);
  if (!rinc_stickZip) return -1;
  n = strlen(rel);
  for (i = 0; i < rinc_stickCount; i++) { /* (each name in it once: a file, or a folder of the files under it) */
    char name[64];
    if (!rinc_stickPart(&rinc_stickFiles[i], rel, n, &part, &k, &isDir)) continue;
    any = 1;
    for (j = 0; j < i; j++) /* (named already, by a file before it?) */
      if (rinc_stickPart(&rinc_stickFiles[j], rel, n, &other, &ok, &otherDir) && ok == k && otherDir == isDir && !memcmp(other, part, k)) break;
    if (j < i || k >= sizeof name) continue;
    memcpy(name, part, k); name[k] = 0;
    each(name, isDir, ud);
  }
  return any || !n ? 0 : -1;
}
static int rinc_vRemove(const char *path) { return rinc_onStick(path) ? -1 : rinc_card->remove(path); }
static int rinc_vRename(const char *from, const char *to) { return rinc_onStick(from) || rinc_onStick(to) ? -1 : rinc_card->rename(from, to); }
static int rinc_vMkdir(const char *path) { return rinc_onStick(path) ? -1 : rinc_card->mkdir(path); }
static int rinc_vCopy(const char *from, const char *to) {
  char dir[300]; const char *slash; int bad;
  if (rinc_onStick(to)) return -1;
  if (!rinc_onStick(from)) return rinc_card->copy(from, to);
  if (rinc_vOpen(from, rinc_READ)) return -1; /* (off the stick: read whole, written to the card, its folders made) */
  slash = strrchr(to, '/');
  if (slash && slash > to) { snprintf(dir, sizeof dir, "%.*s", (int)(slash - to), to); rinc_card->mkdir(dir); }
  bad = rinc_card->open(to, rinc_REPLACE) || (rinc_stickLen && rinc_card->write((const char *)rinc_stickData, (int)rinc_stickLen) != (int)rinc_stickLen);
  bad |= rinc_card->close() != 0;
  rinc_stickShut();
  return bad ? -1 : 0;
}
static const rinc_Fs rinc_volumes = {rinc_vOpen, rinc_vRead, rinc_vWrite, rinc_vClose, rinc_vList, rinc_vRemove, rinc_vCopy, rinc_vRename, rinc_vMkdir, rinc_vSeek, rinc_vSize};

int rinc_stickMount(uint8_t *zip, uint32_t size, const char *name) {
  uint32_t entries = 0, dirSize = 0, dirAt = 0, end, i, at, back; int k, found = 0, n = 0; rinc_StickFile *files;
  if (!zip) { /* (taken out) */
    rinc_stickShut(); free(rinc_stickZip); free(rinc_stickFiles);
    rinc_stickZip = 0; rinc_stickFiles = 0; rinc_stickCount = 0; rinc_stickName[0] = 0;
    return 0;
  }
  back = size < 65557 ? size : 65557;
  for (k = (int)back - 22; k >= 0 && !found; k--) {
    const uint8_t *e = zip + size - back + k;
    if (rinc_le32(e) == 0x06054B50u) { found = 1; entries = rinc_le16(e + 10); dirSize = rinc_le32(e + 12); dirAt = rinc_le32(e + 16); }
  }
  if (!found || dirAt > size || dirSize > size - dirAt || !(files = (rinc_StickFile *)malloc((entries ? entries : 1) * sizeof *files))) return -1;
  end = dirAt + dirSize;
  for (i = 0, at = dirAt; i < entries; i++) {
    const uint8_t *h = zip + at; rinc_StickFile *f = &files[n]; uint32_t nameLen, local, data, j; const char *name0;
    if (at + 46 > end || rinc_le32(h) != 0x02014B50u) { free(files); return -1; }
    nameLen = rinc_le16(h + 28); local = rinc_le32(h + 42);
    if (at + 46 + nameLen > end || local + 30 > size || rinc_le32(zip + local) != 0x04034B50u) { free(files); return -1; }
    name0 = (const char *)h + 46;
    data = local + 30 + rinc_le16(zip + local + 26) + rinc_le16(zip + local + 28);
    at += 46 + nameLen + rinc_le16(h + 30) + rinc_le16(h + 32);
    f->name = name0; f->nameLen = nameLen; f->at = data; f->deflated = rinc_le16(h + 10) == 8;
    f->crc = rinc_le32(h + 16); f->csize = rinc_le32(h + 20); f->usize = rinc_le32(h + 24);
    while (f->nameLen && f->name[f->nameLen - 1] == '/') f->nameLen--; /* (a folder's own entry: its files name it anyway) */
    if (!f->nameLen || nameLen > 200 || (rinc_le16(h + 10) != 0 && !f->deflated) || data > size || f->csize > size - data || (!f->deflated && f->csize != f->usize)) continue;
    for (j = 0; j < f->nameLen && name0[j] != '\\' && name0[j] != ':' && (unsigned char)name0[j] >= 32; j++) {}
    if (j < f->nameLen || (f->nameLen >= 2 && !memcmp(name0, "..", 2)) || memchr(name0, 0, f->nameLen)) continue; /* (names Rinc wouldn't give a file) */
    for (j = 0; j + 2 < f->nameLen && !(name0[j] == '/' && name0[j + 1] == '.' && name0[j + 2] == '.'); j++) {}
    if (j + 2 < f->nameLen) continue;
    if (nameLen != f->nameLen) continue; /* (folders: from their files) */
    n++;
  }
  rinc_stickMount(0, 0, 0);
  rinc_stickZip = zip; rinc_stickFiles = files; rinc_stickCount = n;
  snprintf(rinc_stickName, sizeof rinc_stickName, "%s", name ? name : "");
  if (rinc_fs != &rinc_volumes) { rinc_card = rinc_fs; rinc_fs = &rinc_volumes; }
  return 0;
}
const char *rinc_stickIn(void) { return rinc_stickZip ? rinc_stickName : 0; }
