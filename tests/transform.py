"""python transform.py path/to/rinc work-folder: pc's transform (transform, transformBy, and the helpers),
the screen read back (-o): plain, as without one; moved, as drawn there; a quarter turn, a rect's width
and height swapped; nested (moved, then turned) the same as turned around that point; the one replaced
given back, put back, the shapes after as without one; and plain again at each frame's start (one left
set at a frame's end: the next frame's first shape where it'd be without)."""
import os, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
shutil.rmtree(work, ignore_errors=True)
os.makedirs(os.path.join(work, 'apps', 'tm'))
open(os.path.join(work, 'apps', 'tm', 'tm.rio'), 'w', newline='\n').write('''update* :: proc()
  pc.cls(0)
  pc.rect({200, 100}, {6, 6}, 8)
  _ = pc.transform(pc.plain())
  pc.rect({2, 2}, {10, 6}, 8)
  _ = pc.transform(pc.moved({30.0, 0.0}))
  pc.rect({2, 2}, {10, 6}, 8)
  _ = pc.transform(pc.turnedAround(1.5707964, {64.5, 5.5}))
  pc.rect({60, 4}, {10, 4}, 8)
  _ = pc.transform(pc.moved({100.0, 30.0}))
  _ = pc.transformBy(pc.turned(0.6))
  pc.triangle({0, 0}, {20, 0}, {0, 12}, 8)
  _ = pc.transform(pc.turnedAround(0.6, {100.0, 70.0}))
  pc.triangle({100, 70}, {120, 70}, {100, 82}, 8)
  _ = pc.transform(pc.plain())
  was := pc.transform(pc.scaled({3.0, 3.0}))
  _ = pc.transform(was)
  pc.ellipse({150, 2}, {12, 8}, 8)
  _ = pc.transform(pc.moved({0.0, 50.0})) # (left set: the next frame's first rect is plain only if it's put back)
end
''')
ppm = os.path.join(work, 's.ppm')
r = subprocess.run([rinc, '-data', work, '-app', 'tm', '-frames', '3', '-o', ppm], capture_output=True, text=True, timeout=60)
if not os.path.exists(ppm): sys.exit('transform: no screen (%d): %s' % (r.returncode, r.stdout + r.stderr))
W, H = 320, 240
px = open(ppm, 'rb').read()[-W * H * 3:]
bg = px[0:3]
def on(x, y): return 0 <= x < W and 0 <= y < H and px[(y * W + x) * 3:(y * W + x) * 3 + 3] != bg
def drawn(x0, y0, x1, y1): return {(x, y) for y in range(y0, y1) for x in range(x0, x1) if on(x, y)}
failed = 0
def check(ok, why):
  global failed
  print(('ok   ' if ok else 'FAIL ') + why)
  failed += not ok
box = {(x, y) for x in range(2, 12) for y in range(2, 8)}
check(drawn(0, 0, 20, 12) == box, 'plain: as without one')
check(drawn(28, 0, 48, 12) == {(x + 30, y) for x, y in box}, 'moved: as drawn there')
q = drawn(54, 0, 76, 14)
xs, ys = {x for x, y in q}, {y for x, y in q}
check(q and max(xs) - min(xs) + 1 == 4 and max(ys) - min(ys) + 1 == 10, 'a quarter turn: a 10 by 4 rect 4 by 10 (%r)' % (((min(xs), max(xs), min(ys), max(ys)) if q else None),))
nested = drawn(70, 20, 140, 60)
around = {(x, y - 40) for x, y in drawn(70, 60, 140, 100)}
check(nested and nested == around, 'nested (moved, then turned) the same as turned around that point')
check(drawn(146, 0, 166, 14) == {(x, y) for y in range(2, 10) for x in range(150, 162) if on(x, y)} and on(155, 6) and not on(149, 6) and not on(162, 6), 'the one replaced given back and put back: the ellipse after, as without one')
check(on(202, 102) and not on(202, 152), "plain again at each frame's start: one left set at each frame's end, the next frame's first rect where it'd be without")
print('%d failed' % failed if failed else 'the transforms came through')
sys.exit(1 if failed else 0)
