# Rinc roadmap

The goal is a Rinc 1.0 that's stable and stays put: the `pc` module, the screen, sound, input and files
as the [platform spec](docs/spec/platform.md) describes them, so apps written for it keep running.
Rinc follows [Rio's roadmap](https://github.com/contextfreeinfo/rio/blob/main/ROADMAP.md), since an
app is a Rio program.

## Decide first: changes that would break apps

- Adjust apps and `pc` to each breaking Rio 1.0 change as it's decided.
- Freeze `pc`: names, arguments, and host proc order (compiled images depend on it), once Rio settles
  long argument lists.
- `pc.print` reads UTF-8, for fonts beyond ASCII.

## Next

- The synth (`pc.synth`, `pc.synthMake`: Sound's notes, made in C as they play) is in, with each
  voice's rate, bend, envelope and smoothing changeable as it plays, and waves of an app's own. Still
  to do: deciding by ear which rate and smoothing to start at; a small library that plays Sound's files
  in games; music (patterns of sounds across voices), in Rio on top or in the core.
- Editing code on the board: `rio fmt`, jumping to compile errors, completion.
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
- Fixed, not yet checked on the board or the web: Files' prompt and "New folder here", fullscreen mouse
  confinement, the web's Keyboard Lock.
