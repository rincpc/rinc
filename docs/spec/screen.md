# The screen

The screen is 320 pixels wide and 240 high, `x` from the left and `y` from the top. Each pixel holds a
color number, 0 to 255. A Rinc shows it at any whole-number scale (the Fruit Jam's HDMI output is
640x480, each pixel 2x2). An app starts with the screen all color 1, dark blue: Rinc's text screen,
where what it logs shows until it draws, and where Rinc shows what went wrong if it fails.

## Drawing

| proc | |
|---|---|
| `pc.cls(c)` | every pixel becomes `c` |
| `pc.pset(x, y, c)` | pixel `x, y` becomes `c`; nothing happens off the screen |
| `pc.pget(x, y) -> Int` | pixel `x, y`, or 0 off the screen |
| `pc.rect(x, y, w, h, c)` | pixels from `x` up to `x + w`, and `y` up to `y + h`, become `c`; cut off at the screen's edges; nothing when `w` or `h` is 0 or less |
| `pc.print(s, x, y, c)` | the text's characters, one every 6 pixels from `x`, drawn at `y` in color `c` |

A color is the low 8 bits of the number given: `pc.cls(257)` is `pc.cls(1)`.

```rio
pc.cls(300)
pc.rect(-5, -5, 10, 10, 7)
pc.rect(10, 10, 0, 5, 9)
log(pc.pget(0, 0), pc.pget(4, 4), pc.pget(5, 5), pc.pget(10, 10))
```

```output
7 7 44 44
```

## Text

`pc.print` draws each byte of the text with Rinc's 5x8 font: the pixels of the character's glyph become
`c`, and the rest are left as they are. Characters other than ASCII 33 to 126 draw nothing, and every
character moves 6 pixels to the right. The glyphs are the table in
[`src/core/font.c`](https://github.com/rincpc/rinc/blob/main/src/core/font.c): 8 rows of 5 bits for each
character, bit 0 the leftmost pixel.

## The palette

Colors 0 to 15 are PICO-8's, 16 to 31 its secret palette (PICO-8's 128 to 143), 32 to 247 a 6x6x6 color
cube, and 248 to 255 grays:

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

- **The cube:** color `32 + 36 * r + 6 * g + b`, for `r`, `g` and `b` from 0 to 5, has each component at
  the level 0, 95, 135, 175, 215 or 255.
- **The grays:** colors 248 to 255 have each component at 19, 38, 57, 76, 115, 155, 195 and 235: the
  steps between the cube's own grays (0, 95, 135, 175, 215, 255), so together they go from black to
  white in 14 steps of about 20.

Every Rinc shows the palette at 16 bits a pixel, RGB565: each component's top 5, 6 and 5 bits. Where a
display has more bits, a component `v` of `n` bits becomes `v * 255 / (2^n - 1)`, rounded down, so every
Rinc shows exactly the same colors.
