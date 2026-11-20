# Rinc's logo

A teal orb in a notched ring, drawn in Paint at 16 by 16 (`rinclogo-sheet.png`, a sheet of 16x16 sprites:
the logo, its silhouette, and darker versions of both).

| file | for |
|---|---|
| `rinc-logo.svg`, `rinc-logo-16.png` to `rinc-logo-512.png` | dark backgrounds (pale ring, see-through gaps) |
| `rinc-logo-dark.svg`, `rinc-logo-dark-16.png` to `-512.png` | light backgrounds |

The PNGs are the 16x16 drawing scaled up a whole number of times (each pixel a square), so they stay
crisp; the SVGs are a rectangle for each run of pixels, crisp at any size (`shape-rendering="crispEdges"`).
16 and 32 suit favicons.

For later, at higher resolution: the points toward the circle at the top and bottom could be pointier (16
pixels only allowed so much), and an outline may serve light backgrounds better than a dark version.
