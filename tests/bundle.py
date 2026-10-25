"""python bundle.py path/to/rinc data-folder: installing apps from a bundle (a zip, as a release's
rinc-system-apps.zip) and from an app's folder. The system apps and docs, as tools/install.py lays them
out, zipped (most files deflated, the help texts stored), installed into fresh storage: each file comes
out exactly, an app runs, and a project and a save already there stay. A zip naming a file outside
storage installs nothing; a folder brings what its files.txt lists."""
import os, shutil, subprocess, sys, zipfile

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.dirname(here)
rinc, data = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
work = data + '-work'

def fail(why):
  sys.exit('FAIL: ' + why)

def run(*args):
  return subprocess.run([rinc, '-data', data] + list(args), capture_output=True, text=True, timeout=60)

def put(path, text):
  path = os.path.join(data, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'w') as f: f.write(text)

shutil.rmtree(data, ignore_errors=True)
shutil.rmtree(work, ignore_errors=True)
os.makedirs(work)
stage = os.path.join(work, 'stage')
r = subprocess.run([sys.executable, os.path.join(root, 'tools', 'install.py'), '--system', '--to', stage], capture_output=True, text=True)
if r.returncode: fail('tools/install.py: ' + r.stdout + r.stderr)
zipPath = os.path.join(work, 'apps.zip')
staged = []
with zipfile.ZipFile(zipPath, 'w') as z:
  for d, _, fs in os.walk(os.path.join(stage, 'rinc')):
    for f in sorted(fs):
      full = os.path.join(d, f)
      rel = os.path.relpath(full, stage).replace('\\', '/')  # (rinc/apps/..., rinc/help/...)
      z.write(full, rel, zipfile.ZIP_STORED if rel.startswith('rinc/help/') else zipfile.ZIP_DEFLATED)
      staged.append((rel[5:], full))
if len(staged) < 20: fail('only %d files staged' % len(staged))

put('projects/files/files.rio', 'log("mine")\n')
put('save/files/note', 'kept')
r = run('-install', zipPath)
if r.returncode or 'installed %d files' % len(staged) not in r.stdout: fail('installing the zip: ' + r.stdout + r.stderr)
for rel, full in staged:
  got = os.path.join(data, *rel.split('/'))
  if not os.path.exists(got) or open(got, 'rb').read() != open(full, 'rb').read(): fail(rel + ' did not come out the same')
if open(os.path.join(data, 'projects', 'files', 'files.rio')).read() != 'log("mine")\n' or open(os.path.join(data, 'save', 'files', 'note')).read() != 'kept':
  fail('a project or a save changed')
r = run('-app', 'console', '-frames', '2')
if r.returncode or 'error' in (r.stdout + r.stderr).lower(): fail('an installed app: ' + r.stdout + r.stderr)

evil = os.path.join(work, 'evil.zip')
with zipfile.ZipFile(evil, 'w') as z:
  z.writestr('rinc/apps/ok/ok.rio', 'log(1)\n')
  z.writestr('rinc/apps/../evil.rio', 'log(2)\n')
r = run('-install', evil)
if r.returncode == 0 or os.path.exists(os.path.join(data, 'evil.rio')) or os.path.exists(os.path.join(data, 'apps', 'ok')):
  fail('a zip naming ../ was installed, or part of it')

r = run('-install', os.path.join(root, 'system', 'colors') + '/')
listed = [l.strip() for l in open(os.path.join(root, 'system', 'colors', 'files.txt')) if l.strip()]
for l in listed:
  if not os.path.exists(os.path.join(data, 'apps', 'colors', *l.split('/'))): fail('the folder install left out ' + l)
if r.returncode: fail('installing a folder: ' + r.stdout + r.stderr)

shutil.rmtree(work, ignore_errors=True)
print('bundles came through: %d files' % len(staged))
