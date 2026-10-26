"""python files.py path/to/rinc path/to/files.rio folder: the Files app, typed at: the arrows move a row
at a time, Enter opens a folder, copying and renaming refuse a name that's taken, deleting takes its
whole name typed, and as a picker (pc.call("files", "pick ...")) it gives the path picked back to its
caller (pc.result). docs/spec/apps.md#files"""
import os, shutil, subprocess, sys

rinc, files, data = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2]), sys.argv[3]
DOWN, UP = '\\x1b[B', '\\x1b[A'

def put(path, text):
  path = os.path.join(data, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'w') as f: f.write(text)
def there(path): return os.path.exists(os.path.join(data, *path.split('/')))
def get(path):
  path = os.path.join(data, *path.split('/'))
  return open(path).read() if os.path.exists(path) else None
def typing(keys, app=None, frames=200):
  shutil.rmtree(os.path.join(data, 'stash'), ignore_errors=True) # (each from /rinc)
  args = [rinc, '-data', data] + (['-app', app] if app else [files]) + ['-frames', str(frames), '-type', keys]
  return subprocess.run(args, capture_output=True, text=True, timeout=60).stdout
def check(ok, why):
  if not ok: sys.exit('FAIL: ' + why)

shutil.rmtree(data, ignore_errors=True)
put('apps/chord/chord.rio', 'log("chord")\n')
put('projects/tune/tune.rio', 'log("tune")\n')
put('projects/tune/help/README.md', '# Tune\n')
put('save/chord/best', '42')

# /rinc (the root: no ..): apps, projects, save: down once is projects (not save: no row skipped)
typing(DOWN + '\\n' + DOWN + ' c' + 'copy1\\n')
check(get('projects/copy1/copy1.rio') == 'log("tune")\n' and get('projects/copy1/help/README.md') == '# Tune\n', 'the arrows, Enter and copying a project')
typing(DOWN + '\\n' + DOWN + ' c' + 'copy1\\n')
check(not there('projects/copy1/tune.rio'), 'copying over a name taken')
# deleting: not with a name that isn't its own; with its own, all of it
typing(DOWN + '\\ncopy1' + DOWN + ' d' + 'copy\\n\\x1b')
check(there('projects/copy1/copy1.rio'), 'deleting with the wrong name')
typing(DOWN + '\\ncopy1' + DOWN + ' d' + 'copy1\\n')
check(not there('projects/copy1'), 'deleting, its name typed')
# save files: copied, renamed (a name taken refused), and nothing changed outside /rinc
typing(DOWN + DOWN + '\\n' + DOWN + '\\n' + DOWN + ' c' + 'best2\\n' + DOWN + ' n' + 'best\\n' + ' n' + 'best3\\n')
check(get('save/chord/best') == '42' and get('save/chord/best3') == '42' and not there('save/chord/best2'), 'copying and renaming save files')
# a picker, for its caller: the path picked comes back
shutil.copytree(os.path.dirname(files), os.path.join(data, 'apps', 'files')) # (installed, to be called: its folder, parts and all)
put('apps/caller/caller.rio', '''out: Blob[96]
n := pc.result(out[])
if n < 0
  pc.call("files", "pick /rinc/save/chord")
else
  log("picked", out[0..<n].asString())
end
''')
out = typing(DOWN + '\\n', app='caller', frames=60)
check('picked /rinc/save/chord/best' in out, 'picking: got %r' % out)
# a new folder (Ctrl+Shift+N): in a folder of /rinc's, not in /rinc itself
NEW_FOLDER = '\\x1b[27;6;110~'
typing('save\\nchord\\n' + NEW_FOLDER + 'level2\\n') # (typing narrows to it: the root has more by now)
check(os.path.isdir(os.path.join(data, 'save', 'chord', 'level2')), 'making a folder')
typing(NEW_FOLDER + 'top\\n')
check(not there('top'), 'making a folder in /rinc itself')
print('files came through')
