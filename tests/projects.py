"""python projects.py path/to/rinc folder: a project (chord+) runs with a save space of its own, its
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
  out = '\n'.join(line for line in out.replace('\r', '').split('\n') if line) # (lines, as typed: \r\n where it says them)
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
put('apps/helpls/helpls.rio', '''names: Blob[4096]
text: Blob[64]
n := pc.helps(names[:])
at := 0
for k in 0..<names.len()
  if names[k] == 10
    if names[at:k].asString() == "chord+/README"
      t := pc.helpRead("chord+/README", text[:])
      log(names[at:k].asString(), text[0:t].asString())
    end
    at = k + 1
  end
end
''')

run('chord+', 'project live', "the project's first run: the app's saves, copied in")
if get('save/chord/note') != 'live': sys.exit("FAIL: the project changed the app's saves")
run('chord+', 'project by the project', "the project's own saves")
run('chord', 'app live', "the app's saves, untouched")
for img in ('cache/rinc/apps/chord.rioi', 'cache/rinc/projects/chord.rioi'): # (binary: there, not read as text)
  if not os.path.exists(os.path.join(data, *img.split('/'))): sys.exit('FAIL: no ' + img)
run('helpls', 'chord+/README # Chord, in work', "the project's help, listed and read")
put('apps/setact/setact.rio', '''v: Blob[34]
log(pc.settingRead("active", v[:]), pc.settingWrite("active", "chord+"))
n := pc.settingRead("active", v[:])
log(v[0:n].asString())
''')
run('setact', '-1 0\nchord+', 'the active setting, shared by the apps')
if get('settings/active') != 'chord+': sys.exit("FAIL: the active setting isn't in /rinc/settings")
run('inst', '0', 'install')
if get('apps/chord/chord.rio') != get('projects/chord/chord.rio') or get('apps/chord/help/README.md') != '# Chord, in work\n':
  sys.exit("FAIL: install didn't copy the project")
if get('apps/chord/save/note') is not None: sys.exit("FAIL: install copied the project's saves")
run('chord', 'project by the app', 'the installed project, as the app, with the app\'s saves')
put('apps/ops/ops.rio', '''log(pc.appCopy("chord", "tune+"), pc.appCopy("chord", "tune+"), pc.appCopy("chord", "nope"))
log(pc.appRename("tune+", "song+"), pc.appRename("chord", "x+"), pc.settingWrite("active", "song+"))
log(pc.appDelete("song+"), pc.appDelete("song+"))
''')
run('ops', '0 -1 -1\n0 -1 0\n0 -1', 'copy, rename, delete')
if get('settings/active') is not None: sys.exit("FAIL: the deleted project is still active")
if os.path.exists(os.path.join(data, 'projects', 'song')) or os.path.exists(os.path.join(data, 'projects', 'tune')):
  sys.exit("FAIL: a deleted or renamed project's folder is still there")
put('apps/ops2/ops2.rio', '''log(pc.appCopy("chord", "tune+"))
log(pc.appRename("tune+", "song+"))
''')
run('ops2', '0\n0', 'copy, then rename')
if get('projects/song/song.rio') != get('apps/chord/chord.rio') or get('projects/song/help/README.md') is None:
  sys.exit("FAIL: the copy, renamed, isn't the app's files under its new name")
print('projects came through')
