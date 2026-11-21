# /// script
# dependencies = ["pyserial", "numpy", "scipy"]
# ///
r"""Board speed's costs (plans/board-speed.md), measured: uv run tools/boardspeed.py COM3 RINC_EXE
apps/speed.rio runs on the board (restarted first, so pc.appTime is precise: picotool reboot), with the JIT
and then without; and on the desktop, rinc -meter, which says what each of its tests ran (each kind of
instruction, apart in code the board's JIT compiles, and each call with its pixels or bytes). Each cost
is fitted to the board's times (non-negative least squares, relative), and written to
src/core/boardcost.h; each test's time on the board and as predicted is shown, and how the costs
changed since the header before.
  --board FILE      the board's results already fetched (jit on), skip the board (with --nojit FILE)
  --keep DIR        where the runs' files go (default: ../tmp/boardspeed)
  --no-reboot       don't restart the board first
  --check           only show how well boardcost.h as it is predicts the board (nothing written)
On Git Bash, set MSYS_NO_PATHCONV=1."""
import os, re, subprocess, sys, time
import numpy as np
from scipy.optimize import nnls

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
RIO = os.environ.get('RIO_DIR', os.path.join(os.path.dirname(ROOT), 'rio'))
HEADER = os.path.join(ROOT, 'src', 'core', 'boardcost.h')

# ---------------------------------------------------------------- the ops, from rio's internal.h
def ops():
  src = open(os.path.join(RIO, 'src', 'internal.h'), encoding='utf-8').read()
  def body(name):
    m = re.search(r'#define ' + name + r'\(X\)(.*?)(?=\n#define|\n\n|\ntypedef|\nenum)', src, re.S)
    return m.group(1)
  wide = re.findall(r'X\((\w+)\)', body('rio_OPS'))
  short = [(n, n) for n in re.findall(r'X\((\w+),\s*"', body('rio_SHORT_OPS'))]
  short += [(n, w) for n, w in re.findall(r'X\((\w+),\s*(\w+),\s*"', body('rio_SHORT_MORE'))]
  names = {i: w for i, w in enumerate(wide)}
  for k, (n, w) in enumerate(short): names[128 + k] = w # (a short op costs as its wide op's group: dispatch alike)
  return names

# each op's group: by its wide op's name
GROUPS = {
  'move': 'mov mov2',
  'movn': 'movn zero copy',
  'int': 'add sub and or xor shl shr ushr neg bnot not eq ne lt le iabs imin imax',
  'mul': 'mul',
  'div': 'div mod',
  'float': 'fadd fsub fmul fneg feq fne flt fle fmin fmax itof ftoi',
  'fdiv': 'fdiv',
  'math': 'm1 m2',
  'load': 'ldw ldb lea ldw2',
  'store': 'stw stb stw2',
  'index': 'ldx ldxb stx stxb idx idxv lidx bchk',
  'vector': 'vadd vsub vmul vdiv vmod fvadd fvsub fvmul fvdiv',
  'jump': 'jmp jz jnz jeq jne jlt jle jfeq jfne jflt jfle jfnlt jfnle',
  'loop': 'fori forby each',
  'call': 'call ret getret',
  'calli': 'calli',
  'ffi': 'ffi',
  'list': 'pusha pushs pusht pushv pushf popa lrem lswap lview bpack geti seti pnum',
  'bytes': 'copyv swapv slice enumck enumname',
  'strcmp': 'seq sne scmp',
  'log': 'logi logf logs logb logn logv loge',
  'other': 'fail halt jit',
}
NATIVE = { # (in code the JIT compiles: fewer kinds)
  'n_alu': 'move int mul load store vector',
  'n_div': 'div',
  'n_float': 'float',
  'n_fdiv': 'fdiv',
  'n_math': 'math',
  'n_mem': 'index',
  'n_jump': 'jump',
  'n_loop': 'loop',
  'n_call': 'call calli',
  'n_ffi': 'ffi',
  'n_other': 'movn list bytes strcmp log other',
}
KINDS = re.findall(r'X\((\w+)\)', re.search(r'#define rinc_COST_KINDS\(X\)(.*?)\n#define', open(os.path.join(ROOT, 'src', 'core', 'rinc.h'), encoding='utf-8').read(), re.S).group(1))

def groupNames():
  return list(GROUPS) + list(NATIVE)
def opGroups():
  names, of = ops(), {}
  byWide = {w: g for g, ws in GROUPS.items() for w in ws.split()}
  nat = {g: n for n, gs in NATIVE.items() for g in gs.split()}
  gl = groupNames()
  table = [gl.index('other')] * 512
  for i, w in names.items():
    g = byWide.get(w, 'other')
    table[i] = gl.index(g)
    table[256 + i] = gl.index(nat[g])
  return table

# ---------------------------------------------------------------- runs
def board(port, keep, reboot):
  env = dict(os.environ, MSYS_NO_PATHCONV='1')
  def uv(*a): return subprocess.run(['uv', 'run', os.path.join(HERE, a[0])] + list(a[1:]), env=env, capture_output=True, text=True, cwd=ROOT)
  if reboot:
    subprocess.run([os.environ.get('PICOTOOL', 'C:/Users/Agent/.pico-sdk/picotool/2.3.1/picotool/picotool.exe' if os.name == 'nt' else 'picotool'), 'reboot', '-f'], capture_output=True)
    time.sleep(6)
  empty = os.path.join(keep, 'empty.txt'); open(empty, 'w').close()
  r = uv('boardput.py', port, 'apps/speed/speed.rio=apps/speed.rio')
  print(r.stdout.strip())
  got = {}
  for jit in ('on', 'off'):
    uv('boardput.py', port, 'save/speed/speed.txt=' + empty)
    uv('boardplay.py', port, 'fw:jit ' + jit, 'con:run speed', 'wait 25')
    out = os.path.join(keep, 'board-jit-%s.txt' % jit)
    uv('boardget.py', port, 'save/speed/speed.txt=' + out)
    got[jit] = out
    print('board, jit', jit + ':', len(results(out)), 'tests')
  uv('boardplay.py', port, 'fw:jit on')
  return got['on'], got['off']

def results(path):
  rows = []
  for line in open(path, encoding='utf-8', errors='replace'):
    p = line.split()
    if len(p) == 2 and p[1].lstrip('-').isdigit(): rows.append((p[0], int(p[1])))
  return rows

def desktop(exe, keep, jit):
  data = os.path.join(keep, 'data-' + ('jit' if jit else 'nojit'))
  os.makedirs(data, exist_ok=True)
  meter = os.path.join(keep, 'meter-%s.txt' % ('jit' if jit else 'nojit'))
  cmd = [exe, os.path.join(ROOT, 'apps', 'speed.rio'), '-data', data, '-frames', '1', '-meter', meter] + ([] if jit else ['-nojit'])
  r = subprocess.run(cmd, capture_output=True, text=True)
  if r.returncode: sys.exit('rinc: ' + r.stdout + r.stderr)
  snaps = []
  for line in open(meter):
    if not line.startswith('time'): continue
    d = {}
    for f in line.split()[1:]:
      k, v = f.split('=')
      d[k] = tuple(int(x) for x in v.split(',')) if ',' in v else int(v)
    snaps.append(d)
  if len(snaps) % 2: sys.exit('rinc -meter: an odd count of pc.appTime calls')
  return [diff(snaps[i], snaps[i + 1]) for i in range(0, len(snaps), 2)]

def diff(a, b):
  out = {}
  for k, v in b.items():
    w = a.get(k, (0, 0) if isinstance(v, tuple) else 0)
    out[k] = tuple(x - y for x, y in zip(v, w)) if isinstance(v, tuple) else v - w
  return out

# ---------------------------------------------------------------- the fit
def features(counts, table):
  gl = groupNames()
  x = np.zeros(len(gl) + 2 * len(KINDS))
  for k, v in counts.items():
    if k.startswith('o'): x[table[int(k[1:])]] += v
    elif k in KINDS:
      j = len(gl) + 2 * KINDS.index(k)
      x[j] += v[0]; x[j + 1] += v[1]
  return x

def names():
  return groupNames() + [k + suffix for k in KINDS for suffix in (' call', ' each')]

def fit(rows):
  A = np.array([f for f, t in rows]); b = np.array([t for f, t in rows], float) * 1000.0 # (ns)
  w = 1.0 / np.maximum(b, 1.0)
  used = A.sum(axis=0) > 0
  x = np.zeros(A.shape[1])
  sol, _ = nnls(A[:, used] * w[:, None], b * w)
  x[used] = sol
  return x, used

def readHeader():
  if not os.path.exists(HEADER): return None
  s = open(HEADER, encoding='utf-8').read()
  m = re.search(r'/\* costs \(ns\): (.*?) \*/', s, re.S)
  return dict((k, float(v)) for k, v in re.findall(r'(\S+?)=([\d.]+)', m.group(1))) if m else None

def costsOfHeader():
  old = readHeader()
  if not old: return None
  return np.array([old.get(n.replace(' ', '.'), 0.0) for n in names()])

def writeHeader(x, used, when):
  gl, table = groupNames(), opGroups()
  ng = len(gl)
  lines = ['/* boardcost.h: what each thing costs on the Fruit Jam, for board speed (rinc.c, plans/board-speed.md).',
           '   Made by tools/boardspeed.py from the board\'s own measurements (' + when + '): not edited by hand.',
           '   An op\'s group (rinc_opGroup, by op: +256 in code the board\'s JIT compiles) costs rinc_groupNs; a call',
           '   of each kind (rinc_COST_KINDS) costs rinc_kindNs: once, and for each pixel or byte. */',
           '/* costs (ns): ' + ' '.join('%s=%.3f' % (n.replace(' ', '.'), x[i]) for i, n in enumerate(names())) + ' */',
           '#define rinc_BOARD_MEASURED "' + when + '"',
           'static const uint8_t rinc_opGroup[512] = {']
  for i in range(0, 512, 32): lines.append('  ' + ', '.join(str(v) for v in table[i:i + 32]) + ',')
  lines.append('};')
  lines.append('static const float rinc_groupNs[%d] = {' % ng)
  for i, g in enumerate(gl): lines.append('  %.3ff, /* %s */' % (x[i], g))
  lines.append('};')
  lines.append('static const float rinc_kindNs[rinc_Cost_count][2] = {')
  for k, kind in enumerate(KINDS): lines.append('  [rinc_Cost_%s] = {%.3ff, %.4ff},' % (kind, x[ng + 2 * k], x[ng + 2 * k + 1]))
  lines.append('};')
  open(HEADER, 'w', encoding='utf-8', newline='\n').write('\n'.join(lines) + '\n')

def main():
  args = sys.argv[1:]
  def opt(name, default=None):
    if name in args:
      i = args.index(name); v = args[i + 1]; del args[i:i + 2]; return v
    return default
  def flag(name):
    if name in args: args.remove(name); return True
    return False
  keep = opt('--keep', os.path.join(os.path.dirname(ROOT), 'tmp', 'boardspeed'))
  boardOn, boardOff = opt('--board'), opt('--nojit')
  reboot, check = not flag('--no-reboot'), flag('--check')
  if len(args) != 2: sys.exit(__doc__)
  port, exe = args[0], os.path.abspath(args[1])
  os.makedirs(keep, exist_ok=True)
  if not boardOn: boardOn, boardOff = board(port, keep, reboot)
  table = opGroups()
  rows, labels = [], []
  for path, jit in ((boardOn, True), (boardOff, False)):
    if not path: continue
    times, counts = results(path), desktop(exe, keep, jit)
    if len(times) != len(counts): sys.exit('%s: %d tests, the desktop %d' % (path, len(times), len(counts)))
    for (name, us), c in zip(times, counts):
      rows.append((features(c, table), us)); labels.append(name + ('' if jit else ' (no jit)'))
  old = costsOfHeader()
  if check:
    if old is None: sys.exit('no costs in ' + HEADER)
    x, used = old, np.ones(len(old), bool)
  else: x, used = fit(rows)
  worst = 0
  print('%-24s %10s %10s %7s' % ('test', 'board us', 'predicted', 'off'))
  for (f, us), name in zip(rows, labels):
    p = float(f @ x) / 1000.0
    off = (p - us) / max(us, 1) * 100
    worst = max(worst, abs(off))
    print('%-24s %10d %10.0f %6.0f%%' % (name, us, p, off))
  print('worst: %.0f%% off' % worst)
  if check: return
  print('\n%-18s %10s %10s' % ('cost (ns)', 'now', 'before'))
  for i, n in enumerate(names()):
    if used[i] or (old is not None and old[i]): print('%-18s %10.3f %10s' % (n, x[i], '%.3f' % old[i] if old is not None else '-'))
  writeHeader(x, used, time.strftime('%Y-%m-%d'))
  print('\nwrote', HEADER)

if __name__ == '__main__': main()
