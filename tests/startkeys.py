"""python startkeys.py path/to/rinc work-folder: what's held as an app starts (the key that started it,
from home, say) isn't the app's till it's let go: an app that starts another while A is held (as home
does on A or Enter), A not seen by the new one (btn, btnp) while it's held; pressed again after, seen.
And a game run without a window still gets its start: A held from its first frame."""
import os, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
shutil.rmtree(work, ignore_errors=True)
for name, src in (('held', 'n := 0\nupdate* :: proc()\n  if pc.btn(.a) == 1 or pc.btnp(.a) == 1\n    log("a", n, pc.btn(.a), pc.btnp(.a))\n  end\n  n += 1\nend\n'),
                  ('starter', 'n := 0\nupdate* :: proc()\n  n += 1\n  if n == 3\n    pc.launch("held")\n  end\nend\n')):
  os.makedirs(os.path.join(work, 'apps', name))
  open(os.path.join(work, 'apps', name, name + '.rio'), 'w', newline='\n').write(src)
def run(app, *more):
  r = subprocess.run([rinc, '-data', work, '-app', app, '-frames', '30'] + list(more), capture_output=True, text=True, timeout=60)
  return [l for l in (r.stdout + r.stderr).splitlines() if l.startswith('a ')]
failed = 0
def check(ok, why):
  global failed
  print(('ok   ' if ok else 'FAIL ') + why)
  failed += not ok
got = run('starter', '-hold', 'a 0-12')
check(got == [], 'started with A held (as from home): not its, while held (%r)' % got)
got = run('starter', '-hold', 'a 0-12, a 20-22')
check(len(got) == 3 and got[0].split()[2:] == ['1', '1'], 'started with A held, let go, pressed again: that press seen (%r)' % got)
got = run('held')
check(got[:1] == ['a 0 1 1'] and len(got) == 3, 'a game run without a window: its start, A held from its first frame (%r)' % got)
print('%d failed' % failed if failed else 'the start keys came through')
sys.exit(1 if failed else 0)
