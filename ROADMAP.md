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
- Editing code on the board: `rio fmt`, completion. (Coming back to the editor after a run that stopped
  with an error, compile or runtime, puts the cursor on it, in whichever part it was.)
- Help search.
- Fonts: several fonts and sizes, 5x8 the default.
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
