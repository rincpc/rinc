"""python manyhelps.py path/to/rinc work-folder: every app's help is listed (pc.helps), however many apps
there are: 20 apps here, the last made with a help folder of its own (and a folder in that), where a
list of 16 folders in all once left out all but the first 15 apps' (so Sys+H found no help for the
others)."""
import os, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), sys.argv[2]
shutil.rmtree(work, ignore_errors=True)
def put(path, text):
  path = os.path.join(work, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'w', newline='') as f: f.write(text)
for k in range(20):
  put('data/apps/a%02d/a%02d.rio' % (k, k), 'log("app")\n')
put('data/apps/a19/help/README.md', '# a19\n')
put('data/apps/a19/help/more/deep.md', '# deep\n')
put('data/apps/lister/lister.rio', 'names: Blob[..4000]\nn := pc.helps(names)\nlog(names.asString())\n')
r = subprocess.run([rinc, '-data', os.path.join(work, 'data'), '-app', 'lister', '-frames', '2'], capture_output=True, text=True, timeout=60)
out = (r.stdout + r.stderr).replace('\r', '')
lines = out.split('\n')
if 'a19/README' not in lines or 'a19/more/deep' not in lines:
  sys.exit('help of the 20th app not listed:\n%s' % out)
print('every app\'s help listed, past 16 folders')
