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
| Ctrl+Shift+J | join the next line onto this one (or the selected lines) |
| Ctrl+Space | complete the word (after a module's dot, the list opens by itself) |
| Ctrl+/ | comment the lines (the selected ones, or the cursor's), or uncomment them |
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
a proc and so on); `end`, `else` and `case` step back out as they're typed. In a comment line, Enter
goes on with the comment (its `#` again), and on an empty one, ends it. Tab and Backspace move by
whole steps of two spaces in the indent. Rio's keywords, strings, numbers and comments each have a
color of their own.

Ctrl+Space offers what could finish the word at the cursor: the file's own words (the nearest
first), core's names and Rio's words; with just one, it's put in. After a module's name and a dot
(`pc.`, `core.`, or a module the file or its app's main file imports), the list opens by itself with
the module's names, and what each one is (a proc's parameters, a field's type): `pc.Rect.` offers
Rect's methods and fields. After another dot, Ctrl+Space offers the file's words and core's methods
(`len`, `push`). Typing narrows the list; Up and Down (and Page Up and Down) pick, Tab or Enter puts
the pick in, a click puts one in, and Esc or any other key closes it.

Saving a `.rio` file lays it out the one way, as `rio fmt` does: only spaces and line breaks change
(and a trailing comma may be added), and the cursor stays on the same code, where it was on the
screen. A list in brackets goes on one line, unless it ends with a trailing comma (then an item a
line) or has a line break after its opening bracket (then its items on a line of their own); add the
comma to put it over lines, and take it off, with Ctrl+Shift+J on the opening bracket's line, to
bring it back. Code between `# nofmt` and `# /nofmt` comment lines is left as it is. Ctrl+Z undoes
the layout.

Undo groups typing by words, and remembers the last 256 changes (or 16 KB of what they changed). When
there's an app with unsaved changes, opening another or starting a new one asks first.
