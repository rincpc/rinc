"""python home.py path/to/rinc home-folder data-folder: Home, the launcher, typed at: Page Down picks an app a
page further down, End the last, Home the first, Page Up a page back (13 rows to a page: 12 further); Enter
starts it, and the app started says which it is. (Built in, Rinc's own apps come after these, if any:
here there are none, just Home's own folder, which it doesn't list.)"""
import os, shutil, subprocess, sys

rinc, home, data = os.path.abspath(sys.argv[1]), sys.argv[2], sys.argv[3]
PAGE_UP, PAGE_DOWN, HOME, END, ENTER = '\\x1b[5~', '\\x1b[6~', '\\x1b[H', '\\x1b[F', '\\n'

shutil.rmtree(data, ignore_errors=True)
shutil.copytree(home, os.path.join(data, 'apps', 'home'))
for i in range(30): # (a00 to a29, in order of their titles)
  folder = os.path.join(data, 'apps', 'a%02d' % i)
  os.makedirs(folder)
  with open(os.path.join(folder, 'a%02d.rio' % i), 'w') as f: f.write('# A%02d: an app\nlog("started a%02d")\n' % (i, i))

def started(keys):
  out = subprocess.run([rinc, '-data', data, '-app', 'home', '-frames', '40', '-type', keys], capture_output=True, text=True, timeout=60).stdout
  return [l.split()[1] for l in out.split('\n') if l.startswith('started')]

failed = 0
for name, keys, want in [
  ('Enter: the first', ENTER, 'a00'),
  ('Page Down: a page further', PAGE_DOWN + ENTER, 'a12'),
  ('Page Down twice', PAGE_DOWN + PAGE_DOWN + ENTER, 'a24'),
  ('Page Down three times: the last, not round', PAGE_DOWN * 3 + ENTER, 'a29'),
  ('End: the last', END + ENTER, 'a29'),
  ('End, Page Up: a page back', END + PAGE_UP + ENTER, 'a17'),
  ('End, Home: the first', END + HOME + ENTER, 'a00'),
]:
  got = started(keys)
  if got != [want]:
    failed += 1
    print('FAIL %s: wanted %s, got %s' % (name, want, got))
  else:
    print('ok   %s' % name)
shutil.rmtree(data, ignore_errors=True)
print('%d failed' % failed if failed else 'home came through')
sys.exit(1 if failed else 0)
