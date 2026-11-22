# `/` as Rinc's root

The user (2026-11-21): inside Rinc, paths start at `/`, not `/rinc/`: `/projects/chord/...`, `/save/...`.
Storage keeps its own place for them (`/rinc/` on the card, the data folder on the desktop, the storage
name in a page), so a card stays tidy beside other things on it; only the prefix apps see goes.

## Devices beside Rinc's folders

Recommended (the user decides): devices stay at the top, by short reserved names, with no `/devices/`
level, which would bring back the prefix this removes. The top is already a fixed set of Rinc's own names
(apps, projects, save, stash, cache, help, lib, settings, boot and clipboard files); `stick` joins them,
and later ones get theirs (`/stick2`, `/host` for a desktop's folder, say). Files marks a device as one
(its own color or mark), so it's told apart from Rinc's folders without a longer path. No app makes a
folder at the top today, so reserving more names there costs nothing.

The user's concern (2026-11-21): the top is all names Rinc specifies; arbitrary names belong under
them (in apps, projects...), so devices must not make the top an open namespace. So: each device's
name is one Rinc specifies (stick now), never one a device brings (a drive's label), and a new kind of
device gets its name the way apps and projects got theirs, by a decision. The top-level files want the
same tidying: boot (the app started on boot) into settings, and the clipboard a folder of its own
(its kept entries as files), so the top is folders, each specified. To decide with this change.

## The change

- The core: an app's path `/x/...` is storage's `/rinc/x/...` (rinc_argPath), `/stick/...` the stick's;
  `/` lists Rinc's folders and any device. The platforms' storage is unchanged.
- Every `"/rinc/..."` in system apps, studio, the library, tests and docs: mechanical (about 50 in the apps,
  rinc's and studio's, and 11 docs, counted 2026-11-21), in one change, with no `/rinc/` alias kept (before 1.0, as Rio's changes are).
- Files: its root is `/` (no going up past it); `/rinc` checks become "inside one of Rinc's folders".
- Tests: an app's file at `/save/x/y` is the card's `/rinc/save/x/y`; `/rinc/...` from an app is an error.

Order: after writing to the stick (plans/drives.md, 4), as the stick's paths settle first.

Done (2026-11-22): rinc_argPath maps an app's /x to storage's /rinc/x (/stick as it is, / listing Rinc's
folders and the stick); paths handed back to apps (pc.appError, pc.checkError, the log's messages) as
the app has them (rinc_shown), and taken in (pc.checkRio) as storage has them. Every "/rinc/..." in
rinc's and studio's apps, the library, tests and docs moved (a folder named rinc inside a path kept:
/stash/rinc/clip); Files' root is /, Edit's read-only is an installed app's files. tests/stick.py:
/rinc/... from an app is gone. On the user's card: the system and studio apps put again; their text
files with old paths rewritten (studio's last files, Edit's recents, Files' place, console history,
clipboard entries, two saves); two binary undo stashes dropped; a stray /rinc/rinc (an old console's
write) removed; Files and Maps tried there. The top-level files (boot, clipboard) not moved yet.
