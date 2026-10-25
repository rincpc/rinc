/* Bundles of apps, as zips (a release's rinc-system-apps.zip), installed into storage: rinc_zipInstall.
   The zip is read where the platform has it (a file on the desktop, the page's memory on the web), a
   piece at a time, and each file's inflated into the caller's room (work), so nothing's held but the
   file in hand: no zip library, and no memory of its own. */
#include <stdio.h>
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
    if (!*rel || rel[strlen(rel) - 1] == '/' || (strncmp(rel, "apps/", 5) && strncmp(rel, "help/", 5))) continue; /* (folders, and anything but apps and help) */
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
  else if (rinc_readApp(rinc_appFor("home"), 0, 0) != -1 || rinc_bundled(rinc_appFor("home"))) {
    snprintf(rinc_launchName, sizeof rinc_launchName, "home");
    rinc_launch = 1;
  } else rinc_say("(no home app yet: the home app's in rinc-system-apps.zip, from a release)\r\n");
}
