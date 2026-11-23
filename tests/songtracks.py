"""python songtracks.py path/to/rinc work-folder: a song's tracks (rinc's song library and player:
plans/song-tracks.md). A song of two tracks read (rinc music 3) and written back the same; a song of one
still written as rinc music 2. Played: each track's channels on voices after the tracks' before it (the
first's as its channels say; a later one's just those it uses: a channel 6 alone, one voice); past the song's voices (Song.voices), dropped; the song playing on till
its longest track ends; played from a use of the first track, the second where it'd be then; one use
alone, its track alone (on the first voices). pc.freeVoice: the first voice not playing and not the
song's; a sound on voice -1 takes it. Pan: read and written back, and heard (full left: the right
channel silent). Banks: a pattern's own file of sounds, read, played and written back."""
import os, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), sys.argv[2]
shutil.rmtree(work, ignore_errors=True)
def put(path, text):
  path = os.path.join(work, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'w', newline='') as f: f.write(text)
def sound(speed, notes, loop=(0, 0)):
  line = '%02x%02x%02x' % (speed, loop[0], loop[1]) + ''.join('%02x%x%x%x' % n for n in notes)
  return line + '00000' * (32 - len(notes))
held = sound(6, [(24, 0, 5, 0)] * 4, (0, 4)) # (round its loop: heard till its pattern ends)
put('game/notes.snd', 'rinc sound 1\n' + ''.join(s + '\n' for s in [held] + [sound(1, [])] * 31))
# patterns of 8 steps of 6 ticks: 48 ticks, 0.4 s, 24 frames. Track 1: pattern 0 (channels 0 and 1), then
# pattern 2 (channel 3); track 2: pattern 1 (channel 0), four times round
two = ('rinc music 3\nsounds notes.snd\nlevers 0 8 100\npattern 8 6\n0 0 0 0 8 0 0\n0 1 0 0 8 0 0\npattern 8 6\n0 0 0 0 8 0 0\n'
       'pattern 8 6\n0 3 0 0 8 0 0\npattern 8 6\n0 6 0 0 8 0 0\nsong\n0 0 0 8 8 100 1 -\n2 0 0 8 8 100 1 -\ntrack 2\n1 0 0 8 8 100 4 -\ntrack 3\n3 0 0 8 8 100 4 -\n')
put('game/two.mus', two)
put('game/game.rio', r'''
import song
buf: Blob[2000]
tune := song.Song{buffer = buf[], voice = 4}
log("read", tune.load("two.mus"), tune.tracks, tune.uses, tune.useTrack(0), tune.useTrack(1), tune.useTrack(2), tune.useFlags(2))
text: Blob[..song.TEXT]
log("written", tune.write(text), text.asString() == "''' + two.replace('\n', '\\n') + r'''")
buf1: Blob[2000]
one := song.Song{buffer = buf1[]}
fine := one.load("two.mus")
one.setUseTrack(2, 0)
one.setUseTrack(3, 0)
fine = one.write(text)
log("one track", text.asString()[0..<12], one.useTrack(2))
s: Blob[..40]
playing :: proc(a: Int, b: Int) -> String
  s.clear()
  for v in a..<b
    fine = s.print(pc.soundPlaying(v))
  end
  return s.asString()
end
said: Blob[..40]
frame := 0
update* :: proc() # (a stopped song's voices fade out a while: each step well after the one before)
  frame += 1
  if frame == 1
    fine = tune.play(0, false)
  end
  if frame == 3
    log("voices", playing(0, 12), pc.freeVoice()) # (track 1: channels 0, 1 on 4, 5; track 2's channel 0 after them: 6; track 3's channel 6, its only one: 7)
    tune.stop()
  end
  if frame == 40
    tune.voices = 2
    fine = tune.play(0, false)
  end
  if frame == 42
    log("two voices", playing(0, 12), pc.freeVoice()) # (track 2's past the song's two: dropped)
    tune.stop()
  end
  if frame == 80
    tune.voices = 0
    fine = tune.play(0, false) # (track 1: 48 frames; track 2: 4 times round, 96)
  end
  if frame == 140
    tune.tick()
    log("track 1 ended", tune.playing, tune.playUse, (tune.playPart * 1000.0).toInt(), playing(4, 8)) # (track 2 on; from its next time round on, on the first voice)
  end
  if frame == 185
    tune.tick()
    log("ended", tune.playing, playing(0, 12))
  end
  if frame == 220
    fine = tune.play(1, false)
  end
  if frame == 222
    tune.tick()
    log("from use 1", playing(0, 12), tune.playUse) # (where it plays: the first track's, use 1) # (track 1's pattern 2: channel 3 on 7; track 2, its second time round, after its 4 channels: 8)
    tune.stop()
  end
  if frame == 250
    fine = tune.play(2, false) # (from track 2's use: the first track's where it'd be then, followed)
  end
  if frame == 252
    tune.tick()
    log("from track 2", tune.playUse)
    tune.stop()
  end
  if frame == 280
    fine = tune.play(2, true)
  end
  if frame == 282
    log("alone", playing(0, 12)) # (track 2's use, its track alone: on 4)
    tune.stop()
  end
  if frame == 320
    tune.voice = 0
    tune.voices = 3
    fine = tune.play(0, false)
  end
  if frame == 322
    v := pc.freeVoice()
    tune.sound(-1, 0)
    log("free", v, pc.soundPlaying(3), pc.freeVoice()) # (the song's 0 to 2: 3 free, then taken by sound(-1, 0))
  end
end
''')
r = subprocess.run([rinc, os.path.join(work, 'game') + '/', '-data', os.path.join(work, 'data'), '-frames', '330'], capture_output=True, text=True, timeout=60)
lines = (r.stdout + r.stderr).replace('\r', '').split('\n')
want = ['read true 3 4 0 0 1 0', 'written true true', 'one track rinc music 2 0',
        'voices 000011110000 0', 'two voices 000011000000 0',
        'track 1 ended true 1 1000 1100', 'ended false 000000000000',
        'from use 1 000000011100 1', 'from track 2 0', 'alone 000010000000', 'free 3 1 4']
bad = [w for w in want if w not in lines]
if bad:
  print('FAILED: wanted %r\n%s' % (bad, '\n'.join(lines)))
  sys.exit(1)
# pan: a placement's (an 8th number) and a use's (a 9th and 10th, ramped), read and written back the same;
# played full left, the right channel silent (rinc -wav)
import wave, struct
panned = 'rinc music 2\nsounds notes.snd\nlevers 0 8 100\npattern 8 6\n0 0 0 0 8 0 0 -8\nsong\n0 0 0 8 8 100 2 0 0 -\n'
put('pan/notes.snd', open(os.path.join(work, 'game', 'notes.snd'), newline='').read())
put('pan/song.mus', panned)
put('pan/pan.rio', 'import song\nbuf: Blob[2000]\nt := song.Song{buffer = buf[]}\n'
    'log("pans", t.load("song.mus"), t.placePan(0), t.usePan(0), t.usePanEnd(0))\n'
    'text: Blob[..song.TEXT]\n'
    'log("pans written", t.write(text), text.asString() == "' + panned.replace('\n', '\\n') + '")\n'
    'fine := t.play(0, false)\n')
wav = os.path.join(work, 'pan.wav')
r = subprocess.run([rinc, os.path.join(work, 'pan') + '/', '-data', os.path.join(work, 'data'), '-frames', '30', '-wav', wav], capture_output=True, text=True, timeout=60)
said = (r.stdout + r.stderr).replace('\r', '').split('\n')
w = wave.open(wav)
frames = w.readframes(w.getnframes())
w.close()
samples = struct.unpack('<%dh' % (len(frames) // 2), frames)
left = max(abs(v) for v in samples[0::2])
right = max(abs(v) for v in samples[1::2])
if 'pans true -8 0 0' not in said or 'pans written true true' not in said or not (left > 1000 and right < 50):
  print('FAILED: pan: %r left %d right %d' % (said, left, right))
  sys.exit(1)
# banks: a pattern's own file of sounds (a "sounds" line after its pattern line): the sounds it places
# read after the song's own (only those: to the highest it places), its placements' sounds in that row,
# written back as that file numbers them; the room its notes need, just so; played; a file that isn't
# there, logged, the song loading still
other = 'rinc sound 1\n' + sound(6, [(40, 0, 5, 0)] * 4, (0, 4)) + '\n' + sound(1, []) + '\n' + sound(6, [(50, 0, 5, 0)] * 4, (0, 4)) + '\n' + ''.join(sound(1, []) + '\n' for _ in range(29))
banked = ('rinc music 2\nsounds notes.snd\nlevers 0 8 100\npattern 8 6\n0 0 0 0 8 0 0\npattern 8 6\nsounds other.snd\n'
          '0 0 2 0 8 0 0\n0 1 0 0 8 0 0\nsong\n0 0 0 8 8 100 1 -\n1 0 0 8 8 100 1 -\n')
put('banks/notes.snd', open(os.path.join(work, 'game', 'notes.snd'), newline='').read())
put('banks/other.snd', other)
put('banks/song.mus', banked)
put('banks/gone.mus', banked.replace('other.snd', 'gone.snd'))
put('banks/banks.rio', 'import song\nbuf: Blob[3000]\nt := song.Song{buffer = buf[]}\n'
    'log("banks", t.load("song.mus"), t.banks, t.patternBank(0), t.patternBank(1), t.bankPath(1), t.bankBase[1], t.bankUsed[1], t.needs(), t.notes.len())\n'
    'log("placed", t.placeSound(1), t.placeOwnSound(1), t.placeBank(1), t.placeSound(2), t.placeOwnSound(2), t.notes[0], t.notes[128], t.notes[3 * 128])\n'
    'text: Blob[..song.TEXT]\n'
    'log("banks written", t.write(text), text.asString() == "' + banked.replace('\n', '\\n') + '")\n'
    'fine := t.play(1, true)\n'
    'log("banks playing", pc.soundPlaying(0), pc.soundPlaying(1))\n'
    'gone := song.Song{buffer = buf[]}\n'
    'goneOk := gone.load("gone.mus")\nlog("gone", goneOk, gone.banks)\n')
r = subprocess.run([rinc, os.path.join(work, 'banks') + '/', '-data', os.path.join(work, 'data'), '-frames', '3'], capture_output=True, text=True, timeout=60)
said = (r.stdout + r.stderr).replace('\r', '').split('\n')
for w in ['banks true 2 0 1 other.snd 1 3 512 512', 'placed 3 2 1 1 0 24 40 50', 'banks written true true', 'banks playing 1 1', 'gone true 2']:
  if w not in said:
    print('FAILED: banks: wanted %r in %r' % (w, said))
    sys.exit(1)
if not any('gone.snd: no such file' in l for l in said):
  print('FAILED: banks: a file not there, not logged: %r' % said)
  sys.exit(1)
print('song tracks came through')
