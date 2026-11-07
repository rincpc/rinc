# Files

Files (`system/files/files.rio`, a system app) looks through storage, folder by
folder: the apps (`/rinc/apps`), their projects (`/rinc/projects`, the apps in work, `chord+`), the
apps' saves (`/rinc/save`), their stashes and caches: everything in `/rinc`, the root. It starts there,
or where it was last. A yellow mark at the left is the active app or project, which the editor opens.

| keys | |
|---|---|
| up, down, Page Up, Page Down, Home, End (or the d-pad) | pick one |
| Enter (or A) | open a folder (`..` goes up); edit an app's main file (`chord.rio` in its folder: it becomes the active one, and the editor opens it); show any other file's text |
| Backspace, Alt+Up (or B) | up a folder |
| typing | narrow the list to the names with what's typed |
| Space (or Y, or the secondary button) | what else can be done with it, each by its letter |
| Ctrl+N | a new project: its name, then the editor |
| Ctrl+Shift+N | a new folder, here (inside `/rinc`'s folders) |
| Ctrl+Shift+P (or the system key with P) | every command: these, and what can be done with the one picked, each by name |
| Esc | close what's open, else clear what's typed, else up a folder |

With a mouse, a click picks one, a click on the one picked opens it, and the wheel scrolls.

## Several at once, and moving them

Selecting works as in Windows' Explorer. The arrows (or a click) select the one they pick; with Shift
held, everything from where you started (the anchor) to there; with Ctrl, they move the pick alone,
leaving what's selected as it is, and Ctrl+Space then selects that one too (or not). Ctrl+click adds or
takes away one, Shift+click selects a run, and Ctrl+A selects them all. The one picked has a frame;
what's selected is filled.

| keys | |
|---|---|
| Ctrl+X | cut what's selected (or the one picked): Ctrl+V in another folder moves it there |
| Ctrl+C | copy it: Ctrl+V puts a copy where you paste (in the same folder: `name-2.png`) |
| Ctrl+V | paste here (what's cut or copied waits, even while you go to another app) |
| F2 | rename the one picked |
| Delete | delete what's selected: one after typing its name, several after typing `delete` |
| Esc | with several selected, just the one picked again |

With a controller, X selects the one picked (or not), as Ctrl+Space does, and the left trigger held is
Shift, so the d-pad selects a run; the right trigger is Ctrl.

Renaming, copying as a name and the other prompts edit their text as a text box does: it starts
selected (a file's name, but not its extension), and the arrows, Home and End, Backspace and Delete (with
Ctrl, by words) and Shift to select all work in it. A file's
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
| F | a new folder, here, as Ctrl+Shift+N (which a browser may keep for itself) |

Changes happen only inside `/rinc`'s folders, not to them (`apps`, `save` and the like). Copying and
renaming refuse a name that's taken.

## As a picker

An app can call Files to pick a file: `pc.call("files", "pick")`, or `pc.call("files", "pick
/rinc/save/chord")` to start in a folder. Then Enter on a file gives its path back to that app
(`pc.result`), and Esc gives nothing back. Called with `save` (`pc.call("files", "save
/rinc/projects/chord")`), it picks where to save: what's typed is a name, Enter on a file takes its
name, and Enter gives back that name's path in the folder shown.
