# Tile maps

A tile map is a picture made of tiles: a grid of cells, each a sprite of a sheet (a PNG, as Paint
makes them), or empty. Rinc keeps them in `.map` files, as studio's Maps app makes them, and its
library's `tilemap` reads, writes and draws them, for games and editors alike.

## A map's file

Text, so it's small and can be read and changed in the editor. `rinc map 2`, then each layer (up to
4, drawn one over another, the first at the bottom):

- `layer` and its number;
- `tiles`, its sheet's path and its tiles' size (`8`, or `8x16` for 8 across and 16 down, 4 to 64);
- `across`, how many sprites across its sheet was when the map was saved;
- `size`, its width and height in cells (each 1024 at most);
- `offset`, where its first cell is, in the map's pixels (negative too);
- then a line a row, three hex digits a cell: its sprite's number (from 0, counting across the
  sheet's rows), or `---` for empty.

```
rinc map 2
layer 1
tiles /rinc/save/paint/ground.png 8
across 16
size 4 2
offset 0 0
000001002---
---------003
layer 2
tiles /rinc/save/paint/things.png 16
across 8
size 1 1
offset 4 -8
00a
```

A sprite's number depends on how wide its sheet is. `across` is what keeps a map right when its sheet
gets wider or narrower: renumbered (tilemap's `refit`), each tile stays the same picture, and any now
past the sheet's edge goes empty. The first line says which format it is: `rinc map 2` is the one
there is (a `rinc map 1`, of before layers, isn't read).

## tilemap: the library's maps

`import tilemap` (`/rinc/lib/tilemap.rio`). It's Rio, and keeps nothing of its own: the app gives a
map its room for cells, 2 bytes a cell, as many as it wants, and reads each new level into the same
room. Each layer's sheet is the app's too: it reads the picture and passes it to `draw`.

How much room: the biggest map's cells, all its layers' together, 2 bytes each. Maps shows it at its
top right, as the `Blob` a game needs (and says so when it saves); `tilemap.cellsIn(text)` says from a
map's text; and `load`, when a map doesn't fit, logs the `Blob` it needs.

```rio fragment
import tilemap
cellRoom: Blob[4096 * 2]                   # room for 4096 cells, in all its layers
level := tilemap.Map{cells = cellRoom[]}
text: Blob[..20000]                        # room for the biggest map's file
sheetRoom: Blob[..(128 + 1) * 128]
sheet := pc.Image{}
if level.load("/rinc/save/maps/level1.map", text) # (else the log says why: the room it needs, say)
  sheet = pc.imageRead(level.sheetPath(0), sheetRoom)
end
camera := {0, 0}
draw* :: proc()
  pc.cls(0)
  level.draw(0, sheet, camera)             # layer 0, the map's pixel camera at the screen's top left
  if level.at(0, (camera + {160, 120}) / 8) == 5
    pc.print("on a 5", {4, 4}, 7)
  end
end
```

| | |
|---|---|
| `tilemap.Map` | `layers` (`count` of them), each a `tilemap.Layer`: `size` in cells, `at` (its offset), `tile` (its tiles' size), `across`; and `cells`, the app's room |
| `m.load(path, text) -> Bool` | a map's file read into it, through `text` (the app's `Blob[..]` for its text): false if it can't be, and the log says why: no such file, not a map, or the room it needs (`Blob[..n]` for its text, or `Blob[n]` for its cells) |
| `m.read(text) -> Bool` | a map's text read into it: false if it isn't one, or it has more cells than the room (and the map's as it was) |
| `tilemap.cellsIn(text) -> Int`, `m.needs() -> Int` | the cells a map's text says it has (-1: not a map); the bytes of room a map's cells take |
| `m.write(out) -> Bool` | its text, into `out` (a `Blob[..]`): false if it didn't all fit |
| `m.at(layer, p) -> Int`, `m.set(layer, p, t)` | cell `p`'s tile (`tilemap.EMPTY` for none, and past the layer's edges); and changed |
| `m.sheetPath(layer) -> String`, `m.setSheet(layer, path)` | a layer's sheet |
| `m.draw(layer, sheet, camera, how?)` | a layer from its sheet, the map's pixel `camera` at the top left; `how` (a `tilemap.Drawing`): the box it's in (`at`, `size`), `shrink` (2, 4: an editor's view of big tiles), `missing` (a color for a tile past the sheet's sprites) |
| `m.spriteAt(layer, t, width) -> [2]Int` | sprite `t`'s corner in a sheet `width` pixels wide |
| `m.refit(layer, width) -> Int` | its tiles renumbered for a sheet now `width` pixels wide: -1 if they're as they were, else how many went |
| `m.start(size, path, tile, across) -> Bool` | a new map: one layer of `size`, all empty |
| `m.reshape(layer, size, shift, scratch) -> Bool` | a layer made `size`, its cells moved `shift`, its offset back as far (`scratch`: room to work in, the map's cells' worth) |
| `m.addLayer(after) -> Bool`, `m.removeLayer(layer) -> Bool` | a layer added (empty, as big as the one before it) or taken out |
| `tilemap.notedSize(path) -> [2]Int` | a sheet's sprites' size, as Paint's notes in it say (else 8 by 8) |

A game that draws its own way, edges worked out from the cells around each one, say, reads the cells
with `at` and draws as it likes; or works them out once, as a level loads, into a layer of its own
that `draw` then draws. On the Fruit Jam, a screen of 8 by 8 tiles (41 by 31 in view) takes about 9
ms to draw, about half a frame.

## Draft: `rinc map 3`, with things

Not built yet: the plan, to settle before it is. Studio's Maps, as it would be with it: its
`maps/help-next.md`. Version 2 stays readable until its maps are converted (Maps, saving one, writes
version 3); then it goes, as version 1 did.

### What changes

- **Sheets listed once**, at the top, each a path and its sprites' size; layers name one by number.
  The same PNG can be listed twice with different sprite sizes, so version 2's layers (each with its
  own path and size) always convert.
- **Any number of layers**, as many as the app gives room for, each with a name.
- **Two kinds of layer:** `tiles`, a grid of cells (as now), and `things`, a list of sprites placed at
  pixel positions, each one sprite or a rectangle of them, with a name if it needs one. Things are for
  what a level places rather than paves: the player's start, enemies, items, doors, signs, triggers.
- **One room for the bytes:** the app gives a map one `Blob`, for its cells, its things and their
  names together, laid out in it as the map's read. So the room a map needs is still one number, the
  `Blob[n]` Maps shows. Sheets and layers are structs, in arrays of the app's.

### The file

```
rinc map 3
sheet 1 /rinc/save/paint/cave.png 8 across 16
sheet 2 /rinc/save/paint/chars.png 8x16 across 5
layer 1 ground tiles 1
size 32 16
offset 0 0
000001002...
layer 2 actors things 2
offset 0 0
thing 012 at 40 64 name player
thing 007 at 120 64
thing 003 at 64 96 span 2 1
thing --- at 200 0 span 2 4 name exit
```

- `sheet`: its number (from 1), its path, its sprites' size (`8`, or `8x16` for 8 across and 16 down;
  4 to 64 each) and `across`, how many sprites it had across when the map was saved. A sheet can be any
  width and height: its sprites are the whole ones across and down (pixels past the last whole sprite,
  at its right or bottom, aren't one), numbered from 0 across each row.
- `layer`: its number (from 1, in the order they're drawn, the first at the bottom), its name, its kind
  (`tiles` or `things`) and its sheet's number. A `tiles` layer's `size` follows (cells across and
  down, 1024 at most each), its `offset` (where its first cell is, in the map's pixels) and its rows,
  three hex digits a cell, as in version 2. A `things` layer has an `offset` and no size: its things
  can be anywhere.
- `thing`: its sprite, three hex digits (`---`: none, for a trigger or a spawn point), `at` (pixels
  from its layer's offset, -32768 to 32767 each), then, only when they aren't the usual: `span`, how
  many sprites across and down it is from its sprite at the top left (1 to 16 each; `1 1` left out),
  and `name`. Listed in the order they're drawn.
- A name, a layer's or a thing's: 1 to 15 bytes of letters, digits, `_`, `-` and `.`. No spaces, so a
  line reads only one way. Two things can share a name (`find` finds the first).

### The library's structs

```rio fragment
Sheet* :: struct
  path: Blob[..96]
  sprite: [2]Int     # its sprites' size, in pixels
  across: Int        # sprites across, when the map was saved (-1: not known)
end
Layer* :: struct
  name: Blob[..15]
  kind: Int          # TILES or THINGS
  sheet: Int         # which of the map's sheets (from 0)
  at: [2]Int         # its offset, in pixels
  size: [2]Int       # TILES: in cells
  start: Int         # where its cells, or its things, start in the room
  count: Int         # THINGS: how many
end
Thing* :: struct     # one thing, unpacked (in the room it's packed: below)
  at: [2]Int         # pixels, from its layer's offset
  sprite: Int        # its top-left sprite, or EMPTY
  span: [2]Int       # sprites across and down
  name: Blob[..15]   # "" for most: a copy, so it stays good as the map changes
end
Map* :: struct
  sheets: [..]Sheet  # the app's arrays, as many as it has room for
  layers: [..]Layer
  room: Blob         # the app's room: the cells, the things and their names
end
```

In the room, in order:

1. each `tiles` layer's cells, 2 bytes a cell, row by row (as now);
2. each `things` layer's things, 10 bytes each: x and y (2 bytes each, signed), the sprite (2), the
   span less 1 (a byte: 4 bits across, 4 down), a byte kept for later (flags, say), and where its name
   is (2 bytes: 0xFFFF for none);
3. the names: a length byte, then its bytes, each.

So 200 things take 2 KB, and their names only what they spell.

### In a game

```rio fragment
import tilemap
sheetRoom: [4]tilemap.Sheet
layerRoom: [8]tilemap.Layer
room: Blob[12000]
text: Blob[..20000]
level := tilemap.Map{sheets = sheetRoom[], layers = layerRoom[], room = room[]}
if level.load("/rinc/save/maps/cave.map", text)
  actors := level.layerNamed("actors")       # -1: none
  for i in 0..<level.things(actors)
    t := level.thing(actors, i)
    if t.name.asString() == "player"
      start(t.at)
    case t.sprite != tilemap.EMPTY
      spawn(t.at, t.sprite, t.span)
    end
  end
end
```

| (planned) | |
|---|---|
| `m.load`, `m.read`, `m.write`, `m.at`, `m.set`, `m.draw`, `m.refit`, `m.reshape` | as now (`draw` draws a things layer's sprites too, in order; a thing with none isn't drawn) |
| `m.things(layer) -> Int`, `m.thing(layer, i) -> Thing` | a things layer's count, and its thing `i`, unpacked |
| `m.find(name) -> [2]Int` | the first thing of that name: its layer and its number (`{-1, -1}`: none) |
| `m.layerNamed(name) -> Int` | a layer by its name (-1: none) |
| `m.place(layer, t) -> Bool`, `m.change(layer, i, t) -> Bool`, `m.takeOut(layer, i)` | a thing added (drawn last), changed, or taken out: false if the room's full |
| `m.thingAt(layer, p) -> Int` | the last-drawn thing over the map's pixel `p` (-1: none), as an editor picks one |
| `m.addLayer(after, kind) -> Bool`, `m.removeLayer(layer)`, `m.moveLayer(layer, to)`, `m.nameLayer(layer, name) -> Bool` | layers, any number of them, as the app's array has room |
| `m.compact()` | the room's names packed together again (names renamed or taken out leave theirs until then) |
| `tilemap.roomIn(text) -> Room`, `m.needs() -> Room` | what a map needs: `bytes` (the room's), `sheets` and `layers` (how many) |

`load`'s log says which falls short: `Blob[n]` for the room, or how many sheets or layers.

### Issues to watch

- **The sprite limit.** Three hex digits number 4,095 sprites, `---` taking the last. A 320 by 256
  sheet of 4 by 4 sprites has 5,120, and those past 4,094 can't be placed. Rare; Maps says so when a
  sheet has more.
- **Pixel limits.** A thing's x and y are 16 bits (to 32,767), but a layer 1,024 cells of 64 pixels is
  65,536 across. Things stay inside what fits (place says no past it).
- **Sheets changing width.** `across` renumbers cells as now, and things' sprites the same way. A
  thing's span stays: a rectangle that now runs past the sheet's right edge draws as missing past it.
- **One sheet a things layer.** A level whose things come from two sheets uses two things layers.
- **Moving bytes.** Cells, things and names share one room, so a layer that grows moves what's after
  it (as cells move for later layers now). Things and names go after all the cells, since cells grow
  most while drawing; but moving 32 KB a byte at a time in Rio, on the board, needs measuring.
- **Sheets by reference.** A `Sheet` is 27 words (its path), past Rio's 16 for a copy, so code reaches
  one by number, `&level.sheets[i]`, not with `for s in`.
- **Names left behind.** Renaming a thing, or taking it out, leaves its old name in the room until the
  map's written and read again or `compact` runs (as `place` does when the room fills). `thing` hands
  back a copy, so a name a game holds never goes stale.
- **Order.** Things draw in their list's order. Games that sort by y (a thing lower down drawn over
  one higher) sort them themselves.
- **The text's room.** A thing's line is about 30 bytes: 1,000 things, 30 KB more text to read and
  write. `pc` reads and writes files whole, so the app's text room holds it all; a `pc.fileAppend`, or
  reading a file in parts, would let a map be written and read a piece at a time.
