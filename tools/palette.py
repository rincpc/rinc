"""Rinc's palette (docs/spec/screen.md#the-palette), made: writes it as the C table src/core/palette.h, and
a preview PNG, 16 a row:

- PICO-8's 32, kept as 0 to 31.
- 12 hues, a row each (32 to 223), in OKLCH: sRGB's red, yellow, green, cyan, blue and magenta, where OKLCH
  puts them, but a warmer yellow (--yellow; sRGB's reads lemon), and the hue halfway between each two (or
  --hue-list, degrees). Each has 8 vivid steps, dark to light, then 8 muted. The vivid steps are bent so one
  is where the hue is most vivid (yellow's high, blue's low), at 95% of the most chroma sRGB has there,
  smoothed so no step pops past its neighbours (--smooth); a hue most vivid when light (orange to azure) has
  its lightest step paler (--pale, --pale-chroma, --pale-cap). The muted steps are the same modest chroma
  (--muted) at the same lightnesses for every hue. Each is the 16-bit color nearest it, its hue weighed
  double: a Rinc shows 16 bits a pixel, and dropping the low bits moves dark and pale colors' hues.
- 16 tints (224 to 239): 8 warm grays, 8 cool, dark to light.
- 16 grays (240 to 255): pure (R = G = B), 000000 to FFFFFF, even in lightness as it looks (OKLab's L with
  its toe set right near black: Ottosson's Lr, as CIELAB's L* is).

The preview has each hue's row, then the tints and grays, beside the same of earlier tables (--before, a
palette.h this wrote; newest first), then PICO-8's 32. It also says how near the palette's nearest pairs are
(OKLab distance, as shown at 16 bits), and how many pairs are nearly the same color (--twins lists them).
The table says how it was made. With no options, it makes the palette Rinc has.

  python tools/palette.py [preview.png] [table.h] [--yellow 100] [--hue-list 29,65,...] [--steps 8] [--muted 0.07]
                          [--muted-low 0.36] [--muted-high 0.86] [--tints 16] [--warm 60] [--cool 250]
                          [--tint-chroma 0.025] [--low 0.28] [--high 0.88] [--pale 0.91] [--pale-chroma 0.78]
                          [--pale-cap 0.11] [--smooth 1.1] [--before earlier.h ...] [--twins]
"""
import math, struct, sys, zlib

pico = [0x000000, 0x1D2B53, 0x7E2553, 0x008751, 0xAB5236, 0x5F574F, 0xC2C3C7, 0xFFF1E8,
        0xFF004D, 0xFFA300, 0xFFEC27, 0x00E436, 0x29ADFF, 0x83769C, 0xFF77A8, 0xFFCCAA,
        0x291814, 0x111D35, 0x422136, 0x125359, 0x742F29, 0x49333B, 0xA28879, 0xF3EF7D,
        0xBE1250, 0xFF6C24, 0xA8E72E, 0x00B543, 0x065AB5, 0x754665, 0xFF6E59, 0xFF9D81]

def opt(name, default):
  if name in sys.argv: return type(default)(sys.argv[sys.argv.index(name) + 1])
  return default
# red, orange, yellow, lime, green, teal, cyan, azure, blue, violet, magenta, crimson (pink: PICO-8's 14 and 30)
HUE_LIST = [float(h) for h in opt('--hue-list', '').split(',')] if '--hue-list' in sys.argv else None # (None: sRGB's six and the halves, below)
PALE, PALE_C = opt('--pale', 0.91), opt('--pale-chroma', 0.78) # the lightest step of a hue most vivid when light (orange to azure): lighter, and that much of its chroma
PALE_CAP = opt('--pale-cap', 0.11) # (and no more chroma than this: yellow and lime have the most there)
YELLOW = opt('--yellow', 100.0) # (yellow's hue: sRGB's, 110, reads lemon; the halves on either side follow it)
HUES = len(HUE_LIST) if HUE_LIST else 12
STEPS, MUTED, TINTS = opt('--steps', 8), opt('--muted', 0.07), opt('--tints', 16) # (muted: its chroma, the same for every hue)
WARM, COOL, TINT_C = opt('--warm', 60.0), opt('--cool', 250.0), opt('--tint-chroma', 0.025) # the tints' hues, and their chroma
M_LOW, M_HIGH = opt('--muted-low', 0.36), opt('--muted-high', 0.86) # the muted ramps' darkest and lightest
L_LOW, L_HIGH = opt('--low', 0.28), opt('--high', 0.88) # the ramps' darkest and lightest (black and near-black: PICO-8's and the grays')
VIVID = 0.95             # of the most chroma sRGB has, at that lightness and hue
SMOOTH = opt('--smooth', 1.1) # a vivid step's chroma, at most this times its neighbours' mean (0: as much as sRGB has)
GRAYS = 256 - 32 - HUES * 2 * STEPS - TINTS

# ---------------------------------------------------------------- color spaces (Björn Ottosson's OKLab)

def lin(c): return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
def unlin(c): return 12.92 * c if c <= 0.0031308 else 1.055 * c ** (1 / 2.4) - 0.055
def to_oklab(rgb):
  r, g, b = (lin((rgb >> s & 255) / 255) for s in (16, 8, 0))
  l = (0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b) ** (1 / 3)
  m = (0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b) ** (1 / 3)
  s = (0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b) ** (1 / 3)
  return (0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s, 1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s,
          0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s)
def oklab_to_linear(L, a, b):
  l = (L + 0.3963377774 * a + 0.2158037573 * b) ** 3
  m = (L - 0.1055613458 * a - 0.0638541728 * b) ** 3
  s = (L - 0.0894841775 * a - 1.2914855480 * b) ** 3
  return (4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s, -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
          -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s)
def lch_rgb(L, C, h):  # sRGB, 0xRRGGBB, and whether it was in gamut
  a, b = C * math.cos(math.radians(h)), C * math.sin(math.radians(h))
  lr = oklab_to_linear(L, a, b)
  inside = all(-1e-6 <= c <= 1 + 1e-6 for c in lr)
  v = [round(255 * unlin(min(1, max(0, c)))) for c in lr]
  return v[0] << 16 | v[1] << 8 | v[2], inside
def max_chroma(L, h):  # the most chroma sRGB has at this lightness and hue
  lo, hi = 0.0, 0.4
  for _ in range(30):
    mid = (lo + hi) / 2
    if lch_rgb(L, mid, h)[1]: lo = mid
    else: hi = mid
  return lo
def as565(rgb):  # as a Rinc shows it: 16 bits a pixel
  r, g, b = rgb >> 19 & 31, rgb >> 10 & 63, rgb >> 3 & 31
  return (r * 255 // 31) << 16 | (g * 255 // 63) << 8 | b * 255 // 31

# ---------------------------------------------------------------- the palettes

def hue(rgb):  # its OKLCH hue, in degrees
  _, a, b = to_oklab(rgb)
  return math.degrees(math.atan2(b, a)) % 360
if not HUE_LIST:  # sRGB's red, yellow, green, cyan, blue and magenta, and halfway (in OKLCH) after each
  six = [hue(c) for c in (0xFF0000, 0xFFFF00, 0x00FF00, 0x00FFFF, 0x0000FF, 0xFF00FF)]
  six[1] = YELLOW
  HUE_LIST = [h for k, a in enumerate(six) for h in (a, (a + (six[(k + 1) % 6] - a) % 360 / 2) % 360)]

def cusp(h):  # the lightness where hue h is most vivid
  return max((L / 100 for L in range(20, 99)), key=lambda L: max_chroma(L, h))

def bent(h, low, high):  # STEPS lightnesses, low to high, one of them at h's cusp (the rest even on either side)
  even = [low + (high - low) * i / (STEPS - 1) for i in range(STEPS)]
  Lc = min(max(cusp(h), low + 0.08), high - 0.03)
  c = min(range(1, STEPS - 1), key=lambda i: abs(even[i] - Lc))
  return [low + (Lc - low) * i / c if i <= c else Lc + (high - Lc) * (i - c) / (STEPS - 1 - c) for i in range(STEPS)]

def palette():
  colors = list(pico)
  for h in HUE_LIST:
    inner = [M_LOW + (M_HIGH - M_LOW) * i / (STEPS - 1) for i in range(STEPS)] # (the muted ramps': inside the vivid's, where there's chroma for both)
    light = cusp(h) > 0.75 # (orange to azure: its lightest step paler, toward white)
    steps = bent(h, L_LOW, PALE if light else L_HIGH)
    for vivid in (True, False): # (vivid: as much as sRGB has there; muted: MUTED, or 60% of that, if less)
      chroma_of = (lambda L: max_chroma(L, h) * VIVID) if vivid else (lambda L: min(max_chroma(L, h) * VIVID * 0.6, MUTED))
      Ls = steps if vivid else inner
      Cs = [chroma_of(L) for L in Ls]
      if vivid and light: Cs[-1] = min(Cs[-1] * PALE_C, PALE_CAP)
      if vivid and SMOOTH: # (no step much more vivid than its neighbours: sRGB's most is sharpest at a hue's cusp, and that step pops)
        Cs = [c if i in (0, STEPS - 1) else min(c, (Cs[i - 1] + Cs[i + 1]) / 2 * SMOOTH) for i, c in enumerate(Cs)]
      for L, C in zip(Ls, Cs):
        colors.append(lch_rgb(L, C, h)[0])
  half = TINTS // 2
  for hue, n in ((WARM, half), (COOL, TINTS - half)): # (warm: toward orange; cool: toward blue)
    for i in range(n):
      L = 0.18 + 0.74 * i / max(1, n - 1)
      colors.append(lch_rgb(L, TINT_C, hue)[0])
  for i in range(GRAYS):
    colors.append(gray(toe_inverse(i / (GRAYS - 1))))
  assert len(colors) == 256, len(colors)
  return colors[:32] + [snap(c) for c in colors[32:256 - GRAYS]] + colors[256 - GRAYS:]

# grays even in lightness as it looks: OKLab's L, its toe set right near black (Ottosson's Lr, from Okhsv), as
# CIELAB's L* is; OKLab's own L puts its first step past black at RGB 1
K1, K2 = 0.206, 0.03
K3 = (1 + K1) / (1 + K2)
def toe(L): return (K3 * L - K1 + math.sqrt((K3 * L - K1) ** 2 + 4 * K2 * K3 * L)) / 2
def toe_inverse(Lr): return (Lr * Lr + K1 * Lr) / (K3 * (Lr + K2))

def gray(L):  # a pure gray (R = G = B) whose look at 16 bits is nearest lightness L (by Lr), the least tinted there of those as near
  def off(u):
    shown = as565(u * 0x010101)
    parts = [shown >> 16, shown >> 8 & 255, shown & 255]
    return abs(toe(to_oklab(shown)[0]) - toe(L)) + 0.0005 * (max(parts) - min(parts))
  v = round(255 * unlin(L ** 3)) # (the gray it would be at 24 bits: of those that look the same at 16, the nearest that, so black is 000000 and white FFFFFF)
  return min(range(256), key=lambda u: (round(off(u), 6), abs(u - v))) * 0x010101

def snap(rgb):  # the 16-bit color nearest rgb, its hue weighed double (a Rinc drops the low bits: dark and pale colors' hues drift)
  L, a, b = to_oklab(rgb)
  r, g, bl = rgb >> 19 & 31, rgb >> 10 & 63, rgb >> 3 & 31
  near = [(r + i) * 255 // 31 << 16 | (g + j) * 255 // 63 << 8 | (bl + k) * 255 // 31
          for i in (-1, 0, 1) for j in (-1, 0, 1, 2) for k in (-1, 0, 1) if 0 <= r + i < 32 and 0 <= g + j < 64 and 0 <= bl + k < 32]
  def off(c):
    L2, a2, b2 = to_oklab(c)
    return (L2 - L) ** 2 + 4 * ((a2 - a) ** 2 + (b2 - b) ** 2)
  return min(near, key=off)

def stats(name, colors):
  labs = [to_oklab(as565(c)) for c in colors]
  near, same, dup = [], 0, len(colors) - len(set(as565(c) for c in colors))
  for i in range(len(labs)):
    best = min(math.dist(labs[i], labs[j]) for j in range(len(labs)) if j != i)
    near.append(best)
    same += best < 0.02
  if '--twins' in sys.argv and name == 'this':
    for i in range(len(labs)):
      for j in range(i + 1, len(labs)):
        if math.dist(labs[i], labs[j]) < 0.02: print('  twins %d %d: %06X %06X' % (i, j, colors[i], colors[j]))
  near.sort()
  dark = sum(1 for l in labs if l[0] < 0.45)
  muted = sum(1 for l in labs if 0.03 <= math.hypot(l[1], l[2]) < 0.10)
  print('%-9s nearest pair %.4f, median nearest %.4f; %d with a near twin (< 0.02), %d the same at 16 bits; %d dark (L < 0.45), %d muted (chroma 0.03-0.10)'
        % (name, near[0], near[len(near) // 2], same, dup, dark, muted))

# ---------------------------------------------------------------- the preview and the table

def preview(path, rows):
  """rows of (label, colors): each a strip of 16-pixel swatches"""
  W, cell = 16 * 32, 16
  lines = []
  for label, colors in rows:
    W = max(W, cell * (len(colors) if label == 'side' else 32))
  for label, colors in rows:
    wide = len(colors) if label == 'side' else 32 # (a side-by-side row on one line; None, a gap)
    for start in range(0, len(colors), wide):
      line = colors[start:start + wide]
      for _ in range(cell):
        px = b''.join(bytes([c >> 16 & 255, c >> 8 & 255, c & 255] if c is not None else [0, 0, 0]) * cell for c in line)
        lines.append(b'\x00' + px + bytes(3 * (W - len(px) // 3)))
    for _ in range(8): lines.append(b'\x00' + bytes(3 * W))
  def chunk(t, d): return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
  png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', W, len(lines), 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(b''.join(lines))) + chunk(b'IEND', b'')
  open(path, 'wb').write(png)

def table(path):  # an earlier palette: the first 256 0x numbers in a table this wrote
  return [int(part[:6], 16) for part in open(path).read().split('0x')[1:257]]

new = palette()
befores = [table(sys.argv[k + 1]) for k, a in enumerate(sys.argv) if a == '--before'] # (newest first)
for k, b in enumerate(befores): stats('before %d' % (k + 1), b)
stats('this', new)
valued = {'--hue-list', '--steps', '--muted', '--tints', '--low', '--high', '--pale', '--pale-chroma', '--pale-cap', '--yellow', '--smooth', '--warm', '--cool', '--tint-chroma', '--muted-low', '--muted-high', '--before'} # (the options that take a value: --twins doesn't)
args = [a for k, a in enumerate(sys.argv[1:], 1) if not a.startswith('--') and sys.argv[k - 1] not in valued]
def parts(colors):  # its ramps, each a row, then the tints and grays, 16 a row
  return [colors[32 + k * 2 * STEPS:32 + (k + 1) * 2 * STEPS] for k in range(HUES)] + [colors[k:k + 16] for k in range(32 + HUES * 2 * STEPS, 256, 16)]
if args:
  # side by side, each hue a row: this palette's ramps (vivid then muted) on the left, then the earlier
  # tables', newest first, a gap between; then PICO-8's 32
  columns = [parts(new)] + [parts(b) for b in befores]
  rows = [('side', sum(([None] * (j > 0) + c for j, c in enumerate(row)), [])) for row in zip(*columns)] + [('pico', new[:32])]
  preview(args[0], rows)
if len(args) > 1:
  with open(args[1], 'w', newline='\n') as f:
    made = ' '.join(a for k, a in enumerate(sys.argv[1:], 1) if a.startswith('--') or sys.argv[k - 1] in valued) # (the options given)
    f.write('/* Rinc\'s palette, as 0xRRGGBB (docs/spec/screen.md#the-palette): PICO-8\'s 32, %d hues x 2 chromas x %d steps,\n' % (HUES, STEPS))
    f.write('   %d tints, %d grays. Made by: python tools/palette.py preview.png src/core/palette.h%s\n' % (TINTS, GRAYS, ' ' + made if made else ''))
    f.write('   hues %s; vivid %g to %g, smoothed %g; pale tops %g, %g of their chroma, at most %g; muted %g */\n'
            % (','.join('%d' % round(h) for h in HUE_LIST), L_LOW, L_HIGH, SMOOTH, PALE, PALE_C, PALE_CAP, MUTED))
    f.write('static const uint32_t rinc_colors[256] = {\n')
    for i in range(0, 256, 8): f.write('  ' + ', '.join('0x%06X' % c for c in new[i:i + 8]) + ',\n')
    f.write('};\n')
