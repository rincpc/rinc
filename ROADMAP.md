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

- **Next, in order (the user, 2026-11-23):**
  1. Cloudlamp's dialog font and text (4 below), made on the board with the apps, as a person would.
     Done (2026-11-23): dialog.png, 5x6 cells, proportional, the 31 characters its lines use, drawn in
     Paint on the board (by keys); the notice library takes a font (Notice.font: its box two of its lines
     high; tests/notice.rio); Cloudlamp's dialog in it, its title taken out (title.png left in its folder,
     unused, for the user to keep or drop). Left for the user: touching up letters (its "s" reads small).
  2. Close (drop a file from the recent list; Ctrl+W or such, 1b's note) and Ctrl+Tab in every studio app.
     Done (2026-11-23): rinc's recent library (the editor's list and walk, moved out): Ctrl+Tab and
     Ctrl+W (close: out of the list, the one before opened; not saved: again drops the changes) in the
     editor, Paint, Maps, Music and Sound. Paint and Music had almost no room (372 and ~500 bytes free):
     they say the pick's name instead of drawing the list (the compiler leaves draw out); Paint's fill
     stack made 2-byte (2 KB), its flag records sized to a whole sheet of 8s (1,280: 1.4 KB) and its flag
     buffer to 384 bytes. Tests: rinc's editor.py (Ctrl+W), studio's recent.py (each app). Ctrl+W chosen
     (the user's note: one finger slipping; it only drops a file from the list, and asks first if unsaved).
     The user (2026-11-23): later, maybe simple 3D and an ultra easy 3D modeling app, after a transformed
     blit with shading; and text formatting, text animation and visual novel features in a library.
  The user (2026-11-23): the recent apps (Sys+Tab's) kept over a restart: done (2026-11-23), in rinc's
     own stash (/stash/rinc/recent), an app gone since left out, garbage there never stopping home; Sys+Q
     at home forgets them. Tests: tests/recentapps.py.
  3. Studio state per file (done 2026-11-23: each file's place, in the recent list); Maps' cleanups
     (paste anchor, Ctrl+T for every layer); map parts as brushes.
  4. The app's log over serial while it runs, if it helps working on the board (Sys+M already shows frame
     times on screen).
  5. Sometime: drawing speed, and a transformed blit (rotated, scaled: 3D-ish); the Cloudlamp tutorial
     (it would help polish how things work).
  6. Drawing primitives (the user, 2026-11-24). Done (2026-11-24), first batch: pc.line; pc.rect with
     pc.Shape (outline, radius: rounded corners); pc.ellipse (a square box: a circle); pc.triangle (three
     points, filled or outlined); in C, clipped, charged as rect is; tests/shapes.py. Then pc.curve (a
     cubic Bezier, stroked: lines a few pixels long, its points by forward differences, adds alone); the
     ellipse outline's rows worked out once, the triangle's slopes once. Speed (the user, 2026-11-24):
     designed for it as they're made; a pass over all of them with drawing speed, after the transformed
     blit. Then pc.polygon (2026-11-24): its points a [][2]Int (Rio's ffi taking slices of [2]Int now:
     a sig's [ii]); filled by non-zero winding, a row
     at a time, no room needed (64 crossings a row at most). Polygons and curves take [2]Float points
     (2026-11-24: between pixels, for smooth turning and moving). Then pc.Path (2026-11-24): lines,
     curves and arcs in parts, filled (non-zero: holes the other way round) or outlined; 128 points in the
     app's memory; filled a row at a time, curves and arcs flattened only where they cross it, no room
     needed. Arcs alone: a path of one (an
     option of ellipse). Also (the user, 2026-11-24): gradients, with dithering options, as primitives
     (linear and radial fills), which Paint uses too; and Paint's tools to grow with them (line, rect,
     ellipse, triangle, curves, gradients). Before: pc had only cls, pset, rect, blit and print. Candidates,
     in the host (fast, clipped, every platform alike): line; a rectangle's outline; ellipses and circles,
     filled and outlined; filled triangles and polygons; arcs; Bezier curves (maybe in a library, from
     line). Antialiasing: hard with a fixed palette (a blend needs the colour between two, which the
     palette may lack): ideas, a dither of the two colours at the edge, or blending to the nearest palette
     colour (a table, 256 by 256 by a few levels, too big for the board as is); later, if wanted.
  The user (2026-11-24), before long: a review of rinc and every app for consistency and cohesion, and
     for people coming from modern systems: far less opaque, obvious to use, still efficient by keys.
  The user (2026-11-24): lines thicker than a pixel, dashed, with end caps (flat, square, round) and
     joins (miter, bevel, round), for line, curve, the outlines and paths: pc.Shape growing (width, dash,
     cap, join). (An outline of just a star's outside: its outside points as the polygon, a concave one,
     already: the user, 2026-11-24, nothing to trace.)
  The user (2026-11-24): transformed drawing for everything, not just blit: one current transform (2x3,
     floats) over all drawing; vector shapes transform their points (a rect a 4-point polygon, ellipses and
     rounded rects flattened), images and text by each screen pixel's source (the transformed blit); no
     transform set, today's fast paths. Basics stay Int (pset, rect, blit, print). Recommended (the user
     agreeing, 2026-11-24): two calls, one applying a transform on top of the one there (nesting: a
     part's own turn inside its whole's), one setting it outright, each giving back the one it replaced
     (to put back: no stack in the host); kept until changed, and reset at each frame's start and each
     app's (a forgotten put-back can't leak further); the clip rect alike. Perspective (to decide): the
     textured triangle taking each corner's depth from the start, so perspective (a divide each 8 to 16
     pixels, a few percent more) is an option later, not a new call. For 3D later: the app
     (or a library) projects its points and draws screen triangles textured by their corners' image points
     (the same machinery as the transformed blit; affine, as early consoles').
     Clipping (the user, 2026-11-24): a clip rect (every span cut to it: nearly free; scrolled panes, the
     UI library, split screens); then convex clips (any transformed rect, triangle or convex polygon: one
     span a row, so each span's cut cheaply); and any shape at all through a mask in the app's memory
     (a bit a pixel: 9.6 KB for the screen), drawn into with the shapes themselves.
  The user (2026-11-24): drawing modes through colour tables, as Picotron's. A full table (the colour
     drawn by the colour under it) is 256 by 256, 64 KB: too much to keep in the host; recommended (not
     decided), in steps: a remap for every drawing call (as blit's map, 256 bytes); then a shade table:
     each pixel covered becomes table[the colour under it] (256 bytes each: shadows, light, tint,
     glass-ish), the palette's hues and steps making darker and lighter ones easy to make (a tool, or a
     library); a few translucent colours (256 bytes each); a whole 64 KB table for an app that wants it.
     The user (2026-11-24): every table in the app's own memory (a Blob it passes), none kept by the
     host. Each a pixel read and written: slower than a fill, still a C loop.
  The user (2026-11-24): a general UI library for Rinc, with layout (buttons, lists, fields, menus,
     dialogs, scrolling, focus by keys), used by the system and studio apps so they look and work alike
     (the consistency review). Recommended (not decided): immediate mode (each frame, ui.button("Save")
     draws it and says if it was pressed), the app keeping its own state; the library keeping just what
     it must between frames (focus, the hot item, scroll), by an id per widget. It fits Rio (no closures,
     no allocation), the apps already redraw each frame, and the board's room (no tree of widgets kept).
     Layout as rows and columns that the calls fill in order (a cursor that moves, sizes from text), with
     a measuring pass only where a size must be known first.
  Held (the user, 2026-11-23): more sticks, /mount/stick and other mounts: still being considered.
  Not now (the user, 2026-11-23, the recommendation): patterns from another song (copy and paste
  covers it; a live link only if a real game asks).

- **Priorities (2026-11-15), in order** (the user: keep them all listed; the 2026-11-14 list below is
  the history):
  1. Rio's compiler tables sharing their room (Rio's ROADMAP, Next), before Maps is trimmed (9 below).
     Done (2026-11-20): a program past a table compiled again with its tables as a check counted them
     (Maps: 397 procs, or about 15,900 instructions, from 14,800). Maps' limit now is its code; code
     packed while compiling assessed and written up for later (Rio's ROADMAP: about +5,000 instructions
     for Maps, then its running memory binds: its data, 146 KB of 256, is the place to look first).
  1b. In and out, one way everywhere (the user, 2026-11-20: once current work settles, a priority):
     getting files, apps and projects into and out of a Rinc made easy and the same on every Rinc.
     A thumb drive on the board, a zip dropped on the desktop or web player, an export (rinc -export,
     -web), a download from the web player: each one a volume that comes and goes, as a drive does
     (a zip dropped is a drive plugged in; taking it out is saving it back). An app for it (Files', or
     its own): what came in and what went out, when and from where, so work can be followed both ways;
     and it ties to local history (what changed on the Rinc, and since when), so "what's new since I
     last took this out" is one look. Recommended: one volume layer under Files (the card, a drive, a
     zip, the host's folder alike: a path like /stick/...), so every app gets it at once; the in-and-out
     app built on it; the thumb drive's RAM first (6 below: a few KB to free).
     First round (the user, 2026-11-21): no in-and-out app yet; a drive is /stick, a dropped zip
     mounts as one (replacing today's installing on drop), an export drive on the desktop and the web
     downloads as a zip, and Files does the copying. Bundles lose their fixed size (the card has none).
     Plan: plans/drives.md. Done (2026-11-21): step 1, bundles of any size (5 MB pages and programs
     tested).
     Then (2026-11-21): /stick on the desktop and the web, read only (a zip dropped on a Rinc with home,
     rinc -stick), Files up from /rinc to /, copying off it; and on the board, a thumb drive at /stick
     (its RAM paid by FatFs's tiny buffers). Then writing to the stick everywhere, and the desktop's
     and the web's downloaded as a zip (the command list). Left: a page's stick through a reload, taking
     one out (plans/drives.md).
  1d. `/` as Rinc's root (the user, 2026-11-21): apps say `/projects/...`, not `/rinc/projects/...`;
      storage keeps /rinc/ on the card. Devices stay short names at the top (recommended: /stick, no
      /devices/ level). Plan: plans/root.md; after writing to the stick. Done (2026-11-22): apps use /;
      the board's card updated. Left: boot and the clipboard into folders (the top only folders). Boot done (2026-11-23):
      /save/rinc/boot, the old /boot carried over once and taken away (tests/boot.py; the board's moved).
      The old /clipboard file is a leftover (the clip library keeps /stash/rinc/clip): deleted on the
      user's card; recommended: boot to /save/rinc/boot (moved once). Asked (the user, 2026-11-22): are
      /save/rinc and /stash/rinc better as sys? Today rinc there is Rinc's own owner name, fixed (an
      exported game's storage takes the game's name, but inside it, Rinc's own is still rinc), so nothing
      changes with an export's name; sys would only say "Rinc's own" more plainly. Low priority, the
      user to decide.
  1e. More than one stick, later (the user, 2026-11-22: no rush): today the first drive plugged in is
      the stick and others are ignored; pulled out, the next isn't taken up till replugged (small fix:
      take the next one still in). Perhaps a folder of them (/mount/stick, say), so more devices can
      come; to plan, not to rush.
  1f. Long jobs in pieces (the user, 2026-11-22: very important): Files copying a file or a folder a
      piece a frame (pc.fileReadAt and fileWriteAt), a progress line, Esc to stop; a thumb drive takes
      about 120 ms a small file, so a folder of 50 froze the screen 6 s. Then other long jobs the same
      way (deleting a big folder, saving a big map).
      Done (2026-11-22), Files' pastes: a copy, or a move between drives (copied, then the sources
      taken away), a job a piece a frame (files parts/jobs.rio: a folder made, or 8 KB of a file, a
      step); "Copying 9 of 28: name (Esc stops)"; Esc, or leaving Files, stops it with no file half
      there. tests/stick.py (part of it after a few frames, all of it after enough, Esc, left in the
      middle of a big file, a move onto the stick), failing on the Files before. On the board: about
      3 files a second onto the thumb drive, the screen keeping up. Then (the user: /apps pasted onto the
      stick looked hung): the progress line had been too long for Files' message (53 characters) with a
      long path, so nothing showed: now the file's name, cut to fit; the folder shown refreshed as the job
      makes one in it; deleting a folder a job too ("Deleting 3 of 40"; an app's or project's through
      pc.appDelete as before). On the board: /apps (157 files) onto the stick, about 1.5 a second with
      pictures, shown as it goes. Then (the user: still felt locked up, arrows did nothing): folders
      looked in as steps too (the first piece at once, "8 of 8+" while more's found), a small file
      read and written whole, steps a few milliseconds a frame, Files usable meanwhile (only what
      changes files waits); deletes charged a write's cost on the board clock; and what's copied is
      what's shown (a filter typed dropped hidden selections: up from /stick, save typed, Ctrl+C had
      copied the stick). Then (the user: still unclear what was happening; wanted a plan made first, a
      box in the middle with a bar, and the plan's surprises handled): a job is planned (a folder
      looked in a step, its files and sizes written to a plan in /cache/files/job, as long as it needs
      to be), then done by it; a box from the moment it begins (Looking: folders, files, size; Making
      folders: 21 of 74; 17 of 92 files, 85 KB of 630 KB; the bar by bytes, a folder as 4 KB); Esc
      stops it, the rest waits (modal); what's not as planned (gone, another size, a damaged file, a
      folder too big to list) said in the log and counted; its start and end logged. Tests: said at
      once, 400 files, a damaged file on a zip stick. Left: a job carried on after coming back; the
      stick's flush after each write (each folder made takes about 0.4 s there) maybe once a file.
      Then (the user: stalled at "Looking: 74 folders, 105 files", then done): two board things. A
      file's size was found by reading it all, 256 bytes at a time (rinc_readFile): now storage's own
      size, and only what fits read; planning /apps (157 files) 1.2 s. And the display: the picture is
      copied at a refresh only between an app's frames, so frames back to back, each longer than the
      display's (a stick's writes), left the screen as it was the whole job: now a frame none of whose
      pictures was shown waits for the next refresh (video.c's rinc_waitShown, at most a 60th of a
      second; the copy 3% slower). The user saw it moving. The job's log says how long planning and
      the whole job took. Not tested automatically (it's HDMI's): tried on the board.
      Then the stick's speed (/apps onto it, 157 files, 74 folders): 98 s to 76 s. Folders made only when
      a write finds them missing (not looked up each write); the stick flushed a quarter second after
      writing pauses and at least every 2 s while it goes on (not at each close: it was never the cost);
      a folder made cleared from 32 KB of zeros in flash in a few writes (FatFs's ff_zeros), not 32
      writes of a sector. Measured (the firmware console's stick: reads, writes, flushes): what's left is
      Pico-PIO-USB's one packet a 1 ms frame (64 KB/s, ~8 ms a block); FatFs's tiny buffers made no
      difference (2,504 reads without, 2,529 with). The stick to the card: 52 s. Further, only by more
      packets a frame in Pico-PIO-USB (a submodule: a patch, or upstream; CPU time in its interrupt):
      maybe 5 to 10 times as fast; not now (the user to decide).
  1c. To fix (the user, 2026-11-20): the whole Rinc's web page shows only black in their browser.
      Done (2026-11-21): its bundle, with both docs, had passed the web player's 512 KB for one, and ran
      over its memory; one limit now, 2 MB (rinc_BUNDLE_BYTES), for rinc -web's bundles and the player's.
      web_drop has a page of 700 KB. The speed app is left off the demo site, as bench and micro are.
  2. Board speed everywhere, by default (the user, 2026-11-16): the desktop and the web run an app no
     faster than the Fruit Jam would (Rio's instructions metered to the board's measured rate, drawing
     and sound calls charged their board cost, a frame waiting when its budget's spent), so a game
     made on a computer isn't a surprise on the board, and a too-slow frame shows on the desktop and
     in tests (Paint's font line, 2026-11-16, was fine on the desktop and unusable on the board).
     Not perfect, but close; a turbo mode only when asked (a setting, or rinc -turbo). With it: a
     frame-time readout, and tests that fail when a frame runs over on board time. What's measured:
     plans/board-speed.md (remeasured every few weeks while drawing and the VM change).
     Done (2026-11-20), for the user to try: the desktop's window and the web count each instruction and
     call, skip the frames the board would still be busy for, and pc.time reads the board's clock;
     `rinc -turbo` turns it off, `rinc -board -frames` has it headless (tests). `uv run
     tools/boardspeed.py COM3 build-desktop-rio/Release/rinc.exe` remeasures (2 minutes) and rewrites
     src/core/boardcost.h. Within 10% on the speed app's 100 timings, about 15% on Paint (draw 5.9 ms
     guessed, 6.8 ms on the board) and falling sand. Left: a frame-time readout on screen; tests that fail
     on a slow frame; a setting for turbo (the web has none yet); app starts (compiling) not counted.
     Native code too: the JIT's check of which loops and procs qualify runs on every Rinc (only the
     Thumb-2 output is the board's), so a loop the board runs natively is charged the native rate.
     Related, can wait but before Rio 1.0 (the user, 2026-11-20): only loops marked (`# jit` or such)
     compiled, and an error saying why when a marked one can't be (the same check's reasons).
  3. pc.frame() (decided: "Clocks alike everywhere", under 4 below): the frames since the app started,
     the same on every Rinc; the web test player stepping a frame at a time.
  4. Cloudlamp as a walkthrough (4 below), its setup made of studio's snippets as they come.
     Cloudlamp's dialog font (the user, 2026-11-21): the text at the bottom in a font of its own, smaller
     than the built-in one and proportional, made as a person would, on the board in studio's apps
     (Paint's font sheet, then the game's code changed in Edit), not on the host; the "Cloudlamp" title
     at the top taken out, as the dialog is the better font exercise.
  5. Board tools (5 below): the app's log over serial while it runs, a frame-time readout (the firmware
     console's `frame`, 2026-11-20: update's and draw's time, slow frames, gaps; on screen, still to do).
  6. Studio from Cloudlamp: state per file (6 below); Maps' paste anchor, Ctrl+T for every layer
     (studio's ROADMAP).
  7. To get on the list (the user, 2026-11-15), guesses to check first:
     - USB thumb drives: files, apps and projects in and out with no serial and no card taken out. The
       keyboard is on the Fruit Jam's USB host already (PIO-USB, TinyUSB), and TinyUSB has a mass
       storage host, with FatFS (the card's) reading the drive: a few KB of RAM (a sector buffer, a
       volume), full speed (12 Mbit/s, about 1 MB/s). Files would show the drive beside /rinc.
     - Rotated, scaled and skewed drawing (SNES's mode 7: a whole layer turned and stretched a line at a
       time, and sprites turned), and textured triangles for simple 3D: the RP2350's interpolators
       (two a core) are made for exactly that lookup (u, v stepped along a line). Little RAM (it draws
       into the screen there is); the question is time a frame, and how pc says it (a transform on
       blit, a triangle call).
     - RAM outside app space (measured 2026-11-15, build-rio's rinc.elf): about 2.5 KB of the 512 KB
       free (app space 256 KB, the screen and the frame shown 75 KB each, .data 36 KB, mostly code in
       RAM for speed, PIO-USB's buffers 6 KB, the big stack 6 KB, the rest of .bss about 25 KB).
       Transformed drawing fits (code in flash, the interpolators' state in hardware, only stack); a
       thumb drive is borderline (a volume, a sector buffer, the class's buffers: 1.5 to 3 KB), so free
       a few KB first, keeping the 256 KB app space and the 16 voices (2.4 KB all told, so cutting
       them would buy little). .data is code kept in RAM (docs/guide/fruit-jam.md, Memory): the
       interpreter and sound's mixing stay (they run every frame). Candidates: the log's and the serial
       ring's 4 KB each, halved (if serial keeps up); malloc's 1 KB arena, if nothing calls malloc; the
       command list's 3.5 KB overlay, shared with something not used at once; code that rarely runs
       moved back to flash; then the PSRAM (8 MB, unused) for what isn't speed-critical; the second
       screen buffer last.
     - MIDI keyboards over USB (the user, 2026-11-15: of most interest): playing Sound's and Music's
       notes from a keyboard plugged in beside the computer keyboard (TinyUSB's MIDI host; under 1 KB).
     - A debugger (the user, 2026-11-15: of most interest): breakpoints, stepping, a look at variables,
       in the editor or the console (a few KB).
     - CJK, sometime (the user, 2026-11-15): fonts big enough that they'd live in flash, or be read
       from the card.
     - Fonts from more than one sheet (the user, 2026-11-16): a font made of several of Paint's sheets,
       each for a range of its own (Latin, accents, symbols, kana...), so a font isn't one picture.
     - Rinc's own font 3 pixels taller (the user, 2026-11-16, considering): 5x11 glyphs in 6x12 cells,
       room for Latin's accents and less padding around text everywhere, and closer to what CJK will
       want; the editor then dropping its top or bottom bar to keep 19 lines. Paint's fonts first, to
       explore Latin (32 to 255).
     - Paint unresponsive on a font of 32 to 255 (the user, 2026-11-16, rinc12extra.png, 5x11): fixed
       (studio): the line of the font's drawn letters read every cell of its 224 and made the font again
       each frame, too slow on the board to keep up with keys; now redone only after input. No test yet
       (the desktop is fast enough to hide it): a frame-time readout (board tools, 4 above) would catch
       it. Still open: the file's notes say 16 across, but Paint saved its picture 40 wide (8 cells of
       5: "saves as" trims to what's drawn), so a font read from it has its characters in the wrong
       cells. Fixed (2026-11-16): a font's saved at least its across's cells wide and its characters'
       rows high (studio/tests/paint.py); rinc12extra.png re-saved on the board: 80 by 154.
     - A font's layout (the user, 2026-11-16, then 2026-11-20): rather than fonts 32 across, Paint's
       limit a number of pixels, not a width and a height (at least 320 by 240's), so a font of 224
       characters stays 16 across, a row a hex digit (0x20, 0x30...), and grows taller as it needs. Done meanwhile: - and = step a font's
       cells in its characters' order, and the status line gives a character's code point in hex, as
       Unicode tables do (U+00A2).
     - Sheets read in place (the user, 2026-11-20, considering): a font, or Paint's brushes across
       sheets, read from the card or flash where they are, a glyph or a sprite at a time, without the
       whole sheet in RAM, and one font format everywhere. Recommended: still PNG, but saved
       uncompressed: zlib's stored blocks (5 bytes of header each), a row a block, 8-bit palette pixels,
       filter 0 (as Rinc writes now), so pixel (x, y) is at a sum (start + y * (6 + w) + 6 + x) and any
       program still opens it. Reading in place is free; a pixel written in place means Adler-32 and the
       chunk's CRC done again, so saves stay whole-file writes. Others' PNGs (compressed) are read as
       now, until saved again in Paint. Rather than a format of Rinc's own (tried, and back to PNG). Planned (2026-11-20): plans/sheets.md: sheets
       at 1, 2, 4 or 8 bits with their own palette and a color map on blit (palette swaps; a 1-bit font
       about 1.5 KB, not 12), saved uncompressed, drawn from storage through an app's cache or from flash.
       Next after the current work, before thumb drives (the user, 2026-11-20).
     - The editor's completion and error reporting, exactly where they belong, may be broken again (the
       user, 2026-11-16): to check.
     - Pan in songs and sounds (the user, 2026-11-16; studio's ROADMAP, Pan): the C song player taking a
       pan for each sound placed and each use, the files carrying it.
     - Parallel tracks in songs (the user, 2026-11-20; studio's ROADMAP): patterns side by side (a drum
       track kept going), voices shifted to free ones and the last over dropped; with pan's file change.
     - More than one controller (the user, 2026-11-16): two or more players, each controller its own
       pc.btn (a player number).
     - Mouse and pointer measured (2026-11-16): about 90 bytes of RAM and 1.5 KB of flash; the USB HID
       code that reads a mouse is the keyboard's too. So dropping them frees nearly nothing: mouseless
       would be a design choice, not a memory one. If chosen, desktop and web users moving the mouse
       over the screen would get a note in the window or page (not Rinc's drawing): "Rinc uses the
       keyboard". Not decided (the user: worth considering).
  8. Smaller, any time: a picker for the clipboard's kept ones (Sys+V, say); the volume shown as it
     changes; one snippet for a whole project.
     Home (the user, 2026-11-22): two apps with the same title are hard to tell apart; each such one shows
     its key (its folder's name, chord or chord+) at the start of its description, say.
     Caps Lock on the board (the user asked, 2026-11-21): the desktop and the web type capitals with it
     (the system's text, SDL's and the browser's), the board ignores it (keyboard.c: Shift alone picks
     the letter's case). Recommended: the board keeps it as a toggle and its light (the keyboard's LED
     report), letters only, as the others do; about 15 lines. Shift tapped twice, Rinc's own lock,
     stays. Done (2026-11-21): rinc_hidShifted (tests/capslock.c), the light set on a press and on a
     keyboard plugged in; to try with a real keyboard (no tool here types as a USB keyboard).
     Also (the user, 2026-11-20): Paint feels sluggish drawing pixels and moving round the sprite grid.
     Measured on the board (the firmware console's new `frame`): Paint's draw is 3 to 6 ms (9 at most,
     32x32 sprites), well inside a frame, so Rinc's C drawing wasn't it. It was Paint: on a font sheet,
     its line of the font's letters was read and measured again each frame anything was held (48 ms a
     frame: 209 of 1,194 frames slow with an arrow held; now 4 in 2,243, held 20 s), and its stash (the
     whole sheet, 0.1 to 0.2 s on the card) was written every 3 seconds even while the arrows were held.
     Done: the line redone only when a pixel or a key (not an arrow) may have changed it, and after a
     stroke, not during it, and then just the glyph drawn in looked at again (not all 224); the stash 3
     seconds after the last input. The 0.3 s gap first seen was the measure's own (counted across an
     app's start); held 12 s now, frames at most 63 ms apart. Done (2026-11-20).
     The editor's full lines (a line of 12, the font whole) as a setting, compact the default: its 9s made a line height it keeps (keys.rio, screen.rio); the console app the same, later.
     Also (the user, 2026-11-20): Ctrl+Tab in every studio app as in the editor (the files had lately,
     walked while Ctrl's held), from one library (the editor's files.rio recent list and walk, moved
     out), with Paint's step 4 (plans/sheets.md). A gradients view in Paint, later.
     With it (the user, 2026-11-23): a command to drop a file from the recent list, Close (Ctrl+W, as in
     browsers and most editors; in the command list too): the file shown leaves the list and the next one
     comes up (unsaved changes asked about first), without leaving the app. For files looked at a while
     and done with, a snippet opened from Snippet, say. In the editor first, then the shared library.
     The user (2026-11-23): Ctrl+W may be OK, but one finger slipping shouldn't do something else (W is
     next to Q, and Sys+Q quits the app). Recommended: Ctrl+W, since it only drops the file from the
     list (it's still on the card, Ctrl+Tab's list or Ctrl+O brings it back, and unsaved changes are
     asked about), so a slip costs little either way. Not decided.
     Done (2026-11-23), the {column, row} way (the user's pick): see below. Was: a map's tile numbers are a sheet's sprites counted across its rows, so a sheet saved
     wider (Paint saves as wide as what's drawn) renumbers them. Maps notes each sheet's across and
     renumbers when it opens a map (tilemap's refit), but a game's tilemap.load doesn't, so the game
     shows the wrong tiles until the map is opened in Maps and saved; and Rio code naming sprites by
     number (pc.spr) has nothing to renumber it. Recommended: a sheet's across is set when it's made
     (16 by default, as fonts are: a sprite's number then reads row * 16 + column) and kept in its
     notes; Paint saves it that wide, empty columns too, and it grows only down, so numbers never move.
     Changing across is a command of its own that says it renumbers; readSheets refitting too, as a
     fallback for sheets made elsewhere. The other way (the user, 2026-11-20): sprites always named by
     2D place, {column, row}, in code and maps, so any width works and nothing's noted. Recommended
     for maps: a cell as two bytes, column and row (sheets up to 256x256 sprites), written as four hex
     digits (`0301`, `----` empty) in `rinc map 4`, still text; never renumbered, on save or anywhere.
     Done (2026-11-23): `rinc map 4` (cells and things' sprites as 4 hex digits, column then row; no
     across on sheet lines); maps 3 and 2 read and made {column, row} with their across, written as 4.
     tilemap: at() -> [2]Int ({-1, -1}: none), filled(), set(layer, p, {c, r}), clear(); Thing.sprite
     [2]Int; for editors cell()/setCell() packed (column + row * 256, NO_TILE 65535) and
     packed()/unpacked(); flagsAt by {column, row} in the sheet as it is now; refit, renumber, across
     and EMPTY gone; start() and sheetFor() without across. Its work 4224 bytes (a 1024-cell row, 4 KB,
     fits). Maps keeps tiles packed (the fill's mark: 65534, so {254, 255} can't be placed); its undo
     log version 2 and state 5 (older ones dropped, not misread). Cloudlamp: filled(), its visitor
     {4, 0}, its map converted. Tested: tests/tilemap.py (map 3 across 3 read as {column, row}, map 4
     round trip, packed limits, flags by place, a 1024 row), studio's maps.py (125, a sheet grown or
     shrunk keeping tiles, a map of before), snippet.py; the board (Cloudlamp from its map 3; Maps on
     the user's raceworld map).
  9. Later: the messages library (dialog, visual novels: under 4 below); drawing speed (8 below);
     sizes on disk (9b below).

- Priorities (2026-11-14), in order (open now; the 2026-11-13 list below has the history, its numbers
  as they were):
  1. A print that wraps: done (2026-11-15): `pc.print(s, pos, c, how?)` with a `pc.Text` (its width:
     lines broken at spaces, 9 pixels apart for now) and `pc.textSize`; Cloudlamp's hand wrapping
     goes. Then fonts (the user, 2026-11-14), in steps: font sheets (a grid of any cell size; a glyph's
     width found by trimming its empty columns, a gap after each, the space's width in the notes; one
     color, given when drawn, unless the notes say colors), pc.fontRead and `font` in pc.Text, lines
     spaced by the font's size; Paint's font mode (each cell's character shown, a preview line typed in
     it), and a quick zoom in and out key in Paint; later, more than one color remapped (multicolor
     fonts), and sprites handled as flexibly.
     Done (2026-11-14): pc.Text (width, line, font) and textSize; fonts from sheets (pc.fontRead, fontOf,
     fontNotes: docs/spec/screen.md#fonts); Paint's notes kept (a font's, another program's), any sprite
     size (Set the sprites' size: 6x9), Make it a font, the picked cell's character, and a line of the
     font's drawn letters in it; Cloudlamp's title in a font drawn in Paint on the board, over serial.
     Done (2026-11-15), the rest: Paint's M (the sheet twice, four times as big, following the picked
     sprite), its "Set the font's characters" (32 126, typed), and each cell's character over the
     sheet (with the grid, where a cell has room); a gap for fixed fonts too, negative closer, and a
     loaded font's gap changed before drawing. Met on the way: the firmware's console (Ctrl-B) didn't
     let the app keep its state (no leaving*): fixed; Paint's files.txt lacked four of its parts (an
     install from its folder, or a release, couldn't start it): fixed, and studio/tests/filelists.py
     checks every app's.
  2. Tilemap's last rough edges from Cloudlamp: a thing's name a plain String (no `.asString()`); a
     map's sheets and layers its own, once Rio can keep a view of a struct's own array (Rio's ROADMAP:
     a static lifetime marker), so a game gives just its room; a song's two buffers one.
     - Static views tried (2026-11-14, Rio d2f25c8), where they help and where not:
       - Done: a song's one buffer (`song.Song{buffer = ...}`, a `static Blob` field: `load` puts the
         sounds' notes first, as many as the song plays, then its room; Music shows the one Blob a
         game gives). Cloudlamp's `say` keeps its literal (`s: static String`, no copy), and Music's
         too (its messages are all literals).
       - Not taken: a thing's name as a view. Maps keeps Things across edits (undo, the clipboard)
         with names from buffers it rewrites, so a view would change under them: static is about how
         long memory lasts, not what's in it. The copy stays (`.asString()` stays).
       - Not taken: Paint's, Sound's and Maps' `say`. Each builds some messages in its buffer, which
         stays, so static only moves the copy, no simpler.
       - Done (2026-11-15), another way: the library's own sheets and layers (4 and 8, about 930
         bytes once in an app, not in each Map: a Map inside a local, roomIn's, stays small), for a
         map given none, one map at a time. With notice (a few words shown a while, counted as drawn)
         and song's loop(path), Cloudlamp went from 106 lines (87 of code) to 85 (67): its two
         screens one wide map in Maps (a screen at a time by its camera), made on the board.
  3. Copy as Rio, in studio's apps (Paint: an enum of its flags' names and the calls that read them;
     Maps', Music's likewise), after a clipboard apps share (text at least: below, 3 of 2026-11-13).
     The order (the user, 2026-11-15): Cloudlamp made as simple as generalizes first, so what's copied
     is what simple looks like. The clipboard (the user, 2026-11-15): one for every app, in Rinc's own
     storage (/rinc/stash/rinc/clipboard, say: not /rinc/clipboard, the editor's, nor each app's own
     stash "clip", as Maps' and Music's are now), with flavors (text; Rio, a kind of text; an app's own,
     sprites or uses, beside text for the rest), and the last few kept, the newest pasted; a fancier
     clipboard view later (the recent ones, picked from).
     The clipboard: done (2026-11-15): rinc's clip library (library/clip.rio, docs/spec/apps.md#the-
     clipboard), in /rinc/stash/rinc/clip, the last 8 kept: kinds text, line, and an app's own (paint,
     maps, music, files: Files' paths beside as text), with text beside for the rest. The editor, Files,
     Paint (its copy in memory before: lost as Paint closed), Maps and Music (each its own stash before)
     all on it; tested in each, and on the board (one app's copy pasted in the editor). Next: copy as Rio
     (an app's own kind, its Rio beside as text), and a list of the kept ones to pick from (Sys+V, say).
     Copy as Rio: done (2026-11-15), as an explicit command, not beside each copy (the user: what's
     copied should be seen first): studio's Snippet app shows the code to load a map, sheet, font, song or
     sounds in a box (library/codebox.rio), sized as the file needs, Enter copying it as text; each
     studio app's command list calls it (docs/spec/apps.md#the-code-to-load-a-file; studio's roadmap).
     Then (2026-11-23) the box went: Snippet writes its code to a file in its stash and opens it in Edit
     (copied with Edit's own selection), a map's layers always an enum; library/codebox.rio and its test gone.
     The clipboard's line kind went too: whole lines are text, the editor keeping their number
     (clip.number()) to paste them above a line while they're the newest.
     Found on the way: the command list took 56 of an app's commands and left the rest out without a
     word, so Maps' last 14 (Undo, Redo, Save, Save as, Open, New...) were never in its list. Now Rinc
     reads them in place, in the app's memory (no copy: 1.1 KB less), up to 240, and the log says when
     there are more (tests/commands.py: 150 commands, the last picked by name).
     Also: a growable view of part of a Blob works (the editor's paste: got: Blob[..] = formatted[0..<n]),
     so tilemap's room and pixels could be one Blob after all, as Song's are: worth a look.
  - The dev folder: done (2026-11-15): an app is all of its folder but dev/, its maker's (a project's
    own saves, dev/save, moved in from save/ as it first runs; its tests, dev/test.rio, run as
    game+/dev/test); installing, copying as a project, bundles and exports leave it out. files.txt is
    gone (each listed its whole folder; Paint's had fallen behind), and tools/install.py and send.py
    take folders as the desktop does (tests/devfolder.py).
  4. Cloudlamp as a walkthrough in the docs (8 below), now its code's settled.
     - With it (the user, 2026-11-15: for later): Cloudlamp's setup made of studio's snippets as they
       come (Maps', Music's, Paint's "Copy a code snippet": docs/spec/apps.md#the-code-to-load-a-file),
       so a tutorial can say "paste the snippet" and the game reads the same. That means the snippets'
       names (level, GROUND, FOLK, SOLID: 12 uses in its body renamed, on the board, in the editor) and
       their sizes, exact.
     - To reconsider then (the user, 2026-11-15): a snippet's sizes are just what the file needs now
       (the map's room 1247; Cloudlamp gives 2000, with room to spare). Room to spare lets a map grow a
       while without the code changing, but not forever: a map grown past it fails to load and the log
       says how much it needs, so a new snippet's copied. Maybe a snippet rounds up, or says what the
       room is for, or a game's room grows on its own (a Blob[..] with a most, say).
  - Recommended (2026-11-15), to place in the list (the user decides):
    - Clocks alike everywhere: pc.time() is the board's real time since it started, the desktop's
      window's real time, the desktop's headless runs' frames / 60 (tests, -frames), and the web's the
      browser's clock, so a game timing itself by pc.time() can differ between a test and the real
      thing (notice's first version did: the exported page, in node, ran faster than its frames).
      Decided (the user, 2026-11-15): pc.frame(), the frames since the app started, the same on every
      Rinc, for a game's timing (as notice counts its frames); pc.time() documented as wall time, for
      showing time; and the web test player (tests/webplay.mjs) stepping its clock a frame at a time, as
      headless runs do.
    - Messages, grown from notice (the user, 2026-11-15: later): a library for dialog and visual
      novels as well as a status line: text revealed a letter at a time (its speed, a sound a letter),
      styled within a line (colors, a font, a word held), a speaker's name and portrait, pages, a key to
      go on or see all, and choices; notice's say and draw as its simplest case.
  5. Tools for working on the board: an app's log over serial as it runs, without stopping it (a
     screenshot stops it today); a frame-time readout (is a game keeping 60 frames a second?).
     tools/board*.py are the start: put, get, paste an app, play one and take screenshots.
     Met editing Cloudlamp in the editor over serial (2026-11-14), all fixed: the list left open over
     the next app (Ctrl-C over serial), Print Screen while it's up, its narrowing a word at a time
     ("delete line" finds "Delete the lines"); kitty's Ctrl+Shift+letter (ESC [107;6u) read as xterm's
     is; and an editor's draft lost through the firmware's console (Ctrl-B gave no leaving*: the editor
     keeps a draft, and says how to drop it, when it's left).
  6. Studio: state kept per file, not per app (5 below).
  7. Songs played by the host (9c below): done (2026-11-15).
  8. Drawing speed (10 below): fast paths for scaled and mirrored sprites, less per call; the half and
     quarter screens take much of the need away for games drawn big.
  9. Sizes: on disk (9b below), Maps' memory (9 below); more fonts, maybe a font mode in Paint (9d).
  10. To decide later: the console back to just Rio typed in (5 below); library versions on the card
     (6 below).
  - Recommended from Cloudlamp's song (2026-11-14, made in Sound and Music on the board; the user
    decides where they go; studio's ROADMAP, "From Cloudlamp's song", has studio's side):
    - High, after 2: `song`'s room explodes when sounds' speeds don't divide a pattern's step (each
      channel's notes written at their gcd: 20,951 bytes for a 4-pattern song at speeds 14 and 100);
      render each placement at its own speed.
    - Medium, with 6: an installed app's files and its project's split: Sound and Music changed
      `/rinc/apps/cloudlamp` in place, while the editor works in `/rinc/projects/cloudlamp` (apps read
      only), which kept the old song. One rule: changes go to the project, installing carries them.
    - Medium: Files: a replace choice when pasting onto a taken name; narrowing the list picks the
      first match (it keeps the row's number now); the picker taking a typed path, as Save does.
    - Low: desktop `-type` keys typed while an app hands back to its caller (Files to Music) land in
      the wrong place; waiting for the switch, as `\w` waits for what's typed.
  - 16 voices, measured (2026-11-15; the user: people would appreciate them, if they fit): a voice's
    sound and synth parts now share their room (244 bytes to 152: 8 voices leave 2,820 bytes of SRAM
    free, 16 leave 1,460, with room for 120 host procs; a build tries 16 with `-Drinc_VOICES=16`). Mixing runs on core 1, with the
    HDMI, so it costs the app nothing; the firmware console's `audio` says how much of core 1's time
    it takes, and apps can ask (`pc.audioLoad()`: apps/audiobench.rio plays N voices at a rate and shows
    it, `pc.call("audiobench", "16 48000")`, no serial needed). At
    most, of a block's time: none playing 2%; 8 synth voices 21% at the synth's starting rate (16,000)
    and 50% at its top, smoothed (48,000); 16 voices 42% and 98%. Recommended: 16 voices, with the top
    rate's cost in the sound spec (16 at 48,000 smoothed is too much: sound would break up), or a
    cheaper synth step first (smoothing and the phaser are the costly parts). To hear on the board
    before deciding: 16 at the top rate.
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
     with the sheet, read by tilemap and games. Built (2026-11-14, after a .sheet format of Rinc's own
     the day before): flags in the sheet's PNG, 8 a sprite in a chunk of Rinc's own (rnFg), named in its
     notes, read by host calls (pc.imageFlags, imageSpriteFlags, imageFlag: docs/spec/apps.md#sprite-flags),
     so a game needs no library (8 KB more free than the sheet library left); Paint's flags by each
     sprite's top left pixel (stranded ones counted, snapped or taken out): studio/plans/sprite-flags.md.
     More than 8, if it matters later: a wider record. Then (2026-11-14): tilemap's readFlags and flagsAt
     (a cell's tile's flags), and Cloudlamp's walls from its sheet's "solid" (tests/cloudlampwalk.py).
     Later (the user, 2026-11-14): a command in studio's apps that shows Rio source to copy into a game,
     several lines (Paint's: an enum of its flags' names, with the calls that read them; Maps', Music's
     likewise). It needs a clipboard shared by apps first (the user, 2026-11-14): text at
     least, so what one app copies another (the editor) pastes; pc has none yet, each app's is its own.
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
     - Named and fast: names in the notes (flag 0 solid), the game asks once (`solid := sheet.flag("solid")`,
       a bit) and then tests a byte (`sheet.has(t, solid)`): no strings in the loop.
  4. An ergonomics pass: making a game as quick to write as on PICO-8, keeping Rinc general. Begun
     (2026-11-14), each a general win, no game library: pc.screen (half, quarter), named buttons with
     btnp, dpad and dpadp; a map that reads its own sheets and flags (pixels, readSheets, flag, flagsAt,
     sprite, draw with no sheet, thingIn); song's loop; Rio's `if name := value`. Cloudlamp, rewritten on
     the board over serial with them: 130 lines of code to 96, at half the screen's size. Met on the way,
     still to do: a String given a proc can't be kept (Cloudlamp's say copies it into a Blob; Rio's
     ROADMAP); a map's sheets and layers still the app's arrays to give (a view of a struct's own array
     can't be kept in it); a thing's name a Blob (t.name.asString()); text wrapped by hand at the half
     screen's 26 letters (a print that wraps, or 9d's small font). Ideas still to weigh:
     weigh: a `game` library of the usual (a sheet and a map loaded by name, sprite and map drawn in a
     call, btn and btnp, sounds and a song played by number), so a small game is PICO-8-short; the
     templates new projects start from; and Rio's own rough edges met in Cloudlamp (one-pass order,
     views returned from procs, logging inside an expression). Style (the user, 2026-11-13): short
     names for modules, which Rio has already (`import tilemap as tm`, `import game as g`), in the
     docs, examples and templates: a module's name repeats at every use, so short there cuts noise,
     though other names stay long and readable.
  5. Studio: state kept per file, not per app.
     To decide later (the user, 2026-11-14): maybe the console back to just Rio typed in (its load,
     save and the like dropped). Its load now reads an app's parts, modules and files from its folder
     (pc.rioStart's app), for small apps; bigger ones (tilemap's, song's) are run, not loaded.
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
     binds now is the compiler's instruction table, 84% full (12,560 of 14,945).
     Now (2026-11-15): 31.9 KB free, instructions 92% (13,758 of 14,868: the shared clipboard took
     about 300, Snippet's command 30) and procs 92% (277 of 300), while the compiler's slots are 17%
     used. First (the user, 2026-11-15): Rio's compiler tables sharing their room (Rio's ROADMAP, Next),
     which frees both; then Maps trimmed only if it still needs it (its clipboard code first). Before, the plan was
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
  9c. Songs played by the host: done (2026-11-15, plans/song-player.md): a sequencer in the mixer
     (audio.c), a tick at a time (1/120 s), each placement started at its step at its own speed from
     the app's tables and notes (pc.songPlay, songStop, songPause, songLevers, songAt: sound.md#songs),
     one song at a time. A song's room is its tables now (Cloudlamp's: 841 bytes, from 3,694; a song
     that took 6,703, 53), placements start on their tick, and a game calls nothing each frame (tick
     only tells where it's playing). On the board, 148 bytes of SRAM; 1,092 left with 16 voices.
  9d. More fonts (the user, 2026-11-14: later), maybe a font mode in Paint; a small one for the half and
     quarter screens (until then, text there is drawn big, as the screen is).
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
- The system volume: done (2026-11-15): a keyboard's volume keys (up, down, mute) set it on the board,
  and it's kept in the settings (`/rinc/settings/volume`), so it's the same after a restart (written a
  second after it last changed). Next, in order (the user, 2026-11-15):
  1. Shown while it changes: a bar over whatever's on the screen, for a second or so after the last
     step (the level of 16, or muted), drawn by Rinc, not the app, as the command list is.
  2. Keys for it on any keyboard, not only ones with volume keys, and a controller's. Not Sys with +
     and - (those would suit a text zoom, if one comes). To decide: Sys+V then the arrows, the bar
     showing, until a key or a second's pause; Sys+M to mute (as Sys+L sleeps); a controller's Start
     held with up and down; or only the settings app's slider, keys left to keyboards that have them.
  3. A settings app's slider (with the theme and the other settings), and `pc.settingRead("volume")`
     for an app that wants to show it.
  4. On the desktop and the web: their own volume (the window's, the page's), or the same keys.
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

- Other RP2350 boards (the user, 2026-11-23: cheaper Rincs, and none with a radio). Rinc needs only the
  RP2350's 520 KB (no PSRAM), DVI on HSTX, an SD card, a USB host and sound (HDMI's own works with no
  codec: hdmi_packet.c), so a port is mostly pins: a board setting in the build, its pins under it.
  - Waveshare RP2350-PiZero (about $10): RP2350B, DVI on mini HDMI, microSD (SPI or SDIO), one PIO-USB
    host port (a hub for a keyboard and a gamepad), USB-C, a LiPo header, no radio, no audio codec (sound
    over HDMI, if its DVI is on HSTX's GPIO 12-19). To check in its schematic first: the pins, and whether
    DVI and PIO-USB share any (a summary of its page said they can't be used at once: likely its demo's).
  - Olimex RP2350pc (about EUR 25): RP2350B, HDMI/DVI, 4 USB ports, microSD, an ES8311 codec with
    headphone and line-in jacks, no radio: the closest to the Fruit Jam without Wi-Fi (a codec driver new).
  - Adafruit Metro RP2350 ($25; $28 with PSRAM): an HSTX connector (a DVI breakout), microSD, USB host
    pins: more parts to add.
  - Pimoroni Pico DV Demo Base with a Pico 2 (or Pico Plus 2): HDMI, SD, I2S audio; two boards.

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

- Ideas that take RAM (brainstormed 2026-11-15; the user's picks are in Next: MIDI keyboards, a
  debugger, CJK sometime). Guesses: the PSRAM (8 MB, unused) is where most would go.
  - Apps switched instantly, each kept whole (256 KB each: PSRAM; Later, below).
  - An echo or reverb on the sound, as the SNES had (8 to 32 KB of delay).
  - Controllers (2026-11-22, Claude's thoughts; the user has their own, not shared yet): not a move from
    Xbox to SNES pads but the SNES layout as what every app can count on (d-pad, A B X Y, L R, Start,
    Select: Rinc's buttons already), sticks and triggers extras (pc.stick, pc.trigger 0 without them),
    Xbox pads a superset. To check: cheap USB SNES-style pads (plain HID gamepads) on the board's
    gamepad.c, which knows Xbox 360 pads.
    The user (2026-11-22): the keyboard alone must keep everything usable, so controllers get nothing
    that doesn't map well to keys; four action buttons (SNES-like) is fair for a game to expect; local
    multiplayer up to 4 controllers wanted (the user will get some to test).
  - Recorded sounds: sampled instruments, voices (the app's, or PSRAM).
  - Drawing a scanline at a time from tile and sprite layers, as old consoles did: this one gives RAM
    back (no second 75 KB screen buffer), paying for others.
  - A bigger screen (640 by 480, 256 colors: 300 KB a buffer): PSRAM, if HDMI can read it fast enough
    (doubtful).
  - Local multiplayer or sharing over a USB Wi-Fi stick (radios optional, local only): 20 to 40 KB of
    network stack.
  - A GIF of play recorded, or the last 10 seconds rewound (100s of KB: PSRAM).
  - Longer console and log scrollback (PSRAM).

- An input method for Chinese and Japanese.
- Right-to-left text: drawing first, editing much later if ever.
- USB speakers.
- Other boards: the PicoCalc, and someday a keyboard computer of Rinc's own.
- A VS Code extension, a language server, a web playground.
- A speed throttle matching the board.

## Known bugs

- Fixed 2026-11-24 (found making the shapes demo): what's held as an app starts (the Enter or A that
  started it from home) was the app's: a press on its first frame. Now the buttons and keys held as an app
  starts aren't its till each is let go (btn, btnp, keyHeld); others work at once. (The A an app run
  with -app without a window sees for 3 frames is the desktop's own, to start games in tests: as before.)
  Tests: tests/startkeys.py (an app started by another while A is held, as home does).

- Ctrl-B after the console app runs an app hangs the board's USB serial.
- Desktop tests now and then fail under heavy parallel runs on Windows (`ctest -j 12`): `stash_first`
  twice, `projects` once, each passing alone and in reruns. Maybe a file still held just after rinc
  exits, so a test's clean start (`rm -rf` of its data folder) misses it.
- Fixed, not yet checked on the board or the web: Files' prompt and "New folder here", fullscreen mouse
  confinement, the web's Keyboard Lock.
