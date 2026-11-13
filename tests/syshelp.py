"""python syshelp.py path/to/rinc work-folder: Sys+H opens the help app at the running app's own help
(apps/x/help/README.md) when the help app is in storage, not built in (as on a board, where the card
has the system apps): a stand-in help app here says the topic it was opened at. Once, the check for the
help app read it into no room, took "too big" for "not there", and said there was no help."""
import os, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), sys.argv[2]
shutil.rmtree(work, ignore_errors=True)
def put(path, text):
  path = os.path.join(work, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'w', newline='') as f: f.write(text)
put('data/apps/game/game.rio', 'update* :: proc()\nend\n')
put('data/apps/game/help/README.md', '# game\n')
put('data/apps/help/help.rio', 'topic: Blob[..96]\nn := pc.helpTopic(topic)\nlog("help at", topic.asString())\n')
r = subprocess.run([rinc, '-data', os.path.join(work, 'data'), '-app', 'game', '-frames', '20', '-type', r'\sh'], capture_output=True, text=True, timeout=60)
out = (r.stdout + r.stderr).replace('\r', '')
if 'help at game/README' not in out.split('\n'):
  sys.exit("Sys+H didn't open the game's help:\n%s" % out)
print("Sys+H opened the app's help")
