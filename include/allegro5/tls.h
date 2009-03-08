/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Thread local storage routines.
 *
 *      See readme.txt for copyright information.
 */

/* Title: Thread local storage
 */

#ifndef ALLEGRO_TLS_H
#define ALLEGRO_TLS_H

#include "allegro5/base.h"

#ifdef __cplusplus
   extern "C" {
#endif

/* Enum: ALLEGRO_STATE_FLAGS
 * 
 * Flags which can be passed to <al_store_state>/<al_restore_state> as bit
 * combinations. The following flags store or restore settings corresponding
 * to the following al_set_/al_get_ calls:
 * 
 * ALLEGRO_STATE_NEW_DISPLAY_PARAMETERS - new_display_refresh_rate,
 *                                        new_display_flags
 * ALLEGRO_STATE_NEW_BITMAP_PARAMETERS  - new_bitmap_format,
 *                                        new_bitmap_flags
 * ALLEGRO_STATE_DISPLAY                - current_display
 * ALLEGRO_STATE_TARGET_BITMAP          - target_bitmap 
 * ALLEGRO_STATE_BLENDER                - blender
 */
enum ALLEGRO_STATE_FLAGS {
    ALLEGRO_STATE_NEW_DISPLAY_PARAMETERS = 0x0001,
    ALLEGRO_STATE_NEW_BITMAP_PARAMETERS  = 0x0002,
    ALLEGRO_STATE_DISPLAY                = 0x0004,
    ALLEGRO_STATE_TARGET_BITMAP          = 0x0008,
    ALLEGRO_STATE_BLENDER                = 0x0010,
    
    ALLEGRO_STATE_BITMAP                 = ALLEGRO_STATE_TARGET_BITMAP +\
                                           ALLEGRO_STATE_NEW_BITMAP_PARAMETERS,

    ALLEGRO_STATE_ALL                    = 0xffff
    
};

/* Type: ALLEGRO_STATE
 * 
 * Opaque type which is passed to <al_store_state>/<al_restore_state>.
 */
typedef struct ALLEGRO_STATE
{
   /* Internally, a thread_local_state structure is placed here. */
   char _tls[256];
   int flags;
} ALLEGRO_STATE;

AL_FUNC(void, al_store_state, (ALLEGRO_STATE *state, int flags));
AL_FUNC(void, al_restore_state, (ALLEGRO_STATE const *state));

#ifdef __cplusplus
   }
#endif

#endif /* ifndef ALLEGRO_TLS_H */
