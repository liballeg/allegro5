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
   struct ALLEGRO_USTR tmp = { _al_bfromcstr(s) };
   return tmp;
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
   struct ALLEGRO_USTR tmp = { _al_bstrcpy(us.b) };
   return tmp;
}


/* Function: al_ustr_dup_substr
 */
ALLEGRO_USTR al_ustr_dup_substr(const ALLEGRO_USTR us, int start_pos,
   int end_pos)
{
   struct ALLEGRO_USTR tmp = { _al_bmidstr(us.b, start_pos, end_pos - start_pos) };
   return tmp;
}


/* Function: al_ustr_empty_string
 */
ALLEGRO_USTR al_ustr_empty_string(void)
{
   static struct _al_tagbstring empty = _al_bsStatic("");
   struct ALLEGRO_USTR tmp = { &empty };
   return tmp;
}


/* Function: al_ref_cstr
 */
ALLEGRO_USTR al_ref_cstr(ALLEGRO_USTR_INFO *info, const char *s)
{
   struct _al_tagbstring *tb = (struct _al_tagbstring *) info;
   ASSERT(info);
   ASSERT(s);

   _al_btfromcstr(*tb, s);
   {
   struct ALLEGRO_USTR tmp = { tb };
   return tmp;
   }
}


/* Function: al_ref_buffer
 */
ALLEGRO_USTR al_ref_buffer(ALLEGRO_USTR_INFO *info, const char *s, size_t size)
{
   struct _al_tagbstring *tb = (struct _al_tagbstring *) info;
   ASSERT(s);

   _al_blk2tbstr(*tb, s, size);
   {
   struct ALLEGRO_USTR tmp = { tb };
   return tmp;
   }
}


/* Function: al_ref_ustr
 */
ALLEGRO_USTR al_ref_ustr(ALLEGRO_USTR_INFO *info, const ALLEGRO_USTR us,
   int start_pos, int end_pos)
{
   struct _al_tagbstring *tb = (struct _al_tagbstring *) info;

   _al_bmid2tbstr(*tb, us.b, start_pos, end_pos - start_pos);
   {
   struct ALLEGRO_USTR tmp = { tb };
   return tmp;
   }
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


/* Function: al_ustr_insert_chr
 */
size_t al_ustr_insert_chr(ALLEGRO_USTR us, int pos, int32_t c)
{
   uint32_t uc = c;
   size_t sz;

   if (uc < 128) {
      return (_al_binsertch(us.b, pos, 1, uc) == _AL_BSTR_OK) ? 1 : 0;
   }

   sz = al_utf8_width(c);
   if (_al_binsertch(us.b, pos, sz, '\0') == _AL_BSTR_OK) {
      return al_utf8_encode(_al_bdataofs(us.b, pos), c);
   }

   return 0;
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


/* Function: al_ustr_append_chr
 */
size_t al_ustr_append_chr(ALLEGRO_USTR us, int32_t c)
{
   uint32_t uc = c;

   if (uc < 128) {
      return (_al_bconchar(us.b, uc) == _AL_BSTR_OK) ? 1 : 0;
   }

   return al_ustr_insert_chr(us, al_ustr_size(us), c);
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


/* Function: al_utf8_width
 */
size_t al_utf8_width(int c)
{
   /* So we don't need to check for negative values nor use unsigned ints
    * in the interface, which are a pain.
    */
   uint32_t uc = c;

   if (uc <= 0x7f)
      return 1;
   if (uc <= 0x7ff)
      return 2;
   if (uc <= 0xffff)
      return 3;
   if (uc <= 0x10ffff)
      return 4;
   /* The rest are illegal. */
   return 0;
}


/* Function: al_utf8_encode
 */
size_t al_utf8_encode(char s[], int32_t c)
{
   uint32_t uc = c;

   if (uc <= 0x7f) {
      s[0] = uc;
      return 1;
   }

   if (uc <= 0x7ff) {
      s[0] = 0xC0 | ((uc >> 6) & 0x1F);
      s[1] = 0x80 |  (uc       & 0x3F);
      return 2;
   }

   if (uc <= 0xffff) {
      s[0] = 0xE0 | ((uc >> 12) & 0x0F);
      s[1] = 0x80 | ((uc >>  6) & 0x3F);
      s[2] = 0x80 |  (uc        & 0x3F);
      return 3;
   }

   if (uc <= 0x10ffff) {
      s[0] = 0xF0 | ((uc >> 18) & 0x07);
      s[1] = 0x80 | ((uc >> 12) & 0x3F);
      s[2] = 0x80 | ((uc >>  6) & 0x3F);
      s[3] = 0x80 |  (uc        & 0x3F);
      return 4;
   }

   /* Otherwise is illegal. */
   return 0;
}

/* vim: set sts=3 sw=3 et: */
