# Sound

A Rinc mixes 8 voices, numbered 0 to 7, into 48,000 frames a second of 16-bit stereo. Each voice plays
sound data straight from the app's memory, reading it as it plays, or notes, on [the synth](#the-synth).

## The procs

Each proc takes a voice number first; a number outside 0 to 7 does nothing (and `soundPlaying` gives 0).

| proc | |
|---|---|
| `pc.sound(v, data, bits, channels, rate)` | play `data` from its start, once, at `rate` frames a second, full volume, centered, with no loop, in place of whatever `v` was playing; `bits` 16, 8 or 4 and `channels` 1 or 2, else nothing happens |
| `pc.soundWav(v, data) -> Int` | play a WAV file's sound data at the file's own rate, as `sound` would; 1 if it could, 0 if the file isn't one it plays |
| `pc.soundRate(v, rate)` | frames a second from now on, from 0 (paused) to 768,000 |
| `pc.soundVolume(v, volume)` | from 0 to 1 (outside: the nearest) |
| `pc.soundPan(v, pan)` | from -1 (left only) through 0 (both, full) to 1 (right only) |
| `pc.soundLoop(v, start, stop)` | on reaching frame `stop`, go on from frame `start`; `stop` past the end is the end; `stop` 0 or less, or `start` not before `stop`, ends any loop |
| `pc.soundStop(v)` | stops |
| `pc.soundPlaying(v) -> Int` | 1 while `v` plays, else 0 |
| `pc.synth(v, notes, speed)` | play `notes` on the synth, from the first, each `speed` 120ths of a second (1 to 255), full volume, centered, with no loop, in place of whatever `v` was playing; fewer than 4 bytes of notes stops `v` |
| `pc.synthMake(out, notes, speed) -> Int` | the samples `synth` would make, into `out` instead: 16-bit, little-endian, 16,000 a second, until the notes end or `out` is full; how many |

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
| wave | 0 triangle, 1 tilted (a triangle up slowly, down fast), 2 saw, 3 square, 4 pulse (a quarter up), 5 organ (a triangle and its octave), 6 noise (a new value 8 times a cycle), 7 phaser (two triangles 0.7% apart); only the low 3 bits count |
| volume | 0 (silent) to 7 (full); only the low 3 bits count |
| effect | 0 none, 1 slide (from the note before, its pitch and volume, across the note), 2 vibrato (a quarter tone either way, 7.5 times a second), 3 drop (the pitch down to 0 across the note), 4 fade in, 5 fade out, 6 a fast arpeggio (the four notes of its group of 4, 30 a second), 7 a slow one (15 a second); only the low 3 bits count |

It makes 16,000 samples a second, as it plays, and they're mixed as any sound's at that rate. A note
lasts `speed * 133` samples. Each sample is the wave at the note's pitch, at the volume heard, which
moves 1% of the way to the note's volume each sample (so changes don't click), times 25,600. At full
volume, the triangle and the saw reach 25,600 and 17,920, the square and the pulse 12,800. After the
last note, it fades out over 800 samples (50 ms) and stops.

On a synth voice, `soundLoop`'s `start` and `stop` are notes, `soundRate` speeds it up or slows it
down as a whole (16,000 is as it is), and the rest are the same. Changing the notes while they play
changes what's heard.
