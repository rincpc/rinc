"""python manifests.py path/to/system path/to/apps: each system app's files.txt (and each demo's, in a
folder of its own) lists every file of its folder (what rinc -install name fetches), and nothing else,
but for its tests, test.rio (not installed)"""
import os, sys

bad = 0
for app, folder in sorted((app, os.path.join(top, app)) for top in sys.argv[1:] for app in os.listdir(top)):
  if not os.path.isdir(folder): continue
  have = sorted(os.path.relpath(os.path.join(d, f), folder).replace(os.sep, '/') for d, _, fs in os.walk(folder) for f in fs if d != folder or f not in ('files.txt', 'test.rio'))
  try: listed = sorted(l.strip() for l in open(os.path.join(folder, 'files.txt')) if l.strip())
  except OSError: listed = None
  if listed != have:
    print('FAIL: %s/files.txt should list:\n%s' % (folder, '\n'.join(have)))
    bad = 1
if not bad: print('every files.txt lists its app')
sys.exit(bad)
