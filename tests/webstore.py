"""python webstore.py path/to/rinc work-folder: a page's storage in IndexedDB, other tabs told of each change
(webstore.mjs, in node): a save kept through a reload, seen by another tab of the same storage name as it's
made, nothing in localStorage; and what a page before kept in localStorage, moved in. Without node,
skipped (77)."""
import os, shutil, subprocess, sys

here = os.path.dirname(os.path.abspath(__file__))
rinc, work = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
node = shutil.which('node')
if not node: print('no node: skipped'); sys.exit(77)
def fail(why): sys.exit('web storage: ' + why)
shutil.rmtree(work, ignore_errors=True)
os.makedirs(work)
pages = []
for app in ('keeper', 'watcher'):
  page = os.path.join(work, app + '.html')
  r = subprocess.run([rinc, os.path.join(here, 'webstore', app, app + '.rio'), '-name', 'shared', '-web', page], capture_output=True, text=True)
  if r.returncode: fail('rinc -web: ' + r.stdout + r.stderr)
  pages.append(page)
r = subprocess.run([node, os.path.join(here, 'webstore.mjs'), *pages], capture_output=True, text=True, timeout=120)
out = r.stdout + r.stderr
for want in ('keeper: keeper starts: 1', 'watcher: watcher sees: 1', 'keeper again: keeper starts: 2', 'watcher: watcher sees: 2',
             'keeper, from localStorage: keeper starts: 42', 'localStorage: []'):
  if want not in out: fail('no %r in:\n%s' % (want, out))
print('web storage came through')
