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
 *      GUI inline functions (generic C).
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_GUI_INL
#define ALLEGRO_GUI_INL

#ifdef __cplusplus
   extern "C" {
#endif

AL_INLINE(int, object_message, (DIALOG *d, int msg, int c),
{
   int ret;

   if (msg == MSG_DRAW) {
      if (d->flags & D_HIDDEN) return D_O_K;
      acquire_screen();
   }

   ret = d->proc(msg, d, c);

   if (msg == MSG_DRAW)
      release_screen();

   if (ret & D_REDRAWME) {
      d->flags |= D_DIRTY;
      ret &= ~D_REDRAWME;
   }

   return ret;
})

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_GUI_INL */


