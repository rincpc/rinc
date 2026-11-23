# Sound

A Rinc mixes 16 voices, numbered 0 to 15, into 48,000 frames a second of 16-bit stereo. Each voice plays
sound data straight from the app's memory, reading it as it plays, or notes, on [the synth](#the-synth).
(The Fruit Jam mixes on its second core, beside the HDMI, so sound takes none of an app's time: 16 synth
voices at the synth's starting rate take about 40% of that core's; at its top rate, smoothed, three times
the work a voice, so 16 of those would be too much, and the sound would break up. `pc.audioLoad` says.)

## The procs

Each proc takes a voice number first; a number outside 0 to 15 does nothing (and `soundPlaying` gives 0).

| proc | |
|---|---|
| `pc.sound(v, data, s)` | play `data` from its start, once, full volume, centered, with no loop, in place of whatever `v` was playing, as `s`, a `pc.Sound`, says: `bits` 16, 8 or 4 (0: 16), `channels` 1 or 2 (0: 1), else nothing happens, and `rate` frames a second (0: 16000) |
| `pc.soundWav(v, data) -> Int` | play a WAV file's sound data at the file's own rate, as `sound` would; 1 if it could, 0 if the file isn't one it plays |
| `pc.soundRate(v, rate)` | frames a second from now on, from 0 (paused) to 768,000 |
| `pc.soundVolume(v, volume)` | from 0 to 1 (outside: the nearest) |
| `pc.soundPan(v, pan)` | from -1 (left only) through 0 (both, full) to 1 (right only) |
| `pc.soundLoop(v, start, stop)` | on reaching frame `stop`, go on from frame `start`; `stop` past the end is the end; `stop` 0 or less, or `start` not before `stop`, ends any loop |
| `pc.soundStop(v)` | stops (on the synth, fading out, as after its last note) |
| `pc.soundSeek(v, at)` | on from frame `at` (on the synth, note `at`), as it plays; past the end, the last |
| `pc.audioLoad() -> [2]Int` | how much of its time mixing takes, in percent: on average and at most, since last asked; `{-1, -1}` where it isn't measured (the board measures it: it mixes on its second core, with the HDMI, so its share is that core's, not the app's) |
| `pc.soundPlaying(v) -> Int` | 1 while `v` plays, else 0 |
| `pc.synth(v, notes, speed)` | play `notes` on the synth, from the first, each `speed` 120ths of a second (1 to 255), full volume, centered, with no loop, in place of whatever `v` was playing; fewer than 4 bytes of notes stops `v` |
| `pc.synthMake(out, notes, speed) -> Int` | the samples `synth` would make, added to `out` instead: 16-bit, little-endian, with voice -1's settings (below), until the notes end or `out` is full; how many. `speed` 0: on from where the last one stopped (its next sample, in the `notes` given now), so a sound longer than `out` holds can be made a piece at a time |
| `pc.synthRate(v, rate)` | samples the synth makes a second, from 8,000 to 48,000 (16,000 to start) |
| `pc.synthBend(v, semitones)` | every pitch moved by that many semitones, from -96 to 96 (0 to start) |
| `pc.synthEnvelope(v, attack, release)` | seconds the volume heard takes to rise, and to fall, about 2/3 of the way to a note's: from 0 (at once) to 10 (0.0062 to start) |
| `pc.synthSmooth(v, on)` | 1: the saw, square and pulse smoothed where they jump; 0: not (to start) |
| `pc.synthWave(w, cycle)` | wave `w`, 8 to 15, for every voice: one cycle of 16-bit little-endian samples, read as it plays; fewer than 2 samples: silent (to start) |

The synth's settings (`synthRate` to `synthSmooth`) are a voice's own, kept from one `synth` to the
next, and a voice playing on the synth changes as they're set. Voice -1's are `synthMake`'s. An app
starts with every setting as above, and no waves of its own.

## Sound data

A frame is one sample for each channel; stereo samples come left first.

- **16 bits:** signed, little-endian.
- **8 bits:** signed, one byte each.
- **4 bits, IMA ADPCM,** laid out as in WAV files (format 0x11), in blocks of 256 bytes for each channel
  (256 mono, 512 stereo). A block starts with each channel's first sample (16 bits) and step index (a
  byte, then a byte of 0), then 4-bit codes: for mono, in byte order, the low half of a byte first; for
  stereo, 4 bytes (8 codes) of the left channel, then 4 of the right, and so on. A 256-byte mono block is
  505 frames.

`soundWav` plays WAV files with 16-bit PCM or IMA ADPCM (any block size), mono or stereo. It reads the
`fmt ` and `data` chunks, and plays what there is of a cut-short file. (WAV's 8-bit samples are unsigned,
so it doesn't take those.)

The number of frames is the data's length divided by the bytes a frame takes (rounded down), or for ADPCM
the frames its blocks hold.

## Mixing

A voice moves through its data at `rate / 48000` frames for each frame of output, and takes the value
between the two nearest frames in a straight line. A mono voice plays on both sides. Its volume and pan
scale each side: left by `volume * (1 - pan)` when `pan` is above 0, right by `volume * (1 + pan)` when
it's below 0, and each side by `volume` otherwise. The voices are added together, scaled by the Rinc's
own volume (where it has one), and cut off at the 16-bit range.

Changing a voice's data while it plays changes what's heard. A voice that reaches the end of its data
without a loop stops.

## The synth

On the synth, a voice plays notes: 4 bytes each, the same as Sound's (`studio/sound`).

| byte | |
|---|---|
| pitch | 0 is C0 (65.406 Hz), and each step a semitone up: `65.406 * 2^(pitch / 12)` Hz (above 95: 95) |
| wave | 0 triangle, 1 tilted (a triangle up slowly, down fast), 2 saw, 3 square, 4 pulse (a quarter up), 5 organ (a triangle and its octave), 6 noise (a new value 8 times a cycle), 7 phaser (two triangles 0.7% apart), 8 to 15 the app's own (`synthWave`); only the low 4 bits count |
| volume | 0 (silent) to 7 (full); only the low 3 bits count |
| effect | 0 none, 1 slide (from the note before, its pitch and volume, across the note), 2 vibrato (a quarter tone either way, 7.5 times a second), 3 drop (the pitch down to 0 across the note), 4 fade in, 5 fade out, 6 a fast arpeggio (the four notes of its group of 4, 30 a second), 7 a slow one (15 a second); only the low 3 bits count |

It makes `rate` samples a second (`synthRate`), as it plays, and they're mixed as any sound's at that
rate. A note lasts `speed * 133 * rate / 16000` samples, so the same time at any rate (a note partway
along when the rate changes stays as far along). Each sample is the wave at the note's pitch plus the
bend, at the volume heard, times 10,240: a synth wave's peak is near its average, so this is about as
loud as recorded sound peaking at full scale. The volume heard moves toward the note's by
`1 - e^(-1 / (seconds * rate))` of the way each sample, `seconds` being the attack while it rises and
the release while it falls (`synthEnvelope`; to start, 1% a sample at 16,000), so changes don't
click. At full volume, the triangle and the saw reach 10,240 and 7,168, the square and the pulse
5,120, and an app's wave its own samples times 10,240 / 32,768. After the last note, it fades out
over 50 ms or 5 releases, whichever is longer, and stops.

An app's wave is read where the cycle is, in a straight line between its two nearest samples (after
the last, the first). Smoothing (`synthSmooth`) is PolyBLEP: within a sample's step either side of a
jump, the wave is rounded off, so the jump's overtones too high for the rate mostly don't come back as
false tones lower down.

On a synth voice, `soundLoop`'s `start` and `stop` are notes, `soundRate` speeds it up or slows it
down as a whole (16,000 is as it is, at any `synthRate`), and the rest are the same. Volume and pan
changes ease in within 64 frames of output. Changing the notes while they play changes what's heard.

## Songs

Rinc plays a song as studio's Music makes it, on its own once started, so a game calls nothing each
frame: rinc's `song` library reads the files and calls these, one song at a time.

| proc | |
|---|---|
| `pc.songPlay(v, room, notes, patterns, uses, from, how)` | play the song in `room`'s tables, its sounds' notes in `notes`, from use `from` (its track; the others where they'd be then), on voices from `v` on; `how`: 1 just that use (its track alone), 2 round again from the first at its end, and from bit 8 on how many voices it may take (0: 8); in place of the song playing |
| `pc.songStop()` | its voices fading out, as after a sound's last note |
| `pc.songPause(on)` | 1 holds it where it is (its voices silent), 0 goes on |
| `pc.songLevers(shift, volume, speed)` | semitones (-96 to 96), eighths (0 to 8), percent (1 to 400) over the whole song: shift and volume at once, speed from the next placements on |
| `pc.songAt() -> [3]Int` | the use playing in the track it was started from, its time round (from 0), and how far into its pattern, in thousandths (that track ended, the others on: its last use, 1000); `{-1, 0, 0}` when none plays |
| `pc.freeVoice() -> Int` | the first voice not playing and not the song's (-1: none): a game's sounds there, never in the song's way (the song library's `sound(-1, n)` takes one so) |

`room` holds, from its start: the patterns, 6 bytes each (its steps; a step's length, in 120ths of a
second; its first placement, 2 bytes, low first; how many it has; 0); then the uses, 11 bytes each (its
pattern; its shift and its shift at its end, each plus 64; its volume and at its end, in eighths; its
speed, in percent, 2 bytes, low first; its marks, 1 a loop begins, 2 a loop ends, 4 the song stops, and
its track times 16, 0 to 7; how many times it's played over; its pan and its pan at its end); then the
placements, 9 bytes each (its step; its channel, 0 to 7; its sound; its shift plus 64; its volume, in
eighths; its speed, or 0 for its sound's; its hold, in steps, or 0; its pan; its file of sounds, 0 the
song's own). Its sound is its number in the row of all the song's sounds: the song's own file's first,
then each file a pattern picks of its own, just the sounds the song places from it (the song library
lays them out so, and `notes` holds them in that order). A pan
is a signed byte, -8 to 8, eighths of the way left or right, 0 the center: a placement's and its use's
(ramped across its times round, as shift and volume are) added. Then, for each sound the song plays, from 0, its speed and its loop's start and end, 3 bytes.
`notes` holds the sounds' notes, 128 bytes a sound (32 notes of 4 bytes, as the synth takes them).

A song's uses are in tracks, played side by side (a drum beat in one, the melody's patterns in another):
each track its uses in the table's order, in turn, as below, with its own loops; a stop mark in any
ends the song, and the song ends when every track has. The tracks take voices in order: track 0's
channel `c` on voice `v + c` (its pattern's highest channel, plus one, the voices it takes), a later
track's after the tracks before it, just the channels its pattern uses (a bass on channel 6 alone takes
one voice), worked out again as a track's pattern changes; a channel past the song's
voices is dropped (silent). So the first track's are never dropped.

Time goes in ticks of 1/120 s, kept by the mixer. At its step a placement starts on its channel's voice
as `pc.synth` would play its sound, its notes as long as its speed (scaled by its use's speed and the
speed lever), to its loop's end or its last note heard, round its loop, until its hold is up, the next
placement on its channel starts, or its pattern ends. Its pitch moves by its shift, its use's (ramped
across all its times round) and the shift lever's; its volume is its own times its use's (ramped) times
the volume lever's. At a pattern's end, the next time round, or the next use: after a loop's end, back to
the use where it began; after a stop, or the last use, the song ends (or, as `how` 2 says, starts
again). Rinc reads both as it plays, an app's changes too, and checks every index against them.
