# Drives: bundles without a limit, and files in and out (ROADMAP 1b)

Drafted 2026-11-21 from the user's sketch: the first round of thumb drives needs no import/export app.
A drive is a new place in storage; a zip dropped on the desktop or the web is a drive plugged in; the
desktop and the web have an export drive too, which downloads as a zip; Files is where nearly all of it
shows, and moving things stays manual (copying in Files) for now. That replaces what dropping a zip does
today (installing it). Recommendations are marked: the user decides.

## 1. Bundles with no fixed size

Today a bundle (an exported program's or page's apps and files) is at most 2 MB (rinc_BUNDLE_BYTES): a
static buffer on the desktop, and on the web the 2 MB the player reserves (512 KB before 2026-11-21,
which made the whole Rinc's page black). The card has no such limit, so bundles shouldn't either.

- Desktop: the bundle grows as files are added (malloc, doubled as it fills), and an exported program
  reads its own tail at whatever size. Only memory limits it.
- Web: the page asks the player for room for its bundle's size (an export that grows wasm memory to
  fit, `source(len)`), not a fixed buffer, so the player is no bigger in memory for a small page.
  Recommended over keeping the bundle in JS and reading it through the fs imports: that would save a
  copy, but the player reads bundle files in place today (apps' images, help), and the copy is cheap.
- A page bigger than a few MB is slow to open (base64 in the HTML); past a size (8 MB, say) `rinc -web`
  could write the bundle beside the page (page.zip) and the page fetch it. Later, if pages get that big.
- Tests: a 5 MB page (web_drop's runner) and a 5 MB exported program start and read their last file.
- Done (2026-11-21): rinc_bundleSize and a bundle built at its own size on the desktop (each file read
  on its own, no 2 MB arena, no 256-file most); the page asks the player for room (`source(len)`);
  web_drop's page and exported.py's program of 5 MB, each failing before.

## 2. One place for a drive: /stick

(Named /stick, the user, 2026-11-21: /drive could mean the card too.)

- A drive shows in storage as `/stick`, beside `/rinc`: `/stick/apps/chord/chord.rio`. Recommended: one
  at a time to start (a second plugged in, or a second zip dropped, replaces the first, with a notice),
  as the board's USB ports are few and most of the time it's one stick. The name leaves room for
  `/stick2` later if wanted.
- Which apps reach it: recommended, the system apps (Files first) and the console, as with storage
  outside an app's own folder today; an app is an island, its files its own, so a game doesn't read a
  drive. pc's file procs take the path as they do now; nothing new in pc but a way to ask whether a
  drive is in (`pc.stick() -> Int`: 0 none, or what kind) and to eject it.
- Underneath, one volume layer in rinc_fs: a path's first part picks the volume (the card or host
  folder, a zip in memory, the browser's storage, a USB drive), so every app gets drives at once, and
  the platforms only add volumes.

## 3. A zip dropped is a drive plugged in

- Desktop and web: a zip (or, on the desktop, a folder) dropped on the window or page mounts as /stick,
  read-only for a zip at first. Files shows it; copying `/stick/apps/chord` to `/rinc/apps/chord` is
  installing Chord. Nothing is copied by itself.
- Kept: an empty Rinc (no home: `rinc` with nothing installed, the empty page) still installs a dropped
  release zip (rinc-system-apps.zip) as today, as it has no Files to copy with. Recommended; the
  other way is a minimal Files built in.
- `rinc -drive stuff.zip` (or a folder) mounts one from the start, for tests and for developers.
- A second drop is a second stick: the drive is replaced whole, not merged (merging two zips' files
  makes a drive no one ever plugged in, and a stale file from the first could shadow the second's). If
  the drive was written to and not downloaded since, the drop asks first (download, replace, or keep).
  Recommended.
- Tests: a zip dropped shows in Files under /stick, nothing installed; a folder copied from it runs;
  a second zip replaces the first; the empty page still installs.

## 4. The export drive (desktop and web only)

- `/export`: an empty, writable drive in memory (on the web, kept in the page's storage until
  downloaded, so a reload doesn't lose it). Files copies into it like any folder.
- A command (in Files' command list, and Sys+P's) downloads it as `export.zip` (web: a download; desktop:
  a file beside storage, or a save dialog), then empties it if asked.
- Not on the board: there, the thumb drive itself is writable, so copying onto /stick is exporting.
- The user (2026-11-21): a writable stick on the desktop and the web too, in time (so the one-/stick
  way, below, is the direction; whether /export comes as well is still open). First round: read-only.
- Recommended alternative, simpler: no /export, but /stick writable everywhere (a dropped zip's copy in
  memory), "a new empty drive" when none is in, and "download the drive as a zip". One idea instead of
  two, and the same as the board's stick. The user to pick.
- Tests: files copied to /export, downloaded, the zip holds them (web_drop's runner sees the download).

Done (2026-11-21), first round: zip.c's stick, a zip whole in memory, read only, in front of the
platform's storage (rinc_stickMount: /stick's paths to it, the rest through; copying off it reads one
and writes the other); apps read /stick and / (rinc and stick); `rinc -stick`; a zip dropped on a Rinc
with home (desktop or page) is the stick, on one with none it installs; Files goes up from /rinc to /
while one's in. tests/stick.py (Files copying an app off it by keys, as a person would), web_drop,
install_bundle. Left: the board's thumb drive (6); a writable stick and downloading it (4); a folder
dropped on the desktop as a stick (still installed); taking one out (a command).

## 5. Files

Most of the work: volumes listed at its top (/rinc, /stick, /export), copying a file or a folder across
(asking before replacing; a progress line for a big one), eject, and download (the export drive's).
Moving between volumes is a copy then a delete. History of what came in and out (1b's log) later.

## 6. The board's thumb drive

TinyUSB's mass storage host, and FatFS (the card's) mounting the stick as a second volume. RAM is the
cost (1.5 to 3 KB: a volume, a sector buffer, the class's buffers; ROADMAP 7 lists where to free it).
Most of it can come from app space (recommended): only system apps reach /stick, and they're small
(Files leaves 184 KB of its 256 free), so the volume and its sector buffers live at the top of app space
while such an app runs, and the drive's mounted only then (flushed and let go as a game starts, so a
game keeps all 256 KB). What stays outside: TinyUSB's mass storage class state (a few hundred bytes:
the device, its command and status blocks), so a stick plugged in during a game is noticed, and mounted
when Files starts.
About 1 MB/s. Plugged in: /stick appears and Files says so; pulled out: it goes, a write in flight lost
(so Files writes whole files and says when it's done).

Done (2026-11-21), the board, read only: a thumb drive (through the hub too) as FatFs's drive 1 at
/stick (sd.c: TinyUSB's mass storage host, each request waited for with tuh_task run meanwhile);
listed, read, copied onto the card, Files into it; writing refused. Its RAM (456 bytes of TinyUSB's,
FatFs's second volume) paid by FatFs's tiny buffers (FF_FS_TINY: 1 KB back), so free RAM is where it
was; file speed on the card unchanged (boardspeed --check, before and after). Not from app space after
all: simpler, and nothing lost. Tried with the user's drive (Windows' System Volume Information on it).
Left: writing to it (with the desktop's and the web's, 4); taking it out cleanly while a file's open.

## Order

1, then 2 and 3 together (the volume layer, the zip volume, drops mount), then 5 (Files' copying), then
4, then 6 on the board. Each step tested on the desktop, the web (node runner and a browser shot),
Linux, and the board where it applies.
