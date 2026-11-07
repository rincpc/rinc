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
def typing(keys, app=None, frames=200, extra=(), keep=False): # (extra: more of rinc's options, as -hold and -shift; keep: where it was, its stash)
  if not keep:
    shutil.rmtree(os.path.join(data, 'stash'), ignore_errors=True) # (each from /rinc)
  shutil.rmtree(os.path.join(data, 'cache'), ignore_errors=True) # (and /rinc as it was: apps, projects, save; not the last run's log)
  args = [rinc, '-data', data] + (['-app', app] if app else [files]) + ['-frames', str(frames), '-type', keys] + list(extra)
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
outIn: Blob[..] = out
n := pc.result(outIn)
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
# the command list (the system key with P, or Ctrl+Shift+P here): its keys, and what the menu offers
# for what's picked, by name (renaming: the name given to the prompt; then up a folder, by name too)
LIST, CTRL_SHIFT_P = '\\sp', '\\x1b[27;6;112~'
typing('save\\nchord\\nbest3' + LIST + 'rename\\n' + 'best4\\n') # (typing narrows the list to it)
check(get('save/chord/best4') == '42' and not there('save/chord/best3'), 'renaming from the command list')
typing('save\\nchord\\n' + CTRL_SHIFT_P + '\\w' + 'up a folder\\n' + 'chord' + LIST + 'another name\\n' + 'chord2\\n')
check(there('save/chord2/best4'), 'Ctrl+Shift+P, up a folder, then copying, from the list')
# several selected, as Windows' Explorer has it: Shift+arrows from the anchor, Ctrl+X, then Ctrl+V in another
# folder moves them; Ctrl+A and Ctrl+C, pasted where they are, copy them under new names (-2); Delete, with
# delete typed, deletes all that's selected; F2 renames the one picked
SHIFT_DOWN, CTRL_A, CTRL_X, CTRL_C, CTRL_V, ALT_UP = '\\x1b[1;2B', '\\x01', '\\x18', '\\x03', '\\x16', '\\x1b[1;3A'
DEL, F2, CTRL_SPACE, CTRL_DOWN = '\\x1b[3~', '\\x1bOQ', '\\x1b[27;5;32~', '\\x1b[1;5B'
for name in ('a1', 'a2', 'a3'): put('save/multi/' + name, name)
os.makedirs(os.path.join(data, 'save', 'dest'), exist_ok=True)
typing('save\\nmulti\\n' + DOWN + SHIFT_DOWN + CTRL_X + ALT_UP + 'dest\\n' + CTRL_V)
check(get('save/dest/a1') == 'a1' and get('save/dest/a2') == 'a2' and not there('save/multi/a1') and there('save/multi/a3'), 'Shift+Down, Ctrl+X, then Ctrl+V elsewhere: moved')
typing('save\\ndest\\n' + CTRL_A + CTRL_C + CTRL_V)
check(get('save/dest/a1-2') == 'a1' and get('save/dest/a2-2') == 'a2', 'Ctrl+A, Ctrl+C, Ctrl+V where they are: copies, named apart')
typing('save\\ndest\\n' + DOWN + CTRL_DOWN + CTRL_DOWN + CTRL_SPACE + DEL + 'delete\\n') # (a1; Ctrl+Down twice, past a1-2 to a2, the selection kept; Ctrl+Space adds a2)
check(not there('save/dest/a1') and not there('save/dest/a2') and there('save/dest/a1-2'), 'Ctrl+Down, Ctrl+Space, then Delete: what was selected, deleted')
typing('save\\nmulti\\n' + DOWN + F2 + 'a9\\n')
check(get('save/multi/a9') == 'a3' and not there('save/multi/a3'), 'F2: the one picked, renamed')
out = typing('save\\nmulti' + CTRL_X + '\\n' + CTRL_V) # (multi cut, then pasted in multi: into itself)
check(there('save/multi/a9') and not there('save/multi/multi'), 'a folder not moved into itself')
# Shift held with an arrow, on a keyboard: the arrow typed (with Shift) and held as a button at once, so
# one row moved, two selected (-shift: Shift held, as pc.modsHeld says; the arrow typed at frame 1, \w)
for name in ('k1', 'k2', 'k3'): put('save/keys/' + name, name)
typing('save\\nkeys\\n' + DOWN + '\\w' + SHIFT_DOWN + '\\w' + CTRL_X + ALT_UP + 'dest\\n' + CTRL_V, extra=['-shift', '3', '-hold', 'down 1-2'])
check(there('save/dest/k1') and there('save/dest/k2') and not there('save/dest/k3') and there('save/keys/k3'), 'Shift+Down held: one row, two selected')
# renaming edits the name as a text box does (pc.Field): it starts selected, but for a file's extension;
# right goes to the selection's end, and Backspace takes one
LEFT_KEY, RIGHT_KEY, END_KEY = '\\x1b[D', '\\x1b[C', '\\x1b[F'
put('save/edit/tune.png', 'tune')
typing('save\\nedit\\n' + DOWN + F2 + 'song\\n')
check(get('save/edit/song.png') == 'tune', 'F2: the name selected, not its extension, typed over')
typing('save\\nedit\\n' + DOWN + F2 + RIGHT_KEY + '2\\n')
check(get('save/edit/song2.png') == 'tune', "F2, right: the cursor at the name's end, typed there")
typing('save\\nedit\\n' + DOWN + F2 + END_KEY + '\\x08\\x08\\x08' + 'jpg\\n')
check(get('save/edit/song2.jpg') == 'tune', 'F2, End, Backspace: the extension edited')
# a controller (Files is a pointer app): X selects the one picked, or not; the left trigger held is Shift,
# so the d-pad selects a run
for name in ('p1', 'p2', 'p3'): put('save/pad/' + name, name)
typing('save\\npad\\n' + DOWN + '\\w' + '\\f' * 6 + CTRL_X + ALT_UP + 'dest\\n' + CTRL_V, extra=['-hold', 'lt 1-12, down 3-4']) # (\f: a frame passes)
check(there('save/dest/p1') and there('save/dest/p2') and not there('save/dest/p3'), 'the left trigger and the d-pad: a run selected')
# deleting a folder: all of it, folders in folders too, its name typed; and among several selected
put('save/tree/top', '1'); put('save/tree/a/mid', '2'); put('save/tree/a/b/deep', '3')
typing('save\\ntree' + DEL + 'tree\\n')
check(not there('save/tree') and there('save/dest'), 'a folder deleted, all that was in it')
put('save/trees/t1/a/x', '1'); put('save/trees/t2/y', '2'); put('save/trees/keep', '3')
typing('save\\ntrees\\n' + DOWN + SHIFT_DOWN + DEL + 'delete\\n')
check(not there('save/trees/t1') and not there('save/trees/t2') and there('save/trees/keep'), 'several folders deleted, as selected')
# where it was, kept: started again (as after going to another app and back), the same folder, the
# same one picked and the same selected, so Ctrl+X takes what was selected
for name in ('v1', 'v2', 'v3'): put('save/view/' + name, name)
typing('save\\nview\\n' + DOWN + SHIFT_DOWN)
typing(CTRL_X + ALT_UP + 'dest\\n' + CTRL_V, keep=True)
check(there('save/dest/v1') and there('save/dest/v2') and not there('save/dest/v3'), 'started again: the folder, the pick and what was selected')
# a big folder, copied and deleted whole: more names than a listing at a time holds (60, longish), one
# over 47 characters, and folders 7 deep
for i in range(60): put('save/big/file-with-a-longish-name-%02d' % i, str(i))
for i in range(30): put('save/big/z%02d' % i, str(i)) # (short names after long ones: a run that's full of long ones has room for a short one, which mustn't be taken out of turn)
put('save/big/' + 'n' * 60, 'long')
put('save/big/d1/d2/d3/d4/d5/d6/d7/deep', 'deep')
typing('save\\nbig' + CTRL_C + '\\e' + 'dest\\n' + CTRL_V) # (Esc: what's typed cleared)
check(len(os.listdir(os.path.join(data, 'save', 'dest', 'big'))) == 92 and get('save/dest/big/d1/d2/d3/d4/d5/d6/d7/deep') == 'deep', 'a big folder copied whole')
typing('save\\nbig' + DEL + 'big\\n')
check(not there('save/big'), 'a big folder deleted whole')
print('files came through')
