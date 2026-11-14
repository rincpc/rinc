# Rinc roadmap

The goal is a Rinc 1.0 that's stable and stays put: the `pc` module, the screen, sound, input and files
as the [platform spec](docs/spec/platform.md) describes them, so apps written for it keep running.
Rinc follows [Rio's roadmap](https://github.com/contextfreeinfo/rio/blob/main/ROADMAP.md), since an
app is a Rio program.

## Decide first: changes that would break apps

- Adjust apps and `pc` to each breaking Rio 1.0 change as it's decided.
- Freeze `pc`: names and arguments (images bind host procs by name, and a struct or a proc may grow at
  its end), once its long calls are settled: `blit` takes a `pc.Blit` now, and the rest are weighed in
  notes/pc-long-calls.md. Added since (2026-11-09): `pc.keyHeld(k)`, a key held down (for drawing
  with Space held as the arrows move, in Maps and Paint), on the desktop, the web and the board.
- `pc.print` reads UTF-8, for fonts beyond ASCII; and text measured (a `pc.textWidth`, say) rather
  than counted at 6 pixels a character, which the apps' layouts (and the command list's "38
  characters") assume today; and whether a font is the app's or the system's. Decide these before
  `pc` is frozen. Later, and not breaking: the fonts themselves (glyphs past ASCII, a second size)
  and the system apps' text in other languages. Leaning (2026-11-09): text stays on a grid of
  6-pixel columns (cursors, columns and cutting text to fit stay simple), and apps ask `pc` for the cell
  (`pc.textCell()`, say: 6x9 now) instead of assuming 6 and 9, so a taller cell (6x12) changes only
  rows. Wide characters (CJK) take two columns, so measuring is counting columns, not pixels.
- Points and rects, to discuss: a `pc.Point` (a struct of x and y, or `[2]Float`) and a `pc.Rect` (two
  points, or a point and a size) used across `pc` (rect, blit's source, clipping, hit tests) and kept by
  apps as values, if they're not much slower, since they could make drawing code easier to write and
  read. `blit` would take them too (notes/pc-long-calls.md).

## Next

- Priorities (2026-11-13), in order (the 2026-11-09 list's 1 to 3 done: Print Screen and buttons held
  over serial, the song library, Files' save taking a typed path; and Sound's = at a sound's first note):
  1. Rio's bytecode smaller: done (2026-11-13, Rio f328c2f, lib/rio bumped): 35% off an app's code (Maps
     100 KB to 66), with the JIT as fast as before, bytecode alone 6 to 16% slower on the Fruit Jam
     (rio/plans/smaller-instructions.md has each step's numbers).
  2. Music (more important than sprite flags, the user said 2026-11-13): a pitch shift on each sound a
     pattern plays, kept in the song (as every tracker does); the sounds themselves don't change, so
     Sound isn't needed to make a tune (picking sounds in Music and changing them in Sound was
     unusable, as the user found). The song library plays the shift with
     pc.synthBend (the synth's own: each note made at the shifted pitch, at the same speed and length,
     not the sound played faster). The user leans to a horizontal piano roll (time across, pitch up and
     down), to see timing better than tracker rows. Up to 8 channels wanted: Rinc mixes 8 voices, and
     the song library takes 4 (4 to 7; 0 to 3 a game's effects), so an 8-channel song shares voices with
     effects (an effect taking a channel while it plays, as PICO-8's do) or Rinc mixes more voices
     (about 250 bytes of RAM each, and core 1's time, to measure). Decided (2026-11-13):
     - A song takes the voices it wants, up to all 8 (using them all is the maker's choice); more
       voices maybe later.
     - Patterns, each edited as a piano roll (sounds placed in time, each at its pitch shift), then
       arranged into the song: two views, the pattern's roll and the song's row of patterns, with a
       clear way between them.
     - Playing the whole song doesn't move the view on its own (jarring): a playhead shows where it is,
       and following it is asked for.
     Leaning (to settle when it's built): no nested patterns (repeats are the same pattern placed again,
     and the song's loops); the roll's grid, a pattern's length in steps and each sound's own speed.
     Considered (2026-11-13): a sound as just a pattern, which nests and composes. Against it: the
     player then needs a stack and a depth limit (no recursion in Rio), cycles to forbid, voices
     counted by flattening, tempo passed down; and the levels that make PICO-8 easy blur. A middle way:
     fixed levels (a sound's notes, a pattern's sounds, the song's patterns), each shown as the same
     kind of piano roll, so a sound's notes could be edited in Music too. Decided: the middle way;
     sounds edited in Music later. The data model, for review: studio/plans/music-patterns.md.
  3. Sprite flags, as PICO-8's (fget, fset), and names for them: a sprite's flags are a few classes it
     belongs to (solid, ladder, hurts), a simple ECS-like tag a game asks of a tile. Set in Paint, kept
     with the sheet, read by tilemap and games. Built (2026-11-13): a sheet format of Rinc's own (.sheet:
     a text head, then pixels and flag records as bytes), the `sheet` library, Maps reading it, and Paint's
     flags (up to 32, named, by each sprite's top left pixel; stranded ones counted, snapped or taken
     out): studio/plans/sprite-flags.md. Next: tilemap asking a tile's flags, and a game using them.
     The thoughts before it (kept for the record):
     - Where: in the PNG's notes (the tEXt chunk Paint keeps its sprite size in already: one file, the
       picture still a plain PNG elsewhere), as a line a flag's name and a run of hex, a byte a cell.
       Other editors may drop the notes (they keep the pixels), so Paint says when a sheet it opens has
       lost them; a side file (.flags) is the fallback if that's common.
     - Buffers (the user, 2026-11-13): loading and saving flags shouldn't take much buffer space, and
       should stay easy to use: a 256x256 sheet has 1,024 cells, so its flags are 1 KB, 2 KB as hex,
       past the notes' 512 bytes today. Maybe kept as bytes, not text; maybe not PNGs at all (worth a
       rethink, maybe overkill: later). The user, 2026-11-13: moving away from PNG entirely is worth
       considering. Then: a sheet format of Rinc's own (its pixels a byte each, or packed, its sprite
       size, flags and names together), read with no inflate buffers; PNG kept as an import and export
       in Paint, for other tools and sharing.
     - Kept by cell, not by sprite number: each 8 by 8 cell of the sheet has its flags, so changing the
       sprite size (8, 16, 32) or the sheet's width doesn't move them; a bigger sprite's flags are its
       top left cell's (or all its cells' together). Tested, as the auto-test rule says.
     - Named and fast: names in the notes (flag 0 solid), the game asks once (`SOLID := sheet.flag("solid")`,
       a bit) and then tests a byte (`sheet.has(t, SOLID)`): no strings in the loop.
  4. An ergonomics pass: making a game as quick to write as on PICO-8, keeping Rinc general. Ideas to
     weigh: a `game` library of the usual (a sheet and a map loaded by name, sprite and map drawn in a
     call, btn and btnp, sounds and a song played by number), so a small game is PICO-8-short; the
     templates new projects start from; and Rio's own rough edges met in Cloudlamp (one-pass order,
     views returned from procs, logging inside an expression). Style (the user, 2026-11-13): short
     names for modules, which Rio has already (`import tilemap as tm`, `import game as g`), in the
     docs, examples and templates: a module's name repeats at every use, so short there cuts noise,
     though other names stay long and readable.
  5. Studio: state kept per file, not per app.
  6. A card's stale /rinc/lib copy shown: done (2026-11-13): after a compile, each /rinc/lib file read
     that isn't Rinc's own of that name is said, in the app's log and on the screen (tests/stalelib.py).
     And each app an island (the user): paths relative to the file that names them, not absolute ones.
     Done for songs' sounds (the song library reads them from the song's folder; Music saves them so)
     and maps' sheets (tilemap reads them from the map's folder; Maps saves them so).
     To think about (the user, 2026-11-13: deferred): a card's newer library copy, an upgrade beyond the
     firmware's, is said the same as an old one (it differs). Ideas: a version in each library file
     (VERSION*), so only an older copy is said and a newer one just logged; and an app's own copy, in its
     folder, imported as `import .song`, for one app alone (each app an island).
  7. A read-only tilemap for games (load, at, spriteAt, things), importable on its own: Cloudlamp pays
     4,350 of its 4,845 instructions for all of tilemap, editing and writing included (less once 1 is
     done; still worth it). Likely not needed now (2026-11-13): Rio drops procs nothing calls (its
     ROADMAP, Done recently), so Cloudlamp keeps only the tilemap it uses: its code 37.4 KB to 23.1 KB
     (59 procs dropped), 16.3 KB more free. The same goes for a `game` library (4): an app pays for what
     it calls. Needs rinc's lib/rio bumped once Rio's commit is in.
  8. Cloudlamp as a walkthrough in the docs ("making a game with studio").
  9. Maps' memory: 36.6 KB free since Rio's smaller code (2026-11-13; was 2 KB), so less pressing; what
     binds now is the compiler's instruction table, 84% full (12,560 of 14,945). Before, the plan was
     Maps made smaller on its own (handling smaller maps to start with, say), rather than a bigger app
     space or code in flash, which carry risks.
     Constraints breed creativity: getting by on little is part of the point, so limits are loosened
     with care, and the hardware Rinc needs kept as small as it can be.
  9b. Sizes on disk (the user, 2026-11-13), for later: exported programs and web pages (rinc -export,
     -web) compressed, each app whole at a go maybe, the players inflating them as they're read (they
     have inflate already, for PNG): no oversized .exe or page, whatever a web server does. Maps, and
     maybe other formats, binary too, later. And run lengths
     for sheets' pixels, if their size on the card matters (a 128x128 sheet: 19.5 KB as Rinc's own,
     7 KB as a PNG). Binary where it's for speed and size (sheets' pixels and flag records), text where
     it's for people (their heads, songs, maps).
  10. Drawing speed (docs/guide/fruit-jam.md, Speed): a scaled sprite takes the slow path, a pixel at a
     time as blocks (a solid 16x16 at 2x, 33 us: 299 in 10 ms, where Cloudlamp draws everything at 2x),
     and a small rect or blit costs about 3.4 us before its first pixel (2,960 8x8 rects in 10 ms).
     Fast paths for 2x (and mirrored) sprites, and less per call. A grid drawn as one picture (falling
     sand: 200x200 at 60 frames a second) is already fast; a rect a cell is what's slow (about 50x50).
  Later: working as a team (below, in Later).
- From making Cloudlamp (apps/cloudlamp, 2026-11-09: its sheet, map, sounds and song made with studio's
  apps on the board, typed at over its serial port; the game needs only Rinc):
  - Done: an app run from its folder (rinc game/) reads its own files there, as installed (pc.fileRead,
    fileReadAt, imageRead: tests/folderfiles.py); it read storage's only, so a game's map and sheet
    weren't found.
  - Done: exported, as a program and as a page (2026-11-09): pc.imageRead and pc.imageNotes read an
    app's picture from the bundle too (they read storage's only: an exported game's sheet was empty;
    tests/bundleimage.py), and on Windows, -export to a name with no extension makes name.exe.
  - Done: a song player in the library (song, above): Cloudlamp read .snd and .mus itself, 60
    lines a game shouldn't need.
  - A screen capture over serial (the console's `shot`, as a PNG): the board's screen can't be seen
    from the computer, so testing over serial is blind but for the files an app saves; bug reports too.
  - A card's /rinc/lib copy wins over the built-in one, and goes stale: the card had an older tilemap.
    Listing /rinc/lib (below) could say when a card's copy is older than the one built in.
- Next (2026-11-09): /rinc/lib listed in Files and the editor, the library built in shown there (and
  pc.rio, to read); memory for apps, the next big step (code in flash, or a bigger app space: see Later).
- Rinc's library (library/, as /rinc/lib, installed with the system apps): done (2026-11-08):
  `tilemap`, tile maps read, written and drawn, pure Rio, the app owning its maps' memory (spec/maps.md),
  which studio's Maps uses; and pc.blit a row at a time when it isn't scaled or mirrored across (a screen
  of 8 by 8 tiles, 25 ms on the Fruit Jam before, 9 now). Then (2026-11-09) song, songs as studio's
  Music plays them; and libraries in the firmware's bundle, for a board with no card and to save apps' memory:
  - Mounted read-only as /rinc/lib (the bundle's lib/ entries, read in place in flash, as its apps are
    now), sources too, so the editor and help can open them; a card's /rinc/lib file of the same name
    wins, so a library can still change without new firmware.
  - Done (2026-11-09), for imports: the library built into every Rinc (desktop, web, board: tools/library.cmake),
    after a card's own and the bundle's. Left: its files listed in /rinc/lib (Files, the editor, help), and
    pc.rio there too, to read.
  - Done (2026-11-09): undolog, undo and redo kept on the card (spec/apps.md: Rinc's library).
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
  - First, whatever the fonts: every app laid out from the text cell `pc` says, not 6 and 9 (the
    top and bottom bars, lists, the editor's and console's rows). That's what would fall apart if the
    cell changed under them.
  - A 6x12 font, half-width ASCII and the like, and 12x12 full-width glyphs (two columns) for Chinese,
    Japanese and Korean, as 12-dot CJK fonts are (Shinonome, say): a system setting, with the
    language. 6 pixels across keeps 53 columns; 12 down gives 20 rows, not 26, which the editor and
    console feel most. Whether it becomes the default: decided once it's seen on screen.
  - The editor's (and console's) font chosen on its own, since rows matter most there.
  - Glyphs in flash, read in place (no RAM): ASCII at 6x12 about 1.2 KB; Japanese (kana and about
    7,000 kanji) about 130 KB; all of Chinese's 20,000 about 360 KB; Korean's 11,172 syllables about 200
    KB whole, much less built from their parts. Drawing costs about what it does now, plus decoding
    UTF-8 and a table lookup.
  - The bigger work past fonts: typing CJK (an input method), the apps' text and help translated, and
    text in Rio's byte strings cut and stepped by character (UTF-8) and width. Right-to-left and
    shaped scripts (Arabic): not planned.
  - Diacritics at 5x8.
  - The 8x12 English font already drawn (40 columns, so not the system's: for games, maybe).
  - Proportional fonts: not for the system's UI (its layouts are grids); a library font for games'
    titles and dialogue, with a way to measure text, maybe.

## Later

- Working as a team, on the board itself (ideas, 2026-11-10: not decided; for later, after the priorities in Next). No chat, no netplay, no
  Wi-Fi needed; phones do the talking. What's missing is moving files safely without pulling the card:
  - Not the board as a USB drive (its card shown to a computer): tried before and not working yet, and
    it makes a board depend on a computer, which Rinc shouldn't.
  - A phone on the board's USB hub, for a moment (the user's idea, 2026-11-10): the board is the USB
    host already (keyboards, controllers), so a phone plugged into its hub can be reached. Android: MTP
    (the phone's shared folder, as a computer sees it: no app needed), or Android Open Accessory, which
    a small Rinc app on the phone answers, to automate it (pick a project, send, receive, back it up to
    the phone's cloud or git). iOS: an iPhone as a USB device needs Apple's MFi licensing, so not this
    way; iPhones take the local Wi-Fi or QR roads below.
  - No camera or microphone on a Rinc by default (the user's rule, 2026-11-10): a Rinc can't be made to
    spy. So nothing that needs one to reach the board: QR codes go out only, and no sound modem in.
  - Sync over the USB cable: a desktop `rinc sync` (or a page, through WebSerial or WebUSB, which
    Android phones' Chrome has over a USB-C cable) that brings a project's changed files across, both
    ways, with the serial REPL's put and get. A phone then carries files between teammates' boards,
    or to a shared folder (git, a cloud drive) that the team already uses: Rinc needs no network of its
    own.
  - Bundles that say who made them: a project's bundle (a .zip, as now) signed with its maker's key
    (Ed25519, small and fast on the RP2350), and checked on the way in: trusted keys are a list on the
    card, added by hand once per teammate. Then any channel is safe enough (a drive, a phone, a
    stranger's USB stick): what isn't signed by someone trusted says so before it's installed.
  - Merging by file: a project's files with a version each (its hash, and the one it came from), so a
    sync says which are new, changed on one side, or changed on both (then both kept, side by side,
    for a person to choose): no merge tools on the board.
  - If Wi-Fi ever: optional, for those who want it for shared work, perhaps as a USB Wi-Fi adapter
    plugged in when wanted (so a Rinc without one has no radio at all), or the Fruit Jam's own ESP32.
    Local only, never the internet, so no certificates to update (the ESP32's firmware never reflashed
    for them): two boards, or a board and a computer on the same network,
    paired once by a short code shown on one screen and typed on the other (a pre-shared key, and
    authenticated encryption: Noise, or ChaCha20-Poly1305), then syncing as over the cable.
  - QR codes (the user likes these): the board shows a project as a run of QR codes, a phone's camera
    reads them (its Rinc app, or a web page: no install), and the phone keeps or sends them on. One
    way only (no camera on a Rinc); the way back is the hub or Wi-Fi. A QR code carries about 2 KB, so a project of 40 KB is 20 codes,
    shown a few a second: fine for code and small art.
  - Files as sound, played by the board for a phone to hear (a modem, as on cassettes): out only, as QR
    codes are (no microphone on a Rinc); fun, no radio at all, slow.
  Recommended first: signed bundles (every channel needs them), then QR codes out (no radio, no
  install, delightful), then local Wi-Fi with pairing (both ways, iPhones too), then the Android app
  over the hub. Each keeps the board the place where the work happens.

- Apps prebuilt into a firmware (their images run in place from flash, freeing their code's RAM): later, and
  never in the default firmware, which stays free of apps; a build option for a board of its own (a game
  console, say). Meanwhile, code in RAM; libraries built in (as source) already.
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
