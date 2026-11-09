# Tile maps

A tile map is a picture made of tiles: a grid of cells, each a sprite of a sheet (a PNG, as Paint
makes them), or empty. A map can also hold things: sprites placed anywhere, the player's start, enemies,
doors, signs. Rinc keeps maps in `.map` files, as studio's Maps app makes them, and its library's
`tilemap` reads, writes and draws them, for games and editors alike.

## A map's file

Text, so it's small and can be read and changed in the editor. `rinc map 3`, then its sheets, then its
layers, drawn one over another, the first at the bottom:

```
rinc map 3
sheet 1 /rinc/save/paint/cave.png 8 across 16
sheet 2 /rinc/save/paint/chars.png 8x16 across 5
layer 1 ground tiles 1
size 4 2
offset 0 0
000001002---
---------003
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
  at its right or bottom, aren't one), numbered from 0 across each row. A new map with no sheet yet
  has one with an empty path.
- `layer`: its number (from 1), its name, its kind (`tiles` or `things`) and its sheet's number.
- A `tiles` layer: its `size` (cells across and down, 1024 at most each), its `offset` (where its first
  cell is, in the map's pixels, negative too), then a line a row, three hex digits a cell: its sprite's
  number, or `---` for empty.
- A `things` layer: its `offset`, then a `thing` line each, in the order they're drawn: its sprite
  (three hex digits; `---` for none, a trigger or a start), `at` (pixels from the layer's offset, -32768
  to 32767 each), then only when they aren't the usual: `span`, how many sprites across and down it is
  from its sprite at the top left (1 to 16 each; `1 1` left out), and `name`.
- A name, a layer's or a thing's: 1 to 15 bytes of letters, digits, `_`, `-` and `.`. No spaces, so a
  line reads only one way. Two things can share a name (`find` finds the first).

A sprite's number depends on how wide its sheet is. `across` is what keeps a map right when its sheet
gets wider or narrower: renumbered (tilemap's `refit`), each tile stays the same picture, and any now
past the sheet's edge goes empty.

`rinc map 2`, of before sheets were listed and things, is read too (each layer with its own `tiles`
path and size, and `across`: a sheet each), and written as `rinc map 3`. A `rinc map 1` isn't read.

## tilemap: the library's maps

`import tilemap` (`/rinc/lib/tilemap.rio`). It's Rio, and keeps nothing of a map's own: the app gives a
map its room, one `Blob` for its cells, its things and their names, and arrays for its sheets and
layers, as big as it likes, and reads each new level into the same ones. Each sheet's picture is the
app's too: it reads it (`pc.imageRead`) and passes it to `draw`. The library's own room is 5 KB, to read
and write files through, a piece at a time: a map's text never has to fit in memory.

How much room: Maps shows it at its top right, as the `Blob` a game needs (and says so when it saves);
`tilemap.roomIn(path)` says from a map's file; and `load`, when a map doesn't fit, logs what it needs.

```rio fragment
import tilemap
sheetRoom: [4]tilemap.Sheet
layerRoom: [8]tilemap.Layer
room: Blob[12000]                            # its cells, things and names
level := tilemap.Map{sheets = sheetRoom[], layers = layerRoom[], room = room[]}
pixels: Blob[..(128 + 1) * 128]
sheet := pc.Image{}
if level.load("/rinc/save/maps/cave.map")    # (else the log says why: the room it needs, say)
  sheet = pc.imageRead(level.sheetPath(0), pixels)
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
camera := {0, 0}
draw* :: proc()
  pc.cls(0)
  level.draw(0, sheet, camera)               # layer 0, the map's pixel camera at the screen's top left
  if level.at(0, (camera + {160, 120}) / 8) == 5
    pc.print("on a 5", {4, 4}, 7)
  end
end
```

### Its structs

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
  start: Int         # TILES: its first cell, in the room's cells; THINGS: its first thing
  count: Int         # THINGS: how many
end
Thing* :: struct     # one thing, unpacked (in the room it's packed: below)
  at: [2]Int         # pixels, from its layer's offset
  sprite: Int        # its top-left sprite, or EMPTY
  span: [2]Int       # sprites across and down ({1, 1}: one)
  name: Blob[..15]   # "" for none: a copy, so it stays good as the map changes
end
Room* :: struct      # what a map needs (-1s: not a map)
  bytes: Int         # its room's
  sheets: Int
  layers: Int
end
Map* :: struct
  sheets: []Sheet    # the app's arrays, as many as it has room for
  sheetCount: Int
  layers: []Layer
  count: Int         # layers
  room: Blob         # the app's room: the cells, the things and their names
  cells: Int         # in all its tiles layers
  thingCount: Int    # in all its things layers
  names: Int         # their bytes, at the room's end
end
```

In the room: the tiles layers' cells from its start, 2 bytes a cell, row by row; then the things, 10
bytes each (x and y, 2 bytes each, signed; the sprite, 2; the span less 1, a byte, 4 bits across and 4
down; a byte kept for later; and where its name is, 2 bytes, 0 for none); and the names at the room's
end, growing down, each a length byte and its bytes. A name taken out or changed gives its bytes back
at once. So 200 things take 2 KB, and their names only what they spell.

A layer's cells are where its `start` says: a layer moved (`moveLayer`) moves no bytes. A `Sheet` is
27 words (its path), past Rio's 16 for a copy, so code reaches one by number, `&level.sheets[i]`.

### Its procs

| | |
|---|---|
| `m.load(path) -> Bool`, `m.loadStash(key) -> Bool` | a map's file (or one kept in the app's stash) read into it: false if it can't be, and it's as it was; the log says why: no such file, not a map (and the line), or the room it needs (`Blob[n]`, sheets, layers) |
| `m.save(path) -> Bool`, `m.saveStash(key) -> Bool` | written as `rinc map 3`: false if it couldn't be |
| `tilemap.roomIn(path) -> Room`, `m.needs() -> Room`, `m.used() -> Int` | what a map's file needs, and what this one does; the room's bytes it uses |
| `m.start(size, path, sprite, across) -> Bool` | a new map: one tiles layer of `size`, all empty, of the sheet at `path` |
| `m.at(layer, p) -> Int`, `m.set(layer, p, t)`, `m.inside(layer, p) -> Bool` | cell `p`'s tile (`tilemap.EMPTY` for none, and past the layer's edges); changed; whether it's a cell |
| `m.tile(layer) -> [2]Int`, `m.sheetPath(layer) -> String` | a layer's sheet: its sprites' size, its path |
| `m.spriteAt(layer, t, width) -> [2]Int` | sprite `t`'s top left in the layer's sheet, `width` pixels wide |
| `m.sheetFor(path, sprite, across) -> Int` | a sheet's number in the map's list, added if it isn't there (-1: no room) |
| `m.setSheet(layer, path, sprite) -> Bool` | a layer's sheet changed, its tiles kept where they are in the sheet (renumbered, for a sheet the map has across another number) |
| `m.refit(sheet, width) -> Int` | a sheet's tiles and things renumbered for it now `width` pixels wide: -1 if they're as they were, else how many went |
| `m.draw(layer, sheet, camera, how?)` | a layer from its sheet, the map's pixel `camera` at the top left: a tiles layer's tiles; a things layer's things, in order (one with no sprite isn't drawn). `how` (a `tilemap.Drawing`): the box it's in (`at`, `size`), `shrink` (2, 4: an editor's view of big tiles), `missing` (a color for a sprite past the sheet's) |
| `m.things(layer) -> Int`, `m.thing(layer, j) -> Thing` | a things layer's count, and its thing `j`, unpacked |
| `m.place(layer, t) -> Bool`, `m.placeAt(layer, j, t) -> Bool`, `m.change(layer, j, t) -> Bool`, `m.takeOut(layer, j)` | a thing added (drawn over the others; or as thing `j`, those from it on one later), changed, or taken out: false if there's no room (or it's no thing) |
| `m.thingOver(layer, p) -> Int` | the last-drawn thing over the map's pixel `p` (-1: none), as an editor picks one |
| `m.find(name) -> [2]Int` | the first thing of that name: its layer and its number (`{-1, -1}`: none) |
| `m.layerNamed(name) -> Int`, `m.nameLayer(layer, name) -> Bool`, `tilemap.nameOk(s) -> Bool` | layers by name |
| `m.addLayer(after, kind) -> Bool` | a layer after one (-1: first): a tiles layer as big as the nearest tiles layer, and where it is, empty; a things layer, with none. False if there's no room |
| `m.removeLayer(layer) -> Bool`, `m.moveLayer(layer, to)` | a layer taken out (never the last), or moved to be number `to` |
| `m.reshape(layer, size, shift) -> Bool` | a tiles layer made `size`, its cells moved `shift`, what's past its new edges gone, its offset moved back as far (so its cells stay where they were on the map): false if it doesn't fit |
| `tilemap.sizeIn(s) -> [2]Int`, `tilemap.notedSize(path) -> [2]Int` | `"8"` or `"8x16"` as a size; a sheet's sprites' size as Paint's notes in it say (else 8 by 8) |

A game that draws its own way, edges worked out from the cells around each one, say, reads the cells
with `at` and draws as it likes; or works them out once, as a level loads, into a layer of its own
that `draw` then draws.

### Speed

On the Fruit Jam: a screen of 8 by 8 tiles drawn, about 5 ms; 500 things, 3 ms. A map of 16,384
cells and 500 things (38 KB of text): loaded in about 380 ms, saved in 340 ms, almost all of it the
card's. Reshaping, finding a thing and placing one take a millisecond or two.

### Limits

- **Sprites.** Three hex digits number 4,095 sprites, `---` taking the last. A 320 by 256 sheet of 4 by
  4 sprites has 5,120, and those past 4,094 can't be placed.
- **Pixels.** A thing's x and y are 16 bits, -32,768 to 32,767, though a layer 1,024 cells of 64 pixels
  is 65,536 across.
- **Sheets changing width.** `refit` renumbers cells and things' sprites alike. A thing's span stays:
  one that now runs past the sheet's right edge draws as missing past it.
- **One sheet a layer.** A level whose things come from two sheets uses two things layers.
- **Order.** Things draw in their list's order. Games that sort by y (a thing lower down drawn over one
  higher) sort them themselves.
