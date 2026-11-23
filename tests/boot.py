"""pc.boot(): how many times this Rinc has started, kept in /save/rinc/boot. A count kept where it was
before (/boot, at the top) is carried over the first time and then taken away; each start after counts
on from the new place; with neither, it starts at 1.

  python tests/boot.py path/to/rinc work/folder"""
import os, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
failed = count = 0
def check(name, ok, why):
  global failed, count
  count += 1
  if not ok:
    failed += 1
    print('FAILED: %s\n  %s' % (name, why))

def started(data):
  """rinc started once on data, its app asking pc.boot(): what it said"""
  app = os.path.join(work, 'app', 'app.rio')
  os.makedirs(os.path.dirname(app), exist_ok=True)
  with open(app, 'w', newline='\n') as f: f.write('log("boot", pc.boot())\n')
  r = subprocess.run([rinc, app, '-data', data, '-frames', '1'], capture_output=True, text=True, timeout=60)
  return (r.stdout + r.stderr).strip()
def kept(data, *path):
  p = os.path.join(data, *path)
  return open(p).read() if os.path.exists(p) else None

shutil.rmtree(work, ignore_errors=True)
data = os.path.join(work, 'carried')
os.makedirs(data)
with open(os.path.join(data, 'boot'), 'w', newline='\n') as f: f.write('41')
said = started(data)
check('the old /boot carried over: 42', 'boot 42' in said, said)
check('kept in /save/rinc/boot', kept(data, 'save', 'rinc', 'boot') == '42', kept(data, 'save', 'rinc', 'boot'))
check('the old /boot taken away', kept(data, 'boot') is None, kept(data, 'boot'))
said = started(data)
check('the next start counts on from the new place: 43', 'boot 43' in said, said)

data = os.path.join(work, 'fresh')
os.makedirs(data)
said = started(data)
check('a fresh Rinc: 1', 'boot 1' in said and kept(data, 'save', 'rinc', 'boot') == '1', said)
with open(os.path.join(data, 'boot'), 'w', newline='\n') as f: f.write('7')
said = started(data)
check('a stray old /boot beside the new: the new one wins, the old left alone', 'boot 2' in said and kept(data, 'boot') == '7', said)

print('%d of %d failed' % (failed, count) if failed else 'boot came through (%d cases)' % count)
sys.exit(1 if failed else 0)
