"""python song.py path/to/rinc work-folder: rinc's song library (library/song.rio): a song of Music's read, and
its sounds (Sound's file, read a line at a time) from the path it names, or of that name beside the song
when that path isn't there (a game's, its files together); a file that isn't a song refused, the song
as it was; the room its sounds and its tables need (a load refused and logged when it needs more, or the
file isn't there or isn't a song); version 1 songs converted (a pattern a line, its step its longest
sound's speed); version 2's levers, patterns of placements and uses read, written back the same, and
read again; each channel's notes worked out as hand-figured here (shifts, ramps, a slide's repeat, a
placement cut by the next and held a step); the uses played in turn, a loop's end going back to its
start, a stop stopping it; the song on the voices from its voice on; a sound on its own (a game's
effects); the levers set as it plays."""
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
put('game/tune2.mus', 'rinc music 2\nsounds notes.snd\nlevers 2 6 150\npattern 8 6\n0 6 1 5 4 6 0\n0 1 1 0 8 12 0\n6 1 0 0 8 0 0\n2 6 0 -3 8 0 1\npattern 4 3\n0 0 2 0 8 0 0\nsong\n0 0 4 8 0 100 b\n1 0 0 8 8 200 e\n')
put('game/game.rio', r'''
import song
notes: Blob[song.ALL_SOUNDS]
room: Blob[400]
tune := song.Song{notes = notes[], room = room[], voice = 4}
log("loops", tune.load("loops.mus"), tune.uses, tune.hasSounds, tune.soundsPath.asString(), tune.needs())
log("converted", tune.patterns, tune.uses, tune.places, tune.channels, tune.patternSteps(0), tune.patternSpeed(0), tune.patternSteps(2), tune.patternSpeed(2), tune.useFlags(0), tune.useFlags(1), tune.useFlags(2), tune.roomNeeds())
log("places", tune.placeChannel(0), tune.placeSound(0), tune.placeChannel(1), tune.placeSound(1), tune.placeChannel(2), tune.placeSound(2))
log("sound 1", tune.speeds[1], tune.notesOf(0), tune.notesOf(1), tune.notesOf(2), tune.loopAs[2], tune.loopBs[2], notes[128], notes[129], notes[130], notes[131])
log("not a song", tune.readSong("rinc music 1\nsounds x\n"), tune.readSong("hello"), tune.uses)
few: Blob[2 * song.SOUND_BYTES]
room2: Blob[300]
small := song.Song{notes = few[], room = room2[]}
loaded := [3]Bool{small.load("loops.mus"), small.load("none.mus"), small.load("game.rio")}
log("small", loaded[0], loaded[1], loaded[2])
log("two sounds", small.loadSounds("notes.snd"), small.sounds, small.notesOf(1), small.notesOf(2))
notes2: Blob[song.ALL_SOUNDS]
room3: Blob[200]
tune2 := song.Song{notes = notes2[], room = room3[], voice = 0}
log("v2", tune2.load("tune2.mus"), tune2.patterns, tune2.uses, tune2.places, tune2.channels, tune2.shift, tune2.volume, tune2.speed, tune2.roomNeeds())
log("place 3", tune2.placeStep(3), tune2.placeChannel(3), tune2.placeSound(3), tune2.placeShift(3), tune2.placeVolume(3), tune2.placeSpeed(3), tune2.placeHold(3))
log("uses", tune2.usePattern(0), tune2.useShift(0), tune2.useShiftEnd(0), tune2.useVolume(0), tune2.useVolumeEnd(0), tune2.useSpeed(0), tune2.useFlags(0), tune2.usePattern(1), tune2.useSpeed(1), tune2.useFlags(1))
log("lengths", (tune2.lengthOf(0) * 1000.0).toInt(), (tune2.lengthOf(1) * 1000.0).toInt())
text: Blob[..song.TEXT]
wrote := tune2.write(text)
log("written", wrote, text.asString() == "rinc music 2\nsounds notes.snd\nlevers 2 6 150\npattern 8 6\n0 6 1 5 4 6 0\n0 1 1 0 8 12 0\n6 1 0 0 8 0 0\n2 6 0 -3 8 0 1\npattern 4 3\n0 0 2 0 8 0 0\nsong\n0 0 4 8 0 100 b\n1 0 0 8 8 200 e\n")
again: Blob[200]
tune3 := song.Song{notes = notes2[], room = again[]}
log("read back", tune3.readSong(text.asString()), tune3.places, tune3.placeShift(3), tune3.useVolumeEnd(0), tune3.shift)
short: Blob[100]
tune4 := song.Song{notes = notes2[], room = short[]}
shortLoad := tune4.load("tune2.mus") # (its log says why, then this)
log("short", shortLoad, tune4.uses)
tiny: Blob[20]
tune5 := song.Song{notes = notes2[], room = tiny[]}
log("tiny", tune3.readSong("hello"), tune3.uses, tune5.readSong(text.asString()), tune5.reason())
fine := tune.play(0, false)
frame := 0
was := -1
rows: Blob[..40]
bytes :: proc(what: String, at: Int)
  said: Blob[..120]
  fine = said.print(what)
  for k in at..<at + 16
    fine = said.print(" ", room3[k])
  end
  log(said.asString())
end
update* :: proc()
  tune.tick()
  if tune.playing and tune.playUse != was
    was = tune.playUse
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
  if frame == 163
    fine = tune2.play(0, false)
    bytes("notes 1", 132)
    bytes("notes 1 on", 148)
    bytes("notes 6", 164)
  end
  if frame == 164
    log("v2 voices", pc.soundPlaying(0), pc.soundPlaying(1), pc.soundPlaying(6))
    tune2.setShift(3)
    tune2.setVolume(4)
    log("levers", tune2.shift, tune2.volume)
  end
end
''')
r = subprocess.run([rinc, os.path.join(work, 'game') + '/', '-data', os.path.join(work, 'data'), '-frames', '170'], capture_output=True, text=True, timeout=60)
out = (r.stdout + r.stderr).replace('\r', '')
lines = out.split('\n')
want = [
  'loops true 3 true notes.snd 384', # (its sounds: not at the path it names, but beside it; sounds 0 to 2 its patterns')
  'converted 3 3 5 4 4 6 2 3 0 1 2 202', # (version 1: a pattern a line, its step its longest sound's speed)
  'places 0 0 1 1 3 2',
  'sound 1 3 4 2 3 1 3 30 3 7 1',
  'not a song false false 3',
  'loops.mus: its sounds need notes Blob[384] (has 256)', # (the log says why, room too small)
  'none.mus: no such file',
  "game.rio: not a song of Music's",
  'small false false false',
  'two sounds true 2 2 0',             # (as many as its room has: the first two)
  'v2 true 2 2 5 7 2 6 150 196',       # (version 2: levers, a pattern's placements, the song's uses)
  'place 3 2 6 0 -3 8 0 1',
  'uses 0 0 4 8 0 100 1 1 200 2',
  'lengths 266 33',                    # (8 steps of 6 at 150%; 4 of 3 at 200% of 150%)
  'written true true',
  'read back true 5 -3 0 2',
  'tune2.mus: it needs room Blob[196] (has 100)',
  'short false 0',
  'tiny false 2 false it needs room Blob[68] at least (has 20)', # (the song as it was, when its tables don't fit)
  'voices 0 1 1 0 1',              # (pattern 0: channels 0, 1 and 3 on voices 4, 5 and 7)
  'stops played 0 1  false',
  'a sound 1',
  'notes 1 30 3 7 1 30 3 6 0 32 2 4 0 32 2 3 0', # (a slide's repeat plain; volume ramped down, shift up)
  'notes 1 on 0 0 0 0 0 0 0 0 27 0 1 0 27 0 0 0', # (the next placement, from step 6)
  'notes 6 35 3 3 1 36 2 2 0 22 0 3 0 0 0 0 0', # (cut by the next placement; held one step)
  'v2 voices 0 1 1',
  'levers 3 4']
bad = [w for w in want if w not in lines]
loops = [l for l in lines if l.startswith('loops played ')]
if not loops or not loops[0].split()[2:7] == ['0', '1', '2', '1', '2']:
  bad.append('loops played 0 1 2 1 2 (on round its loop)')
if bad: sys.exit('the song library: missing %r in:\n%s' % (bad, out))
print('the song library read, played and looped songs')
