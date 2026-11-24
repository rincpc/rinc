"""python shapes.py path/to/rinc work-folder: pc's shapes drawn (rect, with an outline and rounded corners;
ellipse; triangle; line; curve), each in a corner of its own, the screen read back (-o): the pixels each drew.
A rectangle's outline, its edge alone; rounded corners left out; an ellipse and a circle the same each
way they're turned, their outlines inside their fills; a triangle's corners in it, its outline inside it;
lines with both ends drawn, each step one pixel, rising ones too (one stepped past its end before); a
line from far off the screen to far off it, clipped (the row it crosses, whole)."""
import os, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
shutil.rmtree(work, ignore_errors=True)
os.makedirs(os.path.join(work, 'apps', 'shapes'))
open(os.path.join(work, 'apps', 'shapes', 'shapes.rio'), 'w', newline='\n').write('''update* :: proc()
  pc.cls(0)
  pc.rect({2, 2}, {10, 6}, 8)
  pc.rect({20, 2}, {10, 6}, 8, {outline = true})
  pc.rect({40, 2}, {12, 12}, 8, {radius = 4})
  pc.ellipse({60, 2}, {13, 9}, 8)
  pc.ellipse({80, 2}, {13, 9}, 8, {outline = true})
  pc.ellipse({100, 2}, {11, 11}, 8)
  pc.triangle({2, 40}, {30, 30}, {20, 60}, 8)
  pc.triangle({40, 40}, {68, 30}, {58, 60}, 8, {outline = true})
  pc.line({90, 64}, {50, 90}, 8)
  pc.line({100, 70}, {140, 70}, 8)
  pc.line({-1000000, 200}, {1000000, 200}, 8)
  pc.curve({160, 10}, {170, 10}, {180, 10}, {190, 10}, 8)
  pc.curve({200, 60}, {210, 20}, {250, 20}, {260, 60}, 8)
  pc.curve({280, 30}, {280, 30}, {280, 30}, {280, 30}, 8)
  pc.curve({-90000, 150}, {50000, -90000}, {90000, 90000}, {-5000, 150}, 8)
end
''')
ppm = os.path.join(work, 's.ppm')
r = subprocess.run([rinc, '-data', work, '-app', 'shapes', '-frames', '3', '-o', ppm], capture_output=True, text=True, timeout=60)
failed = 0
def check(ok, why):
  global failed
  print(('ok   ' if ok else 'FAIL ') + why)
  failed += not ok
if not os.path.exists(ppm):
  sys.exit('shapes: no screen (%d): %s' % (r.returncode, r.stdout + r.stderr))
data = open(ppm, 'rb').read()
W, H = 320, 240
px = data[-W * H * 3:]
bg = px[0:3]
def on(x, y): return 0 <= x < W and 0 <= y < H and px[(y * W + x) * 3:(y * W + x) * 3 + 3] != bg
def drawn(x0, y0, x1, y1): return {(x, y) for y in range(y0, y1) for x in range(x0, x1) if on(x, y)}

check(drawn(0, 0, 16, 10) == {(x, y) for x in range(2, 12) for y in range(2, 8)}, 'rect, filled: as ever')
edge = {(x, y) for x in range(20, 30) for y in range(2, 8) if x in (20, 29) or y in (2, 7)}
check(drawn(18, 0, 34, 10) == edge, 'rect, its outline: its edge alone')
rr = drawn(38, 0, 56, 16)
check(not on(40, 2) and not on(51, 2) and not on(40, 13) and on(46, 2) and on(40, 8) and on(46, 8) and rr == {(91 - x, y) for (x, y) in rr},
      'rect, rounded: its corners left out, its middle in, the same turned left to right')
e = drawn(58, 0, 76, 14)
mirror = lambda s, cx2, cy2: {(cx2 - x, cy2 - y) for (x, y) in s}
check(e and e == mirror(e, 60 + 60 + 12, 2 + 2 + 8) and on(66, 6) and not on(60, 2), 'ellipse, filled: the same each way it\'s turned, its middle in, its box\'s corners out')
o = drawn(78, 0, 96, 14)
check(o and o <= {(x + 20, y) for (x, y) in e} and not on(86, 6) and o == mirror(o, 80 + 80 + 12, 2 + 2 + 8), 'ellipse, its outline: inside its fill, its middle empty, the same each way it\'s turned')
c = drawn(98, 0, 114, 16)
check(c and c == {(y - 2 + 100, x - 100 + 2) for (x, y) in c}, 'a circle (a square box): the same turned a quarter')
t = drawn(0, 28, 34, 64)
check(all(on(x, y) for x, y in ((2, 40), (30, 30), (20, 60))) and len(t) > 200, 'triangle, filled: its corners in it')
to = drawn(38, 28, 72, 64)
near = {(x + dx, y + dy) for (x, y) in t for dx in (-1, 0, 1) for dy in (-1, 0, 1)} # (a line's pixels the nearest to it: half a pixel out, at most)
check(to and {(x - 38, y) for (x, y) in to} <= near and not on(55, 42), 'triangle, its outline: on the filled one\'s edge, its middle empty')
ln = drawn(46, 61, 96, 94)
check(on(90, 64) and on(50, 90) and len(ln) == 41 and len({x for x, y in ln}) == 41, 'a line rising (once stepped past its end): both ends, a pixel a column')
check(drawn(98, 68, 144, 72) == {(x, 70) for x in range(100, 141)}, 'a level line: both ends drawn')
check(drawn(0, 199, W, 202) == {(x, 200) for x in range(W)}, 'a line from far off the screen to far off it: clipped, the row it crosses whole')
check(drawn(150, 5, 200, 15) == {(x, 10) for x in range(160, 191)}, 'a curve, its control points on a line: that line')
arch = drawn(196, 16, 266, 66)
def connected(pts):
  pts = set(pts); seen = set(); todo = [next(iter(pts))] if pts else []
  while todo:
    q = todo.pop()
    if q in seen: continue
    seen.add(q)
    todo += [(q[0] + dx, q[1] + dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1) if (q[0] + dx, q[1] + dy) in pts]
  return seen == pts
check(on(200, 60) and on(260, 60) and connected(arch) and all(any((460 - x + dx, y + dy) in arch for dx in (-1, 0, 1) for dy in (-1, 0, 1)) for (x, y) in arch) and all(200 <= x <= 260 and 20 <= y <= 60 for x, y in arch) and not on(230, 50),
      'a curve, an arch: both ends, no gaps, turned left to right the same within a pixel (halves round one way), inside its control points\' box, its middle empty')
check(drawn(276, 26, 286, 36) == {(280, 30)}, 'a curve of no length: its pixel')
print('%d failed' % failed if failed else 'the shapes came through')
sys.exit(1 if failed else 0)
