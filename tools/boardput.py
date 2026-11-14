# /// script
# dependencies = ["pyserial"]
# ///
"""uv run tools/boardput.py COM3 inside=path ...: files put on the board's card, each at /rinc/inside (apps/x/x.rio,
save/x/y.png), through the firmware console's put (as install.py does), then home. For trying a changed
app on the board without a whole install (a copy under another name keeps the real one's stash alone).
On Git Bash, set MSYS_NO_PATHCONV=1, or it rewrites paths that start with /."""
import sys, time
import serial
port, pairs = sys.argv[1], [a.split('=', 1) for a in sys.argv[2:]]
def until(s, ends, wait=20):
  out, last = b'', time.time()
  while time.time() - last < wait:
    chunk = s.read(4096)
    if chunk:
      out += chunk; last = time.time()
      lines = out.decode(errors='replace').replace('\r', '').split('\n')
      done = [l for l in lines if l.lstrip('> ').startswith(ends)]
      if done: return done[-1].lstrip('> ')
  return None
with serial.Serial(port, 115200, timeout=0.1) as s:
  s.dtr = True
  s.write(b'\x02')
  if not until(s, ('Rinc: rio REPL',), 5): sys.exit('no answer')
  time.sleep(0.2); s.reset_input_buffer()
  for inside, path in pairs:
    hexed = open(path, 'rb').read().hex()
    s.write(b'put ' + inside.encode() + b'\r')
    said = until(s, ('putting', "can't", 'no storage', 'a path'))
    if not said or not said.startswith('putting'): print(inside, said); continue
    s.write(b'\r'.join(hexed[i:i + 128].encode() for i in range(0, len(hexed), 128)) + b'\x04')
    print(inside, until(s, ('saved', "couldn't", 'gave up')))
  s.write(b'home\r')
