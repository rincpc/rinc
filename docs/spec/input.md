# Input

A Rinc reads its keyboard and controller before each frame, and an app sees the same values for the whole
frame. A Rinc may have no controller, or (in principle) no keyboard; the procs then give 0 or -1.

## Buttons

`pc.btn(i)` is 1 while button `i` is held, and 0 otherwise or for an `i` outside 0 to 15:

| `i` | button | keyboard | controller |
|---|---|---|---|
| 0, 1, 2, 3 | left, right, up, down | arrows | d-pad, or the left stick pushed past half way (but not in pointer mode: see below) |
| 4 | A | V | A |
| 5 | B | C | B |
| 6 | X | X | X |
| 7 | Y | Z | Y |
| 8, 9 | LB, RB | | shoulder buttons (pressing one also types Shift+Tab or Tab: see below) |
| 10, 11 | LS, RS | | sticks pressed in |
| 12, 13 | Start, Back | | |

The keyboard and the controller are both player 1: a button is held if either holds it. The face
buttons are named and placed as on an Xbox controller, in order of importance, so an app that needs
one button uses A.

## Sticks and triggers

`pc.stick(i)` is a stick's position from -1 to 1: `i` 0 is the left stick's x (+ right), 1 its y
(+ down), and 2 and 3 the right stick's. Within 0.15 of the middle it's 0, and beyond that it's rescaled
to reach 1 at the edge, so a resting stick reads 0. `pc.trigger(i)` is a trigger from 0 to 1, `i` 0 the
left one and 1 the right. Without a controller, or for another `i`, they're 0.

## Typed text

`pc.key()` takes the next key typed from a queue, or gives -1 when it's empty, for text and for editing
it:

| key | |
|---|---|
| 32 to 126 | printable ASCII, as typed |
| 8, 9, 13, 27 | Backspace, Tab, Enter, Esc |
| 1 to 26 | Ctrl with a letter: 1 for Ctrl-A, 3 for Ctrl-C and so on (Ctrl-H, Ctrl-I and Ctrl-M are 8, 9 and 13) |
| 128, 129, 130, 131 | left, right, up, down |
| 132, 133, 134 | Home, End, Delete |
| 135, 136 | Page Up, Page Down |

A key that isn't a character (all but printable ASCII and Esc) also says which modifiers were held
with it, as bits added on: 256 Shift, 512 Ctrl, 1024 Alt. Ctrl+Right is 129 + 512, Shift+Tab 9 + 256, and
Ctrl+Shift+Z 26 + 256 (Ctrl+letter already says Ctrl). Enter and Backspace don't take Shift, as it's
easily still held from typing. So `c & 255` is the key without its modifiers, and an app that doesn't
care about them can use that.

A controller's RB types Tab as it's pressed, and LB Shift+Tab, so anything moved through with Tab works
from a controller too. Other keys (function keys, the system key) add nothing. A Rinc drops what's typed beyond 63 keys waiting,
and one fed text faster than that (a serial port, a paste) waits for room. Keys also hold buttons down
while they're held (the arrows, V, C, X and Z), so an app reads them either way.

A terminal's escape sequences (`ESC [ A` for up, `ESC [ 3 ~` for Delete, and so on) become the key codes
above, so a Rinc fed by a serial terminal types the same keys. Modifiers come as xterm sends them:
`ESC [ 1 ; 5 C` is Ctrl+Right, `ESC [ Z` Shift+Tab, and `ESC [ 27 ; m ; k ~` key `k` with modifiers `m`.

## The pointer

A mouse moves a pointer, and Rinc draws it: a small arrow over the screen, which `pc.pointerX()` and
`pc.pointerY()` give the point of, in screen pixels. Moving the mouse or pressing one of its buttons shows
it; typing hides it, as do the arrows (the d-pad's too), a controller's center buttons (Start, Back and
guide) and the system key, and so does the mouse leaving the window, unless a button's held. So it's
there only while it's in use, and a drag goes on outside the window till the button's let go.

Without a mouse, the system key moves it too: with the arrows (or a controller's guide button and
d-pad), a pixel a frame, faster the longer they're held, and V and C (A and B) are its primary and
secondary buttons. That's every Rinc's pointer, in any mode but `none`; the app sees just the pointer. `pc.pointerShown()` is 1 while it's shown.
`pc.pointerDown(i)` is 1 while button `i` is held: 0 the primary (left) one, 1 the secondary (right), 2
the middle. `pc.scroll(i)` is the whole steps scrolled since it was last asked, the rest kept for later:
`i` 0 across (+ right), 1 up and down (+ up, away from you, as on a wheel).

`pc.pointer(mode)` says what the pointer is for, until the app ends, with a `pc.Pointer`:

| mode | |
|---|---|
| `game` | each app's to start: the mouse's only; the left stick is still the arrows |
| `ui` | a pointer app's: a controller's too, its left stick moving the pointer (faster the further it's pushed), its right one scrolling, and A and B its primary and secondary buttons (still `pc.btn` 4 and 5 too) |
| `none` | it's never shown |

The d-pad is the arrows in every mode, and everything a pointer does should have keys too, since not
every Rinc has a mouse or a controller. A list that a click picks from, and the arrows too:

```rio
items := [3]String{"Play", "Options", "Quit"}
picked := 0
clicked := 0
pc.pointer(pc.Pointer.ui)
update* :: proc()
  k := pc.key()
  for k >= 0
    switch
    case k == 130 # up
      picked = (picked + 2) % 3
    case k == 131 # down
      picked = (picked + 1) % 3
    end
    k = pc.key()
  end
  down := pc.pointerDown(0)
  y := pc.pointerY()
  if down == 1 and clicked == 0 and y >= 100 and y < 100 + 3 * 10
    picked = (y - 100) / 10
  end
  clicked = down
end
draw* :: proc()
  pc.cls(1)
  for i in 0..<3
    c := 6
    if i == picked
      c = 7
    end
    pc.print(items[i], 140, 100 + i * 10, c)
  end
end
```

## Time

`pc.time()` is the seconds since the Rinc started, as a `Float`.
