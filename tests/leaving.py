"""python leaving.py path/to/rinc path/to/system data-folder: an app about to give way to another (the
system key: Esc, home, here), or Rinc about to stop (the desktop's window closed, or -frames run out),
has its leaving* called, to keep where it is in its stash (docs/spec/apps.md#leaving); not when it's
forgotten (the system key with Q: its stash dropped, and staying so). And the editor, left with changes
not saved, has them again on coming back."""
import os, shutil, subprocess, sys

rinc, system, data = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2]), sys.argv[3]

def put(path, text):
  path = os.path.join(data, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'w', newline='') as f: f.write(text)
def get(path):
  path = os.path.join(data, *path.split('/'))
  return open(path, newline='').read() if os.path.exists(path) else None
def run(app, keys, frames=60):
  r = subprocess.run([rinc, '-data', data, '-app', app, '-frames', str(frames), '-type', keys], capture_output=True, text=True, timeout=60)
  return (r.stdout + r.stderr).strip()

shutil.rmtree(data, ignore_errors=True)
for app in ('edit', 'home'):
  shutil.copytree(os.path.join(system, app), os.path.join(data, 'apps', app))
failed = 0
def check(ok, why):
  global failed
  print(('ok   ' if ok else 'FAIL ') + why)
  failed += not ok

# an app that keeps what's typed only on leaving
put('apps/stay/stay.rio', 'typed: Blob[..32]\nleaving* :: proc()\n  if typed.len() > 0\n    ok := pc.stashWrite("typed", typed.asString())\n  end\nend\n'
    'update* :: proc()\n  c := pc.key()\n  for c >= 0\n    ok := typed.push(c)\n    c = pc.key()\n  end\nend\n')
said = run('stay', 'abc\\se')
check(get('stash/stay/typed') == 'abc', 'the system key with Esc: leaving* called first, its stash written (%r %s)' % (get('stash/stay/typed'), said))
shutil.rmtree(os.path.join(data, 'stash'), ignore_errors=True)
said = run('stay', 'abc\\sq')
check(get('stash/stay/typed') is None, 'the system key with Q: not called, the stash stays dropped (%r %s)' % (get('stash/stay/typed'), said))

# the editor: a file opened, typed in, left (not saved), come back to and saved: what was typed is in it
put('save/notes/t.txt', 'plain\n')
put('apps/caller/caller.rio', 'out: Blob[..8]\nif pc.result(out) < 0\n  pc.call("edit", "open /rinc/save/notes/t.txt")\nend\n')
said = run('caller', '', 60) # (opened: the file the editor has last)
said += run('edit', 'X', 60) # (typed, not saved; then Rinc stops: it leaves)
saved = get('save/notes/t.txt')
said += run('edit', '\\x13', 60)
check(saved == 'plain\n' and get('save/notes/t.txt') == 'Xplain\n', 'the editor, left unsaved: its changes there again on coming back (%r %r %s)' % (saved, get('save/notes/t.txt'), said))

print('all of leaving\'s cases came through' if not failed else '%d failed' % failed)
sys.exit(1 if failed else 0)
