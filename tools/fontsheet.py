"""Rinc's own font as a sheet like any user font (plans/sheets.md, step 3): src/core/rinc-font.png, a 1-bit
sheet of 6x9 cells, 16 across, characters 32 to 126, saved uncompressed (a stored block a row, as Paint
saves sheets), its notes a font's. The firmware carries the file's bytes as they are (src/core/font.c) and
draws from them where they lie, a row at a sum, as a sheet in flash is read.

  python tools/fontsheet.py              font.c from rinc-font.png (after it's edited, in Paint, say)
  python tools/fontsheet.py --from-table rinc-font.png from font.c's old table (once: the 5x8 font's glyphs)"""
import os, re, struct, sys, zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PNG = os.path.join(ROOT, 'src', 'core', 'rinc-font.png')
FONT_C = os.path.join(ROOT, 'src', 'core', 'font.c')
CW, CH, ACROSS, FIRST, LAST = 6, 9, 16, 32, 126
NOTES = 'sprite 6x9\nfont 32 126\nacross 16\n'
WHITE = 0xFFF1E8 # (Rinc's 7: drawn in print's color anyway)

def chunk(t, d): return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)

def writeSheet(rows, w, h):
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
            chunk(b'tRNS', b'\0') + chunk(b'tEXt', b'Rinc\0' + NOTES.encode()) + chunk(b'IDAT', bytes(data)) + chunk(b'IEND', b''))

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
else: toC()
