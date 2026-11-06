"""imageRead's room: PNGs of several kinds (indexed at 8, 4 and 1 bits, RGB, gray and alpha), each read
with exactly the room it needs ((w + 1) * h for indexed ones; each row's bytes and its filter byte, times h,
for the rest) and with a byte less (its size, no pixels, and a line saying so): tests/imageroom.py <rinc>
<data dir>"""
import os, shutil, struct, subprocess, sys, zlib

def chunk(t, d):
  return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)

def png(w, h, depth, kind, rows, plte=None):
  raw = b''.join(bytes([i % 5]) + r for i, r in enumerate(rows))  # (each filter, in turn)
  # filters applied for real: encode each row with its filter so decoding must undo it
  out = b''
  prev = bytes(len(rows[0]))
  bpp = max(1, {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[kind] * depth // 8)
  for i, r in enumerate(rows):
    f = i % 5
    enc = bytearray()
    for x in range(len(r)):
      a = r[x - bpp] if x >= bpp else 0
      u = prev[x]
      c = prev[x - bpp] if x >= bpp else 0
      if f == 0: p = 0
      elif f == 1: p = a
      elif f == 2: p = u
      elif f == 3: p = (a + u) // 2
      else:
        pa, pb, pc = abs(u - c), abs(a - c), abs(a + u - 2 * c)
        p = a if pa <= pb and pa <= pc else u if pb <= pc else c
      enc.append((r[x] - p) & 255)
    out += bytes([f]) + bytes(enc)
    prev = r
  d = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, depth, kind, 0, 0, 0))
  if plte: d += chunk(b'PLTE', plte)
  return d + chunk(b'IDAT', zlib.compress(out)) + chunk(b'IEND', b'')

rinc, data = sys.argv[1], sys.argv[2]
shutil.rmtree(data, ignore_errors=True)
app = os.path.join(data, 'imageroom.rio')
folder = os.path.join(data, 'save', 'k')
os.makedirs(folder, exist_ok=True)
w, h = 10, 7
plte = b''.join(bytes([v, v, v]) for v in range(0, 256, 16))  # (16 grays)
kinds = {
  'i8': (8, 3, [bytes((x + y) % 16 for x in range(w)) for y in range(h)], plte, w + 1),
  'i4': (4, 3, [bytes(((2 * x + y) % 16) << 4 | (2 * x + 1 + y) % 16 for x in range(w // 2)) for y in range(h)], plte, w + 1),
  'i1': (1, 3, [bytes([0b10110010 ^ y, 0b11000000]) for y in range(h)], plte, w + 1),
  'rgb': (8, 2, [bytes(v for x in range(w) for v in (x * 20, y * 30, 7)) for y in range(h)], None, 3 * w + 1),
  'ga': (8, 4, [bytes(v for x in range(w) for v in (x * 25, 255 if x % 3 else 0)) for y in range(h)], None, 2 * w + 1),
}
lines = []
for name, (depth, kind, rows, pl, rowNeed) in kinds.items():
  open(os.path.join(folder, name + '.png'), 'wb').write(png(w, h, depth, kind, rows, pl))
  need = rowNeed * h
  lines.append(f'''a: Blob[..{need}]
i := pc.imageRead("/rinc/save/k/{name}.png", a)
b: Blob[..{need - 1}]
j := pc.imageRead("/rinc/save/k/{name}.png", b)
sum := 0
for k in 0..<i.pixels.len() do sum = (sum * 31 + i.pixels[k]) % 1000003
log("{name}", i.size, i.pixels.len(), j.size, j.pixels.len(), sum)'''.replace('\n', '\nend\n', 0))
src = ''
for n, l in enumerate(lines):
  src += f'p{n} :: proc()\n' + ''.join('  ' + s + '\n' for s in l.split('\n')) + f'end\np{n}()\n'
open(app, 'w').write(src)
out = subprocess.run([rinc, '-data', data, app, '-frames', '1'], capture_output=True, text=True, timeout=30).stdout.replace('\r', '')
got = {l.split()[0]: l for l in out.split('\n') if l and not l.startswith('imageRead:')}
sums = {k: v.split()[-1] for k, v in got.items()}
for k in kinds:
  if k not in got or not got[k].startswith(k + ' [10, 7] 70 [10, 7] 0 '): sys.exit('FAIL: %s: %r' % (k, got.get(k)))
  if 'imageRead: /rinc/save/k/%s.png needs' % k not in out: sys.exit('FAIL: %s: no line saying it needs more room' % k)
if sums['i8'] != sums['i4']: sys.exit('FAIL: the same picture at 8 and 4 bits came out different')
print('image room came through')