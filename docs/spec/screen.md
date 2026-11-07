# The screen

The screen is 320 pixels wide and 240 high, `x` from the left and `y` from the top. Each pixel holds a
color number, 0 to 255. A Rinc shows it at any whole-number scale (the Fruit Jam's HDMI output is
640x480, each pixel 2x2). An app starts with the screen all color 1, dark blue: Rinc's text screen,
where what it logs shows until it draws, and where Rinc shows what went wrong if it fails.

## Drawing

| proc | |
|---|---|
| `pc.cls(c)` | every pixel becomes `c` |
| `pc.pset({x, y}, c)` | pixel `x, y` becomes `c`; nothing happens off the screen |
| `pc.pget({x, y}) -> Int` | pixel `x, y`, or 0 off the screen |
| `pc.rect({x, y}, {w, h}, c)` | pixels from `x` up to `x + w`, and `y` up to `y + h`, become `c`; cut off at the screen's edges; nothing when `w` or `h` is 0 or less |
| `pc.rgb(c) -> Int` | color `c` of the palette (0 to 255), as `0xRRGGBB` |
| `pc.print(s, {x, y}, c)` | the text's characters, one every 6 pixels from `x`, drawn at `y` in color `c` |
| `pc.blit(image, at, b?)` | a picture (a `pc.Image`: its `pixels`, a byte each, row by row, and its `size`; as `pc.imageRead` gives one, see [apps](apps.md#files)), its top left at `at`, or `b.size` of it from `b.from` (a 0 in `size`: the rest of it that way), each pixel `b.scale` wide and high (a 0: 1; negative mirrors it that way: `{-1, 1}` flips it left for right); its 0s are see-through. `b` is a `pc.Blit` (`from`, `size` and `scale`, each a `[2]Int`), so what's left out of `{...}` is 0, and `b` left out is all 0s: `pc.blit(image, {10, 20})` draws all of it at its size |

A color is the low 8 bits of the number given: `pc.cls(257)` is `pc.cls(1)`.

```rio
pc.cls(300)
pc.rect({-5, -5}, {10, 10}, 7)
pc.rect({10, 10}, {0, 5}, 9)
log(pc.pget({0, 0}), pc.pget({4, 4}), pc.pget({5, 5}), pc.pget({10, 10}))
```

```output
7 7 44 44
```

A sprite drawn twice as big, its see-through pixel leaving what's under it, then its bottom row alone:

```rio
pc.cls(1)
pic: Blob[4] # (2 by 2)
pic[0] = 8
pic[1] = 0
pic[2] = 9
pic[3] = 10
image := pc.Image{pixels = pic[], size = {2, 2}}
pc.blit(image, {10, 10}, {scale = {2, 2}})
pc.blit(image, {40, 10}, {scale = {-2, 2}}) # (mirrored)
log(pc.pget({10, 10}), pc.pget({11, 11}), pc.pget({12, 10}), pc.pget({13, 13}), pc.pget({14, 14}))
pc.blit(image, {30, 30}, {from = {0, 1}, size = {0, 1}})
log(pc.pget({30, 30}), pc.pget({31, 30}), pc.pget({30, 31}))
```

```output
8 8 1 10 1
9 10 1
```

## Text

`pc.print` draws each byte of the text with Rinc's 5x8 font: the pixels of the character's glyph become
`c`, and the rest are left as they are. Characters other than ASCII 33 to 126 draw nothing, and every
character moves 6 pixels to the right. The glyphs are the table in
[`src/core/font.c`](https://github.com/rincpc/rinc/blob/main/src/core/font.c): 8 rows of 5 bits for each
character, bit 0 the leftmost pixel.

## The palette

The palette is 16 bands of 16 colors, a band to each row of 16 (as the Colors app shows them). Colors 0
to 15 are PICO-8's, 16 to 31 its secret palette (PICO-8's 128 to 143); then a band for each of 12 hues;
then 16 tints and 16 grays. PICO-8's are:

| color | RGB | color | RGB |
|---|---|---|---|
| 0 | `000000` black | 8 | `FF004D` red |
| 1 | `1D2B53` dark blue | 9 | `FFA300` orange |
| 2 | `7E2553` dark purple | 10 | `FFEC27` yellow |
| 3 | `008751` dark green | 11 | `00E436` green |
| 4 | `AB5236` brown | 12 | `29ADFF` blue |
| 5 | `5F574F` dark gray | 13 | `83769C` lavender |
| 6 | `C2C3C7` light gray | 14 | `FF77A8` pink |
| 7 | `FFF1E8` white | 15 | `FFCCAA` peach |
| 16 | `291814` brownish black | 24 | `BE1250` dark red |
| 17 | `111D35` darker blue | 25 | `FF6C24` dark orange |
| 18 | `422136` darker purple | 26 | `A8E72E` lime green |
| 19 | `125359` blue green | 27 | `00B543` medium green |
| 20 | `742F29` dark brown | 28 | `065AB5` true blue |
| 21 | `49333B` darker gray | 29 | `754665` mauve |
| 22 | `A28879` medium gray | 30 | `FF6E59` dark peach |
| 23 | `F3EF7D` light yellow | 31 | `FF9D81` peach |

The rest are made in OKLCH (Björn Ottosson's OKLab, as lightness, chroma and hue: a color space made so
equal steps look equal), by `tools/palette.py`, which writes them as the table in
[`src/core/palette.h`](https://github.com/rincpc/rinc/blob/main/src/core/palette.h):

| colors | band | hue |
|---|---|---|
| 32 to 47 | red | 29 |
| 48 to 63 | orange | 65 |
| 64 to 79 | yellow | 100 |
| 80 to 95 | lime | 121 |
| 96 to 111 | green | 142 |
| 112 to 127 | teal | 169 |
| 128 to 143 | cyan | 195 |
| 144 to 159 | azure | 229 |
| 160 to 175 | blue | 264 |
| 176 to 191 | violet | 296 |
| 192 to 207 | magenta | 328 |
| 208 to 223 | crimson | 359 |
| 224 to 239 | tints: 8 warm, then 8 cool | 60, 250 |
| 240 to 255 | grays | |

- **A hue's band:** its first 8 are vivid, dark to light, then 8 muted, dark to light: `32 + 16 * k`
  is hue `k`'s darkest vivid, and `+ 8` its darkest muted. The hues are sRGB's red, yellow, green,
  cyan, blue and magenta (where OKLCH puts them, but a warmer yellow), and the hue halfway between each
  two. The vivid steps run from lightness 0.28 to 0.88 (0.91 for orange to azure, most vivid when light), one of them where the
  hue is most vivid, at nearly the most chroma sRGB has there, no step much more than its neighbours;
  the muted ones from 0.36 to 0.86, at the same chroma, 0.07, for every hue.
- **The tints:** warm grays (toward orange) and cool ones (toward blue), each 8 from lightness 0.18 to
  0.92, at chroma 0.025.
- **The grays:** pure grays, black to white, even in lightness as it looks: `000000`, `101010`,
  `202020`, `303030`, `404040`, `4E4E4E`, `5E5E5E`, `6E6E6E`, `808080`, `919191`, `A2A2A2`, `B4B4B4`,
  `C7C7C7`, `D9D9D9`, `EBEBEB`, `FFFFFF`. Color 240 is black, as 0 is, for when 0 is see-through.

Each of the hues' and tints' colors is the one at 16 bits a pixel (below) nearest what OKLCH asked for,
so what a Rinc shows keeps its hue.

Every Rinc shows the palette at 16 bits a pixel, RGB565: each component's top 5, 6 and 5 bits. Where a
display has more bits, a component `v` of `n` bits becomes `v * 255 / (2^n - 1)`, rounded down, so every
Rinc shows exactly the same colors.
