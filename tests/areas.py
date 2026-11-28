"""python areas.py path/to/rinc work-folder: an app's own areas, by a word and a colon, with the file calls
(docs/spec/apps.md#an-apps-own-files): save:, stash:, cache: and settings:, each where it belongs in
storage, folders in them; the area itself looked in, not changed; names that aren't a save file's, and a
word that's no area's, refused; the same file by its absolute path; a project's save space its own; and
a stash of before keys (a file where its folder goes) dropped as it's written."""
import os, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])

APP = '''out: Blob[..200]
say :: proc(what: String, n: Int)
  log(what, n, out.asString())
  out.clear()
end
say("save", pc.fileWrite("save:best", "42"))
say("folder", pc.fileWrite("save:levels/1", "L1"))
say("read", pc.fileRead("save:best", out))
say("absolute", pc.fileRead("/save/game/best", out))
say("dir", pc.dir("save:", out))
say("stash", pc.fileWrite("stash:view", "abc"))
say("stashAt", pc.fileWriteAt("stash:view", -1, "def"))
say("stashReadAt", pc.fileReadAt("stash:view", 2, out))
say("cache", pc.fileWrite("cache:thumbs", "t"))
say("settings", pc.fileWrite("settings:areas/key", "s"))
say("area", pc.fileWrite("save:", "x"))
say("hidden", pc.fileWrite("save:.x", "x"))
say("twice", pc.fileWrite("save:a//b", "x"))
say("up", pc.fileWrite("save:../x", "x"))
say("long", pc.fileWrite("save:abcdefghijabcdefghijabcdefghijabc", "x"))
say("word", pc.fileRead("nope:best", out))
say("gone", pc.fileDelete("save:levels"))
say("after", pc.fileRead("save:levels/1", out))
'''

def run(app, data):
  r = subprocess.run([rinc, '-data', data, '-app', app, '-frames', '2'], capture_output=True, text=True, timeout=60)
  return [l for l in (r.stdout + r.stderr).replace('\r', '').split('\n') if l]

def file(*parts):
  p = os.path.join(work, *parts)
  return open(p, encoding='utf-8').read() if os.path.isfile(p) else None

shutil.rmtree(work, ignore_errors=True)
os.makedirs(os.path.join(work, 'apps', 'game'))
with open(os.path.join(work, 'apps', 'game', 'game.rio'), 'w', newline='') as f: f.write(APP)
os.makedirs(os.path.join(work, 'stash'))
with open(os.path.join(work, 'stash', 'game'), 'w', newline='') as f: f.write('old') # (a stash of before keys: a file)
lines = run('game', work)
want = ['save 0', 'folder 0', 'read 2 42', 'absolute 2 42', 'dir 2 d levels\nf best\n', 'stash 0', 'stashAt 3',
        'stashReadAt 6 cdef', 'cache 0', 'settings 0', 'area -1', 'hidden -1', 'twice -1', 'up -1', 'long -1', 'word -1', 'gone 0', 'after -1']
said = '\n'.join(lines)
bad = []
at = 0
for w in want: # (in order, each where the last left off: dir's spans lines)
  k = said.find(w, at)
  if k < 0: bad.append('not said: %r' % w)
  else: at = k + len(w)
if file('save', 'game', 'best') != '42': bad.append('save:best not at /save/game/best')
if file('stash', 'game', 'view') != 'abcdef': bad.append('stash:view not at /stash/game/view: %r' % file('stash', 'game', 'view'))
if file('cache', 'game', 'thumbs') != 't': bad.append('cache:thumbs not at /cache/game/thumbs')
if file('settings', 'areas', 'key') != 's': bad.append('settings:areas/key not at /settings/areas/key')
if os.path.exists(os.path.join(work, 'save', 'game', 'levels')): bad.append('save:levels not deleted')

# a project's save space: its own, in its dev folder
os.makedirs(os.path.join(work, 'projects', 'game'))
with open(os.path.join(work, 'projects', 'game', 'game.rio'), 'w', newline='') as f: f.write('log("project", pc.fileWrite("save:mine", "p"))\n')
lines = run('game+', work)
if 'project 0' not in lines: bad.append("a project's save:mine: %r" % lines)
if file('projects', 'game', 'dev', 'save', 'mine') != 'p': bad.append("a project's save:mine not in its dev folder")
if file('save', 'game', 'mine') is not None: bad.append("a project's save:mine in the app's save space")

if bad: sys.exit('the areas: ' + '; '.join(bad) + '\n' + said)
print("an app's own areas came through")
