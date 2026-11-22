"""python stick.py path/to/rinc work-folder: a zip as the stick (rinc -stick, plans/drives.md), read only at /stick: / lists
rinc and stick; its folders list (each name once, folders from their files); a deflated file reads whole and
a stored one too; writing to it works (in memory); a folder copied off it into storage (pc.fileCopy, as Files copies) is
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
  log("copy", pc.fileCopy("/stick/notes", "/save/use/notes"))
  log("app", pc.fileCopy("/stick/rinc/apps/hi", "/apps/hi"))
  got.clear()
  log("copied", pc.fileRead("/save/use/notes/deeper/x.txt", got), got.asString())
  log("old way", pc.fileRead("/rinc/save/use/notes/deeper/x.txt", got)) # (/ is Rinc's own: plans/root.md)
end
''')
data = os.path.join(work, 'data')
code, out = run(app, '-stick', z, '-data', data, '-frames', '2')
for want in ('dir / 1 d stick\n', 'dir /stick 2 d notes\nd rinc\n', 'dir /stick/notes 3 d deeper\nf long.txt\nf short.txt\n',
             'dir /stick/nothing -1', 'long 7200 7200', 'short 5 short', 'write 0', 'copy 0', 'app 0', 'copied 1 x', 'old way -1'):
  if want not in out: fail('no %r in:\n%s' % (want, out))
code, out = run('-data', data, '-app', 'hi', '-frames', '2')
if 'hi from the stick' not in out: fail('the app copied off the stick: ' + out)
if run(app, '-stick', os.path.join(work, 'use', 'use.rio'), '-data', data, '-frames', '1')[0] != 2: fail('a file that is no zip, as the stick, ran')

# Files, as a person: up from /rinc to /, into the stick, an app copied (Ctrl+C), and pasted into /apps (Ctrl+V)
files = os.path.join(here, '..', 'system', 'files', 'files.rio')
fdata = os.path.join(work, 'fdata')
os.makedirs(os.path.join(fdata, 'apps', 'other'))
open(os.path.join(fdata, 'apps', 'other', 'other.rio'), 'w').write('log(1)\n')
code, out = run(files, '-data', fdata, '-stick', z, '-frames', '400', '-type', 'st\\nri\\nap\\nhi\\x03\\e\\x08\\x08\\x08ap\\n\\x16')
if not os.path.exists(os.path.join(fdata, 'apps', 'hi', 'hi.rio')): fail('Files copying an app off the stick:\n' + out)

# writing to it (in memory): a file made in a folder made, one written over, renamed, removed, one copied
# from storage; then the command list's "download" (pc.showCommands, then typed) writes it out as a zip
# (-downloads), which has all that (each file stored: one zip writer, rinc_treeZip, for every stick)
wapp = os.path.join(work, 'writer', 'writer.rio')
os.makedirs(os.path.dirname(wapp))
open(wapp, 'w').write('''got: Blob[..8000]
out: Blob[..400]
frames := 0
main* :: proc()
  log("w1", pc.fileWrite("/stick/made/new.txt", "made here"))
  log("w2", pc.fileWrite("/stick/notes/short.txt", "longer now"))
  log("mk", pc.dirMake("/stick/empty"))
  log("mv", pc.fileRename("/stick/notes/deeper", "/stick/notes/moved"))
  log("rm", pc.fileDelete("/stick/rinc"))
  log("save", pc.saveWrite("from", "from storage"))
  log("cp", pc.fileCopy("/save/writer/from", "/stick/copied.txt"))
  log("wr", pc.fileWrite("/save/writer/x", "storage still"))
  got.clear()
  log("back", pc.fileRead("/stick/notes/short.txt", got), got.asString())
  log("dir", pc.dir("/stick", out), out.asString())
end
update* :: proc()
  frames += 1
  if frames == 2 do pc.showCommands()
end
''')
downloads = os.path.join(work, 'downloads')
os.makedirs(downloads)
code, out = run(wapp, '-stick', z, '-data', os.path.join(work, 'wdata'), '-downloads', downloads, '-frames', '30', '-type', 'download\\n')
for want in ('w1 0', 'w2 0', 'mk 0', 'mv 0', 'rm 0', 'cp 0', 'back 10 longer now', 'the stick, downloaded:'):
  if want not in out: fail('writing to the stick: no %r in:\n%s' % (want, out))
got = os.path.join(downloads, 'things.zip')
if not os.path.exists(got): fail('the stick downloaded: no things.zip in ' + str(os.listdir(downloads)) + '\n' + out)
with zipfile.ZipFile(got) as f:
  names = set(f.namelist())
  for name, text in (('made/new.txt', 'made here'), ('notes/short.txt', 'longer now'), ('notes/long.txt', long), ('notes/moved/x.txt', 'x'), ('copied.txt', 'from storage')):
    if name not in names or f.read(name).decode() != text: fail('the downloaded zip: %s (has %s)' % (name, sorted(names)))
  if 'empty/' not in names or any(n.startswith('rinc/') or n.startswith('notes/deeper') for n in names): fail('the downloaded zip: %s' % sorted(names))
  if f.testzip() is not None: fail('the downloaded zip: damaged')
if not os.path.exists(os.path.join(work, 'wdata', 'save', 'writer', 'x')): fail('storage written beside the stick')

# a folder as the stick (rinc -stick folder): read, written to as it's changed, downloaded as a zip
folder = os.path.join(work, 'myfolder')
os.makedirs(os.path.join(folder, 'inner'))
open(os.path.join(folder, 'inner', 'a.txt'), 'w').write('from the folder')
fapp = os.path.join(work, 'onfolder', 'onfolder.rio')
os.makedirs(os.path.dirname(fapp))
open(fapp, 'w').write('''got: Blob[..200]
out: Blob[..400]
frames := 0
main* :: proc()
  log("fr", pc.fileRead("/stick/inner/a.txt", got), got.asString())
  log("fw", pc.fileWrite("/stick/made/b.txt", "made on the folder"))
  log("fmv", pc.fileRename("/stick/inner/a.txt", "/stick/inner/c.txt"))
  log("froot", pc.dir("/", out), out.asString())
end
update* :: proc()
  frames += 1
  if frames == 2 do pc.showCommands()
end
''')
fdown = os.path.join(work, 'fdownloads')
os.makedirs(fdown)
code, out = run(fapp, '-stick', folder, '-data', os.path.join(work, 'fsdata'), '-downloads', fdown, '-frames', '30', '-type', 'download\\n')
for want in ('fr 15 from the folder', 'fw 0', 'fmv 0', 'froot 1 d stick\n', 'the stick, downloaded:'):
  if want not in out: fail('a folder as the stick: no %r in:\n%s' % (want, out))
if open(os.path.join(folder, 'made', 'b.txt')).read() != 'made on the folder' or not os.path.exists(os.path.join(folder, 'inner', 'c.txt')):
  fail('a folder as the stick: not changed on disk')
with zipfile.ZipFile(os.path.join(fdown, 'myfolder.zip')) as f:
  if sorted(f.namelist()) != ['inner/c.txt', 'made/b.txt'] or f.read('made/b.txt') != b'made on the folder': fail('the folder, downloaded: %s' % f.namelist())

# a drop: on a Rinc with home, the stick; with nothing installed, installed (a release's bundle)
home = os.path.join(work, 'withhome')
code, out = run('-data', home, '-install', os.path.join(here, '..', 'system', 'home', 'home.rio'))
if code: fail('installing home: ' + out)
code, out = run('-data', home, '-app', 'home', '-drop', z, '-frames', '3')
if 'things.zip is in, as /stick' not in out or os.path.exists(os.path.join(home, 'apps', 'hi')): fail('a drop on a Rinc with home:\n' + out)
code, out = run('-data', home, '-app', 'home', '-drop', folder, '-frames', '3') # (a folder that's no app's: the stick)
if 'myfolder is in, as /stick' not in out or os.path.exists(os.path.join(home, 'apps', 'myfolder')): fail('a folder dropped on a Rinc with home:\n' + out)
code, out = run('-data', home, '-app', 'home', '-drop', os.path.dirname(wapp), '-frames', '3') # (an app's folder: installed, as before)
if not os.path.exists(os.path.join(home, 'apps', 'writer', 'writer.rio')): fail("an app's folder dropped, installed:\n" + out)
empty = os.path.join(work, 'empty')
code, out = run('-data', empty, '-drop', z, '-frames', '3')
if not os.path.exists(os.path.join(empty, 'apps', 'hi', 'hi.rio')): fail('a drop on an empty Rinc installs:\n' + out)
print('the stick came through')
