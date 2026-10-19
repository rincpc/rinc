"""Build Rinc's web players: Rinc's VM as wasm (built with zig cc; no emscripten), for rinc -web to build
in (RINC_WEB_WASM, RINC_WEB_WASM_FULL), which makes the pages: a small JS host (src/web/page.html), a
player and a bundle of apps, all inline.

  python tools/web.py --wasm=rinc.wasm [--wasm-full=rinc-full.wasm] [--rio=path/to/rio]

The VM-only player runs apps compiled, as images: about a quarter the size of the full one, which has
the compiler too, for apps that run Rio themselves (pc.rio*, as the console does) and apps from storage.
"""
import os, subprocess, sys, tempfile, time

opts = dict(a[2:].split('=', 1) for a in sys.argv[1:] if a.startswith('--') and '=' in a)
if not opts.keys() & {'wasm', 'wasm-full'}: sys.exit(__doc__.strip())
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
rio = opts.get('rio', os.path.join(root, 'lib', 'rio'))

def build_wasm(path, full):
  # rio's core: every source but its CLI (main.c), its C backend (aot.c) and the ARM-only jit.c (with
  # rio_NO_COMPILER, the compiler's files compile to nothing)
  rio_core = sorted(f for f in os.listdir(os.path.join(rio, 'src')) if f.endswith('.c') and f not in ('main.c', 'aot.c', 'jit.c'))
  # the build's id (rinc_buildId, as tools/buildid.cmake makes it): another build's cached app images go
  build_id = os.path.join(tempfile.mkdtemp(), 'build_id.c')
  with open(build_id, 'w') as f: f.write('const char rinc_buildId[] = "%s";\n' % time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()))
  # pc.rio, built in (as tools/embed_text.cmake makes it): the pc module, compiled with each app
  pc_source = os.path.join(os.path.dirname(build_id), 'pc_source.c')
  with open(os.path.join(root, 'src', 'core', 'pc.rio'), 'rb') as f: pc = f.read()
  with open(pc_source, 'w') as f:
    f.write('const char rinc_pcSource[] = {%s0};\nconst int rinc_pcSource_len = %d;\n' % (''.join('%d,' % b for b in pc), len(pc)))
  sources = [os.path.join(root, 'src', 'web', 'main.c'), build_id] + ([pc_source] if full else []) + [os.path.join(root, 'src', 'core', f) for f in ('rinc.c', 'console.c', 'font.c', 'audio.c')] + [os.path.join(rio, 'src', f) for f in rio_core]
  # (rio_MAX_FFI: CMakeLists.txt's RINC_MAX_FFI)
  subprocess.run(['zig', 'cc', '-target', 'wasm32-wasi', '-mexec-model=reactor', '-Oz', '-Drio_SMALL', '-Drio_MAX_FFI=96'] + ([] if full else ['-Drio_NO_COMPILER']) +
                 ['-I' + os.path.join(rio, 'src'), '-I' + os.path.join(root, 'src', 'core'), '-Wl,--strip-all', '-o', path] + sources, check=True)
  print('%s: %d bytes' % (path, os.path.getsize(path)))

if 'wasm' in opts: build_wasm(opts['wasm'], False)
if 'wasm-full' in opts: build_wasm(opts['wasm-full'], True)
