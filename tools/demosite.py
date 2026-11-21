"""python tools/demosite.py path/to/rinc site: the demo site's pages (rincpc.com, .github/workflows/pages.yml), made
with a rinc that has the web players built in: a page for each app in apps/ (not the timings: bench, micro,
speed), an empty Rinc (empty.html), the whole Rinc (rinc.html: every system app, the demos, the themes, and
Rinc's and Rio's docs as help), and index.html listing them. tests/demosite.py makes the same pages and plays each,
so what's put on the site is what's tested."""
import base64, glob, html, os, subprocess, sys

TIMINGS = {'bench', 'micro', 'speed'}  # (timings, not demos)
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def demos():
  """(name, its main .rio) for each demo in apps/: an app of one file, or of a folder"""
  found = []
  for path in sorted(glob.glob(os.path.join(root, 'apps', '*.rio')) + glob.glob(os.path.join(root, 'apps', '*', ''))):
    name = os.path.basename(os.path.normpath(path))
    if name.endswith('.rio'): name = name[:-4]
    else: path = os.path.join(path, name + '.rio')
    if name in TIMINGS or not os.path.isfile(path): continue
    found.append((name, path))
  return found

def systemApps():
  return [os.path.join(d, os.path.basename(os.path.normpath(d)) + '.rio') for d in sorted(glob.glob(os.path.join(root, 'system', '*', '')))
          if os.path.isfile(os.path.join(d, os.path.basename(os.path.normpath(d)) + '.rio'))]

def icons():
  """Rinc's logo as the page's icon (docs/logo): the pale one on dark browser themes, the dark one on light"""
  def uri(f): return 'data:image/png;base64,' + base64.b64encode(open(os.path.join(root, 'docs', 'logo', f), 'rb').read()).decode()
  return ['<link rel="icon" type="image/png" media="(prefers-color-scheme: %s)" href="%s">' % (scheme, uri(f))
          for scheme, f in (('dark', 'rinc-logo-32.png'), ('light', 'rinc-logo-dark-32.png'))]

def run(cmd):
  r = subprocess.run(cmd, capture_output=True, text=True)
  if r.returncode: sys.exit('site: %s\n%s%s' % (' '.join(cmd), r.stdout, r.stderr))

def wholeRinc(rinc, out, export=False):
  """the whole Rinc, as a page (or, export: a program)"""
  run([rinc, *systemApps(), *[p for _, p in demos()], '-home', 'home', '-name', 'rinc',
       '-docs', 'rinc=' + os.path.join(root, 'docs'), '-docs', 'rio=' + os.path.join(root, 'lib', 'rio', 'docs'),
       '-files', 'apps/rinc=' + os.path.join(root, 'system', 'rinc'), '-export' if export else '-web', out])

def make(rinc, site):
  os.makedirs(site, exist_ok=True)
  items = []
  for name, path in demos():
    run([rinc, path, '-web', os.path.join(site, name + '.html')])
    about = open(path, encoding='utf-8').readline().lstrip('#').strip()
    items.append('<li><a href="%s.html">%s</a>: %s</li>' % (name, name, html.escape(about, quote=False)))
  run([rinc, '-name', 'rinc-empty', '-web', os.path.join(site, 'empty.html')])
  wholeRinc(rinc, os.path.join(site, 'rinc.html'))
  page = '\n'.join([
    '<!doctype html><html lang="en"><head><meta charset="utf-8">',
    '<meta name="viewport" content="width=device-width, initial-scale=1"><title>Rinc apps</title>',
    *icons(),
    '<style>body{margin:0;padding:24px 16px;background:#1d2b53;color:#fff1e8;font:16px/1.5 ui-monospace,monospace}',
    'main{max-width:720px;margin:auto}a{color:#ffec27}h1{font-size:22px}li{margin:10px 0}p{color:#c2c3c7}</style>',
    '</head><body><main><h1>Rinc apps</h1>',
    '<p>Rinc apps, playing in the browser: the same Rio VM as on the board and the desktop. Arrows and V C X Z, or a controller. <a href="docs/">Docs</a>: making apps, and what a Rinc provides.</p>',
    '<p><a href="rinc.html">The whole Rinc</a>: home, the console, the editor, files, help (with these docs), colors, themes and every app below, as on the Fruit Jam. What you save stays in this browser. Ctrl and Shift tapped together, and then Esc, goes home.</p>',
    '<p><a href="empty.html">An empty Rinc</a>: nothing installed yet. Drop a bundle of apps on it, such as <code>rinc-system-apps.zip</code> from a <a href="https://github.com/rincpc/rinc/releases">release</a>. Its Reset button takes it back to empty.</p><ul>',
    *items,
    '</ul><p><a href="docs/">Docs</a> &middot; <a href="https://github.com/rincpc/rinc">Source</a></p></main></body></html>', ''])
  open(os.path.join(site, 'index.html'), 'w', encoding='utf-8', newline='\n').write(page)

if __name__ == '__main__':
  if len(sys.argv) != 3: sys.exit(__doc__)
  make(os.path.abspath(sys.argv[1]), sys.argv[2])
  for f in sorted(os.listdir(sys.argv[2])): print('%10d %s' % (os.path.getsize(os.path.join(sys.argv[2], f)), f))
