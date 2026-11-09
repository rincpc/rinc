"""python libwins.py path/to/rinc data-folder: the library built in (library/*.rio), imported with nothing
on the card; then a card's own /rinc/lib/tilemap.rio, which wins."""
import os, shutil, subprocess, sys

rinc, data = os.path.abspath(sys.argv[1]), sys.argv[2]
here = os.path.dirname(os.path.abspath(__file__))
shutil.rmtree(data, ignore_errors=True)
os.makedirs(data)
r = subprocess.run([rinc, os.path.join(here, 'undolog.rio'), '-data', data, '-frames', '1'], capture_output=True, text=True, timeout=60)
said = (r.stdout + r.stderr).strip()
if 'undolog came through' not in said:
  sys.exit('FAIL: the library built in, with nothing on the card: %s' % said)
os.makedirs(os.path.join(data, 'lib'))
with open(os.path.join(data, 'lib', 'tilemap.rio'), 'w') as f: f.write('WINS* :: 7\n')
r = subprocess.run([rinc, os.path.join(here, 'libwins.rio'), '-data', data, '-frames', '1'], capture_output=True, text=True, timeout=60)
said = (r.stdout + r.stderr).strip()
if "the card's own: 7" not in said:
  sys.exit("FAIL: a card's own library file: %s" % said)
print('the library built in, and the card\'s own first')
