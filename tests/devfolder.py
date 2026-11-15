"""python devfolder.py path/to/rinc folder: an app's dev folder, dev/ (docs/spec/apps.md#storage), its
maker's, not the app's: left out of -install (an app's folder: all the rest of it, no list of files) and
of pc.appInstall (a project installed as its app); a project's own saves in it (dev/save), and a
project's saves from before it (save/) moved in as it first runs; its tests, dev/test.rio, run as the
project (game+/dev/test) and from a folder (rinc game/dev/test.rio), the app's other files from the
app's folder."""
import os, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
data = os.path.join(work, 'data')
failed = 0
def put(path, text):
  path = os.path.join(work, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'w', newline='\n') as f: f.write(text)
def there(path): return os.path.exists(os.path.join(data, *path.split('/')))
def run(args):
  r = subprocess.run([rinc, '-data', data] + args + ['-frames', '1'], capture_output=True, text=True, timeout=30)
  return '\n'.join(l for l in (r.stdout + r.stderr).replace('\r', '').split('\n') if l)
def check(ok, what, got=''):
  global failed
  print(('ok   ' if ok else 'FAIL ') + what + ('' if ok else ': ' + repr(got)))
  failed += not ok

shutil.rmtree(work, ignore_errors=True)
game = {'game/game.rio': 'include "parts/a.rio"\n_ = pc.saveWrite("best", part())\nlog("game", part())\n',
        'game/parts/a.rio': 'part :: proc() -> String do return "a"\n',
        'game/tune.mus': 'not rio: installed all the same\n',
        'game/dev/test.rio': 'include "parts/a.rio"\nassert(part() == "a", "its part")\nlog("tests passed")\n',
        'game/dev/notes.txt': 'the maker\'s\n'}
for p, t in game.items(): put(p, t)
got = run(['-install', os.path.join(work, 'game') + '/'])
check(there('apps/game/game.rio') and there('apps/game/parts/a.rio') and there('apps/game/tune.mus') and not there('apps/game/dev'),
      '-install: all of its folder (no files.txt), but its dev folder', got)
got = run([os.path.join(work, 'game', 'dev', 'test.rio')])
check('tests passed' in got, 'its tests from the folder: game/dev/test.rio, its part from game/', got)
for p, t in game.items(): put('data/projects/' + p, t)
got = run(['-app', 'game+'])
check('game a' in got and there('projects/game/dev/save/best'), 'a project\'s own saves: in its dev folder, dev/save', got)
got = run(['-app', 'game+/dev/test'])
check('tests passed' in got, 'its tests run as the project: game+/dev/test', got)
put('data/projects/old/old.rio', 'out: Blob[..8]\nlog("kept", pc.saveRead("kept", out), out.asString())\n')
put('data/projects/old/save/kept', 'yes')
got = run(['-app', 'old+'])
check('kept 3 yes' in got and there('projects/old/dev/save/kept') and not there('projects/old/save'), 'a project\'s saves from before the dev folder (save/): moved in, as they were', got)
put('data/apps/installer/installer.rio', 'log("installed", pc.appInstall("game"))\n')
shutil.rmtree(os.path.join(data, 'apps', 'game'), ignore_errors=True)
got = run(['-app', 'installer'])
check('installed 0' in got and there('apps/game/game.rio') and there('apps/game/parts/a.rio') and not there('apps/game/dev'), 'pc.appInstall: a project as its app, but its dev folder', got)
shutil.rmtree(work, ignore_errors=True)
if failed: sys.exit(1)
print('the dev folder came through')
