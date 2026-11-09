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
/rinc/apps/rinc/themes/...     Rinc's own themes (see themes): rinc is no app, so it isn't listed
/rinc/projects/chord/...      its project, chord+: the app in work (see projects), its own saves in save/
/rinc/save/chord/...          its own files (pc.saveRead and so on): only the app changes them
/rinc/stash/chord/...         where it was, for its next start (pc.stashRead): Rinc may drop it
/rinc/cache/chord/...         what it can make again: anything may clear it
/rinc/cache/rinc/apps/chord.rioi   Rinc's own cache: the app, compiled (see compiled apps)
/rinc/cache/rinc/projects/chord.rioi   and its project
/rinc/help/...                help texts that are no app's own (Rinc's and Rio's docs)
/rinc/settings/...            the whole Rinc's settings, shared by the apps: active, the app in work
```

An app's name is 1 to 32 letters, digits, `_` and `-`. If its first line is a comment, that's its
description, by convention a title, a colon, and what it does:

```rio fragment
# Chord: an ADPCM chord on 8 voices, with pitch and pan
```

## Bundles

A bundle is the files a Rinc carries with it, laid out as in storage: an app is `apps/<name>/<name>.rio`,
or, compiled for a Rinc without a compiler, `apps/<name>/<name>.rioi` (a Rio image for a 256 KB app
space) with its description in `apps/<name>/<name>.info`. A bundle also names its **home app**, the one
`home` means, and its **storage folder**, where its apps' save spaces go.

- **The Fruit Jam's firmware, the desktop and the web** carry none, so they stay small: their apps and
  help texts are in storage (on the Fruit Jam, its SD card, filled by `tools/install.py`). A firmware
  can be built with a bundle of its own, though, such as a game's.
- **An exported game** carries its own apps, starting at its home app. One app on its own is home.
  It carries them compiled, unless one of them runs Rio itself or writes apps (the console, the
  editor): then their sources, so they can be read and changed too.

`home` means, in this order:
- the app a Rinc was started with, for that session: `rinc chord` on the desktop makes chord home until
  it quits, so system+Esc comes back to it, though it's still named chord;
- a bundle's home app, if the bundle carries apps (an exported game, a web page, a firmware of its own);
- the `home` setting, if it names an app (see settings);
- else the app named `home`.

An app on storage of the same name as a bundled one takes its place.

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
| `pc.appSource(name, out) -> Int` | app `name`'s source (from storage, else the bundle), added to `out`, as much as fits: its whole length, or -1 if there's none |
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
| `pc.saveRead(file, out) -> Int` | the file's contents, added to `out`, as much as fits: its whole length, or -1 if there's no such file |
| `pc.saveWrite(file, data) -> Int` | `data` becomes the file's contents: 0, or -1 |
| `pc.saveDelete(file) -> Int` | removes it: 0, or -1 |

Whatever reads into `out` takes a byte builder (`Blob[..N]`, or a view, `Blob[..]`, over an array or
part of one) and adds to what it holds, so `out.len()` tells how much came in; the result says how long
the whole thing is, so more than came in means it didn't all fit.

```rio
log(pc.saveWrite("best.txt", "42"))
out: Blob[..16]
n := pc.saveRead("best.txt", out)
log(n, out.asString(), pc.saveRead("nope.txt", out))
out.clear()
log(pc.saveDelete("best.txt"), pc.saveRead("best.txt", out))
```

```output
0
2 42 -1
0 -1
```

Where a Rinc has no storage, an app can't save: these give -1. A project's save space is its own (see
projects), so work on it can't spoil the app's.

## Projects

An app in work is a project: `/rinc/projects/chord`, which is `chord+` wherever an app's name goes (a
star after it), and installing it makes it the app, `chord`. It's the same app at another stage, so
it shares the app's name, and what goes with the name: its stash, and its own cache. A star marks it
where it's listed: home, the switcher, the editor's list.

```
/rinc/projects/chord/chord.rio      its source
/rinc/projects/chord/help/...       its help texts
/rinc/projects/chord/save/...       its own save space: the app's saves, copied in on its first run
```

Only its saves are its own. A project being worked on can do anything to them, and the app's own,
`/rinc/save/chord`, the live ones, are left alone: on its first run (when it has no save space yet), the
app's saves are copied in, so it starts from real data. Deleting its `save` folder starts it over from
the app's again. Nothing goes back the other way.

`pc.launch("chord+")` runs the project (and `pc.launch("chord/test")` its tests: see an app's tests), and `pc.appSource`, `pc.appSave` and `pc.appDelete` take a
project's name too (`pc.appDelete` removes all of its folder: a project's own saves too, but never the
app's, in `/rinc/save`). The rest, each 0 or -1:

| proc | |
|---|---|
| `pc.appInstall("chord")` | project `chord+` installed as app `chord`: its files copied over the app's (its save space aside); the console's `install chord` too |
| `pc.appCopy(from, "tune+")` | app or project `from` copied as new project `tune+`: its files, its main file renamed `tune.rio` (a project's own saves aside) |
| `pc.appRename("tune+", "song+")` | project `tune+` renamed `song+`: its folder and its main file (and the active setting, if it was the active one) |

## Files

Any app can look through storage by path, as Files (`system/files/files.rio`) does: absolute paths,
`/rinc/save/chord/best`, with no `.` or `..` parts. `/rinc` is the root, for now: looking goes
anywhere in it, changing only inside it, never `/rinc` itself. (An exported program's storage is its
own folder, still `/rinc` to its apps.) Reading sees a bundle's files too (see bundles), after
storage's: an exported game's own files, say, or the themes a web page carries; changes go to storage.
Each gives 0, or -1, unless it says otherwise:

| proc | |
|---|---|
| `pc.dir(path, out) -> Int` | a folder's entries, added to `out`, folders first, then by name, a line each (`d name`, `f name`): how many, or -1 if it's no folder |
| `pc.fileRead(path, out) -> Int` | a file, added to `out`, as much as fits: its whole length, or -1 |
| `pc.fileWrite(path, data) -> Int` | a file's contents, its folders made |
| `pc.fileReadAt(path, at, out) -> Int` | a file from byte `at` on, added to `out`, as much as fits: its whole length, or -1 |
| `pc.fileWriteAt(path, at, data) -> Int` | `data` written over a file from byte `at` on (-1: at its end, adding to it; past its end, no), the file made if it isn't there (its folders too): where it went, or -1 |
| `pc.fileDelete(path) -> Int` | a file, or a folder and everything in it |
| `pc.fileCopy(from, to) -> Int` | a file, or a folder and everything in it, to a path not taken |
| `pc.fileRename(from, to) -> Int` | to a path not taken |
| `pc.dirMake(path) -> Int` | a folder, at a path not taken (the folders along the way too) |

```rio
log(pc.fileWrite("/rinc/save/demo/notes/a.txt", "hi"), pc.fileCopy("/rinc/save/demo/notes", "/rinc/save/demo/copy"))
out: Blob[..64]
n := pc.dir("/rinc/save/demo", out)
log(n, out.asString())
log(pc.fileCopy("/rinc/save/demo/notes", "/rinc/save/demo/copy"), pc.fileDelete("/rinc/save/demo"))
```

```output
0 0
2 d copy
d notes

-1 0
```

`fileReadAt` and `fileWriteAt` read and change a part of a file, so a big one (a log, a level) needn't
all be in memory at once. Each opens the file and closes it again, so what's written is on the card
when it says so, even if the power goes then.

```rio
log(pc.fileWriteAt("/rinc/save/demo/log", -1, "abc"), pc.fileWriteAt("/rinc/save/demo/log", -1, "def"), pc.fileWriteAt("/rinc/save/demo/log", 1, "XY"))
out: Blob[..4]
log(pc.fileReadAt("/rinc/save/demo/log", 2, out), out.asString())
log(pc.fileWriteAt("/rinc/save/demo/log", 9, "?"), pc.fileDelete("/rinc/save/demo"))
```

```output
0 3 1
6 Ydef
-1 0
```

Files shows them: its help text, `files/README`, says how. Deleting there takes the whole name typed.

Pictures are image files, PNGs so far. A picture in memory is a `pc.Image`: its `pixels`, a `Blob`, a
color number each, row by row, 0 see-through, and its `size`, a `[2]Int`:

| proc | |
|---|---|
| `pc.imageWrite(path, image, o?) -> Int` | a picture as an indexed PNG: Rinc's 256 colors are its own, 0 see-through, so other programs show it as Rinc does. `o` is a `pc.ImageWrite`: `notes`, the app's notes about it (text up to 512 bytes: a sprite sheet's sprite size, say), kept in the PNG as a text chunk named `Rinc`, which other programs keep or ignore; and `format`, a `pc.ImageFormat`: `fromName` (left out: the path ends `.png`) or `png`. 0, or -1 |
| `pc.imageRead(path, pixels) -> pc.Image` | a PNG (8 bits a color, or indexed at 1 to 8 bits; not interlaced), its pixels added to the builder `pixels`, each color Rinc's nearest (exactly the same, for one `imageWrite` wrote): the picture, its `pixels` viewing them (size `{0, 0}`, and none, if it can't be read). `pixels` needs room past what it holds for the PNG's rows as they come, each with a byte saying how it's packed: `(w + 1) * h` bytes for one of Rinc's, or any indexed or gray PNG; `(3 * w + 1) * h` for RGB, `(4 * w + 1) * h` with alpha. Without it, the picture has its size but no pixels, and the log says how much room it needs |
| `pc.imageNotes(path, out) -> Int` | those notes, added to `out`: how many bytes, or -1 (none, or no such PNG) |

```rio
pic: Blob[4]
pic[0] = 8
pic[3] = 12
log(pc.imageWrite("/rinc/save/demo2/tiny.png", {pixels = pic[], size = {2, 2}}, {notes = "sprite 2"}))
back: Blob[..16]
image := pc.imageRead("/rinc/save/demo2/tiny.png", back)
log(image.size, image.pixels.len(), image.pixels[0], image.pixels[1], image.pixels[3])
notes: Blob[..16]
log(pc.imageNotes("/rinc/save/demo2/tiny.png", notes), notes.asString())
log(pc.fileDelete("/rinc/save/demo2"))
```

```output
0
[2, 2] 4 8 0 12
8 sprite 2
0
```

## The app for a file

An app says which files it opens in `opens.txt` in its folder (listed in its `files.txt`, so it
installs with it): a line each, the extension, then what such a file is.

```
png  picture or sprite sheet
```

A line `*` says it opens any file (the editor's: `* any file, as text`). Such an app is listed in
Files' Open with for every file, after the apps for its kind, but Enter opens a file in it only once
it's been picked for that kind there.

So installing an app is all that registers it, and removing it takes it away. `pc.appsFor(ext, out) ->
Int` gives the installed apps that open files ending `.ext`, into `out` a line each, `name what`, in
order of name: how many. The one picked for them is the setting `files/appreg/<ext>`; with one app for
a kind, that one, picked or not. An app is given a file as `pc.call(name, "open /its/path")`, so it
reads `pc.args` as it starts, and opens that (the system key with Q comes back, to Files, say).

Files opens a file with Enter in its kind's app: the one picked, else the only one. With two or more
and none picked, it asks, in Open with, and the one picked there becomes the kind's. Open with (the menu's
W) lists them all, then Files' own view of the file's text: Enter opens it in the one picked, this time;
Ctrl+Enter makes it the kind's from now on. A file no app opens shows its text. Rinc's own apps open
these: the editor `rio`, `md`, `txt`, `json`, `csv` and `cfg` (text that isn't `.rio` is edited as
plain text: no Rio checking or completing); Help `md` too (read, as help is); Paint `png`; Sound `snd`;
Maps `map`; Music `mus`.

## Calling an app

An app can start another for something, such as a file picked, and have the answer back. `pc.call(name,
args)` starts app `name` after this frame, with `args`, which it reads with `pc.args(out) -> Int` (added to
`out`, as text that reads into a builder does; how long: 0 for none). When it's done, `pc.done(result)` goes back to the app that called it (it's left out
of the recent apps, as the system key's Q leaves an app out), and that app, starting again, reads the
result with `pc.result(out) -> Int` (how long, or -1 if there's none: it's taken). What's typed ahead,
after a call, is the called app's.

Files is a picker this way: `pc.call("files", "pick /rinc/save/chord")` starts it in that folder, and
Enter on a file gives its path back (Esc at the top, nothing: an empty result):

```rio fragment
out: Blob[..96]
if pc.result(out) < 0
  pc.call("files", "pick")      # (the first time)
else
  log("picked", out.asString())
end
```

The editor saves to projects: a new app is a project, and an installed app opens read only, like a
page's source in a browser, until saving it makes its project.

## Errors

An app that stops with an error, one that didn't compile or a runtime error, shows it on the screen,
and Rinc keeps where it was for an editor: `pc.appError(path, message) -> pc.AppError` gives its line
(0 when there's none), column and width (0 when they're not known, as for a runtime error's), and
whether it was a runtime error, with the file's path (the app's main file, or the part it was in) and
what was wrong added to the builders `path` and `message`, as much as fits. It's taken: a second call gives none. The editor
takes it as it starts, so coming back to it after Ctrl+R puts the cursor on the error.

```rio fragment
path: Blob[..128]
message: Blob[..96]
e := pc.appError(path, message)
if e.line > 0
  log(path.asString(), e.line, e.col, message.asString())
end
```

## Settings

The whole Rinc's settings, not any one app's, are in `/rinc/settings`, a file a key, and any app can
read and change them: `pc.settingRead(key, out) -> Int` (added to `out`, as much as fits: its whole length, or -1 if
there's none) and `pc.settingWrite(key, data) -> Int` (0, or -1). Keys are named as save files are, or
are such names with `/` between them, for settings in folders (`files/appreg/png`).

`active` is the app or project being worked on: `chord`, or its project, `chord+` (an installed app
can be active too, to read its source). The editor makes what it opens active, and starts with what's
active, so an app that picks one (a file browser, say) has the editor open it next. Home's Ctrl+Enter
makes the app picked there active and opens the editor, and marks the active one; the console's
`active name` makes `name` active.

`home` names the app `home` means (see bundles), when the Rinc wasn't started with one: `launcher`, say,
for a home app of another name. Without it, home is the app named `home`.

`theme` names the theme in use, `rinc/slate` say (see themes).

`files/appreg/<ext>` names the app picked to open files ending `.ext`: `files/appreg/png` holding
`paint`, say (see [the app for a file](#the-app-for-a-file)).

## Themes

A theme gives a palette color to each role the system apps draw with, so they look alike and code is
colored the same in all of them. `theme` (a setting) names the one in use, `owner/id`: the file
`themes/id.theme.rio` of app `owner`, in its folder (or the bundle's) or its save space
(`/rinc/save/owner/themes`), or of project `owner+`; `owner/id+` is only the save space's, a copy of a
locked one. Rinc's own themes are `rinc`'s: `/rinc/apps/rinc/themes`, `rinc/slate` and so on. Without
a theme, or for a role it leaves out, the color is Rinc's own. The Themes app picks, makes and edits
them (see its help).

`pc.theme(role) -> Int` is the color a role has in the theme in use, read as each app starts:

| `pc.Role` | Rinc's own | for |
|---|---|---|
| `bg`, `bar` | 1, 0 | the background; bars and boxes to type in |
| `text`, `soft`, `dim`, `faint` | 7, 6, 13, 5 | text; long text; what goes with it; hints |
| `accent`, `heading` | 10, 9 | what stands out, the cursor; other headings |
| `pick`, `select` | 12, 2 | behind the row picked; behind selected text |
| `link`, `error`, `ok` | 12, 8, 11 | links; what went wrong; what went right |
| `codeBg` | 17 | behind code, among text |
| `code`, `punct`, `keyword`, `type` | 7, 6, 14, 15 | code: names, operators, keywords, a capital's names |
| `call`, `number`, `string`, `comment` | 12, 9, 11, 13 | code: calls, numbers, strings, comments |

```rio
pc.cls(pc.theme(pc.Role.bg))
pc.print("hello", {8, 8}, pc.theme(pc.Role.text))
```

A theme's file is Rio constants, but Rinc only reads them, never runs it: lines of `name :: number` (a
star after the name or not), comments, and its first line, a comment, its title.

```rio fragment
# Slate: warm slate gray, with yellow and blue
bg*      :: 5    # dark gray
text*    :: 7    # white
```

## Where an app was: its stash

An app's stash is what it keeps of where it was, for its next start: the text it was showing, an
unsaved draft, something slow to make again. It keeps it under keys of its own (named as save files
are), and writes it whenever it likes, as it goes. Unlike its save space, which only the app changes,
the Rinc drops all of an app's stash when it's told to start the app afresh: the system key with R
(this app again) or Q (back to the app before).

| proc | |
|---|---|
| `pc.stashWrite(key, data) -> Int` | `data` becomes what's kept under `key`: 0, or -1 |
| `pc.stashRead(key, out) -> Int` | what's kept under `key`, added to `out`, as much as fits: its whole length, or -1 if there's none |
| `pc.stashReadAt(key, at, out) -> Int`, `pc.stashWriteAt(key, at, data) -> Int` | as `fileReadAt` and `fileWriteAt`, what's kept under `key`: a part of it, for a stash too big to hold at once (an undo log, say) |
| `pc.stashDelete(key) -> Int` | drops it: 0, or -1 |
| `pc.boot() -> Int` | how many times this Rinc has started (0 without storage): to tell a restart from being opened again |

Rinc keeps an app's stash when the app changes (a newer one installed, a project edited), so its drafts
aren't lost; Sys+Q and Sys+R are what drop it. So an app that stashes something in a layout of its own,
in bytes especially (an undo log, say), starts it with that layout's version, and checks it as it reads
it back: one it doesn't know, from a version of the app before or after it, it leaves be and starts
again. Checking what it reads as it goes (Maps' undo checks each step against the map) also catches a
file cut short, or a card that's failing.

### Leaving

An app that keeps its stash only now and then (once it's rested a moment, say, not each step of a held
arrow) can have a `leaving* :: proc()`. The Rinc calls it just before the app gives way to another
(the system key with Tab, Esc or H, an app called, `pc.launch`), and before it stops (the desktop's
window closed), so the app can keep where it is then, and coming back finds it as it was. Not when the
app's being forgotten (the system key with Q or R: its stash dropped). It can't draw or wait: the next
app's on its way. Rinc's own apps have one, and the editor keeps changes not yet saved with it, as a
draft, to have again.

```rio
place: Blob[..64]
leaving* :: proc()
  ok := pc.stashWrite("place", place.asString())
end
```

## Its cache, and its log

What an app can always make again (thumbnails, an index, a download) goes in its cache space,
`/rinc/cache/<app>` (a project's, `chord+`, its own), under names as save files have them. Anything may
clear that, at any time, and the app carries on, only slower.

| proc | |
|---|---|
| `pc.cacheWrite(file, data) -> Int` | `data` becomes the file: 0, or -1 |
| `pc.cacheRead(file, out) -> Int` | the file, added to `out`, as much as fits: its whole length, or -1 if there's none |
| `pc.cacheDelete(file) -> Int` | removes it: 0, or -1 |

Rinc keeps each app's log there too, as `log`: what the app logged lately and what Rinc said about it
(a runtime error, an `imageRead` that needs more room), the newest 4 KB of it, with a `-- started` line
each time the app starts. It's written as it comes, at most a second apart, and whenever the app stops.
The system key with O opens it in the Log app, at this app's log, and Esc there comes back; Tab goes
through the other apps' logs. On a board, that's the one place to see what an app logged once it's
drawing.

## Compiled apps

An app starts from its source, compiled whole. Rinc keeps what that makes, its Rio image, so the next
start can load it instead of compiling again:

```
/rinc/apps/help/help.rio              the source (or the firmware's, for an app built in)
/rinc/cache/rinc/apps/help.rioi        its image: Rinc's header, then Rio's image
```

Help, on the Fruit Jam, starts in about 30 ms this way, instead of about 140; breakout in about 12,
instead of 23.

**When an image is used.** The header says which source it's from and which Rinc made it:

| header | |
|---|---|
| `RINCIMG1` | what it is |
| the build | which build of Rinc made it (when it was built): a new firmware, or a new `rinc`, compiles again |
| the source's length, and its hash | 64-bit FNV-1a, of the source's bytes |
| the image's length | then Rio's image, that long |

An image is used only when all of those match the source as it is now. A Rinc can't go by file dates
(the Fruit Jam has no clock, and a card can be changed on another computer), so it reads the source
and hashes it, which is far quicker than compiling it. Anything else (no image, another build's, one
cut short or damaged, a source that's changed) and the app is compiled again, and its image written
over the old one. If the image can't be written (a full card, say), the app still runs.

**Checking once.** On a Rinc whose storage changes only through itself while it runs (the Fruit Jam), an
app checked once loads its image straight away from then on, without reading its source, until:

- the Rinc starts again (a start always checks: the card may have changed while it was off);
- anything in `/rinc/apps` or `/rinc/lib` is written or removed (the editor's saves among them);
- the card has to be mounted again, after a failure (it may be another card, or this one changed).

The desktop and the web check every time: there, other programs (another editor, another tab of the
page) can change storage while Rinc runs.

**The system key.** With R, the app starts again fresh: its stash and its image are both dropped, so
it's compiled anew. With Q, the app is left out of the recent apps and its stash is dropped, but its
image stays: it's still the image of its source, and only makes the next start quicker.

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
  next key uses it up, whether it makes a combination or not (but not the pointer's keys, below: with
  them, steer and click, then tap Ctrl and Shift again); tapping Ctrl and Shift again turns it off.
  (Ctrl+Shift+Esc belongs to Windows, so the system key here is a lock, never held.)

| combination | |
|---|---|
| system + Esc (or a controller's Back) | home, from any app |
| system + Tab (or a controller's RB) | the switcher: the recent apps, the one before this picked |
| system + Shift+Tab (or LB) | the switcher, from the other end |
| system + Q | back to the app before, this one left out of the recent apps (its stash dropped) |
| system + R | this app again, fresh: its stash and its compiled image dropped |
| system + H | this app's help (its `help/README`), in the help app; or a notice, at the bottom, that it has none |
| system + O | this app's log, in the Log app (see [its cache, and its log](#its-cache-and-its-log)); Esc there comes back |
| system + arrows (or the d-pad) | move the pointer, faster the longer they're held |
| system + V, C (or A, B) | the pointer's primary and secondary buttons |
| system + P | the command list: this app's commands, then the system's own (see [its commands](#its-commands)) |
| system + M | the performance strip, on or off |
| system + L | sleep: the screen black and the sound silent, and on a board the video signal stopped, so the monitor goes to standby; the app goes on. Once every key's let go, any key or button wakes it (that key isn't the app's). Nothing's locked |
| system + Enter | the whole display, or a window again, where the Rinc can (the desktop's F11 too). The whole display keeps the mouse on Rinc's screen; on the web, where the browser lets a page have Esc and its own Ctrl+N, Ctrl+T, Ctrl+W (Chrome, Edge: holding Esc leaves), after a click there. Alt+Tab and Cmd+Tab are always the system's |

Print Screen (no system key: the app never sees it) saves the screen as the app drew it, as
`/rinc/shots/<app>-<n>.png` (the next number free), and says so at the bottom. Over serial it's
`ESC [ 57361 u`, as a terminal speaking kitty's keyboard protocol sends it; the console's `get` brings
the file back (see [the Fruit Jam](../guide/fruit-jam.md)).

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

## Its commands

The system key with P shows a list of what the app can do, over its screen: the app's own commands,
then the system's (home, switching apps, this app's help and log, and the rest of the system key's).
An app may also show it with a key of its own (the editor's Ctrl+Shift+P), with `pc.showCommands()`.
Typing narrows the list to the commands with that in their names; up and down (or the d-pad) move,
Enter (a controller's A) does the one picked, as does a click on it; Esc (B), or a click outside,
closes it. While it's up, the app waits: no `update*` or `draw*`.

An app gives its commands with `commandList*`, which Rinc asks each time the list opens, so it can say
what makes sense then:

```rio fragment
commands := [2]pc.Command{
  {label = "Flip the sprite sideways", keys = "H", key = 'h'},
  {label = "Grid, shown or not", keys = "G", key = 'g'},
}
commandList* :: proc() -> []pc.Command
  return commands[0..<2]
end
```

| `pc.Command` | |
|---|---|
| `label` | what it does (38 characters show) |
| `keys` | its keys, as shown at the right ("" for none; 11 characters show) |
| `key` | what picking it types: the key, as `pc.key()` gives it next (before anything typed after), so a command is just its key; or `4096 + n`, a command of the app's own with no key |

Up to 56 of an app's are listed. A key or button still held as the list closes isn't the app's until
it's let go.

## The console

The console (`system/console/console.rio`) is an ordinary app: Rio, typed and run in a session of its own (see
[Rio inside an app](rio.md)), and commands for the apps in storage. Its help text, `console/README`, says how.

## Help texts

Help texts are Markdown, for the help app (`system/help/help.rio`) and any app that shows help. Each app's
own are in its folder, as `/rinc/apps/<app>/help/<path>.md` (named `<app>/<path>`), and those that are
no app's in `/rinc/help/<path>.md` (named `<path>`); a bundle carries them the same way
(`apps/<app>/help/...`, `help/...`), and storage takes the place of the bundle, as for apps. A name
leaves `.md` off: `rinc/spec/apps`. A set of texts (the first part of their names) with a `SUMMARY`
text is listed by that, as the docs' tables of contents are. A project's texts are its own, in
`/rinc/projects/<app>/help`, named with its +: `chord+/README`, which the system key with H shows
while the project runs.

| proc | |
|---|---|
| `pc.helps(out) -> Int` | looks again, and adds the help texts' names to `out`, in order, each with a 10 after it, as many as fit: how many |
| `pc.helpRead(name, out) -> Int` | a text, added to `out`, as much as fits: its whole length, or -1 if there's none |
| `pc.help(topic)` | after this frame, the help app, at that text and maybe a heading: `pc.help("rio/spec/statements#if")` |
| `pc.helpTopic(out) -> Int` | for the help app: the topic it was opened at, taken (0: none) |

The help app shows them, and follows their links; its own help text, `help/README`, says how.

## More files

An app can be more files than one, in its folder: `include "parts/a.rio"` makes another file part of
this one, and `import .shapes` makes a module of `shapes.rio` (or `shapes/shapes.rio`), both from the
app's own folder; `import tween` is from the library, `/rinc/lib` (`tween.rio`, or `tween/tween.rio`),
shared by every app. Storage's files come first, then the bundle's (`apps/<app>/...`, `lib/...`), then,
for the library, Rinc's own, built into every Rinc (its `library` folder: `tilemap`, `song` and `undolog`), so
those are there with no card, or none installed; a card's own file of the same name wins. An app
is at most 16 files besides its main one, of any length (they're read a part at a time as it compiles:
see [memory](memory.md)). Its compiled image notes every file it read (see compiled apps), so a change
to any of them compiles it again. On the desktop, `rinc game/` (or `rinc .`, in
the folder) runs the app of a folder, `game/game.rio`, its own files from beside it; exporting it
carries all of its folder. The system apps are made this way, each a main file and its `parts`.

```
/rinc/projects/game/game.rio          include "parts/levels.rio", import .shapes, import tween
/rinc/projects/game/parts/levels.rio
/rinc/projects/game/shapes/shapes.rio
/rinc/lib/tween.rio
```

### Rinc's library

Built into every Rinc (and installed in `/rinc/lib` with the system apps, where a newer one can go):

- `tilemap`: tile maps, read, written and drawn, as studio's Maps makes them ([maps](maps.md)).
- `song`: songs as studio's Music makes them, and their sounds (Sound's file): read a line at a time,
  and played on 4 of the 8 voices (from its `voice`), a pattern after another, round their loops; and
  a sound of the file on its own, for a game's effects. The app gives it room for the sounds' notes,
  128 bytes a sound (`song.SOUND_BYTES`), as many as it likes from the first: Music shows what a song
  needs, Sound what a file's sounds do, and `load` logs it when there isn't enough. The sounds come
  from the path the song names, or a file of that name beside the song, so a game keeps its song and
  sounds together in its folder. Music plays songs with it, so a game plays them just as Music does.

```rio fragment
import song
notes: Blob[4 * song.SOUND_BYTES]         # sounds 0 to 3 (song.ALL_SOUNDS: all 32)
tune := song.Song{notes = notes[], voice = 4} # voices 4 to 7: 0 to 3 the game's own
if tune.load("tune.mus")                  # its sounds too; else the log says why
  _ = tune.play(0, false)                 # from its first pattern (true: that one alone)
end
update* :: proc()
  tune.tick()                             # each frame: on to the next pattern when one's done
end
tune.sound(0, 2)                          # sound 2, once, on voice 0
```

- `undolog`: undo and redo kept on the card, in the app's stash, so they reach back as far as the app
  likes and are there again after another app has run. The app's steps are its own bytes; the library
  keeps them, groups them (one undo takes back a group), checks them as they're read back, and hands
  each to the app to undo or redo. A log has a head: the library's version, the app's (`version`),
  and what it's about (a file's path, say), so an app before or after this one, or another file, isn't
  taken on. Studio's Paint, Sound and Music, and the editor, keep their undo with it.

```rio fragment
import undolog
room: Blob[..1100]                        # the most a step can be, and 8 more (120 bytes at least)
u := undolog.Log{room = room, version = 1}
_ = u.resume("undo", "my.thing")          # the log as it was, if it's about this; else a new one
apply :: callback proc(step: String, back: Bool) -> Bool  # (undone, or redone: false if it doesn't fit)
  return true
end
u.group()                                 # what one undo takes back starts here
_ = u.step("its bytes")
_ = u.undo(apply)                         # u.broke: a step didn't fit, and the log started again
_ = u.redo(apply)
u.tick()                                  # each frame: what's waiting goes onto the card after a moment
u.flush()                                 # before leaving (leaving*)
```

| | |
|---|---|
| `u.start(key, about)`, `u.resume(key, about) -> Bool` | a new log under the stash's `key`; or the one there, if it's about `about` (and this version) |
| `u.group()`, `u.step(s) -> Bool` | the next step starts a group; a step (false: bigger than `room`) |
| `u.undo(apply) -> Bool`, `u.redo(apply) -> Bool` | the last group undone (its steps last to first), or the next redone, each through `apply` |
| `u.rewrite(s)` | in `apply`: the step's bytes made `s` (the same length), for an app that swaps what a step keeps with what's there |
| `u.retitle(about)` | what it's about, changed (a file saved under another name) |
| `u.tick()`, `u.flush()` | steps onto the card a moment after the last; at once |

### An app's tests

An app's tests go in `test.rio`, beside its main file. It includes the parts it tests, and any setup
part they need, then calls their procs and checks what they do with `assert`, so a failure stops it
at its line. `pc.launch("game/test")`, or `run game/test` in the console, runs it as the app does, on
every Rinc: its files from the app's folder and its saves the app's. Being the app's own files, an
app's tests are left out of installing, bundles and exports (and out of `files.txt`). The editor's
are an example: `system/edit/test.rio`, with `parts/setup.rio` its sizes and colors.

## The editor

The editor (`system/edit/edit.rio`) is an ordinary app too: a file at a time, by path, up to 32 KB,
with the keys of most editors. Ctrl+O opens a file, through Files (called to pick one), and
Ctrl+Shift+S saves as another (Files, called to pick where: what's unsaved waits in the editor's
stash meanwhile). An installed app's files, and anything outside `/rinc`, are read only: saving one of
an app's saves it in its project, made from the app first (see projects). Ctrl+R saves and runs the
app or project the file is in, and Ctrl+Shift+P lists every command. Its help text, `edit/README`,
lists the keys.
