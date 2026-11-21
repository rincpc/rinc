"""python stick.py path/to/rinc work-folder: a zip as the stick (rinc -stick, plans/drives.md), read only at /stick: / lists
rinc and stick; its folders list (each name once, folders from their files); a deflated file reads whole and
a stored one too; writing to it fails; a folder copied off it into storage (pc.fileCopy, as Files copies) is
there after, and an app copied off it runs. And a zip dropped on a Rinc with home (rinc -drop) is the stick,
not installed; on one with nothing, it installs, as before."""
import os, shutil, subprocess, sys, zipfile

rinc, work = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
here = os.path.dirname(os.path.abspath(__file__))
shutil.rmtree(work, ignore_errors=True)
os.makedirs(work)
def fail(why): sys.exit('stick: ' + why)
def run(*args):
  r = subprocess.run([rinc, *args], capture_output=True, text=True, timeout=120)
  return r.returncode, r.stdout + r.stderr

z = os.path.join(work, 'things.zip')
long = 'kept on the stick ' * 400  # (7200 bytes: deflated small)
with zipfile.ZipFile(z, 'w') as f:
  f.writestr('notes/long.txt', long, zipfile.ZIP_DEFLATED)
  f.writestr('notes/short.txt', 'short', zipfile.ZIP_STORED)
  f.writestr('notes/deeper/', '')
  f.writestr('notes/deeper/x.txt', 'x', zipfile.ZIP_DEFLATED)
  f.writestr('rinc/apps/hi/hi.rio', 'log("hi from the stick")\n', zipfile.ZIP_DEFLATED)

app = os.path.join(work, 'use', 'use.rio')
os.makedirs(os.path.dirname(app))
open(app, 'w').write('''out: Blob[..400]
got: Blob[..8000]
show :: proc(p: String)
  out.clear()
  n := pc.dir(p, out)
  log("dir", p, n, out.asString())
end
main* :: proc()
  show("/")
  show("/stick")
  show("/stick/notes")
  show("/stick/nothing")
  log("long", pc.fileRead("/stick/notes/long.txt", got), got.len())
  got.clear()
  log("short", pc.fileRead("/stick/notes/short.txt", got), got.asString())
  log("write", pc.fileWrite("/stick/new.txt", "no"))
  log("copy", pc.fileCopy("/stick/notes", "/rinc/save/use/notes"))
  log("app", pc.fileCopy("/stick/rinc/apps/hi", "/rinc/apps/hi"))
  got.clear()
  log("copied", pc.fileRead("/rinc/save/use/notes/deeper/x.txt", got), got.asString())
end
''')
data = os.path.join(work, 'data')
code, out = run(app, '-stick', z, '-data', data, '-frames', '2')
for want in ('dir / 2 d rinc\nd stick\n', 'dir /stick 2 d notes\nd rinc\n', 'dir /stick/notes 3 d deeper\nf long.txt\nf short.txt\n',
             'dir /stick/nothing -1', 'long 7200 7200', 'short 5 short', 'write -1', 'copy 0', 'app 0', 'copied 1 x'):
  if want not in out: fail('no %r in:\n%s' % (want, out))
code, out = run('-data', data, '-app', 'hi', '-frames', '2')
if 'hi from the stick' not in out: fail('the app copied off the stick: ' + out)
if run(app, '-stick', os.path.join(work, 'use', 'use.rio'), '-data', data, '-frames', '1')[0] != 2: fail('a file that is no zip, as the stick, ran')

# Files, as a person: up from /rinc to /, into the stick, an app copied (Ctrl+C), and pasted into /rinc/apps (Ctrl+V)
files = os.path.join(here, '..', 'system', 'files', 'files.rio')
fdata = os.path.join(work, 'fdata')
os.makedirs(os.path.join(fdata, 'apps', 'other'))
open(os.path.join(fdata, 'apps', 'other', 'other.rio'), 'w').write('log(1)\n')
code, out = run(files, '-data', fdata, '-stick', z, '-frames', '400', '-type', '\\x08st\\nri\\nap\\nhi\\x03\\e\\x08\\x08\\x08ri\\nap\\n\\x16')
if not os.path.exists(os.path.join(fdata, 'apps', 'hi', 'hi.rio')): fail('Files copying an app off the stick:\n' + out)

# a drop: on a Rinc with home, the stick; with nothing installed, installed (a release's bundle)
home = os.path.join(work, 'withhome')
code, out = run('-data', home, '-install', os.path.join(here, '..', 'system', 'home', 'home.rio'))
if code: fail('installing home: ' + out)
code, out = run('-data', home, '-app', 'home', '-drop', z, '-frames', '3')
if 'things.zip is in, as /stick' not in out or os.path.exists(os.path.join(home, 'apps', 'hi')): fail('a drop on a Rinc with home:\n' + out)
empty = os.path.join(work, 'empty')
code, out = run('-data', empty, '-drop', z, '-frames', '3')
if not os.path.exists(os.path.join(empty, 'apps', 'hi', 'hi.rio')): fail('a drop on an empty Rinc installs:\n' + out)
print('the stick came through')
