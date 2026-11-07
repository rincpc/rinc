"""python colors.py path/to/rinc colors-folder data-folder: moving in Colors, each case typed into a fresh
Colors and checked in what it stashes (once it's rested half a second): the color. It starts at color 37
(red's sixth vivid step), 16 a row: up and down go a row, Tab between a row's halves."""
import os, shutil, subprocess, sys

rinc, colors, data = os.path.abspath(sys.argv[1]), sys.argv[2], sys.argv[3]
RIGHT, LEFT, DOWN, UP, TAB = '\x1b[C', '\x1b[D', '\x1b[B', '\x1b[A', '\t'

cases = [
  ('right: the next step', RIGHT, 38),
  ('down: the next hue, the same step', DOWN, 53),
  ('up twice: PICO-8\'s, its partners then 0 to 15', UP + UP, 5),
  ('left, past the row\'s start: its first', LEFT * 7, 32),
  ('Tab: muted, the same step', TAB, 45),
  ('Tab twice: vivid again', TAB + TAB, 37),
  ('down, past the grays: the last row', DOWN * 20, 245),
]

failed = 0
for name, typed, want in cases:
  shutil.rmtree(data, ignore_errors=True)
  r = subprocess.run([rinc, colors, '-data', data, '-frames', '60', '-type', typed], capture_output=True, text=True, timeout=60)
  path = os.path.join(data, 'stash', 'colors', 'pick')
  got = open(path, 'rb').read() if os.path.exists(path) else None
  if r.returncode or r.stdout.strip() or r.stderr.strip() or not got or got[0] != want:
    failed += 1
    print('FAIL %s: wanted %s, got %s %s' % (name, want, list(got) if got else None, (r.stdout + r.stderr).strip()))
  else:
    print('ok   %s' % name)
shutil.rmtree(data, ignore_errors=True)
print('%d of %d failed' % (failed, len(cases)) if failed else 'colors came through')
sys.exit(1 if failed else 0)
