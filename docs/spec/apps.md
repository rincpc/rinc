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
n := pc.saveRead("best.txt", out[])
log(n, out[..<n].asString(), pc.saveRead("nope.txt", out[]))
log(pc.saveDelete("best.txt"), pc.saveRead("best.txt", out[]))
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
| `pc.dir(path, out) -> Int` | a folder's entries into `out`, folders first, then by name, a line each (`d name`, `f name`): how many, or -1 if it's no folder |
| `pc.fileRead(path, out) -> Int` | a file, as much as fits: its whole length, or -1 |
| `pc.fileWrite(path, data) -> Int` | a file's contents, its folders made |
| `pc.fileDelete(path) -> Int` | a file, or a folder and everything in it |
| `pc.fileCopy(from, to) -> Int` | a file, or a folder and everything in it, to a path not taken |
| `pc.fileRename(from, to) -> Int` | to a path not taken |
| `pc.dirMake(path) -> Int` | a folder, at a path not taken (the folders along the way too) |

```rio
log(pc.fileWrite("/rinc/save/demo/notes/a.txt", "hi"), pc.fileCopy("/rinc/save/demo/notes", "/rinc/save/demo/copy"))
out: Blob[64]
n := pc.dir("/rinc/save/demo", out[])
log(n, out[0..<14].asString())
log(pc.fileCopy("/rinc/save/demo/notes", "/rinc/save/demo/copy"), pc.fileDelete("/rinc/save/demo"))
```

```output
0 0
2 d copy
d notes
-1 0
```

Files shows them: its help text, `files/README`, says how. Deleting there takes the whole name typed.

Pictures are PNG files. A picture in memory is its width and height (2 bytes each, low first), then its
pixels, a color number each, row by row, 0 see-through:

| proc | |
|---|---|
| `pc.pngWrite(path, picture, notes) -> Int` | a picture as an indexed PNG: Rinc's 256 colors are its own, 0 see-through, so other programs show it as Rinc does; with the app's notes about it (text up to 512 bytes, `""` for none: a sprite sheet's sprite size, say), kept in the PNG as a text chunk named `Rinc`, which other programs keep or ignore |
| `pc.pngRead(path, out) -> Int` | a PNG (8 bits a color, or indexed at 1 to 8 bits; not interlaced) into `out` as a picture, each color Rinc's nearest (exactly the same, for one `pngWrite` wrote): its size, or -1. `out` needs room for the PNG's rows as they come too: `w * h + 2 * h + 4` bytes for one of Rinc's |
| `pc.pngNotes(path, out) -> Int` | those notes, into `out`: how many bytes, or -1 (none, or no such PNG) |

```rio
pic: Blob[4 + 4]
pic[0] = 2
pic[2] = 2
pic[4] = 8
pic[7] = 12
log(pc.pngWrite("/rinc/save/demo2/tiny.png", pic[].asString(), "sprite 2"))
back: Blob[16]
log(pc.pngRead("/rinc/save/demo2/tiny.png", back[]), back[0], back[2], back[4], back[5], back[7])
notes: Blob[16]
n := pc.pngNotes("/rinc/save/demo2/tiny.png", notes[])
log(n, notes[0..<max(n, 0)].asString())
log(pc.fileDelete("/rinc/save/demo2"))
```

```output
0
8 2 2 8 0 12
8 sprite 2
0
```

## Calling an app

An app can start another for something, such as a file picked, and have the answer back. `pc.call(name,
args)` starts app `name` after this frame, with `args`, which it reads with `pc.args(out) -> Int` (how
long: 0 for none). When it's done, `pc.done(result)` goes back to the app that called it (it's left out
of the recent apps, as the system key's Q leaves an app out), and that app, starting again, reads the
result with `pc.result(out) -> Int` (how long, or -1 if there's none: it's taken). What's typed ahead,
after a call, is the called app's.

Files is a picker this way: `pc.call("files", "pick /rinc/save/chord")` starts it in that folder, and
Enter on a file gives its path back (Esc at the top, nothing: an empty result):

```rio fragment
out: Blob[96]
n := pc.result(out[:])
if n < 0
  pc.call("files", "pick")      # (the first time)
else
  log("picked", out[0:n].asString())
end
```

The editor saves to projects: a new app is a project, and an installed app opens read only, like a
page's source in a browser, until saving it makes its project.

## Settings

The whole Rinc's settings, not any one app's, are in `/rinc/settings`, a file a key, and any app can
read and change them: `pc.settingRead(key, out) -> Int` (as much as fits: its whole length, or -1 if
there's none) and `pc.settingWrite(key, data) -> Int` (0, or -1). Keys are named as save files are.

`active` is the app or project being worked on: `chord`, or its project, `chord+` (an installed app
can be active too, to read its source). The editor makes what it opens active, and starts with what's
active, so an app that picks one (a file browser, say) has the editor open it next. Home's Ctrl+Enter
makes the app picked there active and opens the editor, and marks the active one; the console's
`active name` makes `name` active.

`home` names the app `home` means (see bundles), when the Rinc wasn't started with one: `launcher`, say,
for a home app of another name. Without it, home is the app named `home`.

`theme` names the theme in use, `rinc/slate` say (see themes).

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
pc.print("hello", 8, 8, pc.theme(pc.Role.text))
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
| `pc.stashRead(key, out) -> Int` | what's kept under `key`, as much as fits in `out`: its whole length, or -1 if there's none |
| `pc.stashDelete(key) -> Int` | drops it: 0, or -1 |
| `pc.boot() -> Int` | how many times this Rinc has started (0 without storage): to tell a restart from being opened again |

And what an app can always make again, it keeps in `/rinc/cache/<app>`, by convention: anything may
clear that, at any time, and the app carries on, only slower.

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
| system + arrows (or the d-pad) | move the pointer, faster the longer they're held |
| system + V, C (or A, B) | the pointer's primary and secondary buttons |
| system + P | the performance strip, on or off |
| system + Enter | the whole display, or a window again, where the Rinc can (the desktop's F11 too). The whole display keeps the mouse on Rinc's screen; on the web, where the browser lets a page have Esc and its own Ctrl+N, Ctrl+T, Ctrl+W (Chrome, Edge: holding Esc leaves), after a click there. Alt+Tab and Cmd+Tab are always the system's |

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
text is listed by that, as the docs' tables of contents are. A project's texts are its own, in
`/rinc/projects/<app>/help`, named with its +: `chord+/README`, which the system key with H shows
while the project runs.

| proc | |
|---|---|
| `pc.helps(out) -> Int` | looks again, and puts the help texts' names in `out`, in order, each with a 10 after it, as many as fit: how many |
| `pc.helpRead(name, out) -> Int` | a text, as much as fits in `out`: its whole length, or -1 if there's none |
| `pc.help(topic)` | after this frame, the help app, at that text and maybe a heading: `pc.help("rio/spec/statements#if")` |
| `pc.helpTopic(out) -> Int` | for the help app: the topic it was opened at, taken (0: none) |

The help app shows them, and follows their links; its own help text, `help/README`, says how.

## More files

An app can be more files than one, in its folder: `include "parts/a.rio"` makes another file part of
this one, and `import .shapes` makes a module of `shapes.rio` (or `shapes/shapes.rio`), both from the
app's own folder; `import tween` is from the library, `/rinc/lib` (`tween.rio`, or `tween/tween.rio`),
shared by every app. Storage's files come first, then the bundle's (`apps/<app>/...`, `lib/...`). An app
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
