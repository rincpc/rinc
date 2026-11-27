# Fewer host procs: pc redesigned smaller

The user (2026-11-24): with host procs near rinc's cap, see how many `pc` can lose with some redesign.
Nothing here is decided; the user picks. All of it breaks apps, so it belongs before 1.0 (ROADMAP's
"Decide first").

## Where it stands

- `pc` declares 120 host procs (`src/core/pc.rio`), plus one of its own (`imageLoad`, imageRead's).
- Rinc's cap is `RINC_MAX_FFI` in `CMakeLists.txt` (and `tools/web.py`): 122 since the transforms (it
  was 120). Each slot is 16 bytes in each of rinc's two VMs (the app's and the console's nest), and the
  board's RAM is short: the transforms' first version overflowed it by 1,580 bytes.
- The cap is per VM, so registering some procs only for system apps doesn't help: a system app's VM
  still needs them all.
- The user's direction (2026-11-24): more optional parameters (a `how?`, as `Shape`, `Blit` and `Text`
  are) rather than more procs, as calls are added.

## Clear wins (about 26): smaller and simpler

| Group | Now | Redesigned | Saves |
|---|---|---|---|
| Storage | `saveRead`, `saveWrite`, `saveDelete`; `stashRead`, `stashWrite`, `stashReadAt`, `stashWriteAt`, `stashDelete`; `cacheRead`, `cacheWrite`, `cacheDelete`; `settingRead`, `settingWrite` | The file calls (`fileRead`, `fileWrite`, `fileReadAt`, `fileWriteAt`, `fileDelete`) on app-relative paths: `stash/key`, `save/name`, `cache/name` (apps already reach `/stash/rinc/clip` with them); settings likewise, or a path of their own | 11 to 13 |
| Pointer | `pointerAt`, `pointerX`, `pointerY`, `pointerDown`, `pointerShown`, `scroll` | One `pointerState()` giving a struct: where, the buttons down, shown, scrolled. `pointer(mode)` stays | 5 |
| App files | `appSource`, `appSave`, `appDelete`, `appCopy`, `appRename` | The file calls under `/apps/...`, the host dropping a changed app's compiled image as it does now. `appInstall` stays | 4 |
| App list | `apps`, `appName(i)`, `appInfo(i)` | One `apps(out)`: a line each, its name and its info | 2 |
| Help | `helps`, `helpRead`, `helpTopic`, `help` | `dir` and `fileRead` on help's folder. `help(topic)` stays | 2 |

Storage first, recommended: the largest cut, it makes reaching files one API, and it fits apps being
islands (paths relative to the app).

## Reasonable merges (about 9)

- `transformBy` into `transform(m, by?)`: 1.
- `triangle` into `polygon` (its fast path kept for three points): 1.
- `curve` into a path of one part: 1, only if its no-room path stays as fast.
- `stick` and `trigger` into one call giving a struct: 1.
- `screenSize` given back by `screen`: 1.
- `appError` and `checkError` as one, with a flag: 1.
- `imageFlags`, `imageSpriteFlags` and `imageFlag` as one: 2.
- `appTimeReset` into `appTime(reset?)`: 1.

## Debatable (about 9): the sound setters

- `soundRate`, `soundVolume`, `soundPan`, `soundLoop`, `soundSeek` as one change call taking a
  settings struct, with flags for which fields it sets: 4 to 5.
- `synthRate`, `synthBend`, `synthEnvelope`, `synthSmooth` likewise: about 3.
- The song's controls (`songStop`, `songPause`, `songLevers`): 1 or 2.

As fast, but harder to discover than a call each.

## Probably not

- The Rio tools (`rioStart`, `rioEval`, `rioFrame`, `rioType`, `rioError`, `rioLog`, `rioIndent`,
  `checkRio`, `completeRio`, `formatRio`, `moduleSource`: 11): only Edit and the console use them, but
  the cap is per VM, so a system app's still needs them all.
- `fromHex`, as plain Rio: 1, if its speed doesn't matter.

## In all

About 40 of 121 could go, leaving about 80: room for what's coming (gradients, clip, colour tables,
the UI library's needs) without raising the cap, and RAM back on the board.

## Notes for doing it

- A struct given back (the pointer's state) costs no more than one Int: one call, not several; and a
  struct can gain fields at its end later without breaking an app (Rio's host spec says so).
- Each change: pc.rio, pc_host.h made again (`rinc -H src/core/pc_host.h src/core/pc.rio`), rinc.c's
  registrations, the system and studio apps and the demos that call the old procs, docs/spec, and tests
  (the old behaviour through the new call).
- Lower `RINC_MAX_FFI` afterwards to what's used plus a little room, for the board's RAM.
