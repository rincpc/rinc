"""python colors.py path/to/rinc colors-folder data-folder: Colors' views, each case typed into a fresh
Colors and checked in what it stashes (once it's rested half a second): the color, the cube's turn,
where in the cube's view (0 PICO-8's, 1 the grays, 2 the cube), and the view (0 the cube, 1 by hue).
It starts at color 221 (red 5, green 1, blue 3), in the cube."""
import os, shutil, subprocess, sys

rinc, colors, data = os.path.abspath(sys.argv[1]), sys.argv[2], sys.argv[3]
RIGHT, DOWN, UP = '\\x1b[C', '\\x1b[B', '\\x1b[A'
COLOR, AREA, VIEW = 0, 2, 3

cases = [
  ('V: by hue, at the same color', 'v', {VIEW: 1, COLOR: 221}),
  ('V twice: the cube again', 'vv', {VIEW: 0, COLOR: 221, AREA: 2}),
  ('by hue, right: the nearest that way, PICO-8 14', 'v' + RIGHT, {VIEW: 1, COLOR: 14}),
  ('back in the cube, at that color: among PICO-8\'s', 'v' + RIGHT + 'v', {VIEW: 0, COLOR: 14, AREA: 0}),
  ('by hue, up from 221: the cell above, 191', 'v' + UP, {COLOR: 191}),
  ('by hue, down from 221: the cell below, 216', 'v' + DOWN, {COLOR: 216}),
  ('the cube still moves as it did', RIGHT + DOWN, {VIEW: 0, COLOR: 32 + 5 * 36 + 2 * 6 + 4}),
]

failed = 0
for name, typed, want in cases:
  shutil.rmtree(data, ignore_errors=True)
  r = subprocess.run([rinc, colors, '-data', data, '-frames', '60', '-type', typed], capture_output=True, text=True, timeout=60)
  path = os.path.join(data, 'stash', 'colors', 'pick')
  got = open(path, 'rb').read() if os.path.exists(path) else None
  if r.returncode or r.stdout.strip() or r.stderr.strip() or got is None or any(len(got) <= k or got[k] != v for k, v in want.items()):
    failed += 1
    print('FAIL %s: wanted %s, got %s %s' % (name, want, list(got) if got else None, (r.stdout + r.stderr).strip()))
  else:
    print('ok   %s' % name)
shutil.rmtree(data, ignore_errors=True)
print('%d of %d failed' % (failed, len(cases)) if failed else 'colors came through')
sys.exit(1 if failed else 0)
