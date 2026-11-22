# The web

```sh
rinc apps/breakout.rio -web breakout.html
```

makes one self-contained page: a bundle of the app compiled to an image, Rinc's VM as wasm (gzipped,
and unzipped by the browser), and a small JS host (`src/web/page.html`). Breakout comes to 94 KB, most of
it the 54 KB player, in base64. Open it from disk or any server. The arrows and V, C, X and
Z are the buttons (in a game: in a pointer app the letters type), and any controller the browser maps to the standard layout is player 1. Sound starts
with the first key or click, as browsers require.

A page can carry several apps, as an exported program does (see [the desktop](desktop.md)):

```sh
rinc system/home system/console system/edit system/files system/help system/colors system/themes \
  apps/*.rio apps/chord -home home -name rinc -files apps/rinc=system/rinc -web rinc.html
```

is the whole Rinc, as on the Fruit Jam: every system app, and the themes (`-files` carries a folder's
files, here the themes, which apps read as they would storage's). A page's apps and files have no
fixed limit (a big one is slower to open: it's all in the page). [Storage](../spec/apps.md#storage)
is the browser's IndexedDB, under the bundle's storage name, so what's saved stays in that browser, for pages from the
same site (or, from disk, the same folder), as much as the browser allows (localStorage's few MB, before; what a page
kept there moves in the first time). Each change is kept as it's made, and other tabs of the site hold
it too at once (the newest change wins). Ctrl and Shift tapped together, and then Esc, goes home; on
a controller it's the guide button and Back. The page keeps a lone Alt from the browser's menus, as Alt
tapped twice is Rinc's Alt lock.

A .zip dropped on such a page is plugged in as the stick, as on [the desktop](desktop.md): its files at
`/stick`, for Files to show, copy from and onto (the command list's "The stick: download it, as a zip"
saves it as a file; it's in the page's storage meanwhile, kept through a reload) (an app's folder copied to `/apps` is
installed, into the page's storage). A second one dropped replaces the first. Installed apps come before
the page's own of the same name. A page with only the VM-only player (below) can't run apps from storage,
so it says so and installs nothing.

```sh
rinc -name rinc-empty -web empty.html
```

is a Rinc with nothing installed: it says so, and installs a dropped bundle (no stick: there's no Files yet), as the desktop rinc does
with none. Its Reset button, in the corner, takes it back to empty (it asks first). The storage name,
which an empty page must have, keeps it apart from other pages from the same site: each keeps its
storage under its own.

## The two players

The desktop rinc carries two web players and gives a page the smaller one it can:

- **VM-only** (54 KB gzipped): runs compiled apps, the common case.
- **With the compiler** (114 KB gzipped): for a page with an app that runs Rio itself
  ([`pc.rio*`](../spec/rio.md), as the console does) or writes apps (`pc.appSave`, as the editor does).
  It also runs apps saved to storage, from source, and such a page carries its apps as source, so the
  editor can open them.

They're built in when the desktop rinc is configured with `-DRINC_WEB_WASM=rinc.wasm` and
`-DRINC_WEB_WASM_FULL=rinc-full.wasm`, which the CI does for the releases. Making them takes zig
(`zig cc`, no emscripten): `python tools/web.py --wasm=rinc.wasm --wasm-full=rinc-full.wasm`.

The [demo site](https://rincpc.com/) is made this way from `apps/` on each push to `main`, with a page
for each app, one of the whole Rinc, and these docs at [rincpc.com/docs](https://rincpc.com/docs/). `tools/demosite.py` makes
the pages (with Rinc's logo as their icon, as every page `rinc -web` makes has), and the demo_site test makes
the same ones and plays each.
