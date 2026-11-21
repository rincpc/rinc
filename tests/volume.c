/* the system volume, kept (rinc_volumeRead, rinc_volumeKeep): read from /rinc/settings/volume as Rinc
   starts; written a second after the volume keys last changed it ("13", or "13 muted"), once for a knob
   turned many steps; not written when nothing changed. Storage here is one file in memory, the time
   set by hand */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "rinc.h"

static char file[64], path[64], openPath[64]; static int fileLen = -1, writes, at, mode;
static int fsOpen(const char *p, int how) {
  if (how == rinc_READ && (fileLen < 0 || strcmp(p, path))) return -1;
  strcpy(openPath, p); mode = how; at = 0;
  if (how == rinc_REPLACE) { strcpy(path, p); fileLen = 0; writes++; }
  return 0;
}
static int fsRead(char *buf, int n) { int k = fileLen - at < n ? fileLen - at : n; memcpy(buf, file + at, (size_t)k); at += k; return k; }
static int fsWrite(const char *buf, int n) { memcpy(file + fileLen, buf, (size_t)n); fileLen += n; return n; }
static int fsClose(void) { return 0; }
static int32_t fsSize(void) { return fileLen; }
static int fsSeek(uint32_t to) { at = (int)to; return 0; }
static double now;
static double clock(void) { return now; }
static int failed;
static void check(int ok, const char *what) { printf("%s%s\n", ok ? "ok   " : "FAIL ", what); failed += !ok; }

int main(void) {
  static rinc_Fs fs;
  fs.open = fsOpen; fs.read = fsRead; fs.write = fsWrite; fs.close = fsClose; fs.size = fsSize; fs.seek = fsSeek;
  rinc_fs = &fs; rinc_time = clock;
  rinc_volumeRead();
  check(rinc_audioLevel() == rinc_VOLUME_STEPS && writes == 0, "nothing kept yet: full volume, nothing written");
  rinc_volumeKeep(); now = 5; rinc_volumeKeep();
  check(writes == 0, "unchanged: not written");
  rinc_audioVolume(-1); rinc_volumeKeep(); now = 5.3f; rinc_audioVolume(-1); rinc_volumeKeep(); now = 5.6f; rinc_audioVolume(-1); rinc_volumeKeep();
  now = 6.2f; rinc_volumeKeep();
  check(writes == 0, "a knob turned: not written while it's turning (under a second since the last step)");
  now = 6.7f; rinc_volumeKeep();
  file[fileLen > 0 ? fileLen : 0] = 0;
  check(writes == 1 && !strcmp(path, "/rinc/settings/volume") && !strcmp(file, "13"), "written once, a second after the last step: 13");
  rinc_audioVolume(0); rinc_volumeKeep(); now = 8; rinc_volumeKeep();
  file[fileLen] = 0;
  check(writes == 2 && !strcmp(file, "13 muted"), "muted: kept too");
  strcpy(file, "5"); fileLen = 1; rinc_audioSet(rinc_VOLUME_STEPS, 0);
  rinc_volumeRead(); now = 20; rinc_volumeKeep();
  check(rinc_audioLevel() == 5 && writes == 2, "read as it starts: 5, and not written back");
  strcpy(file, "7 muted"); fileLen = 7; rinc_volumeRead();
  check(rinc_audioLevel() == -1 && rinc_audioStep() == 7, "read muted: 7, muted");
  if (failed) return 1;
  printf("volume: all ok\n");
  return 0;
}
