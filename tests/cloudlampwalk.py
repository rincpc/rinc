"""Cloudlamp's walls, from its sheet's flags (Paint's "solid", on the rock and the cloud): walking left from
the start (cell 3, 7, on the path), the keeper stops at cell 2, beside the clouds at 1 and 0, not on them.
The screen shows where: cell 2 unlike the path's next cell (5, 7, the keeper's not there), cell 1 like
cell 0 (both just cloud). python cloudlampwalk.py <rinc> <cloudlamp's folder> <work folder>"""
import os, shutil, subprocess, sys
rinc, folder, work = sys.argv[1], sys.argv[2], sys.argv[3]
shutil.rmtree(work, ignore_errors=True)
os.makedirs(work)
shot = os.path.join(work, 'shot.ppm')
subprocess.run([rinc, folder, '-data', os.path.join(work, 'data'), '-frames', '200', '-hold', 'left 5-180', '-o', shot],
               capture_output=True, timeout=120)
data = open(shot, 'rb').read()
pixels = data[-320 * 240 * 3:]
def cell(x, y): # (a cell's pixels: 16 by 16 on the screen)
  return b''.join(pixels[((y * 16 + r) * 320 + x * 16) * 3:((y * 16 + r) * 320 + x * 16 + 16) * 3] for r in range(16))
here, path, cloud, cloud2 = cell(2, 7), cell(5, 7), cell(1, 7), cell(0, 7)
shutil.rmtree(work, ignore_errors=True)
if here != path and cloud == cloud2:
  print('cloudlamp walk: stopped by the clouds (solid, by its sheet\'s flags)')
else:
  print('FAIL cloudlamp walk: keeper at 2? %s; clouds alike? %s' % (here != path, cloud == cloud2))
  sys.exit(1)
