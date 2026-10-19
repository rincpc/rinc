"""python editor.py path/to/rinc path/to/system folder: the editor, on files by path, typed at: it starts
with the active app or project, saves, opens a file through Files (Ctrl+O), saves as through Files
(Ctrl+Shift+S), saves an installed app's file in its project, made first, and does a command from its
list (Ctrl+Shift+P). docs/spec/apps.md#the-editor"""
import os, shutil, subprocess, sys

rinc, system, data = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2]), sys.argv[3]
SAVE, OPEN, SAVE_AS, ALL = '\\x13', '\\x0f', '\\x1b[27;6;115~', '\\x1b[27;6;112~'
DOWN = '\\x1b[B'

def put(path, text):
  path = os.path.join(data, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'w', newline='') as f: f.write(text)
def get(path):
  path = os.path.join(data, *path.split('/'))
  return open(path, newline='').read() if os.path.exists(path) else None
def typing(keys, frames=120):
  return subprocess.run([rinc, '-data', data, '-app', 'edit', '-frames', str(frames), '-type', keys], capture_output=True, text=True, timeout=60).stdout
def check(ok, why):
  if not ok: sys.exit('FAIL: ' + why)

shutil.rmtree(data, ignore_errors=True)
for app in ('edit', 'files'): # (installed: their folders, parts and all)
  shutil.copytree(os.path.join(system, app), os.path.join(data, 'apps', app))
put('projects/demo/demo.rio', 'log(1)\n')
put('projects/demo/parts/a.rio', 'a :: 1\n')
put('apps/live/live.rio', 'log(2)\n')
put('settings/active', 'demo*')

typing('x' + SAVE)
check(get('projects/demo/demo.rio') == 'xlog(1)\n', 'the active project, edited and saved: %r' % get('projects/demo/demo.rio'))
typing(OPEN + DOWN + '\\n' + DOWN + '\\n') # (Files: .., parts, demo.rio; then .., a.rio)
typing('y' + SAVE)
check(get('projects/demo/parts/a.rio') == 'ya :: 1\n', 'a file opened through Files, edited and saved: %r' % get('projects/demo/parts/a.rio'))
typing('z' + SAVE_AS + 'b.rio\\n')
check(get('projects/demo/parts/b.rio') == 'yza :: 1\n' and get('projects/demo/parts/a.rio') == 'ya :: 1\n', 'saved as another, through Files')
put('settings/active', 'live')
typing('w' + SAVE)
check(get('apps/live/live.rio') == 'log(2)\n' and get('projects/live/live.rio') == 'wlog(2)\n', "an installed app's file, saved in its project")
typing('v' + ALL + 'save\\n')
check(get('projects/live/live.rio') == 'wvlog(2)\n', 'a command from the list')
print('the editor came through')
