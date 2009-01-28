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
const char *al_cstr(const ALLEGRO_USTR us)
{
   /* May or may not be NUL terminated. */
   return _al_bdata(us.b);
}


/* Function: al_ustr_dup
 */
ALLEGRO_USTR al_ustr_dup(const ALLEGRO_USTR us)
{
   return (ALLEGRO_USTR) { _al_bstrcpy(us.b) };
}


/* Function: al_ustr_dup_substr
 */
ALLEGRO_USTR al_ustr_dup_substr(const ALLEGRO_USTR us, int start_pos,
   int end_pos)
{
   return (ALLEGRO_USTR) { _al_bmidstr(us.b, start_pos, end_pos - start_pos) };
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


/* Function: al_ustr_insert
 */
bool al_ustr_insert(ALLEGRO_USTR us1, int pos, const ALLEGRO_USTR us2)
{
   return _al_binsert(us1.b, pos, us2.b, '\0') == _AL_BSTR_OK;
}


/* Function: al_ustr_insert_cstr
 */
bool al_ustr_insert_cstr(ALLEGRO_USTR us, int pos, const char *s)
{
   ALLEGRO_USTR_INFO info;

   return al_ustr_insert(us, pos, al_ref_cstr(&info, s));
}


/* Function: al_ustr_append
 */
bool al_ustr_append(ALLEGRO_USTR us1, const ALLEGRO_USTR us2)
{
   return _al_bconcat(us1.b, us2.b) == _AL_BSTR_OK;
}


/* Function: al_ustr_append_cstr
 */
bool al_ustr_append_cstr(ALLEGRO_USTR us, const char *s)
{
   return _al_bcatcstr(us.b, s) == _AL_BSTR_OK;
}


/* Function: al_ustr_remove_range
 */
bool al_ustr_remove_range(ALLEGRO_USTR us, int start_pos, int end_pos)
{
   return _al_bdelete(us.b, start_pos, end_pos - start_pos) == _AL_BSTR_OK;
}


/* Function: al_ustr_truncate
 */
bool al_ustr_truncate(ALLEGRO_USTR us, int start_pos)
{
   return _al_btrunc(us.b, start_pos) == _AL_BSTR_OK;
}


/* Function: al_ustr_ltrim_ws
 */
bool al_ustr_ltrim_ws(ALLEGRO_USTR us)
{
   return _al_bltrimws(us.b) == _AL_BSTR_OK;
}


/* Function: al_ustr_rtrim_ws
 */
bool al_ustr_rtrim_ws(ALLEGRO_USTR us)
{
   return _al_brtrimws(us.b) == _AL_BSTR_OK;
}


/* Function: al_ustr_trim_ws
 */
bool al_ustr_trim_ws(ALLEGRO_USTR us)
{
   return _al_btrimws(us.b) == _AL_BSTR_OK;
}


/* Function: al_ustr_equal
 */
bool al_ustr_equal(const ALLEGRO_USTR us1, const ALLEGRO_USTR us2)
{
   return _al_biseq(us1.b, us2.b) == 1;
}


/* vim: set sts=3 sw=3 et: */
