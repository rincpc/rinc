"""python exported.py path/to/rinc work-folder [web]: Cloudlamp (apps/cloudlamp: a map, a sheet, a song and its
sounds beside it, read with rinc's library) exported as a program and as a page, each run as it's handed
out: the program draws its island (the sheet's colors, not an empty screen) and plays its song (heard,
all through: it goes round); the page, run in node (webplay.mjs), draws just what the program does. Without
node, or web (a rinc with no web player), the page is skipped (the program's checked all the same)."""
import os, re, shutil, struct, subprocess, sys, wave

rinc, work, web = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2]), sys.argv[3:] == ['web']
here = os.path.dirname(os.path.abspath(__file__))
app = os.path.join(here, '..', 'apps', 'cloudlamp') + '/'
shutil.rmtree(work, ignore_errors=True)
os.makedirs(work)
def fail(why): sys.exit('exported Cloudlamp: ' + why)
def screen(path):
  b = open(path, 'rb').read()
  m = re.match(rb'P6\s+(\d+)\s+(\d+)\s+\d+\s', b)
  return b[m.end():]

game = os.path.join(work, 'cloudlamp' + ('.exe' if os.name == 'nt' else ''))
r = subprocess.run([rinc, app, '-export', game], capture_output=True, text=True, timeout=120)
if r.returncode or not os.path.exists(game): fail('the export: ' + r.stdout + r.stderr)
shot, wav = os.path.join(work, 'game.ppm'), os.path.join(work, 'game.wav')
r = subprocess.run([game, '-frames', '300', '-o', shot, '-wav', wav, '-data', os.path.join(work, 'data')], capture_output=True, text=True, timeout=120)
if r.returncode or (r.stdout + r.stderr).strip(): fail('the program ran: ' + r.stdout + r.stderr)
pixels = screen(shot)
colors = {pixels[i:i + 3] for i in range(0, len(pixels), 3)}
if len(colors) < 5: fail('the program drew its island: %d colors on its screen' % len(colors))
with wave.open(wav) as f:
  rate, n = f.getframerate(), f.getnchannels()
  samples = struct.unpack('<%dh' % (f.getnframes() * n), f.readframes(f.getnframes()))
quiet = [k for k in range(0, len(samples) - rate * n // 2, rate * n // 2) if max(abs(v) for v in samples[k:k + rate * n // 2]) < 1000]
if quiet: fail('the program played its song all through: quiet at %s s' % [k / (rate * n) for k in quiet])

node = shutil.which('node')
if not node or not web:
  print('exported Cloudlamp: the program came through (%s: the page skipped)' % ('no web player' if not web else 'no node'))
  sys.exit(0)
page, webShot = os.path.join(work, 'cloudlamp.html'), os.path.join(work, 'page.ppm')
r = subprocess.run([rinc, app, '-web', page], capture_output=True, text=True, timeout=120)
if r.returncode: fail('the page: ' + r.stdout + r.stderr)
r = subprocess.run([node, os.path.join(here, 'webplay.mjs'), page, '300', webShot], capture_output=True, text=True, timeout=120)
if r.returncode or 'FAIL' in r.stdout or 'error' in r.stdout: fail('the page ran: ' + r.stdout + r.stderr)
if screen(webShot) != pixels: fail('the page drew just what the program did')
print('exported Cloudlamp came through, as a program and as a page')
