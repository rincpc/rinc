/* Made by rio_hostHeader (rio -H) from the Rio module that declares these host procs: don't edit.
   A struct parameter comes as 2 words: rio_getStruct(vm, a + i, &s, sizeof s) copies it into s.
   Register the module's source with its procs (_SOURCE, below: rio_addSource), and each proc with
   its _SIG (_HOST_PROCS lists them all). */
#include <stdint.h>
#include "rio.h"

typedef struct {
  rio_View pixels;
  int32_t size[2];
} pc_Image;
typedef char pc_Image_size[sizeof(pc_Image) == 16 ? 1 : -1]; /* (Rio's size) */
#define pc_Image_LAYOUT "{pixels:Blob,size:[2]Int}"

typedef struct {
  int32_t from[2];
  int32_t size[2];
  int32_t scale[2];
} pc_Blit;
typedef char pc_Blit_size[sizeof(pc_Blit) == 24 ? 1 : -1]; /* (Rio's size) */
#define pc_Blit_LAYOUT "{from:[2]Int,size:[2]Int,scale:[2]Int}"

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
} pc_AppError;
typedef char pc_AppError_size[sizeof(pc_AppError) == 16 ? 1 : -1]; /* (Rio's size) */
#define pc_AppError_LAYOUT "{line:Int,col:Int,len:Int,runtime:Int}"

typedef struct {
  rio_View notes;
  int32_t format;
} pc_ImageWrite;
typedef char pc_ImageWrite_size[sizeof(pc_ImageWrite) == 12 ? 1 : -1]; /* (Rio's size) */
#define pc_ImageWrite_LAYOUT "{notes:String,format:Enum{fromName,png}}"

#define pc_cls_SIG "i"
#define pc_pset_SIG "iii"
#define pc_pget_SIG "ii>i"
#define pc_rect_SIG "iiiii"
#define pc_rgb_SIG "i>i"
#define pc_blit_SIG pc_Image_LAYOUT "ii" pc_Blit_LAYOUT
#define pc_print_SIG "siii"
#define pc_btn_SIG "i>i"
#define pc_key_SIG ">i"
#define pc_modsHeld_SIG ">i"
#define pc_stick_SIG "i>f"
#define pc_trigger_SIG "i>f"
#define pc_time_SIG ">f"
#define pc_showCommands_SIG ""
#define pc_pointer_SIG "Enum{game,ui,none}"
#define pc_pointerAt_SIG ">ii"
#define pc_pointerX_SIG ">i"
#define pc_pointerY_SIG ">i"
#define pc_pointerDown_SIG "i>i"
#define pc_pointerShown_SIG ">i"
#define pc_scroll_SIG "i>i"
#define pc_fromHex_SIG "Bs>i"
#define pc_formatRio_SIG "Bs>i"
#define pc_moduleSource_SIG "sB>i"
#define pc_checkRio_SIG "sssb>i"
#define pc_completeRio_SIG "sB>i"
#define pc_sound_SIG "ib" pc_Sound_LAYOUT
#define pc_soundWav_SIG "ib>i"
#define pc_soundRate_SIG "if"
#define pc_soundVolume_SIG "if"
#define pc_soundPan_SIG "if"
#define pc_soundLoop_SIG "iii"
#define pc_soundStop_SIG "i"
#define pc_soundPlaying_SIG "i>i"
#define pc_synth_SIG "ibi"
#define pc_synthMake_SIG "Bbi>i"
#define pc_synthRate_SIG "if"
#define pc_synthBend_SIG "if"
#define pc_synthEnvelope_SIG "iff"
#define pc_synthSmooth_SIG "ii"
#define pc_synthWave_SIG "ib"
#define pc_soundSeek_SIG "ii"
#define pc_apps_SIG ">i"
#define pc_appName_SIG "iB>i"
#define pc_appInfo_SIG "iB>i"
#define pc_launch_SIG "s"
#define pc_call_SIG "ss"
#define pc_args_SIG "B>i"
#define pc_done_SIG "s"
#define pc_result_SIG "B>i"
#define pc_appError_SIG "BB>" pc_AppError_LAYOUT
#define pc_checkError_SIG "BB>" pc_AppError_LAYOUT
#define pc_appSource_SIG "sB>i"
#define pc_appSave_SIG "ss>i"
#define pc_appDelete_SIG "s>i"
#define pc_appInstall_SIG "s>i"
#define pc_appCopy_SIG "ss>i"
#define pc_appsFor_SIG "sB>i"
#define pc_appRename_SIG "ss>i"
#define pc_dir_SIG "sB>i"
#define pc_fileRead_SIG "sB>i"
#define pc_fileWrite_SIG "ss>i"
#define pc_fileDelete_SIG "s>i"
#define pc_fileCopy_SIG "ss>i"
#define pc_fileRename_SIG "ss>i"
#define pc_dirMake_SIG "s>i"
#define pc_imageLoad_SIG "sB>ii"
#define pc_imageWrite_SIG "s" pc_Image_LAYOUT pc_ImageWrite_LAYOUT ">i"
#define pc_imageNotes_SIG "sB>i"
#define pc_helps_SIG "B>i"
#define pc_helpRead_SIG "sB>i"
#define pc_help_SIG "s"
#define pc_helpTopic_SIG "B>i"
#define pc_saveRead_SIG "sB>i"
#define pc_saveWrite_SIG "ss>i"
#define pc_saveDelete_SIG "s>i"
#define pc_stashWrite_SIG "ss>i"
#define pc_stashRead_SIG "sB>i"
#define pc_stashDelete_SIG "s>i"
#define pc_cacheRead_SIG "sB>i"
#define pc_cacheWrite_SIG "ss>i"
#define pc_cacheDelete_SIG "s>i"
#define pc_settingRead_SIG "sB>i"
#define pc_settingWrite_SIG "ss>i"
#define pc_theme_SIG "Enum{bg,bar,text,soft,dim,faint,accent,heading,pick,select,link,error,ok,codeBg,code,punct,keyword,type,call,number,string,comment}>i"
#define pc_boot_SIG ">i"
#define pc_rioStart_SIG "bb>i"
#define pc_rioEval_SIG "s>i"
#define pc_rioFrame_SIG ">i"
#define pc_rioType_SIG "i"
#define pc_rioError_SIG "B>i"
#define pc_rioLog_SIG "B>i"
#define pc_rioIndent_SIG "s>i"

#define pc_HOST_PROCS(X) \
  X(cls, pc_cls_SIG) \
  X(pset, pc_pset_SIG) \
  X(pget, pc_pget_SIG) \
  X(rect, pc_rect_SIG) \
  X(rgb, pc_rgb_SIG) \
  X(blit, pc_blit_SIG) \
  X(print, pc_print_SIG) \
  X(btn, pc_btn_SIG) \
  X(key, pc_key_SIG) \
  X(modsHeld, pc_modsHeld_SIG) \
  X(stick, pc_stick_SIG) \
  X(trigger, pc_trigger_SIG) \
  X(time, pc_time_SIG) \
  X(showCommands, pc_showCommands_SIG) \
  X(pointer, pc_pointer_SIG) \
  X(pointerAt, pc_pointerAt_SIG) \
  X(pointerX, pc_pointerX_SIG) \
  X(pointerY, pc_pointerY_SIG) \
  X(pointerDown, pc_pointerDown_SIG) \
  X(pointerShown, pc_pointerShown_SIG) \
  X(scroll, pc_scroll_SIG) \
  X(fromHex, pc_fromHex_SIG) \
  X(formatRio, pc_formatRio_SIG) \
  X(moduleSource, pc_moduleSource_SIG) \
  X(checkRio, pc_checkRio_SIG) \
  X(completeRio, pc_completeRio_SIG) \
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
  X(checkError, pc_checkError_SIG) \
  X(appSource, pc_appSource_SIG) \
  X(appSave, pc_appSave_SIG) \
  X(appDelete, pc_appDelete_SIG) \
  X(appInstall, pc_appInstall_SIG) \
  X(appCopy, pc_appCopy_SIG) \
  X(appsFor, pc_appsFor_SIG) \
  X(appRename, pc_appRename_SIG) \
  X(dir, pc_dir_SIG) \
  X(fileRead, pc_fileRead_SIG) \
  X(fileWrite, pc_fileWrite_SIG) \
  X(fileDelete, pc_fileDelete_SIG) \
  X(fileCopy, pc_fileCopy_SIG) \
  X(fileRename, pc_fileRename_SIG) \
  X(dirMake, pc_dirMake_SIG) \
  X(imageLoad, pc_imageLoad_SIG) \
  X(imageWrite, pc_imageWrite_SIG) \
  X(imageNotes, pc_imageNotes_SIG) \
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
  X(cacheRead, pc_cacheRead_SIG) \
  X(cacheWrite, pc_cacheWrite_SIG) \
  X(cacheDelete, pc_cacheDelete_SIG) \
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

#define pc_HASH 0x4c209b47u /* (rio_hashText of the source) */
#define pc_SOURCE \
  "# pc.rio: Rinc's host procs, the pc module. Apps don't import it: pc is there everywhere (pc.cls(0)).\n" \
  "# This is what pc is, written in Rio, and built into Rinc: each app compiles it first, against what\n" \
  "# rinc_start registers, so each declaration's types are checked, every host proc must be declared here,\n" \
  "# and the types they use (pc.Pointer) are here too. (Compiled images keep it, as part of the app.)\n" \
  "\n" \
  "# ---------------------------------------------------------------- the screen: 320x240, palette indexes\n" \
  "\n" \
  "cls* :: host proc(c: Int)                           # fill the screen with color c\n" \
  "pset* :: host proc(pos: [2]Int, c: Int)             # one pixel\n" \
  "pget* :: host proc(pos: [2]Int) -> Int\n" \
  "rect* :: host proc(pos, size: [2]Int, c: Int)       # a filled rectangle: its top left, and its size\n" \
  "rgb* :: host proc(c: Int) -> Int                    # color c of the palette, as 0xRRGGBB\n" \
  "\n" \
  "# a rectangle on the screen: its top left and its size, in pixels\n" \
  "Rect* :: struct\n" \
  "  pos, size: [2]Int\n" \
  "end\n" \
  "Rect.of* :: static proc(pos, size: [2]Int) -> Rect do return {pos, size}   # Rect.of(at, {8, 8})\n" \
  "Rect.corner* :: proc() -> [2]Int do return self.pos + self.size   # just past its bottom right\n" \
  "Rect.has* :: proc(p: [2]Int) -> Bool                 # is p inside it\n" \
  "  q := p - self.pos\n" \
  "  return q.x >= 0 and q.y >= 0 and q.x < self.size.x and q.y < self.size.y\n" \
  "end\n" \
  "Rect.clip* :: proc(r: Rect) -> Rect                  # where the two overlap (an empty size if they don't)\n" \
  "  at := max(self.pos, r.pos)\n" \
  "  return Rect{pos = at, size = max(min(self.corner(), r.corner()) - at, 0)}\n" \
  "end\n" \
  "Rect.between* :: static proc(a, b: [2]Int) -> Rect   # from two corners, either way round, both inside it\n" \
  "  at := min(a, b)\n" \
  "  return Rect{pos = at, size = max(a, b) - at + 1}\n" \
  "end\n" \
  "# a picture: its pixels, a byte each, row by row (0: see-through), size.x to a row, and its size\n" \
  "Image* :: struct\n" \
  "  pixels: Blob\n" \
  "  size: [2]Int\n" \
  "end\n" \
  "# which part of a picture blit draws, and how; left out, the whole picture at its size\n" \
  "Blit* :: struct\n" \
  "  from: [2]Int  # the part's top left, in the picture\n" \
  "  size: [2]Int  # the part's size (a 0: the rest of the picture, that way)\n" \
  "  scale: [2]Int # each pixel so many wide and high (a 0: 1); negative mirrors it that way: {-1, 1}, left for right\n" \
  "end\n" \
  "# a picture, or part of it, as b says, its top left at at; color 0 is see-through: blit(image, {10, 20})\n" \
  "blit* :: host proc(image: Image, at: [2]Int, b?: Blit)\n" \
  "print* :: host proc(s: String, pos: [2]Int, c: Int)  # text at its top left, in the 5x8 font, 6 pixels a character\n" \
  "\n" \
  "# ---------------------------------------------------------------- input\n" \
  "\n" \
  "# 1 while button i is held: 0 left, 1 right, 2 up, 3 down (arrows), then the face buttons, as on an Xbox\n" \
  "# controller: 4 A (V key), 5 B (C), 6 X (X), 7 Y (Z); then a controller's 8 LB, 9 RB, 10 and 11 the\n" \
  "# sticks pressed in, 12 Start, 13 Back\n" \
  "btn* :: host proc(i: Int) -> Int\n" \
  "# the next key typed, or -1: 32-126 as typed, 13 Enter, 8 Backspace, 9 Tab, 27 Esc, Ctrl+letter as 1-26,\n" \
  "# and 128 left, 129 right, 130 up, 131 down, 132 Home, 133 End, 134 Delete, 135 Page Up, 136 Page Down,\n" \
  "# 140 to 151 F1 to F12 (where the Rinc gets them: a browser keeps some, F11 is the desktop's whole display);\n" \
  "# a key that isn't a character with the modifiers held added on: 256 Shift, 512 Ctrl, 1024 Alt; and\n" \
  "# 4096 + n, a command of the app's own picked from its list (Command, below)\n" \
  "key* :: host proc() -> Int\n" \
  "# the modifiers held now, as key() adds them: 256 Shift, 512 Ctrl, 1024 Alt (one locked, tapped twice, too;\n" \
  "# and in a pointer app, Pointer.ui, a controller's left trigger is Shift, its right one Ctrl); -1 where it\n" \
  "# can't tell (in a terminal): for a walk that goes on while Ctrl is held, as Ctrl+Tab's, and the d-pad\n" \
  "# with Shift, selecting\n" \
  "modsHeld* :: host proc() -> Int\n" \
  "# a controller's sticks: 0 left x, 1 left y, 2 right x, 3 right y; -1 to 1, + right and down\n" \
  "stick* :: host proc(i: Int) -> Float\n" \
  "trigger* :: host proc(i: Int) -> Float              # 0 left, 1 right; 0 to 1\n" \
  "time* :: host proc() -> Float                       # seconds since start\n" \
  "# a command, in the list the system key with P shows (or showCommands): an app's own come from its\n" \
  "# commandList* :: proc() -> []pc.Command, asked each time the list opens (up to 56 of them), and\n" \
  "# Rinc's are after them. Picking one types its key, as key() gives it next; while the list is up, the\n" \
  "# app waits (no update or draw)\n" \
  "Command* :: struct\n" \
  "  label: String # what it does: \"Flip the sprite sideways\" (38 characters show)\n" \
  "  keys: String  # its keys, as shown at the right: \"H\" (\"\" for none; 11 show)\n" \
  "  key: Int      # what picking it types: a key as key() gives it, or 4096 + n, a command of its own\n" \
  "end\n" \
  "showCommands* :: host proc()                        # the command list, after this frame (an app's own key for it)\n" \
  "# the pointer, which Rinc draws while it's shown: a mouse moving it or pressing a button shows it; typing,\n" \
  "# the arrows (the d-pad's too) and a controller's center buttons hide it. What it's for, pointer(mode):\n" \
  "Pointer* :: enum\n" \
  "  game  # (each app starts so) a mouse's only: the left stick is still the arrows\n" \
  "  ui    # a controller's too: its left stick moves it, its right one scrolls, A and B are its buttons\n" \
  "  none  # never shown\n" \
  "end\n" \
  "pointer* :: host proc(mode: Pointer)\n" \
  "pointerAt* :: host proc() -> [2]Int                 # where it points, in screen pixels: {x, y}\n" \
  "pointerX* :: host proc() -> Int                     # (its x, and its y, alone)\n" \
  "pointerY* :: host proc() -> Int\n" \
  "pointerDown* :: host proc(i: Int) -> Int            # 1 while button i is held: 0 primary, 1 secondary, 2 middle\n" \
  "pointerShown* :: host proc() -> Int\n" \
  "# the whole steps scrolled since it was last asked: 0 across, + right; 1 up and down, + up\n" \
  "scroll* :: host proc(i: Int) -> Int\n" \
  "\n" \
  "# ---------------------------------------------------------------- data\n" \
  "\n" \
  "# bytes from hex digits added to out (anything else in the text is skipped), until it's full: how many\n" \
  "fromHex* :: host proc(out: Blob[..], text: String) -> Int\n" \
  "# Rio code laid out the one way (as rio fmt does: only its spaces and line breaks change), added to out:\n" \
  "# how long, or -1 (too little room in out: nothing added) or -2 (it couldn't be sure of keeping the code\n" \
  "# the same)\n" \
  "formatRio* :: host proc(out: Blob[..], code: String) -> Int\n" \
  "# a module's source, in Rio, added to out, as much as fits (for an editor's completion): pc's (this), or\n" \
  "# core's (the builtins: Rio's core.rio); its whole length, or -1 for another\n" \
  "moduleSource* :: host proc(name: String, out: Blob[..]) -> Int\n" \
  "# an editor's completion, in two steps. checkRio: main (an app's main file, by its path: path itself if\n" \
  "# it's the one) checked, up to path's text, code: the file being edited as the editor has it, cut at the\n" \
  "# start of the cursor's line. Its compiler's tables go in room, the app's own memory, lent (60 KB is\n" \
  "# plenty): 0, or -1 (an error before the cut; rioError says what; the cut's own, missing 'end', is\n" \
  "# expected). It uses the session's place: a session is over\n" \
  "checkRio* :: host proc(main: String, path: String, code: String, room: Blob) -> Int\n" \
  "# then the names that fit at the end of line (the cursor's line, to the cursor), from what the check\n" \
  "# found (if room is as it left it), added to out, a line each: the name, a tab, what it is (k a word of\n" \
  "# Rio's, v a variable, c a constant, t a type, p a proc, m a method, f a field, M a module, e an enum's\n" \
  "# value), a tab, its type or signature. How long\n" \
  "completeRio* :: host proc(line: String, out: Blob[..]) -> Int\n" \
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
  "# the notes made instead, added to out, as 16-bit samples at voice -1's rate (16000 a second unless set),\n" \
  "# to their end or out's: how many samples (speed 0: on from where the last one stopped, for more than out\n" \
  "# holds)\n" \
  "synthMake* :: host proc(out: Blob[..], notes: Blob, speed: Int) -> Int\n" \
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
  "# what adds text to out below adds as much as fits, and gives its whole length\n" \
  "appName* :: host proc(i: Int, out: Blob[..]) -> Int # app i's name (a project's with its +): how long\n" \
  "appInfo* :: host proc(i: Int, out: Blob[..]) -> Int # its first line: \"Title: what it does\"\n" \
  "launch* :: host proc(name: String)                  # switch to an app or project (or \"home\", \"console\") after this frame\n" \
  "# one app calling another: call starts it with args (after this frame), which it reads with args; done goes\n" \
  "# back to the caller (this app forgotten, as the system key's Q does), which, starting again, reads result\n" \
  "call* :: host proc(name: String, args: String)\n" \
  "args* :: host proc(out: Blob[..]) -> Int            # the args this app was called with: how long (0: none)\n" \
  "done* :: host proc(result: String)                  # back to the caller, with a result (or to the app before)\n" \
  "result* :: host proc(out: Blob[..]) -> Int          # the result of the app this one called: how long, or -1 (taken)\n" \
  "# where the last app to stop with an error stopped, for an editor to go to: its file's path added to path,\n" \
  "# and what was wrong to message (as much as fits). Taken: a second call gives none\n" \
  "AppError* :: struct\n" \
  "  line: Int             # from 1 (0: there's none)\n" \
  "  col: Int              # from 1 (0: not known, as for a runtime error)\n" \
  "  len: Int              # how wide what's wrong is (0: not known)\n" \
  "  runtime: Bool         # a runtime error (else it didn't compile)\n" \
  "end\n" \
  "appError* :: host proc(path: Blob[..], message: Blob[..]) -> AppError\n" \
  "\n" \
  "# the last checkRio's error, as appError has an app's (line 0: none), its file the main file or one it\n" \
  "# includes or imports (/rinc/...): for an editor's underline. Checking the whole file finds its first\n" \
  "checkError* :: host proc(path: Blob[..], message: Blob[..]) -> AppError\n" \
  "# an app's source added to out, as much as fits: its whole length, or -1 if there's no such app\n" \
  "appSource* :: host proc(name: String, out: Blob[..]) -> Int\n" \
  "appSave* :: host proc(name: String, text: String) -> Int   # 0, or -1\n" \
  "appDelete* :: host proc(name: String) -> Int               # all of its folder, its compiled image too: 0, or -1\n" \
  "appInstall* :: host proc(name: String) -> Int              # project name* installed as app name: 0, or -1\n" \
  "appCopy* :: host proc(from: String, to: String) -> Int     # app or project from, as new project to (to*): 0, or -1\n" \
  "# the installed apps that open files ending .ext (as each one's opens.txt says: a line each, the\n" \
  "# extension, then what such a file is), into out a line each, \"name what\": how many. The one picked for\n" \
  "# them is the setting files/appreg/ext; an app is given one as pc.call(name, \"open /its/path\")\n" \
  "appsFor* :: host proc(ext: String, out: Blob[..]) -> Int\n" \
  "appRename* :: host proc(from: String, to: String) -> Int   # project from (from*) as new project to (to*): 0, or -1\n" \
  "\n" \
  "# ---------------------------------------------------------------- files, by path, for a file browser\n" \
  "\n" \
  "# A path is absolute (/rinc/save/chord/best), in /rinc, the root: looking goes anywhere in it, changing\n" \
  "# only inside it. Or, not starting with /, it's one of the app's own files, in its folder (as a project,\n" \
  "# /rinc/projects/chord/, or installed, /rinc/apps/chord/): imageRead(\"sprites.png\", pixels), for reading\n" \
  "# only. What reads into a builder adds to what's in it, as much as fits: out.len() tells how much it holds\n" \
  "dir* :: host proc(path: String, out: Blob[..]) -> Int      # its entries, folders first, a line each (\"d name\", \"f name\"): how many, or -1\n" \
  "fileRead* :: host proc(path: String, out: Blob[..]) -> Int # its whole length, or -1\n" \
  "fileWrite* :: host proc(path: String, data: String) -> Int # 0, or -1\n" \
  "fileDelete* :: host proc(path: String) -> Int              # a file, or a folder and all in it: 0, or -1\n" \
  "fileCopy* :: host proc(from: String, to: String) -> Int    # a file or a folder, to a path not taken: 0, or -1\n" \
  "fileRename* :: host proc(from: String, to: String) -> Int  # to a path not taken: 0, or -1\n" \
  "dirMake* :: host proc(path: String) -> Int                 # a folder, at a path not taken: 0, or -1\n" \
  "# pictures in files (Image: its pixels and size). imageRead adds one's pixels to pixels, each color the\n" \
  "# palette's nearest (exactly, for one imageWrite wrote), and gives it as an Image, viewing them (size {0,\n" \
  "# 0} for none). pixels needs room past what it holds for the file's rows as they come: (w + 1) * h bytes\n" \
  "# for a PNG of the palette's; without it, the Image has its size but no pixels, and the log says why. imageWrite writes one as o says, and\n" \
  "# imageNotes adds its notes to out. Only PNG files, so far\n" \
  "ImageFormat* :: enum\n" \
  "  fromName\n" \
  "  png\n" \
  "end\n" \
  "imageLoad :: host proc(path: String, pixels: Blob[..]) -> [2]Int # (imageRead's: the pixels added, and its size)\n" \
  "imageRead* :: proc(path: String, pixels: Blob[..]) -> Image\n" \
  "  start := pixels.len()\n" \
  "  size := imageLoad(path, pixels)\n" \
  "  return {pixels = pixels[start..<pixels.len()], size = size}\n" \
  "end\n" \
  "# how imageWrite writes a picture; left out, as its name ends (.png), with no notes\n" \
  "ImageWrite* :: struct\n" \
  "  notes: String        # the app's notes about it (text, up to 512 bytes, kept in the file: a sprite sheet's sprite size, say)\n" \
  "  format: ImageFormat  # fromName: as its name ends\n" \
  "end\n" \
  "imageWrite* :: host proc(path: String, image: Image, o?: ImageWrite) -> Int # 0, or -1\n" \
  "imageNotes* :: host proc(path: String, out: Blob[..]) -> Int  # how many bytes, or -1 (none)\n" \
  "\n" \
  "# ---------------------------------------------------------------- help texts, for a help app: /rinc/help, and the bundle's\n" \
  "\n" \
  "# the help texts (Markdown): their names (paths, .md left off: rinc/spec/apps; a project's, chord+/...),\n" \
  "# in order, added to out, each with a 10 after it, as many as fit: how many\n" \
  "helps* :: host proc(out: Blob[..]) -> Int\n" \
  "# a help text added to out, as much as fits (storage's, else the bundle's): its whole length, or -1 if there's none\n" \
  "helpRead* :: host proc(name: String, out: Blob[..]) -> Int\n" \
  "# the help app, after this frame, at a help text (and maybe a heading): help(\"rio/spec/statements#if\")\n" \
  "help* :: host proc(topic: String)\n" \
  "helpTopic* :: host proc(out: Blob[..]) -> Int         # for the help app: the topic it was opened at, taken (0: none)\n" \
  "\n" \
  "# ---------------------------------------------------------------- this app's own files: /rinc/save/<app>\n" \
  "\n" \
  "# a file's contents added to out, as much as fits: its whole length, or -1 if there's no such file. Names\n" \
  "# are 1 to 32 letters, digits, _, - and . (not first)\n" \
  "saveRead* :: host proc(file: String, out: Blob[..]) -> Int\n" \
  "saveWrite* :: host proc(file: String, data: String) -> Int  # 0, or -1\n" \
  "saveDelete* :: host proc(file: String) -> Int               # 0, or -1\n" \
  "\n" \
  "# ---------------------------------------------------------------- this app's stash: /rinc/stash/<app>/<key>\n" \
  "\n" \
  "# what an app keeps of where it was, for its next start, under keys of its own (named as save files are),\n" \
  "# written as it goes; Rinc drops all of it on Sys+Q and Sys+R, where it never touches the save space\n" \
  "stashWrite* :: host proc(key: String, data: String) -> Int  # 0, or -1\n" \
  "stashRead* :: host proc(key: String, out: Blob[..]) -> Int  # added to out, as much as fits: its whole length, or -1 for none\n" \
  "stashDelete* :: host proc(key: String) -> Int               # 0, or -1\n" \
  "# ---------------------------------------------------------------- this app's cache: /rinc/cache/<app>\n" \
  "\n" \
  "# what an app can make again (a project's, chord+, its own), named as save files are; any of it may go at\n" \
  "# any time. Rinc keeps the app's log there, as log: what it logged lately, and what Rinc said about it\n" \
  "cacheRead* :: host proc(file: String, out: Blob[..]) -> Int  # added to out, as much as fits: its whole length, or -1 for none\n" \
  "cacheWrite* :: host proc(file: String, data: String) -> Int  # 0, or -1\n" \
  "cacheDelete* :: host proc(file: String) -> Int               # 0, or -1\n" \
  "\n" \
  "# the whole Rinc's settings, shared by the apps (/rinc/settings/<key>): \"active\" is the app or project\n" \
  "# being worked on (chord, or chord+); keys are named as save files are\n" \
  "settingRead* :: host proc(key: String, out: Blob[..]) -> Int  # added to out, as much as fits: its whole length, or -1\n" \
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
  "rioError* :: host proc(out: Blob[..]) -> Int        # the last error, as text, added to out: how long\n" \
  "rioLog* :: host proc(out: Blob[..]) -> Int          # what it logged since, as much as fits, taken: how much (lines end with 10)\n" \
  "rioIndent* :: host proc(text: String) -> Int        # the indent steps an unfinished input's next line takes\n" \
  "\n" \
  "# ---------------------------------------------------------------- a line of text, edited\n" \
  "\n" \
  "# a line of text being edited (a name, something to find): its text, in room of the app's own\n" \
  "# (Field{text = room}, room a Blob[..n]), the cursor (0 to its length), the selection's other end\n" \
  "# (anchor == cursor: none), and draw's scroll. key() edits it as a text box does in Windows: typing\n" \
  "# (in place of what's selected), Backspace and Delete (with Ctrl, a word), left and right (with Ctrl, by\n" \
  "# words; with Shift, selecting), Home and End, and Ctrl+A (all of it); draw() shows it\n" \
  "Field* :: struct\n" \
  "  text: Blob[..]\n" \
  "  cursor: Int\n" \
  "  anchor: Int\n" \
  "  scroll: Int\n" \
  "end\n" \
  "Field.set* :: proc(s: String) # its text, all of it selected (typing takes its place)\n" \
  "  self.text.clear()\n" \
  "  for c in s\n" \
  "    _ = self.text.push(c)\n" \
  "  end\n" \
  "  self.anchor = 0\n" \
  "  self.cursor = self.text.len()\n" \
  "  self.scroll = 0\n" \
  "end\n" \
  "Field.select* :: proc(from: Int, to: Int) # from to, the cursor at to\n" \
  "  self.anchor = max(0, min(from, self.text.len()))\n" \
  "  self.cursor = max(0, min(to, self.text.len()))\n" \
  "end\n" \
  "Field.selected* :: proc() -> Bool do return self.anchor != self.cursor\n" \
  "Field.wordAt :: proc(i: Int) -> Bool # (a letter, digit or _)\n" \
  "  c := self.text[i]\n" \
  "  return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or (c >= '0' and c <= '9') or c == '_'\n" \
  "end\n" \
  "Field.wordLeft :: proc(from: Int) -> Int # (the start of the word before from)\n" \
  "  i := from\n" \
  "  for i > 0 and not self.wordAt(i - 1)\n" \
  "    i -= 1\n" \
  "  end\n" \
  "  for i > 0 and self.wordAt(i - 1)\n" \
  "    i -= 1\n" \
  "  end\n" \
  "  return i\n" \
  "end\n" \
  "Field.wordRight :: proc(from: Int) -> Int # (the end of the word after from, as Windows': past what's after it too)\n" \
  "  i := from\n" \
  "  n := self.text.len()\n" \
  "  for i < n and self.wordAt(i)\n" \
  "    i += 1\n" \
  "  end\n" \
  "  for i < n and not self.wordAt(i)\n" \
  "    i += 1\n" \
  "  end\n" \
  "  return i\n" \
  "end\n" \
  "Field.cut :: proc(a: Int, b: Int) # (a to b taken out, the cursor there)\n" \
  "  n := self.text.len()\n" \
  "  for i in b..<n\n" \
  "    self.text[a + i - b] = self.text[i]\n" \
  "  end\n" \
  "  for _ in a..<b\n" \
  "    _ = self.text.pop()\n" \
  "  end\n" \
  "  self.cursor = a\n" \
  "  self.anchor = a\n" \
  "end\n" \
  "Field.cutSelected :: proc()\n" \
  "  if self.anchor != self.cursor\n" \
  "    self.cut(min(self.anchor, self.cursor), max(self.anchor, self.cursor))\n" \
  "  end\n" \
  "end\n" \
  "Field.moveTo :: proc(i: Int, shift: Bool)\n" \
  "  self.cursor = i\n" \
  "  if not shift\n" \
  "    self.anchor = i\n" \
  "  end\n" \
  "end\n" \
  "Field.key* :: proc(c: Int) -> Bool # a key, as key() gives it: true if it was the field's\n" \
  "  k := c & 255\n" \
  "  shift := c & 256 != 0\n" \
  "  ctrl := c & 512 != 0\n" \
  "  lo := min(self.anchor, self.cursor)\n" \
  "  hi := max(self.anchor, self.cursor)\n" \
  "  if c >= 32 and c < 127 # (typed, in place of what's selected)\n" \
  "    self.cutSelected()\n" \
  "    if self.text.push(c) # (room for it: then moved up to the cursor)\n" \
  "      i := self.text.len() - 1\n" \
  "      for i > self.cursor\n" \
  "        self.text[i] = self.text[i - 1]\n" \
  "        i -= 1\n" \
  "      end\n" \
  "      self.text[self.cursor] = c\n" \
  "      self.cursor += 1\n" \
  "      self.anchor = self.cursor\n" \
  "    end\n" \
  "  case k == 8 # Backspace, Ctrl+Backspace\n" \
  "    if lo != hi\n" \
  "      self.cutSelected()\n" \
  "    case ctrl\n" \
  "      self.cut(self.wordLeft(self.cursor), self.cursor)\n" \
  "    case self.cursor > 0\n" \
  "      self.cut(self.cursor - 1, self.cursor)\n" \
  "    end\n" \
  "  case k == 134 # Delete, Ctrl+Delete\n" \
  "    if lo != hi\n" \
  "      self.cutSelected()\n" \
  "    case ctrl\n" \
  "      self.cut(self.cursor, self.wordRight(self.cursor))\n" \
  "    case self.cursor < self.text.len()\n" \
  "      self.cut(self.cursor, self.cursor + 1)\n" \
  "    end\n" \
  "  case k == 128 # left: by a word with Ctrl; with nothing held, out of a selection to its start\n" \
  "    if ctrl\n" \
  "      self.moveTo(self.wordLeft(self.cursor), shift)\n" \
  "    case lo != hi and not shift\n" \
  "      self.moveTo(lo, false)\n" \
  "    else\n" \
  "      self.moveTo(max(self.cursor - 1, 0), shift)\n" \
  "    end\n" \
  "  case k == 129\n" \
  "    if ctrl\n" \
  "      self.moveTo(self.wordRight(self.cursor), shift)\n" \
  "    case lo != hi and not shift\n" \
  "      self.moveTo(hi, false)\n" \
  "    else\n" \
  "      self.moveTo(min(self.cursor + 1, self.text.len()), shift)\n" \
  "    end\n" \
  "  case k == 132\n" \
  "    self.moveTo(0, shift)\n" \
  "  case k == 133\n" \
  "    self.moveTo(self.text.len(), shift)\n" \
  "  case c == 1 # Ctrl+A\n" \
  "    self.select(0, self.text.len())\n" \
  "  else\n" \
  "    return false\n" \
  "  end\n" \
  "  return true\n" \
  "end\n" \
  "Field.draw* :: proc(at: [2]Int, chars: Int, color: Int, selectColor: Int, cursorColor: Int) # chars across, scrolled to show the cursor\n" \
  "  if self.cursor < self.scroll\n" \
  "    self.scroll = self.cursor\n" \
  "  end\n" \
  "  if self.cursor > self.scroll + chars - 1\n" \
  "    self.scroll = self.cursor - chars + 1\n" \
  "  end\n" \
  "  self.scroll = max(0, min(self.scroll, self.text.len()))\n" \
  "  upTo := min(self.text.len(), self.scroll + chars)\n" \
  "  lo := max(min(self.anchor, self.cursor), self.scroll)\n" \
  "  hi := min(max(self.anchor, self.cursor), upTo)\n" \
  "  if hi > lo\n" \
  "    rect(at + {(lo - self.scroll) * 6 - 1, -1}, {(hi - lo) * 6 + 1, 9}, selectColor)\n" \
  "  end\n" \
  "  print(self.text[self.scroll..<upTo].asString(), at, color)\n" \
  "  rect(at + {(self.cursor - self.scroll) * 6 - 1, -1}, {1, 9}, cursorColor)\n" \
  "end\n" \
  "\n"
