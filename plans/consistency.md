# Consistency: rinc and its apps, cohesive and obvious

The user (2026-11-24): rinc and every app reviewed so they're consistent, cohesive and easy for people
coming from modern systems: far less opaque, obvious to use, and still efficient from the keyboard.
Nothing here is decided; the user picks from it.

## Known inconsistencies (to grow as the inventory finds more)

- **Arrows.** Up and Down move the cursor in most apps (Edit, Files, Paint, Maps, Sound), but in Music
  they change the value at the cursor, and Ctrl+Up and Ctrl+Down move. One meaning for plain arrows
  everywhere (move), with values changed by something else (+ and -, Alt+arrows, or typing), would match
  the other apps and modern editors.
- **Tab.** Tab does different things in different apps: the next area (Paint), the next field (Music),
  a completion or a snippet's next field (Edit), and Ctrl+Tab the recent files. One meaning (the next
  part of the screen, Shift+Tab the one before, as on the web and in desktop apps) would help.

## The inventory (to do: no changes made in it)

For each app: every system app (home, Files, Edit, the console, log, help, and the rest in system/)
and every studio app (Paint, Maps, Music, Sound, Snippet), first; the demos after, mattering a bit less:
- its keys, by mode: what moves, what changes values, what picks, what confirms, what backs out;
- what it shows: hints, a status line, the command list, help;
- how it does the shared jobs: new, open, save, save as, close (Ctrl+W), the recent files (Ctrl+Tab),
  undo and redo, cut, copy and paste, select, delete, find, confirm, errors, help, leaving.

Then a table, a row per job, a column per app, with each mismatch marked and ranked.

## Directions to weigh

- One set of shared keys everywhere: Ctrl+N/O/S/W/Z/Y/X/C/V/F/P, Ctrl+Tab, Esc backs out, F1 (or
  Ctrl+?) for help, arrows move, Tab goes to the next area.
- The same bottom hint bar in every app: the keys that work in this mode or field, in plain words.
- The command list (Ctrl+P, as in Edit) in every app: searchable, each command with its key beside it, so
  using it teaches the shortcuts.
- Prompts and errors in plain words: what happened, and what to do next.
- A first-run tip per app, and help that opens where it's needed (the field or mode at the cursor).
- What's efficient stays: nothing added between an expert and a key; the help is there, not in the way.
