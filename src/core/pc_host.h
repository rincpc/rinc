/* Made by rio_hostHeader (rio -H) from the Rio module that declares these host procs: don't edit.
   A struct parameter comes as 2 words: rio_getStruct(vm, a + i, &s, sizeof s) copies it into s.
   Register the module's source with its procs (_SOURCE, below: rio_addSource), and each proc with
   its _SIG (_HOST_PROCS lists them all). */
#include <stdint.h>
#include "rio.h"

typedef struct {
  int32_t outline;
  int32_t radius;
} pc_Shape;
typedef char pc_Shape_size[sizeof(pc_Shape) == 8 ? 1 : -1]; /* (Rio's size) */
#define pc_Shape_LAYOUT "{outline:Int,radius:Int}"

typedef struct {
  rio_View pixels;
  int32_t size[2];
  int32_t depth;
} pc_Image;
typedef char pc_Image_size[sizeof(pc_Image) == 20 ? 1 : -1]; /* (Rio's size) */
#define pc_Image_LAYOUT "{pixels:Blob,size:[2]Int,depth:Int}"

typedef struct {
  int32_t from[2];
  int32_t size[2];
  int32_t scale[2];
  rio_View map;
} pc_Blit;
typedef char pc_Blit_size[sizeof(pc_Blit) == 32 ? 1 : -1]; /* (Rio's size) */
#define pc_Blit_LAYOUT "{from:[2]Int,size:[2]Int,scale:[2]Int,map:[]Int}"

typedef struct {
  pc_Image image;
  int32_t cell[2];
  int32_t across;
  int32_t first;
  int32_t last;
  int32_t line;
  int32_t gap;
  int32_t colors;
  rio_View glyphs;
} pc_Font;
typedef char pc_Font_size[sizeof(pc_Font) == 60 ? 1 : -1]; /* (Rio's size) */
#define pc_Font_LAYOUT "{image:{pixels:Blob,size:[2]Int,depth:Int},cell:[2]Int,across:Int,first:Int,last:Int,line:Int,gap:Int,colors:Int,glyphs:Blob}"

typedef struct {
  int32_t width;
  pc_Font font;
} pc_Text;
typedef char pc_Text_size[sizeof(pc_Text) == 64 ? 1 : -1]; /* (Rio's size) */
#define pc_Text_LAYOUT "{width:Int,font:{image:{pixels:Blob,size:[2]Int,depth:Int},cell:[2]Int,across:Int,first:Int,last:Int,line:Int,gap:Int,colors:Int,glyphs:Blob}}"

typedef struct {
  int32_t at[2];
  int32_t primary;
  int32_t secondary;
  int32_t middle;
  int32_t shown;
  int32_t scrolled[2];
} pc_PointerState;
typedef char pc_PointerState_size[sizeof(pc_PointerState) == 32 ? 1 : -1]; /* (Rio's size) */
#define pc_PointerState_LAYOUT "{at:[2]Int,primary:Int,secondary:Int,middle:Int,shown:Int,scrolled:[2]Int}"

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
  int32_t bytes;
  int32_t from[2];
  int32_t size[2];
} pc_ImageRead;
typedef char pc_ImageRead_size[sizeof(pc_ImageRead) == 20 ? 1 : -1]; /* (Rio's size) */
#define pc_ImageRead_LAYOUT "{bytes:Int,from:[2]Int,size:[2]Int}"

typedef struct {
  rio_View notes;
  int32_t format;
  rio_View flags;
  int32_t sheet;
} pc_ImageWrite;
typedef char pc_ImageWrite_size[sizeof(pc_ImageWrite) == 24 ? 1 : -1]; /* (Rio's size) */
#define pc_ImageWrite_LAYOUT "{notes:String,format:Enum{fromName,png},flags:[]Int,sheet:Int}"

#define pc_cls_SIG "i"
#define pc_screen_SIG "Enum{full,half,quarter}"
#define pc_screenSize_SIG ">ii"
#define pc_pset_SIG "iii"
#define pc_pget_SIG "ii>i"
#define pc_rect_SIG "iiiii" pc_Shape_LAYOUT
#define pc_line_SIG "iiiii"
#define pc_ellipse_SIG "iiiii" pc_Shape_LAYOUT
#define pc_triangle_SIG "iiiiiii" pc_Shape_LAYOUT
#define pc_curve_SIG "ffffffffi"
#define pc_polygon_SIG "[ff]i" pc_Shape_LAYOUT
#define pc_path_SIG "[ff]si" pc_Shape_LAYOUT
#define pc_transform_SIG "ffffff>ffffff"
#define pc_transformBy_SIG "ffffff>ffffff"
#define pc_rgb_SIG "i>i"
#define pc_blit_SIG pc_Image_LAYOUT "ii" pc_Blit_LAYOUT
#define pc_print_SIG "siii" pc_Text_LAYOUT
#define pc_textSize_SIG "s" pc_Text_LAYOUT ">ii"
#define pc_btn_SIG "Enum{left,right,up,down,a,b,x,y,lb,rb,ls,rs,start,back}>i"
#define pc_btnp_SIG "Enum{left,right,up,down,a,b,x,y,lb,rb,ls,rs,start,back}>i"
#define pc_key_SIG ">i"
#define pc_modsHeld_SIG ">i"
#define pc_keyHeld_SIG "i>i"
#define pc_stick_SIG "i>f"
#define pc_trigger_SIG "i>f"
#define pc_appTime_SIG ">f"
#define pc_appTimeReset_SIG ""
#define pc_showCommands_SIG ""
#define pc_pointer_SIG "Enum{game,ui,none}"
#define pc_pointerState_SIG ">" pc_PointerState_LAYOUT
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
#define pc_audioLoad_SIG ">ii"
#define pc_songPlay_SIG "ibbiiii"
#define pc_songStop_SIG ""
#define pc_songPause_SIG "i"
#define pc_songLevers_SIG "iii"
#define pc_songAt_SIG ">iii"
#define pc_freeVoice_SIG ">i"
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
#define pc_fileReadAt_SIG "siB>i"
#define pc_fileWriteAt_SIG "sis>i"
#define pc_fileDelete_SIG "s>i"
#define pc_fileCopy_SIG "ss>i"
#define pc_fileRename_SIG "ss>i"
#define pc_dirMake_SIG "s>i"
#define pc_imageLoad_SIG "sB" pc_ImageRead_LAYOUT ">iii"
#define pc_imageNotes_SIG "sB>i"
#define pc_imageWrite_SIG "s" pc_Image_LAYOUT pc_ImageWrite_LAYOUT ">i"
#define pc_imageFlags_SIG "s[i]>i"
#define pc_imageSpriteFlags_SIG "siiB>i"
#define pc_imageFlag_SIG "ss>i"
#define pc_helps_SIG "B>i"
#define pc_helpRead_SIG "sB>i"
#define pc_help_SIG "s"
#define pc_helpTopic_SIG "B>i"
#define pc_theme_SIG "Enum{bg,bar,text,soft,dim,faint,accent,heading,pick,select,link,error,ok,codeBg,code,punct,keyword,type,call,number,string,comment}>i"
#define pc_boot_SIG ">i"
#define pc_rioStart_SIG "bbs>i"
#define pc_rioEval_SIG "s>i"
#define pc_rioFrame_SIG ">i"
#define pc_rioType_SIG "i"
#define pc_rioError_SIG "B>i"
#define pc_rioLog_SIG "B>i"
#define pc_rioIndent_SIG "s>i"

#define pc_HOST_PROCS(X) \
  X(cls, pc_cls_SIG) \
  X(screen, pc_screen_SIG) \
  X(screenSize, pc_screenSize_SIG) \
  X(pset, pc_pset_SIG) \
  X(pget, pc_pget_SIG) \
  X(rect, pc_rect_SIG) \
  X(line, pc_line_SIG) \
  X(ellipse, pc_ellipse_SIG) \
  X(triangle, pc_triangle_SIG) \
  X(curve, pc_curve_SIG) \
  X(polygon, pc_polygon_SIG) \
  X(path, pc_path_SIG) \
  X(transform, pc_transform_SIG) \
  X(transformBy, pc_transformBy_SIG) \
  X(rgb, pc_rgb_SIG) \
  X(blit, pc_blit_SIG) \
  X(print, pc_print_SIG) \
  X(textSize, pc_textSize_SIG) \
  X(btn, pc_btn_SIG) \
  X(btnp, pc_btnp_SIG) \
  X(key, pc_key_SIG) \
  X(modsHeld, pc_modsHeld_SIG) \
  X(keyHeld, pc_keyHeld_SIG) \
  X(stick, pc_stick_SIG) \
  X(trigger, pc_trigger_SIG) \
  X(appTime, pc_appTime_SIG) \
  X(appTimeReset, pc_appTimeReset_SIG) \
  X(showCommands, pc_showCommands_SIG) \
  X(pointer, pc_pointer_SIG) \
  X(pointerState, pc_pointerState_SIG) \
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
  X(audioLoad, pc_audioLoad_SIG) \
  X(songPlay, pc_songPlay_SIG) \
  X(songStop, pc_songStop_SIG) \
  X(songPause, pc_songPause_SIG) \
  X(songLevers, pc_songLevers_SIG) \
  X(songAt, pc_songAt_SIG) \
  X(freeVoice, pc_freeVoice_SIG) \
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
  X(fileReadAt, pc_fileReadAt_SIG) \
  X(fileWriteAt, pc_fileWriteAt_SIG) \
  X(fileDelete, pc_fileDelete_SIG) \
  X(fileCopy, pc_fileCopy_SIG) \
  X(fileRename, pc_fileRename_SIG) \
  X(dirMake, pc_dirMake_SIG) \
  X(imageLoad, pc_imageLoad_SIG) \
  X(imageNotes, pc_imageNotes_SIG) \
  X(imageWrite, pc_imageWrite_SIG) \
  X(imageFlags, pc_imageFlags_SIG) \
  X(imageSpriteFlags, pc_imageSpriteFlags_SIG) \
  X(imageFlag, pc_imageFlag_SIG) \
  X(helps, pc_helps_SIG) \
  X(helpRead, pc_helpRead_SIG) \
  X(help, pc_help_SIG) \
  X(helpTopic, pc_helpTopic_SIG) \
  X(theme, pc_theme_SIG) \
  X(boot, pc_boot_SIG) \
  X(rioStart, pc_rioStart_SIG) \
  X(rioEval, pc_rioEval_SIG) \
  X(rioFrame, pc_rioFrame_SIG) \
  X(rioType, pc_rioType_SIG) \
  X(rioError, pc_rioError_SIG) \
  X(rioLog, pc_rioLog_SIG) \
  X(rioIndent, pc_rioIndent_SIG)

#define pc_HASH 0xecfbd1e1u /* (rio_hashText of the source) */
#define pc_SOURCE \
  "# pc.rio: Rinc's host procs, the pc module. Apps don't import it: pc is there everywhere (pc.cls(0)).\n" \
  "# This is what pc is, written in Rio, and built into Rinc: each app compiles it first, against what\n" \
  "# rinc_start registers, so each declaration's types are checked, every host proc must be declared here,\n" \
  "# and the types they use (pc.Pointer) are here too. (Compiled images keep it, as part of the app.)\n" \
  "\n" \
  "# ---------------------------------------------------------------- the screen: 320x240, palette indexes\n" \
  "\n" \
  "cls* :: host proc(c: Int)                           # fill the screen with color c\n" \
  "# the screen's size, for this app: full (320x240), half (160x120) or quarter (80x60), each pixel shown 2 or\n" \
  "# 4 times as big (text too, and the pointer's place in these pixels); every app starts full\n" \
  "Screen* :: enum\n" \
  "  full\n" \
  "  half\n" \
  "  quarter\n" \
  "end\n" \
  "screen* :: host proc(size: Screen)\n" \
  "screenSize* :: host proc() -> [2]Int                # the screen's size now, in its pixels\n" \
  "pset* :: host proc(pos: [2]Int, c: Int)             # one pixel\n" \
  "pget* :: host proc(pos: [2]Int) -> Int\n" \
  "# how a shape's drawn (rect, ellipse, triangle; left out: filled, square)\n" \
  "Shape* :: struct\n" \
  "  outline: Bool   # just its edge, a pixel wide (else filled)\n" \
  "  radius: Int     # rect's corners rounded, by this many pixels (0: square)\n" \
  "end\n" \
  "rect* :: host proc(pos, size: [2]Int, c: Int, how?: Shape) # a rectangle: its top left, and its size\n" \
  "line* :: host proc(from, to: [2]Int, c: Int)       # a line, both ends' pixels drawn\n" \
  "ellipse* :: host proc(pos, size: [2]Int, c: Int, how?: Shape) # an ellipse in the rectangle pos, size (a square: a circle)\n" \
  "triangle* :: host proc(p, q, r: [2]Int, c: Int, how?: Shape) # a triangle, its corners pixels\n" \
  "curve* :: host proc(from, c1, c2, to: [2]Float, c: Int) # a curve (cubic Bezier) from from to to, pulled toward c1 then c2 (between pixels too: {10.5, 3})\n" \
  "polygon* :: host proc(points: [][2]Float, c: Int, how?: Shape) # a polygon: its corners in order (between pixels too); filled where its edges wind around\n" \
  "path* :: host proc(points: [][2]Float, kinds: String, c: Int, how?: Shape) # a path's points and their kinds, as a Path keeps them (Path.draw)\n" \
  "\n" \
  "# the transform all drawing goes through (pset, line, rect, ellipse, triangle, curve, polygon, path, blit,\n" \
  "# print): a, b, c, d, e, f, the plane the pixels tile (pixel x, y: the square from x, y to x + 1, y + 1)\n" \
  "# taken to a x + c y + e, b x + d y + f; an image's or a rect's area turned, scaled, as a whole. Back to\n" \
  "# plain at each frame's start (and each app's). transform sets it; transformBy puts m under it (m first,\n" \
  "# then the one there: a part turned inside its whole, moved); each gives back the one it replaced, to put\n" \
  "# back with transform\n" \
  "transform* :: host proc(m: [6]Float) -> [6]Float\n" \
  "transformBy* :: host proc(m: [6]Float) -> [6]Float\n" \
  "plain* :: proc() -> [6]Float # (none: each point where it is)\n" \
  "  return {1.0, 0.0, 0.0, 1.0, 0.0, 0.0}\n" \
  "end\n" \
  "moved* :: proc(by: [2]Float) -> [6]Float\n" \
  "  return {1.0, 0.0, 0.0, 1.0, by.x, by.y}\n" \
  "end\n" \
  "scaled* :: proc(by: [2]Float) -> [6]Float # (from 0, 0: by.x across, by.y down)\n" \
  "  return {by.x, 0.0, 0.0, by.y, 0.0, 0.0}\n" \
  "end\n" \
  "turned* :: proc(angle: Float) -> [6]Float # (around 0, 0; radians, as sin and cos: a quarter turn, right to down)\n" \
  "  return {cos(angle), sin(angle), -sin(angle), cos(angle), 0.0, 0.0}\n" \
  "end\n" \
  "turnedAround* :: proc(angle: Float, at: [2]Float) -> [6]Float # (around at)\n" \
  "  c := cos(angle)\n" \
  "  s := sin(angle)\n" \
  "  return {c, s, -s, c, at.x - c * at.x + s * at.y, at.y - s * at.x - c * at.y}\n" \
  "end\n" \
  "\n" \
  "# a path: lines, curves and arcs from point to point, in parts (move starts one), drawn filled (each part\n" \
  "# closed, filled where its edges wind around: a part inside another, the other way round, a hole) or\n" \
  "# outlined (a part closed only if close says so). Kept here, in the app's memory: 128 points at most (a\n" \
  "# curve takes 3, an arc 3); past that, what's added is left out\n" \
  "Path* :: struct\n" \
  "  points: [..128][2]Float\n" \
  "  kinds: Blob[..128]\n" \
  "end\n" \
  "Path.clear* :: proc()\n" \
  "  self.points.clear()\n" \
  "  self.kinds.clear()\n" \
  "end\n" \
  "Path.add :: proc(k: Int, p: [2]Float)\n" \
  "  if self.points.len() < 128\n" \
  "    _ = self.points.push(p)\n" \
  "    _ = self.kinds.push(k)\n" \
  "  end\n" \
  "end\n" \
  "Path.move* :: proc(to: [2]Float) # a new part, starting at to\n" \
  "  self.add('M', to)\n" \
  "end\n" \
  "Path.line* :: proc(to: [2]Float) # a line to to (with no part yet: the start of one)\n" \
  "  self.add('L', to)\n" \
  "end\n" \
  "Path.curve* :: proc(c1, c2, to: [2]Float) # a curve (cubic Bezier) to to, pulled toward c1 then c2\n" \
  "  if self.points.len() + 3 <= 128\n" \
  "    self.add('c', c1)\n" \
  "    self.add('c', c2)\n" \
  "    self.add('C', to)\n" \
  "  end\n" \
  "end\n" \
  "# an arc of the ellipse at center with radii (a circle's: both the same), from angle from to angle to\n" \
  "# (radians, as sin and cos: 0 right, a quarter turn down; to less than from: the other way round); a line\n" \
  "# to its start first, from where the path is\n" \
  "Path.arc* :: proc(center, radii: [2]Float, from, to: Float)\n" \
  "  if self.points.len() + 3 <= 128\n" \
  "    self.add('a', center)\n" \
  "    self.add('r', radii)\n" \
  "    self.add('A', {from, to})\n" \
  "  end\n" \
  "end\n" \
  "Path.close* :: proc() # back to the part's start, closed\n" \
  "  self.add('Z', {0.0, 0.0})\n" \
  "end\n" \
  "Path.draw* :: proc(c: Int, how?: Shape)\n" \
  "  path(self.points[], self.kinds.asString(), c, how)\n" \
  "end\n" \
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
  "  depth: Int      # bits a pixel: 0 (or 8), a byte a pixel, Rinc's color, as most are; 1, 2 or 4, its rows\n" \
  "                  # packed (a row's bytes: (width * depth + 7) / 8), each pixel an index into its own colors,\n" \
  "                  # its palette, after its rows in pixels (2^depth bytes: index -> Rinc's color, 0 see-through)\n" \
  "end\n" \
  "# which part of a picture blit draws, and how; left out, the whole picture at its size\n" \
  "Blit* :: struct\n" \
  "  from: [2]Int  # the part's top left, in the picture\n" \
  "  size: [2]Int  # the part's size (a 0: the rest of the picture, that way)\n" \
  "  scale: [2]Int # each pixel so many wide and high (a 0: 1); negative mirrors it that way: {-1, 1}, left for right\n" \
  "  map: []Int    # its colors this time: a pixel's index (or, a byte a pixel, its color) -> the color drawn (0\n" \
  "                # or -1 see-through), instead of its own palette: a sheet in other colors (empty: as it is)\n" \
  "end\n" \
  "# a picture, or part of it, as b says, its top left at at; color 0 is see-through: blit(image, {10, 20})\n" \
  "blit* :: host proc(image: Image, at: [2]Int, b?: Blit)\n" \
  "# a font, from a sheet (fontRead): its glyphs' picture, in cells cell big across its rows, the first's\n" \
  "# character first, on to last; line, pixels from a line to the next (0: its cell's height); colors: its\n" \
  "# own colors (else each glyph's pixels are print's color). A proportional one has glyphs, two bytes for\n" \
  "# each: where it starts in its cell, and how wide it is (its empty columns trimmed; an empty one, a\n" \
  "# space's width); a fixed one, each glyph its cell's width; gap pixels after each (negative: closer). A\n" \
  "# character it hasn't: half a cell. The zero Font is Rinc's own, 5x11 in 6x12, compact (15 words: a pc.Text holding one is 16)\n" \
  "Font* :: struct\n" \
  "  image: Image\n" \
  "  cell: [2]Int\n" \
  "  across: Int  # its cells across a row (0: as many as its picture's width has)\n" \
  "  first: Int\n" \
  "  last: Int\n" \
  "  line: Int\n" \
  "  gap: Int\n" \
  "  colors: Bool\n" \
  "  glyphs: Blob\n" \
  "end\n" \
  "# how print lays text out: its width, in pixels (0: no wrapping; past it, a line breaks at a space, and a\n" \
  "# word longer than a line where it must); line, pixels from a line to the next (0: the font's: 9 for\n" \
  "# Rinc's own); font (left out: Rinc's own, 5x11, 6 pixels a character)\n" \
  "Text* :: struct\n" \
  "  width: Int\n" \
  "  font: Font   # its line spacing too: font.line (0: its cell's height; Rinc's own, 9, compact: its top 3 rows left out): {font = {line = 12}}, whole\n" \
  "end\n" \
  "print* :: host proc(s: String, pos: [2]Int, c: Int, how?: Text) # text at its top left, in how's font (Rinc's 5x11, 6 pixels a character; UTF-8); a newline starts a line\n" \
  "textSize* :: host proc(s: String, how?: Text) -> [2]Int # how big print draws it: its widest line's width, and its lines' height\n" \
  "\n" \
  "# ---------------------------------------------------------------- input\n" \
  "\n" \
  "# the buttons: the arrows, then the face buttons, as on an Xbox controller: A (V key), B (C), X (X), Y (Z);\n" \
  "# then a controller's LB and RB, the sticks pressed in, Start and Back\n" \
  "Button* :: enum\n" \
  "  left\n" \
  "  right\n" \
  "  up\n" \
  "  down\n" \
  "  a\n" \
  "  b\n" \
  "  x\n" \
  "  y\n" \
  "  lb\n" \
  "  rb\n" \
  "  ls\n" \
  "  rs\n" \
  "  start\n" \
  "  back\n" \
  "end\n" \
  "btn* :: host proc(b: Button) -> Int  # 1 while it's held\n" \
  "btnp* :: host proc(b: Button) -> Int # 1 when it's pressed, and again while it's held (after 15 frames, each 4th), as PICO-8's\n" \
  "dpad* :: proc() -> [2]Int do return {btn(.right) - btn(.left), btn(.down) - btn(.up)} # the arrows held: -1, 0 or 1 each way\n" \
  "# the button an arrow key from key() is (128 to 131: left, right, up, down), for telling a key typed from\n" \
  "# one held (pc.btn(pc.keyButton(k)) == 0: typed, as over serial, not held); another key: .back\n" \
  "keyButton* :: proc(k: Int) -> Button\n" \
  "  switch k\n" \
  "  case 128\n" \
  "    return .left\n" \
  "  case 129\n" \
  "    return .right\n" \
  "  case 130\n" \
  "    return .up\n" \
  "  case 131\n" \
  "    return .down\n" \
  "  end\n" \
  "  return .back\n" \
  "end\n" \
  "# the arrows as btnp has them: a step one way at a time (left or right before up or down), for moving on a grid\n" \
  "dpadp* :: proc() -> [2]Int\n" \
  "  d := {btnp(.right) - btnp(.left), btnp(.down) - btnp(.up)}\n" \
  "  if d.x != 0 do d.y = 0\n" \
  "  return d\n" \
  "end\n" \
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
  "# 1 while key k is held down (k as key() gives it, without the modifiers: a letter in either case, 32\n" \
  "# Space, 13 Enter, 8 Backspace, 134 Delete, 128 to 131 the arrows...), else 0; -1 where it can't tell (in\n" \
  "# a terminal): for what goes on while a key's held, as drawing with Space held while the arrows move\n" \
  "keyHeld* :: host proc(k: Int) -> Int\n" \
  "# a controller's sticks: 0 left x, 1 left y, 2 right x, 3 right y; -1 to 1, + right and down\n" \
  "stick* :: host proc(i: Int) -> Float\n" \
  "trigger* :: host proc(i: Int) -> Float              # 0 left, 1 right; 0 to 1\n" \
  "appTime* :: host proc() -> Float                    # seconds since the app started (or since appTimeReset)\n" \
  "appTimeReset* :: host proc()                        # appTime from 0 again\n" \
  "# a command, in the list the system key with P shows (or showCommands): an app's own come from its\n" \
  "# commandList* :: proc() -> []pc.Command, asked each time the list opens (up to 240 of them), and\n" \
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
  "# where the pointer is and what it's doing, this frame (each ask the same, all through it)\n" \
  "PointerState* :: struct\n" \
  "  at: [2]Int        # where it points, in screen pixels: {x, y}\n" \
  "  primary: Bool     # its buttons held: the mouse's left (in a ui app, a controller's A, while it's shown)\n" \
  "  secondary: Bool   # its right (B)\n" \
  "  middle: Bool\n" \
  "  shown: Bool       # drawn now\n" \
  "  scrolled: [2]Int  # whole steps scrolled this frame: across (+ right), up and down (+ up)\n" \
  "end\n" \
  "pointerState* :: host proc() -> PointerState\n" \
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
  "# ---------------------------------------------------------------- sound: 16 voices, 0 to 15\n" \
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
  "audioLoad* :: host proc() -> [2]Int # mixing's share of its time, in percent: on average and at most, since last asked ({-1, -1}: not measured here; the board measures it)\n" \
  "# a song, played by Rinc (docs/spec/sound.md#songs): rinc's song library's tables in room (patterns,\n" \
  "# uses, placements, then its sounds' speeds and loops), its sounds' notes in notes, from use from, its\n" \
  "# channels on voices v on; how: 1 just that use, 2 round again at its end, and from bit 8 on how many\n" \
  "# voices it may take (0: 8). Its uses in tracks (a use's marks, bits 4 to 6), played side by side. One at a time\n" \
  "songPlay* :: host proc(v: Int, room: Blob, notes: Blob, patterns: Int, uses: Int, from: Int, how: Int)\n" \
  "songStop* :: host proc()\n" \
  "songPause* :: host proc(on: Int)                     # 1 holds it where it is, 0 goes on\n" \
  "songLevers* :: host proc(shift: Int, volume: Int, speed: Int) # semitones, eighths, percent (speed: from its next placements)\n" \
  "songAt* :: host proc() -> [3]Int                     # the use playing, its time round, thousandths of its pattern played ({-1, 0, 0}: none)\n" \
  "freeVoice* :: host proc() -> Int                     # the first voice not playing and not the song's (-1: none): a game's sounds there\n" \
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
  "# A path is absolute (/save/chord/best), in Rinc's root: looking goes anywhere in it, changing only\n" \
  "# inside it. Or, not starting with /, it's one of the app's own files, in its folder (as a project,\n" \
  "# /projects/chord/, or installed, /apps/chord/): imageRead(\"sprites.png\", pixels), for reading only.\n" \
  "# Or it's in one of the app's own areas, by a word and a colon, then names with / between (folders), each\n" \
  "# 1 to 32 letters, digits, _, - and . (not first); the word and colon alone are the area itself:\n" \
  "#   save:best      its save space (/save/chord/best; a project's own, chord+, in its dev folder)\n" \
  "#   stash:view     its stash (/stash/chord/view): where it was, for its next start, written as it goes;\n" \
  "#                  Rinc drops it all on Sys+Q and Sys+R, where it never touches the save space\n" \
  "#   cache:thumbs   its cache (/cache/chord/thumbs; a project's own): what it can make again, any of it\n" \
  "#                  gone at any time. Rinc keeps the app's log there, as log\n" \
  "#   settings:theme the whole Rinc's settings (/settings/theme), shared by the apps: settings:active is the\n" \
  "#                  app or project being worked on (chord, or chord+)\n" \
  "# What reads into a builder adds to what's in it, as much as fits: out.len() tells how much it holds\n" \
  "dir* :: host proc(path: String, out: Blob[..]) -> Int      # its entries, folders first, a line each (\"d name\", \"f name\"): how many, or -1\n" \
  "fileRead* :: host proc(path: String, out: Blob[..]) -> Int # its whole length, or -1\n" \
  "fileWrite* :: host proc(path: String, data: String) -> Int # 0, or -1\n" \
  "fileReadAt* :: host proc(path: String, at: Int, out: Blob[..]) -> Int # from byte at on, added: its whole length, or -1\n" \
  "fileWriteAt* :: host proc(path: String, at: Int, data: String) -> Int # over it from byte at on (-1: its end), made if need be: where it went, or -1\n" \
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
  "# how imageRead reads one; left out, at its own depth (a 1, 2 or 4 bit one, packed, with its palette)\n" \
  "ImageRead* :: struct\n" \
  "  bytes: Bool    # a byte a pixel, each Rinc's color, whatever its depth: for an app that reads or edits pixels\n" \
  "  from: [2]Int   # just a part of it, size big from from (a size of 0: all of it), read into memory, the rest\n" \
  "  size: [2]Int   # never there: a sheet saved uncompressed (Paint's), else none, and the log says so\n" \
  "end\n" \
  "imageLoad :: host proc(path: String, pixels: Blob[..], o: ImageRead) -> [3]Int # (imageRead's: what it added; its size, its depth)\n" \
  "imageRead* :: proc(path: String, pixels: Blob[..], o?: ImageRead) -> Image\n" \
  "  start := pixels.len()\n" \
  "  got := imageLoad(path, pixels, o)\n" \
  "  size := {got[0], got[1]}\n" \
  "  if got[2] < 8 and size.x > 0 # (packed: its rows, then its palette)\n" \
  "    return {pixels = pixels[start..<pixels.len()], size = size, depth = got[2]}\n" \
  "  end\n" \
  "  return {pixels = pixels[start..<pixels.len()], size = size}\n" \
  "end\n" \
  "# a pixel's color, Rinc's, at p in a picture of any depth (0, see-through, outside it)\n" \
  "imageAt* :: proc(image: Image, p: [2]Int) -> Int\n" \
  "  if p.x < 0 or p.y < 0 or p.x >= image.size.x or p.y >= image.size.y\n" \
  "    return 0\n" \
  "  end\n" \
  "  d := image.depth\n" \
  "  if d <= 0 or d >= 8\n" \
  "    return image.pixels[p.y * image.size.x + p.x]\n" \
  "  end\n" \
  "  row := (image.size.x * d + 7) / 8\n" \
  "  bit := p.x * d\n" \
  "  i := image.pixels[p.y * row + bit / 8] >> (8 - d - bit % 8) & ((1 << d) - 1)\n" \
  "  if row * image.size.y + i < image.pixels.len() do return image.pixels[row * image.size.y + i] # (its palette's)\n" \
  "  return i\n" \
  "end\n" \
  "imageNotes* :: host proc(path: String, out: Blob[..]) -> Int  # how many bytes, or -1 (none)\n" \
  "# a number in t from at on, and where it stops\n" \
  "noteNumber :: proc(t: String, at: Int) -> [2]Int\n" \
  "  v := 0\n" \
  "  i := at\n" \
  "  for i < t.len() and t[i] >= '0' and t[i] <= '9'\n" \
  "    v = v * 10 + t[i] - '0'\n" \
  "    i += 1\n" \
  "  end\n" \
  "  return {v, i}\n" \
  "end\n" \
  "# what a font sheet's notes say (fontNotes): a font (its \"font\" line there), its cells, its characters\n" \
  "# and the rest, as Font has them; proportional, its glyphs to be trimmed\n" \
  "FontNotes* :: struct\n" \
  "  isFont: Bool\n" \
  "  proportional: Bool\n" \
  "  cell: [2]Int\n" \
  "  across: Int\n" \
  "  first: Int\n" \
  "  last: Int\n" \
  "  line: Int\n" \
  "  gap: Int\n" \
  "  space: Int\n" \
  "  colors: Bool\n" \
  "  hasGap: Bool  # (a gap line: else 1 for a proportional font, 0 for a fixed one)\n" \
  "end\n" \
  "# a font sheet's notes, read: a line each, \"font 32 126\" (its first character and its last, in its cells\n" \
  "# across the rows), \"sprite 6x9\" (its cells' size), and as it likes \"proportional\" (each glyph as wide as\n" \
  "# it is, its empty columns trimmed), \"gap 1\" (after each, negative closer: if it's left out, 1 for a\n" \
  "# proportional font, 0 for a fixed one), \"space 3\" (a space's width: half a cell if it's left out),\n" \
  "# \"line 10\", \"across 16\" (its cells across a row: else as many as its picture's width has) and \"colors\"\n" \
  "fontNotes* :: proc(t: String) -> FontNotes\n" \
  "  n := FontNotes{}\n" \
  "  at := 0\n" \
  "  for at < t.len()\n" \
  "    stop := at\n" \
  "    for stop < t.len() and t[stop] != 10 do stop += 1\n" \
  "    l := t[at..<stop]\n" \
  "    if l.len() > 5 and l[0..<5] == \"font \"\n" \
  "      a := noteNumber(l, 5)\n" \
  "      b := noteNumber(l, a[1] + 1)\n" \
  "      n.first = a[0]\n" \
  "      n.last = b[0]\n" \
  "      n.isFont = true\n" \
  "    case l.len() > 7 and l[0..<7] == \"sprite \"\n" \
  "      w := noteNumber(l, 7)\n" \
  "      h := w\n" \
  "      if w[1] < l.len() and l[w[1]] == 'x' do h = noteNumber(l, w[1] + 1)\n" \
  "      n.cell = {w[0], h[0]}\n" \
  "    case l == \"proportional\"\n" \
  "      n.proportional = true\n" \
  "    case l.len() > 4 and l[0..<4] == \"gap \"\n" \
  "      if l[4] == '-'\n" \
  "        n.gap = -noteNumber(l, 5)[0]\n" \
  "      else\n" \
  "        n.gap = noteNumber(l, 4)[0]\n" \
  "      end\n" \
  "      n.hasGap = true\n" \
  "    case l.len() > 6 and l[0..<6] == \"space \"\n" \
  "      n.space = noteNumber(l, 6)[0]\n" \
  "    case l.len() > 5 and l[0..<5] == \"line \"\n" \
  "      n.line = noteNumber(l, 5)[0]\n" \
  "    case l.len() > 7 and l[0..<7] == \"across \"\n" \
  "      n.across = noteNumber(l, 7)[0]\n" \
  "    case l == \"colors\"\n" \
  "      n.colors = true\n" \
  "    end\n" \
  "    at = stop + 1\n" \
  "  end\n" \
  "  return n\n" \
  "end\n" \
  "# a font from a picture and what its notes say (fontNotes), a proportional one's glyphs' widths added to\n" \
  "# room: for a font that isn't in a file yet (Paint's, as it's drawn); not a font: a Font with no cell\n" \
  "fontOf* :: proc(image: Image, n: FontNotes, room: Blob[..]) -> Font\n" \
  "  if not n.isFont or n.cell.x <= 0 or n.cell.y <= 0 or n.last < n.first or image.size.x == 0\n" \
  "    return Font{}\n" \
  "  end\n" \
  "  f := Font{image = image, cell = n.cell, across = n.across, first = n.first, last = n.last, line = n.line, gap = n.gap, colors = n.colors}\n" \
  "  space := n.space\n" \
  "  if space == 0 do space = f.cell.x / 2\n" \
  "  if not n.hasGap and n.proportional do f.gap = 1 # (a proportional font's gap, left out: 1; a fixed one's, 0)\n" \
  "  if not n.proportional\n" \
  "    return f\n" \
  "  end\n" \
  "  start := room.len()\n" \
  "  across := max(1, image.size.x / f.cell.x)\n" \
  "  if f.across > 0 do across = f.across\n" \
  "  for g in 0..<f.last - f.first + 1 # (each glyph: where its columns with any pixel start and end)\n" \
  "    corner := {g % across, g / across} * f.cell\n" \
  "    lo := f.cell.x\n" \
  "    hi := -1\n" \
  "    for x in 0..<f.cell.x\n" \
  "      for y in 0..<f.cell.y\n" \
  "        if imageAt(image, corner + {x, y}) != 0\n" \
  "          lo = min(lo, x)\n" \
  "          hi = max(hi, x)\n" \
  "        end\n" \
  "      end\n" \
  "    end\n" \
  "    if hi < 0 # (empty: a space's width)\n" \
  "      lo = 0\n" \
  "      hi = space - 1\n" \
  "    end\n" \
  "    if not room.push(lo) or not room.push(hi - lo + 1)\n" \
  "      return Font{} # (no room for its widths)\n" \
  "    end\n" \
  "  end\n" \
  "  f.glyphs = room[start..<room.len()]\n" \
  "  return f\n" \
  "end\n" \
  "# a font, from a sheet (Paint's: docs/spec/screen.md#fonts), its picture read into room, then, for a\n" \
  "# proportional one, two bytes for each glyph; its notes say what it is (fontNotes). Not a font (no such\n" \
  "# picture, no room, no \"font\" line): a Font with no cell, which print takes as Rinc's own\n" \
  "fontRead* :: proc(path: String, room: Blob[..], o?: ImageRead) -> Font\n" \
  "  image := imageRead(path, room, o)\n" \
  "  if image.size.x == 0 or image.pixels.len() == 0\n" \
  "    return Font{}\n" \
  "  end\n" \
  "  notes: Blob[..512]\n" \
  "  _ = imageNotes(path, notes)\n" \
  "  n := fontNotes(notes.asString())\n" \
  "  if o.size.x > 0 and o.size.y > 0 and n.cell.y > 0 # (a part: whole rows of its cells, from its first column: those characters)\n" \
  "    across := n.across\n" \
  "    if across <= 0 do across = image.size.x / max(n.cell.x, 1)\n" \
  "    n.first += o.from.y / n.cell.y * across\n" \
  "    n.last = min(n.last, n.first + image.size.y / n.cell.y * across - 1)\n" \
  "    n.across = across\n" \
  "  end\n" \
  "  return fontOf(image, n, room)\n" \
  "end\n" \
  "# how imageWrite writes a picture; left out, as its name ends (.png), with no notes\n" \
  "ImageWrite* :: struct\n" \
  "  notes: String        # the app's notes about it (text, up to 512 bytes, kept in the file: a sprite sheet's sprite size, say)\n" \
  "  format: ImageFormat  # fromName: as its name ends\n" \
  "  flags: []Int         # a sprite sheet's flags: x, y, flags for each sprite with any (8 bits), by its top left pixel\n" \
  "  sheet: Bool          # as a sheet: the colors it uses its own palette, at as few bits as hold them (2 colors, 0\n" \
  "                       # with them: 1 bit; 4: 2; 16: 4; else 8), its rows uncompressed, to be read in place\n" \
  "end\n" \
  "imageWrite* :: host proc(path: String, image: Image, o?: ImageWrite) -> Int # 0, or -1\n" \
  "# a sprite sheet's flags, as imageWrite kept them (docs/spec/apps.md#sprite-flags): imageFlags puts x, y, flags\n" \
  "# in out for each, 3 Ints each, as many as fit; imageSpriteFlags adds a byte for each sprite to out, its\n" \
  "# flags (counted across the rows, its sprites w by h; 0 for none): how many the picture has, or -1 (no\n" \
  "# picture, or no room for a byte a sprite). imageFlag is a flag's bit, by the name its notes give it\n" \
  "# (\"flag 0 solid\": 1), or 0\n" \
  "imageFlags* :: host proc(path: String, out: []Int) -> Int\n" \
  "imageSpriteFlags* :: host proc(path: String, w, h: Int, out: Blob[..]) -> Int\n" \
  "imageFlag* :: host proc(path: String, name: String) -> Int\n" \
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
  "# tables, both the app's own memory, handed over: 0, or -1. app: an app (chord, or a project, chord+)\n" \
  "# whose folder the session's own paths start in, as that app's do (include \"parts/x.rio\", import .x, a\n" \
  "# file it reads: pc.imageRead(\"sheet.png\")); left out, none (only absolute paths, and Rinc's library)\n" \
  "rioStart* :: host proc(mem, tables: Blob, app?: String) -> Int\n" \
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
