# /// script
# dependencies = ["numpy", "scipy"]
# ///
"""uv run tools/boardframe.py COUNTS.txt [FRAME]: what a frame cost on the board, as board speed reckons it
(src/core/boardcost.h's costs), biggest first: from rinc -board -meter COUNTS.txt (each frame's counts,
a "frame" line), the frame numbered FRAME (from 0; default: the last). To see why a frame's slow, or why
board speed's guess is off."""
import os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import boardspeed as b

def parse(line):
  d = {}
  for f in line.split()[1:]:
    k, v = f.split('=')
    d[k] = tuple(int(x) for x in v.split(',')) if ',' in v else int(v)
  return d

def main():
  if len(sys.argv) < 2: sys.exit(__doc__)
  frames = [parse(l) for l in open(sys.argv[1]) if l.startswith('frame')]
  k = int(sys.argv[2]) if len(sys.argv) > 2 else len(frames) - 1
  d = b.diff(frames[k - 1] if k > 0 else {}, frames[k])
  names, table, gl = b.ops(), b.opGroups(), b.groupNames()
  s = open(b.HEADER, encoding='utf-8').read()
  cost = dict((n, float(v)) for n, v in re.findall(r'(\S+?)=([\d.]+)', re.search(r'costs \(ns\): (.*?) \*/', s, re.S).group(1)))
  rows = []
  for n, v in d.items():
    if n.startswith('o'):
      i = int(n[1:]); g = gl[table[i]]
      if v: rows.append((v * cost[g] / 1000, ('native ' if i >= 256 else '') + names.get(i % 256, '?'), str(v), g))
    elif v[0]:
      rows.append((v[0] * cost[n + '.call'] / 1000 + v[1] * cost[n + '.each'] / 1000, n, '%d calls, %d each' % v, ''))
  rows.sort(reverse=True)
  print('frame %d: %.0f us on the board' % (k, sum(r[0] for r in rows)))
  for r in rows[:30]: print('%9.1f us  %-16s %-22s %s' % r)

if __name__ == '__main__': main()
