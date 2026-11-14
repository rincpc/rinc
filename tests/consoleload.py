"""The console's load: an app's source, run in the console's session, its names there after. The app is
big (about 2,500 instructions), so the session's room for code must be the size of its memory, not of
its compiler's tables (which once gave it 1,440). python consoleload.py <rinc> <console's folder> <work folder>"""
import os, shutil, subprocess, sys
rinc, console, work = sys.argv[1], sys.argv[2], sys.argv[3]
shutil.rmtree(work, ignore_errors=True)
subprocess.run([rinc, '-data', work, '-install', console], capture_output=True, timeout=60)
os.makedirs(os.path.join(work, 'apps', 'big'))
lines = ['total := 0']
for i in range(591):
  lines.append('total = (total * 3 + 7) % 1000')
open(os.path.join(work, 'apps', 'big', 'big.rio'), 'w', newline='\n').write('\n'.join(lines) + '\n')
want = 0
for i in range(591):
  want = (want * 3 + 7) % 1000
got = subprocess.run([rinc, '-data', work, '-app', 'console', '-frames', '90', '-type', 'load big\\ntotal\\n'],
                     capture_output=True, text=True, timeout=60).stdout
shutil.rmtree(work, ignore_errors=True)
if 'loaded big' in got and '\n%d' % want in got.replace('\r', ''):
  print('console load: a big app loaded, and its names there (total %d)' % want)
else:
  print('FAIL console load (want %d): %s' % (want, got[-400:]))
  sys.exit(1)
