# Colors

Colors (`system/colors/colors.rio`) shows all of Rinc's 256 colors, to look through and to pick from:

- **PICO-8's 32** at the top, in two rows: 0 to 15, and under each its partner from 16 to 31.
- **Every gray**, dark to light: the cube's six and the eight grays of their own.
- **The 6x6x6 color cube:** a face of it, big; the column through it at the face's color, at its
  right; and all six of its slices, small, under the color picked.

| keys | |
|---|---|
| arrows (or the d-pad) | move over the colors, up and down between the rows and the cube |
| Tab, Shift+Tab (or RB, LB), Page Down, Page Up, the wheel | deeper into the cube, and back out |
| Space (or Y, or the secondary button) | turn the cube, to face another way: its depth red, then green, then blue |
| Enter (or A) | pick it, when an app called Colors for one |
| Esc (or B) | back with nothing, when called |

With a mouse, a click picks a color, and a drag goes through them: across the face, down the column,
over the slices.

## As a picker

An app can call Colors to pick a color: `pc.call("colors", "pick")`, or `pc.call("colors", "pick
12")` to start at color 12. Enter, or a click on the color already picked (or on the big one), gives its
number back as text; Esc gives nothing, an empty result:

```rio fragment
out: Blob[8]
n := pc.result(out[:])
if n < 0
  pc.call("colors", "pick")     # (the first time)
case n > 0
  log("picked", out[0:n].asString())
end
```
