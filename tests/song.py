"""python song.py path/to/rinc work-folder: rinc's song library (library/song.rio): a song of Music's read, and
its sounds (Sound's file, read a line at a time) from the path it names, or of that name beside the song
when that path isn't there (a game's, its files together); a file that isn't a song refused, the song
as it was; the room its sounds need (as many read as there's room for, a load refused and logged when
its song needs more, or the file isn't there or isn't a song); the patterns played in turn, a loop's end
going back to its start, a stop stopping it; the song on the voices from its voice on; a sound on its own
(a game's effects)."""
import os, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), sys.argv[2]
shutil.rmtree(work, ignore_errors=True)
def put(path, text):
  path = os.path.join(work, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'w', newline='') as f: f.write(text)
def sound(speed, notes, loop=(0, 0)): # (notes: pitch, wave, volume, effect each; to 32 with silence)
  line = '%02x%02x%02x' % (speed, loop[0], loop[1]) + ''.join('%02x%x%x%x' % n for n in notes)
  return line + '00000' * (32 - len(notes))
sounds = [sound(6, [(24, 0, 5, 0)] * 4), sound(3, [(30, 3, 7, 1), (31, 2, 6, 0)]), sound(9, [(12, 6, 4, 0)] * 3, (1, 3))]
put('game/notes.snd', 'rinc sound 1\r\n' + ''.join(s + '\r\n' for s in sounds + [sound(1, [])] * (32 - len(sounds))))
put('game/loops.mus', 'rinc music 1\nsounds /rinc/save/sound/elsewhere/notes.snd\n00 01 -- 02 -\n00 -- -- -- b\n01 -- -- -- e\n')
put('game/stops.mus', 'rinc music 1\r\nsounds notes.snd\r\n00 -- -- -- -\r\n01 -- -- -- s\r\n00 -- -- -- -\r\n')
put('game/game.rio', '''
import song
notes: Blob[song.ALL_SOUNDS]
tune := song.Song{notes = notes[], voice = 4}
log("loops", tune.load("loops.mus"), tune.count, tune.hasSounds, tune.soundsPath.asString(), tune.needs())
log("chans", tune.chans[0], tune.chans[1], tune.chans[2], tune.chans[3], tune.chans[4], tune.flags[0], tune.flags[1], tune.flags[2])
log("sound 1", tune.speeds[1], tune.notesOf(0), tune.notesOf(1), tune.notesOf(2), tune.loopAs[2], tune.loopBs[2], notes[128], notes[129], notes[130], notes[131])
log("not a song", tune.readSong("rinc music 1\\nsounds x\\n"), tune.readSong("hello"), tune.count)
few: Blob[2 * song.SOUND_BYTES]
small := song.Song{notes = few[]}
loaded := [3]Bool{small.load("loops.mus"), small.load("none.mus"), small.load("game.rio")}
log("small", loaded[0], loaded[1], loaded[2])
log("two sounds", small.loadSounds("notes.snd"), small.sounds, small.notesOf(1), small.notesOf(2))
fine := tune.play(0, false)
frame := 0
was := -1
rows: Blob[..40]
update* :: proc()
  tune.tick()
  if tune.playing and tune.playRow != was
    was = tune.playRow
    fine = rows.print(was, " ")
  end
  frame += 1
  if frame == 2
    log("voices", pc.soundPlaying(3), pc.soundPlaying(4), pc.soundPlaying(5), pc.soundPlaying(6), pc.soundPlaying(7))
  end
  if frame == 80
    log("loops played", rows.asString())
    fine = tune.load("stops.mus") and tune.play(0, false)
    rows.clear()
    was = -1
  end
  if frame == 160
    log("stops played", rows.asString(), tune.playing)
    tune.sound(1, 1)
  end
  if frame == 161
    log("a sound", pc.soundPlaying(1))
  end
end
''')
r = subprocess.run([rinc, os.path.join(work, 'game') + '/', '-data', os.path.join(work, 'data'), '-frames', '165'], capture_output=True, text=True, timeout=60)
out = (r.stdout + r.stderr).replace('\r', '')
lines = out.split('\n')
want = [
  'loops true 3 true notes.snd 384', # (its sounds: not at the path it names, but beside it; sounds 0 to 2 its patterns')
  'chans 0 1 255 2 0 0 1 2',
  'sound 1 3 4 2 3 1 3 30 3 7 1',
  'not a song false false 3',
  'loops.mus: its sounds need notes Blob[384] (has 256)', # (the log says why, room too small)
  'none.mus: no such file',
  "game.rio: not a song of Music's",
  'small false false false',
  'two sounds true 2 2 0',             # (as many as its room has: the first two)
  'voices 0 1 1 0 1',              # (pattern 0: channels 0, 1 and 3 on voices 4, 5 and 7)
  'stops played 0 1  false',
  'a sound 1']
bad = [w for w in want if w not in lines]
loops = [l for l in lines if l.startswith('loops played ')]
if not loops or not loops[0].split()[2:7] == ['0', '1', '2', '1', '2']:
  bad.append('loops played 0 1 2 1 2 (on round its loop)')
if bad: sys.exit('the song library: missing %r in:\n%s' % (bad, out))
print('the song library read, played and looped songs')
