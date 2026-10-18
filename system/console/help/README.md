# The console

The console (`system/console/console.rio`, built into the Fruit Jam's firmware) runs Rio as it's typed, a line
or a block at a time, in a session of its own: try things out, and look at what apps do. On the Fruit
Jam, Ctrl+C over the USB serial port opens it from anywhere, which is how `tools/send.py` sends apps.

## Typing Rio

What's typed is Rio: a statement runs when it's complete, and a block (a proc, an `if`, a loop) when
its `end` comes. A value on its own is shown. Up and down bring back lines typed before.

```text
> x := 6 * 7
> sq :: proc(n: Int) -> Int
.   return n * n
. end
> sq(x)
1764
```

## Commands

A line on its own can be a command instead:

| command | |
|---|---|
| `run` | run what's been typed as a game (its `update*` and `draw*`), until Esc |
| `ls` | list the apps |
| `load name` | app `name`'s source, into a fresh session |
| `run name` | run app `name`, as itself |
| `save name` | what's typed or pasted next, until Ctrl-D, becomes app `name`'s source (Ctrl-C gives up) |
| `delete name` | remove app `name` |
| `home` | back home |
