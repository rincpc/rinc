"""python webplayer.py path/to/rinc work-folder: a page gets the smaller web player it can (docs/guide/web.md#the-two-players):
an app that only runs gets the VM-only one; one that writes apps, by importing Rinc's library appfiles
(as Files does, with no pc.rio* of its own), gets the one with the compiler, so the apps it writes run."""
import os, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
shutil.rmtree(work, ignore_errors=True)
apps = {'plain': 'log("hi")\n',
        'writer': 'import appfiles\nlog(appfiles.save("made+", "log(1)\\n"))\n'}
size = {}
for name, src in apps.items():
  os.makedirs(os.path.join(work, name))
  path = os.path.join(work, name, name + '.rio')
  with open(path, 'w', newline='') as f: f.write(src)
  page = os.path.join(work, name + '.html')
  r = subprocess.run([rinc, path, '-web', page], capture_output=True, text=True, timeout=60)
  if r.returncode or not os.path.exists(page): sys.exit('rinc -web %s: %s' % (name, r.stdout + r.stderr))
  size[name] = os.path.getsize(page)
if size['writer'] < size['plain'] + 30000:
  sys.exit("the app that writes apps didn't get the player with the compiler: its page %d bytes, a plain app's %d" % (size['writer'], size['plain']))
print("each page got its player: %d bytes, and %d with the compiler" % (size['plain'], size['writer']))
