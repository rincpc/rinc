# /// script
# dependencies = ["pyserial"]
# ///
"""Send an app to Rinc's console over USB serial and start it, or save it to the board's SD card.

  uv run tools/send.py apps/bounce.rio [COM3] [--no-run]     (uv fetches pyserial) saved as the app sent
                                                             (/rinc/apps/sent/, replaced each time) and run
  uv run tools/send.py apps/bounce.rio [COM3] --save         to /rinc/apps/bounce/bounce.rio
  uv run tools/send.py apps/*.rio --save                     several at once
  uv run tools/send.py system/files [COM3]                   an app of a folder: saved, every file
                                                             its files.txt lists (else its .rio files)
  uv run tools/send.py system/edit --test                    and its test.rio too, then run as edit/test:
                                                             what it says, and a failed assert's line

Ctrl-C over serial opens the console, in place of whatever's running, so this also replaces a game
that's playing. Prints what the board says back: errors show up with their lines. A saved app runs
from home, or with `run bounce` in the console. An app of a folder is always saved (its other files
are read from the card as it compiles), to /rinc/apps/<folder's name>/; add --project to save it as
that project instead, /rinc/projects/<name>/. Files are sent as text (ASCII: other bytes are dropped).
"""
import glob, os, sys, time
import serial  # pyserial

args = [a for a in sys.argv[1:] if not a.startswith('--')]
port = next((a for a in args if a.upper().startswith('COM') or a.startswith('/dev/')), 'COM3')
apps = [c for a in args if a != port for c in (glob.glob(a) or [a])]
run = '--no-run' not in sys.argv
save = '--save' in sys.argv
project = '--project' in sys.argv
test = '--test' in sys.argv
failed = 0

def talk(s, data):
  """send, then read until the board has been quiet for a moment"""
  s.write(data)
  out, quiet = b'', 0
  while quiet < 3:
    chunk = s.read(4096)
    out += chunk; quiet = 0 if chunk else quiet + 1
  return out.decode(errors='replace').replace('\r', '')

def text(path):
  """a file's text, as typed: lines ended by CR, and one at its end"""
  src = open(path, encoding='utf-8').read().replace('\r\n', '\n')
  return src.replace('\n', '\r').encode('ascii', errors='ignore') + (b'\r' if not src.endswith('\n') else b'')

def runTest(s, ref):
  """run ref/test (an app's test.rio, beside its main file): what it says until it's done or fails"""
  global failed
  s.write(b'run ' + ref.encode() + b'/test\r')
  out, t = b'', time.time()
  while time.time() - t < 30 and not any(w in out for w in (b'passed', b'error', b'no file', b'(done)')):
    out += s.read(4096)
  out += s.read(4096)
  lines = [l for l in out.decode(errors='replace').replace('\r', '').split('\n') if l.strip() and not l.startswith(('> run', 'run '))]
  print('\n'.join(lines) if lines else ref + '/test: no answer')
  if not any('passed' in l for l in lines): failed = 1

def saveAs(s, target, path):
  """save the file at path on the board, as save's target: an app (name) or a file of one (name/a/b)"""
  said = talk(s, b'save ' + target.encode() + b'\r' + text(path) + b'\x04')
  saved = [l for l in said.split('\n') if l.startswith(('saved', "can't", 'no storage', 'couldn', 'names are', 'a file of'))]
  print('%s: %s' % (target, saved[-1] if saved else 'no answer'))

def folderFiles(folder, name):
  """an app folder's files, relative to it: its files.txt's, else its .rio files (but its tests,
  test.rio); the main one first"""
  listed = os.path.join(folder, 'files.txt')
  if os.path.exists(listed):
    files = [l.strip() for l in open(listed, encoding='utf-8') if l.strip() and not l.startswith('#')]
  else:
    files = sorted(os.path.relpath(p, folder).replace('\\', '/') for p in glob.glob(os.path.join(folder, '**', '*.rio'), recursive=True))
  main = name + '.rio'
  return [main] + [f for f in files if f not in (main, 'test.rio')]

with serial.Serial(port, 115200, timeout=0.2) as s:
  s.dtr = True
  s.write(b'\x03')                      # stop what's running (home, a game), into the console
  time.sleep(0.2); s.reset_input_buffer()
  for app in apps:
    if os.path.isdir(app):              # an app of a folder: each of its files, into its folder there
      folder = os.path.normpath(app)
      name = os.path.basename(os.path.abspath(folder))
      ref = name + ('+' if project else '')
      for f in folderFiles(folder, name):
        saveAs(s, ref if f == name + '.rio' else ref + '/' + f, os.path.join(folder, f))
      if test and os.path.exists(os.path.join(folder, 'test.rio')):
        saveAs(s, ref + '/test.rio', os.path.join(folder, 'test.rio'))
        runTest(s, ref)
    elif save:
      saveAs(s, os.path.splitext(os.path.basename(app))[0], app)
    else:                               # saved as the app sent, then run: compiled whole, so it starts when it's all there
      saveAs(s, 'sent', app)            # and its errors have the file's lines
      if run:
        s.write(b'run sent\r')
        out, t, quiet = b'', time.time(), 0
        while time.time() - t < 20 and quiet < 10 and b'error' not in out: # (what it says, till it's quiet for 2 s)
          chunk = s.read(4096)
          out += chunk; quiet = 0 if chunk else quiet + 1
        out += s.read(4096)
        lines = [l for l in out.decode(errors='replace').replace('\r', '').split('\n') if l.strip() and not l.startswith(('> run', 'run ', '[usb]'))]
        print('\n'.join(lines) if lines else 'running')
sys.exit(failed)
