/* The Fruit Jam's microSD card, over SPI (SPI0: clock GPIO 34, out 35, in 36; chip select 39 as a plain
   GPIO; card detect 33), as FatFs's disk (the diskio functions it calls), and through FatFs as Rinc's
   files (rinc_fs). The card is set up the usual way (CMD0, CMD8, ACMD41 until ready, CMD58 for its
   addressing) at 400 kHz, then run at 25 MHz; blocks are 512 bytes, read and written one at a time.
   The filesystem is mounted when first used, so a card can go in after starting.
   And the stick (plans/drives.md): a thumb drive on the USB-A ports, through TinyUSB's mass storage
   host, as FatFs's drive 1, at /stick. */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ff.h"
#include "diskio.h"
#include "rinc.h"
#include "tusb.h"

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

/* the stick: the first thumb drive plugged in (512-byte blocks), its blocks read and written through
   TinyUSB, each request waited for (TinyUSB run meanwhile, as the keyboard's poll runs it) */
const unsigned char ff_zeros[FF_ZEROS_SECTORS * 512] = {0}; /* (zeros in flash, for FatFs's clearing of a folder made: ffconf.h) */
static uint8_t rinc_mscDev; static volatile int rinc_mscDone, rinc_mscOk;
static int rinc_mscDirty; static uint64_t rinc_mscWrote, rinc_mscFlushed; /* (written to since the stick's cache was last flushed, and when) */
static FATFS rinc_stickVolume; static int rinc_stickMounted;
void tuh_msc_mount_cb(uint8_t dev) { if (!rinc_mscDev && tuh_msc_get_block_size(dev, 0) == 512) rinc_mscDev = dev; }
void tuh_msc_umount_cb(uint8_t dev) {
  if (dev != rinc_mscDev) return;
  rinc_mscDev = 0;
  if (rinc_stickMounted) f_mount(0, "1:", 0);
  rinc_stickMounted = 0;
}
static bool rinc_mscCb(uint8_t dev, tuh_msc_complete_data_t const *d) { (void)dev; rinc_mscOk = d->csw->status == 0; rinc_mscDone = 1; return true; }
static int rinc_mscWait(int ready) { /* (ready: till it takes a request; else till the one sent's done) */
  absolute_time_t until = make_timeout_time_ms(3000);
  while (rinc_mscDev && (ready ? !tuh_msc_ready(rinc_mscDev) : !rinc_mscDone)) { tuh_task(); if (time_reached(until)) return 0; }
  return rinc_mscDev && (ready || rinc_mscOk);
}
static uint32_t rinc_mscN[3], rinc_mscBlocks[3]; static uint64_t rinc_mscUs[3]; /* (reads, writes, flushes: since last asked) */
static void rinc_mscStats(char *said, size_t cap) {
  snprintf(said, cap, "stick: %lu reads (%lu blocks, %lu ms), %lu writes (%lu blocks, %lu ms), %lu flushes (%lu ms)\r\n",
           (unsigned long)rinc_mscN[0], (unsigned long)rinc_mscBlocks[0], (unsigned long)(rinc_mscUs[0] / 1000), (unsigned long)rinc_mscN[1],
           (unsigned long)rinc_mscBlocks[1], (unsigned long)(rinc_mscUs[1] / 1000), (unsigned long)rinc_mscN[2], (unsigned long)(rinc_mscUs[2] / 1000));
  memset(rinc_mscN, 0, sizeof rinc_mscN); memset(rinc_mscBlocks, 0, sizeof rinc_mscBlocks); memset(rinc_mscUs, 0, sizeof rinc_mscUs);
}
static DRESULT rinc_mscIo(int write, BYTE *buf, LBA_t sector, UINT count) {
  uint64_t t0 = time_us_64();
  rinc_stickStats = rinc_mscStats;
  rinc_mscN[write]++; rinc_mscBlocks[write] += count;
  for (; count; ) {
    uint16_t n = count > 64 ? 64 : (uint16_t)count; int sent;
    if (!rinc_mscWait(1)) return RES_NOTRDY;
    rinc_mscDone = 0;
    sent = write ? tuh_msc_write10(rinc_mscDev, 0, buf, (uint32_t)sector, n, rinc_mscCb, 0) : tuh_msc_read10(rinc_mscDev, 0, buf, (uint32_t)sector, n, rinc_mscCb, 0);
    if (write) { if (!rinc_mscDirty) rinc_mscFlushed = time_us_64(); rinc_mscDirty = 1; rinc_mscWrote = time_us_64(); } /* (the first unflushed write: the 2 seconds from it) */
    if (!sent || !rinc_mscWait(0)) return RES_ERROR;
    count -= n; sector += n; buf += (size_t)n * 512;
  }
  rinc_mscUs[write] += time_us_64() - t0;
  return RES_OK;
}

/* the stick's own cache written out (SCSI's SYNCHRONIZE CACHE (10)), so what's written is on the stick, not
   just sent to it: 0 if the stick's gone. One that doesn't know the command says so, and that's fine: it
   has no cache to lose. Not at each of FatFs's syncs (each file closed, each folder made: a flush each made
   a copy of many files slow), but once the writing's paused a moment (rinc_stickIdle) */
static int rinc_mscSync(void) {
  msc_cbw_t cbw; uint64_t t0 = time_us_64(); int ok;
  memset(&cbw, 0, sizeof cbw);
  cbw.signature = MSC_CBW_SIGNATURE; cbw.tag = 0x54555342; cbw.cmd_len = 10; cbw.command[0] = 0x35;
  if (!rinc_mscWait(1)) return 0;
  rinc_mscDone = 0;
  if (!tuh_msc_scsi_command(rinc_mscDev, &cbw, 0, rinc_mscCb, 0)) return 0;
  rinc_mscWait(0); /* (its status: either way) */
  ok = rinc_mscDev != 0;
  rinc_mscN[2]++; rinc_mscUs[2] += time_us_64() - t0;
  return ok;
}

/* FatFs's disks: 0 the card, 1 the stick */
DSTATUS disk_status(BYTE drive) { return drive == 1 ? (rinc_mscDev ? 0 : STA_NOINIT) : drive || !rinc_ready ? STA_NOINIT : 0; }
DSTATUS disk_initialize(BYTE drive) {
  if (drive == 1) return rinc_mscDev && rinc_mscWait(1) ? 0 : STA_NOINIT;
  if (drive) return STA_NOINIT;
  rinc_ready = rinc_cardInit(); return rinc_ready ? 0 : STA_NOINIT;
}
DRESULT disk_read(BYTE drive, BYTE *buf, LBA_t sector, UINT count) {
  if (drive == 1) return rinc_mscIo(0, buf, sector, count);
  if (drive || !rinc_ready) return RES_NOTRDY;
  for (; count; count--, sector++, buf += 512) if (!rinc_readBlock(buf, (uint32_t)sector)) return RES_ERROR;
  return RES_OK;
}
DRESULT disk_write(BYTE drive, const BYTE *buf, LBA_t sector, UINT count) {
  if (drive == 1) return rinc_mscIo(1, (BYTE *)buf, sector, count);
  if (drive || !rinc_ready) return RES_NOTRDY;
  for (; count; count--, sector++, buf += 512) if (!rinc_writeBlock(buf, (uint32_t)sector)) return RES_ERROR;
  return RES_OK;
}
DRESULT disk_ioctl(BYTE drive, BYTE cmd, void *buf) {
  if (drive == 1) {
    if (cmd == CTRL_SYNC) return rinc_mscDev ? RES_OK : RES_NOTRDY; /* (FatFs's own buffers written by now; the stick's cache, once it's quiet: rinc_stickIdle) */
    if (cmd == GET_BLOCK_SIZE) { *(DWORD *)buf = 1; return rinc_mscDev ? RES_OK : RES_NOTRDY; }
    return RES_PARERR;
  }
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
/* a path as FatFs takes it: the stick's (/stick/...) on drive 1 ("1:/..."), mounted as it's first wanted;
   the card's as it is. 0: the stick's, and none's in */
static const char *rinc_fat(const char *path, char *buf, size_t cap) {
  if (strncmp(path, "/stick", 6) || (path[6] && path[6] != '/')) return path;
  if (!rinc_mscDev) return 0;
  if (!rinc_stickMounted && f_mount(&rinc_stickVolume, "1:", 1) != FR_OK) return 0;
  rinc_stickMounted = 1;
  snprintf(buf, cap, "1:%s", path[6] ? path + 6 : "/");
  return buf;
}
static int rinc_mkdirs(const char *path) { /* every directory along path (its last part is the file) */
  char d[128]; const char *p;
  for (p = strchr(path + (path[0] && path[1] == ':' ? 3 : 1), '/'); p; p = strchr(p + 1, '/')) { /* (after the stick's 1:/) */
    size_t n = (size_t)(p - path);
    if (n >= sizeof d) return 0;
    memcpy(d, path, n); d[n] = 0;
    { FRESULT r = f_mkdir(d); if (r != FR_OK && r != FR_EXIST) return 0; }
  }
  return 1;
}
static int rinc_fsOpen(const char *path, int how) {
  char fat[128];
  BYTE mode = how == rinc_READ ? FA_READ : how == rinc_UPDATE ? FA_READ | FA_WRITE | FA_OPEN_ALWAYS : FA_WRITE | FA_CREATE_ALWAYS;
  if (rinc_isOpen) { f_close(&rinc_file); rinc_isOpen = 0; }
  if (!rinc_mount()) { rinc_mounted = 0; return -1; }
  if (!(path = rinc_fat(path, fat, sizeof fat))) return -1;
  if (how != rinc_READ) rinc_storageChanged(path);
  { FRESULT r = f_open(&rinc_file, path, mode);
    if (r == FR_NO_PATH && how != rinc_READ) { /* (its folders made only when they aren't there: not each write, a lookup each over a thumb drive's USB) */
      if (!rinc_mkdirs(path)) return -1;
      r = f_open(&rinc_file, path, mode);
    }
    if (r != FR_OK) return rinc_failed(r); }
  rinc_isOpen = 1;
  return 0;
}
static int rinc_fsRead(char *buf, int n) { UINT got = 0; if (!rinc_isOpen || f_read(&rinc_file, buf, (UINT)n, &got) != FR_OK) return -1; return (int)got; }
static int rinc_fsSeek(uint32_t at) { return rinc_isOpen && f_lseek(&rinc_file, (FSIZE_t)at) == FR_OK ? 0 : -1; } /* (its end at most: reading, FatFs stops there; writing, the caller sees to it) */
static int32_t rinc_fsSize(void) { return rinc_isOpen ? (int32_t)f_size(&rinc_file) : -1; }
static int rinc_fsWrite(const char *buf, int n) { UINT put = 0; if (!rinc_isOpen || f_write(&rinc_file, buf, (UINT)n, &put) != FR_OK || (int)put != n) return -1; return n; }
static int rinc_fsClose(void) { int ok = rinc_isOpen && f_close(&rinc_file) == FR_OK; rinc_isOpen = 0; return ok ? 0 : -1; }
static int rinc_fsList(const char *dir, void (*each)(const char *name, int dir, void *ud), void *ud) {
  DIR d; FILINFO info; char fat[128];
  FRESULT r;
  if (!rinc_mount()) return -1;
  if (!strcmp(dir, "/")) { each("rinc", 1, ud); if (rinc_mscDev) each("stick", 1, ud); return 0; } /* (/: the card's Rinc, and the stick) */
  if (!(dir = rinc_fat(dir, fat, sizeof fat))) return -1;
  if ((r = f_opendir(&d, dir)) != FR_OK) return rinc_failed(r);
  while (f_readdir(&d, &info) == FR_OK && info.fname[0]) each(info.fname, (info.fattrib & AM_DIR) != 0, ud);
  f_closedir(&d);
  return 0;
}
static int rinc_fsRemove(const char *path) {
  char fat[128];
  if (rinc_isOpen) { f_close(&rinc_file); rinc_isOpen = 0; }
  rinc_storageChanged(path);
  return rinc_mount() && (path = rinc_fat(path, fat, sizeof fat)) && f_unlink(path) == FR_OK ? 0 : -1;
}
static FIL rinc_copyTo; /* (the one file open beside rinc_file: a copy's) */
static int rinc_fsCopy(const char *from, const char *to) {
  uint8_t buf[512]; UINT got = 0, put = 0; FRESULT r; int bad = 0; char fat[128], fatTo[128];
  if (rinc_isOpen) { f_close(&rinc_file); rinc_isOpen = 0; }
  if (!rinc_mount() || !(from = rinc_fat(from, fat, sizeof fat)) || !(to = rinc_fat(to, fatTo, sizeof fatTo))) return -1; /* (either way: the card, the stick) */
  if ((r = f_open(&rinc_file, from, FA_READ)) != FR_OK) return rinc_failed(r);
  if (!rinc_mkdirs(to)) { f_close(&rinc_file); return -1; }
  rinc_storageChanged(to);
  if ((r = f_open(&rinc_copyTo, to, FA_WRITE | FA_CREATE_ALWAYS)) != FR_OK) { f_close(&rinc_file); return rinc_failed(r); }
  while (!bad && f_read(&rinc_file, buf, sizeof buf, &got) == FR_OK && got) bad = f_write(&rinc_copyTo, buf, got, &put) != FR_OK || put != got;
  f_close(&rinc_file);
  return f_close(&rinc_copyTo) != FR_OK || bad ? -1 : 0;
}
static int rinc_fsRename(const char *from, const char *to) {
  FRESULT r; char fat[128], fatTo[128];
  if (rinc_isOpen) { f_close(&rinc_file); rinc_isOpen = 0; }
  if (!rinc_mount()) return -1;
  rinc_storageChanged(from); rinc_storageChanged(to);
  if (!(from = rinc_fat(from, fat, sizeof fat)) || !(to = rinc_fat(to, fatTo, sizeof fatTo)) || (from[1] == ':') != (to[1] == ':')) return -1; /* (on one drive) */
  return (r = f_rename(from, to[1] == ':' ? to + 2 : to)) == FR_OK ? 0 : rinc_failed(r); /* (FatFs takes the new name without its drive) */
}
static int rinc_fsMkdir(const char *path) {
  char inside[128], fat[128];
  if (!rinc_mount() || !(path = rinc_fat(path, fat, sizeof fat)) || snprintf(inside, sizeof inside, "%s/x", path) >= (int)sizeof inside) return -1;
  rinc_storageChanged(path);
  return rinc_mkdirs(inside) ? 0 : -1; /* (it, and those along the way) */
}
static const rinc_Fs rinc_sdFs = {rinc_fsOpen, rinc_fsRead, rinc_fsWrite, rinc_fsClose, rinc_fsList, rinc_fsRemove, rinc_fsCopy, rinc_fsRename, rinc_fsMkdir, rinc_fsSeek, rinc_fsSize};
void rinc_sdSetup(void) { gpio_init(rinc_PIN_DETECT); gpio_pull_up(rinc_PIN_DETECT); rinc_fs = &rinc_sdFs; }

/* between frames: the stick's cache flushed once writing to it has paused a quarter of a second, and at
   least every 2 seconds while it goes on (a long copy): a finished save is on the stick a moment after,
   and a long copy's files done so far within 2 seconds, so no eject's needed (a flush takes ~10 ms; a
   flush a file had made a copy of many no faster, the stick's own requests being what takes the time) */
void rinc_stickIdle(void) {
  uint64_t now = time_us_64();
  if (rinc_mscDirty && rinc_mscDev && (now - rinc_mscWrote > 250000 || now - rinc_mscFlushed > 2000000)) {
    rinc_mscDirty = 0;
    rinc_mscFlushed = now;
    rinc_mscSync();
  }
}
