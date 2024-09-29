#ifndef __al_included_allegro5_render_state_h
#define __al_included_allegro5_render_state_h

#include "allegro5/base.h"

#ifdef __cplusplus
   extern "C" {
#endif

/* Enum: A5O_RENDER_STATE
 */
typedef enum A5O_RENDER_STATE {
   /* A5O_ALPHA_TEST was the name of a rare bitmap flag only used on the
    * Wiz port.  Reuse the name but retain the same value.
    */
   A5O_ALPHA_TEST = 0x0010,
   A5O_WRITE_MASK,
   A5O_DEPTH_TEST,
   A5O_DEPTH_FUNCTION,
   A5O_ALPHA_FUNCTION,
   A5O_ALPHA_TEST_VALUE
} A5O_RENDER_STATE;

/* Enum: A5O_RENDER_FUNCTION
 */
typedef enum A5O_RENDER_FUNCTION {
   A5O_RENDER_NEVER,
   A5O_RENDER_ALWAYS,
   A5O_RENDER_LESS, 
   A5O_RENDER_EQUAL,     
   A5O_RENDER_LESS_EQUAL,       
   A5O_RENDER_GREATER,        
   A5O_RENDER_NOT_EQUAL, 
   A5O_RENDER_GREATER_EQUAL
} A5O_RENDER_FUNCTION;

/* Enum: A5O_WRITE_MASK_FLAGS
 */
typedef enum A5O_WRITE_MASK_FLAGS {
   A5O_MASK_RED = 1 << 0,
   A5O_MASK_GREEN = 1 << 1,
   A5O_MASK_BLUE = 1 << 2,
   A5O_MASK_ALPHA = 1 << 3,
   A5O_MASK_DEPTH = 1 << 4,
   A5O_MASK_RGB = (A5O_MASK_RED | A5O_MASK_GREEN | A5O_MASK_BLUE),
   A5O_MASK_RGBA = (A5O_MASK_RGB | A5O_MASK_ALPHA)
} A5O_WRITE_MASK_FLAGS;

AL_FUNC(void, al_set_render_state, (A5O_RENDER_STATE state, int value));

#ifdef __cplusplus
   }
#endif

#endif
