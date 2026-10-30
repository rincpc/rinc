# Editor

The editor (`system/edit/edit.rio`, a system app) edits a file at a time, by
path (an app's source, its other files, its help, its saves), up to 32 KB, with the keys of most
editors. Ctrl+O opens one through Files, and Ctrl+Shift+S saves as another, through Files too, which
asks where. Ctrl+R saves it and runs the app or project it's in, and the system key with Tab comes back
to the editor. If the app stopped with an error, a compile error or a runtime one, the editor comes
back at it: in the file it was in (one of the app's parts, say), the cursor on it, and what was wrong
at the bottom. Ctrl+Shift+P lists every command, with its keys.

Projects are where apps are worked on: `chord+` is app `chord` in work, in `/rinc/projects`, with saves
of its own (a copy of the app's), so trying things can't spoil the app's. An installed app's files are
read only, for reading; saving one saves it in the app's project, made from the app first, and Ctrl+R
on it, unchanged, runs the app. Files (or the console's `install chord`) installs project `chord+` as
the app. Anything outside `/rinc` is read only too.

The app or project of the file it has open is the active one, the whole Rinc's setting, and it starts
with what's active (another app can pick one for it), else the file it had last, where it was.

## Keys

| keys | |
|---|---|
| arrows, Home, End, Page Up, Page Down | move: with Ctrl, by words and to either end; with Shift, select |
| Ctrl+O, Ctrl+S, Ctrl+Shift+S | open a file (Files picks it), save it, save it as another (Files picks where) |
| Ctrl+R, Ctrl+N | save and run its app or project, start a new file |
| Ctrl+Shift+P | every command, to type to narrow and pick one |
| Ctrl+Z, Ctrl+Y (or Ctrl+Shift+Z) | undo, redo |
| Ctrl+X, Ctrl+C, Ctrl+V | cut, copy, paste (the whole line, with nothing selected) |
| Ctrl+A, Ctrl+L | select all, select the line (and the next, again) |
| Tab, Shift+Tab | indent, outdent (the selected lines) |
| Alt+Up, Alt+Down, Ctrl+Shift+K | move the lines up or down, delete them |
| Ctrl+Backspace, Ctrl+Delete | delete a word |
| Ctrl+F, Ctrl+G | find (Enter finds the next), go to a line |
| Esc | let go of the selection, or close the bar at the bottom |

With a mouse, a click puts the cursor there and dragging selects (past the top or bottom, scrolling),
and the wheel scrolls (sideways too, tilted, or on a trackpad, as far as the longest line in view). In
the list of apps that Ctrl+O shows, a click opens one, and a click outside the list closes it. A
controller's left stick moves the pointer, A clicks, and the right stick scrolls, up and down or
sideways.

## Writing Rio

Enter keeps the line's indent, and goes a step further in after a line that opens a block (`if`, `for`,
a proc and so on); `end`, `else` and `case` step back out as they're typed. Tab and Backspace move by
whole steps of two spaces in the indent. Rio's keywords, strings, numbers and comments each have a
color of their own.

Undo groups typing by words, and remembers the last 256 changes (or 16 KB of what they changed). When
there's an app with unsaved changes, opening another or starting a new one asks first.
