"""The Colors app's second view: Rinc's 256 colors by hue (across) and lightness (down), as OKLCH sees
them. Prints the table colors.rio keeps (hueCells: each color's cell), so the app needn't search.

  python tools/huegrid.py [preview.png]

The grid is COLS x ROWS: the grays (chroma under NEUTRAL) in the first two columns, the other colors in
the rest. Each color wants the cell at its hue and lightness; a greedy pass places them (the most vivid
first), then swaps and moves that bring colors nearer their cells, until none does. The palette is
fixed, so this runs only when the grid changes."""
import math, struct, sys, zlib

pico = [0x000000, 0x1D2B53, 0x7E2553, 0x008751, 0xAB5236, 0x5F574F, 0xC2C3C7, 0xFFF1E8,
        0xFF004D, 0xFFA300, 0xFFEC27, 0x00E436, 0x29ADFF, 0x83769C, 0xFF77A8, 0xFFCCAA,
        0x291814, 0x111D35, 0x422136, 0x125359, 0x742F29, 0x49333B, 0xA28879, 0xF3EF7D,
        0xBE1250, 0xFF6C24, 0xA8E72E, 0x00B543, 0x065AB5, 0x754665, 0xFF6E59, 0xFF9D81]
LEVELS = [0, 95, 135, 175, 215, 255]
GRAYS = [19, 38, 57, 76, 115, 155, 195, 235]
COLS, ROWS, GRAY_COLS = 26, 14, 2
NEUTRAL = 0.03  # chroma below this: a gray
HUE0 = 20       # the hue at the left edge, in degrees (pinks, then reds)

def rgb(i):  # (as src/core/rinc.c's rinc_rgb)
    if i < 32: return pico[i]
    if i < 248:
        k = i - 32
        return LEVELS[k // 36] << 16 | LEVELS[k // 6 % 6] << 8 | LEVELS[k % 6]
    return GRAYS[i - 248] * 0x010101

def oklch(i):  # (Björn Ottosson's OKLab, from sRGB)
    def lin(c):
        c /= 255
        return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
    v = rgb(i)
    r, g, b = lin(v >> 16), lin(v >> 8 & 255), lin(v & 255)
    l = (0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b) ** (1 / 3)
    m = (0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b) ** (1 / 3)
    s = (0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b) ** (1 / 3)
    L = 0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s
    a = 1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s
    bb = 0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s
    return L, math.hypot(a, bb), math.degrees(math.atan2(bb, a)) % 360

lch = [oklch(i) for i in range(256)]
gray = [lch[i][1] < NEUTRAL for i in range(256)]

def want(i):  # the cell a color wants: x across, y down, as floats
    L, c, h = lch[i]
    x = GRAY_COLS / 2 if gray[i] else GRAY_COLS + ((h - HUE0) % 360) / 360 * (COLS - GRAY_COLS)
    return x, (1 - L) * ROWS

def cost(i, cell):
    x, y = cell
    wx, wy = want(i)
    dx = abs(x + 0.5 - wx)
    if not gray[i]: dx = min(dx, COLS - GRAY_COLS - dx)  # (hue wraps around)
    return dx * dx + (y + 0.5 - wy) ** 2

def cells(i):  # where a color may go: a gray in the grays' columns, the rest in the others
    xs = range(GRAY_COLS) if gray[i] else range(GRAY_COLS, COLS)
    return [(x, y) for x in xs for y in range(ROWS)]

place, at = {}, {}
for i in sorted(range(256), key=lambda i: -lch[i][1]):  # (greedy: the most vivid first)
    cell = min((c for c in cells(i) if c not in at), key=lambda c: cost(i, c))
    place[i] = cell
    at[cell] = i
moved = True
while moved:  # (then better: a move to a free cell, or a swap, wherever it lowers the total)
    moved = False
    for i in range(256):
        for cell in cells(i):
            j, mine = at.get(cell), place[i]
            if j == i or (j is not None and gray[j] != gray[i]): continue
            before = cost(i, mine) + (cost(j, cell) if j is not None else 0)
            after = cost(i, cell) + (cost(j, mine) if j is not None else 0)
            if after < before - 1e-9:
                if j is None: del at[mine]
                else: place[j] = mine; at[mine] = j
                place[i] = cell
                at[cell] = i
                moved = True

table = [place[i][1] * COLS + place[i][0] for i in range(256)]
print('hueCells := [256]Int{')
for k in range(0, 256, 16):
    print('  ' + ', '.join(str(v) for v in table[k:k + 16]) + ',')
print('}')

if len(sys.argv) > 1:  # a preview, 12 pixels a cell
    S = 12
    w, h = COLS * S, ROWS * S
    px = [0x303030] * (w * h)
    for i, (x, y) in place.items():
        for yy in range(y * S + 1, y * S + S):
            for xx in range(x * S + 1, x * S + S):
                px[yy * w + xx] = rgb(i)
    raw = b''.join(b'\0' + bytes(v for p in px[y * w:(y + 1) * w] for v in (p >> 16, p >> 8 & 255, p & 255)) for y in range(h))
    def chunk(t, d): return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    open(sys.argv[1], 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) +
                                  chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))
