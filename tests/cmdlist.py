"""python cmdlist.py path/to/rinc work-folder: the command list (Sys+P) and what's around it:
- Print Screen while it's up saves the screen (it used to wait for the list to close)
- another app started while it's up (Sys+H here; Ctrl-C over serial starting the console, on a board)
  starts without it: the list closed, not left over the new app taking its typing and its frames"""
import os, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), sys.argv[2]
shutil.rmtree(work, ignore_errors=True)
def put(path, text):
  path = os.path.join(work, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'w', newline='') as f: f.write(text)
put('data/apps/game/game.rio', 'update* :: proc()\nend\ndraw* :: proc()\n  pc.cls(3)\nend\n')
put('data/apps/game/help/README.md', '# game\n')
put('data/apps/help/help.rio', 'frames := 0\nupdate* :: proc()\n  frames += 1\n  if frames == 5 do log("help ran its frames")\nend\n')
def run(typed):
  r = subprocess.run([rinc, '-data', os.path.join(work, 'data'), '-app', 'game', '-frames', '40', '-type', typed], capture_output=True, text=True, timeout=60)
  return (r.stdout + r.stderr).replace('\r', '')
failed = 0
run(r'\sp\w\x1b[57361u')
if not os.path.exists(os.path.join(work, 'data', 'shots', 'game-1.png')):
  print('FAIL: Print Screen with the command list up saved nothing'); failed += 1
out = run(r'\sp\w\sh')
if 'help ran its frames' not in out.split('\n'):
  print('FAIL: the app started with the command list up ran no frames (the list left over it):\n' + out[-400:]); failed += 1
shutil.rmtree(work, ignore_errors=True)
if failed: sys.exit(1)
print('the command list came through')
