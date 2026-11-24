"""python transform.py path/to/rinc work-folder: pc's transform (transform, transformBy, and the helpers),
the screen read back (-o): plain, as without one; moved, as drawn there; a quarter turn, a rect's width
and height swapped; nested (moved, then turned) the same as turned around that point; the one replaced
given back, put back, the shapes after as without one; and plain again at each frame's start (one left
set at a frame's end: the next frame's first shape where it'd be without)."""
import os, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
shutil.rmtree(work, ignore_errors=True)
os.makedirs(os.path.join(work, 'apps', 'tm'))
open(os.path.join(work, 'apps', 'tm', 'tm.rio'), 'w', newline='\n').write('''pix: Blob[12]
img := pc.Image{pixels = pix[], size = {4, 3}}
update* :: proc()
  pc.cls(0)
  pc.rect({200, 100}, {6, 6}, 8)
  for i in 0..<12
    pix[i] = 8 + i
  end
  pix[5] = 0
  pc.blit(img, {10, 150})
  _ = pc.transform(pc.moved({20.0, 0.0}))
  pc.blit(img, {10, 150})
  _ = pc.transform(pc.plain())
  pc.blit(img, {60, 150}, {scale = {2, 2}})
  _ = pc.transform(pc.scaled({2.0, 2.0}))
  pc.blit(img, {45, 75})
  _ = pc.transform(pc.turnedAround(1.5707964, {122.0, 151.0}))
  pc.blit(img, {120, 150})
  _ = pc.transform(pc.moved({300.0, 0.0}))
  pc.blit(img, {-150, 150}, {scale = {-1, 1}})
  _ = pc.transform(pc.plain())
  pc.print("Ab", {10, 190}, 7)
  _ = pc.transform(pc.moved({30.0, 0.0}))
  pc.print("Ab", {10, 190}, 7)
  _ = pc.transform(pc.turnedAround(0.5, {100.0, 195.0}))
  pc.print("Ab", {100, 190}, 7)
  _ = pc.transform(pc.plain())
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
def colors(x0, y0, x1, y1): return {(x - x0, y - y0): px[(y * W + x) * 3:(y * W + x) * 3 + 3] for y in range(y0, y1) for x in range(x0, x1) if on(x, y)}
plain = colors(10, 150, 14, 153)
check(len(plain) == 11 and colors(30, 150, 34, 153) == plain, 'blit, moved: as drawn there (its see-through pixel too)')
check(colors(90, 150, 98, 156) == colors(60, 150, 68, 156) and len(colors(60, 150, 68, 156)) == 44, "blit, scaled by the transform: as blit's own scale")
rot = {}
for (u, v), c in plain.items(): rot[(122 - v, 149 + u)] = c # (pixel u, v's center, 120 + u + 0.5, 150 + v + 0.5, a quarter turn around 122, 151)
got = {(x, y): px[(y * W + x) * 3:(y * W + x) * 3 + 3] for y in range(140, 165) for x in range(110, 135) if on(x, y)}
check(got == rot, 'blit, a quarter turn around its middle: each pixel where it turns to (%d of %d)' % (len(set(got.items()) & set(rot.items())), len(rot)))
mirror = colors(150, 150, 154, 153)
check(mirror == {(3 - u, v): c for (u, v), c in plain.items()}, 'blit, mirrored (its own scale -1) and moved: each pixel mirrored')
text = drawn(8, 186, 30, 200)
check(text and {(x + 30, y) for x, y in text} == drawn(38, 186, 60, 200), 'print, moved: as drawn there')
check(len(drawn(95, 180, 125, 215)) > 10, 'print, turned: drawn')
print('%d failed' % failed if failed else 'the transforms came through')
sys.exit(1 if failed else 0)
