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
for want in ('dir / 2 d stash\nd stick\n', 'dir /stick 2 d notes\nd rinc\n', 'dir /stick/notes 3 d deeper\nf long.txt\nf short.txt\n',
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
code, out = run(files, '-data', fdata, '-stick', z, '-frames', '400', '-type', 'sti\\nri\\nap\\nhi\\x03\\e\\x08\\x08\\x08ap\\n\\x16')
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
  log("save", pc.fileWrite("save:from", "from storage"))
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
for want in ('fr 15 from the folder', 'fw 0', 'fmv 0', 'froot 2 d stash\nd stick\n', 'the stick, downloaded:'):
  if want not in out: fail('a folder as the stick: no %r in:\n%s' % (want, out))
if open(os.path.join(folder, 'made', 'b.txt')).read() != 'made on the folder' or not os.path.exists(os.path.join(folder, 'inner', 'c.txt')):
  fail('a folder as the stick: not changed on disk')
with zipfile.ZipFile(os.path.join(fdown, 'myfolder.zip')) as f:
  if sorted(f.namelist()) != ['inner/c.txt', 'made/b.txt'] or f.read('made/b.txt') != b'made on the folder': fail('the folder, downloaded: %s' % f.namelist())

# Files' long pastes, a piece a frame (jobs.rio): a folder of 60 files and a 20 KB one, copied onto a folder
# stick: with few frames, part of it there (the screen keeps up meanwhile); with enough, all of it, each
# byte; Esc soon after the paste stops it, no file half there; cut and pasted onto the stick, moved
# (copied there, then gone from storage)
def longPaste(keys, frames, name, *more):
  d = os.path.join(work, name)
  stick = os.path.join(d, 'plugged')
  os.makedirs(os.path.join(d, 'save', 'many'))
  os.makedirs(stick)
  for i in range(60): open(os.path.join(d, 'save', 'many', 'f%02d.txt' % i), 'w').write('file %d\n' % i)
  open(os.path.join(d, 'save', 'many', 'big.bin'), 'wb').write(bytes(range(256)) * 80)
  run(files, '-data', d, '-stick', stick, '-frames', str(frames), '-type', keys, *more)
  there = os.path.join(stick, 'many')
  return d, stick, (sorted(os.listdir(there)) if os.path.isdir(there) else [])
COPY = 'sa\\nmany\\x03\\e\\x08\\x08sti\\n\\x16'
d, stick, got = longPaste(COPY, 400, 'long-all')
if len(got) != 61 or open(os.path.join(stick, 'many', 'big.bin'), 'rb').read() != bytes(range(256)) * 80 or open(os.path.join(stick, 'many', 'f42.txt')).read() != 'file 42\n':
  fail('a long paste, all of it: %d of 61' % len(got))
DELETE = '\\x08\\x08sti\\nmany\\x1b[3~many\\n' # (Delete, then its name typed: a folder, a piece a frame too)
run(files, '-data', d, '-stick', stick, '-frames', '40', '-type', DELETE, '-board') # (-board: as fast as the board, its file calls their cost)
left = os.listdir(os.path.join(stick, 'many')) if os.path.isdir(os.path.join(stick, 'many')) else []
if not 0 < len(left) < 61: fail('a long delete, a piece a frame: %d of 61 left after a few frames' % len(left))
run(files, '-data', d, '-stick', stick, '-frames', '400', '-type', DELETE)
if os.path.exists(os.path.join(stick, 'many')): fail('a long delete, all of it: %s left' % os.listdir(os.path.join(stick, 'many')))
# a job says so at once: after only a few frames (nothing copied yet, still looking), it's begun, its box
# up (the log says so as the box does)
code, out = run(files, '-data', os.path.join(work, 'long-all'), '-stick', stick, '-frames', '3', '-type', COPY, '-board')
if 'files: copying 1 to /stick' not in out: fail('a paste, said at once: ' + out)
# a plan as long as it needs to be (in Files' cache, not its memory): a folder of 400 files, each copied
d = os.path.join(work, 'long-400')
os.makedirs(os.path.join(d, 'save', 'lots'))
os.makedirs(os.path.join(d, 'plugged'))
for i in range(400): open(os.path.join(d, 'save', 'lots', 'a-rather-long-file-name-%03d.txt' % i), 'w').write('%d' % i)
code, out = run(files, '-data', d, '-stick', os.path.join(d, 'plugged'), '-frames', '600', '-type', 'sa\\nlots\\x03\\e\\x08\\x08sti\\n\\x16')
there = os.path.join(d, 'plugged', 'lots')
if not os.path.isdir(there) or len(os.listdir(there)) != 400 or 'Copied 400 of 400 files' not in out: fail('400 files: %s' % (len(os.listdir(there)) if os.path.isdir(there) else out))
# what doesn't go by the plan: a file on a zip stick that can't be read (damaged), copied off with the rest:
# said in the log, counted, the rest copied
bad = os.path.join(work, 'damaged.zip')
with zipfile.ZipFile(bad, 'w') as f:
  f.writestr('box/good.txt', 'fine', zipfile.ZIP_STORED)
  f.writestr('box/bad.txt', 'x' * 2000, zipfile.ZIP_DEFLATED)
  f.writestr('box/also.txt', 'also fine', zipfile.ZIP_STORED)
b = bytearray(open(bad, 'rb').read())
at = b.find(b'box/bad.txt') + len('box/bad.txt') # (its local header's name, then its deflated bytes: one changed)
b[at + 2] ^= 0xFF
open(bad, 'wb').write(bytes(b))
d = os.path.join(work, 'damaged')
os.makedirs(os.path.join(d, 'save', 'here'))
code, out = run(files, '-data', d, '-stick', bad, '-frames', '200', '-type', 'sti\\nbox\\x03\\e\\x08\\x08sa\\nhere\\n\\x16')
copied = os.path.join(d, 'save', 'here', 'box')
if not (os.path.exists(os.path.join(copied, 'good.txt')) and os.path.exists(os.path.join(copied, 'also.txt')) and not os.path.exists(os.path.join(copied, 'bad.txt'))):
  fail('a damaged file, the rest copied: %s\n%s' % (os.listdir(copied) if os.path.isdir(copied) else '-', out))
if '1 couldn\'t be' not in out or 'bad.txt' not in out: fail('a damaged file, said and counted:\n' + out)
# what's copied is what's shown: up from /stick (it picked, and selected), then save typed (stick hidden):
# Ctrl+C copies /save, not the stick, which the filter hid
d = os.path.join(work, 'hidden-copy')
os.makedirs(os.path.join(d, 'stash', 'files'))
os.makedirs(os.path.join(d, 'save', 'x'))
os.makedirs(os.path.join(d, 'plugged'))
open(os.path.join(d, 'stash', 'files', 'place'), 'w').write('/stick')
run(files, '-data', d, '-stick', os.path.join(d, 'plugged'), '-frames', '30', '-type', '\\esa\\x03')
clipDir = os.path.join(d, 'stash', 'rinc', 'clip')
last = open(os.path.join(clipDir, 'last')).read().strip() if os.path.exists(os.path.join(clipDir, 'last')) else ''
held = open(os.path.join(clipDir, last + '.data')).read() if last else ''
if '/save' not in held or '/stick' in held: fail('copied with a filter typed: %r, not what was shown' % held)
d, stick, got = longPaste(COPY + '\\w\\f\\f\\f\\x08sa\\n', 60, 'long-moving', '-board') # (keys typed while it copies: its box up, they don't move Files, and it goes on)
place = open(os.path.join(d, 'stash', 'files', 'place')).read() if os.path.exists(os.path.join(d, 'stash', 'files', 'place')) else ''
if place != '/stick' or not 0 < len(got) < 61: fail('keys typed during a long paste (its box up: only Esc counts): in %r, %d of 61 copied' % (place, len(got)))
d, stick, got = longPaste(COPY, 45, 'long-part', '-board')
if not 0 < len(got) < 61: fail('a long paste, a piece a frame: %d of 61 after a few frames' % len(got))
for name in got: # (Files stopped mid-paste, leaving: each file there whole, none half copied)
  path = os.path.join(stick, 'many', name)
  whole = open(path, 'rb').read() == bytes(range(256)) * 80 if name == 'big.bin' else open(path).read() == 'file %d\n' % int(name[1:3])
  if not whole: fail('a long paste, left part way: %s not whole' % name)
d = os.path.join(work, 'long-left') # (one big file, 50 pieces; Files left in the middle of it: nothing half there)
os.makedirs(os.path.join(d, 'save', 'huge'))
os.makedirs(os.path.join(d, 'plugged'))
open(os.path.join(d, 'save', 'huge', 'huge.bin'), 'wb').write(bytes(range(256)) * 1600)
run(files, '-data', d, '-stick', os.path.join(d, 'plugged'), '-frames', '40', '-type', 'sa\\nhuge\\x03\\e\\x08\\x08sti\\n\\x16', '-board')
if not os.path.isdir(os.path.join(d, 'plugged', 'huge')) or os.path.exists(os.path.join(d, 'plugged', 'huge', 'huge.bin')):
  fail('Files left in the middle of a big file: %s' % os.listdir(os.path.join(d, 'plugged')))
d, stick, got = longPaste(COPY + '\\e', 400, 'long-stopped', '-board')
if len(got) >= 61 or (os.path.exists(os.path.join(stick, 'many', 'big.bin')) and os.path.getsize(os.path.join(stick, 'many', 'big.bin')) != 20480):
  fail('a long paste, Esc: stopped, nothing half there: %d of 61' % len(got))
d, stick, got = longPaste('sa\\nmany\\x18\\e\\x08\\x08sti\\n\\x16', 400, 'long-moved')
if len(got) != 61 or os.path.exists(os.path.join(d, 'save', 'many')): fail('a long move, onto the stick: %d there, still in storage: %s' % (len(got), os.path.exists(os.path.join(d, 'save', 'many'))))

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
