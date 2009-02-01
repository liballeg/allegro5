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


#define IS_SINGLE_BYTE(c)  (((unsigned)(c) & 0x80) == 0)
#define IS_LEAD_BYTE(c)    (((unsigned)(c) - 0xC0) < 0x3E)
#define IS_TRAIL_BYTE(c)   (((unsigned)(c) & 0xC0) == 0x80)


static bool all_ascii(const ALLEGRO_USTR us)
{
   const unsigned char *data = (const unsigned char *) _al_bdata(us.b);
   int size = _al_blength(us.b);

   while (size-- > 0) {
      if (*data > 127)
         return false;
      data++;
   }

   return true;
}


/* Function: al_ustr_new
 */
ALLEGRO_USTR al_ustr_new(const char *s)
{
   struct ALLEGRO_USTR tmp = { _al_bfromcstr(s) };
   return tmp;
}


/* Function: al_ustr_new_from_buffer
 */
ALLEGRO_USTR al_ustr_new_from_buffer(const char *s, size_t size)
{
   struct ALLEGRO_USTR tmp = { _al_blk2bstr(s, size) };
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


/* Function: al_cstr_dup
 */
char *al_cstr_dup(const ALLEGRO_USTR us)
{
   return _al_bstr2cstr(us.b, '\0');
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
size_t al_ustr_size(const ALLEGRO_USTR us)
{
   return _al_blength(us.b);
}


/* Function: al_ustr_length
 */
size_t al_ustr_length(const ALLEGRO_USTR us)
{
   int pos = 0;
   int c = 0;

   while (al_ustr_next(us, &pos))
      c++;

   return c;
}


/* Function: al_ustr_offset
 */
int al_ustr_offset(const ALLEGRO_USTR us, int index)
{
   int pos = 0;

   if (index < 0)
      index += al_ustr_length(us);

   while (index-- > 0) {
      if (!al_ustr_next(us, &pos))
         return pos;
   }

   return pos;
}


/* Function: al_ustr_next
 */
bool al_ustr_next(const ALLEGRO_USTR us, int *pos)
{
   const unsigned char *data = (const unsigned char *) _al_bdata(us.b);
   int size = _al_blength(us.b);
   int c;

   if (*pos >= size) {
      return false;
   }

   while (++(*pos) < size) {
      c = data[*pos];
      if (IS_SINGLE_BYTE(c) || IS_LEAD_BYTE(c))
         break;
   }

   return true;
}


/* Function: al_ustr_prev
 */
bool al_ustr_prev(const ALLEGRO_USTR us, int *pos)
{
   const unsigned char *data = (const unsigned char *) _al_bdata(us.b);
   int c;

   if (*pos <= 0)
      return false;

   while (*pos > 0) {
      (*pos)--;
      c = data[*pos];
      if (IS_SINGLE_BYTE(c) || IS_LEAD_BYTE(c))
         break;
   }

   return true;
}


/* Function: al_ustr_get
 */
int32_t al_ustr_get(const ALLEGRO_USTR ub, int pos)
{
   int32_t c;
   int remain;
   int32_t minc;
   const unsigned char *data;

   c = _al_bchare(ub.b, pos, -1);

   if (c < 0) {
      /* Out of bounds. */
      al_set_errno(ERANGE);
      return -1;
   }

   if (c <= 0x7F) {
      /* Plain ASCII. */
      return c;
   }

   if (c <= 0xC1) {
      /* Trailing byte of multi-byte sequence or an overlong encoding for
       * code point <= 127.
       */
      al_set_errno(EILSEQ);
      return -2;
   }

   if (c <= 0xDF) {
      /* 2-byte sequence. */
      c &= 0x1F;
      remain = 1;
      minc = 0x80;
   }
   else if (c <= 0xEF) {
      /* 3-byte sequence. */
      c &= 0x0F;
      remain = 2;
      minc = 0x800;
   }
   else if (c <= 0xF4) {
      /* 4-byte sequence. */
      c &= 0x07;
      remain = 3;
      minc = 0x10000;
   }
   else {
      /* Otherwise invalid. */
      al_set_errno(EILSEQ);
      return -2;
   }

   if (pos + remain > _al_blength(ub.b)) {
      al_set_errno(EILSEQ);
      return -2;
   }

   data = (const unsigned char *) _al_bdata(ub.b);
   while (remain--) {
      int d = data[++pos];

      if (!IS_TRAIL_BYTE(d)) {
         al_set_errno(EILSEQ);
         return -2;
      }

      c = (c << 6) | (d & 0x3F);
   }

   /* Check for overlong forms, which could be used to bypass security
    * validations.  We could also check code points aren't above U+10FFFF or in
    * the surrogate ranges, but we don't.
    */

   if (c < minc) {
      al_set_errno(EILSEQ);
      return -2;
   }

   return c;
}


/* Function: al_ustr_get_next
 */
int32_t al_ustr_get_next(const ALLEGRO_USTR us, int *pos)
{
   int32_t c = al_ustr_get(us, *pos);

   if (c >= 0) {
      (*pos) += al_utf8_width(c);
      return c;
   }

   if (c == -1) {
      /* Past end. */
      return c;
   }

   /* Some invalid byte sequence. */
   al_ustr_next(us, pos);
   return c;
}


/* Function: al_ustr_prev_get
 */
int32_t al_ustr_prev_get(const ALLEGRO_USTR us, int *pos)
{
   if (al_ustr_prev(us, pos)) {
      return al_ustr_get(us, *pos);
   }

   /* Past beginning. */
   return -1;
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


/* Function: al_ustr_find_chr
 */
int al_ustr_find_chr(const ALLEGRO_USTR us, int start_pos, int32_t c)
{
   char encc[4];
   size_t sizec;
   struct _al_tagbstring enctb;
   int rc;

   /* Fast path for ASCII characters. */
   if (c < 128) {
      rc = _al_bstrchrp(us.b, c, start_pos);
      return (rc == _AL_BSTR_ERR) ? -1 : rc;
   }

   /* Non-ASCII.  We can simply encode the character into a string and search
    * for that.
    */

   sizec = al_utf8_encode(encc, c);
   if (!sizec) {
      al_set_errno(EINVAL);
      return -1; /* error */
   }

   _al_blk2tbstr(enctb, encc, sizec);
   rc = _al_binstr(us.b, start_pos, &enctb);
   return (rc == _AL_BSTR_ERR) ? -1 : rc;
}


/* Function: al_ustr_rfind_chr
 */
int al_ustr_rfind_chr(const ALLEGRO_USTR us, int end_pos, int32_t c)
{
   char encc[4];
   size_t sizec;
   struct _al_tagbstring enctb;
   int rc;

   /* Fast path for ASCII characters. */
   if (c < 128) {
      rc = _al_bstrrchrp(us.b, c, end_pos - 1);
      return (rc == _AL_BSTR_ERR) ? -1 : rc;
   }

   /* Non-ASCII.  We can simply encode the character into a string and search
    * for that.
    */

   sizec = al_utf8_encode(encc, c);
   if (!sizec) {
      al_set_errno(EINVAL);
      return -1; /* error */
   }

   _al_blk2tbstr(enctb, encc, sizec);
   rc = _al_binstrr(us.b, end_pos - sizec, &enctb);
   return (rc == _AL_BSTR_ERR) ? -1 : rc;
}


/* Function: al_ustr_find_set
 */
int al_ustr_find_set(const ALLEGRO_USTR us, int start_pos,
   const ALLEGRO_USTR accept)
{
   int rc;
   int32_t c, d;
   int pos;
   int set_pos;

   /* Fast path for ASCII characters. */
   if (all_ascii(accept)) {
      rc = _al_binchr(us.b, start_pos, accept.b);
      return (rc == _AL_BSTR_ERR) ? -1 : rc;
   }

   /* Non-ASCII. */
   pos = 0;
   while ((c = al_ustr_get(us, pos)) != -1) {
      if (c == -2) {
         /* Invalid byte sequence. */
         pos++;
         continue;
      }

      set_pos = 0;
      while ((d = al_ustr_get_next(accept, &set_pos)) != -1) {
         if (c == d)
            return pos;
      }

      pos += al_utf8_width(c);
   }

   return -1;
}


/* Function: al_ustr_find_set_cstr
 */
int al_ustr_find_set_cstr(const ALLEGRO_USTR us, int start_pos,
   const char *accept)
{
   ALLEGRO_USTR_INFO info;
   ALLEGRO_USTR accept_us = al_ref_cstr(&info, accept);

   return al_ustr_find_set(us, start_pos, accept_us);
}


/* Function: al_ustr_find_cset
 */
int al_ustr_find_cset(const ALLEGRO_USTR us, int start_pos,
   const ALLEGRO_USTR reject)
{
   int rc;
   int32_t c, d;
   int pos;
   int set_pos;

   /* Fast path for ASCII characters. */
   if (all_ascii(reject)) {
      rc = _al_bninchr(us.b, start_pos, reject.b);
      return (rc == _AL_BSTR_ERR) ? -1 : rc;
   }

   /* Non-ASCII. */
   pos = 0;
   while ((c = al_ustr_get(us, pos)) != -1) {
      if (c == -2) {
         /* Invalid byte sequence. */
         pos++;
         continue;
      }

      set_pos = 0;
      while ((d = al_ustr_get_next(reject, &set_pos)) != -1) {
         if (c == d)
            break;
      }

      if (d == -1) {
         return pos;
      }

      pos += al_utf8_width(c);
   }

   return -1;
}


/* Function: al_ustr_find_cset_cstr
 */
int al_ustr_find_cset_cstr(const ALLEGRO_USTR us, int start_pos,
   const char *reject)
{
   ALLEGRO_USTR_INFO info;
   ALLEGRO_USTR reject_us = al_ref_cstr(&info, reject);

   return al_ustr_find_cset(us, start_pos, reject_us);
}


/* Function: al_ustr_find_str
 */
int al_ustr_find_str(const ALLEGRO_USTR haystack, int start_pos,
   const ALLEGRO_USTR needle)
{
   int rc = _al_binstr(haystack.b, start_pos, needle.b);
   return (rc == _AL_BSTR_ERR) ? -1 : rc;
}


/* Function: al_ustr_find_cstr
 */
int al_ustr_find_cstr(const ALLEGRO_USTR haystack, int start_pos,
   const char *needle)
{
   ALLEGRO_USTR_INFO info;
   ALLEGRO_USTR needle_us = al_ref_cstr(&info, needle);

   return al_ustr_find_str(haystack, start_pos, needle_us);
}


/* Function: al_ustr_rfind_str
 */
int al_ustr_rfind_str(const ALLEGRO_USTR haystack, int end_pos,
   const ALLEGRO_USTR needle)
{
   int rc = _al_binstrr(haystack.b, end_pos - _al_blength(needle.b), needle.b);
   return (rc == _AL_BSTR_ERR) ? -1 : rc;
}


/* Function: al_ustr_rfind_cstr
 */
int al_ustr_rfind_cstr(const ALLEGRO_USTR haystack, int end_pos,
   const char *needle)
{
   ALLEGRO_USTR_INFO info;
   ALLEGRO_USTR needle_us = al_ref_cstr(&info, needle);

   return al_ustr_rfind_str(haystack, end_pos, needle_us);
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
