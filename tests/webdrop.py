"""python webdrop.py path/to/rinc work-folder: a bundle of apps (a zip) dropped on a web page (rinc -web),
run in node (webdrop.mjs): with the compiler, its files go into storage (localStorage) and home runs, now
the dropped one; a zip naming ../ installs nothing; the VM-only player says it doesn't install. An empty
page (rinc -name x -web) says nothing's installed, takes the drop and runs its home, and its Reset takes
it back to empty. Without node, skipped (77)."""
import os, shutil, subprocess, sys, zipfile

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.dirname(here)
rinc, work = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
node = shutil.which('node')
if not node: print('no node: skipped'); sys.exit(77)

def fail(why):
  sys.exit('FAIL: ' + why)

def drop(page, z, *more):
  r = subprocess.run([node, os.path.join(here, 'webdrop.mjs'), page, z] + list(more), capture_output=True, text=True, timeout=60)
  if r.returncode: fail('node: ' + r.stdout + r.stderr)
  return r.stdout

shutil.rmtree(work, ignore_errors=True)
os.makedirs(work)
full, small, empty = os.path.join(work, 'full.html'), os.path.join(work, 'small.html'), os.path.join(work, 'empty.html')
# (the console runs Rio itself, so its page has the compiler; it's home there, so the dropped console is home)
r = subprocess.run([rinc, os.path.join(root, 'system', 'console', 'console.rio'), '-name', 'drop', '-web', full], capture_output=True, text=True)
if r.returncode: fail('rinc -web: ' + r.stdout + r.stderr)
r = subprocess.run([rinc, os.path.join(root, 'apps', 'breakout.rio'), '-web', small], capture_output=True, text=True)
if r.returncode: fail('rinc -web: ' + r.stdout + r.stderr)
r = subprocess.run([rinc, '-name', 'empty', '-web', empty], capture_output=True, text=True)
if r.returncode: fail('rinc -name empty -web: ' + r.stdout + r.stderr)
r = subprocess.run([rinc, '-web', empty], capture_output=True, text=True)
if r.returncode != 2: fail('an empty page without a storage name was made')
good, bad = os.path.join(work, 'apps.zip'), os.path.join(work, 'bad.zip')
with zipfile.ZipFile(good, 'w') as z:
  z.writestr('rinc/apps/console/console.rio', 'log("the dropped console runs")\n', zipfile.ZIP_DEFLATED)
  z.writestr('rinc/apps/console/data.txt', 'kept ' * 50, zipfile.ZIP_STORED)
  z.writestr('rinc/apps/home/home.rio', 'log("the dropped home runs")\n', zipfile.ZIP_DEFLATED)
  z.writestr('rinc/help/dropped.txt', 'help text\n', zipfile.ZIP_DEFLATED)
  z.writestr('rinc/save/elsewhere.txt', 'not installed: not apps or help\n')
with zipfile.ZipFile(bad, 'w') as z:
  z.writestr('rinc/apps/x/x.rio', 'log(1)\n')
  z.writestr('rinc/apps/../../evil.txt', 'no\n')

out = drop(full, good)
for want in ('installed 4 files from apps.zip: console home', 'the dropped console runs', 'drop/apps/console/console.rio', 'drop/apps/console/data.txt', 'drop/help/dropped.txt'):
  if want not in out: fail('a dropped bundle: no %r in:\n%s' % (want, out))
if 'save/elsewhere' in out or 'error' in out.lower() or 'Reset' in out: fail('a dropped bundle:\n' + out)
out = drop(full, bad)
if 'not installed' not in out or 'nothing installed' not in out or '/apps/x/' in out or 'evil' in out.split('keys:')[1]: fail('a zip naming ../:\n' + out)
out = drop(small, good)
if 'runs its own apps' not in out or 'apps/console' in out.split('keys:')[1]: fail('the VM-only player:\n' + out)
out = drop(empty, good, 'reset')
keys = out.split('keys:')[1].split()
if 'No apps here yet' not in out or 'Drop a bundle' not in out or 'the dropped home runs' not in out: fail('an empty page:\n' + out)
if 'something on the screen' not in out: fail("an empty page doesn't show that nothing's installed:\n" + out)
if 'reset, reloaded' not in out or keys: fail('an empty page reset:\n' + out)
print('web drops came through')
