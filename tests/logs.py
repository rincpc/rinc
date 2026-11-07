"""python logs.py path/to/rinc path/to/system folder: each app's log, in its cache space (/rinc/cache/<app>/log):
what it logged and Rinc's runtime error, kept across runs; an app's own cache files (pc.cacheRead, cacheWrite,
cacheDelete); and Sys+O, the log app at the app's log. docs/spec/apps.md#logs"""
import os, shutil, subprocess, sys

rinc, system, data = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2]), sys.argv[3]

def put(path, text):
  path = os.path.join(data, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'w', newline='') as f: f.write(text)
def get(path):
  path = os.path.join(data, *path.split('/'))
  return open(path, newline='').read() if os.path.exists(path) else None
def run(app, frames=30, keys=None, out=None):
  args = [rinc, '-data', data, '-app', app, '-frames', str(frames)] + (['-type', keys] if keys else []) + (['-o', out] if out else [])
  return subprocess.run(args, capture_output=True, text=True, timeout=60).stdout
def check(ok, why):
  if not ok: sys.exit('FAIL: ' + why)

shutil.rmtree(data, ignore_errors=True)
shutil.copytree(os.path.join(system, 'log'), os.path.join(data, 'apps', 'log'))
put('apps/oops/oops.rio', 'log("hello from oops")\nxs: [2]Int\ni := 5\nlog(xs[i])\n')
run('oops')
got = get('cache/oops/log') or ''
check('hello from oops' in got and 'runtime error' in got and got.startswith('-- started\n'), 'the log and its runtime error, in the cache: %r' % got)
run('oops')
got = get('cache/oops/log') or ''
check(got.count('-- started') == 2 and got.count('hello from oops') == 2, 'a second run, after the first: %r' % got)

# an app's own cache: read, written and deleted, beside its log
put('apps/keeper/keeper.rio', '''ok := pc.cacheWrite("thumbs", "abc")
got: Blob[..8]
n := pc.cacheRead("thumbs", got)
log("cached", ok, n, got.asString(), pc.cacheRead("../x", got), pc.cacheDelete("thumbs"), pc.cacheRead("thumbs", got))
''')
out = run('keeper')
check('cached 0 3 abc -1 0 -1' in out, "an app's cache files: %r" % out)
check('cached 0 3 abc' in (get('cache/keeper/log') or ''), "its log, in its cache too: %r" % get('cache/keeper/log'))

# Sys+O: the log app, at this app's log (its rows on the screen); Esc, back
put('apps/calm/calm.rio', 'log("calm here")\ndraw* :: proc()\n  pc.cls(1)\nend\n')
shot = os.path.join(data, 'screen.ppm')
run('calm', 60, '\\so', shot)
logShot = open(shot, 'rb').read()
run('calm', 60, None, shot)
calmShot = open(shot, 'rb').read()
check(logShot != calmShot, 'Sys+O: the log app shown')
print('logs came through')
