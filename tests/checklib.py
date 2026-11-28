"""python checklib.py path/to/rinc work-folder: Edit's check (pc.checkRio, which completion and error marks
rely on) of an app importing a library as big as tilemap, in the room Edit lends (94,208 bytes): the
library's declarations are taken (its procs' bodies skipped), so the check fits, completion after a
library value offers its fields and methods, and an error in the app's own file is marked at its line;
and a part, cut where it's edited, ends the check (the main file after it uses the part's names past the
cut)."""
import os, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), sys.argv[2]

GAME = '''import tilemap as tm

level: tm.Map
count := 3
_ = level.load("map.map")
include "parts/late.rio"
log(early + later)
'''
LATE = 'early :: 1\nlater :: 2\n'

CHECKER = '''room: Blob[94208]
out: Blob[..8192]
ep: Blob[..200]
em: Blob[..200]
game := "/apps/game/game.rio"
code :: "import tilemap as tm\\n\\nlevel: tm.Map\\ncount := 3\\n"
r := pc.checkRio(game, game, code, room[])
e := pc.checkError(ep, em)
log("CUT", r, e.line, em.asString())
n := pc.completeRio("_ = level.", out)
_ = pc.fileWrite("/save/items.txt", out.asString())
bad :: "import tilemap as tm\\n\\nlevel: tm.Map\\ncount: Int = \\"three\\"\\n"
r = pc.checkRio(game, game, bad, room[])
e = pc.checkError(ep, em)
log("BAD", r, e.line, em.asString())
part := "/apps/game/parts/late.rio"
r = pc.checkRio(game, part, "early :: 1\\n", room[])
ep.clear()
em.clear()
e = pc.checkError(ep, em)
log("PART", r, e.line, em.asString())
'''

shutil.rmtree(work, ignore_errors=True)
for app, text in (('game', GAME), ('checker', CHECKER)):
  os.makedirs(os.path.join(work, 'apps', app))
  with open(os.path.join(work, 'apps', app, app + '.rio'), 'w', newline='') as f:
    f.write(text)
os.makedirs(os.path.join(work, 'apps', 'game', 'parts'))
with open(os.path.join(work, 'apps', 'game', 'parts', 'late.rio'), 'w', newline='') as f:
  f.write(LATE)
r = subprocess.run([rinc, '-data', work, '-app', 'checker', '-frames', '2'], capture_output=True, text=True, timeout=60)
out = (r.stdout + r.stderr).replace('\r', '')
lines = out.split('\n')
bad = []
cut = [l for l in lines if l.startswith('CUT ')]
if cut != ['CUT 0 0 ']: bad.append('the check, cut before the line: %r' % cut)
with open(os.path.join(work, 'save', 'items.txt'), encoding='utf-8') as f:
  items = [l.split('\t')[0] for l in f.read().split('\n')]
for want in ('layers', 'sheets', 'load', 'draw'):
  if want not in items: bad.append('%s not offered after level.' % want)
badLine = [l for l in lines if l.startswith('BAD ')]
if len(badLine) != 1 or not badLine[0].startswith('BAD -1 4 ') or 'mismatch' not in badLine[0]:
  bad.append("an error in the app's own file: %r" % badLine)
part = [l for l in lines if l.startswith('PART ')]
if part != ['PART 0 0 ']: bad.append("a part checked, cut where it's edited (the main file after it uses its names past the cut): %r" % part)
if bad: sys.exit('a check of an app importing tilemap: ' + '; '.join(bad) + '\n' + out)
print("a check of an app importing tilemap fits, completes and marks errors")
