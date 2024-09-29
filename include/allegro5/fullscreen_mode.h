#ifndef __al_included_allegro5_fullscreen_mode_h
#define __al_included_allegro5_fullscreen_mode_h

#include "allegro5/base.h"

#ifdef __cplusplus
   extern "C" {
#endif


/* Type: A5O_DISPLAY_MODE
 */
typedef struct A5O_DISPLAY_MODE
{
   int width;
   int height;
   int format;
   int refresh_rate;
} A5O_DISPLAY_MODE;


AL_FUNC(int, al_get_num_display_modes, (void));
AL_FUNC(A5O_DISPLAY_MODE*, al_get_display_mode, (int index,
        A5O_DISPLAY_MODE *mode));


#ifdef __cplusplus
   }
#endif

#endif

/* vim: set ts=8 sts=3 sw=3 et: */
