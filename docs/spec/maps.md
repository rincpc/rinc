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
past the sheet's edge goes empty. A map of before layers, `rinc map 1`, is read too: one layer, its
`size`, `tiles` and maybe `across`, then two hex digits a cell (`--` for empty), no `offset`.

## tilemap: the library's maps

`import tilemap` (`/rinc/lib/tilemap.rio`). It's Rio, and keeps nothing of its own: the app gives a
map its room for cells, 2 bytes a cell, as many as it wants, and reads each new level into the same
room. Each layer's sheet is the app's too: it reads the picture and passes it to `draw`.

```rio fragment
import tilemap
cellRoom: Blob[4096 * 2]                   # room for 4096 cells, in all its layers
level := tilemap.Map{cells = cellRoom[]}
text: Blob[..20000]
_ = pc.fileRead("/rinc/save/maps/level1.map", text)
sheetRoom: Blob[..(128 + 1) * 128]
sheet := pc.Image{}
if level.read(text.asString())
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
| `m.read(text) -> Bool` | a map's text read into it: false if it isn't one, or it has more cells than the room (and the map's as it was) |
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
