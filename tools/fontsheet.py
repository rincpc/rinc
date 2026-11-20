"""Rinc's own font as a sheet like any user font (plans/sheets.md, step 3): src/core/rinc-font.png, a 1-bit
sheet of 6x9 cells, 16 across, characters 32 to 126, saved uncompressed (a stored block a row, as Paint
saves sheets), its notes a font's. The firmware carries the file's bytes as they are (src/core/font.c) and
draws from them where they lie, a row at a sum, as a sheet in flash is read.

  python tools/fontsheet.py              font.c from rinc-font.png (after it's edited, in Paint, say)
  python tools/fontsheet.py --from X.png rinc-font.png from a font sheet of any depth Paint saves (Rinc's
                                         colors, 0 see-through: any other color drawn), then font.c. Its
                                         notes give the cells ("sprite 5x11", "font 32 255", "across 16");
                                         drawn fixed width whatever they say. A glyph of 33 to 126 left
                                         blank in it keeps rinc-font.png's (named, as a placeholder), its
                                         top rows matched (the rows above every ASCII glyph: compact's)
  python tools/fontsheet.py --from-table rinc-font.png from font.c's old table (once: the 5x8 font's glyphs)"""
import os, re, struct, sys, zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PNG = os.path.join(ROOT, 'src', 'core', 'rinc-font.png')
FONT_C = os.path.join(ROOT, 'src', 'core', 'font.c')
CW, CH, ACROSS, FIRST, LAST = 6, 9, 16, 32, 126
NOTES = 'sprite 6x9\nfont 32 126\nacross 16\n'
WHITE = 0xFFF1E8 # (Rinc's 7: drawn in print's color anyway)

def chunk(t, d): return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)

def readPng(path):
    """a PNG's chunks, its pixels as rows of palette numbers (or 0 and 1 for gray: 0 none), and its notes"""
    b, i, c = open(path, 'rb').read(), 8, {}
    while i < len(b):
        n, t = struct.unpack('>I4s', b[i:i + 8]); c[t] = c.get(t, b'') + b[i + 8:i + 8 + n]; i += 12 + n
    w, h, depth, kind = struct.unpack('>IIBB', c[b'IHDR'][:10])
    assert kind in (0, 3), 'a palette or gray PNG, as Paint saves'
    rb, bpp = (w * depth + 7) // 8, max(1, depth // 8)
    raw, rows, prev = zlib.decompress(c[b'IDAT']), [], bytearray(rb)
    for y in range(h):
        f, line = raw[y * (rb + 1)], bytearray(raw[y * (rb + 1) + 1:(y + 1) * (rb + 1)])
        for x in range(rb):
            a, up, ul = line[x - bpp] if x >= bpp else 0, prev[x], prev[x - bpp] if x >= bpp else 0
            if f == 1: line[x] = (line[x] + a) & 255
            elif f == 2: line[x] = (line[x] + up) & 255
            elif f == 3: line[x] = (line[x] + (a + up) // 2) & 255
            elif f == 4:
                q = a + up - ul; pa, pb, pc = abs(q - a), abs(q - up), abs(q - ul)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else up if pb <= pc else ul)) & 255
        rows.append([line[x * depth // 8] >> (8 - depth - x * depth % 8) & (1 << depth) - 1 for x in range(w)]); prev = line
    notes = c.get(b'tEXt', b'').split(b'\0', 1)[-1].decode('latin-1')
    return w, h, rows, notes

def noted(notes, key, default):
    for line in notes.splitlines():
        if line.startswith(key + ' '): return [int(v) for v in line[len(key) + 1:].replace('x', ' ').split()]
    return default

def cells(path):
    """a font sheet's glyphs: {character: rows of 0 and 1}, its cell, and its top (rows above every ASCII one)"""
    w, h, rows, notes = readPng(path)
    cw, ch = noted(notes, 'sprite', [6, 9])
    first, last = noted(notes, 'font', [32, 126])
    across = noted(notes, 'across', [w // cw])[0]
    g = {}
    for c in range(first, last + 1):
        x0, y0 = (c - first) % across * cw, (c - first) // across * ch
        g[c] = [[1 if y0 + y < h and x0 + x < w and rows[y0 + y][x0 + x] else 0 for x in range(cw)] for y in range(ch)]
    used = [y for c in range(33, 127) if c in g for y in range(ch) if any(g[c][y])]
    return g, cw, ch, first, last, min(used) if used else 0

def fromPng(path):
    g, cw, ch, first, last, top = cells(path)
    old, ocw, och, _, _, otop = cells(PNG)
    kept = []
    for c in range(33, 127):
        if c in g and not any(map(any, g[c])) and c in old and any(map(any, old[c])):
            kept.append(chr(c))
            g[c] = [[old[c][y - top + otop][x] if 0 <= y - top + otop < och and x < ocw else 0 for x in range(cw)] for y in range(ch)]
    rowsOf = (last - first + ACROSS) // ACROSS
    w, h = ACROSS * cw, rowsOf * ch
    rows = [[0] * w for _ in range(h)]
    for c in range(first, last + 1):
        for y in range(ch):
            for x in range(cw):
                rows[(c - first) // ACROSS * ch + y][(c - first) % ACROSS * cw + x] = g[c][y][x]
    notes = 'sprite %dx%d\nfont %d %d\nacross %d\n' % (cw, ch, first, last, ACROSS)
    open(PNG, 'wb').write(writeSheet(rows, w, h, notes))
    print('wrote', PNG, '%dx%d, cells %dx%d, %d to %d, top %d (compact: a line of %d)' % (w, h, cw, ch, first, last, top, ch - top + 1))
    if kept: print('placeholders (blank in it: rinc-font.png\'s kept):', ' '.join(kept))
    toC()

def writeSheet(rows, w, h, notes=NOTES):
    """a 1-bit sheet, rows of 0s and 1s, as rinc_pngWrite's sheet: one IDAT, a stored block a row"""
    rowBytes = (w + 7) // 8
    data, a, b = bytearray(b'\x78\x01'), 1, 0
    for y, row in enumerate(rows):
        packed = bytearray(rowBytes)
        for x, v in enumerate(row):
            if v: packed[x // 8] |= 0x80 >> (x % 8)
        n = rowBytes + 1
        data += bytes([1 if y == h - 1 else 0, n & 255, n >> 8, ~n & 255, (~n >> 8) & 255])
        for byte in b'\0' + bytes(packed):
            data.append(byte)
            a = (a + byte) % 65521; b = (b + a) % 65521
    data += struct.pack('>I', b << 16 | a)
    plte = b'\0\0\0' + struct.pack('>I', WHITE)[1:]
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 1, 3, 0, 0, 0)) + chunk(b'PLTE', plte) +
            chunk(b'tRNS', b'\0') + chunk(b'tEXt', b'Rinc\0' + notes.encode()) + chunk(b'IDAT', bytes(data)) + chunk(b'IEND', b''))

def fromTable():
    text = open(FONT_C, encoding='utf-8').read()
    glyphs = [[int(v, 16) for v in re.findall(r'0x([0-9A-Fa-f]{2})', line)] for line in re.findall(r'\{(0x[^}]*)\}', text)]
    assert len(glyphs) == LAST - FIRST + 1 and all(len(g) == 8 for g in glyphs), 'not the old table'
    rowsOf = (LAST - FIRST + ACROSS) // ACROSS
    w, h = ACROSS * CW, rowsOf * CH
    rows = [[0] * w for _ in range(h)]
    for i, g in enumerate(glyphs):
        cx, cy = i % ACROSS * CW, i // ACROSS * CH
        for r in range(8):
            for k in range(5):
                rows[cy + r][cx + k] = g[r] >> k & 1 # (bit 0 the leftmost column)
    open(PNG, 'wb').write(writeSheet(rows, w, h))
    print('wrote', PNG, '%dx%d' % (w, h))

def toC():
    b = open(PNG, 'rb').read()
    lines = ['/* Rinc\'s own font: src/core/rinc-font.png\'s bytes, as they are (a sheet of 6x9 cells, 16 across, characters',
             '   32 to 126, 1 bit, its rows uncompressed: drawn from where they lie). Made by tools/fontsheet.py: edit the',
             '   PNG (in Paint), then run it */',
             '#include "rinc.h"', '', 'const uint8_t rinc_fontSheet[%d] = {' % len(b)]
    for i in range(0, len(b), 16):
        lines.append('  ' + ' '.join('0x%02X,' % v for v in b[i:i + 16]))
    lines += ['};', 'const uint32_t rinc_fontSheetLen = sizeof rinc_fontSheet;', '']
    open(FONT_C, 'w', newline='\n').write('\n'.join(lines))
    print('wrote', FONT_C, len(b), 'bytes')

if '--from-table' in sys.argv: fromTable()
elif '--from' in sys.argv: fromPng(sys.argv[sys.argv.index('--from') + 1])
else: toC()
