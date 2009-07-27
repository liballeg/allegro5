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
 */
enum ALLEGRO_STATE_FLAGS {
    ALLEGRO_STATE_NEW_DISPLAY_PARAMETERS = 0x0001,
    ALLEGRO_STATE_NEW_BITMAP_PARAMETERS  = 0x0002,
    ALLEGRO_STATE_DISPLAY                = 0x0004,
    ALLEGRO_STATE_TARGET_BITMAP          = 0x0008,
    ALLEGRO_STATE_BLENDER                = 0x0010,
    ALLEGRO_STATE_NEW_FILE_INTERFACE     = 0x0020,

    ALLEGRO_STATE_BITMAP                 = ALLEGRO_STATE_TARGET_BITMAP +\
                                           ALLEGRO_STATE_NEW_BITMAP_PARAMETERS,

    ALLEGRO_STATE_ALL                    = 0xffff
    
};


/* Type: ALLEGRO_STATE
 */
typedef struct ALLEGRO_STATE ALLEGRO_STATE;

struct ALLEGRO_STATE
{
   /* Internally, a thread_local_state structure is placed here. */
   char _tls[256];
   int flags;
};


AL_FUNC(void, al_store_state, (ALLEGRO_STATE *state, int flags));
AL_FUNC(void, al_restore_state, (ALLEGRO_STATE const *state));


#ifdef __cplusplus
   }
#endif

#endif /* ifndef ALLEGRO_TLS_H */
