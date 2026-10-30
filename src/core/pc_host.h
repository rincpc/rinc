/* Made by rio_hostHeader (rio -H) from the Rio module that declares these host procs: don't edit.
   A struct parameter comes as 2 words: rio_getStruct(vm, a + i, &s, sizeof s) copies it into s.
   Register the module's source with its procs (_SOURCE, below: rio_addSource), and each proc with
   its _SIG (_HOST_PROCS lists them all). */
#include <stdint.h>
#include "rio.h"

typedef struct {
  int32_t x;
  int32_t y;
  int32_t sx;
  int32_t sy;
  int32_t w;
  int32_t h;
  int32_t scale;
} pc_Blit;
typedef char pc_Blit_size[sizeof(pc_Blit) == 28 ? 1 : -1]; /* (Rio's size) */
#define pc_Blit_LAYOUT "{x:Int,y:Int,sx:Int,sy:Int,w:Int,h:Int,scale:Int}"

typedef struct {
  int32_t bits;
  int32_t channels;
  float rate;
} pc_Sound;
typedef char pc_Sound_size[sizeof(pc_Sound) == 12 ? 1 : -1]; /* (Rio's size) */
#define pc_Sound_LAYOUT "{bits:Int,channels:Int,rate:Float}"

typedef struct {
  int32_t line;
  int32_t col;
  int32_t len;
  int32_t runtime;
  int32_t pathLen;
  int32_t messageLen;
} pc_AppError;
typedef char pc_AppError_size[sizeof(pc_AppError) == 24 ? 1 : -1]; /* (Rio's size) */
#define pc_AppError_LAYOUT "{line:Int,col:Int,len:Int,runtime:Int,pathLen:Int,messageLen:Int}"

#define pc_cls_SIG "i"
#define pc_pset_SIG "iii"
#define pc_pget_SIG "ii>i"
#define pc_rect_SIG "iiiii"
#define pc_blit_SIG "s" pc_Blit_LAYOUT
#define pc_print_SIG "siii"
#define pc_btn_SIG "i>i"
#define pc_key_SIG ">i"
#define pc_stick_SIG "i>f"
#define pc_trigger_SIG "i>f"
#define pc_time_SIG ">f"
#define pc_pointer_SIG "Enum{game,ui,none}"
#define pc_pointerX_SIG ">i"
#define pc_pointerY_SIG ">i"
#define pc_pointerDown_SIG "i>i"
#define pc_pointerShown_SIG ">i"
#define pc_scroll_SIG "i>i"
#define pc_fromHex_SIG "bs>i"
#define pc_sound_SIG "ib" pc_Sound_LAYOUT
#define pc_soundWav_SIG "ib>i"
#define pc_soundRate_SIG "if"
#define pc_soundVolume_SIG "if"
#define pc_soundPan_SIG "if"
#define pc_soundLoop_SIG "iii"
#define pc_soundStop_SIG "i"
#define pc_soundPlaying_SIG "i>i"
#define pc_synth_SIG "ibi"
#define pc_synthMake_SIG "bbi>i"
#define pc_synthRate_SIG "if"
#define pc_synthBend_SIG "if"
#define pc_synthEnvelope_SIG "iff"
#define pc_synthSmooth_SIG "ii"
#define pc_synthWave_SIG "ib"
#define pc_soundSeek_SIG "ii"
#define pc_apps_SIG ">i"
#define pc_appName_SIG "ib>i"
#define pc_appInfo_SIG "ib>i"
#define pc_launch_SIG "s"
#define pc_call_SIG "ss"
#define pc_args_SIG "b>i"
#define pc_done_SIG "s"
#define pc_result_SIG "b>i"
#define pc_appError_SIG "bb>" pc_AppError_LAYOUT
#define pc_appSource_SIG "sb>i"
#define pc_appSave_SIG "ss>i"
#define pc_appDelete_SIG "s>i"
#define pc_appInstall_SIG "s>i"
#define pc_appCopy_SIG "ss>i"
#define pc_appRename_SIG "ss>i"
#define pc_dir_SIG "sb>i"
#define pc_fileRead_SIG "sb>i"
#define pc_fileWrite_SIG "ss>i"
#define pc_fileDelete_SIG "s>i"
#define pc_fileCopy_SIG "ss>i"
#define pc_fileRename_SIG "ss>i"
#define pc_dirMake_SIG "s>i"
#define pc_pngRead_SIG "sb>i"
#define pc_pngWrite_SIG "sss>i"
#define pc_pngNotes_SIG "sb>i"
#define pc_helps_SIG "b>i"
#define pc_helpRead_SIG "sb>i"
#define pc_help_SIG "s"
#define pc_helpTopic_SIG "b>i"
#define pc_saveRead_SIG "sb>i"
#define pc_saveWrite_SIG "ss>i"
#define pc_saveDelete_SIG "s>i"
#define pc_stashWrite_SIG "ss>i"
#define pc_stashRead_SIG "sb>i"
#define pc_stashDelete_SIG "s>i"
#define pc_settingRead_SIG "sb>i"
#define pc_settingWrite_SIG "ss>i"
#define pc_theme_SIG "Enum{bg,bar,text,soft,dim,faint,accent,heading,pick,select,link,error,ok,codeBg,code,punct,keyword,type,call,number,string,comment}>i"
#define pc_boot_SIG ">i"
#define pc_rioStart_SIG "bb>i"
#define pc_rioEval_SIG "s>i"
#define pc_rioFrame_SIG ">i"
#define pc_rioType_SIG "i"
#define pc_rioError_SIG "b>i"
#define pc_rioLog_SIG "b>i"
#define pc_rioIndent_SIG "s>i"

#define pc_HOST_PROCS(X) \
  X(cls, pc_cls_SIG) \
  X(pset, pc_pset_SIG) \
  X(pget, pc_pget_SIG) \
  X(rect, pc_rect_SIG) \
  X(blit, pc_blit_SIG) \
  X(print, pc_print_SIG) \
  X(btn, pc_btn_SIG) \
  X(key, pc_key_SIG) \
  X(stick, pc_stick_SIG) \
  X(trigger, pc_trigger_SIG) \
  X(time, pc_time_SIG) \
  X(pointer, pc_pointer_SIG) \
  X(pointerX, pc_pointerX_SIG) \
  X(pointerY, pc_pointerY_SIG) \
  X(pointerDown, pc_pointerDown_SIG) \
  X(pointerShown, pc_pointerShown_SIG) \
  X(scroll, pc_scroll_SIG) \
  X(fromHex, pc_fromHex_SIG) \
  X(sound, pc_sound_SIG) \
  X(soundWav, pc_soundWav_SIG) \
  X(soundRate, pc_soundRate_SIG) \
  X(soundVolume, pc_soundVolume_SIG) \
  X(soundPan, pc_soundPan_SIG) \
  X(soundLoop, pc_soundLoop_SIG) \
  X(soundStop, pc_soundStop_SIG) \
  X(soundPlaying, pc_soundPlaying_SIG) \
  X(synth, pc_synth_SIG) \
  X(synthMake, pc_synthMake_SIG) \
  X(synthRate, pc_synthRate_SIG) \
  X(synthBend, pc_synthBend_SIG) \
  X(synthEnvelope, pc_synthEnvelope_SIG) \
  X(synthSmooth, pc_synthSmooth_SIG) \
  X(synthWave, pc_synthWave_SIG) \
  X(soundSeek, pc_soundSeek_SIG) \
  X(apps, pc_apps_SIG) \
  X(appName, pc_appName_SIG) \
  X(appInfo, pc_appInfo_SIG) \
  X(launch, pc_launch_SIG) \
  X(call, pc_call_SIG) \
  X(args, pc_args_SIG) \
  X(done, pc_done_SIG) \
  X(result, pc_result_SIG) \
  X(appError, pc_appError_SIG) \
  X(appSource, pc_appSource_SIG) \
  X(appSave, pc_appSave_SIG) \
  X(appDelete, pc_appDelete_SIG) \
  X(appInstall, pc_appInstall_SIG) \
  X(appCopy, pc_appCopy_SIG) \
  X(appRename, pc_appRename_SIG) \
  X(dir, pc_dir_SIG) \
  X(fileRead, pc_fileRead_SIG) \
  X(fileWrite, pc_fileWrite_SIG) \
  X(fileDelete, pc_fileDelete_SIG) \
  X(fileCopy, pc_fileCopy_SIG) \
  X(fileRename, pc_fileRename_SIG) \
  X(dirMake, pc_dirMake_SIG) \
  X(pngRead, pc_pngRead_SIG) \
  X(pngWrite, pc_pngWrite_SIG) \
  X(pngNotes, pc_pngNotes_SIG) \
  X(helps, pc_helps_SIG) \
  X(helpRead, pc_helpRead_SIG) \
  X(help, pc_help_SIG) \
  X(helpTopic, pc_helpTopic_SIG) \
  X(saveRead, pc_saveRead_SIG) \
  X(saveWrite, pc_saveWrite_SIG) \
  X(saveDelete, pc_saveDelete_SIG) \
  X(stashWrite, pc_stashWrite_SIG) \
  X(stashRead, pc_stashRead_SIG) \
  X(stashDelete, pc_stashDelete_SIG) \
  X(settingRead, pc_settingRead_SIG) \
  X(settingWrite, pc_settingWrite_SIG) \
  X(theme, pc_theme_SIG) \
  X(boot, pc_boot_SIG) \
  X(rioStart, pc_rioStart_SIG) \
  X(rioEval, pc_rioEval_SIG) \
  X(rioFrame, pc_rioFrame_SIG) \
  X(rioType, pc_rioType_SIG) \
  X(rioError, pc_rioError_SIG) \
  X(rioLog, pc_rioLog_SIG) \
  X(rioIndent, pc_rioIndent_SIG)

#define pc_HASH 0x68a79117u /* (rio_hashText of the source) */
#define pc_SOURCE \
  "# pc.rio: Rinc's host procs, the pc module. Apps don't import it: pc is there everywhere (pc.cls(0)).\n" \
  "# This is what pc is, written in Rio, and built into Rinc: each app compiles it first, against what\n" \
  "# rinc_start registers, so each declaration's types are checked, every host proc must be declared here,\n" \
  "# and the types they use (pc.Pointer) are here too. (Compiled images keep it, as part of the app.)\n" \
  "\n" \
  "# ---------------------------------------------------------------- the screen: 320x240, palette indexes\n" \
  "\n" \
  "cls* :: host proc(c: Int)                           # fill the screen with color c\n" \
  "pset* :: host proc(x, y, c: Int)                    # one pixel\n" \
  "pget* :: host proc(x, y: Int) -> Int\n" \
  "rect* :: host proc(x, y, w, h, c: Int)              # a filled rectangle\n" \
  "# where blit draws a picture, and which part of it; left out, the whole picture at its size\n" \
  "Blit* :: struct\n" \
  "  x, y: Int     # where its top left goes\n" \
  "  sx, sy: Int   # the part's top left, in the picture\n" \
  "  w, h: Int     # the part's size (0: the rest of the picture)\n" \
  "  scale: Int    # each pixel scale by scale (0: 1)\n" \
  "end\n" \
  "# a picture (its width and height, 2 bytes each, low first, then its pixels: as pngRead gives it), or\n" \
  "# part of it, as b says; color 0 is see-through\n" \
  "blit* :: host proc(picture: String, b: Blit)\n" \
  "print* :: host proc(s: String, x, y, c: Int)        # text, in the 5x8 font, 6 pixels a character\n" \
  "\n" \
  "# ---------------------------------------------------------------- input\n" \
  "\n" \
  "# 1 while button i is held: 0 left, 1 right, 2 up, 3 down (arrows), then the face buttons, as on an Xbox\n" \
  "# controller: 4 A (V key), 5 B (C), 6 X (X), 7 Y (Z); then a controller's 8 LB, 9 RB, 10 and 11 the\n" \
  "# sticks pressed in, 12 Start, 13 Back\n" \
  "btn* :: host proc(i: Int) -> Int\n" \
  "# the next key typed, or -1: 32-126 as typed, 13 Enter, 8 Backspace, 9 Tab, 27 Esc, Ctrl+letter as 1-26,\n" \
  "# and 128 left, 129 right, 130 up, 131 down, 132 Home, 133 End, 134 Delete, 135 Page Up, 136 Page Down;\n" \
  "# a key that isn't a character with the modifiers held added on: 256 Shift, 512 Ctrl, 1024 Alt\n" \
  "key* :: host proc() -> Int\n" \
  "# a controller's sticks: 0 left x, 1 left y, 2 right x, 3 right y; -1 to 1, + right and down\n" \
  "stick* :: host proc(i: Int) -> Float\n" \
  "trigger* :: host proc(i: Int) -> Float              # 0 left, 1 right; 0 to 1\n" \
  "time* :: host proc() -> Float                       # seconds since start\n" \
  "# the pointer, which Rinc draws while it's shown: a mouse moving it or pressing a button shows it; typing,\n" \
  "# the arrows (the d-pad's too) and a controller's center buttons hide it. What it's for, pointer(mode):\n" \
  "Pointer* :: enum\n" \
  "  game  # (each app starts so) a mouse's only: the left stick is still the arrows\n" \
  "  ui    # a controller's too: its left stick moves it, its right one scrolls, A and B are its buttons\n" \
  "  none  # never shown\n" \
  "end\n" \
  "pointer* :: host proc(mode: Pointer)\n" \
  "pointerX* :: host proc() -> Int                     # where it points, in screen pixels\n" \
  "pointerY* :: host proc() -> Int\n" \
  "pointerDown* :: host proc(i: Int) -> Int            # 1 while button i is held: 0 primary, 1 secondary, 2 middle\n" \
  "pointerShown* :: host proc() -> Int\n" \
  "# the whole steps scrolled since it was last asked: 0 across, + right; 1 up and down, + up\n" \
  "scroll* :: host proc(i: Int) -> Int\n" \
  "\n" \
  "# ---------------------------------------------------------------- data\n" \
  "\n" \
  "# bytes from hex digits into out (anything else in the text is skipped), until it's full: how many\n" \
  "fromHex* :: host proc(out: Blob, text: String) -> Int\n" \
  "\n" \
  "# ---------------------------------------------------------------- sound: 8 voices, 0 to 7\n" \
  "\n" \
  "# how sound reads its data; left out, 16-bit mono at 16000 frames a second\n" \
  "Sound* :: struct\n" \
  "  bits: Int       # 16 or 8 (signed), or 4 for IMA ADPCM (0: 16)\n" \
  "  channels: Int   # 1 or 2 (0: 1)\n" \
  "  rate: Float     # frames a second (0: 16000)\n" \
  "end\n" \
  "sound* :: host proc(v: Int, data: Blob, s: Sound) # play data on voice v, once, as s says\n" \
  "soundWav* :: host proc(v: Int, data: Blob) -> Int   # a WAV file's sound, at its own rate: 1 if it could\n" \
  "soundRate* :: host proc(v: Int, rate: Float)\n" \
  "soundVolume* :: host proc(v: Int, volume: Float)    # 0 to 1\n" \
  "soundPan* :: host proc(v: Int, pan: Float)          # -1 to 1\n" \
  "soundLoop* :: host proc(v, start, stop: Int)        # from frame stop, back to start (stop 0: no loop)\n" \
  "soundStop* :: host proc(v: Int)                     # (on the synth, fading out, as after its last note)\n" \
  "soundPlaying* :: host proc(v: Int) -> Int\n" \
  "# the synth: play notes on voice v, 4 bytes each: pitch (0 is C0, 65.4 Hz; a semitone a step), wave (0\n" \
  "# triangle, tilted, saw, square, pulse, organ, noise, 7 phaser; 8 to 15 the app's, from synthWave),\n" \
  "# volume (0 to 7) and effect (0 none, slide, vibrato, drop, fade in, fade out, fast arpeggio, 7 slow),\n" \
  "# each speed 120ths of a second. The sound procs above work on it too: its loop's start and stop are\n" \
  "# notes\n" \
  "synth* :: host proc(v: Int, notes: Blob, speed: Int)\n" \
  "# the notes made into out instead, as 16-bit samples at voice -1's rate (16000 a second unless set), to\n" \
  "# their end or out's: how many (speed 0: on from where the last one stopped, for more than out holds)\n" \
  "synthMake* :: host proc(out: Blob, notes: Blob, speed: Int) -> Int\n" \
  "# a voice's synth settings, kept from one synth to the next, and heard at once if it's playing (voice\n" \
  "# -1: synthMake's): samples made a second (8000 to 48000; 16000 to start), every pitch moved (in\n" \
  "# semitones), seconds the volume takes to rise and fall about 2/3 of the way to a note's (0.0062 to\n" \
  "# start), and the saw, square and pulse smoothed where they jump (1) or not (0, to start)\n" \
  "synthRate* :: host proc(v: Int, rate: Float)\n" \
  "synthBend* :: host proc(v: Int, semitones: Float)\n" \
  "synthEnvelope* :: host proc(v: Int, attack, release: Float)\n" \
  "synthSmooth* :: host proc(v, on: Int)\n" \
  "# wave w (8 to 15) for every voice: a cycle of 16-bit samples, read as it plays (so keep it in globals)\n" \
  "synthWave* :: host proc(w: Int, cycle: Blob)\n" \
  "# on from frame at (on the synth, note at), as it plays\n" \
  "soundSeek* :: host proc(v, at: Int)\n" \
  "\n" \
  "# ---------------------------------------------------------------- apps, for home and the console\n" \
  "\n" \
  "# A name is an installed app's (chord), or with a + after it, its project's (chord+): the app in work,\n" \
  "# with a save space of its own\n" \
  "apps* :: host proc() -> Int                         # the apps and projects (sorted, each project after its app): how many\n" \
  "appName* :: host proc(i: Int, out: Blob) -> Int     # app i's name (a project's with its +), into out: how long\n" \
  "appInfo* :: host proc(i: Int, out: Blob) -> Int     # its first line: \"Title: what it does\"\n" \
  "launch* :: host proc(name: String)                  # switch to an app or project (or \"home\", \"console\") after this frame\n" \
  "# one app calling another: call starts it with args (after this frame), which it reads with args; done goes\n" \
  "# back to the caller (this app forgotten, as the system key's Q does), which, starting again, reads result\n" \
  "call* :: host proc(name: String, args: String)\n" \
  "args* :: host proc(out: Blob) -> Int                # the args this app was called with: how long (0: none)\n" \
  "done* :: host proc(result: String)                  # back to the caller, with a result (or to the app before)\n" \
  "result* :: host proc(out: Blob) -> Int              # the result of the app this one called: how long, or -1 (taken)\n" \
  "# where the last app to stop with an error stopped, for an editor to go to: its file's path into path,\n" \
  "# and what was wrong into message (as much as fits). Taken: a second call gives none\n" \
  "AppError* :: struct\n" \
  "  line: Int             # from 1 (0: there's none)\n" \
  "  col: Int              # from 1 (0: not known, as for a runtime error)\n" \
  "  len: Int              # how wide what's wrong is (0: not known)\n" \
  "  runtime: Bool         # a runtime error (else it didn't compile)\n" \
  "  pathLen: Int          # how long the path and the message are\n" \
  "  messageLen: Int\n" \
  "end\n" \
  "appError* :: host proc(path: Blob, message: Blob) -> AppError\n" \
  "# an app's source into out, as much as fits: its whole length, or -1 if there's no such app\n" \
  "appSource* :: host proc(name: String, out: Blob) -> Int\n" \
  "appSave* :: host proc(name: String, text: String) -> Int   # 0, or -1\n" \
  "appDelete* :: host proc(name: String) -> Int               # all of its folder, its compiled image too: 0, or -1\n" \
  "appInstall* :: host proc(name: String) -> Int              # project name* installed as app name: 0, or -1\n" \
  "appCopy* :: host proc(from: String, to: String) -> Int     # app or project from, as new project to (to*): 0, or -1\n" \
  "appRename* :: host proc(from: String, to: String) -> Int   # project from (from*) as new project to (to*): 0, or -1\n" \
  "\n" \
  "# ---------------------------------------------------------------- files, by path, for a file browser\n" \
  "\n" \
  "# A path is absolute (/rinc/save/chord/best), in /rinc, the root: looking goes anywhere in it, changing\n" \
  "# only inside it\n" \
  "dir* :: host proc(path: String, out: Blob) -> Int          # its entries, folders first, a line each (\"d name\", \"f name\"): how many, or -1\n" \
  "fileRead* :: host proc(path: String, out: Blob) -> Int     # as much as fits: its whole length, or -1\n" \
  "fileWrite* :: host proc(path: String, data: String) -> Int # 0, or -1\n" \
  "fileDelete* :: host proc(path: String) -> Int              # a file, or a folder and all in it: 0, or -1\n" \
  "fileCopy* :: host proc(from: String, to: String) -> Int    # a file or a folder, to a path not taken: 0, or -1\n" \
  "fileRename* :: host proc(from: String, to: String) -> Int  # to a path not taken: 0, or -1\n" \
  "dirMake* :: host proc(path: String) -> Int                 # a folder, at a path not taken: 0, or -1\n" \
  "# pictures as PNG files: a picture is its width and height (2 bytes each, low first), then its pixels, a\n" \
  "# byte each, row by row (0: see-through). pngRead reads one into out, each color the palette's nearest\n" \
  "# (exactly, for one written by pngWrite): its size, or -1 (out needs room for the PNG's rows as they come\n" \
  "# too: w * h + 2 * h + 4 bytes for one of the palette's). pngWrite writes one as an indexed PNG, with the\n" \
  "# app's notes about it (text, up to 512 bytes, kept in the PNG: a sprite sheet's sprite size, say; \"\":\n" \
  "# none), and pngNotes reads them back\n" \
  "pngRead* :: host proc(path: String, out: Blob) -> Int\n" \
  "pngWrite* :: host proc(path: String, picture: String, notes: String) -> Int # 0, or -1\n" \
  "pngNotes* :: host proc(path: String, out: Blob) -> Int      # the notes, into out: how many bytes, or -1 (none)\n" \
  "\n" \
  "# ---------------------------------------------------------------- help texts, for a help app: /rinc/help, and the bundle's\n" \
  "\n" \
  "# the help texts (Markdown): their names (paths, .md left off: rinc/spec/apps; a project's, chord+/...),\n" \
  "# in order, into out, each with a 10 after it, as many as fit: how many\n" \
  "helps* :: host proc(out: Blob) -> Int\n" \
  "# a help text into out, as much as fits (storage's, else the bundle's): its whole length, or -1 if there's none\n" \
  "helpRead* :: host proc(name: String, out: Blob) -> Int\n" \
  "# the help app, after this frame, at a help text (and maybe a heading): help(\"rio/spec/statements#if\")\n" \
  "help* :: host proc(topic: String)\n" \
  "helpTopic* :: host proc(out: Blob) -> Int             # for the help app: the topic it was opened at, taken (0: none)\n" \
  "\n" \
  "# ---------------------------------------------------------------- this app's own files: /rinc/save/<app>\n" \
  "\n" \
  "# a file's contents into out, as much as fits: its whole length, or -1 if there's no such file. Names are\n" \
  "# 1 to 32 letters, digits, _, - and . (not first)\n" \
  "saveRead* :: host proc(file: String, out: Blob) -> Int\n" \
  "saveWrite* :: host proc(file: String, data: String) -> Int  # 0, or -1\n" \
  "saveDelete* :: host proc(file: String) -> Int               # 0, or -1\n" \
  "\n" \
  "# ---------------------------------------------------------------- this app's stash: /rinc/stash/<app>/<key>\n" \
  "\n" \
  "# what an app keeps of where it was, for its next start, under keys of its own (named as save files are),\n" \
  "# written as it goes; Rinc drops all of it on Sys+Q and Sys+R, where it never touches the save space\n" \
  "stashWrite* :: host proc(key: String, data: String) -> Int  # 0, or -1\n" \
  "stashRead* :: host proc(key: String, out: Blob) -> Int      # as much as fits: its whole length, or -1 for none\n" \
  "stashDelete* :: host proc(key: String) -> Int               # 0, or -1\n" \
  "# the whole Rinc's settings, shared by the apps (/rinc/settings/<key>): \"active\" is the app or project\n" \
  "# being worked on (chord, or chord+); keys are named as save files are\n" \
  "settingRead* :: host proc(key: String, out: Blob) -> Int      # as much as fits: its whole length, or -1\n" \
  "settingWrite* :: host proc(key: String, data: String) -> Int  # 0, or -1\n" \
  "\n" \
  "# the theme: a color for each role, which the system apps draw with, so they look alike and a theme (the\n" \
  "# theme setting: owner/id, a file owner/themes/id.theme.rio) changes them all. Read at each app's start\n" \
  "Role* :: enum\n" \
  "  bg       # the background\n" \
  "  bar      # bars (a title's, the keys' at the bottom), and boxes to type in\n" \
  "  text     # text\n" \
  "  soft     # long text, a little quieter (help's)\n" \
  "  dim      # what goes with text: descriptions, line numbers, notes\n" \
  "  faint    # hints: the keys at the bottom, an empty box's placeholder\n" \
  "  accent   # what stands out: the main heading, the cursor, marks\n" \
  "  heading  # other headings, emphasis\n" \
  "  pick     # behind the row or item picked\n" \
  "  select   # behind selected text\n" \
  "  link     # links\n" \
  "  error    # what went wrong\n" \
  "  ok       # what went right\n" \
  "  codeBg   # behind code, among text\n" \
  "  code     # code: names\n" \
  "  punct    # code: operators and punctuation\n" \
  "  keyword  # code: if, for, proc...\n" \
  "  type     # code: a name with a capital (Int, Blob)\n" \
  "  call     # code: a name called, f(...)\n" \
  "  number   # code: 12, 1.5\n" \
  "  string   # code: \"text\"\n" \
  "  comment  # code: # comments\n" \
  "end\n" \
  "theme* :: host proc(role: Role) -> Int\n" \
  "boot* :: host proc() -> Int                                 # how many times this Rinc has started (0: no storage)\n" \
  "\n" \
  "# ---------------------------------------------------------------- Rio inside an app: a session\n" \
  "\n" \
  "# a fresh session, for a console or an editor: its code and data in mem, its compiler's tables in\n" \
  "# tables, both the app's own memory, handed over: 0, or -1\n" \
  "rioStart* :: host proc(mem, tables: Blob) -> Int\n" \
  "rioEval* :: host proc(text: String) -> Int          # run a piece: 0 done, 1 unfinished (needs more), -1 error\n" \
  "rioFrame* :: host proc() -> Int                     # its update then draw: 0, 1 if it has neither, -1 error\n" \
  "rioType* :: host proc(key: Int)                     # a key for its pc.key()\n" \
  "rioError* :: host proc(out: Blob) -> Int            # the last error, as text: how long\n" \
  "rioLog* :: host proc(out: Blob) -> Int              # what it logged since, taken: how long (lines end with 10)\n" \
  "rioIndent* :: host proc(text: String) -> Int        # the indent steps an unfinished input's next line takes\n"
