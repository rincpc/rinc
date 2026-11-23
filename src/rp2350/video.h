/* HDMI out, run by core 1, showing rinc_screen (see video.c) */
#ifndef rinc_VIDEO_H
#define rinc_VIDEO_H
#include <stdint.h>
void rinc_videoStart(void);
void rinc_videoSleep(int on); /* rinc_sleeping: no signal (on), or back */
void rinc_hdmiSound(const int16_t *stereo, int frames); /* mixed sound, for HDMI too (core 1 only) */
extern volatile uint32_t rinc_frames; /* frames shown: it changes at each vertical blank */
extern volatile uint32_t rinc_shownCopies; /* whole frames copied to the screen shown: it changes as one's shown */
/* after a frame: if none was shown since copies (rinc_shownCopies then), wait (poll meanwhile) till this one
   has been copied whole, at most a little over a 60th of a second */
void rinc_waitShown(uint32_t copies, void (*poll)(void));
extern volatile int rinc_drawing; /* set while an app's frame is under way: the blank doesn't copy a half-drawn screen (a slow update: a check in the editor) */
#endif
