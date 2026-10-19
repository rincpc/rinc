"""python cache.py path/to/rinc folder: an app's compiled image is cached (docs/spec/apps.md#compiled-apps),
used while it's still its source's (an image changed by hand shows it's what ran), and made again when
the source changes, or when it's another build's."""
import os, shutil, subprocess, sys

rinc, data = os.path.abspath(sys.argv[1]), sys.argv[2]
app, img = os.path.join(data, 'apps', 'cached', 'cached.rio'), os.path.join(data, 'cache', 'rinc', 'apps', 'cached.img')

def source(text):
  os.makedirs(os.path.dirname(app), exist_ok=True)
  with open(app, 'w') as f: f.write(text)
def image(change):
  with open(img, 'rb') as f: b = f.read()
  with open(img, 'wb') as f: f.write(change(b))
def run(want, why):
  out = subprocess.run([rinc, '-data', data, '-app', 'cached', '-frames', '1'], capture_output=True, text=True, timeout=30).stdout
  if not out.startswith(want): sys.exit('FAIL: %s: got %r, wanted %r' % (why, out, want))

shutil.rmtree(data, ignore_errors=True)
source('log("one")\n')
run('one', 'compiled')
if not os.path.exists(img): sys.exit('FAIL: no image cached')
image(lambda b: b.replace(b'one', b'uno'))
run('uno', 'the cached image')
source('log("two")\n')
run('two', 'a changed source, compiled again')
run('two', 'its image')
image(lambda b: b.replace(b'two', b'dos')[:8] + b'another build'.ljust(32, b'\0') + b.replace(b'two', b'dos')[40:])
run('two', "another build's image, compiled again")
image(lambda b: b[:len(b) // 2])
run('two', 'a cut-short image, compiled again')
print('the cache came through')
