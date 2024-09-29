#ifndef __al_included_allegro5_monitor_h
#define __al_included_allegro5_monitor_h

#include "allegro5/base.h"

#ifdef __cplusplus
   extern "C" {
#endif


/* Type: A5O_MONITOR_INFO
 */
typedef struct A5O_MONITOR_INFO
{
   int x1;
   int y1;
   int x2;
   int y2;
} A5O_MONITOR_INFO;

enum {
   A5O_DEFAULT_DISPLAY_ADAPTER = -1
};

AL_FUNC(int, al_get_num_video_adapters, (void));
AL_FUNC(bool, al_get_monitor_info, (int adapter, A5O_MONITOR_INFO *info));
AL_FUNC(int, al_get_monitor_dpi, (int adapter));
#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_SRC)
AL_FUNC(int, al_get_monitor_refresh_rate, (int adapter));
#endif

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set ts=8 sts=3 sw=3 et: */
