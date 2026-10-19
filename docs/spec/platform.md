# What a Rinc is

This part says what every Rinc provides, so an app that runs on one runs the same on all of them: the
Fruit Jam, the desktop and the web today, and any other Rinc someone makes. Where this implementation
doesn't match yet, the page says so.

A Rinc provides:

- **[Rio](https://contextfreeinfo.github.io/rio/)**, the language apps are written in, at the version
  in `lib/rio`.
- **The `pc` module:** [`src/core/pc.rio`](https://github.com/rincpc/rinc/blob/main/src/core/pc.rio),
  built into Rinc and compiled with each app (and in the console): its host procs, with exactly those
  names and types, and the types they use, such as `pc.Pointer`, visible in every module without an
  import. Every host proc this Rinc registers must be declared there, with matching types, or nothing
  compiles; the tests check it too.
- **[Running apps](running.md):** compiling one, then its frames, 60 a second.
- **[The screen](screen.md):** 320x240, a 256-color palette, and a 5x8 font.
- **[Input](input.md):** buttons, a controller's sticks and triggers, typed text and time.
- **[Sound](sound.md):** 8 voices playing from the app's memory.
- **[Memory](memory.md):** 256 KB for each app, and nothing allocated while it runs.
- **[Apps and storage](apps.md):** apps, home, the console and the system key.
- **[Rio inside an app](rio.md):** a session of Rio that an app runs, for a console or an editor.

A Rinc can have more than this, such as a debugger or a volume knob, but an app can't depend on it.
