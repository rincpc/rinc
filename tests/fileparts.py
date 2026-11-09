"""python fileparts.py path/to/rinc data-folder: pc.fileReadAt and pc.fileWriteAt, a part of a file at a time
(docs/spec/apps.md#files), and pc.stashReadAt and pc.stashWriteAt, the same in an app's stash: a file
made by writing at its end, added to, written over in the middle (not cut short), not written past its
end; read from a byte on, past its end (nothing, and its length), not before its start; every byte
kept (0 and 255 too), as the card has it after; an app's own files read but not written; and a stash
written even where a file from before keys stands in its folder's place. Then the same on a web page
(rinc -web, run in node by webdrop.mjs: localStorage, the web's storage), where there's node and a
player to build one with."""
import json, os, shutil, subprocess, sys, zipfile

rinc, data = os.path.abspath(sys.argv[1]), sys.argv[2]
shutil.rmtree(data, ignore_errors=True)
def put(path, text):
  path = os.path.join(data, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'w', newline='') as f: f.write(text)
def raw(path):
  path = os.path.join(data, *path.split('/'))
  return open(path, 'rb').read() if os.path.exists(path) else None

put('apps/parts/own.txt', 'its own')
put('stash/parts', 'from before keys') # (a file where the stash's folder goes)
put('apps/parts/parts.rio', '''
p := "/rinc/save/parts/deep/log"
log("made", pc.fileWriteAt(p, -1, "abc"))
log("added", pc.fileWriteAt(p, -1, "defg"))
log("over", pc.fileWriteAt(p, 1, "XY"))
log("at its end", pc.fileWriteAt(p, 7, "h"))
log("past it", pc.fileWriteAt(p, 9, "?"))
log("before it", pc.fileWriteAt(p, -2, "?"))
out: Blob[..4]
log("read", pc.fileReadAt(p, 2, out), out.asString())
out.clear()
log("past its end", pc.fileReadAt(p, 8, out), out.len())
log("before its start", pc.fileReadAt(p, -1, out), out.len())
log("none", pc.fileReadAt("/rinc/save/parts/none", 0, out), pc.fileWriteAt("/rinc", -1, "?"))
bytes: Blob[..4]
fine := bytes.push(0) and bytes.push(255) and bytes.push(10) and bytes.push(0)
log("bytes", pc.fileWriteAt("/rinc/save/parts/bytes", -1, bytes.asString()))
back: Blob[..8]
log("bytes back", pc.fileReadAt("/rinc/save/parts/bytes", 1, back), back[0], back[1], back[2])
log("own", pc.fileReadAt("own.txt", 4, out), out.asString(), pc.fileWriteAt("own.txt", -1, "?"))
log("stash", pc.stashWriteAt("undo", -1, "one"), pc.stashWriteAt("undo", -1, "two"), pc.stashWriteAt("undo", 0, "ON"))
kept: Blob[..16]
log("stash back", pc.stashReadAt("undo", 1, kept), kept.asString(), pc.stashReadAt("none", 0, kept))
''')
r = subprocess.run([rinc, '-data', data, '-app', 'parts', '-frames', '2'], capture_output=True, text=True, timeout=60)
said = (r.stdout + r.stderr).strip().replace('\r', '')
want = '''made 0
added 3
over 1
at its end 7
past it -1
before it -1
read 8 Ydef
past its end 8 0
before its start -1 0
none -1 -1
bytes 0
bytes back 4 255 10 0
own 7 own -1
stash 0 3 0
stash back 6 Netwo -1'''
failed = 0
def check(ok, why):
  global failed
  print(('ok   ' if ok else 'FAIL ') + why)
  failed += not ok
def said_right(said, where):
  lines = [l for l in said.split('\n') if l.split(' ')[0] in ('made', 'added', 'over', 'at', 'past', 'before', 'read', 'none', 'bytes', 'own', 'stash')]
  for w, got in zip(want.split('\n'), lines + [''] * 20):
    check(w == got, '%s: said %r (wanted %r)' % (where, got, w))
  check(len(lines) == len(want.split('\n')), '%s: every line said (%s)' % (where, said))
said_right(said, 'desktop')
check(raw('save/parts/deep/log') == b'aXYdefgh', 'the card has it: %r' % raw('save/parts/deep/log'))
check(raw('save/parts/bytes') == b'\x00\xff\n\x00', 'every byte: %r' % raw('save/parts/bytes'))
check(raw('stash/parts/undo') == b'ONetwo', 'the stash, in its folder: %r' % raw('stash/parts/undo'))
check(raw('apps/parts/own.txt') == b'its own', "the app's own file, as it was")
# the web: its own file carried (-files: only own.txt, so the page runs its compiled image), a zip of
# nothing dropped (webdrop.mjs's way to run a page), and what's in localStorage after
node = shutil.which('node')
page = os.path.join(data, 'page.html')
os.makedirs(os.path.join(data, 'own'))
shutil.copy(os.path.join(data, 'apps', 'parts', 'own.txt'), os.path.join(data, 'own'))
made = subprocess.run([rinc, os.path.join(data, 'apps', 'parts', 'parts.rio'), '-files', 'apps/parts=' + os.path.join(data, 'own'), '-web', page], capture_output=True, text=True)
if not node or made.returncode:
  print('(the web: not tried, %s)' % ('no node' if not node else 'no player built in'))
else:
  none = os.path.join(data, 'none.zip')
  with zipfile.ZipFile(none, 'w') as z: z.writestr('rinc/save/x.txt', 'x')
  r = subprocess.run([node, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'webdrop.mjs'), page, none], capture_output=True, timeout=60,
                     env=dict(os.environ, RINC_VALUES='1'), encoding='utf-8')
  out = (r.stdout + r.stderr).replace('\r', '')
  said_right(out.split('keys:')[0], 'web')
  stored = dict(l.split('\t', 1) for l in out.split('keys:')[-1].strip().split('\n') if '\t' in l)
  for key, value in (('parts/save/parts/deep/log', 'aXYdefgh'), ('parts/save/parts/bytes', '\x00\xff\n\x00'), ('parts/stash/parts/undo', 'ONetwo')):
    check(json.loads(stored.get(key, 'null')) == value, 'web: %s is %r' % (key, stored.get(key)))
if failed: sys.exit('FAIL: %d' % failed)
print('file parts came through')
