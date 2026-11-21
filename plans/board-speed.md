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
