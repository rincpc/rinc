/* The Fruit Jam's microSD card, over SPI (SPI0: clock GPIO 34, out 35, in 36; chip select 39 as a plain
   GPIO; card detect 33), as FatFs's disk (the diskio functions it calls), and through FatFs as Rinc's
   files (rinc_fs). The card is set up the usual way (CMD0, CMD8, ACMD41 until ready, CMD58 for its
   addressing) at 400 kHz, then run at 25 MHz; blocks are 512 bytes, read and written one at a time.
   The filesystem is mounted when first used, so a card can go in after starting. */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ff.h"
#include "diskio.h"
#include "rinc.h"

#define rinc_SD_SPI spi0
#define rinc_PIN_SCK 34
#define rinc_PIN_MOSI 35
#define rinc_PIN_MISO 36
#define rinc_PIN_CS 39
#define rinc_PIN_DETECT 33

static int rinc_ready, rinc_blocks; /* blocks: the card addresses blocks, not bytes (SDHC and up) */

static uint8_t rinc_xfer(uint8_t b) { uint8_t r; spi_write_read_blocking(rinc_SD_SPI, &b, &r, 1); return r; }
static void rinc_cs(int on) { gpio_put(rinc_PIN_CS, !on); if (!on) rinc_xfer(0xFF); } /* (a byte after: the card lets go of MISO) */
static int rinc_waitReady(uint32_t ms) { /* the card holds MISO low while busy */
  absolute_time_t until = make_timeout_time_ms(ms);
  while (rinc_xfer(0xFF) != 0xFF) if (time_reached(until)) return 0;
  return 1;
}
/* a command, selected: its R1 response (bit 7 set: none) */
static uint8_t rinc_command(uint8_t cmd, uint32_t arg) {
  uint8_t b[6] = {(uint8_t)(0x40 | cmd), (uint8_t)(arg >> 24), (uint8_t)(arg >> 16), (uint8_t)(arg >> 8), (uint8_t)arg,
                  cmd == 0 ? 0x95 : cmd == 8 ? 0x87 : 0x01}, r = 0xFF; int i;
  rinc_cs(0); rinc_cs(1);
  if (cmd != 0 && !rinc_waitReady(500)) return 0xFF;
  spi_write_blocking(rinc_SD_SPI, b, 6);
  if (cmd == 12) rinc_xfer(0xFF); /* (a stop's stuff byte) */
  for (i = 0; i < 10 && ((r = rinc_xfer(0xFF)) & 0x80); i++) {}
  return r;
}
static uint8_t rinc_appCommand(uint8_t cmd, uint32_t arg) { uint8_t r = rinc_command(55, 0); return r > 1 ? r : rinc_command(cmd, arg); }

static int rinc_cardInit(void) {
  int i, v2; uint8_t r, ocr[4]; absolute_time_t until;
  spi_init(rinc_SD_SPI, 400 * 1000);
  gpio_set_function(rinc_PIN_SCK, GPIO_FUNC_SPI); gpio_set_function(rinc_PIN_MOSI, GPIO_FUNC_SPI); gpio_set_function(rinc_PIN_MISO, GPIO_FUNC_SPI);
  gpio_pull_up(rinc_PIN_MISO);
  gpio_init(rinc_PIN_CS); gpio_set_dir(rinc_PIN_CS, GPIO_OUT); gpio_put(rinc_PIN_CS, 1);
  for (i = 0; i < 10; i++) rinc_xfer(0xFF); /* 80 clocks, deselected: the card wakes into SPI mode with CMD0 */
  for (i = 0; i < 10 && (r = rinc_command(0, 0)) != 1; i++) {}
  if (r != 1) { rinc_cs(0); return 0; }
  v2 = rinc_command(8, 0x1AA) == 1;
  if (v2) { for (i = 0; i < 4; i++) ocr[i] = rinc_xfer(0xFF); if (ocr[2] != 0x01 || ocr[3] != 0xAA) { rinc_cs(0); return 0; } }
  until = make_timeout_time_ms(1500);
  while ((r = rinc_appCommand(41, v2 ? 0x40000000u : 0)) != 0) if (r > 1 || time_reached(until)) { rinc_cs(0); return 0; }
  rinc_blocks = 0;
  if (v2 && rinc_command(58, 0) == 0) { for (i = 0; i < 4; i++) ocr[i] = rinc_xfer(0xFF); rinc_blocks = (ocr[0] & 0x40) != 0; }
  if (!rinc_blocks && rinc_command(16, 512) != 0) { rinc_cs(0); return 0; }
  rinc_cs(0);
  spi_set_baudrate(rinc_SD_SPI, 25 * 1000 * 1000);
  return 1;
}

static int rinc_readBlock(uint8_t *buf, uint32_t n) {
  absolute_time_t until; uint8_t t;
  if (rinc_command(17, rinc_blocks ? n : n * 512) != 0) { rinc_cs(0); return 0; }
  until = make_timeout_time_ms(200);
  while ((t = rinc_xfer(0xFF)) == 0xFF) if (time_reached(until)) { rinc_cs(0); return 0; }
  if (t != 0xFE) { rinc_cs(0); return 0; }
  memset(buf, 0xFF, 512); spi_read_blocking(rinc_SD_SPI, 0xFF, buf, 512);
  rinc_xfer(0xFF); rinc_xfer(0xFF); /* (its CRC) */
  rinc_cs(0);
  return 1;
}
static int rinc_writeBlock(const uint8_t *buf, uint32_t n) {
  uint8_t r;
  if (rinc_command(24, rinc_blocks ? n : n * 512) != 0) { rinc_cs(0); return 0; }
  rinc_xfer(0xFE); spi_write_blocking(rinc_SD_SPI, buf, 512); rinc_xfer(0xFF); rinc_xfer(0xFF);
  r = rinc_xfer(0xFF);
  if ((r & 0x1F) != 0x05) { rinc_cs(0); return 0; } /* (not accepted) */
  r = (uint8_t)rinc_waitReady(500);
  rinc_cs(0);
  return r;
}

/* FatFs's disk */
DSTATUS disk_status(BYTE drive) { return drive || !rinc_ready ? STA_NOINIT : 0; }
DSTATUS disk_initialize(BYTE drive) { if (drive) return STA_NOINIT; rinc_ready = rinc_cardInit(); return rinc_ready ? 0 : STA_NOINIT; }
DRESULT disk_read(BYTE drive, BYTE *buf, LBA_t sector, UINT count) {
  if (drive || !rinc_ready) return RES_NOTRDY;
  for (; count; count--, sector++, buf += 512) if (!rinc_readBlock(buf, (uint32_t)sector)) return RES_ERROR;
  return RES_OK;
}
DRESULT disk_write(BYTE drive, const BYTE *buf, LBA_t sector, UINT count) {
  if (drive || !rinc_ready) return RES_NOTRDY;
  for (; count; count--, sector++, buf += 512) if (!rinc_writeBlock(buf, (uint32_t)sector)) return RES_ERROR;
  return RES_OK;
}
DRESULT disk_ioctl(BYTE drive, BYTE cmd, void *buf) {
  if (drive || !rinc_ready) return RES_NOTRDY;
  if (cmd == CTRL_SYNC) { int ok; rinc_cs(1); ok = rinc_waitReady(500); rinc_cs(0); return ok ? RES_OK : RES_ERROR; }
  if (cmd == GET_BLOCK_SIZE) { *(DWORD *)buf = 1; return RES_OK; }
  return RES_PARERR;
}

/* Rinc's files, through FatFs: one file open at a time */
static FATFS rinc_volume; static FIL rinc_file; static int rinc_mounted, rinc_isOpen;
/* mounted again after a failure, which may be another card, or this one changed elsewhere: what Rinc
   checked of it goes (rinc_storageChanged). Only a real failure unmounts: a file that isn't there doesn't */
static int rinc_mount(void) {
  if (!rinc_mounted && (rinc_mounted = f_mount(&rinc_volume, "", 1) == FR_OK)) rinc_storageChanged("/rinc/apps/");
  return rinc_mounted;
}
static int rinc_failed(FRESULT r) { if (r != FR_NO_FILE && r != FR_NO_PATH) rinc_mounted = 0; return -1; }
static int rinc_mkdirs(const char *path) { /* every directory along path (its last part is the file) */
  char d[128]; const char *p;
  for (p = strchr(path + 1, '/'); p; p = strchr(p + 1, '/')) {
    size_t n = (size_t)(p - path);
    if (n >= sizeof d) return 0;
    memcpy(d, path, n); d[n] = 0;
    { FRESULT r = f_mkdir(d); if (r != FR_OK && r != FR_EXIST) return 0; }
  }
  return 1;
}
static int rinc_fsOpen(const char *path, int write) {
  if (rinc_isOpen) { f_close(&rinc_file); rinc_isOpen = 0; }
  if (!rinc_mount()) { rinc_mounted = 0; return -1; }
  if (write && !rinc_mkdirs(path)) return -1;
  if (write) rinc_storageChanged(path);
  { FRESULT r = f_open(&rinc_file, path, write ? FA_WRITE | FA_CREATE_ALWAYS : FA_READ); if (r != FR_OK) return rinc_failed(r); }
  rinc_isOpen = 1;
  return 0;
}
static int rinc_fsRead(char *buf, int n) { UINT got = 0; if (!rinc_isOpen || f_read(&rinc_file, buf, (UINT)n, &got) != FR_OK) return -1; return (int)got; }
static int rinc_fsWrite(const char *buf, int n) { UINT put = 0; if (!rinc_isOpen || f_write(&rinc_file, buf, (UINT)n, &put) != FR_OK || (int)put != n) return -1; return n; }
static int rinc_fsClose(void) { int ok = rinc_isOpen && f_close(&rinc_file) == FR_OK; rinc_isOpen = 0; return ok ? 0 : -1; }
static int rinc_fsList(const char *dir, void (*each)(const char *name, int dir, void *ud), void *ud) {
  DIR d; FILINFO info;
  FRESULT r;
  if (!rinc_mount()) return -1;
  if ((r = f_opendir(&d, dir)) != FR_OK) return rinc_failed(r);
  while (f_readdir(&d, &info) == FR_OK && info.fname[0]) each(info.fname, (info.fattrib & AM_DIR) != 0, ud);
  f_closedir(&d);
  return 0;
}
static int rinc_fsRemove(const char *path) {
  if (rinc_isOpen) { f_close(&rinc_file); rinc_isOpen = 0; }
  rinc_storageChanged(path);
  return rinc_mount() && f_unlink(path) == FR_OK ? 0 : -1;
}
static FIL rinc_copyTo; /* (the one file open beside rinc_file: a copy's) */
static int rinc_fsCopy(const char *from, const char *to) {
  uint8_t buf[512]; UINT got = 0, put = 0; FRESULT r; int bad = 0;
  if (rinc_isOpen) { f_close(&rinc_file); rinc_isOpen = 0; }
  if (!rinc_mount()) return -1;
  if ((r = f_open(&rinc_file, from, FA_READ)) != FR_OK) return rinc_failed(r);
  if (!rinc_mkdirs(to)) { f_close(&rinc_file); return -1; }
  rinc_storageChanged(to);
  if ((r = f_open(&rinc_copyTo, to, FA_WRITE | FA_CREATE_ALWAYS)) != FR_OK) { f_close(&rinc_file); return rinc_failed(r); }
  while (!bad && f_read(&rinc_file, buf, sizeof buf, &got) == FR_OK && got) bad = f_write(&rinc_copyTo, buf, got, &put) != FR_OK || put != got;
  f_close(&rinc_file);
  return f_close(&rinc_copyTo) != FR_OK || bad ? -1 : 0;
}
static int rinc_fsRename(const char *from, const char *to) {
  FRESULT r;
  if (rinc_isOpen) { f_close(&rinc_file); rinc_isOpen = 0; }
  if (!rinc_mount()) return -1;
  rinc_storageChanged(from); rinc_storageChanged(to);
  return (r = f_rename(from, to)) == FR_OK ? 0 : rinc_failed(r);
}
static int rinc_fsMkdir(const char *path) {
  char inside[128];
  if (!rinc_mount() || snprintf(inside, sizeof inside, "%s/x", path) >= (int)sizeof inside) return -1;
  rinc_storageChanged(path);
  return rinc_mkdirs(inside) ? 0 : -1; /* (it, and those along the way) */
}
static const rinc_Fs rinc_sdFs = {rinc_fsOpen, rinc_fsRead, rinc_fsWrite, rinc_fsClose, rinc_fsList, rinc_fsRemove, rinc_fsCopy, rinc_fsRename, rinc_fsMkdir};
void rinc_sdSetup(void) { gpio_init(rinc_PIN_DETECT); gpio_pull_up(rinc_PIN_DETECT); rinc_fs = &rinc_sdFs; }
