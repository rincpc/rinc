# Colors

Colors (`system/colors/colors.rio`) shows all of Rinc's 256 colors, to look through and to pick from:
in their numbers' order, 16 a row, each row a band of them.

- **PICO-8's 32** at the top: 0 to 15, and under each its partner from 16 to 31.
- **A row for each hue**, red round to crimson: red, orange, yellow, lime, green, teal, cyan, azure,
  blue, violet, magenta and crimson. Each has 8 vivid colors, dark to light, then 8 muted ones.
- **The tints:** 8 warm grays, then 8 cool ones, dark to light.
- **The grays:** 16, dark to light.

| keys | |
|---|---|
| arrows (or the d-pad) | move over them: across a band, and up and down between bands, at the same step |
| Tab (or Y, or the secondary button) | between a band's halves: vivid and muted, warm and cool, at the same step |
| Page Up, Page Down, the wheel | up and down, a few bands at a time |
| Enter (or A) | pick it, when an app called Colors for one |
| Esc (or B) | back with nothing, when called |

With a mouse, a click picks a color, and a drag goes through them.

Beside them, the color picked, big, and what it is: its number, its RGB, its place in its band (PICO-8's
number, for one of those), and its OKLCH: L, lightness, and C, chroma (how colorful), out of 100, and h,
its hue in degrees (for all but a gray). OKLCH is a color space made so equal steps look equal, and the
palette's bands are made in it.

## As a picker

An app can call Colors to pick a color: `pc.call("colors", "pick")`, or `pc.call("colors", "pick
12")` to start at color 12. Enter, or a click on the color already picked (or on the big one), gives its
number back as text; Esc gives nothing, an empty result:

```rio fragment
out: Blob[..8]
n := pc.result(out)
if n < 0
  pc.call("colors", "pick")     # (the first time)
case n > 0
  log("picked", out.asString())
end
```
