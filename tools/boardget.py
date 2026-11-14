# /// script
# dependencies = ["pyserial"]
# ///
"""uv run tools/boardget.py COM3 inside=local ...: files from the board's card, each /rinc/inside (as put
takes it: apps/x/x.rio, shots/x-1.png) to local, by the firmware console's get (hex, 64 bytes a line,
then "got N bytes"), then home: a second or so a file. For bringing back what was made on the board (an
app written there, a screenshot, a song), to look at or keep in the repo."""
import re, sys, time
import serial
port, pairs = sys.argv[1], [a.split('=', 1) for a in sys.argv[2:]]
with serial.Serial(port, 115200, timeout=0.1) as s:
  s.dtr = True
  s.write(b'\x02')
  out, end = b'', time.time() + 5
  while b'Rinc: rio REPL' not in out and time.time() < end: out += s.read(4096)
  if b'Rinc: rio REPL' not in out: sys.exit('no answer')
  time.sleep(0.2); s.reset_input_buffer()
  for inside, local in pairs:
    s.write(b'get ' + inside.encode() + b'\r')
    out, last = b'', time.time()
    while time.time() - last < 10:
      chunk = s.read(65536)
      if chunk:
        out += chunk; last = time.time()
        if re.search(rb'(got \d+ bytes|no file|couldn\'t read|a path under)', out): break
    text = out.decode('ascii', 'replace').replace('\r', '')
    m = re.search(r'got (\d+) bytes', text)
    if not m: print(inside, text.strip().split('\n')[-1]); continue
    data = bytes.fromhex(''.join(l for l in text.split('\n') if re.fullmatch(r'[0-9a-f]+', l)))
    if len(data) != int(m.group(1)): print(inside, 'got %d of %s bytes' % (len(data), m.group(1))); continue
    open(local, 'wb').write(data)
    print(inside, '%d bytes' % len(data))
  s.write(b'home\r')
