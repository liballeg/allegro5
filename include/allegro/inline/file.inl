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
   f->buf_size--;
   if (f->buf_size > 0)
      return *(f->buf_pos++);
   else
      return _sort_out_getc(f);
})


AL_INLINE(int, pack_putc, (int c, PACKFILE *f),
{
   f->buf_size++;
   if (f->buf_size >= F_BUF_SIZE)
      return _sort_out_putc(c, f);
   else
      return (*(f->buf_pos++) = c);
})


AL_INLINE(int, pack_feof, (PACKFILE *f),
{
   return (f->flags & PACKFILE_FLAG_EOF);
})


AL_INLINE(int, pack_ferror, (PACKFILE *f),
{
   return (f->flags & PACKFILE_FLAG_ERROR);
})

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_FILE_INL */


