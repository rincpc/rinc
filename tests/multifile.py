"""python multifile.py path/to/rinc folder: an app of more files than one (docs/spec/apps.md#more-files):
include "parts/a.rio" and import .shapes, from its own folder, and import tween, from /rinc/lib; its
cached image used while none of them changes, and made again when any does."""
import os, shutil, subprocess, sys

rinc, data = os.path.abspath(sys.argv[1]), sys.argv[2]

def put(path, text):
  path = os.path.join(data, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'w') as f: f.write(text)
def run(name, want, why):
  out = subprocess.run([rinc, '-data', data, '-app', name, '-frames', '1'], capture_output=True, text=True, timeout=30).stdout
  out = '\n'.join(line for line in out.replace('\r', '').split('\n') if line)
  if out != want: sys.exit('FAIL: %s: got %r, wanted %r' % (why, out, want))

shutil.rmtree(data, ignore_errors=True)
put('projects/multi/multi.rio', 'include "parts/a.rio"\nimport .shapes\nimport tween\nlog(part(), shapes.area(3), tween.ease(2))\n')
put('projects/multi/parts/a.rio', 'part :: proc() -> String\n  return "a"\nend\n')
put('projects/multi/shapes/shapes.rio', 'area* :: proc(n: Int) -> Int\n  return n * n\nend\n')
put('lib/tween.rio', 'ease* :: proc(n: Int) -> Int\n  return n + 100\nend\n')
run('multi*', 'a 9 102', 'its files, compiled')
if not os.path.exists(os.path.join(data, 'cache', 'rinc', 'projects', 'multi.rioi')): sys.exit('FAIL: no image cached')
run('multi*', 'a 9 102', 'its image')
put('projects/multi/parts/a.rio', 'part :: proc() -> String\n  return "b"\nend\n')
run('multi*', 'b 9 102', 'an included file changed: compiled again')
put('lib/tween.rio', 'ease* :: proc(n: Int) -> Int\n  return n + 200\nend\n')
run('multi*', 'b 9 202', "the library's file changed: compiled again")
os.remove(os.path.join(data, 'projects', 'multi', 'shapes', 'shapes.rio'))
out = subprocess.run([rinc, '-data', data, '-app', 'multi*', '-frames', '1'], capture_output=True, text=True, timeout=30).stdout
if 'module not found: shapes' not in out: sys.exit('FAIL: a file gone: got %r' % out)
print('more files came through')
