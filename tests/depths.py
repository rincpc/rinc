"""python depths.py path/to/rinc work-folder: pictures at 1, 2, 4 and 8 bits (indexed PNGs, their palettes
Rinc's colors, index 0 see-through), read at their own depth and as bytes: each pixel the same (imageAt);
drawn the same (blit: as it is, scaled, mirrored), and in other colors with a map; and a 1-bit font printed
as its bytes' twin is, in two colors. The app logs "depths came through" (plans/sheets.md, step 1)"""
import os, re, shutil, struct, subprocess, sys, zlib

rinc, work = os.path.abspath(sys.argv[1]), sys.argv[2]
here = os.path.dirname(os.path.abspath(__file__))
shutil.rmtree(work, ignore_errors=True)
app = os.path.join(work, 'depths')
os.makedirs(app)
colors = [int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{6})', open(os.path.join(here, '..', 'src', 'core', 'palette.h')).read())]

def png(path, w, h, depth, pal, pixels, notes=''):
    """an indexed PNG: pixels (indices) packed at depth, pal the Rinc colors of each index (0: see-through)"""
    rows = b''
    for y in range(h):
        bits, row = 0, bytearray()
        acc = 0
        for x in range(w):
            acc = acc << depth | pixels[y * w + x]
            bits += depth
            if bits == 8: row.append(acc); acc = bits = 0
        if bits: row.append(acc << (8 - bits))
        rows += b'\0' + bytes(row)
    plte = b''.join(struct.pack('>I', colors[c])[1:] for c in pal)
    def chunk(t, d): return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    text = chunk(b'tEXt', b'Rinc\0' + notes.encode()) if notes else b''
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, depth, 3, 0, 0, 0)) + chunk(b'PLTE', plte) +
                           chunk(b'tRNS', b'\0') + text + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b''))

W, H = 13, 7 # (13: rows that end mid-byte at each depth)
pals = {1: [0, 8], 2: [0, 8, 11, 12], 4: [0] + list(range(1, 16)), 8: [0] + list(range(1, 256))}
for d in (1, 2, 4, 8):
    n = 1 << d
    png(os.path.join(app, 'd%d.png' % d), W, H, d, pals[d], [(x * 3 + y * 5 + x * y) % n for y in range(H) for x in range(W)])
# a 1-bit font: 4x6 cells, 8 across, two rows: characters 48 (0) to 63 (?), each a different bar
font = [0] * (32 * 12)
for g in range(16):
    for y in range(1, 5):
        for x in range(4):
            if (x + y + g) % 3 != 0 and x < 3 - g // 8: font[(g // 8 * 6 + y) * 32 + g % 8 * 4 + x] = 1
png(os.path.join(app, 'font.png'), 32, 12, 1, [0, 7], font, 'sprite 4x6\nfont 48 63\nacross 8\nproportional\n')

open(os.path.join(app, 'depths.rio'), 'w', newline='\n').write('''
room: Blob[..4000]
bytesRoom: Blob[..4000]
same :: proc(a: [2]Int, b: [2]Int, size: [2]Int) -> Bool # (two parts of the screen alike)
  for y in 0..<size.y
    for x in 0..<size.x
      if pc.pget(a + {x, y}) != pc.pget(b + {x, y}) do return false
    end
  end
  return true
end
depths := [4]Int{1, 2, 4, 8}
for d in depths
  room.clear()
  bytesRoom.clear()
  name: Blob[..8]
  _ = name.print("d", d, ".png")
  packed := pc.imageRead(name.asString(), room)
  bytes := pc.imageRead(name.asString(), bytesRoom, {bytes = true})
  want := d
  if d == 8 do want = 0
  assert(packed.size == {13, 7} and packed.depth == want and bytes.depth == 0, "read at its depth, and as bytes")
  assert(d == 8 or packed.pixels.len() < bytes.pixels.len(), "packed: smaller")
  for y in 0..<7
    for x in 0..<13
      assert(pc.imageAt(packed, {x, y}) == bytes.pixels[y * 13 + x], "a pixel, at any depth, its color")
    end
  end
  pc.cls(1)
  pc.blit(packed, {0, 0})
  pc.blit(bytes, {20, 0})
  assert(same({0, 0}, {20, 0}, {13, 7}), "drawn as it is")
  pc.blit(packed, {0, 10}, {scale = {-2, 2}, from = {1, 1}, size = {9, 5}})
  pc.blit(bytes, {40, 10}, {scale = {-2, 2}, from = {1, 1}, size = {9, 5}})
  assert(same({0, 10}, {40, 10}, {18, 10}), "drawn scaled, mirrored, a part")
end
# a map: d1's index 1 (red, 8) drawn orange (9), its 0 see-through
room.clear()
one := pc.imageRead("d1.png", room)
pc.cls(1)
orange := [2]Int{-1, 9}
pc.blit(one, {0, 0}, {map = orange[]})
for y in 0..<7
  for x in 0..<13
    c := pc.imageAt(one, {x, y})
    want := 1
    if c == 8 do want = 9
    assert(pc.pget({x, y}) == want, "a map: its index 1 in another color, 0 see-through")
  end
end
# a 1-bit font printed as its bytes' twin, in two colors
fontRoom: Blob[..600]
notes: Blob[..128]
_ = pc.imageNotes("font.png", notes)
small := pc.fontRead("font.png", fontRoom)
twinRoom: Blob[..600]
twinPic := pc.imageRead("font.png", twinRoom, {bytes = true})
twin := pc.fontOf(twinPic, pc.fontNotes(notes.asString()), twinRoom)
assert(small.image.depth == 1 and twin.image.depth == 0, "the font packed, its twin bytes")
inks := [2]Int{7, 12}
for c in inks
  pc.cls(1)
  pc.print("01234567", {0, 0}, c, {font = small})
  pc.print("01234567", {0, 20}, c, {font = twin})
  assert(same({0, 0}, {0, 20}, {40, 6}) and pc.pget({0, 1}) == c, "a 1-bit font printed as its twin, in its color")
end
# sheets written (imageWrite's sheet): the colors they use, as few bits as hold them, read back the same
pic: Blob[13 * 7]
counts := [4]Int{2, 3, 11, 21}
bitsFor := [4]Int{1, 2, 4, 0}
for k in 0..<4
  for i in 0..<13 * 7 do pic[i] = (i * 7 + i / 13) % counts[k] # (0 to counts - 1: Rinc's colors, 0 see-through)
  sheetName: Blob[..24]
  _ = sheetName.print("/save/s", k, ".png") # (in its storage: an app run from a folder doesn't write there)
  assert(pc.imageWrite(sheetName.asString(), {pixels = pic[], size = {13, 7}}, {sheet = true}) == 0, "a sheet written")
  room.clear()
  back := pc.imageRead(sheetName.asString(), room)
  assert(back.depth == bitsFor[k], "as few bits as its colors need")
  for i in 0..<13 * 7
    assert(pc.imageAt(back, {i % 13, i / 13}) == pic[i], "read back as it was written")
  end
end
# parts of a sheet saved uncompressed, at each depth, from each column in a byte, as the whole has them
for k in 0..<4
  partName: Blob[..24]
  _ = partName.print("/save/s", k, ".png")
  room.clear()
  whole := pc.imageRead(partName.asString(), room, {bytes = true})
  for x0 in 0..<8
    for asBytes in 0..<2
      bytesRoom.clear()
      part := pc.imageRead(partName.asString(), bytesRoom, {bytes = asBytes == 1, from = {x0, 2}, size = {5, 4}})
      assert(part.size == {5, 4} and (asBytes == 1 or part.depth == bitsFor[k]), "a part: its size, its depth")
      for y in 0..<4
        for x in 0..<5
          assert(pc.imageAt(part, {x, y}) == whole.pixels[(2 + y) * 13 + x0 + x], "a part's pixel, as the whole has it")
        end
      end
    end
  end
  bytesRoom.clear()
  edge := pc.imageRead(partName.asString(), bytesRoom, {from = {10, 5}, size = {8, 8}})
  assert(edge.size == {3, 2} and pc.imageAt(edge, {2, 1}) == whole.pixels[6 * 13 + 12], "a part past its edges: what's in it")
end
# a part of a compressed PNG: none (the log says why)
bytesRoom.clear()
squeezed := pc.imageRead("d4.png", bytesRoom, {from = {1, 1}, size = {4, 4}})
assert(squeezed.size == {0, 0} and bytesRoom.len() == 0, "a part of a compressed one: none")
# a font, its second row of characters read alone, prints as the whole one does
fontNotes: Blob[..128]
_ = pc.imageNotes("font.png", fontNotes)
fontPic: Blob[..600]
wholeFont := pc.imageRead("font.png", fontPic, {bytes = true})
assert(pc.imageWrite("/save/font.png", wholeFont, {notes = fontNotes.asString(), sheet = true}) == 0, "the font, as a sheet")
allRoom: Blob[..600]
all := pc.fontRead("/save/font.png", allRoom)
rowRoom: Blob[..600]
row := pc.fontRead("/save/font.png", rowRoom, {from = {0, 6}, size = {32, 6}})
assert(row.first == 56 and row.last == 63 and row.image.size == {32, 6}, "a font's second row: its characters")
assert(rowRoom.len() < allRoom.len(), "a part: less room")
pc.cls(1)
pc.print("89:;<=>?", {0, 0}, 7, {font = all})
pc.print("89:;<=>?", {0, 20}, 7, {font = row})
lit := 0
for y in 0..<6
  for x in 0..<40
    if pc.pget({x, y}) == 7 do lit += 1
  end
end
assert(same({0, 0}, {0, 20}, {40, 6}) and lit > 0, "printed as the whole font prints them")
log("depths came through")
''')
r = subprocess.run([rinc, app, '-data', os.path.join(work, 'data'), '-frames', '1'], capture_output=True, text=True, timeout=60)
said = (r.stdout + r.stderr).strip()
# each sheet's shape, for reading in place: one IDAT, zlib's header, then each row in a stored block of its own
def shapeOf(path):
  b, i, idats, depth, w, h = open(path, 'rb').read(), 8, [], 0, 0, 0
  while i < len(b):
    n, t = struct.unpack('>I4s', b[i:i + 8])
    if t == b'IHDR': w, h, depth = struct.unpack('>IIB', b[i + 8:i + 17])
    if t == b'IDAT': idats.append(b[i + 8:i + 8 + n])
    i += 12 + n
  if len(idats) != 1: return 'IDATs: %d' % len(idats)
  d, rowBytes = idats[0], (w * depth + 7) // 8
  for y in range(h):
    at = 2 + y * (5 + 1 + rowBytes)
    final, length, nlen = d[at], d[at + 1] | d[at + 2] << 8, d[at + 3] | d[at + 4] << 8
    if final != (y == h - 1) or length != rowBytes + 1 or nlen != (~length & 0xffff) or d[at + 5] != 0: return 'row %d' % y
  if zlib.decompress(d) is None: return 'not zlib'
  return 'ok'
for k in range(4):
  path = os.path.join(work, 'data', 'save', 's%d.png' % k)
  shape = shapeOf(path) if os.path.exists(path) else 'not written'
  if shape != 'ok': said += '\nFAIL s%d.png, its shape: %s' % (k, shape)
print(said)
