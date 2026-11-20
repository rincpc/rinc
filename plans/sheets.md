# Sheets drawn where they are, at their own depth

A plan (2026-11-20), for the user's review before any code. The aim: fonts, brushes and sprite sheets
cost far less memory, and a sheet can be drawn straight from the card (or flash) without reading it
all in, while staying ordinary PNG files that other programs open. And one format everywhere: Rinc's own
font in flash, a font on the card and a font in an app's memory are the same kind of sheet, read by the
same code into the same `Font` (and CJK, later, just a bigger one, in flash or on the card).

## Today

- `pc.imageRead` inflates a PNG into the app's memory, one byte a pixel, Rinc's colors (each the
  palette's nearest). It needs `(w + 1) * h` bytes while reading (PNG's rows with their filter bytes).
- `pc.fontRead` is that, plus two bytes a glyph for a proportional font.
- So a 5x11 Latin font of 224 characters, 16 across (80 by 154), costs every app that prints with it
  about 12 KB of its 256 KB.
- Paint writes PNGs indexed at 8 bits, compressed. The reader already reads indexed PNGs at 1, 2, 4
  and 8 bits (`png.c`), widening them to a byte a pixel as it goes.

## 1. Sheets at their own depth: 2, 4, 16 or 256 colors

A sheet that uses few colors is kept, and drawn, at 1, 2, 4 or 8 bits a pixel, not always 8:

| depth | colors (0: see-through) | for |
|---|---|---|
| 1 bit | 2 | fonts, masks, simple glyphs |
| 2 bits | 4 | fonts with a shade or outline; Game Boy-like sprites |
| 4 bits | 16 | most sprites and tiles (as PICO-8's) |
| 8 bits | 256 | anything |

- **A sheet's own palette.** The pixels are indices into the sheet's own small palette, its PNG's PLTE
  (index 0 see-through, as now), and each entry is one of Rinc's colors. Drawn, each pixel goes through
  that palette to Rinc's color.
- **Color mapping, nearly free.** Drawing already maps through a palette, so an app can give another one:
  `Blit{map = ...}`, an index to a Rinc color (or see-through) for each. That gives palette swaps (a red
  team and a blue one from one sheet; a hit flash; night and day) and lets `print` draw a 1-bit font in
  any color, as it does now. A 2-bit font can have a body, a shade and an outline, each mapped.
- **Memory:** that Latin font at 1 bit is about 1.5 KB, not 12. A 4-bit 128x128 sheet is 8 KB, not 16.
- **Cost in the blitter:** a row's pixels are unpacked into a row buffer (up to 320 bytes, on the C
  stack, not the app's) and mapped as they go, then drawn as now (scale, mirror, see-through), so one
  drawing path serves every depth. Slower than a straight byte copy, but by a little: an unpack and a
  table lookup a pixel. To measure on the board (the frame-time readout, roadmap).
- **Not hard:** the reader does the unpacking already (for widening); the new work is keeping the
  packed form, the map, and the row unpack in blit and print.

API, a sketch: one reader, not two (`imageRead`, at the file's own depth by default):

```rio fragment
Image* :: struct
  pixels: Blob
  size: [2]Int
  depth: Int        # bits a pixel: 1, 2, 4 or 8 (0: 8, as now)
  palette: Blob     # its own colors, Rinc's: index -> color (empty: indices are Rinc's colors, as now)
end
ImageRead* :: struct
  bytes: Bool       # a byte a pixel, each Rinc's color (as imageRead gives now): for apps that edit pixels
end
imageRead* :: proc(path: String, room: Blob[..], o?: ImageRead) -> Image  # at its own depth, unless o.bytes
imageAt* :: proc(image: Image, p: [2]Int) -> Int   # a pixel's index, at any depth (its color: its palette's)
Blit* :: struct     # as now, and:
  map: []Int        # its indices' colors this time (-1: see-through), instead of its palette
end
```

- Most pictures are only drawn, and drawing takes any depth, so games, tilemap and fonts save memory
  without a change; an app that reads or edits pixels itself (Paint) asks for bytes, or reads with
  `imageAt`. (Why not a second reader: two names for one thing, and every app would have to pick.)
- `fontRead` is `imageRead` and the font's notes, so a font keeps its sheet's depth too, and `print`
  unpacks glyph rows as it goes. Rinc's own font is the same: a sheet in flash, read where it lies.
- What changes for apps today: one that reads `image.pixels[y * w + x]` from a picture it didn't make
  needs `{bytes = true}` (or `imageAt`). Studio's apps and the library are checked as part of step 1.

## 2. Sheets saved uncompressed, readable in place

Still PNG, still opened by any program, but Paint saves the pixel data in zlib's *stored* (uncompressed)
blocks, one block a row:

```
IHDR, PLTE (its own colors), tRNS, tEXt (notes), rnFg (flags), IDAT:
  zlib header (2 bytes)
  for each row: a stored block's header (5 bytes: final bit, length, its complement)
                filter byte 0, then the row's packed pixels (rowBytes = (w * depth + 7) / 8)
  Adler-32 (4 bytes)
IEND
```

- A row is at `dataStart + 2 + y * (5 + 1 + rowBytes) + 5 + 1`: reading a glyph or a sprite needs only
  its rows, found by a sum.
- A reader knows a file it can read in place by its shape: one IDAT, zlib's header, and the first and
  last blocks stored with length `rowBytes + 1`. Any other PNG (another program's, compressed) is read
  whole, as now, until Paint saves it again.
- On disk it's bigger than compressed, but small at low depth: that font at 1 bit, about 2.5 KB (rows of
  10 bytes plus 6 of framing), against about 1 KB compressed. A 320x240 sheet at 8 bits is 77 KB. Exports
  and bundles can compress again as they're made (roadmap 9b: sizes on disk).
- Writing stays whole-file (Adler-32 and the chunk's CRC cover all of it): Paint saves as now.

## 3a. Parts of a sheet, read into memory

The user's idea (2026-11-20): `imageRead` with a part, `{from, size}` in `pc.ImageRead`, reads just that
rectangle of a sheet into memory, at its depth, with its palette: a font's range of glyphs, a few sprites,
a character's frames, for reuse, without the rest ever in memory.

- A sheet saved uncompressed (Paint's, step 2): each row it needs read from where it is (a sum), its
  columns kept, shifted to start on a byte at 1, 2 or 4 bits. No cache, no room past the part itself.
- Any other PNG (compressed: another program's): its rows can't be reached without inflating all before
  them, so a part fails (`size` {0, 0}) and the log says so (save it in Paint). Not read whole quietly: the
  point is the memory a part saves.
- A font from a part: `fontRead` takes the same part (whole rows of cells: a range of characters), its
  notes' first and last moved to match.
- Tests: a part at each depth, from every column offset in a byte, equal to the same part of the whole
  read; a part past an edge; a compressed PNG refused, and said so.

## 3. Drawing from storage (and flash)

A sheet opened in place (`sheetOpen(path, cache)`) is drawn without its pixels in memory: each row is
read as it's drawn.

- **A cache in the app's memory, not the firmware's** (the firmware has about 2.5 KB free): the app gives
  a Blob (1 KB, say: two of the card's 512-byte sectors), and the sheet's rows are read through it. A font
  at 1 bit and 16 rows a sector is a handful of sectors; text in it mostly hits the cache.
- **Flash:** a sheet built into the firmware (Rinc's own font, later a CJK font) is read where it lies,
  with no cache: memory-mapped. One font format everywhere: Rinc's built-in font a sheet like any other.
- **Paint:** brushes from other sheets drawn and picked from where they are, so several sheets are at
  hand without each one in memory; the sheet being edited is still in memory, at 8 bits, to edit.
- **Speed:** a card read per sector missed. To measure; fonts and brushes are the cases that matter, and
  both fit a small cache.

## 4. Paint

- Saves the smallest depth that holds the sheet's colors (or the depth chosen: a setting in the command
  list), as uncompressed PNG. Shows the depth and the color count by the size ("1 bit, 2 colors").
- Edits at 8 bits in memory, as now; its palette view marks the sheet's own colors.
- A font made in Paint (Make it a font) starts at 1 bit.
- Later, maybe: recolor a sheet's palette entry (every pixel of that index at once).

## Progress

- Step 1 done (2026-11-20): `Image.depth` (1, 2, 4; 0 or 8 a byte a pixel), its palette after its rows in
  `pixels` (no field of its own: `Font` stays within what Rio passes by value), `imageRead` at the file's
  depth (`{bytes = true}`: a byte a pixel, Rinc's colors), `imageAt`, `Blit.map`, blit and print reading
  any depth. Tilemap, Maps and Paint read bytes (tilemap keeping sheets packed: later). `pc.Text.line` went,
  for `Font.line` (Rinc's own font's too: `{font = {line = 12}}`), to keep `Text` at 16 words.
  tests/depths.py: each depth read, drawn (plain, scaled, mirrored, a part) and mapped as its bytes are, and
  a 1-bit font printed as its 8-bit twin. Fonts save memory once Paint writes low depths (step 2).

- Step 2 done (2026-11-20): `ImageWrite.sheet`: the colors used, 0 first and then in order, as the file's
  own palette, at 1, 2, 4 or 8 bits, its rows in stored blocks, one IDAT (so a row's at a sum). Paint saves
  so: a sheet re-saved in Paint shrinks (a 1-color font, 1 bit). tests/depths.py: sheets of 2, 3, 11 and 21
  colors written, read back at 1, 2, 4 and 8 bits, and each file's shape checked; Paint's tests read any
  depth.

- Step 3a done (2026-11-20): `ImageRead.from` and `size`: a part of a sheet saved uncompressed read into
  memory a row at a time (its rows checked as stored blocks; its columns shifted to start on a byte), at
  its depth with its palette, or as bytes; a part past the edges clipped; a compressed PNG refused, the log
  saying why. `fontRead` takes the same, as whole rows of cells (its first and last moved to match).
  tests/depths.py: parts at each depth from each column in a byte, packed and as bytes, against the whole;
  an edge; a compressed one refused; a font's second row printing as the whole font prints it.

- Step 3, as far as it makes sense (2026-11-20): Rinc's own font is a sheet like any user font,
  src/core/rinc-font.png (6x9 cells, 16 across, 32 to 126, 1 bit, uncompressed: tools/fontsheet.py made it
  from the old table, glyph for glyph, and makes font.c from it: the file's bytes), drawn from where it
  lies in flash (rinc_fontBit: a row at a sum, its stored blocks' 6 bytes stepped over). 1,116 bytes of
  flash for the font (760 as a table). tests/fontsheet.py: every character printed, each pixel as the PNG
  has it. Drawing sheets from the card through an app's cache, set aside: reading the card a frame at a
  time is slow, and parts read into memory (3a) give what's needed once. Next, when wanted: other fonts in
  flash an app can name (a 5x11 default, CJK), which needs a way for a Font to name one (a field: pc.Text
  is at Rio's 16 words, so likely a Font's cell or first, or Rio passing more). Drawn a pixel at a time
  (rinc_fontBit a pixel) it was 4x slower than the old table; rinc_char now reads a glyph's row as one
  16-bit window, as fast as the table was (a host bench: 0.137 s to the table's 0.132 s for 2.8 million
  glyphs; a pixel at a time, 0.601 s). Replacing the font (a 5x11 in 6x12, say) needs no name: the PNG
  replaced, and console.c's rinc_CW and rinc_CH (and print's 6 across) changed to its cell.

- Rinc's own font replaced (2026-11-20): the user's rinc12 (5x11 in 6x12, 32 to 255, fixed width), its
  cells from its notes; compact by default (a line of 9: the top 3 rows, above every ASCII glyph, worked out
  and left out), whole at a line of 12; print reads UTF-8. p q r s t v w ~ were blank in it: the old
  glyphs kept as placeholders. 2,609 bytes of flash. Paint's "Font: fixed width" command.

## Steps
(3a, parts read into memory, comes before 3: simpler, no cache, and most of the use.)

1. Low-depth sheets in memory, and the map: `Image.depth` and `palette`, `imageRead` at the file's depth
   (and `{bytes = true}`, `imageAt`), blit and print unpacking rows, `Blit.map`; every app and library
   that reads pixels checked. Tests: each depth drawn equal to its 8-bit read; a map; scale and mirror at
   each depth; a 1-bit font printed in two colors. The biggest win for the least.
2. Paint saving low depths, uncompressed. Tests: saved and read back equal; other programs' PNGs still
   read; the in-place shape recognized, and not recognized when it isn't.
3. Drawing from storage through an app's cache, and from flash. Tests: drawn from the card equal to drawn
   from memory; a cache of one sector; a sheet changed on the card while open (its rows read again).
4. Paint's brushes from other sheets: brushes and copy and paste made one thing (below).

Each step stands on its own; 1 and 2 give most of it.

## Step 4: a brush is what's copied (done, 2026-11-20)

Done as below: Ctrl+C and Ctrl+X hold the copy as the brush, Space, a click or A stamps it (0s
see-through, a stamp an undo), Esc or a tool puts it down, H V T on the brush (T any shape), Ctrl+V picks
up the newest (in the sheet view, a whole sprite pasted, 0s too), K the Brushes view. K keeps a 16x16
thumbnail of each (2 KB: 8 whole ones, 8 KB, left Paint out of memory) and reads the one picked whole
from the clipboard again (gone since: it says so). Tests in studio's tests/paint.py (8 cases), each
shown to fail with its part taken out; the view checked on the board. Ctrl+Tab in the studio apps is
next on its own (rinc's ROADMAP).

Today Ctrl+C puts the selection (or the sprite: to 32x32) on the clipboard every app shares, kind
"paint" (its width, height, whether it was a selection, its pixels), and Ctrl+V writes it once, at the
cursor (a selection) or the sprite's corner, its 0s too. A brush and a copy are the same thing, some
pixels of a size, so make them one:

- **Ctrl+C (and Ctrl+X) picks the copy up as the brush, at once** (recommended, the user's idea). It's
  shown at the cursor, under the mouse or the arrows, not yet drawn; a click, Space or A stamps it, and
  stamps again as often as wanted (held and moved: a stroke of it, as the pencil's). Esc or a tool's key
  puts it down (it stays on the clipboard). One undo a stamp, or a stroke.
- **Its 0s are see-through when stamped,** as pc.blit draws, so a brush has a shape (the user: great).
- **Ctrl+V:** in the pixel view, the clipboard's newest "paint" item picked up as the brush (another
  run's, another sheet's); in the sheet view at the right, a whole sprite pasted into the sprite picked
  there, all its pixels, 0s too, as today (the user: a sprite at a time still wanted).
- **H, V and T act on the brush while it's held** (the sprite otherwise; the user likes it). T turns any
  brush, square or not: it's its own block of pixels, so turned its width and height swap. A brush is
  32x32 at most (the clipboard's room).
- **Recent brushes:** the clipboard keeps the last 8 (clip's KEEP). A strip of them on Paint's screen if
  there's room; there isn't much at 320x240 (the sprite and sheet are 128 square each, the colors at the
  right), so a Brushes view, K, opened as C opens Colors: the 8 drawn in a row, the arrows or a click
  picking one up, Esc back. Each kept brush is its own pixels, in clip's stash on the card (32x32 at most, a
  file each), not a reference to its sheet, so a sheet moved, deleted or no longer a PNG costs nothing:
  the view reads only the stash, and a stash file missing or short is skipped (as clip.paste skips one
  now). If brushes were ever kept as a sheet's path and part, to save room, that fallback is needed: a
  part that won't read drawn as an empty, crossed cell, and dropped from the list. Rinc's clipboard picker (the roadmap's Next 8, Sys+V) can share its drawing.
- **From other sheets:** the clipboard is on the card and outlives a run, so a brush from another sheet
  is Ctrl+O there, Ctrl+C, then back with Ctrl+Tab: the editor's Ctrl+Tab (the files had lately, walked
  while Ctrl's held) in every studio app, a library they share (rinc's ROADMAP). Showing a second sheet
  beside Paint's own, read in part (3a), saves the trip but costs memory Paint's 80 KB sheet already uses
  on the board (about 2.5 KB free): left for later, if the trip is felt.
- **Later, not this step:** a gradients view (the user: likely needed); Maps pasting a "paint" item as
  tiles.

Tests: Ctrl+C then Space stamping (0s left alone), twice, undone once; Ctrl+V in the sheet view equal to
today's paste; H, V and T on a held brush, T on a 3x5 one (5x3 after); Esc putting it down with the
clipboard kept; a brush copied in one Paint run and stamped in the next; K's view picking the one
before; Ctrl+Tab's walk, in each app.

## To decide (the user)

- Names: `sheetOpen` (drawing from storage), `ImageRead.bytes`, `imageAt`, `Blit.map`, `Image.depth` /
  `palette`. (One reader, `imageRead`, chosen over a second `sheetRead`: the assistant's call, 2026-11-20.)
- Whether Paint picks the depth by itself (recommended) or asks.
- Whether a sheet's own palette is shown in Paint as its own row of colors.
- Rinc's own font moved into flash as a sheet (step 3), replacing the font built into the code.
- Step 4 (left to decide): Ctrl+C holding the brush at once (recommended); Ctrl+Tab from a library every studio app shares (recommended); a second sheet beside,
  later. K for the Brushes view: the user left the key to the assistant (2026-11-20; Ctrl+Tab is the
  recent sheets, to pick brushes from). The user's already: 0s see-through, Ctrl+V a whole sprite in the sheet view, H/V/T on the
  brush, T turning any shape.
