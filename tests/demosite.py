"""python demosite.py path/to/rinc work-folder: the demo site's pages, made as the site is (tools/demosite.py), each
played in node (webplay.mjs) for 120 frames: each starts, draws something, says no error, and has Rinc's
icon; the index lists every demo. And the whole Rinc exported as a program, run for 120 frames. (The whole
Rinc's page once showed only black: its bundle ran over the player's memory.) Without node, the pages
are made but not played (77: skipped)."""
import os, re, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, '..', 'tools'))
import demosite

def fail(why): sys.exit('demo site: ' + why)
def colors(path):
  b = open(path, 'rb').read()
  m = re.match(rb'P6\s+(\d+)\s+(\d+)\s+\d+\s', b)
  p = b[m.end():]
  return len({p[i:i + 3] for i in range(0, len(p), 3)})

shutil.rmtree(work, ignore_errors=True)
site = os.path.join(work, 'site')
demosite.make(rinc, site)
index = open(os.path.join(site, 'index.html'), encoding='utf-8').read()
for name, _ in demosite.demos():
  if 'href="%s.html"' % name not in index: fail('the index has no link to ' + name)
if index.count('rel="icon"') != 2: fail('the index has no icon')

game = os.path.join(work, 'rinc-whole' + ('.exe' if os.name == 'nt' else ''))
demosite.wholeRinc(rinc, game, export=True)
shot = os.path.join(work, 'whole.ppm')
r = subprocess.run([game, '-frames', '120', '-o', shot, '-data', os.path.join(work, 'data')], capture_output=True, text=True, timeout=120)
out = r.stdout + r.stderr
if r.returncode or 'error' in out.lower() or colors(shot) < 4: fail('the whole Rinc, as a program: %d colors drawn\n%s' % (colors(shot), out))

node = shutil.which('node')
if not node:
  print('demo site: made, not played (no node)')
  sys.exit(77)
pages = sorted(f for f in os.listdir(site) if f.endswith('.html') and f != 'index.html')
for page in pages:
  path, shot = os.path.join(site, page), os.path.join(work, page + '.ppm')
  if open(path, encoding='utf-8').read().count('rel="icon"') != 2: fail(page + ' has no icon')
  r = subprocess.run([node, os.path.join(here, 'webplay.mjs'), path, '120', shot], capture_output=True, text=True, timeout=180)
  out = r.stdout + r.stderr
  if r.returncode or 'FAIL' in out or 'error' in out.lower(): fail('%s: %s' % (page, out))
  if colors(shot) < 2: fail(page + ': nothing on the screen')
print('demo site came through: %d pages, and the whole Rinc as a program' % len(pages))
