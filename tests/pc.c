/* pc.rio is what Rinc's host procs are, and src/core/pc_host.h, its C, is pc.rio's: made from it again
   (rio_hostHeader), it must be the one there (else: rinc -H src/core/pc_host.h src/core/pc.rio). Then pc,
   as the header has it, compiles against what rinc_hostProcs registers: each declaration's types match,
   and every host proc is declared. argv[1]: src/core/pc.rio, argv[2]: src/core/pc_host.h */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "rinc.h"

static unsigned char mem[1 << 20], scratch[2 << 20];
static rio_Vm vm;
static char src[64 << 10], header[256 << 10], made[256 << 10];

/* a file's bytes into buf (cap), without \r (as a checkout may have them): how many, or -1 */
static long readText(const char *path, char *buf, size_t cap) {
  FILE *f = fopen(path, "rb");
  size_t n, i, k = 0;
  if (!f) return -1;
  n = fread(buf, 1, cap, f);
  fclose(f);
  if (n >= cap) return -1;
  for (i = 0; i < n; i++)
    if (buf[i] != '\r') buf[k++] = buf[i];
  buf[k] = 0;
  return (long)k;
}

int main(int argc, char **argv) {
  static const char use[] = "import pc\npc.cls(1)\n";
  long len = argc > 1 ? readText(argv[1], src, sizeof src) : -1, hlen = argc > 2 ? readText(argv[2], header, sizeof header) : -1;
  uint32_t n;
  if (len < 0 || hlen < 0) { printf("FAIL: can't read pc.rio and pc_host.h\n"); return 1; }
  rio_init(&vm, mem, sizeof mem, 0, 0);
  if (!(n = rio_hostHeader(&vm, src, (uint32_t)len, scratch, sizeof scratch, "pc", made, sizeof made))) {
    printf("FAIL: pc.rio:%s\n", rio_error(&vm));
    return 1;
  }
  if ((long)n != hlen || memcmp(made, header, n)) {
    printf("FAIL: src/core/pc_host.h isn't pc.rio's now: make it again, rinc -H src/core/pc_host.h src/core/pc.rio\n");
    return 1;
  }
  rio_init(&vm, mem, sizeof mem, 0, 0);
  if (rinc_hostProcs(&vm)) { printf("FAIL: can't register the host procs\n"); return 1; }
  if (rio_compile(&vm, use, sizeof use - 1)) {
    rio_Error e = rio_errorInfo(&vm);
    printf("FAIL: %s:%s\n", e.file ? e.file : "(main)", rio_error(&vm));
    return 1;
  }
#ifdef rinc_MAX_FFI
  if (vm.ffiCount > rinc_MAX_FFI) { printf("FAIL: %d host procs, more than a Fruit Jam's or a web player's vm holds (RINC_MAX_FFI: %d)\n", vm.ffiCount, rinc_MAX_FFI); return 1; }
#endif
  printf("pc.rio declares all %d host procs\n", vm.ffiCount);
  return 0;
}
