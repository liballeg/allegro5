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


#include <stdarg.h>
#include "allegro5/allegro.h"
#include "allegro5/utf8.h"
#include "allegro5/internal/bstrlib.h"
#include "allegro5/internal/aintern.h"

A5O_STATIC_ASSERT(utf8,
   sizeof(A5O_USTR_INFO) >= sizeof(struct _al_tagbstring));

#ifdef A5O_MSVC
   #pragma warning (disable: 4066)
#endif

#ifndef A5O_HAVE_VA_COPY
   /* If va_copy() is not defined we assume that a simple assignment suffices.
    * From a few web searches, this appears to be true for MSVC 7.
    */
   #define va_copy(a, b)   ((a) = (b))
#endif


#define IS_SINGLE_BYTE(c)  (((unsigned)(c) & 0x80) == 0)
#define IS_LEAD_BYTE(c)    (((unsigned)(c) - 0xC0) < 0x3E)
#define IS_TRAIL_BYTE(c)   (((unsigned)(c) & 0xC0) == 0x80)


static bool all_ascii(const A5O_USTR *us)
{
   const unsigned char *data = (const unsigned char *) _al_bdata(us);
   int size = _al_blength(us);

   while (size-- > 0) {
      if (*data > 127)
         return false;
      data++;
   }

   return true;
}


/* Function: al_ustr_new
 */
A5O_USTR *al_ustr_new(const char *s)
{
   return _al_bfromcstr(s);
}


/* Function: al_ustr_new_from_buffer
 */
A5O_USTR *al_ustr_new_from_buffer(const char *s, size_t size)
{
   return _al_blk2bstr(s, size);
}


/* Function: al_ustr_newf
 */
A5O_USTR *al_ustr_newf(const char *fmt, ...)
{
   A5O_USTR *us;
   va_list ap;

   us = al_ustr_new("");
   va_start(ap, fmt);
   al_ustr_vappendf(us, fmt, ap);
   va_end(ap);
   return us;
}


/* Function: al_ustr_free
 */
void al_ustr_free(A5O_USTR *us)
{
   _al_bdestroy(us);
}


/* Function: al_cstr
 */
const char *al_cstr(const A5O_USTR *us)
{
   /* May or may not be NUL terminated. */
   return _al_bdata(us);
}


/* Function: al_ustr_to_buffer
 */
void al_ustr_to_buffer(const A5O_USTR *us, char *buffer, int size)
{
   int need;

   if (size <= 0)
      return;
   /* add 1 for terminating 0 byte */
   need = _al_blength(us) + 1;
   if (size > need)
      size = need;
   _al_sane_strncpy(buffer, _al_bdata(us), size);
}


/* Function: al_cstr_dup
 */
char *al_cstr_dup(const A5O_USTR *us)
{
   return _al_bstr2cstr(us, '\0');
}


/* Function: al_ustr_dup
 */
A5O_USTR *al_ustr_dup(const A5O_USTR *us)
{
   return _al_bstrcpy(us);
}


/* Function: al_ustr_dup_substr
 */
A5O_USTR *al_ustr_dup_substr(const A5O_USTR *us, int start_pos,
   int end_pos)
{
   return _al_bmidstr(us, start_pos, end_pos - start_pos);
}


/* Function: al_ustr_empty_string
 */
const A5O_USTR *al_ustr_empty_string(void)
{
   static struct _al_tagbstring empty = _al_bsStatic("");
   return &empty;
}


/* Function: al_ref_cstr
 */
const A5O_USTR *al_ref_cstr(A5O_USTR_INFO *info, const char *s)
{
   struct _al_tagbstring *tb = (struct _al_tagbstring *) info;
   ASSERT(info);
   ASSERT(s);

   _al_btfromcstr(*tb, s);
   return tb;
}


/* Function: al_ref_buffer
 */
const A5O_USTR *al_ref_buffer(A5O_USTR_INFO *info, const char *s, size_t size)
{
   struct _al_tagbstring *tb = (struct _al_tagbstring *) info;
   ASSERT(s);

   _al_blk2tbstr(*tb, s, size);
   return tb;
}


/* Function: al_ref_ustr
 */
const A5O_USTR *al_ref_ustr(A5O_USTR_INFO *info, const A5O_USTR *us,
   int start_pos, int end_pos)
{
   struct _al_tagbstring *tb = (struct _al_tagbstring *) info;

   _al_bmid2tbstr(*tb, us, start_pos, end_pos - start_pos);
   return tb;
}


/* Function: al_ref_info
 */
const A5O_USTR *al_ref_info(const A5O_USTR_INFO *info)
{
   return info;
}

/* Function: al_ustr_size
 */
size_t al_ustr_size(const A5O_USTR *us)
{
   return _al_blength(us);
}


/* Function: al_ustr_length
 */
size_t al_ustr_length(const A5O_USTR *us)
{
   int pos = 0;
   int c = 0;

   while (al_ustr_next(us, &pos))
      c++;

   return c;
}


/* Function: al_ustr_offset
 */
int al_ustr_offset(const A5O_USTR *us, int index)
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
bool al_ustr_next(const A5O_USTR *us, int *pos)
{
   const unsigned char *data = (const unsigned char *) _al_bdata(us);
   int size = _al_blength(us);
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
bool al_ustr_prev(const A5O_USTR *us, int *pos)
{
   const unsigned char *data = (const unsigned char *) _al_bdata(us);
   int c;

   if (!data)
      return false;

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
int32_t al_ustr_get(const A5O_USTR *ub, int pos)
{
   int32_t c;
   int remain;
   int32_t minc;
   const unsigned char *data;

   c = _al_bchare(ub, pos, -1);

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

   if (pos + remain > _al_blength(ub)) {
      al_set_errno(EILSEQ);
      return -2;
   }

   data = (const unsigned char *) _al_bdata(ub);
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
int32_t al_ustr_get_next(const A5O_USTR *us, int *pos)
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
int32_t al_ustr_prev_get(const A5O_USTR *us, int *pos)
{
   if (al_ustr_prev(us, pos)) {
      return al_ustr_get(us, *pos);
   }

   /* Past beginning. */
   return -1;
}


/* Function: al_ustr_insert
 */
bool al_ustr_insert(A5O_USTR *us1, int pos, const A5O_USTR *us2)
{
   return _al_binsert(us1, pos, us2, '\0') == _AL_BSTR_OK;
}


/* Function: al_ustr_insert_cstr
 */
bool al_ustr_insert_cstr(A5O_USTR *us, int pos, const char *s)
{
   A5O_USTR_INFO info;

   return al_ustr_insert(us, pos, al_ref_cstr(&info, s));
}


/* Function: al_ustr_insert_chr
 */
size_t al_ustr_insert_chr(A5O_USTR *us, int pos, int32_t c)
{
   uint32_t uc = c;
   size_t sz;

   if (uc < 128) {
      return (_al_binsertch(us, pos, 1, uc) == _AL_BSTR_OK) ? 1 : 0;
   }

   sz = al_utf8_width(c);
   if (_al_binsertch(us, pos, sz, '\0') == _AL_BSTR_OK) {
      char* data = _al_bdataofs(us, pos);
      if (data)
         return al_utf8_encode(data, c);
      else
         return 0;
   }

   return 0;
}


/* Function: al_ustr_append
 */
bool al_ustr_append(A5O_USTR *us1, const A5O_USTR *us2)
{
   return _al_bconcat(us1, us2) == _AL_BSTR_OK;
}


/* Function: al_ustr_append_cstr
 */
bool al_ustr_append_cstr(A5O_USTR *us, const char *s)
{
   return _al_bcatcstr(us, s) == _AL_BSTR_OK;
}


/* Function: al_ustr_append_chr
 */
size_t al_ustr_append_chr(A5O_USTR *us, int32_t c)
{
   uint32_t uc = c;

   if (uc < 128) {
      return (_al_bconchar(us, uc) == _AL_BSTR_OK) ? 1 : 0;
   }

   return al_ustr_insert_chr(us, al_ustr_size(us), c);
}


/* Function: al_ustr_appendf
 */
bool al_ustr_appendf(A5O_USTR *us, const char *fmt, ...)
{
   va_list ap;
   bool rc;

   va_start(ap, fmt);
   rc = al_ustr_vappendf(us, fmt, ap);
   va_end(ap);
   return rc;
}


/* Function: al_ustr_vappendf
 */
bool al_ustr_vappendf(A5O_USTR *us, const char *fmt, va_list ap)
{
   va_list arglist;
   int sz;
   int rc;

#ifdef DEBUGMODE
   /* Exercise resizing logic more often. */
   sz = 1;
#else
   sz = 128;
#endif

   for (;;) {
      /* Make a copy of the argument list as vsnprintf() may clobber it. */
      va_copy(arglist, ap);
      rc = _al_bvcformata(us, sz, fmt, arglist);
      va_end(arglist);

      if (rc >= 0) {
         return true;
      }

      if (rc == _AL_BSTR_ERR) {
         /* A real error? */
         return false;
      }

      /* Increase size */
      sz = -rc;
   }
}


/* Function: al_ustr_remove_chr
 */
bool al_ustr_remove_chr(A5O_USTR *us, int pos)
{
   int32_t c;
   size_t w;

   c = al_ustr_get(us, pos);
   if (c < 0)
      return false;

   w = al_utf8_width(c);
   return _al_bdelete(us, pos, w) == _AL_BSTR_OK;
}


/* Function: al_ustr_remove_range
 */
bool al_ustr_remove_range(A5O_USTR *us, int start_pos, int end_pos)
{
   return _al_bdelete(us, start_pos, end_pos - start_pos) == _AL_BSTR_OK;
}


/* Function: al_ustr_truncate
 */
bool al_ustr_truncate(A5O_USTR *us, int start_pos)
{
   return _al_btrunc(us, start_pos) == _AL_BSTR_OK;
}


/* Function: al_ustr_ltrim_ws
 */
bool al_ustr_ltrim_ws(A5O_USTR *us)
{
   return _al_bltrimws(us) == _AL_BSTR_OK;
}


/* Function: al_ustr_rtrim_ws
 */
bool al_ustr_rtrim_ws(A5O_USTR *us)
{
   return _al_brtrimws(us) == _AL_BSTR_OK;
}


/* Function: al_ustr_trim_ws
 */
bool al_ustr_trim_ws(A5O_USTR *us)
{
   return _al_btrimws(us) == _AL_BSTR_OK;
}


/* Function: al_ustr_assign
 */
bool al_ustr_assign(A5O_USTR *us1, const A5O_USTR *us2)
{
   return _al_bassign(us1, us2) == _AL_BSTR_OK;
}


/* Function: al_ustr_assign_substr
 */
bool al_ustr_assign_substr(A5O_USTR *us1, const A5O_USTR *us2,
   int start_pos, int end_pos)
{
   int rc = _al_bassignmidstr(us1, us2, start_pos, end_pos - start_pos);
   return rc == _AL_BSTR_OK;
}


/* Function: al_ustr_assign_cstr
 */
bool al_ustr_assign_cstr(A5O_USTR *us1, const char *s)
{
   return _al_bassigncstr(us1, s) == _AL_BSTR_OK;
}


/* Function: al_ustr_set_chr
 */
size_t al_ustr_set_chr(A5O_USTR *us, int start_pos, int32_t c)
{
   int32_t oldc;
   size_t oldw;
   size_t neww;
   int rc;

   oldc = al_ustr_get(us, start_pos);
   if (oldc == -2)
      return 0;

   oldw = al_utf8_width(oldc);
   neww = al_utf8_width(c);
   if (neww == 0)
      return 0;

   if (oldw > neww)
      rc = _al_bdelete(us, start_pos, oldw - neww);
   else if (neww > oldw)
      rc = _al_binsertch(us, start_pos, neww - oldw, '\0');
   else
      rc = _AL_BSTR_OK;

   if (rc == _AL_BSTR_OK) {
      char* data = _al_bdataofs(us, start_pos);
      if (data) {
         return al_utf8_encode(data, c);
      }
      else {
         return 0;
      }
   }
   else {
      return 0;
   }
}


/* Function: al_ustr_replace_range
 */
bool al_ustr_replace_range(A5O_USTR *us1, int start_pos1, int end_pos1,
   const A5O_USTR *us2)
{
   return _al_breplace(us1, start_pos1, end_pos1 - start_pos1, us2, '\0')
      == _AL_BSTR_OK;
}


/* Function: al_ustr_find_chr
 */
int al_ustr_find_chr(const A5O_USTR *us, int start_pos, int32_t c)
{
   char encc[4];
   size_t sizec;
   struct _al_tagbstring enctb;
   int rc;

   /* Fast path for ASCII characters. */
   if (c < 128) {
      rc = _al_bstrchrp(us, c, start_pos);
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
   rc = _al_binstr(us, start_pos, &enctb);
   return (rc == _AL_BSTR_ERR) ? -1 : rc;
}


/* Function: al_ustr_rfind_chr
 */
int al_ustr_rfind_chr(const A5O_USTR *us, int end_pos, int32_t c)
{
   char encc[4];
   size_t sizec;
   struct _al_tagbstring enctb;
   int rc;

   /* Fast path for ASCII characters. */
   if (c < 128) {
      rc = _al_bstrrchrp(us, c, end_pos - 1);
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
   rc = _al_binstrr(us, end_pos - sizec, &enctb);
   return (rc == _AL_BSTR_ERR) ? -1 : rc;
}


/* Function: al_ustr_find_set
 */
int al_ustr_find_set(const A5O_USTR *us, int start_pos,
   const A5O_USTR *accept)
{
   int rc;
   int32_t c, d;
   int pos;
   int set_pos;

   /* Fast path for ASCII characters. */
   if (all_ascii(accept)) {
      rc = _al_binchr(us, start_pos, accept);
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
int al_ustr_find_set_cstr(const A5O_USTR *us, int start_pos,
   const char *accept)
{
   A5O_USTR_INFO info;
   const A5O_USTR *accept_us = al_ref_cstr(&info, accept);

   return al_ustr_find_set(us, start_pos, accept_us);
}


/* Function: al_ustr_find_cset
 */
int al_ustr_find_cset(const A5O_USTR *us, int start_pos,
   const A5O_USTR *reject)
{
   int rc;
   int32_t c, d;
   int pos;
   int set_pos;

   /* Fast path for ASCII characters. */
   if (all_ascii(reject)) {
      rc = _al_bninchr(us, start_pos, reject);
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
int al_ustr_find_cset_cstr(const A5O_USTR *us, int start_pos,
   const char *reject)
{
   A5O_USTR_INFO info;
   const A5O_USTR *reject_us = al_ref_cstr(&info, reject);

   return al_ustr_find_cset(us, start_pos, reject_us);
}


/* Function: al_ustr_find_str
 */
int al_ustr_find_str(const A5O_USTR *haystack, int start_pos,
   const A5O_USTR *needle)
{
   int rc = _al_binstr(haystack, start_pos, needle);
   return (rc == _AL_BSTR_ERR) ? -1 : rc;
}


/* Function: al_ustr_find_cstr
 */
int al_ustr_find_cstr(const A5O_USTR *haystack, int start_pos,
   const char *needle)
{
   A5O_USTR_INFO info;
   const A5O_USTR *needle_us = al_ref_cstr(&info, needle);

   return al_ustr_find_str(haystack, start_pos, needle_us);
}


/* Function: al_ustr_rfind_str
 */
int al_ustr_rfind_str(const A5O_USTR *haystack, int end_pos,
   const A5O_USTR *needle)
{
   int rc = _al_binstrr(haystack, end_pos - _al_blength(needle), needle);
   return (rc == _AL_BSTR_ERR) ? -1 : rc;
}


/* Function: al_ustr_rfind_cstr
 */
int al_ustr_rfind_cstr(const A5O_USTR *haystack, int end_pos,
   const char *needle)
{
   A5O_USTR_INFO info;
   const A5O_USTR *needle_us = al_ref_cstr(&info, needle);

   return al_ustr_rfind_str(haystack, end_pos, needle_us);
}


/* Function: al_ustr_find_replace
 */
bool al_ustr_find_replace(A5O_USTR *us, int start_pos,
   const A5O_USTR *find, const A5O_USTR *replace)
{
   return _al_bfindreplace(us, find, replace, start_pos) == _AL_BSTR_OK;
}


/* Function: al_ustr_find_replace_cstr
 */
bool al_ustr_find_replace_cstr(A5O_USTR *us, int start_pos,
   const char *find, const char *replace)
{
   A5O_USTR_INFO find_info;
   A5O_USTR_INFO repl_info;
   const A5O_USTR *find_us = al_ref_cstr(&find_info, find);
   const A5O_USTR *repl_us = al_ref_cstr(&repl_info, replace);

   return al_ustr_find_replace(us, start_pos, find_us, repl_us);
}


/* Function: al_ustr_equal
 */
bool al_ustr_equal(const A5O_USTR *us1, const A5O_USTR *us2)
{
   return _al_biseq(us1, us2) == 1;
}


/* Function: al_ustr_compare
 */
int al_ustr_compare(const A5O_USTR *us1, const A5O_USTR *us2)
{
   int pos1 = 0;
   int pos2 = 0;

   for (;;) {
      int32_t c1 = al_ustr_get_next(us1, &pos1);
      int32_t c2 = al_ustr_get_next(us2, &pos2);

      if (c1 != c2) {
         /* This happens to work even when one of c1 or c2 is -1. */
         return c1 - c2;
      }

      if (c1 == -1) /* == c2 */
         return 0;
   }
}


/* Function: al_ustr_ncompare
 */
int al_ustr_ncompare(const A5O_USTR *us1, const A5O_USTR *us2, int n)
{
   int pos1 = 0;
   int pos2 = 0;

   if (n <= 0)
      return 0;

   for (;;) {
      int32_t c1 = al_ustr_get_next(us1, &pos1);
      int32_t c2 = al_ustr_get_next(us2, &pos2);

      if (c1 != c2) {
         /* This happens to work even when one of c1 or c2 is -1. */
         return c1 - c2;
      }

      if ((c1 == -1) || (--n <= 0))
         return 0;
   }
}


/* Function: al_ustr_has_prefix
 */
bool al_ustr_has_prefix(const A5O_USTR *us1, const A5O_USTR *us2)
{
   return 0 == _al_bstrncmp(us1, us2, _al_blength(us2));
}


/* Function: al_ustr_has_prefix_cstr
 */
bool al_ustr_has_prefix_cstr(const A5O_USTR *us1, const char *s2)
{
   A5O_USTR_INFO info;
   const A5O_USTR *us2 = al_ref_cstr(&info, s2);

   return al_ustr_has_prefix(us1, us2);
}


/* Function: al_ustr_has_suffix
 */
bool al_ustr_has_suffix(const A5O_USTR *us1, const A5O_USTR *us2)
{
   struct _al_tagbstring tb1;
   int pos;

   pos = _al_blength(us1) - _al_blength(us2);
   _al_bmid2tbstr(tb1, us1, pos, INT_MAX);
   return _al_biseq(&tb1, us2);
}


/* Function: al_ustr_has_suffix_cstr
 */
bool al_ustr_has_suffix_cstr(const A5O_USTR *us1, const char *s2)
{
   A5O_USTR_INFO info;
   const A5O_USTR *us2 = al_ref_cstr(&info, s2);

   return al_ustr_has_suffix(us1, us2);
}


/* Function: al_utf8_width
 */
size_t al_utf8_width(int32_t c)
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


/* Function: al_utf16_width
 */
size_t al_utf16_width(int c)
{
   /* So we don't need to check for negative values nor use unsigned ints
    * in the interface, which are a pain.
    */
   uint32_t uc = c;

   /* We do not check for invalid code points. */
   if (uc <= 0xffff)
      return 2;
   if (uc <= 0x10ffff)
      return 4;

   /* The rest are illegal. */
   return 0;
}


/* Function: al_utf16_encode
 */
size_t al_utf16_encode(uint16_t s[], int32_t c)
{
   uint32_t uc = c;

   if (uc <= 0xffff) {
      /* Note: We always assume the native endianness here. */
      s[0] = uc;
      return 2;
   }

   if (uc <= 0x10ffff) {
      uint32_t u_ = uc - 0x10000;
      /* Note: We always assume the native endianness here. */
      s[0] = 0xd800 | (u_ >> 10);
      s[1] = 0xdc00 | (u_ & 0x3ff);
      return 4;
   }

   /* Otherwise is illegal. */
   return 0;
}


static size_t _al_utf16_get(uint16_t const *s, int n, int *c)
{
   if (s[0] < 0xd800 || s[0] > 0xdfff) {
      *c = s[0];
      return 1;
   }
   if (n < 2)
      return 0;
   *c = 0x10000 | ((s[0] & 0x3ff) << 10) | (s[1] & 0x3ff);
   return 2;
}


/* Function: al_ustr_new_from_utf16
 */
A5O_USTR *al_ustr_new_from_utf16(uint16_t const *s)
{
   unsigned int i = 0;
   A5O_USTR *ustr = al_ustr_new("");
   while (1) {
      int c;
      /* We expect the passed string to be 0 terminated, so there are
       * always 2 words available.
       */
      size_t n = _al_utf16_get(s + i, 2, &c);
      /* Note: The string already is 0 terminated. */
      if (c == 0)
         break;
      al_ustr_append_chr(ustr, c);
      i += n;
   }
   return ustr;
}


/* Function: al_ustr_size_utf16
 */
size_t al_ustr_size_utf16(const A5O_USTR *us)
{
   int pos = 0;
   size_t sz = 0;
   while (1) {
      int32_t c = al_ustr_get_next(us, &pos);
      if (c < 0)
         break;
      sz += al_utf16_width(c);
   }
   /* Size of terminating 0 character - al_ustr_get_next will not
    * return it.
    */
   sz += 2;
   return sz;
}


/* Function: al_ustr_encode_utf16
 */
size_t al_ustr_encode_utf16(const A5O_USTR *us, uint16_t *s,
   size_t n)
{
   int pos = 0;
   size_t i = 0;
   while (1) {
      /* Used to hold one encoded UTF-16 character. */
      uint16_t encoded[2] = {0, 0};
      size_t sz;
      int32_t c = al_ustr_get_next(us, &pos);
      if (c < 0)
         break;
      sz = al_utf16_encode(encoded, c);
      /* Need two bytes for terminating 0. */
      if (i * 2 + sz > n - 2)
         break;
      s[i++] = encoded[0];
      if (sz == 4)
         s[i++] = encoded[1];
   }
   /* Append terminating 0 - al_ustr_get_next withheld it. */
   if (i * 2 + 1 < n)
      s[i++] = 0;

   return i * 2;
}


/* vim: set sts=3 sw=3 et: */
