# /// script
# dependencies = ["pyserial"]
# ///
r"""Play an app on the board as a person would, over serial, and look at it: uv run tools/boardplay.py COM3
step step ... Meant for trying a game or an app as its user meets it (what's awkward shows), and for
checking a change on the board, not just on the desktop:
  con:LINE         Ctrl-C (the console app), then LINE typed and Enter (run cloudlamp, say)
  fw:LINE          Ctrl-B (the firmware's own console: the app is left), LINE, what it says printed, then
                   home (fw:audio: how long mixing takes)
  wait S           S seconds
  tap B            button B pressed and let go (left right up down a b x y): kitty's key down, then up
  hold B S         button B held S seconds
  type TEXT        TEXT typed as keys, with rinc -type's escapes (\n Enter, \e Esc, \t Tab, \xHH a byte:
                   \x1b[A up), each escape sequence sent whole, a key every 0.08 s; Ctrl+C and
                   Ctrl+B (\x03, \x02) sent as kitty's keys, so the app gets them (as bytes, over
                   serial, they'd open the console)
  as NAME          the app on the screen is NAME now (one another app started: its shots are named for it)
  shot FILE        Print Screen (the board saves /rinc/shots/<app>-<n>.png), then it fetched (the firmware
                   console's get: the app is left) and written to FILE, 4 times as big, to look at
Before the first con:, /rinc/shots is emptied (through the console), so shots number from 1. For example:
  uv run tools/boardplay.py COM3 "con:run cloudlamp" wait 2 hold right 4 shot lamp.png"""
import re, struct, sys, time, zlib
import serial
KEYS = {'left': ('1;1', 'D'), 'right': ('1;1', 'C'), 'up': ('1;1', 'A'), 'down': ('1;1', 'B'),
        'a': ('118;1', 'u'), 'b': ('99;1', 'u'), 'x': ('120;1', 'u'), 'y': ('122;1', 'u')}
port, steps = sys.argv[1], sys.argv[2:]
s = serial.Serial(port, 115200, timeout=0.05)
def read(t):
  out, end = b'', time.time() + t
  while time.time() < end: out += s.read(4096)
  return out
def send(b, gap=0.0):
  s.write(b); s.flush()
  if gap: time.sleep(gap)
def button(name, down):
  code, end = KEYS[name]
  send(b'\x1b[' + code.encode() + (b':1' if down else b':3') + end.encode())
def console(line):
  send(b'\x03'); read(2.5)
  for ch in line + '\r': send(ch.encode(), 0.01)
  return read(1.5)
def enlarge(png, k):
  i, chunks = 8, []
  while i < len(png):
    n, kind = struct.unpack('>I4s', png[i:i + 8]); chunks.append((kind, png[i + 8:i + 8 + n])); i += 12 + n
  head = dict(chunks)[b'IHDR']; w, h = struct.unpack('>II', head[:8])
  rows = zlib.decompress(b''.join(b for kind, b in chunks if kind == b'IDAT'))
  px = [rows[y * (w + 1) + 1:(y + 1) * (w + 1)] for y in range(h)] # (Rinc writes filter 0 rows)
  big = b''.join((b'\0' + bytes(c for c in row for _ in range(k))) * k for row in px)
  def chunk(kind, body): return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body))
  out = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>II', w * k, h * k) + head[8:])
  for kind, body in chunks:
    if kind in (b'PLTE', b'tRNS'): out += chunk(kind, body)
  return out + chunk(b'IDAT', zlib.compress(big)) + chunk(b'IEND', b'')
def get(path):
  s.dtr = True
  send(b'\x02')
  out = read(2.0)
  s.reset_input_buffer()
  send(b'get ' + path.encode() + b'\r')
  out, last = b'', time.time()
  while time.time() - last < 5:
    c = s.read(65536)
    if c:
      out += c; last = time.time()
      if re.search(rb'(got \d+ bytes|no file)', out): break
  send(b'home\r')
  text = out.decode('ascii', 'replace').replace('\r', '')
  m = re.search(r'got (\d+) bytes', text)
  if not m: return None
  return bytes.fromhex(''.join(l for l in text.split('\n') if re.fullmatch(r'[0-9a-f]+', l)))
def keysOf(text):
  """text as rinc -type takes it (\\n Enter, \\e Esc, \\t Tab, \\xHH a byte: \\x1b[A the up arrow), as keys:
  each a byte, or an escape sequence whole (sent at once, so the board reads it as one key)"""
  b = re.sub(r'\\(x[0-9a-fA-F]{2}|[nte\\])', lambda m: {'n': '\r', 't': '\t', 'e': '\x1b', '\\': '\\'}.get(m.group(1)) or chr(int(m.group(1)[1:], 16)), text).encode('latin-1')
  keys = [m.group() for m in re.finditer(rb'\x1b\[[0-9;:?]*[\x40-\x7e]|\x1bO.|.', b, re.S)] # (ESC [ ... and ESC O x, F1 to F4: whole)
  return [{b'\x03': b'\x1b[99;5u', b'\x02': b'\x1b[98;5u'}.get(k, k) for k in keys] # (Ctrl+C and Ctrl+B as kitty's keys: over serial, 3 opens the console and 2 the firmware's)
read(0.3)
shots, app, cleared = 0, None, False
i = 0
while i < len(steps):
  st = steps[i]
  if st.startswith('con:'):
    line = st[4:]
    if not cleared: console('pc.fileDelete("/rinc/shots")'); cleared = True
    m = re.match(r'run (\S+)', line) or re.search(r'pc\.call\("([^"]+)"', line) # (the app its shots are named for)
    if m: app = m.group(1)
    print(re.sub(r'\x1b\[[0-9;?]*[A-Za-z]', '', console(line).decode('utf-8', 'replace')).strip()[-300:])
  elif st.startswith('fw:'):
    s.dtr = True
    send(b'\x02'); read(2.0)
    s.reset_input_buffer()
    send(st[3:].encode() + b'\r')
    print(re.sub(r'\x1b\[[0-9;?]*[A-Za-z]', '', read(1.5).decode('utf-8', 'replace')).strip()[-300:])
    send(b'home\r'); read(1.0)
  elif st == 'wait': read(float(steps[i + 1])); i += 1
  elif st == 'as': app = steps[i + 1]; i += 1 # (the app on the screen now, its shots' name: one an app started)
  elif st == 'tap': button(steps[i + 1], True); read(0.12); button(steps[i + 1], False); read(0.12); i += 1
  elif st == 'hold': button(steps[i + 1], True); read(float(steps[i + 2])); button(steps[i + 1], False); read(0.1); i += 2
  elif st == 'type':
    for k in keysOf(steps[i + 1]): send(k, 0.15 if k == b'\x1b' else 0.08) # (a lone Esc waited after: not the start of a sequence)
    i += 1
  elif st == 'shot':
    send(b'\x1b[57361u'); read(1.5); shots += 1
    data = get('shots/%s-%d.png' % (app, shots))
    if data: open(steps[i + 1], 'wb').write(enlarge(data, 4)); print('shot', steps[i + 1], len(data), 'bytes')
    else: print('shot: none came back')
    i += 1
  i += 1
