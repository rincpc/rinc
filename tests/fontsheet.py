"""python fontsheet.py path/to/rinc work-folder: Rinc's own font, drawn from its sheet where it lies (font.c:
src/core/rinc-font.png's bytes, its cells as its notes say): every ASCII character printed in a grid, compact
(print's default, a line of 9: the rows above every ASCII glyph left out) and full (a line of 12), and Latin-1
ones as UTF-8, each pixel as the PNG has it (decoded here, apart from Rinc). Prints "fontsheet came through" """
import os, shutil, struct, subprocess, sys, zlib

rinc, work = os.path.abspath(sys.argv[1]), sys.argv[2]
here = os.path.dirname(os.path.abspath(__file__))
shutil.rmtree(work, ignore_errors=True)
os.makedirs(work)
b, i, c = open(os.path.join(here, '..', 'src', 'core', 'rinc-font.png'), 'rb').read(), 8, {}
while i < len(b):
    n, t = struct.unpack('>I4s', b[i:i + 8]); c[t] = c.get(t, b'') + b[i + 8:i + 8 + n]; i += 12 + n
w, h, depth = struct.unpack('>IIB', c[b'IHDR'][:9])
notes = dict(l.split(' ', 1) for l in c[b'tEXt'].split(b'\0', 1)[1].decode().splitlines() if ' ' in l)
cw, ch = map(int, notes['sprite'].split('x'))
first, last = map(int, notes['font'].split())
across = int(notes['across'])
rows, rb = zlib.decompress(c[b'IDAT']), (w * depth + 7) // 8
def bit(ch_, x, y):
    if ch_ <= first or ch_ > last: return 0
    g = ch_ - first; sx, sy = g % across * cw + x, g // across * ch + y
    return rows[sy * (rb + 1) + 1 + sx // 8] >> (7 - sx % 8) & 1
top = min(y for k in range(33, 127) for y in range(ch) for x in range(cw) if bit(k, x, y))
chars = list(range(33, 127)) + [0xC9, 0xE9, 0xF1, 0xA9] # (É é ñ ©: UTF-8, two bytes each)
app = os.path.join(work, 'grid.rio')
src = ['pc.cls(1)', 'line: Blob[..16]', 'chars := []Int{%s}' % ', '.join(map(str, chars)), 'for i in 0..<chars.len()',
       '  line.clear()', '  k := chars[i]', '  if k < 128', '    _ = line.push(k)', '  else',
       '    _ = line.push(192 + k / 64) and line.push(128 + k % 64)', '  end',
       '  pc.print(line.asString(), {i % 32 * 6, i / 32 * 9}, 7)',
       '  pc.print(line.asString(), {i % 32 * 6, 60 + i / 32 * 12}, 7, {font = {line = 12}})', 'end',
       'update* :: proc() # (running on: no "(done)" over it)', 'end', '']
open(app, 'w', newline='\n').write('\n'.join(src))
shot = os.path.join(work, 'grid.ppm')
out = subprocess.run([rinc, app, '-data', os.path.join(work, 'data'), '-frames', '1', '-o', shot], capture_output=True, text=True, timeout=60)
px = open(shot, 'rb').read()[-320 * 240 * 3:]
bad = []
for n, k in enumerate(chars):
    for y0, cut, tall in ((n // 32 * 9, top, ch - top), (60 + n // 32 * 12, 0, ch)):
        for y in range(tall):
            for x in range(cw + 1):
                sx, sy = n % 32 * 6 + x, y0 + y
                lit = px[(sy * 320 + sx) * 3:(sy * 320 + sx) * 3 + 3] != px[0:3] # (not the background)
                if lit != bool(x < cw and bit(k, x, y + cut)): bad.append((chr(k), cut, x, y))
print('fontsheet came through' if not bad else 'FAIL: %d pixels differ, first %r %s' % (len(bad), bad[:4], out.stdout[-300:]))
