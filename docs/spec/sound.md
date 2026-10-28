# Sound

A Rinc mixes 8 voices, numbered 0 to 7, into 48,000 frames a second of 16-bit stereo. Each voice plays
sound data straight from the app's memory, reading it as it plays, or notes, on [the synth](#the-synth).

## The procs

Each proc takes a voice number first; a number outside 0 to 7 does nothing (and `soundPlaying` gives 0).

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
| `pc.soundPlaying(v) -> Int` | 1 while `v` plays, else 0 |
| `pc.synth(v, notes, speed)` | play `notes` on the synth, from the first, each `speed` 120ths of a second (1 to 255), full volume, centered, with no loop, in place of whatever `v` was playing; fewer than 4 bytes of notes stops `v` |
| `pc.synthMake(out, notes, speed) -> Int` | the samples `synth` would make, into `out` instead: 16-bit, little-endian, with voice -1's settings (below), until the notes end or `out` is full; how many. `speed` 0: on from where the last one stopped (its next sample, in the `notes` given now), so a sound longer than `out` holds can be made a piece at a time |
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
