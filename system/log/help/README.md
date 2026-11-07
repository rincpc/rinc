# Log

What an app logged lately, and what Rinc said about it: a runtime error, say, or a picture that needed
more room. The system key with O opens it at the log of the app you were in, and Esc goes back there.

| key | |
|---|---|
| Tab, Shift+Tab | the next app with a log, or the one before |
| Up, Down, Page Up, Page Down | move through it (the wheel too) |
| Home, End | its start, its end (where it opens: the newest) |
| Esc | back to the app |

Each app's log is the newest 4 KB of it, kept in the app's cache space (`/rinc/cache/<app>/log`), with a
`-- started` line each time the app starts. Lines that say "error" are in the error color.
