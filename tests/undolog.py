"""python undolog.py path/to/rinc path/to/library data-folder: library/undolog.rio, as an app imports it from
/rinc/lib: tests/undolog.rio's asserts (its log kept in the app's stash, on the card)."""
import os, shutil, subprocess, sys

rinc, library, data = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2]), sys.argv[3]
here = os.path.dirname(os.path.abspath(__file__))
shutil.rmtree(data, ignore_errors=True)
os.makedirs(os.path.join(data, 'lib'))
shutil.copy(os.path.join(library, 'undolog.rio'), os.path.join(data, 'lib'))
os.makedirs(os.path.join(data, 'apps', 'undologtest'))
shutil.copy(os.path.join(here, 'undolog.rio'), os.path.join(data, 'apps', 'undologtest', 'undologtest.rio'))
r = subprocess.run([rinc, '-data', data, '-app', 'undologtest', '-frames', '1'], capture_output=True, text=True, timeout=60)
said = (r.stdout + r.stderr).strip()
if 'undolog came through' not in said or 'assert' in said or 'error' in said:
  sys.exit('FAIL: %s' % said)
print('undolog came through')
