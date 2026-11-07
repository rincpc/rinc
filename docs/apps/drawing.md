# Drawing

The screen is 320 by 240 pixels, `x` from the left and `y` from the top. Each pixel is a color number
from 0 to 255, picked from Rinc's palette: the 16 PICO-8 colors first, then PICO-8's 16 secret colors,
then a band of 16 for each of 12 hues, then 16 tints and 16 grays. The screen keeps what's drawn until something draws over it, so a game usually starts its
`draw` with `pc.cls`.

```rio
pc.cls(0)                        # fill the screen: black
pc.pset({5, 5}, 8)                 # one red pixel
pc.rect({10, 20}, {30, 10}, 12)      # x, y, width, height, color: blue
pc.print("SCORE 12", {0, 40}, 7)   # text, 6 pixels a character
log(pc.pget({5, 5}), pc.pget({15, 25}), pc.pget({400, 0}))
```

```output
8 12 0
```

Drawing off the screen is fine: it's cut off at the edges, and reading a pixel off the screen gives 0.

## The palette

| colors | |
|---|---|
| 0 to 15 | PICO-8's: 0 black, 1 dark blue, 2 dark purple, 3 dark green, 4 brown, 5 dark gray, 6 light gray, 7 white, 8 red, 9 orange, 10 yellow, 11 green, 12 blue, 13 lavender, 14 pink, 15 peach |
| 16 to 31 | PICO-8's secret palette, darker and in-between shades of those (see [the screen](../spec/screen.md#the-palette)) |
| 32 to 223 | a band of 16 for each hue: red, orange, yellow, lime, green, teal, cyan, azure, blue, violet, magenta, crimson. Each is 8 vivid colors, dark to light, then 8 muted ones |
| 224 to 239 | tints: 8 warm grays, then 8 cool ones, dark to light |
| 240 to 255 | grays, black to white |

The Colors app shows them that way, a band to a row, and says what each is.

```rio
RED :: 0      # the hues, in their order
ORANGE :: 1
BLUE :: 8
vivid :: proc(hue, step: Int) -> Int    # step 0, darkest, to 7, lightest
  return 32 + 16 * hue + step
end
muted :: proc(hue, step: Int) -> Int
  return 32 + 16 * hue + 8 + step
end
gray :: proc(step: Int) -> Int          # 0, black, to 15, white
  return 240 + step
end
for x in 0..<8
  pc.rect({x * 20, 0}, {20, 20}, vivid(ORANGE, x))   # dark brown to light orange
end
log(vivid(RED, 0), muted(BLUE, 7), gray(15), pc.rgb(gray(15)))
```

```output
32 175 255 16777215
```

## Pictures as data

Rinc has no sprite format of its own: a picture is just bytes, one color number each, drawn with
`pset`. `hexBlob` writes them in the app:

```rio
heart := hexBlob(
  "00 08 00 08 00
  "08 08 08 08 08
  "00 08 08 08 00
  "00 00 08 00 00"
)
drawPic :: proc(pic: Blob, w, x, y: Int)
  for c, i in pic
    if c != 0                    # 0: see-through
      pc.pset({x + i % w, y + i / w}, c)
    end
  end
end
drawPic(heart, 5, 100, 100)
log(pc.pget({101, 100}), pc.pget({100, 100}))
```

```output
8 1
```

(The 1 is the screen's dark blue, from before anything was drawn there.)

Next: [input](input.md).
