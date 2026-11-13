"""python helpplace.py path/to/rinc work-folder: help remembers where it was, reading an app's help (Sys+H,
then scrolled down): back after Sys+Tab to the app and Sys+Tab again, and after Sys+H again from the app
(the same text: where it was, not its top); and, not scrolled, back after Sys+Tab at the text it was
opened at, not where it was before that. Each checked by the screen."""
import os, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), sys.argv[2]
here = os.path.dirname(os.path.abspath(__file__))
shutil.rmtree(work, ignore_errors=True)
def put(path, text):
  path = os.path.join(work, *path.split('/'))
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'w', newline='') as f: f.write(text)
put('data/apps/game/game.rio', 'update* :: proc()\nend\n')
put('data/apps/game/help/README.md', '# game\n\n' + ''.join('Line %d of the help.\n\n' % k for k in range(80)))
shutil.copytree(os.path.join(here, '..', 'system', 'help'), os.path.join(work, 'data', 'apps', 'help'))
DOWN = r'\x1b[B' * 20
put('data/apps/other/other.rio', 'update* :: proc()\nend\n')
put('data/apps/other/help/README.md', "# other\n\nAnother app's help.\n")
def screen(typed, name, seen=None):
  shutil.rmtree(os.path.join(work, 'data', 'stash'), ignore_errors=True)
  if seen: put('data/stash/help/place', '0\n0 %s\n' % seen) # (where help was last: another app's help)
  shot = os.path.join(work, name + '.ppm')
  subprocess.run([rinc, '-data', os.path.join(work, 'data'), '-app', 'game', '-frames', '240', '-type', typed, '-o', shot], capture_output=True, timeout=60)
  return open(shot, 'rb').read()
scrolled = screen(r'\sh' + DOWN, 'scrolled')
top = screen(r'\sh', 'top')
bad = []
if scrolled == top: bad.append("it didn't scroll")
if screen(r'\sh' + DOWN + r'\st\st', 'tab') != scrolled: bad.append('Sys+Tab away and back: not where it was')
if screen(r'\sh' + DOWN + r'\st\sh', 'again') != scrolled: bad.append('Sys+H again: not where it was')
if screen(r'\sh\st\st', 'unscrolled', 'other/README') != top: bad.append("opened at the game's help, not scrolled, away and back: another help shown")
if bad: sys.exit('help: ' + '; '.join(bad))
print('help came back where it was')
