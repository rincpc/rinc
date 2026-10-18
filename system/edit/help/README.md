# Editor

The editor (`system/edit/edit.rio`, built into the Fruit Jam's firmware) edits an app's source: one at a
time, up to 32 KB, with the keys of most editors. It starts with the app it had last, where it was.
Ctrl+R saves and runs the app, and the system key with Tab comes back to the editor.

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

## Writing Rio

Enter keeps the line's indent, and goes a step further in after a line that opens a block (`if`, `for`,
a proc and so on); `end`, `else` and `case` step back out as they're typed. Tab and Backspace move by
whole steps of two spaces in the indent. Rio's keywords, strings, numbers and comments each have a
color of their own.

Undo groups typing by words, and remembers the last 256 changes (or 16 KB of what they changed). When
there's an app with unsaved changes, opening another or starting a new one asks first.
