# Apps and storage

Every app is one Rio file, run as [running apps](running.md) says, with the whole app space to
itself. A Rinc finds apps in two places: its **storage** (the Fruit Jam's SD card, a folder on the
desktop, the browser's localStorage), and its **bundle**, the apps it carries with it. Storage comes first, so an app there takes
the place of a bundled one of the same name.

## Storage

A Rinc's storage is a folder of its own, `/rinc`, and apps can't reach outside it:

```
/rinc/apps/chord/chord.rio    an app: a folder of its name, holding its Rio file of the same name
/rinc/apps/chord/help/...     its help texts, Markdown (see help texts)
/rinc/save/chord/...          its own files (pc.saveRead and so on): only the app changes them
/rinc/stash/chord             where it was, for its next start (pc.stashRead): Rinc may drop it
/rinc/cache/chord/...         what it can make again: anything may clear it
/rinc/help/...                help texts that are no app's own (Rinc's and Rio's docs)
```

An app's name is 1 to 32 letters, digits, `_` and `-`. If its first line is a comment, that's its
description, by convention a title, a colon, and what it does:

```rio fragment
# Chord: an ADPCM chord on 8 voices, with pitch and pan
```

## Bundles

A bundle is the files a Rinc carries with it, laid out as in storage: an app is `apps/<name>/<name>.rio`,
or, compiled for a Rinc without a compiler, `apps/<name>/<name>.img` (a Rio image for a 256 KB app
space) with its description in `apps/<name>/<name>.info`. A bundle also names its **home app**, the one
`home` means, and its **storage folder**, where its apps' save spaces go.

- **The Fruit Jam's firmware** bundles home, the console, the editor and help by default, home as home,
  and Rinc's and Rio's docs as help texts.
- **The desktop and the web** carry none, so their programs stay small.
- **An exported game** carries its own apps, starting at its home app. One app on its own is home.
  It carries them compiled, unless one of them runs Rio itself or writes apps (the console, the
  editor): then their sources, so they can be read and changed too.

`home` means an app named `home` in storage if there is one, else the bundle's home app. An app on
storage of the same name as a bundled one takes its place.

Every Rinc reads the same format, all numbers little-endian:

| bytes | |
|---|---|
| 8 | `RINCBUN1` |
| 4 | how many files |
| 32 | the storage folder's name, 0-padded (empty: Rinc's own) |
| 32 | the home app's name, 0-padded |
| 104 each | each file's entry: its path (96 bytes, 0-padded, at most 95 long), where its bytes start (4, from the bundle's start) and how many (4) |
| | the files' bytes |

A desktop program carries its bundle at its end, followed by the bundle's size (4 bytes) and `RINCBUN1`
again. A web page carries it in base64.

## Finding apps

| proc | |
|---|---|
| `pc.apps() -> Int` | looks again, in storage and in the bundle, and says how many apps there are (the first 64, by name; `home` aside) |
| `pc.appName(i, out) -> Int` | app `i`'s name, from 0, in order of name ignoring case |
| `pc.appInfo(i, out) -> Int` | app `i`'s description: its first line after `#` and any spaces, if it starts with `#` |
| `pc.appSource(name, out) -> Int` | app `name`'s source (from storage, else the bundle), as much as fits in `out`: its whole length, or -1 if there's none |
| `pc.appSave(name, text) -> Int` | `text` becomes app `name`'s source in storage (its folder too, if it's new): 0, or -1 |
| `pc.appDelete(name) -> Int` | removes app `name`'s source from storage, and its folder if that's all it held: 0, or -1 |
| `pc.launch(name)` | after this frame, leave this app and run app `name` |

`appName`, `appInfo` and `appSource` copy as much as fits, and give the whole length (0 for an `i` that
isn't an app), so an app can tell when its buffer was too small.

## An app's own files

Every app has a save space of its own, `/rinc/save/<app>`, for high scores, settings and the like. A
file's name is 1 to 32 letters, digits, `_`, `-` and `.`, not starting with a dot:

| proc | |
|---|---|
| `pc.saveRead(file, out) -> Int` | the file's contents, as much as fits in `out`: its whole length, or -1 if there's no such file |
| `pc.saveWrite(file, data) -> Int` | `data` becomes the file's contents: 0, or -1 |
| `pc.saveDelete(file) -> Int` | removes it: 0, or -1 |

```rio
log(pc.saveWrite("best.txt", "42"))
out: Blob[16]
n := pc.saveRead("best.txt", out[:])
log(n, out[:n].asString(), pc.saveRead("nope.txt", out[:]))
log(pc.saveDelete("best.txt"), pc.saveRead("best.txt", out[:]))
```

```output
0
2 42 -1
0 -1
```

Where a Rinc has no storage, an app can't save: these give -1.

## Where an app was: its stash

An app's stash is what it keeps of where it was, for its next start: the text it was showing, an
unsaved draft. It writes it whenever it likes, as it goes. Unlike its save space, which only the app
changes, the Rinc drops an app's stash when it's told to start the app afresh: the system key with R
(this app again) or Q (back to the app before).

| proc | |
|---|---|
| `pc.stashWrite(data) -> Int` | `data` becomes the stash: 0, or -1 |
| `pc.stashRead(out) -> Int` | the stash, as much as fits in `out`: its whole length, or -1 if there's none |

And what an app can always make again, it keeps in `/rinc/cache/<app>`, by convention: anything may
clear that, at any time, and the app carries on, only slower.

## Home

A Rinc with apps starts at `home` (on the Fruit Jam, `system/home/home.rio`), which lists the apps in order of
title, and starts what's picked with `pc.launch`.

## The system key

One key belongs to the Rinc, not to apps: the **system key**. With another key it makes a
combination for the Rinc itself; on its own it does nothing, so it isn't pressed by accident. While it's
on, what's typed isn't the app's. It's:

- the Windows key, held, on a keyboard that has one and a Rinc that gets it (the Fruit Jam);
- a controller's guide button (the Xbox button), held;
- or, anywhere, Ctrl and Shift tapped together: the system key for the next key, as a lock (SYS). The
  next key uses it up, whether it makes a combination or not; tapping Ctrl and Shift again turns it off.
  (Ctrl+Shift+Esc belongs to Windows, so the system key here is a lock, never held.)

| combination | |
|---|---|
| system + Esc (or a controller's Back) | home, from any app |
| system + Tab (or a controller's RB) | the switcher: the recent apps, the one before this picked |
| system + Shift+Tab (or LB) | the switcher, from the other end |
| system + Q | back to the app before, this one left out of the recent apps |
| system + P | the performance strip, on or off |
| system + Enter | the whole display, or a window again, where the Rinc can (the desktop's F11 too) |

While the system key is on, a strip across the bottom of the screen shows these, apart from the app's
own screen (which it doesn't change), and the app gets none of the keys or buttons that make them. In
the switcher, Tab and Shift+Tab (RB and LB) move along the recent apps; Enter, or letting go of a held
system key, switches to the one picked, and Esc stays. Nothing here has a time limit.

**Locks.** Shift, Ctrl or Alt tapped twice locks it: it then counts as held for every key typed, until
it's tapped again. A tap is a press of those keys alone, let go within 0.4 seconds, with the two taps of
a lock within 0.4 seconds of each other; any other key pressed during them means no lock, so typing
capitals or Ctrl+letter never locks anything. A locked Shift capitalizes letters, a locked Ctrl makes a
letter Ctrl+letter, and on a key that isn't a character each adds its bit, as held ones do. Each lock
on shows as a badge at the bottom right (SHIFT, CTRL, ALT, SYS), over the screen, which shows through
around them.

The performance strip shows how long each frame's `update*` and `draw*` take, against the 1/60 s a
frame has: the latest, and a bar for each of the last few seconds' frames, 8 pixels high for the whole
1/60 s (green under half of it, yellow up to all of it, red over).

## The console

The console (`system/console/console.rio`) is an ordinary app: Rio, typed and run in a session of its own (see
[Rio inside an app](rio.md)), and commands for the apps in storage. Its help text, `console/README`, says how.

## Help texts

Help texts are Markdown, for the help app (`system/help/help.rio`) and any app that shows help. Each app's
own are in its folder, as `/rinc/apps/<app>/help/<path>.md` (named `<app>/<path>`), and those that are
no app's in `/rinc/help/<path>.md` (named `<path>`); a bundle carries them the same way
(`apps/<app>/help/...`, `help/...`), and storage takes the place of the bundle, as for apps. A name
leaves `.md` off: `rinc/spec/apps`. A set of texts (the first part of their names) with a `SUMMARY`
text is listed by that, as the docs' tables of contents are.

| proc | |
|---|---|
| `pc.helps() -> Int` | looks again, and says how many help texts there are (the first 128, by name) |
| `pc.helpName(i, out) -> Int` | text `i`'s name |
| `pc.helpRead(name, out) -> Int` | its text, as much as fits in `out`: its whole length, or -1 if there's none |
| `pc.help(topic)` | after this frame, the help app, at that text and maybe a heading: `pc.help("rio/spec/statements#if")` |
| `pc.helpTopic(out) -> Int` | for the help app: the topic it was opened at, taken (0: none) |

The help app shows them, and follows their links; its own help text, `help/README`, says how.

## The editor

The editor (`system/edit/edit.rio`) is an ordinary app too: one app's source at a time, up to 32 KB, with
the keys of most editors. Its help text, `edit/README`, lists them.
