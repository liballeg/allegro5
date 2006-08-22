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
 *      File System Hooks.
 *
 *      By Thomas Fjellstrom.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/fshook.h"
#include ALLEGRO_INTERNAL_HEADER

#define AL_FS_HOOK_NONE AL_ID('N','O','N','E')

struct AL_FSHOOK_VTABLE {
   uint32_t id;
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
   AL_METHOD(uint32_t, name, (...) );
};

/* for you freaks running vim/emacs. */
/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
