"""python commands.py path/to/rinc tests-folder data-folder: the command list (docs/spec/apps.md#its-commands),
the system key with P, or an app's own key for it (pc.showCommands): typed at, and checked in what the app
logs of the keys it gets (tests/commands.rio). An app without commandList* has the system's alone."""
import os, shutil, subprocess, sys

rinc, tests, data = os.path.abspath(sys.argv[1]), sys.argv[2], sys.argv[3]
app = os.path.join(tests, 'commands.rio')
LIST, ENTER, ESC, WAIT, DOWN = '\\sp', '\\n', '\\e', '\\w', '\\x1b[B' # (as rinc -type takes them: \w waits for what's typed to be taken)

def run(typed, path=app, frames=60):
  shutil.rmtree(data, ignore_errors=True)
  r = subprocess.run([rinc, path, '-data', data, '-frames', str(frames), '-type', typed], capture_output=True, text=True, timeout=60)
  return [l for l in r.stdout.split('\n') if l.strip()], r.stderr.strip()

cases = [
  ('Sys+P, narrowed, Enter: its key first, then what was typed after', LIST + 'turn' + ENTER + 'x', ['key 116 asked 1', 'key 120 asked 1']),
  ('the first, picked as it opens', LIST + ENTER, ['key 104 asked 1']),
  ('down a row', LIST + DOWN + ENTER, ['key 116 asked 1']),
  ("one of the app's own, with no key: 4096 + n", LIST + 'own' + ENTER, ['key 4097 asked 1']),
  ("Esc: closed, nothing picked; then keys are the app's again", LIST + ESC + 'y', ['key 121 asked 1']),
  ('narrowed to nothing: Enter picks nothing', LIST + 'zzz' + ENTER + ESC + 'y', ['key 121 asked 1']),
  ("the app's own key for it (pc.showCommands)", 'p' + WAIT + DOWN + ENTER, ['key 112 asked 0', 'key 116 asked 1']),
  ('asked each time it opens', LIST + ENTER + WAIT + LIST + ENTER, ['key 104 asked 1', 'key 104 asked 2']),
  ('a system command, by name', LIST + 'whole display' + ENTER + 'y', ['key 121 asked 1']),
]
failed = 0
for name, typed, want in cases:
  got, err = run(typed)
  if got != want or err:
    failed += 1
    print('FAIL %s: wanted %s, got %s %s' % (name, want, got, err))
  else:
    print('ok   %s' % name)
# an app with no commandList*: the system's alone, and the app goes on
plain = os.path.join(data + '-plain', 'plain.rio')
os.makedirs(os.path.dirname(plain), exist_ok=True)
with open(plain, 'w') as f: f.write('update* :: proc()\n  k := pc.key()\n  if k >= 0\n    log("key", k)\n  end\nend\n')
got, err = run(LIST + 'switch' + ESC + 'a', plain)
if got != ['key 97'] or err:
  failed += 1
  print("FAIL no commandList*: the system's alone: %s %s" % (got, err))
else:
  print("ok   no commandList*: the system's alone")
shutil.rmtree(data, ignore_errors=True)
shutil.rmtree(data + '-plain', ignore_errors=True)
print('%d failed' % failed if failed else 'commands came through')
sys.exit(1 if failed else 0)
