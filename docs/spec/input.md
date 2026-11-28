# Input

A Rinc reads its keyboard and controller before each frame, and an app sees the same values for the whole
frame. A Rinc may have no controller, or (in principle) no keyboard; the procs then give 0 or -1.

## Buttons

`pc.btn(b)` is 1 while button `b` (a `pc.Button`) is held, and 0 otherwise:

| `b` | button | keyboard | controller |
|---|---|---|---|
| `.left`, `.right`, `.up`, `.down` | the arrows | arrows | d-pad, or the left stick pushed past half way (but not in pointer mode: see below) |
| `.a` | A | V | A |
| `.b` | B | C | B |
| `.x` | X | X | X |
| `.y` | Y | Z | Y |
| `.lb`, `.rb` | LB, RB | | shoulder buttons (pressing one also types Shift+Tab or Tab: see below) |
| `.ls`, `.rs` | LS, RS | | sticks pressed in |
| `.start`, `.back` | Start, Back | | |

`pc.btnp(b)` is 1 in the frame `b` is pressed, and again while it's held: after 15 frames, then each
4th, as PICO-8's `btnp` (a menu's step, held). `pc.dpad()` is the arrows held, a `[2]Int` of -1, 0 or 1
each way (`{1, 0}`: right); `pc.dpadp()` is the arrows as `btnp` has them, one way at a time (left or
right before up or down), for moving a step on a grid. `pc.keyButton(k)` is the button an arrow key
from `pc.key()` is (128 to 131), so `pc.btn(pc.keyButton(k)) == 0` says it was typed (over serial,
say), not held.

The keyboard and the controller are both player 1: a button is held if either holds it. The face
buttons are named and placed as on an Xbox controller, in order of importance, so an app that needs
one button uses A.

Over serial, where a terminal sends keys only as they're typed, a developer's tool can hold buttons
the way kitty's keyboard protocol says when keys go down and up: `ESC [ 1 ; 1 : 1 C` presses right,
`ESC [ 1 ; 1 : 3 C` lets it go (A, B, D: up, down, left), and `ESC [ 118 ; 1 : 1 u` and `: 3 u` V, the A
button (`99` C, `120` X, `122` Z). A press is typed too, a release isn't; a button still held when
another app starts is let go. So a test can play a game on a board.

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
| 140 to 151 | F1 to F12 (where the Rinc gets them: a browser keeps F5, F11 and F12 for itself too, and the desktop's F11 is its whole display) |
| 4096 and up | one of the app's own commands, picked from the command list (see [its commands](apps.md#its-commands)) |

A key that isn't a character (all but printable ASCII and Esc) also says which modifiers were held
with it, as bits added on: 256 Shift, 512 Ctrl, 1024 Alt. Ctrl+Right is 129 + 512, Shift+Tab 9 + 256, and
Ctrl+Shift+Z 26 + 256 (Ctrl+letter already says Ctrl). Enter and Backspace don't take Shift, as it's
easily still held from typing. So `c & 255` is the key without its modifiers, and an app that doesn't
care about them can use that.

`pc.modsHeld() -> Int` says which modifiers are held now, with the same bits: 512 while Ctrl is down,
say, or locked (tapped twice). It's for what goes on while one's held, as the editor's Ctrl+Tab walks back through its files
until Ctrl is let go, or the arrows held with Shift selecting. Where Rinc can't tell (in a terminal),
it's -1. In a pointer app (`pc.Pointer.ui`, below), a controller's left trigger, held, counts as Shift
and its right one as Ctrl, so its d-pad selects as the arrows do (a game's triggers stay its own:
`pc.trigger`).

`pc.keyHeld(k) -> Int` is 1 while key `k` is held down, and 0 otherwise: `k` as `pc.key()` gives it, without
the modifiers (a letter in either case, 32 Space, 13 Enter, 8 Backspace, 134 Delete, 128 to 131 the arrows,
the rest as the table says), its key as the keyboard's layout has it. It's for what goes on while a key's
held in a pointer app, whose keys type rather than hold buttons: Maps and Paint draw while Space is held
and the arrows move. Where Rinc can't tell (in a terminal) it's -1, and while a system combination's held,
0. `rinc -hold` holds keys too, for tests: `space`, `enter`, `delete` and the other editing keys by name,
`keyq` for Q, and the arrows' names.

A controller's RB types Tab as it's pressed, and LB Shift+Tab, so anything moved through with Tab works
from a controller too. Caps Lock types letters as capitals (with Shift, small), on every Rinc; other keys (the system key and the like) add nothing. A Rinc drops what's typed beyond 63 keys waiting,
and one fed text faster than that (a serial port, a paste) waits for room. Keys also hold buttons down
while they're held (the arrows, V, C, X and Z), so an app reads them either way: in a game. In a pointer
app (`pc.Pointer.ui`, below) the keyboard types, and only its arrows are buttons too; there a
controller's A and B, with the pointer hidden, type Enter and Esc, so one works such an app through the
keys it reads.

## A line of text, edited

`pc.Field` edits a line of text as a text box does (in Windows, say): a name to give, something to
find. It holds the text, in room of the app's own, the cursor, and the selection's other end:

```rio fragment
room: Blob[..32]
name := pc.Field{text = room}
name.set("tune.png")       # its text, all of it selected
name.select(0, 4)          # just "tune": typing takes its place
c := pc.key()
if c >= 0 and not name.key(c) # (Enter and Esc aren't its: the app's)
  log("the app's key", c)
end
name.draw({10, 100}, 30, 7, 2, 10) # 30 characters across, scrolled to show the cursor: text, selection and cursor colors
```

| key | |
|---|---|
| typing | in place of what's selected, while there's room |
| Backspace, Delete | the character before or after, or what's selected; with Ctrl, a word |
| left, right | a character, or with Ctrl a word; with Shift, selecting; with nothing held, out of a selection to that side |
| Home, End | its start, its end (with Shift, selecting) |
| Ctrl+A | all of it |

`key(c)` says whether the key was the field's; the rest (Enter, Esc, Tab, up and down) are the app's.
`set(s)` gives it new text, all selected; `select(from, to)` selects from `from` to `to`, the cursor at
`to`; `selected()` says whether anything is. Its `text` is the app's own room, so the app reads it
there: `name.text.asString()`.

A terminal's escape sequences (`ESC [ A` for up, `ESC [ 3 ~` for Delete, and so on) become the key codes
above, so a Rinc fed by a serial terminal types the same keys. Modifiers come as xterm sends them:
`ESC [ 1 ; 5 C` is Ctrl+Right, `ESC [ Z` Shift+Tab, and `ESC [ 27 ; m ; k ~` key `k` with modifiers `m`.

## The pointer

A mouse moves a pointer, and Rinc draws it: a small arrow over the screen. Moving the mouse or pressing one of its buttons shows
it; typing hides it, as do the arrows (the d-pad's too), a controller's center buttons (Start, Back and
guide) and the system key, and so does the mouse leaving the window, unless a button's held. So it's
there only while it's in use, and a drag goes on outside the window till the button's let go.

Without a mouse, the system key moves it too: with the arrows (or a controller's guide button and
d-pad), a pixel a frame, faster the longer they're held, and V and C (A and B) are its primary and
secondary buttons. That's every Rinc's pointer, in any mode but `none`; the app sees just the pointer.

`pc.pointerState()` gives all of it, as it is this frame (asking again in the same frame gives the
same), as a `pc.PointerState`:

| field | |
|---|---|
| `at: [2]Int` | where it points, in screen pixels |
| `primary: Bool`, `secondary: Bool`, `middle: Bool` | its buttons held: the left one, the right one, the middle |
| `shown: Bool` | it's drawn now |
| `scrolled: [2]Int` | the whole steps scrolled this frame: across (+ right), and up and down (+ up, away from you, as on a wheel); a trackpad's fractions of a step wait for the frames after |

`pc.pointer(mode)` says what the pointer is for, until the app ends, with a `pc.Pointer`:

| mode | |
|---|---|
| `game` | each app's to start: the mouse's only; the left stick is still the arrows |
| `ui` | a pointer app's: a controller's too, its left stick moving the pointer (faster the further it's pushed), its right one scrolling, and A and B its primary and secondary buttons while it's shown (still `pc.btn` 4 and 5 too; hidden, they type Enter and Esc, and don't show it). The keyboard types: V, C, X and Z aren't buttons here |
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
  p := pc.pointerState()
  down := p.primary.toInt()
  y := p.at.y
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
    pc.print(items[i], {140, 100 + i * 10}, c)
  end
end
```

## Time

`pc.appTime()` is the seconds since the app started (each start: an app called back to starts again),
as a `Float`, and `pc.appTimeReset()` starts it from 0 again, so the microseconds a `Float` keeps are
there for as long as an app runs, not lost as a board stays on for days. On the desktop and the web, it's
the board's clock (board speed: what the app's work would have taken on the Fruit Jam), unless `-turbo`.
