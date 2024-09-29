#ifndef __al_included_allegro5_blender_h
#define __al_included_allegro5_blender_h

#include "allegro5/base.h"
#include "allegro5/color.h"

#ifdef __cplusplus
   extern "C" {
#endif


/*
 * Blending modes
 */
enum A5O_BLEND_MODE {
   A5O_ZERO                = 0,
   A5O_ONE                 = 1,
   A5O_ALPHA               = 2,
   A5O_INVERSE_ALPHA       = 3,
   A5O_SRC_COLOR           = 4,
   A5O_DEST_COLOR          = 5,
   A5O_INVERSE_SRC_COLOR   = 6,
   A5O_INVERSE_DEST_COLOR  = 7,
   A5O_CONST_COLOR         = 8,
   A5O_INVERSE_CONST_COLOR = 9,
   A5O_NUM_BLEND_MODES
};

enum A5O_BLEND_OPERATIONS {
   A5O_ADD                = 0,
   A5O_SRC_MINUS_DEST     = 1,
   A5O_DEST_MINUS_SRC     = 2,
   A5O_NUM_BLEND_OPERATIONS
};


AL_FUNC(void, al_set_blender, (int op, int source, int dest));
AL_FUNC(void, al_set_blend_color, (A5O_COLOR color));
AL_FUNC(void, al_get_blender, (int *op, int *source, int *dest));
AL_FUNC(A5O_COLOR, al_get_blend_color, (void));
AL_FUNC(void, al_set_separate_blender, (int op, int source, int dest,
   int alpha_op, int alpha_source, int alpha_dest));
AL_FUNC(void, al_get_separate_blender, (int *op, int *source, int *dest,
   int *alpha_op, int *alpha_src, int *alpha_dest));


#ifdef __cplusplus
   }
#endif

#endif
