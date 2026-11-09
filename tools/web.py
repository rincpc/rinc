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
  # rio's core: every source but its CLI's (main.c, its C backend aot.c, its C header writer header.c)
  # and the ARM-only jit.c (with rio_NO_COMPILER, the compiler's files compile to nothing)
  rio_core = sorted(f for f in os.listdir(os.path.join(rio, 'src')) if f.endswith('.c') and f not in ('main.c', 'aot.c', 'header.c', 'jit.c'))
  # the build's id (rinc_buildId, as tools/buildid.cmake makes it): another build's cached app images go
  build_id = os.path.join(tempfile.mkdtemp(), 'build_id.c')
  with open(build_id, 'w') as f: f.write('const char rinc_buildId[] = "%s";\n' % time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()))
  # Rinc's library, built in (as tools/library.cmake makes it): only the player with the compiler needs it,
  # for an app's import, but both have it (an empty table where nothing compiles)
  library = os.path.join(os.path.dirname(build_id), 'library.c')
  with open(library, 'w') as f:
    libs = sorted(n for n in os.listdir(os.path.join(root, 'library')) if n.endswith('.rio')) if full else []
    f.write('#include "rinc.h"\n')
    for i, n in enumerate(libs):
      data = open(os.path.join(root, 'library', n), 'rb').read()
      f.write('static const char rinc_lib%d[] = {%s0};\n' % (i, ''.join('%d,' % b for b in data)))
    f.write('const rinc_File rinc_library[] = {\n%s  {0, 0, 0}};\n' % ''.join('  {"%s", rinc_lib%d, %d},\n' % (n, i, len(open(os.path.join(root, 'library', n), 'rb').read())) for i, n in enumerate(libs)))
  # (pc.rio, the pc module, is built in through src/core/pc_host.h: only the player with the compiler keeps it)
  sources = [os.path.join(root, 'src', 'web', 'main.c'), build_id, library] + [os.path.join(root, 'src', 'core', f) for f in ('rinc.c', 'console.c', 'font.c', 'audio.c', 'png.c', 'zip.c')] + [os.path.join(rio, 'src', f) for f in rio_core]
  # (rio_MAX_FFI: CMakeLists.txt's RINC_MAX_FFI)
  subprocess.run(['zig', 'cc', '-target', 'wasm32-wasi', '-mexec-model=reactor', '-Oz', '-Drio_SMALL', '-Drio_MAX_FFI=104'] + ([] if full else ['-Drio_NO_COMPILER']) +
                 ['-I' + os.path.join(rio, 'src'), '-I' + os.path.join(root, 'src', 'core'), '-Wl,--strip-all', '-o', path] + sources, check=True)
  print('%s: %d bytes' % (path, os.path.getsize(path)))

if 'wasm' in opts: build_wasm(opts['wasm'], False)
if 'wasm-full' in opts: build_wasm(opts['wasm-full'], True)
