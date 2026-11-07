"""python opening.py path/to/rinc path/to/system data-folder: apps called with "open /a/path" (as Files calls
the app for a file's kind: docs/spec/apps.md#files) open that file: the editor, a text file (typed in and
saved there), and Help, a .md file of its own (read, and kept as where it is)."""
import os, shutil, subprocess, sys

rinc, system, data = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2]), sys.argv[3]

def put(path, text):
  path = os.path.join(data, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'w', newline='') as f: f.write(text)
def get(path):
  path = os.path.join(data, *path.split('/'))
  return open(path, newline='').read() if os.path.exists(path) else None
caller = 'out: Blob[..8]\nif pc.result(out) < 0\n  pc.call("%s", "open %s")\nend\n'
def calling(app, path, keys, frames=120):
  put('apps/caller/caller.rio', caller % (app, path))
  return subprocess.run([rinc, '-data', data, '-app', 'caller', '-frames', str(frames), '-type', keys], capture_output=True, text=True, timeout=60)

shutil.rmtree(data, ignore_errors=True)
for app in ('edit', 'help', 'files'):
  shutil.copytree(os.path.join(system, app), os.path.join(data, 'apps', app))
failed = 0
def check(ok, why):
  global failed
  print(('ok   ' if ok else 'FAIL ') + why)
  failed += not ok

put('save/notes/t.txt', 'plain text\n')
calling('edit', '/rinc/save/notes/t.txt', 'X\\x13') # (X, then Ctrl+S)
check(get('save/notes/t.txt') == 'Xplain text\n', 'the editor, called to open a text file: it, typed in and saved: %r' % get('save/notes/t.txt'))

put('save/notes/n.md', '# Notes\n\n' + ''.join('A line of notes, %d.\n\n' % i for i in range(60)))
calling('help', '/rinc/save/notes/n.md', '\\x1b[6~', 200) # (Page Down: it's kept where it is, a second after)
place = get('stash/help/place') or ''
check('/rinc/save/notes/n.md' in place, 'Help, called to open a .md file: it, read there: %r' % place[:80])

shutil.rmtree(data, ignore_errors=True)
print('%d failed' % failed if failed else 'opening came through')
sys.exit(1 if failed else 0)
