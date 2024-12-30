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
 *      Copies of the inline functions in allegro.h, in case anyone needs
 *      to take the address of them, or is compiling without optimisation.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/platform/alplatf.h"

// apple's cross-compiler already adds the symbols for the "extern __inline__"
// declared variants, so the ones here are duplicates and the linker dies
#ifndef ALLEGRO_IPHONE

#define AL_INLINE(type, name, args, code)    \
   extern type name args; \
   type name args code

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"

#ifdef ALLEGRO_INTERNAL_HEADER
   #include ALLEGRO_INTERNAL_HEADER
#endif

/* not used now */
/* #include "allegro5/internal/aintern_atomicops.h" */

#include "allegro5/internal/aintern_float.h"
#include "allegro5/internal/aintern_vector.h"

#endif
