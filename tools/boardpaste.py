# /// script
# dependencies = ["pyserial"]
# ///
"""uv run tools/boardpaste.py COM3 name file: an app's source pasted in, as a person with a serial terminal
does: Ctrl-B (the firmware's console), save name, the file's text, Ctrl-D; then home. App name's source
(/rinc/apps/name/name.rio) becomes the file's text. For changing an app on the board the way its user
would (Cloudlamp was rewritten so), then running it with boardplay.py."""
import re, sys, time
import serial
port, name, path = sys.argv[1], sys.argv[2], sys.argv[3]
text = open(path, encoding='utf-8').read().replace('\r\n', '\n')
s = serial.Serial(port, 115200, timeout=0.05)
def read(t):
  out, end = b'', time.time() + t
  while time.time() < end: out += s.read(4096)
  return out.decode('utf-8', 'replace').replace('\r', '')
s.dtr = True
s.write(b'\x02'); print(read(2.0).strip().split('\n')[-1])
s.write(('save ' + name + '\r').encode()); print(read(1.0).strip().split('\n')[-1])
data = text.encode()
for i in range(0, len(data), 256): # (as a terminal pastes: a little at a time)
  s.write(data[i:i + 256]); s.flush(); time.sleep(0.03)
s.write(b'\x04')
print(read(2.0).strip().split('\n')[-1])
s.write(b'home\r'); read(0.5)
