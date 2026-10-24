# Themes

Themes (`system/themes/themes.rio`, a system app) picks the colors the system apps share: home, the
console, the editor, help, files and Themes itself. A theme gives a color to each **role** (the
background, text, the row picked, code's keywords and strings, and so on), so the apps look alike, and
code is colored the same everywhere. A new theme shows in each app as it starts.

## Picking one

The list has every theme, each with a few of its colors and its title, and the one picked shown at
the right: a little of home, of code and of help's text. The one in use has a mark at its left.

| key | |
|---|---|
| Up and down | pick a theme |
| Enter (or a click on the one picked) | use it |
| E | edit it |
| N | a new theme, from this one: type its name |
| R | rename one of yours |
| Delete | delete one of yours (not the one in use) |

## Editing one

Each role, with its color and its number, and the theme at the right as it changes:

| key | |
|---|---|
| Up and down | pick a role |
| Enter (or a click on the role picked) | Colors, to pick its color |
| Left and right | the color before or after |
| Esc | back to the list |

Each change is saved as it's made. Rinc's own themes, and any installed app's, are locked: editing one
asks for a name and makes a copy of it, yours, to change.

## Where themes are

A theme is a file, `<name>.theme.rio`, in an app's `themes` folder: Themes' own are
`/rinc/apps/themes/themes/`, the ones you make `/rinc/save/themes/themes/`, and a project's
`/rinc/projects/<app>/themes/`, so an app or a project can bring themes of its own. A theme is named
for where it is, `themes/slate` or `chord*/sunset`, and the `theme` setting holds the name of the one
in use (none: Rinc's own colors, `themes/rinc`).

The file is Rio constants, and Rinc reads only those, never running it. Its first line is its title:

```rio fragment
# Slate: warm slate gray, with yellow and blue
bg*      :: 5    # dark gray
text*    :: 7    # white
accent*  :: 10   # yellow
```

A role a theme leaves out has Rinc's own color. The roles, as `pc.Role` lists them: `bg`, `bar`, `text`,
`soft`, `dim`, `faint`, `accent`, `heading`, `pick`, `select`, `link`, `error`, `ok`, `codeBg`, and for
code `code`, `punct`, `keyword`, `type`, `call`, `number`, `string` and `comment`. An app of your own can
use them too: `pc.theme(pc.Role.text)` is the text's color in the theme in use.
