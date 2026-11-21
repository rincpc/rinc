# Board speed everywhere: what to measure

ROADMAP Next 2. The desktop and the web charge each frame what it would cost on the Fruit Jam and run
no faster than it would. This is the list of costs the model needs, measured on the board by one bench
app and a tool, and re-measured every few weeks while Rio and the drawing code still change.

## The model, kept small (the web build too)

- One table of costs in board microseconds (or tenths of them), about 60 numbers, generated into
  `src/core/boardcost.h` from the board's measurements. It's the only part that changes when
  remeasured.
- Rio: a cost for each group of instructions (below, not each of the 173 ops). Each op maps to its
  group through a byte table, and the interpreter adds that group's cost as it runs: one add an
  instruction, so desktop is barely slower. A loop or proc the board would run natively is charged at
  its group's native cost.
- C calls (drawing, files, sound): a fixed cost plus a cost per pixel, byte or glyph, added where the call
  is made.
- Counted, not slowed (recommended, the user to decide): nothing is skipped and no op is made slower.
  A frame's costs add up on a board clock; at the frame's end it takes as many 60ths of a second as
  the board would need (over one: it drops to 30, as the board's waits for the next), and the desktop
  waits that long in real time. pc.time() reads the board clock, so an app timing itself sees board
  numbers. Tests and the web player's stepping run on the board clock without waiting (fast, and the
  same every run); turbo doesn't wait at all.
- The JIT's check of what qualifies, moved out of the Thumb-2-only part of jit.c so every Rinc runs it:
  a few KB of code.

## Rio's instructions (interpreted, and with the JIT)

Each measured as a loop of many, minus the empty loop's cost:

1. Moves and integer math (mov, add, sub, and, or, shifts, compares; the short forms).
2. Multiply; divide and modulo.
3. Float math (fadd, fmul; fdiv; itof, ftoi; sqrt, floor).
4. Branches and loops (jmp, the conditional jumps, fori, each), a loop's step on its own.
5. Loads and stores: a slot, a global, an array element (with its bounds check), a struct field,
   2-wide (vectors).
6. Vector math ([2]Int, [2]Float).
7. Calls: a Rio proc (call, ret, and per argument), through a proc value (calli), a host proc's own
   overhead (ffi, apart from its work).
8. Lists: push and pop, remove.
9. Strings and blobs: compare, slice, copy (fixed plus per byte), format to text.
10. log (only for its cost: the board's log goes to serial).
11. Native: groups 1 to 6 with the JIT, and a native loop calling C (rio_jitOp: log and most math).

## Drawing (each a fixed cost and a cost per pixel)

1. cls; the frame's own overhead (input, showing the frame).
2. rect, filled and outlined, small and large.
3. pset and pget, a call each.
4. blit: solid, with see-through pixels, scaled, mirrored or turned, clipped at the screen's edge, by
   size (8x8, 16x16, 32x32, a whole screen).
5. print: a glyph of the built-in font, of a sheet's font (pc.fontOf), proportional; textSize.
6. scroll; the half and quarter screens, if they cost differently.
7. imageLoad (PNG decoded, per pixel) and imageWrite (encoded).

## Files (a fixed cost for each file opened, plus per KB)

1. Reading and writing: fileRead/Write, At, saves, stashes, settings, the cache.
2. Directory listings, per entry; fileCopy, rename, delete.
3. audioLoad (WAV and ADPCM).

## Sound

Mixing runs on the second core, so an app pays only for its calls: sound, synth and song calls, a fixed
cost each. One check that heavy mixing (16 voices) doesn't slow the first core (memory shared).

## Apps

1. Starting an app: compiling its source (per KB), loading a compiled one, the JIT's compiling.
2. pc.call and returning (Ctrl+Tab between apps leans on it).
3. Input calls (btn, key, pointer, stick), time: a small fixed cost each, one number for all.

## How it's measured

- `apps/speed.rio` (bench.rio and drawbench.rio grown together) times each item above and logs one
  line each: `name fixed perUnit`.
- `tools/boardspeed.py COM3` runs it on the board over serial, writes `src/core/boardcost.h`, and
  shows what changed since the last run, so a faster blit shows in the diff (and in git).
- Checked against the real thing: the same bench run with board speed on the desktop, each item
  within about 25% of the board's; and apps (Paint, Maps, a game) through the firmware's `frame`.
- docs/guide/fruit-jam.md's Speed tables come from the same run.
- Remeasured every few weeks, and after a change to the VM, the JIT or drawing.

## Done (2026-11-20)

- Rio (built with `rio_METER`, the desktop's and the web's only): each instruction counted by op, in
  one place in the dispatch (the web player grew 10 KB: 215 to 225 KB); `rio_jitPlan` makes the JIT's
  choice of loops and procs on any machine (its check moved out of the Thumb-2-only part of jit.c,
  shared), so code the board compiles is counted apart.
- Rinc: `rinc_charge` on the drawing and file calls (rect and its rows, blit by path and pixels, scaled
  blits by their drawn blocks, print and textSize by bytes, files by bytes, images by pixels);
  boardcost.h the costs; a frame's end skips the frames the board would still be busy for; pc.time the
  board's clock. `rinc -turbo` off, `-board` for headless runs, `-nojit`, `-meter`.
- The firmware console's `jit off` / `jit on`, and `frame` counted from the app's start.
- The platforms' clock is a double now: a float of seconds since the board started had lost the
  microseconds within the hour.
- `tools/boardspeed.py COM3 rinc.exe`: the board restarted, apps/speed.rio run there with the JIT and
  without, the desktop's counts for each test, a non-negative least-squares fit, boardcost.h written,
  each test's error shown (`--check`: only that). About 2 minutes. `tools/boardframe.py` breaks a
  frame down (`rinc -board -meter`).

How close (the speed app's 50 tests, with and without the JIT, fitted): within 10%, but small rects
(17%) and SD writes (which vary run to run). Checked on what wasn't fitted: a frame of 60 printed lines
and 500 rects, 10.43 ms guessed, 10.47 on the board; falling sand's step 6 to 9% under, its drawing
within 5%; Paint on a font sheet, draw 5.9 ms guessed and 6.8 on the board, its opening frame 210 ms
and 177, 3 slow frames each.

Next: small rects' rows (the cost of a row likely depends on its width); app starts (compiling,
loading) not counted yet; a turbo setting (the web has none); tests that fail on a frame slow on the
board. (The readout is Sys+M's performance strip: on the desktop and the web, it shows board time.)

Recommended (2026-11-21, the user to decide): exported and web play skip a frame only past about 120%
of the board's time, so a game that plays on the board plays at least as well there; the developer's
window stays exact, with Sys+M's numbers. Apps kept under about 12 ms of board time a frame are safe
everywhere (the model's worst misses are 15 to 25%). pc.time() counted from the app's start (or a
pc.appTime(), with a way to reset it), so a Float keeps its microseconds; whole Ints for time since the
board started if wanted (pc.seconds() and pc.micros(): 68 years); frames as an Int at 60 a second last
414 days. A board run after the clock fix (2026-11-21): print2 (two letters a call) 26% under on the
board, from 17%: a short print's call cost moves with the code's layout in flash.
