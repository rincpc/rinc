"""pc.screen (tests/screenmode.rio): in half mode, a 10x10 rect at 0, 0 is 20x20 on the screen; one past the
edge clipped there; a pixel at 159, 0 is the top right 2x2; and the sizes in each mode logged.
python screenmode.py <rinc> <the .rio> <work folder>"""
import os, shutil, subprocess, sys
rinc, app, work = sys.argv[1], sys.argv[2], sys.argv[3]
shutil.rmtree(work, ignore_errors=True)
os.makedirs(work)
shot = os.path.join(work, 'shot.ppm')
out = subprocess.run([rinc, app, '-frames', '2', '-o', shot], capture_output=True, text=True, timeout=60).stdout
px = open(shot, 'rb').read()[-320 * 240 * 3:]
def at(x, y): return px[(y * 320 + x) * 3:(y * 320 + x) * 3 + 3]
back, red, green, blue = at(100, 100), at(0, 0), at(318, 238), at(318, 0)
ok = (at(19, 19) == red and at(20, 19) == back and at(19, 20) == back and at(299, 219) == back and at(300, 220) == green
      and at(319, 239) == green and blue != back and at(319, 1) == blue and at(317, 0) == back and len({red, green, blue, back}) == 4
      and 'full [320, 240]' in out and 'quarter [80, 60]' in out and 'half [160, 120]' in out)
shutil.rmtree(work, ignore_errors=True)
print('screen modes came through' if ok else 'FAIL screen modes: %s' % out[-300:])
sys.exit(0 if ok else 1)
