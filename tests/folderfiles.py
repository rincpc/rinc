"""python folderfiles.py path/to/rinc work-folder: an app run from its folder (rinc game/, its other files beside
it) reads them there by their paths in it, as it would installed: pc.fileRead, pc.fileReadAt and
pc.imageRead (a PNG, read a part at a time); one that isn't there, -1; and storage's copy of the app
(installed before) isn't what it reads."""
import os, shutil, struct, subprocess, sys, zlib

rinc, work = os.path.abspath(sys.argv[1]), sys.argv[2]
shutil.rmtree(work, ignore_errors=True)
def put(path, data):
  path = os.path.join(work, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'wb') as f: f.write(data.encode() if isinstance(data, str) else data)
def png(w, h, rows): # (indexed, 8 bits: two colors, a checkerboard)
  chunk = lambda t, d: struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d))
  return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 3, 0, 0, 0)) + chunk(b'PLTE', bytes(3) + bytes([255, 255, 255]))
          + chunk(b'IDAT', zlib.compress(b''.join(b'\0' + bytes(r) for r in rows))) + chunk(b'IEND', b''))

put('game/own.txt', 'its own, beside it')
put('game/art/dots.png', png(3, 2, [[1, 0, 1], [0, 1, 0]]))
put('data/apps/game/own.txt', 'storage\'s copy') # (installed before: not the one it's run from)
put('game/game.rio', '''
b: Blob[..40]
log("read", pc.fileRead("own.txt", b), b.asString())
b.clear()
log("part", pc.fileReadAt("own.txt", 4, b), b.asString())
px: Blob[..(3 + 1) * 2]
log("image", pc.imageRead("art/dots.png", px).size, px[0] == px[2] and px[2] == px[4] and px[1] == px[3] and px[0] != px[1])
log("none", pc.fileRead("none.txt", b))
''')
r = subprocess.run([rinc, os.path.join(work, 'game') + '/', '-data', os.path.join(work, 'data'), '-frames', '1'], capture_output=True, text=True, timeout=60)
out = (r.stdout + r.stderr).replace('\r', '')
want = ['read 18 its own, beside it', 'part 18 own, beside it', 'image [3, 2] true', 'none -1']
bad = [w for w in want if w not in out.split('\n')]
if bad:
  sys.exit('an app run from its folder reads its files there: missing %r in:\n%s' % (bad, out))
print('an app run from its folder read its own files there')
