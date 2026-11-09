"""python shot.py path/to/rinc work-folder: Print Screen (typed as a terminal sends it, ESC [ 57361 u) saves
the screen as the app drew it, as /rinc/shots/<app>-<n>.png, the next number free: 320 by 240, its pixels
the screen's colors, color 0 solid (no tRNS chunk: a sprite sheet's is see-through, a screen's isn't)."""
import os, shutil, struct, subprocess, sys, zlib

rinc, work = os.path.abspath(sys.argv[1]), sys.argv[2]
shutil.rmtree(work, ignore_errors=True)
os.makedirs(os.path.join(work, 'snap'))
with open(os.path.join(work, 'snap', 'snap.rio'), 'w') as f:
  f.write('''
draw* :: proc()
  pc.cls(0)
  pc.rect({10, 20}, {30, 40}, 7)
end
''')
def run():
  r = subprocess.run([rinc, os.path.join(work, 'snap') + '/', '-data', os.path.join(work, 'data'), '-frames', '10', '-type', '\\x1b[57361u'],
                     capture_output=True, text=True, timeout=60)
  return (r.stdout + r.stderr).strip()
def chunks(b):
  at, out = 8, {}
  while at < len(b):
    n, t = struct.unpack('>I4s', b[at:at + 8])
    out[t] = out.get(t, b'') + b[at + 8:at + 8 + n]
    at += 12 + n
  return out
def fail(why): sys.exit('Print Screen: ' + why)

said = run()
path = os.path.join(work, 'data', 'shots', 'snap-1.png')
if not os.path.exists(path): fail('no %s saved (the run said %r)' % (path, said))
c = chunks(open(path, 'rb').read())
w, h, depth, kind = struct.unpack('>IIBB', c[b'IHDR'][:10])
if (w, h, depth, kind) != (320, 240, 8, 3): fail('the screen, 320 by 240, indexed: %r' % ((w, h, depth, kind),))
if b'tRNS' in c: fail('color 0 is see-through in a screen saved')
raw = zlib.decompress(c[b'IDAT'])
rows = [raw[y * 321 + 1:(y + 1) * 321] for y in range(240)] # (each row's filter byte, then its pixels)
if any(raw[y * 321] for y in range(240)): fail('rows saved unfiltered, as the writer does')
inside, outside = rows[30][20], rows[5][5]
if inside == outside or rows[59][39] != inside or rows[60][40] != outside or rows[19][10] != outside:
  fail('the pixels are the screen\'s: a rect at 10,20 of 30 by 40 (%d inside, %d outside)' % (inside, outside))
run()
if not os.path.exists(os.path.join(work, 'data', 'shots', 'snap-2.png')): fail('the next one, snap-2.png, not saved')
print('Print Screen saved the screen')
