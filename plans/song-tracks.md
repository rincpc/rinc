# A song's tracks, its voices shifting, and a free voice

Status: started (2026-11-23), the user's go: tracks with voices shifting automatically; a free voice for
a game's own sounds. Pan comes next, in the same file version.

## What

A song's uses in tracks played side by side, not only one after another: a drum beat in a track of its
own, kept going under the melody's patterns and changed on its own. Old songs are one track, and play as
they did.

## The song's room: unchanged in layout

- A use's marks byte (its 8th) has room: its low bits stay the marks (1 a loop begins, 2 a loop ends, 4
  the song stops); bits 4 to 6 are its track (0 to 7). A track's uses are those of its number, in the
  table's order. So the room's layout, `pc.songPlay`'s arguments and every old song stay as they are.
- `how` keeps 1 (just that use: its track alone) and 2 (round again at its end), and from bit 8 on the
  voices the song may take (0: 8, as before).

## Voices, shifting

Each track's pattern playing uses some channels (those its placements name). The tracks take voices in
order: track 0's channels the first, then track 1's, and so on, each track's in its channels' order,
from the song's first voice; past the song's voices, a channel's notes are dropped (silent). Worked out
again each time a track's pattern changes; a note playing on a voice that moves ends there. A song of
one track whose pattern uses channels 0 and 4 now takes two voices, not five.

Recommended (the user agreed): shift, not "the first to take a voice keeps it", so a song written for
fewer voices plays as written, the first track's never dropped. Music shows where notes drop (later).

## Time

Each track plays its uses as the one row did: its times round, its loops (a loop's begin and end in the
same track), and its end. A stop mark in any track ends the song. The song ends when every track has;
`how` 2 starts every track again from its first use then, together. Playing from a use of one track
starts the others where they'd be at that time, counted straight along their uses (loops not followed).
`songAt` reports the track of the use it started from.

## A free voice

`pc.freeVoice() -> Int`: the first voice that isn't playing and isn't the song's (-1: none). So a game's
effects needn't keep voices apart from the song's: `v := pc.freeVoice()`, then `pc.synth(v, ...)`. The
song library's `sound(-1, n)` takes one this way.

## The file

`rinc music 3` when a song has more than one track: after `song` (track 1's uses), `track 2`, its uses,
and so on (`track N`, then its use lines, as now). One track: still written as `rinc music 2`, so older
Rincs read it. Both read.

## Asked (the user, 2026-11-23): sounds for each pattern, patterns from other songs

- **A sound bank for each pattern:** small in the file (a pattern's line names its file of sounds, or
  none: the song's), and the room has a spare byte in each placement for which bank. The cost is
  memory: each bank's notes are up to 4 KB (32 sounds of 128 bytes), read in as the song loads, and
  Music would show which bank a pattern uses.
- **Patterns from another song:** a pattern's line saying `from other.mus name`, read from that file as
  the song loads (with that song's sounds, so this needs banks too). Loading follows the reference (a
  missing file or pattern, and cycles, refused, the log saying so); editing it in Music edits the other
  song, or copies it in. Copying patterns between songs is there already: Ctrl+C on uses in Music puts
  them and their patterns on the clipboard every app shares, and Ctrl+V in another song pastes them
  (their sounds by number, so the songs should share a file of sounds).
- Recommended: banks first, then references, with maps' regions and sheets in one design (studio's
  ROADMAP: libraries).
- Easy, and the RAM the user's choice (the user, 2026-11-23: banks matter): by default a song has one
  file of sounds, as now, and costs nothing more. A pattern can choose its own file (Ctrl+T in its roll;
  in the song, Ctrl+T stays the song's), and only then does the song carry it: just the sounds its
  patterns place, to the highest (as `needs` counts now), so a pattern placing sounds 0 to 3 of another
  file costs 4 x 128 bytes, not 4 KB. Music shows each pattern's file and what it costs, and the Blob at
  its top right grows with it, so the choice is seen as it's made. In the room: the placement's spare
  byte says which bank, and each bank's speeds and loops follow the song's.

## Steps

1. The player (audio.c): tracks from the marks byte, voices shifting, songs ending when all tracks do,
   playing from a use with the others alongside; `pc.freeVoice`. Tests: tests/song.py, rendered.
2. The library (song.rio): tracks read and written (`rinc music 3`), `useTrack`, `trackCount`; `sound`
   with -1. Tests too.
3. Done (2026-11-23): Music's tracks: a row each, lined up in time (with more than one), T adds one,
   Ctrl+Up and Down move between them, N, D and paste add to the cursor's track, an emptied track goes;
   a use whose notes would be dropped outlined in red; rinc music 3 written with more than one track.
   Tests: studio's music.py (60, six on tracks).
4. Done: the spec (sound.md), Music's help; the board (the player's test, and Music as it was).
5. Next: pan (studio's ROADMAP), then sound banks a pattern (above), as the user decides.
