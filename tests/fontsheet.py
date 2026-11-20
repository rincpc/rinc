"""python fontsheet.py path/to/rinc work-folder: Rinc's own font, drawn from its sheet where it lies (font.c:
src/core/rinc-font.png's bytes): every character 32 to 126 printed in a grid, each pixel as the PNG has it
(decoded here, apart from Rinc). Prints "fontsheet came through" """
import os, shutil, struct, subprocess, sys, zlib

rinc, work = os.path.abspath(sys.argv[1]), sys.argv[2]
here = os.path.dirname(os.path.abspath(__file__))
shutil.rmtree(work, ignore_errors=True)
os.makedirs(work)
b, i, c = open(os.path.join(here, '..', 'src', 'core', 'rinc-font.png'), 'rb').read(), 8, {}
while i < len(b):
    n, t = struct.unpack('>I4s', b[i:i + 8]); c[t] = c.get(t, b'') + b[i + 8:i + 8 + n]; i += 12 + n
w, h, depth = struct.unpack('>IIB', c[b'IHDR'][:9])
rows, rb = zlib.decompress(c[b'IDAT']), (w * depth + 7) // 8
def bit(x, y): return rows[y * (rb + 1) + 1 + x // 8] >> (7 - x % 8) & 1
app = os.path.join(work, 'grid.rio')
open(app, 'w', newline='\n').write('''pc.cls(1)
line: Blob[..16]
for ch in 32..<127
  line.clear()
  _ = line.push(ch)
  pc.print(line.asString(), {(ch - 32) % 16 * 6, (ch - 32) / 16 * 9}, 7)
end
update* :: proc() # (running on: no "(done)" over it)
end
''')
shot = os.path.join(work, 'grid.ppm')
subprocess.run([rinc, app, '-data', os.path.join(work, 'data'), '-frames', '1', '-o', shot], capture_output=True, timeout=60)
px = open(shot, 'rb').read()[-320 * 240 * 3:]
bad = []
for ch in range(32, 127):
    g = ch - 32
    for y in range(8): # (print draws a glyph's 8 rows of its 9)
        for x in range(6):
            sx, sy = g % 16 * 6 + x, g // 16 * 9 + y
            lit = px[(sy * 320 + sx) * 3:(sy * 320 + sx) * 3 + 3] != px[0:3] # (not the background)
            if lit != bool(bit(sx, sy)): bad.append((chr(ch), x, y))
print('fontsheet came through' if not bad else 'FAIL: %d pixels differ, first %r' % (len(bad), bad[:4]))
