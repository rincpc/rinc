"""What pc.synth plays, as mixed (rinc -wav): tests/synth.rio ends by looping a note, A4 on the square
wave, so two seconds in it's still heard, at 440 Hz, at the square's level. (And its frame-by-frame tests,
of seeking and stopping, which need the sound mixed, pass.)

  python tests/synth.py path/to/rinc out-folder"""
import os, struct, subprocess, sys, wave

rinc, folder = sys.argv[1], sys.argv[2]
os.makedirs(folder, exist_ok=True)
out = os.path.join(folder, 'synth.wav')
here = os.path.dirname(os.path.abspath(__file__))
r = subprocess.run([rinc, os.path.join(here, 'synth.rio'), '-frames', '150', '-wav', out], capture_output=True, text=True, timeout=60)
if r.returncode or 'synth tests passed' not in r.stdout or 'seek tests passed' not in r.stdout: sys.exit('synth.rio: ' + r.stdout + r.stderr)
with wave.open(out) as w:
  rate, channels = w.getframerate(), w.getnchannels()
  w.setpos(rate * 2)
  raw = w.readframes(rate // 2)
left = struct.unpack('<%dh' % (len(raw) // 2), raw)[::channels]
loud = max(abs(x) for x in left)
crossings = sum((a >= 0) != (b >= 0) for a, b in zip(left, left[1:]))
hz = crossings / 2 / (len(left) / rate)
print(f'from 2 s: loudest {loud}, {hz:.1f} Hz')
if not 4400 < loud <= 5120: sys.exit('not at the square\'s level')
if not 435 < hz < 445: sys.exit('not 440 Hz')
print('the synth came through')
