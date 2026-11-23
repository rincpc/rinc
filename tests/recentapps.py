"""python recentapps.py path/to/rinc work-folder: the recent apps (Sys+Tab's) kept in rinc's own stash
(/stash/rinc/recent), so they're there after a restart: two apps started, each a run of its own, both
listed, the latest first; an app deleted since, left out; a kept list that's garbage (bytes, long lines)
neither stops an app starting nor stays; and the system key with Q at home forgets them (but home)."""
import os, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
recent = os.path.join(work, 'stash', 'rinc', 'recent')
failed = 0
def check(ok, why):
  global failed
  print(('ok   ' if ok else 'FAIL ') + why)
  failed += not ok
def run(app, keys=''):
  r = subprocess.run([rinc, '-data', work, '-app', app, '-frames', '20'] + (['-type', keys] if keys else []), capture_output=True, text=True, timeout=60)
  return (r.stdout + r.stderr).strip()
def kept():
  return open(recent, 'rb').read().decode('latin-1') if os.path.exists(recent) else None

shutil.rmtree(work, ignore_errors=True)
for name in ('one', 'two', 'three'):
  os.makedirs(os.path.join(work, 'apps', name))
  open(os.path.join(work, 'apps', name, name + '.rio'), 'w', newline='\n').write('log("%s runs")\nupdate* :: proc()\nend\n' % name)

said = run('one') + run('two')
check(kept() == 'two\none\n', 'two apps, a run each (a restart between): both kept, the latest first (%r %s)' % (kept(), said))
shutil.rmtree(os.path.join(work, 'apps', 'one'))
said = run('three')
check(kept() == 'three\ntwo\n', 'an app deleted since: left out (%r %s)' % (kept(), said))
os.makedirs(os.path.dirname(recent), exist_ok=True)
open(recent, 'wb').write(bytes(range(1, 150)) + b'\n' + b'x' * 60 + b'\n\n\ntwo') # (junk lines, then a name: it's kept; past the 8 names' room, or a 0, nothing's read)
said = run('three')
check('three runs' in said and kept() == 'three\ntwo\n', 'a kept list of garbage: the app starts anyway, and what\'s kept is sane (%r %s)' % (kept(), said))
said = run('two', '\\sq') # (an app run alone is home: the system key with Q there)
check(kept() == 'two\n', 'the system key with Q at home: the recent apps forgotten, but home (%r %s)' % (kept(), said))
print('%d failed' % failed if failed else 'the recent apps came through')
sys.exit(1 if failed else 0)
