"""python projects.py path/to/rinc folder: a project (chord*) runs with a save space of its own, its
app's saves copied in on its first run, so the app's own aren't touched; its compiled image is its own;
and installing it copies it over the app (its save space aside). docs/spec/apps.md#projects"""
import os, shutil, subprocess, sys

rinc, data = os.path.abspath(sys.argv[1]), sys.argv[2]

def put(path, text):
  path = os.path.join(data, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'w') as f: f.write(text)
def get(path):
  path = os.path.join(data, *path.split('/'))
  return open(path).read() if os.path.exists(path) else None
def run(name, want, why):
  out = subprocess.run([rinc, '-data', data, '-app', name, '-frames', '1'], capture_output=True, text=True, timeout=30).stdout
  if not out.startswith(want): sys.exit('FAIL: %s: got %r, wanted %r' % (why, out, want))

app = '''note: Blob[16]
n := pc.saveRead("note", note[0:16])
log("%s", note[0:max(n, 0)].asString())
ok := pc.saveWrite("note", "%s")
'''
shutil.rmtree(data, ignore_errors=True)
put('apps/chord/chord.rio', app % ('app', 'by the app'))
put('apps/chord/help/README.md', '# Chord\n')
put('save/chord/note', 'live')
put('projects/chord/chord.rio', app % ('project', 'by the project'))
put('projects/chord/help/README.md', '# Chord, in work\n')
put('apps/inst/inst.rio', 'log(pc.appInstall("chord"))\n')

run('chord*', 'project live', "the project's first run: the app's saves, copied in")
if get('save/chord/note') != 'live': sys.exit("FAIL: the project changed the app's saves")
run('chord*', 'project by the project', "the project's own saves")
run('chord', 'app live', "the app's saves, untouched")
for img in ('cache/rinc/apps/chord.img', 'cache/rinc/projects/chord.img'):
  if get(img) is None: sys.exit('FAIL: no ' + img)
run('inst', '0', 'install')
if get('apps/chord/chord.rio') != get('projects/chord/chord.rio') or get('apps/chord/help/README.md') != '# Chord, in work\n':
  sys.exit("FAIL: install didn't copy the project")
if get('apps/chord/save/note') is not None: sys.exit("FAIL: install copied the project's saves")
run('chord', 'project by the app', 'the installed project, as the app, with the app\'s saves')
print('projects came through')
