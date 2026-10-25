# /// script
# dependencies = ["pyserial"]
# ///
"""Put Rinc's apps and help texts on the board's SD card, over USB serial, or into a folder (an SD card
in a reader on this computer, say). The firmware carries none of them (docs/spec/apps.md#bundles).

  uv run tools/install.py                 everything: the system apps, the demos, and Rinc's and Rio's
                                          docs (as help texts), over COM3
  uv run tools/install.py COM5            another port
  uv run tools/install.py home chord      just these apps, by name (system apps and demos)
  uv run tools/install.py --system        the system apps and the docs (rinc-system-apps.zip in a release)
  uv run tools/install.py --demos         the demos (rinc-demo-apps.zip in a release)
  uv run tools/install.py --apps          all the apps, no docs
  uv run tools/install.py --docs          just the docs
  uv run tools/install.py --to E:         into E:/rinc instead (or any folder), no serial port needed

Over serial, Ctrl-B opens the firmware's own console, whatever is running (it needs no apps on the
card), and its put command takes each file as hex, so any bytes arrive whole. Then it goes home. A
file already on the card is replaced; nothing else there is touched.
"""
import glob, os, re, shutil, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DOCS = [('rinc', os.path.join(ROOT, 'docs')), ('rio', os.path.join(ROOT, 'lib', 'rio', 'docs'))]

def rel(path, base):
  return os.path.relpath(path, base).replace('\\', '/')

def appFiles(name):
  """an app's files, as (path under /rinc, file here): a system app's, or a demo's (a folder, else a
  .rio file); None if there's no app of that name. A folder's files.txt lists them, else its .rio files
  and help texts"""
  for top in ('system', 'apps'):
    folder = os.path.join(ROOT, top, name)
    if os.path.isdir(folder):
      listed = os.path.join(folder, 'files.txt')
      if os.path.exists(listed):
        files = [l.strip() for l in open(listed, encoding='utf-8') if l.strip() and not l.startswith('#')]
      else:
        files = sorted(rel(p, folder) for p in glob.glob(os.path.join(folder, '**', '*.rio'), recursive=True) +
                       glob.glob(os.path.join(folder, 'help', '**', '*.md'), recursive=True))
      return [('apps/%s/%s' % (name, f), os.path.join(folder, f)) for f in files]
  single = os.path.join(ROOT, 'apps', name + '.rio')
  if os.path.exists(single):
    return [('apps/%s/%s.rio' % (name, name), single)]
  return None

def systemApps():
  return sorted(os.path.basename(os.path.dirname(p)) for p in glob.glob(os.path.join(ROOT, 'system', '*', 'files.txt')))

def demoApps():
  names = [os.path.basename(p) for p in glob.glob(os.path.join(ROOT, 'apps', '*')) if os.path.isdir(p)]
  names += [os.path.splitext(os.path.basename(p))[0] for p in glob.glob(os.path.join(ROOT, 'apps', '*.rio'))]
  return sorted(set(names))

def docFiles():
  """Rinc's and Rio's docs, as help texts: help/rinc/..., help/rio/..."""
  return [('help/%s/%s' % (prefix, rel(p, folder)), p)
          for prefix, folder in DOCS for p in sorted(glob.glob(os.path.join(folder, '**', '*.md'), recursive=True))]

args = sys.argv[1:]
to = args[args.index('--to') + 1] if '--to' in args else None
words = [a for i, a in enumerate(args) if not a.startswith('--') and not (i and args[i - 1] == '--to')]
port = next((a for a in words if re.fullmatch(r'COM[0-9]+', a, re.I) or a.startswith('/dev/')), 'COM3')
names = [a for a in words if a != port]
picked = [f for f in ('--system', '--demos', '--apps', '--docs') if f in args]
everything = not picked and not names
system = everything or '--system' in args or '--apps' in args
demos = everything or '--demos' in args or '--apps' in args
docs = everything or '--system' in args or '--docs' in args

files = []
for name in (names or (systemApps() if system else []) + (demoApps() if demos else [])):
  got = appFiles(name)
  if got is None: sys.exit('install.py: no app named %s (in system/ or apps/)' % name)
  files += got
if docs: files += docFiles()

if to:
  for inside, path in files:
    out = os.path.join(to, 'rinc', *inside.split('/'))
    os.makedirs(os.path.dirname(out), exist_ok=True)
    shutil.copyfile(path, out)
  print('%d files into %s' % (len(files), os.path.join(to, 'rinc')))
  sys.exit()

import serial  # pyserial

def until(s, ends, wait=20):
  """what the board says, until a line starts with one of ends (or it's been quiet for wait seconds)"""
  out, last = b'', time.time()
  while time.time() - last < wait:
    chunk = s.read(4096)
    if chunk:
      out += chunk; last = time.time()
      lines = out.decode(errors='replace').replace('\r', '').split('\n')
      done = [l for l in lines if l.lstrip('> ').startswith(ends)]
      if done: return done[-1].lstrip('> ')
  return None

with serial.Serial(port, 115200, timeout=0.1) as s:
  s.dtr = True
  s.write(b'\x02')                      # the firmware's own console, in place of whatever's running
  if not until(s, ('Rinc: rio REPL',), 5): sys.exit('install.py: no answer from the board on %s' % port)
  time.sleep(0.2); s.reset_input_buffer()
  bad = 0
  for inside, path in files:
    hexed = open(path, 'rb').read().hex()
    s.write(b'put ' + inside.encode() + b'\r')
    said = until(s, ('putting', "can't", 'no storage', 'a path'))
    if not said or not said.startswith('putting'):
      print('%s: %s' % (inside, said or 'no answer')); bad += 1; continue
    s.write(b'\r'.join(hexed[i:i + 128].encode() for i in range(0, len(hexed), 128)) + b'\x04')
    said = until(s, ('saved', "couldn't", 'gave up'))
    ok = said == 'saved %d bytes' % (len(hexed) // 2)
    bad += not ok
    print('%s: %s' % (inside, said or 'no answer'))
  s.write(b'home\r')                    # and back home (now on the card)
  print('%d files, %s' % (len(files), 'all saved' if not bad else '%d not saved' % bad))
  sys.exit(1 if bad else 0)
