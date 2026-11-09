"""python tilemap.py path/to/rinc path/to/library data-folder: library/tilemap.rio, as an app imports it from
/rinc/lib: tests/tilemap.rio's asserts, then its screen: a map's two layers drawn from their sheet."""
import os, shutil, struct, subprocess, sys, zlib

rinc, library, data = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2]), sys.argv[3]
here = os.path.dirname(os.path.abspath(__file__))
shutil.rmtree(data, ignore_errors=True)
os.makedirs(os.path.join(data, 'lib'))
shutil.copy(os.path.join(library, 'tilemap.rio'), os.path.join(data, 'lib'))
os.makedirs(os.path.join(data, 'apps', 'tilemaptest'))
shutil.copy(os.path.join(here, 'tilemap.rio'), os.path.join(data, 'apps', 'tilemaptest', 'tilemaptest.rio'))
red, green = (0xFF, 0x00, 0x4D), (0x00, 0xE4, 0x36) # (PICO-8's 8 and 11: sprite 0 red, 1 green)
def png(w, h, color_of):
  def chunk(t, d): return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
  raw = b''.join(b'\x00' + b''.join(bytes(color_of(x, y)) for x in range(w)) for y in range(h))
  return b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b'')
os.makedirs(os.path.join(data, 'save'))
open(os.path.join(data, 'save', 't.png'), 'wb').write(png(16, 8, lambda x, y: red if x < 8 else green))
shot = os.path.join(data, 'shot.ppm')
r = subprocess.run([rinc, '-data', data, '-app', 'tilemaptest', '-frames', '3', '-o', shot], capture_output=True, text=True, timeout=60)
said = (r.stdout + r.stderr).strip()
if 'tilemap came through' not in said or 'assert' in said or 'error' in said:
  sys.exit('FAIL: %s' % said)
pixels = open(shot, 'rb').read()[-320 * 240 * 3:]
def pixel(x, y): return tuple(pixels[(y * 320 + x) * 3:(y * 320 + x) * 3 + 3])
def near(a, b): return all(abs(p - q) <= 8 for p, q in zip(a, b))
white = (0xFF, 0xF1, 0xE8)
checks = [((2, 2), red, 'layer 0, cell 0,0: sprite 0'), ((10, 2), green, 'cell 1,0: sprite 1'), ((18, 2), None, 'cell 2,0: empty'),
          ((2, 10), white, 'cell 0,1: past the sheet, a box'), ((10, 10), green, 'cell 1,1'), ((6, 18), green, 'layer 1, 4 right and 16 down: sprite 1'),
          ((1, 102), red, 'again, 4 on: cell 0,0 half off the left, its right half there'), ((4, 102), green, 'and cell 1,0 from x 4'), ((12, 102), None, 'cell 2,0 empty, from 12')]
bad = [(name, pixel(*at)) for at, want, name in checks if (want is None and (near(pixel(*at), red) or near(pixel(*at), green))) or (want is not None and not near(pixel(*at), want))]
if bad: sys.exit('FAIL: drawn: %s' % bad)
print('tilemap came through')
