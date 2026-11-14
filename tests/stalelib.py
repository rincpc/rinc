"""python stalelib.py path/to/rinc work-folder: a card's /rinc/lib copy that isn't Rinc's own (the library built
in) is said, in the app's log, when an app reads it (the card's wins, so it's what to look at when an app
goes wrong): Rinc's own copy says nothing; a changed one is said; one that stops the compile partway (an
older copy an app no longer fits, say) is said too."""
import os, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), sys.argv[2]
here = os.path.dirname(os.path.abspath(__file__))
own = open(os.path.join(here, '..', 'library', 'song.rio'), encoding='utf-8', newline='').read()
SAID = "(the card's /rinc/lib/song.rio isn't Rinc's own: an older or changed copy, used instead)"

def run(copy):
  shutil.rmtree(work, ignore_errors=True)
  os.makedirs(os.path.join(work, 'apps', 'game'))
  os.makedirs(os.path.join(work, 'lib'))
  with open(os.path.join(work, 'apps', 'game', 'game.rio'), 'w', newline='') as f:
    f.write('import song\nlog("SOUNDS", song.SOUNDS)\n')
  with open(os.path.join(work, 'lib', 'song.rio'), 'w', newline='') as f:
    f.write(copy)
  r = subprocess.run([rinc, '-data', work, '-app', 'game', '-frames', '2'], capture_output=True, text=True, timeout=60)
  return (r.stdout + r.stderr).replace('\r', '').split('\n')

bad = []
lines = run(own)
if SAID in lines or 'SOUNDS 32' not in lines: bad.append("Rinc's own copy: %r" % lines)
lines = run(own.replace('# song:', '# (changed) song:', 1))
if SAID not in lines or 'SOUNDS 32' not in lines: bad.append('a changed copy, not said: %r' % lines)
lines = run(own[:len(own) // 2] + '\nthis isn\'t Rio\n')
if SAID not in lines: bad.append('a copy that stops the compile, not said: %r' % lines)
if bad: sys.exit('stale library copies: ' + '; '.join(bad))
print("a card's library copy that isn't Rinc's own is said")
