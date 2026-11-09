"""python bundleimage.py path/to/rinc work-folder: an app's picture beside it, exported with it (rinc game/
-export), read in the exported game from its bundle: pc.imageRead and pc.imageNotes, as from storage
(a game's sprite sheet, as Paint saves it, where there's no card's copy). On Windows, a game exported
to a name with no extension is name.exe."""
import os, shutil, struct, subprocess, sys, zlib

rinc, work = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
shutil.rmtree(work, ignore_errors=True)
def put(path, data):
  path = os.path.join(work, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'wb') as f: f.write(data.encode() if isinstance(data, str) else data)
def png(w, h, rows, notes): # (indexed, 8 bits: two colors, a checkerboard; its notes in a tEXt chunk named Rinc)
  chunk = lambda t, d: struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d))
  return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 3, 0, 0, 0)) + chunk(b'PLTE', bytes(3) + bytes([255, 255, 255]))
          + chunk(b'tEXt', b'Rinc\0' + notes) + chunk(b'IDAT', zlib.compress(b''.join(b'\0' + bytes(r) for r in rows))) + chunk(b'IEND', b''))

put('sheet/dots.png', png(3, 2, [[1, 0, 1], [0, 1, 0]], b'sprite 8'))
put('sheet/files.txt', 'sheet.rio\ndots.png\n')
put('sheet/sheet.rio', '''
px: Blob[..(3 + 1) * 2]
notes: Blob[..20]
log("image", pc.imageRead("dots.png", px).size, px[0] == px[2] and px[1] == px[3] and px[0] != px[1], pc.imageNotes("dots.png", notes), notes.asString())
''')
game = os.path.join(work, 'sheet-game') # (on Windows, made sheet-game.exe: a name with no extension)
r = subprocess.run([rinc, os.path.join(work, 'sheet') + '/', '-export', game], capture_output=True, text=True, timeout=60)
if r.returncode != 0: sys.exit('the export: %s' % (r.stdout + r.stderr))
if os.name == 'nt':
  if not os.path.exists(game + '.exe'): sys.exit('exported on Windows, a name with no extension: %s.exe, not %s' % (game, os.listdir(work)))
  game += '.exe'
r = subprocess.run([game, '-frames', '1', '-data', os.path.join(work, 'data')], capture_output=True, text=True, timeout=60)
out = (r.stdout + r.stderr).replace('\r', '')
if 'image [3, 2] true 8 sprite 8' not in out.split('\n'):
  sys.exit("an exported game's picture, read from its bundle: wanted 'image [3, 2] true 8 sprite 8' in:\n%s" % out)
print("an exported game's picture read from its bundle")
