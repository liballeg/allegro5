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
 *      File inline functions (generic C).
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_FILE_INL
#define ALLEGRO_FILE_INL

#ifdef __cplusplus
   extern "C" {
#endif

AL_INLINE(int, pack_getc, (PACKFILE *f),
{
   ASSERT(f);
   ASSERT(f->vtable);
   ASSERT(f->vtable->pf_getc);

   return f->vtable->pf_getc(f->userdata);
})


AL_INLINE(int, pack_putc, (int c, PACKFILE *f),
{
   ASSERT(f);
   ASSERT(f->vtable);
   ASSERT(f->vtable->pf_putc);

   return f->vtable->pf_putc(c, f->userdata);
})


AL_INLINE(int, pack_feof, (PACKFILE *f),
{
   ASSERT(f);
   ASSERT(f->vtable);
   ASSERT(f->vtable->pf_feof);

   return f->vtable->pf_feof(f->userdata);
})


AL_INLINE(int, pack_ferror, (PACKFILE *f),
{
   ASSERT(f);
   ASSERT(f->vtable);
   ASSERT(f->vtable->pf_ferror);

   return f->vtable->pf_ferror(f->userdata);
})

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_FILE_INL */


