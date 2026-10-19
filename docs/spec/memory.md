# Memory

An app gets one block of memory, its **app space**: 256 KB, for everything it is. Nothing is allocated
while it runs, so whether an app fits is known before it runs, and it's the same on every Rinc.

## How an app is compiled

Every Rinc compiles an app the same way, so it gets the same answer for the same app:

1. Rio's compiler works in the app space itself. That's memory the program will use for globals and
   arrays, which are zero until it runs, so the compiler borrows it.
2. The app's files are read a part at a time, through a 3.5 KB window (2 KB of text, and room for the
   tokens and names it's reading) at the top of the compiler's string pool, so no file has to fit in
   memory whole, and an app can be as many files, as big, as it likes. Its string literals go into the
   string pool as they're read (in Rio's C API: `rio_init` with the app space as its memory and no
   separate code buffer, then `rio_compileStream`: see [Rio's memory
   model](https://contextfreeinfo.github.io/rio/guide/memory.html)).
3. The program's final layout then takes the whole app space.

So an app has two limits:

- **Compiling:** the compiler's tables, the constants and the strings share the app space. The tables
  are sized to fill it, the same for every app, however many files it is or however long they are.
- **Running:** the code (8 bytes an instruction), the constants, the globals and arrays, the procs'
  frames (procs that never run at the same time share one), the strings, and Rio's small runtime tables.

An app that doesn't fit gets a compile error ("out of memory", "too many symbols" and so on), and
doesn't run.

```rio
big: [60000]Int                    # 240 KB of the 256
big[59999] = 1
log(big.len())
```

```output
60000
```

```rio
toobig: [70000]Int  # error: out of memory
```

## Seeing how near the limits an app comes

`rinc -m app.rio` on the desktop compiles an app as every Rinc does and reports both limits: how full
each of the compiler's tables got, and where the memory goes when it runs:

```
apps/chord/chord.rio: fits in the 256 KB app space
  compiler room   248208 bytes
  source window   3584 bytes
  symbols         112 of 1742: 6%
  ...
  string pool     11756 of 21120: 55%
  constants       356 bytes
  data            82876 bytes
  code            2112 bytes
  strings, tables 11672 bytes
  free            165128 bytes
```

Whatever comes nearest 100% is what would run out first: here the string pool, as chord carries its
sound as hex text (in `parts/sound.rio`). When an app runs out of something, the Fruit Jam and the
desktop show the same report with the error.

## Outside the app space

A Rinc keeps its own things outside the app space, where apps can't reach them except through the `pc`
procs: the screen (75 KB, a byte a pixel), the voices' state, the typed-text queues, and one Rio session
(see [Rio inside an app](rio.md)), whose memory is the app's own. They don't count against an app's
256 KB, and a Rinc needs room for them besides. The voices read sound data from the app space as they
play, so they need no copies.

## Images

An app can also be compiled ahead of time into a Rio image, as the web pages and exported games are. An
image's memory addresses are absolute, so images for a Rinc are compiled for a 256 KB app space, as
above, and load into one.
