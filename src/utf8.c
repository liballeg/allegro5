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
 *      UTF-8 string handling functions.
 *
 *      By Peter Wang.
 *
 *      See LICENSE.txt for copyright information.
 */


#include "allegro5/allegro5.h"
#include "allegro5/utf8.h"
#include "allegro5/internal/bstrlib.h"


/* Function: al_ustr_new
 */
ALLEGRO_USTR al_ustr_new(const char *s)
{
   return (ALLEGRO_USTR) { _al_bfromcstr(s) };
}


/* Function: al_ustr_free
 */
void al_ustr_free(ALLEGRO_USTR us)
{
   _al_bdestroy(us.b);
}


/* Function: al_cstr
 */
const char *al_cstr(ALLEGRO_USTR us)
{
   /* May or may not be NUL terminated. */
   return _al_bdata(us.b);
}


/* Function: al_ustr_empty_string
 */
ALLEGRO_USTR al_ustr_empty_string(void)
{
   static struct _al_tagbstring empty = _al_bsStatic("");
   return (ALLEGRO_USTR) { &empty };
}


/* Function: al_ref_cstr
 */
ALLEGRO_USTR al_ref_cstr(ALLEGRO_USTR_INFO *info, const char *s)
{
   struct _al_tagbstring *tb = (struct _al_tagbstring *) info;
   ASSERT(info);
   ASSERT(s);

   _al_btfromcstr(*tb, s);
   return (ALLEGRO_USTR) { tb };
}


/* Function: al_ref_buffer
 */
ALLEGRO_USTR al_ref_buffer(ALLEGRO_USTR_INFO *info, const char *s, size_t size)
{
   struct _al_tagbstring *tb = (struct _al_tagbstring *) info;
   ASSERT(s);

   _al_blk2tbstr(*tb, s, size);
   return (ALLEGRO_USTR) { tb };
}


/* Function: al_ref_ustr
 */
ALLEGRO_USTR al_ref_ustr(ALLEGRO_USTR_INFO *info, const ALLEGRO_USTR us,
   int start_pos, int end_pos)
{
   struct _al_tagbstring *tb = (struct _al_tagbstring *) info;

   _al_bmid2tbstr(*tb, us.b, start_pos, end_pos - start_pos);
   return (ALLEGRO_USTR) { tb };
}


/* Function: al_ustr_size
 */
size_t al_ustr_size(ALLEGRO_USTR us)
{
   return _al_blength(us.b);
}


/* vim: set sts=3 sw=3 et: */
