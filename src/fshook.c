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
   AL_METHOD(AL_FILE *, fopen, (const char *path, const char *mode) );
   AL_METHOD(uint32_t, fclose, (AL_FILE *fp) );
   AL_METHOD(size_t, fread, (void *ptr, size_t size, AL_FILE *fp) );
   AL_METHOD(size_t, fwrite, (const void *ptr, size_t size, AL_FILE *fp) );
   AL_METHOD(uint32_t, fflush, (AL_FILE *fp) );
   AL_METHOD(uint32_t, fseek, (AL_FILE *fp, uint32_t offset, uint32_t whence) );
   AL_METHOD(uint32_t, ftell, (AL_FILE *fp) );

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
