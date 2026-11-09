# Rinc roadmap

The goal is a Rinc 1.0 that's stable and stays put: the `pc` module, the screen, sound, input and files
as the [platform spec](docs/spec/platform.md) describes them, so apps written for it keep running.
Rinc follows [Rio's roadmap](https://github.com/contextfreeinfo/rio/blob/main/ROADMAP.md), since an
app is a Rio program.

## Decide first: changes that would break apps

- Adjust apps and `pc` to each breaking Rio 1.0 change as it's decided.
- Freeze `pc`: names and arguments (images bind host procs by name, and a struct or a proc may grow at
  its end), once its long calls are settled: `blit` takes a `pc.Blit` now, and the rest are weighed in
  notes/pc-long-calls.md.
- `pc.print` reads UTF-8, for fonts beyond ASCII; and text measured (a `pc.textWidth`, say) rather
  than counted at 6 pixels a character, which the apps' layouts (and the command list's "38
  characters") assume today; and whether a font is the app's or the system's. Decide these before
  `pc` is frozen. Later, and not breaking: the fonts themselves (glyphs past ASCII, a second size)
  and the system apps' text in other languages.
- Points and rects, to discuss: a `pc.Point` (a struct of x and y, or `[2]Float`) and a `pc.Rect` (two
  points, or a point and a size) used across `pc` (rect, blit's source, clipping, hit tests) and kept by
  apps as values, if they're not much slower, since they could make drawing code easier to write and
  read. `blit` would take them too (notes/pc-long-calls.md).

## Next

- Rinc's library (library/, as /rinc/lib, installed with the system apps): done (2026-11-08):
  `tilemap`, tile maps read, written and drawn, pure Rio, the app owning its maps' memory (spec/maps.md),
  which studio's Maps uses; and pc.blit a row at a time when it isn't scaled or mirrored across (a screen
  of 8 by 8 tiles, 25 ms on the Fruit Jam before, 9 now). Next: one for songs, as studio's Music plays
  them; and libraries in the firmware's bundle, for a board with no card and to save apps' memory:
  - Mounted read-only as /rinc/lib (the bundle's lib/ entries, read in place in flash, as its apps are
    now), sources too, so the editor and help can open them; a card's /rinc/lib file of the same name
    wins, so a library can still change without new firmware.
  - Compiled once, the bytecode kept in flash and run from there, so an app pays only for the library's
    data (tilemap: ~17 KB of an app's 256 KB now, 13 KB of it code). Needs from Rio: a module compiled
    on its own, its tables kept and copied into each compile (Rio's roadmap, compiler speed), and a
    second, read-only code segment the VM calls into (apps' code is in their memory now). Kept by its
    source's hash, so a card's newer source is compiled as now. Its constants and strings stay in RAM
    (~3 KB for tilemap) unless the VM can read those from flash too. Check what running from flash, through
    the 16 KB cache, costs a library's hot loops and core 1.
- An app's own files by relative path: in (pc's file procs: a path not starting with / is in the app's
  folder, its project's or as installed; for reading). Next with it: run from the desktop's own files
  (rinc mygame/), the same path reads from there; and tools/send.py sending binary files (a PNG, a WAV)
  with the console's put, as hex (today it sends text only, so a picture arrives broken).
- Apps for file types: done (2026-11-07): each app's opens.txt, pc.appsFor, the setting
  files/appreg/<ext>, Files' Enter and Open with (asking when two apps open a kind and none's picked),
  apps given a file as "open /its/path", and the editor editing any text. Left: Ctrl+O in the editor
  starting in the current project's folder.
- Where an app's messages go: done (2026-11-06): each app's log in its cache space, and the Log app
  (Sys+O). Was: its log, and Rinc's lines about it (imageRead's "needs N bytes of room",
  runtime errors short of stopping it), go to the platform's output (serial, stdout, the browser's
  console) and to the screen only until the app draws. On the board, that's nowhere to be seen. Maybe a
  log app: each app's log kept as it runs (in its stash, say: per app, kept across a restart, dropped on
  Sys+Q, and out of the save space), shown per app, opened from a Sys key or the console; and maybe a
  mark on screen when there's something new.
- Paths: maybe show /rinc as /, since nothing gets out of it: /save/chord/best, /apps/chord. Apps,
  the docs and the console would all change at once; /rinc/ given would still have to work, or not.
- pc's reads into builders: done (2026-11-06). What reads into out takes a Blob[..], adds as much as
  fits, and gives the whole length; pictures are pc.Image (imageRead, imageWrite, blit(image, at, b?)),
  imageRead needing (w + 1) * h of room for Rinc's PNGs and saying so in the log. Left: the system
  apps read into arrays through views declared just before each call (fooIn: Blob[..] = foo[...]),
  made mechanically; where a buffer is only ever read into, a builder of its own would read better.
- The editor's screen flickers while a check runs (about half a second on the board, for a big app).
  Suspect: core 1 (HDMI and sound) running some of its code from flash, which the compiler (~225 KB,
  also from flash) pushes out of the 16 KB cache they share. Check: whether video's and sound's hot
  paths are all in SRAM; whether a long busy loop in an app (no compiling) flickers too; and what the
  frame loop does after a slow frame.
- A palette with a perceptual spread, in place of the 6x6x6 cube's many bright, saturated colors:
  done (2026-11-07): PICO-8's 32, then 12 OKLCH hue bands (8 vivid, 8 muted each), 16 tints and 16
  pure grays, 16 a row (src/core/palette.h, made by tools/palette.py), and Colors showing them a band
  to a row. Tried on 25 photos and game screenshots (people of many skin colors, skies, foliage, food,
  night, a reef): it holds up well; retuning (a sky-blue azure, darker muted steps) gained ~2.5% and
  cost near-duplicates, so it stays. What any 256 colors can't do is smooth gradients (skies band to
  the nearest color): that wants dithering, below.
- Dithered image reading: an option on pc.imageRead (in pc.ImageRead, say) to map a full-color PNG to
  the palette with error diffusion (Floyd-Steinberg, the error carried in OKLab) instead of the nearest
  color, so photos' gradients come out smooth; on a board, a row of error at a time, within imageRead's
  (w + 1) * h of room.
- Paint: dithering tools for gradients: a gradient fill between two colors (or along a ramp of the
  palette's bands) drawn with ordered (Bayer) dithering, so it stays clean as pixel art, and a dither
  brush or pattern fill.
- Themes: the gray theme (in progress) as the default; no theme called Rinc.
- Display modes, maybe (not decided: one mode for every app has its appeal): 640x480 in 16 colors,
  drawn straight to what's shown (no second buffer: the same memory), for a text editor with more on
  the screen in a bigger font; and half and quarter resolutions (160x120, 80x60) for apps that want
  big pixels. Each would need the HDMI line code (video.c) to read a different layout, apps a call to
  pick one, and the desktop and web players the same modes.
- Sleep, Sys+L: done (2026-11-06): the screen black and the sound silent everywhere, and on the board
  the HDMI pins let go of, so the monitor sees no signal and goes to standby; the app goes on; any key
  or button wakes it, once everything's been let go. Later: a lock (a PIN, say) on waking, on the same
  path, said plainly to be no real security (anyone with the board has its card).
- Commands for every serious app, as the editor has them: an app declares its list (label, keys, what
  it does), and Rinc draws and filters the list the same way for all of them. Ctrl+Shift+P stays the
  key (as most editors have it), with Sys+P opening the same list from any app; the editor's own list
  becomes the first user of the shared one. Most commands are an app's own; a few are every app's
  (Help, Home, copy and paste), which Rinc adds to each list.
- A clipboard shared across apps, kept through switching apps (and restarting): a file on the card
  (/rinc/clipboard: text, or a picture), read and written through pc procs (clipRead, clipWrite), so
  the editor's cut, copy and paste, Paint's, and others' all meet there; cached in PSRAM later, when
  that's in.
- Ctrl+Tab: done in the editor (2026-11-06: its stash "recent", the last 8, where each was; Ctrl+Tab
  the one before, again within 1.5 s the next along, Ctrl+Shift+Tab the other way). Left: the same in
  Paint and Sound; and walking while Ctrl is held, ending when it's let go (as browsers do), which needs
  pc to say whether a modifier is held.
- The synth (`pc.synth`, `pc.synthMake`: Sound's notes, made in C as they play) is in, with each
  voice's rate, bend, envelope and smoothing changeable as it plays, and waves of an app's own. Still
  to do: deciding by ear which rate and smoothing to start at, and the synth's loudness beside
  recorded sound (8 dB down for now: notes/native-synth.md); a small library that plays Sound's
  files in games; music (patterns of sounds across voices), in Rio on top or in the core.
- Editing code on the board: completion knows types now (pc.checkRio, Rio's check up to the cursor's
  line, then pc.completeRio; Ctrl+Space only), and a pause in typing checks the
  whole file and underlines its first error. Next: go to definition (across files: the check's tables
  need where each name was declared), then renames (a check that reports a name's every use).
- Speed, compiling and checking on the board (measured 2026-11-02, the editor checking itself, ~2,600
  lines in 9 files: 746 ms on the board, 4 ms on a desktop; pc alone, which every check and compile
  compiles first, 34 ms and 0.4 ms). Not started; ideas, roughly by what they'd likely give:
  - Profiled (Rio's bench/compile-profile.md): of a 763 ms check of the editor, the lexer ~150 ms of
    its own, reading the card 150 ms (twice reading the files whole: 2 KB windows, a file reopened at
    each include and back), lookups 55 ms, and half in the rest of Rio's compiler. Rio's roadmap has
    the compiler's side; the board's: the lexer's hot loop in SRAM, as the interpreter's is (the
    compiler runs from flash, through a 16 KB cache), and reading in bigger windows with files kept
    open (FatFS's fast seek), or sources cached in PSRAM (the SDK has `hardware_psram`).
  - pc compiled once, its compiler tables kept and copied into each check and compile after (34 ms
    each; most of a small app's compile).
  - Rio's own: see Rio's roadmap (name lookup by hash, a check that makes no code).
  - Imports instead of includes would only help once Rio can keep a module's compiled tables between
    compiles (it compiles every module every time now), so not yet: keep using whichever reads best.
- The editor and native code: the check's room (formatted, now 60 KB) leaves the editor 1.6 KB free,
  and rio_jit, which puts an app's native loops in its free memory (needing ~8 KB to compile them),
  now does nothing for the editor: on the board, it all runs as bytecode (it had 26 KB free before).
  Measured (2026-11-02, a big file, a full draw): 4.3 ms with native code, 6.6 ms without, of a
  frame's 16.7 ms; moving the cursor, next to nothing either way. So the editor's well inside a frame
  as it is. If its room is wanted back later: a smaller check room (Rio's check tables sized closer to
  need), or freeing the 32 KB edit scratch (an in-place move: a memmove-like builtin, or a byte loop). Desktop and web have no
  JIT and nothing to reserve: native code only ever uses what an app leaves free, so an app that fits
  one fits all.
- A diff view in the editor (inline, maybe): what changed since the file was saved, or between two.
- Key handling, an audit and a pass to refine it (for review): how each platform's keys become
  `pc.key()`'s codes (Ctrl with letters, and now Ctrl+Space and Ctrl+/ added one at a time), other
  keyboard layouts (where / or a letter's key is elsewhere, AltGr's characters), and typing text in
  other languages (i18n: dead keys, characters past ASCII).
- The themes, another pass: roles that are the same color where one is drawn on the other (select and
  bar in slate and green, faint and bar in green: a list's pick, and its dim text, didn't show; the
  editor works around them for now).
- Help search.
- Fonts: several fonts and sizes, 5x8 the default, and text measured, so apps lay text out by its
  size rather than counting 6-pixel characters (home's columns, the editor's, Paint's panels).
  - Diacritics at 5x8.
  - A 12-pixel font, for Chinese.
  - The 8x12 English font already drawn.
  - Proportional fonts, with a way to measure text.

## Later

- The card as a USB drive (branch `usb-drive`), or USB thumb drives.
- Installing bundles on the web and onto a card; zip bundles everywhere.
- Saving into the active project; apps registered for file types.
- Sprite depths: 1, 2, 4 and 8-bit sprites through small palettes, and a sprite library for apps.
- A hex editor, as a system app.
- Themes: editing titles, saving into the active project.
- Lending the app's free memory to Rinc while Rio isn't running.
- PSRAM (the Fruit Jam's 8 MB, unused so far), as a cache only, never where anything lives: the card
  stays the truth, everything read back from PSRAM is checked (a hash, kept with it, of what it holds
  and of what it was made from), and a check that fails just means doing the slow thing. A plan:
  1. Bring-up, alone first: the QMI's second chip select set up at our clocks (252 MHz system, with HSTX
     video and PIO-USB running), and a test at boot (patterns over all of it, or a sample each boot and
     all of it on request); PSRAM that fails it isn't used, and everything works as now without it.
     Reached through the XIP cache, which flash shares: measure what reading it costs flash's code.
  2. Files, cached: a source file's bytes, kept by path, size and its hash (or the card's date); a
     compile or check reads from there (about half of a big check's time now is the card). Written
     through: saving writes the card first, then the cache.
  3. Compiled pc, and apps' images: pc's compiler tables, copied into each check and compile; an app's
     image, kept as the card's image cache keeps it now (keyed by its sources' hashes), so starting it
     again needs no compiling and no reading.
  4. Apps swapped out whole, so going back to one is instant. Coming back as it was is the stash's job
     already (pc.stash*: an app keeps what it needs, and gets it back as it starts again), so this is
     for speed, and for what an app doesn't stash: its 256 KB of app space, its vm, and the host's state
     that's the app's (sound's voices, keys typed ahead, its screen, open files). The hard part, and
     why it's last: that host state is C statics all over today; it would have to be gathered into one
     per-app struct first, with a test that swaps and resumes every system app mid-use. Until then,
     images (3) and the stash get most of it.

## Maybe

- An input method for Chinese and Japanese.
- Right-to-left text: drawing first, editing much later if ever.
- USB speakers.
- Other boards: the PicoCalc, and someday a keyboard computer of Rinc's own.
- A VS Code extension, a language server, a web playground.
- A speed throttle matching the board.

## Known bugs

- Ctrl-B after the console app runs an app hangs the board's USB serial.
- Desktop tests now and then fail under heavy parallel runs on Windows (`ctest -j 12`): `stash_first`
  twice, `projects` once, each passing alone and in reruns. Maybe a file still held just after rinc
  exits, so a test's clean start (`rm -rf` of its data folder) misses it.
- Fixed, not yet checked on the board or the web: Files' prompt and "New folder here", fullscreen mouse
  confinement, the web's Keyboard Lock.
