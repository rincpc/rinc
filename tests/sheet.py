"""python sheet.py path/to/rinc work-folder: rinc's sheet library (library/sheet.rio): a sheet file read (its
size, its sprites' size, its flags' names, its pixels as bytes, its flag records by each sprite's top left: 9
by 9 sprites here, and a record that starts no sprite, stranded, kept), its flags asked by name and by sprite,
written back as it was read; the room it needs said when it's short; a PNG's pixels read (no flags);
files that aren't sheets refused; two sheets' pixels added to one room (loadAdding, as Maps keeps them);
a sheet made, not read (Paint's), written."""
import os, shutil, struct, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), sys.argv[2]
shutil.rmtree(work, ignore_errors=True)
def put(path, text):
  path = os.path.join(work, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'w', newline='') as f: f.write(text)
W, H = 18, 9
pixels = bytes((x + y) % 16 for y in range(H) for x in range(W)) # (raw: 10s and 13s among them)
def record(x, y, flags): return struct.pack('<iii', x, y, flags)
data = (b'rinc sheet 1\nsize 18 9\nsprite 9x9\nflag 0 solid\nflag 9 water\nmarks 3\npixels\n' + pixels +
        record(0, 0, 1) + record(9, 0, 0x201) + record(4, 4, 2))
os.makedirs(os.path.join(work, 'game'), exist_ok=True)
open(os.path.join(work, 'game', 't.sheet'), 'wb').write(data)
put('game/game.rio', r'''
import sheet
pixels: Blob[..(18 + 1) * 9] # (a sheet's pixels; a PNG's need a byte more a row as it's read)
flagRoom: [4]Int
marks: [12]Int
s := sheet.Sheet{pixels = pixels, flags = flagRoom[], marks = marks[]}
log("loaded", s.load("t.sheet"), s.size.x, s.size.y, s.sprite.x, s.sprite.y, s.across, s.sprites, s.marked, s.stranded)
log("named", s.flag("solid"), s.flag("water"), s.flag("lava"), s.flagName(9), s.flagName(3).len())
log("flags", s.flagsOf(0), s.flagsOf(1), s.flagsOf(2), s.flagsOf(-1))
log("pixels", pixels.len(), pixels[3 * 18 + 5], pixels[8 * 18 + 17])
log("places", s.spriteAt(1).x, s.spriteAt(1).y, s.spriteOf({9, 0}), s.spriteOf({4, 4}), s.spriteOf({18, 0}))
log("written", s.write("/rinc/save/back.sheet"))
back: Blob[..1000]
n := pc.fileRead("/rinc/save/back.sheet", back)
want: Blob[..1000]
n2 := pc.fileRead("t.sheet", want)
log("as read", back.asString() == want.asString())
few: Blob[..100]
small := sheet.Sheet{pixels = few}
ok := small.load("t.sheet")
log("small", ok, small.reason())
log("png written", pc.imageWrite("/rinc/save/t.png", s.image()))
p := sheet.Sheet{pixels = pixels, flags = flagRoom[]}
log("png", p.load("/rinc/save/t.png"), p.size.x, p.size.y, p.sprite.x, p.flagsOf(0), pixels[3 * 18 + 5])
q := sheet.Sheet{pixels = pixels}
okNone := q.load("none.sheet")
okBad := q.load("game.rio")
log("refused", okNone, okBad)
room: Blob[..400]
fine := room.push(7) # (something there already)
adder := sheet.Sheet{pixels = room} # (each read's pixels after those there: loadAdding, then its image)
ok1 := adder.loadAdding("t.sheet")
a := adder.image()
ok2 := adder.loadAdding("/rinc/save/t.png")
b := adder.image()
log("added", a.size.x, a.size.y, b.size.x, room.len(), a.pixels[3 * 18 + 5], b.pixels[3 * 18 + 5], room[0])
madeRoom: Blob[..4]
fine = madeRoom.push(1) and madeRoom.push(2) and madeRoom.push(3) and madeRoom.push(4)
madeFlags: [4]Int
made := sheet.Sheet{pixels = madeRoom, flags = madeFlags[]}
made.shape({2, 2}, {1, 1})
made.nameFlag(0, "hot")
madeFlags[3] = 1
log("made", made.write("/rinc/save/made.sheet"), made.sprites)
''')
r = subprocess.run([rinc, os.path.join(work, 'game') + '/', '-data', os.path.join(work, 'data'), '-frames', '2'], capture_output=True, text=True, timeout=60)
out = (r.stdout + r.stderr).replace('\r', '')
lines = out.split('\n')
want = [
  'loaded true 18 9 9 9 2 2 3 1',          # (2 sprites of 9 by 9; 3 flag lines, 1 stranded: 4,4 starts none)
  'named 1 512 0 water 0',                  # (solid bit 0, water bit 9)
  'flags 1 513 0 0',                        # (sprite 1: solid and water; none past them)
  'pixels 162 8 9',                         # (pixel 5,3: (5+3)%16; 17,8: 25%16)
  'places 9 0 1 -1 -1',
  'written true',
  'as read true',                           # (byte for byte, the stranded record too)
  't.sheet: it needs pixels Blob[..162] (has 100)',
  'small false it needs pixels Blob[..162] (has 100)',
  'png written 0',
  'png true 18 9 8 0 8',                    # (a PNG: its pixels, sprites 8 by 8, no flags)
  'none.sheet: no such file',
  "game.rio: not a sheet of Paint's",
  'refused false false',
  'added 18 9 18 325 8 8 7',              # (two sheets in one room, after what was there: a sheet's, a PNG's)
  'made true 4']
bad = [w for w in want if w not in lines]
made = os.path.join(work, 'data', 'save', 'made.sheet') # (a sheet made, not read, as Paint makes one: its file)
want_made = b'rinc sheet 1\nsize 2 2\nsprite 1x1\nflag 0 hot\nmarks 1\npixels\n\x01\x02\x03\x04' + record(1, 1, 1)
if not os.path.exists(made) or open(made, 'rb').read() != want_made:
  bad.append('made: %r' % (open(made, 'rb').read() if os.path.exists(made) else None))
if bad: sys.exit('the sheet library: missing %r in:\n%s' % (bad, out))
print('the sheet library read, asked and wrote sheets')
