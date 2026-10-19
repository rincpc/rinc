# Editor

The editor (`system/edit/edit.rio`, built into the Fruit Jam's firmware) edits an app's source: one at a
time, up to 32 KB, with the keys of most editors. It starts with the app it had last, where it was.
Ctrl+R saves and runs the app, and the system key with Tab comes back to the editor.

It saves to projects: `chord*` is app `chord` in work, in `/rinc/projects`, with saves of its own (a
copy of the app's), so trying things can't spoil the app's. A new app is a project. An installed app
opens read only, for reading its source; saving it makes its project, and Ctrl+R on it, unchanged,
runs the app. The console's `install chord` makes project `chord*` the app.

What it has open is the active app or project, the whole Rinc's setting, and it starts with what's
active: another app can pick one for it.

## Keys

| keys | |
|---|---|
| arrows, Home, End, Page Up, Page Down | move: with Ctrl, by words and to either end; with Shift, select |
| Ctrl+O, Ctrl+S, Ctrl+R, Ctrl+N | open an app (from a list, or a new name), save it, save and run it, start a new one |
| Ctrl+Z, Ctrl+Y (or Ctrl+Shift+Z) | undo, redo |
| Ctrl+X, Ctrl+C, Ctrl+V | cut, copy, paste (the whole line, with nothing selected) |
| Ctrl+A, Ctrl+L | select all, select the line (and the next, again) |
| Tab, Shift+Tab | indent, outdent (the selected lines) |
| Alt+Up, Alt+Down, Ctrl+Shift+K | move the lines up or down, delete them |
| Ctrl+Backspace, Ctrl+Delete | delete a word |
| Ctrl+F, Ctrl+G | find (Enter finds the next), go to a line |
| Esc | let go of the selection, or close the bar at the bottom |

With a mouse, a click puts the cursor there and dragging selects (past the top or bottom, scrolling),
and the wheel scrolls. In the list of apps that Ctrl+O shows, a click opens one, and a click outside
the list closes it. A controller's left stick moves the pointer, A clicks, and the right stick scrolls.

## Writing Rio

Enter keeps the line's indent, and goes a step further in after a line that opens a block (`if`, `for`,
a proc and so on); `end`, `else` and `case` step back out as they're typed. Tab and Backspace move by
whole steps of two spaces in the indent. Rio's keywords, strings, numbers and comments each have a
color of their own.

Undo groups typing by words, and remembers the last 256 changes (or 16 KB of what they changed). When
there's an app with unsaved changes, opening another or starting a new one asks first.
