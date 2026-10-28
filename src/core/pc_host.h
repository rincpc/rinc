/* Made by rio_hostHeader (rio -H) from the Rio module that declares these host procs: don't edit.
   A struct parameter comes as 2 words: rio_getStruct(vm, a + i, &s, sizeof s) copies it into s. */
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
#define pc_pointer_SIG "i"
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
#define pc_theme_SIG "i>i"
#define pc_boot_SIG ">i"
#define pc_rioStart_SIG "bb>i"
#define pc_rioEval_SIG "s>i"
#define pc_rioFrame_SIG ">i"
#define pc_rioType_SIG "i"
#define pc_rioError_SIG "b>i"
#define pc_rioLog_SIG "b>i"
#define pc_rioIndent_SIG "s>i"
