# The desktop

```sh
cmake -S . -B build-desktop -DRINC_PLATFORM=desktop   # fetches and builds SDL3 the first time
cmake --build build-desktop --config Release
```

The result is one program, with SDL3 (and, with MSVC, the C runtime) linked in. `-DRINC_SDL=OFF` builds
it without a window. A release is that one program, zipped on Windows and a `.tar.gz` on macOS and Linux (x86-64, and
arm64 for a Raspberry Pi 4 or 5 with 64-bit Raspberry Pi OS), so `rinc -help` and `rinc -license`
carry what would otherwise be files beside it.

```
rinc app.rio                      play, in a window (closing it quits); it's home until then
rinc game/   (or rinc ., in it)   an app of a folder: game/game.rio, with its other files (include, import)
rinc                              home (or the app the home setting names), in a window; with nothing
                                  installed, a window that says so and takes a bundle dropped on it
rinc -app chord                   an app from storage, by name, home for this session too
rinc -install apps.zip game/      apps into storage: a bundle of them (a release's rinc-system-apps.zip),
                                  an app's folder (what its files.txt lists, else all of it), or app.rio
  -data folder                    with any of these: storage in this folder instead
rinc -repl                        Rio typed in the terminal, a line at a time
rinc app.rio -m                   how near the app comes to its limits (see memory)
rinc app.rio -frames 120          no window: run 120 frames (A held for the first 3, unless -type)
  -o screen.ppm                   then save the screen
  -wav sound.wav                  and the sound those frames made
  -type 'x := 1\nlog(x)\n'        and type this, as fast as the app takes it (\n Enter, \e Esc, \t Tab,
                                  \xHH a byte as a terminal sends it: \x13 Ctrl+S, \x1b[1;5C Ctrl+right)
  -drop apps.zip                  and first, as if it were dropped on the window
  -hold 'a 10-40, right 20-30'    and hold buttons, as a controller would, in those frames (counted from
                                  0): left right up down a b x y lb rb ls rs start back, and the
                                  sticks pushed: lsleft lsright lsup lsdown rsleft rsright rsup rsdown
rinc app.rio -i app.rioi          compile to an image, and list the host procs its code calls
rinc a.rio b.rio -export game     a program to hand out (game.exe on Windows): rinc with the apps in it
  -home a                         the one it starts at (by default the only one, or one named home)
  -name folder                    its storage folder's name (by default its home app's)
  -docs rinc=docs                 help texts for the help app: the folder's .md files, as rinc/... (repeatable)
  -files apps/rinc=system/rinc    other files: all of the folder, as apps/rinc/... (under apps/, help/ or
                                  lib/; repeatable), such as the themes
  -runner path/to/rinc            a copy of that rinc instead, such as another system's from a release
rinc a.rio b.rio -web game.html   a page that plays them (see the web)
rinc -help                        all this; and rinc -license, the licenses of what's in it
```

In a window, the arrows and V, C, X and Z are the buttons (in a game: in a pointer app, such as home,
the letters type, and a controller's A and B are Enter and Esc), any controller SDL knows is player 1 (Xbox,
PlayStation, Switch and more), and Esc is the app's. The system key is Ctrl and Shift tapped together
(the operating system keeps the Windows key), or a controller's guide button; with Esc (or Back) it goes
home. F11, or the system key with Enter, fills the display, and keeps the mouse on it until it's a
window again (or another window gets the focus: Alt+Tab, Cmd+Tab).

Dropping a bundle of apps (a `.zip`) or an app's folder on the window installs it, as `rinc -install`
does, and then goes home. On Windows, a double-clicked `rinc.exe` closes the console Windows opens for it,
so there's only the window; started in a terminal, it prints there as usual.

## Storage

The desktop rinc carries no apps of its own, so the program stays small. Its storage is a folder in your
user data folder, laid out as on the Fruit Jam's SD card (`apps/name/name.rio`, and each app's
`save/name`):

| | |
|---|---|
| Windows | `%APPDATA%\rinc` |
| macOS | `~/Library/Application Support/rinc` |
| Linux and others | `$XDG_DATA_HOME/rinc`, or `~/.local/share/rinc` |

`rinc -install rinc-system-apps.zip` gets the same setup as the Fruit Jam: home, the console and the
rest of the standard apps, and the help texts, from the zip a [release](https://github.com/rincpc/rinc/releases)
carries (`rinc-demo-apps.zip` has the demos). A bundle's apps replace those of the same names; projects
and saves stay. `rinc -install mygame/` copies an app of your own in (the files its `files.txt` lists,
else all of its folder), as does `rinc -install mygame.rio`. Rinc itself never goes on the network.
Apps only see their Rinc's folder, never the rest of the computer.

`ctest --test-dir build-desktop -C Release` runs every app headless, checks that an exported game draws
exactly what rinc does, runs home, the console and the editor (typing at them), checks `pc.rio`, and runs the examples
in these docs.

## Exported games

`-export` writes a copy of rinc with a bundle of the apps after it (see
[bundles](../spec/apps.md#bundles)): each app compiled to an image. Started without an app, rinc looks
for a bundle at its own end, and starts its home app, in a window, or headless with `-frames`. An
exported game keeps its save files in a storage folder of its own name (`-name`), not Rinc's. With
`-runner`, one machine can make games for all three systems (on Linux and macOS, `chmod +x` a game made
elsewhere).
