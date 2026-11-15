# Songs played by the host

Status: plan (2026-11-15), for the user to look over before it's built.

## Why

rinc's `song` library plays a song by writing each channel's notes for the next pattern into the app's
room, at one speed per channel: the gcd of its sounds' speeds and its placements' start times (in
120ths of a second), since the synth plays one speed a voice. Speeds that don't divide the step make
that gcd small: a bass of speed 14 under steps of 100 is 2 ticks a note, and Cloudlamp's first song
needed 20,951 bytes of room (3,694 with speeds chosen to divide). The game also has to call `tick()`
every frame, and a pattern changes when a frame notices the last one ended, so up to a frame late.

## What changes

A sequencer in the audio mixer (src/core/audio.c, so the board, the desktop and the web all have it)
plays the song from tables in the app's own memory, as `pc.soundData` plays samples from it:

- Each placement starts on its channel's voice at its own speed, its sound's notes read as they play,
  so nothing is written ahead: the room is just the tables (patterns, uses, placements), a few
  hundred bytes for Cloudlamp's song. A sound is heard as Sound plays it (its effects, its loop), not
  cut into slices of a common unit (the library's "a note repeated" rules go).
- Time moves in ticks (1/120 s, the synth's unit, as every speed is), counted in the mixer, so a
  placement starts on its tick exactly, and a pattern follows the last without waiting for a frame.
- No `tick()`. A game starts it and leaves it; it stops, pauses and resumes when the game asks.

## The host procs (pc)

```rio
songPlay* :: host proc(v: Int, room: Blob, notes: Blob, from: Int, how: Int) # how: 1 alone (just that use), 2 loop (from its start again at its end)
songStop* :: host proc()                        # its voices silent
songPause* :: host proc(on: Int)                # 1 holds it where it is (its voices silent), 0 goes on
songLevers* :: host proc(shift: Int, volume: Int, speed: Int) # semitones, eighths, percent: at once
songAt* :: host proc() -> [2]Int                # the use playing and its time round ({-1, 0}: none)
```

One song at a time (a game's music), on voices `v` to `v` plus its channels less 1; sounds of the
game's own go on the others, as now. `room` holds the tables as `song` lays them out today (6 bytes a
pattern, 9 a use, 8 a placement, their counts and offsets in a head at its start, documented in
docs/spec/sound.md), and `notes` its sounds, 128 bytes each (`song.SOUND_BYTES`: 32 notes of 4 bytes,
then the speed and loop). The host reads both as it plays, checking every index against their lengths
(Music changes a song as it plays), and stops when the app ends, as its sounds do.

## The sequencer

Per tick, for the use playing (its pattern, its time round): each placement whose step starts now
starts its sound on its channel's voice (as `synth` does, at its speed or its sound's), ending what
played there; a placement's hold, or the pattern's end, stops it. Its pitch shift (the placement's,
plus the use's ramped across all its times round, plus the lever's) is the voice's bend, and its
volume (the sound's notes, times the placement's eighths, times the use's ramped, times the lever's)
the voice's volume, both set each tick. A use's speed and the lever's scale the ticks a step takes
and a note lasts (the synth's speed, as a tick length: a new field in a voice, so pitch isn't
touched). At a pattern's end: its next time round, else the next use (a loop's end back to its start,
a stop mark the end, `how`'s loop from the first use again).

State, in SRAM: the song's views, the use, its time round, the tick in the pattern, the levers, and per
channel the placement playing and its tick left: about 200 bytes. Its code is in flash, but for the
few lines in the mixer that count ticks (about 4 KB of SRAM is free on the board).

## The library and Music

- `song` keeps reading and writing files (a .mus a line at a time, into the tables in room; the sounds
  into notes), `needs` and `roomNeeds` (now just the tables), `sound` (a game's effects), and the
  levers; `play`, `loop`, `stop` call the host; `tick` goes (kept a while as nothing, for old games).
- The one buffer stays (notes first, then the tables), and Music's top right shows its smaller size.
- Music plays through it as it does now, and shows where it's playing from `songAt`.

## Tests

- rinc: tests/song.py's playing checks become the host's: placements started on their steps, at their
  speeds (two sounds of speeds 14 and 12 on one channel, under steps of 100: the room just the tables),
  a hold, the next placement ending one, ramps across times round, loops and stops, pause and resume,
  levers, and a song changed as it plays (indexes past the ends ignored). Rendered with `-wav`.
- studio: Music's tests as they are; its "room" display the tables' size.
- The board: Cloudlamp's song, and Music playing a song while it's edited.

## Steps

1. The sequencer and the host procs, with tests, on the desktop.
2. `song` on them (no `tick`), its notes as before; Music and Cloudlamp moved over.
3. The spec (sound.md: the tables, the procs), Music's help, the library's header.
4. The board: flash and SRAM measured, Cloudlamp's song heard, Music used on it over serial.
