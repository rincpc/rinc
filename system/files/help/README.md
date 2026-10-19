# Files

Files (`system/files/files.rio`, built into the Fruit Jam's firmware) looks through storage, folder by
folder: the apps (`/rinc/apps`), their projects (`/rinc/projects`, the apps in work, `chord*`), the
apps' saves (`/rinc/save`), their stashes and caches, and on the Fruit Jam the rest of the card too.
It starts in `/rinc`, or where it was last. A yellow mark at the left is the active app or project,
which the editor opens.

| keys | |
|---|---|
| up, down, Page Up, Page Down, Home, End (or the d-pad) | pick one |
| Enter (or A) | open a folder (`..` goes up); edit an app's main file (`chord.rio` in its folder: it becomes the active one, and the editor opens it); show any other file's text |
| Backspace, Alt+Up (or B) | up a folder |
| typing | narrow the list to the names with what's typed |
| Space (or Y, or the secondary button) | what else can be done with it, each by its letter |
| Ctrl+N | a new project: its name, then the editor |
| Esc | close what's open, else clear what's typed, else up a folder |

With a mouse, a click picks one, a click on the one picked opens it, and the wheel scrolls. A file's
text shows its first 16 KB, bytes that aren't text as dots; Esc goes back.

## What's done with one

| letter | |
|---|---|
| O, V | open a folder, view a file |
| E, R | edit, run: an app or project (its folder, or its main file) |
| C | copy it, as a name typed (its name to start with, selected: typing takes its place). An app or a project copies as a new project, its main file renamed to match, its own saves left out |
| N | rename it: a project, or anything inside an app's or project's folder, or the saves |
| D | delete it, after typing its whole name: all of it, for good (a project's own saves too; an app's saves, in `/rinc/save`, stay) |
| I | install a project as its app |

Changes happen only inside `/rinc`, and not to its own folders (`apps`, `save` and the like): looking
goes everywhere. Copying and renaming refuse a name that's taken.

## As a picker

An app can call Files to pick a file: `pc.call("files", "pick")`, or `pc.call("files", "pick
/rinc/save/chord")` to start in a folder. Then Enter on a file gives its path back to that app
(`pc.result`), and Esc at the top gives nothing back.
