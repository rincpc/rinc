"""python webdrop.py path/to/rinc work-folder: a bundle of apps (a zip) dropped on a web page (rinc -web),
run in node (webdrop.mjs): with the compiler, its files go into storage (localStorage) and home runs, now
the dropped one; a zip naming ../ installs nothing; the VM-only player says it doesn't install. Without
node, skipped (77)."""
import os, shutil, subprocess, sys, zipfile

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.dirname(here)
rinc, work = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
node = shutil.which('node')
if not node: print('no node: skipped'); sys.exit(77)

def fail(why):
  sys.exit('FAIL: ' + why)

def drop(page, z):
  r = subprocess.run([node, os.path.join(here, 'webdrop.mjs'), page, z], capture_output=True, text=True, timeout=60)
  if r.returncode: fail('node: ' + r.stdout + r.stderr)
  return r.stdout

shutil.rmtree(work, ignore_errors=True)
os.makedirs(work)
full, small = os.path.join(work, 'full.html'), os.path.join(work, 'small.html')
# (the console runs Rio itself, so its page has the compiler; it's home there, so the dropped console is home)
r = subprocess.run([rinc, os.path.join(root, 'system', 'console', 'console.rio'), '-name', 'drop', '-web', full], capture_output=True, text=True)
if r.returncode: fail('rinc -web: ' + r.stdout + r.stderr)
r = subprocess.run([rinc, os.path.join(root, 'apps', 'breakout.rio'), '-web', small], capture_output=True, text=True)
if r.returncode: fail('rinc -web: ' + r.stdout + r.stderr)
good, bad = os.path.join(work, 'apps.zip'), os.path.join(work, 'bad.zip')
with zipfile.ZipFile(good, 'w') as z:
  z.writestr('rinc/apps/console/console.rio', 'log("the dropped console runs")\n', zipfile.ZIP_DEFLATED)
  z.writestr('rinc/apps/console/data.txt', 'kept ' * 50, zipfile.ZIP_STORED)
  z.writestr('rinc/help/dropped.txt', 'help text\n', zipfile.ZIP_DEFLATED)
  z.writestr('rinc/save/elsewhere.txt', 'not installed: not apps or help\n')
with zipfile.ZipFile(bad, 'w') as z:
  z.writestr('rinc/apps/x/x.rio', 'log(1)\n')
  z.writestr('rinc/apps/../../evil.txt', 'no\n')

out = drop(full, good)
for want in ('installed 3 files from apps.zip: console', 'the dropped console runs', 'drop/apps/console/console.rio', 'drop/apps/console/data.txt', 'drop/help/dropped.txt'):
  if want not in out: fail('a dropped bundle: no %r in:\n%s' % (want, out))
if 'save/elsewhere' in out or 'error' in out.lower(): fail('a dropped bundle:\n' + out)
out = drop(full, bad)
if 'not installed' not in out or 'nothing installed' not in out or '/apps/x/' in out or 'evil' in out.split('keys:')[1]: fail('a zip naming ../:\n' + out)
out = drop(small, good)
if 'runs its own apps' not in out or 'apps/console' in out.split('keys:')[1]: fail('the VM-only player:\n' + out)
print('web drops came through')
