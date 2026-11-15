"""The console's load: an app's source run in the console's session, its names there after, its relative
paths from its own folder (pc.rioStart's app), as when it runs. python consoleload.py <rinc> <console's
folder> <work folder>
- a big app (about 2,500 instructions): the session's room for code is the size of its memory, not of its
  compiler's tables (which once gave it 1,440)
- an app of parts: an included part (parts/more.rio), a module of its own (import .helper), a library
  module (import tiny, from the card's /rinc/lib: Rinc's own, as tilemap, need more room than a session
  has) and a file it reads by its path in its folder (words.txt), each there; and the same as
  a project (its folder in /rinc/projects, loaded as name+)
- chord (Rinc's), whose parts and strings need the session's room past 40 KB (the console's 48 KB)"""
import os, shutil, subprocess, sys
rinc, console, work = sys.argv[1], sys.argv[2], sys.argv[3]
failed = 0
def put(path, text):
  os.makedirs(os.path.dirname(os.path.join(work, path)), exist_ok=True)
  open(os.path.join(work, path), 'w', newline='\n').write(text)
def typed(text):
  return subprocess.run([rinc, '-data', work, '-app', 'console', '-frames', '120', '-type', text],
                        capture_output=True, text=True, timeout=60).stdout.replace('\r', '')
def check(ok, what, got):
  global failed
  print(('console load: ' if ok else 'FAIL console load: ') + what + ('' if ok else ': ' + got[-500:]))
  failed += not ok
shutil.rmtree(work, ignore_errors=True)
subprocess.run([rinc, '-data', work, '-install', console], capture_output=True, timeout=60)

lines = ['total := 0'] + ['total = (total * 3 + 7) % 1000'] * 591
put('apps/big/big.rio', '\n'.join(lines) + '\n')
want = 0
for i in range(591):
  want = (want * 3 + 7) % 1000
got = typed('load big\\ntotal\\n')
check('loaded big' in got and '\n%d\n' % want in got, 'a big app loaded, and its names there (total %d)' % want, got)

put('lib/tiny.rio', 'LIMIT* :: 4095\n')
app = ('include "parts/more.rio"\nimport .helper\nimport tiny\nwords: Blob[..32]\n'
       'n := pc.fileRead("words.txt", words)\n')
for folder in ('apps/parted', 'projects/kept'):
  name = os.path.basename(folder)
  put(folder + '/%s.rio' % name, app)
  put(folder + '/parts/more.rio', 'more := 40 + 2\n')
  put(folder + '/helper.rio', 'twice* :: proc(x: Int) -> Int do return x * 2\n')
  put(folder + '/words.txt', 'hello there')
ask = 'more\\nhelper.twice(21)\\ntiny.LIMIT\\nwords.asString()\\n'
got = typed('load parted\\n' + ask)
check('loaded parted' in got and got.count('\n42\n') >= 2 and '\n4095\n' in got and 'hello there' in got,
      'an app of parts: its part, its module, the library, its file', got)
got = typed('load kept+\\n' + ask)
check('loaded kept+' in got and got.count('\n42\n') >= 2 and '\n4095\n' in got and 'hello there' in got,
      'a project of parts (kept+): from its folder in projects', got)
chord = os.path.join(os.path.dirname(console), '..', 'apps', 'chord') # (Rinc's: its parts, and strings
subprocess.run([rinc, '-data', work, '-install', chord], capture_output=True, timeout=60) # that need the room past 40 KB)
got = typed('load chord\\nwaveNames[2]\\n')
check('loaded chord' in got and '\nSAW\n' in got, 'chord, a real app of parts, its strings room enough', got)

typed('123 * 7\\n') # (the lines typed, kept in its stash: up brings them back after another app's run, or a new start)
got = typed('\\x1b[A\\n')
check('\n861\n' in got, 'a line typed before, brought back by up in a fresh start', got)
typed(''.join('500 + %d\\n' % k for k in range(20))) # (more than 16 lines: all kept, in its save)
got = typed('\\x1b[A' * 20 + '\\n')
check('\n500\n' in got and os.path.exists(os.path.join(work, 'save', 'console', 'history.txt')), '20 lines kept (in /rinc/save/console/history.txt): the 20th back brought back', got)
got = typed('history\\n')
check('500 + 19' in got and '500 + 2\n' in got and '500 + 1\n' not in got, 'history: the last 20 lines typed (500 + 0 again, and history itself, the last two)', got)
typed('history clear\\n')
got = typed('\\x1b[A\\x1b[A\\n')
check('\n519\n' not in got and '\n518\n' not in got, 'history clear: up brings back nothing typed before it', got)
shutil.rmtree(work, ignore_errors=True)
if failed: sys.exit(1)
print('console load: all came through')
