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
- `pc.print` reads UTF-8, for fonts beyond ASCII.
- Points and rects, to discuss: a `pc.Point` (a struct of x and y, or `[2]Float`) and a `pc.Rect` (two
  points, or a point and a size) used across `pc` (rect, blit's source, clipping, hit tests) and kept by
  apps as values, if they're not much slower, since they could make drawing code easier to write and
  read. `blit` would take them too (notes/pc-long-calls.md).

## Next

- The synth (`pc.synth`, `pc.synthMake`: Sound's notes, made in C as they play) is in, with each
  voice's rate, bend, envelope and smoothing changeable as it plays, and waves of an app's own. Still
  to do: deciding by ear which rate and smoothing to start at, and the synth's loudness beside
  recorded sound (8 dB down for now: notes/native-synth.md); a small library that plays Sound's
  files in games; music (patterns of sounds across voices), in Rio on top or in the core.
- Editing code on the board: completion knows types now (branch `exact-completion`: pc.checkRio, Rio's
  check up to the cursor's line, then pc.completeRio; Ctrl+Space only), and a pause in typing checks the
  whole file and underlines its first error. Next: go to definition (across files: the check's tables
  need where each name was declared), then renames (a check that reports a name's every use).
- Speed, compiling and checking on the board (measured 2026-11-02, the editor checking itself, ~2,600
  lines in 9 files: 746 ms on the board, 4 ms on a desktop; pc alone, which every check and compile
  compiles first, 34 ms and 0.4 ms). Not started; ideas, roughly by what they'd likely give:
  - Reading sources from the card: about half of the 746 ms. The reader keeps one file open, reopening
    and seeking (a walk of FAT's cluster chain) each time compiling moves between a part and the file
    including it. Keep a few files open, use FatFS's fast seek, or read bigger windows than 2 KB.
  - The compiler runs from flash, through a 16 KB cache, and is ~225 KB: its hot paths (the lexer,
    name lookup) in SRAM, as the interpreter is, if the room can be found.
  - pc compiled once, its compiler tables kept and copied into each check and compile after (34 ms
    each; most of a small app's compile).
  - Rio's own: see Rio's roadmap (name lookup by hash, a check that makes no code).
  - Imports instead of includes would only help once Rio can keep a module's compiled tables between
    compiles (it compiles every module every time now), so not yet: keep using whichever reads best.
- The editor and native code: the check's room (formatted, now 60 KB) leaves the editor 1.6 KB free,
  and rio_jit, which puts an app's native loops in its free memory (needing ~8 KB to compile them),
  now does nothing for the editor: on the board, it all runs as bytecode (it had 26 KB free before).
  To decide: a smaller check room (Rio's check tables sized closer to need), freeing the 32 KB edit
  scratch (an in-place move, which Rio would need: a memmove-like builtin, or a byte loop), or
  measuring first whether the editor's frames are slower enough to matter. Desktop and web have no
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
