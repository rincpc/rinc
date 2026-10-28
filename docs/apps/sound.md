# Sound

Rinc plays sound on 8 voices, numbered 0 to 7. A voice plays bytes from the app's own memory, at
whatever rate it's told, so an app can make sound by computing it, or carry recorded sound as data.

```rio
wave: Blob[2048]
for i in 0..<wave.len()
  wave[i] = (i / 16 % 2) * 60 - 30 # a square wave: 8-bit signed samples
end
pc.sound(0, wave[], {bits = 8, rate = 22050.0})  # voice 0: 8-bit samples, 22050 a second
log(pc.soundPlaying(0))
```

```output
1
```

`pc.sound(voice, data, s)` starts `data` playing on that voice, once, from the start, at full volume and
centered. `s`, a `pc.Sound`, says how to read it: `bits` 16 or 8 (signed samples) or 4 (IMA ADPCM,
below), `channels` 1 or 2, and `rate`, frames a second. What's left out is 0, which means 16 bits, 1
channel and 16000 a second, so `pc.sound(0, data, {})` plays 16-bit mono at 16000. While it plays:

- `pc.soundRate(v, rate)` changes its speed, and so its pitch.
- `pc.soundVolume(v, volume)` from 0 to 1, and `pc.soundPan(v, pan)` from -1 (left) to 1 (right).
- `pc.soundLoop(v, start, stop)` makes it go back to frame `start` each time it reaches frame `stop`,
  for music and engine hums (`stop` 0: no loop).
- `pc.soundStop(v)` stops it, and `pc.soundPlaying(v)` is 1 while it plays.

The voice reads the bytes as it plays, so they have to stay put: keep sound in globals.

## Notes, on the synth

A voice can play notes instead, made into sound as it goes: 4 bytes a note, its pitch (0 is C0, and a
semitone a step), wave, volume (0 to 7) and effect, as Sound (`studio/sound`) keeps them.

```rio
tune: Blob[16]
for i in 0..<4
  tune[i * 4] = 24 + i * 4  # pitch: C2, E2, G#2, C3
  tune[i * 4 + 1] = 3       # square
  tune[i * 4 + 2] = 5       # volume
end
pc.synth(0, tune[], 20)     # voice 0, each note 20 120ths of a second
pc.soundLoop(0, 0, 4)       # notes 0 to 3, over and over
log(pc.soundPlaying(0))
```

```output
1
```

The waves are triangle, tilted, saw, square, pulse, organ, noise and phaser (0 to 7), and the effects
slide, vibrato, drop, fade in, fade out and two arpeggios (1 to 7): see [the synth](../spec/sound.md#the-synth).
`pc.synthMake(out, notes, speed)` makes the same sound into a `Blob`, 16-bit at 16,000 samples a second,
for a WAV file or to change before playing it.

Each voice has settings of its own, which change what it's playing as they're set:

```rio
saw: Blob[4]
saw[0] = 36                    # C3
saw[1] = 2                     # saw
saw[2] = 7
pc.synth(0, saw[], 30)
pc.synthRate(0, 48000.0)       # samples made a second: cleaner high notes, for 3 times the work
pc.synthSmooth(0, 1)           # the saw, square and pulse rounded off where they jump: cleaner, cheaply
pc.synthBend(0, 0.5)           # every pitch up a quarter tone
pc.synthEnvelope(0, 0.2, 1.0)  # notes rise slowly (0.2 seconds) and fall more slowly still
log(pc.soundPlaying(0))
```

```output
1
```

Voice -1's settings are the ones `synthMake` uses. Waves 8 to 15 are the app's own: `pc.synthWave(8,
cycle[])` makes wave 8 one cycle of 16-bit samples, which any voice plays at any pitch.

## Recorded sound

IMA ADPCM keeps sound in a quarter of the space of 16-bit, and sounds close to it: a second of 22 kHz
mono is 11 KB. `tools/adpcm.py` turns a 16-bit WAV file into Rio source for an app, which `pc.fromHex`
reads into a `Blob` (see `apps/chord`, its sound in `parts/sound.rio`). `pc.soundWav(voice, data)` plays a whole WAV file's bytes,
16-bit or ADPCM, at its own rate.

Next: [apps and home](apps.md).
