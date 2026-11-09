"""python typedbuttons.py path/to/rinc work-folder: buttons held over serial, as kitty's keyboard protocol
sends presses and releases (ESC [ 1 ; 1 : 1 C, right pressed; ESC [ 1 ; 1 : 3 C, let go; ESC [ 118 ; 1 : 1 u,
V, the A button): held from the press to the release, frame by frame, as pc.btn says; each press typed
too (pc.key), its release not; and keys sent the usual way, ESC [ C, typed but holding nothing."""
import os, shutil, subprocess, sys

rinc, work = os.path.abspath(sys.argv[1]), sys.argv[2]
shutil.rmtree(work, ignore_errors=True)
os.makedirs(os.path.join(work, 'held'))
with open(os.path.join(work, 'held', 'held.rio'), 'w') as f:
  f.write('''
right := 0
a := 0
frame := 0
typed: Blob[..40]
update* :: proc()
  right += pc.btn(1)
  a += pc.btn(4)
  for _ in 0..<8
    k := pc.key()
    if k >= 0
      fine := typed.print(k, " ")
    end
  end
  frame += 1
  if frame == 40
    log("right", right, "a", a, "typed", typed.asString())
  end
end
''')
F = '\\f' * 10 # (ten frames pass)
typing = '\\x1b[1;1:1C' + F + '\\x1b[1;1:3C' + F + '\\x1b[118;1:1u' + F[:10] + '\\x1b[118;1:3u' + F + '\\x1b[C'
r = subprocess.run([rinc, os.path.join(work, 'held') + '/', '-data', os.path.join(work, 'data'), '-frames', '45', '-type', typing],
                   capture_output=True, text=True, timeout=60)
out = (r.stdout + r.stderr).strip()
line = [l for l in out.split('\n') if l.startswith('right ')]
if not line: sys.exit('buttons held over serial: no line from the app in %r' % out)
w = line[0].split()
right, a, typed = int(w[1]), int(w[3]), w[5:]
if not 8 <= right <= 12: sys.exit('right held for the ten frames from its press to its release, not %d (%s)' % (right, line[0]))
if not 3 <= a <= 7: sys.exit('A (V) held for the five frames from its press to its release, not %d (%s)' % (a, line[0]))
if typed != ['129', '118', '129']: sys.exit('each press typed (right, v), each release not, and a plain right typed: %s' % line[0])
print('buttons held over serial, from press to release')
