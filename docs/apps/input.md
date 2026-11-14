# Input

## Buttons

`pc.btn(b)` is 1 while button `b` (`.left`, `.a` and the like: a `pc.Button`) is held, else 0; `pc.btnp(b)` is 1 as it's pressed, and again while it's held, as PICO-8's. The arrow keys and the first four face buttons work
everywhere, on the keyboard or on a controller's d-pad and buttons:

| `b` | button | key |
|---|---|---|
| `.left` `.right` `.up` `.down` | the arrows | the arrows |
| `.a` | A | V |
| `.b` | B | C |
| `.x` | X | X |
| `.y` | Y | Z |
| `.lb` `.rb` `.ls` `.rs` `.start` `.back` | a controller's LB, RB, left stick pressed, right stick pressed, Start, Back | |

```rio
x := 160
update* :: proc()
  x += pc.btn(.right) - pc.btn(.left)     # right minus left
  if pc.btn(.a) == 1
    log("A")
  end
end
draw* :: proc()
  pc.cls(0)
  pc.rect({x, 200}, {16, 8}, 7)
end
```

## Sticks and triggers

`pc.stick(i)` reads a controller's sticks, from -1 to 1 (`i`: 0 left stick x, 1 its y, 2 and 3 the right
stick's; down is +), and `pc.trigger(i)` its triggers, from 0 to 1 (0 left, 1 right). Without a
controller they're 0. The left stick pushed well over also presses the arrow buttons.

## Typing

`pc.key()` gives the next character typed, or -1 when there's none: for names, chat or a typed-in
command. It's the printable ASCII characters, 13 for Enter, 8 for Backspace and 27 for Esc. Keys held
down still press buttons too, so an app can use either.

```rio
name: Blob[..16]
update* :: proc()
  for
    c := pc.key()
    if c < 0
      break
    end
    if c == 8 and name.len() > 0
      _ = name.pop()
    case c >= 32
      _ = name.push(c)
    end
  end
end
draw* :: proc()
  pc.cls(1)
  pc.print(name[].asString(), {10, 10}, 7)
end
```

## Time

`pc.time()` is seconds since Rinc started, as a `Float`. Frames come 60 times a second, so counting
frames also measures time.

Next: [sound](sound.md).
