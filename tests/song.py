"""python song.py path/to/rinc work-folder: rinc's song library (library/song.rio): a song of Music's read, and
its sounds (Sound's file, read a line at a time) from the path it names, or of that name beside the song
when that path isn't there (a game's, its files together); a file that isn't a song refused, the song
as it was; the patterns played in turn, a loop's end going back to its start, a stop stopping it; the
song on the voices from song.voice on; a sound on its own (a game's effects)."""
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
log("loops", song.load("loops.mus"), song.count, song.hasSounds, song.soundsPath.asString())
log("chans", song.chans[0], song.chans[1], song.chans[2], song.chans[3], song.chans[4], song.flags[0], song.flags[1], song.flags[2])
log("sound 1", song.speeds[1], song.notesOf(0), song.notesOf(1), song.notesOf(2), song.loopAs[2], song.loopBs[2], song.notes[128], song.notes[129], song.notes[130], song.notes[131])
log("not a song", song.readSong("rinc music 1\\nsounds x\\n"), song.readSong("hello"), song.count)
song.voice = 4
fine := song.play(0, false)
frame := 0
was := -1
rows: Blob[..40]
update* :: proc()
  song.tick()
  if song.playing and song.playRow != was
    was = song.playRow
    fine = rows.print(was, " ")
  end
  frame += 1
  if frame == 2
    log("voices", pc.soundPlaying(3), pc.soundPlaying(4), pc.soundPlaying(5), pc.soundPlaying(6), pc.soundPlaying(7))
  end
  if frame == 80
    log("loops played", rows.asString())
    fine = song.load("stops.mus") and song.play(0, false)
    rows.clear()
    was = -1
  end
  if frame == 160
    log("stops played", rows.asString(), song.playing)
    song.sound(1, 1)
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
  'loops true 3 true notes.snd', # (its sounds: not at the path it names, but beside it)
  'chans 0 1 255 2 0 0 1 2',
  'sound 1 3 4 2 3 1 3 30 3 7 1',
  'not a song false false 3',
  'voices 0 1 1 0 1',              # (pattern 0: channels 0, 1 and 3 on voices 4, 5 and 7)
  'stops played 0 1  false',
  'a sound 1']
bad = [w for w in want if w not in lines]
loops = [l for l in lines if l.startswith('loops played ')]
if not loops or not loops[0].split()[2:7] == ['0', '1', '2', '1', '2']:
  bad.append('loops played 0 1 2 1 2 (on round its loop)')
if bad: sys.exit('the song library: missing %r in:\n%s' % (bad, out))
print('the song library read, played and looped songs')
