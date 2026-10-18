# Help

Help (`system/help.rio`, built into the Fruit Jam's firmware) reads the help texts a Rinc has: Rinc's
docs and Rio's, these pages among them, and any app's own. It starts where it was last, or with the
contents: a list of the sets of texts, each by its own table of contents.

## Moving about

| keys | |
|---|---|
| up, down, Page Up, Page Down, Home, End | scroll |
| Tab, Shift+Tab | pick the next link, or the one before |
| Enter | follow the picked link: to another text, or a heading in one |
| Backspace, Esc, Alt+Left | back |
| Alt+Right | forward |
| Alt+Up | up: to a text's contents, then to all of them |
| Alt+Home, Ctrl+O | all the contents |

With a controller, the d-pad scrolls, LB and RB (or left and right) pick links, A follows a link and B
goes back. Links to web pages show where they go but don't open.

Help remembers the last 32 places it's been, back and forward, and where it was in each, through a
restart: that's its stash, which the system key with R drops, to start over.

## Help texts

A help text is Markdown: headings, paragraphs, lists, tables, code (in blocks and `inline`),
**strong** and *emphasized* text, and links. Rinc finds them in two kinds of place:

- **An app's own**, in its `help` folder: `/rinc/apps/chord/help/keys.md` is the text `chord/keys`.
  They come and go with the app, and are edited with it.
- **Those that are no app's**, in `/rinc/help`: `/rinc/help/rinc/spec/apps.md` is `rinc/spec/apps`.
  Rinc's docs are the set `rinc`, and Rio's `rio`.

A Rinc's bundle can carry help texts the same way (the Fruit Jam's carries Rinc's and Rio's docs), and
a text in storage takes the place of a bundled one of the same name. A set with a `SUMMARY` text is
listed in the contents by that, as these docs are.

## For apps

An app opens help at a text, and a heading in it, with `pc.help("rio/spec/statements#if")`. The rest of
what help uses is there for any app: [help texts](../spec/apps.md#help-texts) in the platform's spec.
