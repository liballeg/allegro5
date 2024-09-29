#ifndef __al_included_allegro5_utf8_h
#define __al_included_allegro5_utf8_h

#include "allegro5/base.h"

#ifdef __cplusplus
   extern "C" {
#endif

/* Type: A5O_USTR
 */
typedef struct _al_tagbstring A5O_USTR;

/* Type: A5O_USTR_INFO
 */
typedef struct _al_tagbstring A5O_USTR_INFO;

#ifndef __al_tagbstring_defined
#define __al_tagbstring_defined
struct _al_tagbstring {
	int mlen;
	int slen;
	unsigned char * data;
};
#endif

/* Creating strings */
AL_FUNC(A5O_USTR *, al_ustr_new, (const char *s));
AL_FUNC(A5O_USTR *, al_ustr_new_from_buffer, (const char *s, size_t size));
AL_PRINTFUNC(A5O_USTR *, al_ustr_newf, (const char *fmt, ...), 1, 2);
AL_FUNC(void, al_ustr_free, (A5O_USTR *us));
AL_FUNC(const char *, al_cstr, (const A5O_USTR *us));
AL_FUNC(void, al_ustr_to_buffer, (const A5O_USTR *us, char *buffer, int size));
AL_FUNC(char *, al_cstr_dup, (const A5O_USTR *us));
AL_FUNC(A5O_USTR *, al_ustr_dup, (const A5O_USTR *us));
AL_FUNC(A5O_USTR *, al_ustr_dup_substr, (const A5O_USTR *us,
      int start_pos, int end_pos));

/* Predefined string */
AL_FUNC(const A5O_USTR *, al_ustr_empty_string, (void));

/* Reference strings */
AL_FUNC(const A5O_USTR *, al_ref_cstr, (A5O_USTR_INFO *info, const char *s));
AL_FUNC(const A5O_USTR *, al_ref_buffer, (A5O_USTR_INFO *info, const char *s,
      size_t size));
AL_FUNC(const A5O_USTR *, al_ref_ustr, (A5O_USTR_INFO *info,
      const A5O_USTR *us, int start_pos, int end_pos));
AL_FUNC(const A5O_USTR *, al_ref_info, (const A5O_USTR_INFO *info));

/* Sizes and offsets */
AL_FUNC(size_t, al_ustr_size, (const A5O_USTR *us));
AL_FUNC(size_t, al_ustr_length, (const A5O_USTR *us));
AL_FUNC(int, al_ustr_offset, (const A5O_USTR *us, int index));
AL_FUNC(bool, al_ustr_next, (const A5O_USTR *us, int *pos));
AL_FUNC(bool, al_ustr_prev, (const A5O_USTR *us, int *pos));

/* Get codepoints */
AL_FUNC(int32_t, al_ustr_get, (const A5O_USTR *us, int pos));
AL_FUNC(int32_t, al_ustr_get_next, (const A5O_USTR *us, int *pos));
AL_FUNC(int32_t, al_ustr_prev_get, (const A5O_USTR *us, int *pos));

/* Insert */
AL_FUNC(bool, al_ustr_insert, (A5O_USTR *us1, int pos,
      const A5O_USTR *us2));
AL_FUNC(bool, al_ustr_insert_cstr, (A5O_USTR *us, int pos,
      const char *us2));
AL_FUNC(size_t, al_ustr_insert_chr, (A5O_USTR *us, int pos, int32_t c));

/* Append */
AL_FUNC(bool, al_ustr_append, (A5O_USTR *us1, const A5O_USTR *us2));
AL_FUNC(bool, al_ustr_append_cstr, (A5O_USTR *us, const char *s));
AL_FUNC(size_t, al_ustr_append_chr, (A5O_USTR *us, int32_t c));
AL_PRINTFUNC(bool, al_ustr_appendf, (A5O_USTR *us, const char *fmt, ...),
      2, 3);
AL_FUNC(bool, al_ustr_vappendf, (A5O_USTR *us, const char *fmt,
      va_list ap));

/* Remove */
AL_FUNC(bool, al_ustr_remove_chr, (A5O_USTR *us, int pos));
AL_FUNC(bool, al_ustr_remove_range, (A5O_USTR *us, int start_pos,
      int end_pos));
AL_FUNC(bool, al_ustr_truncate, (A5O_USTR *us, int start_pos));
AL_FUNC(bool, al_ustr_ltrim_ws, (A5O_USTR *us));
AL_FUNC(bool, al_ustr_rtrim_ws, (A5O_USTR *us));
AL_FUNC(bool, al_ustr_trim_ws, (A5O_USTR *us));

/* Assign */
AL_FUNC(bool, al_ustr_assign, (A5O_USTR *us1, const A5O_USTR *us2));
AL_FUNC(bool, al_ustr_assign_substr, (A5O_USTR *us1, const A5O_USTR *us2,
      int start_pos, int end_pos));
AL_FUNC(bool, al_ustr_assign_cstr, (A5O_USTR *us1, const char *s));

/* Replace */
AL_FUNC(size_t, al_ustr_set_chr, (A5O_USTR *us, int pos, int32_t c));
AL_FUNC(bool, al_ustr_replace_range, (A5O_USTR *us1, int start_pos1,
      int end_pos1, const A5O_USTR *us2));

/* Searching */
AL_FUNC(int, al_ustr_find_chr, (const A5O_USTR *us, int start_pos,
      int32_t c));
AL_FUNC(int, al_ustr_rfind_chr, (const A5O_USTR *us, int start_pos,
      int32_t c));
AL_FUNC(int, al_ustr_find_set, (const A5O_USTR *us, int start_pos,
      const A5O_USTR *accept));
AL_FUNC(int, al_ustr_find_set_cstr, (const A5O_USTR *us, int start_pos,
      const char *accept));
AL_FUNC(int, al_ustr_find_cset, (const A5O_USTR *us, int start_pos,
      const A5O_USTR *reject));
AL_FUNC(int, al_ustr_find_cset_cstr, (const A5O_USTR *us, int start_pos,
      const char *reject));
AL_FUNC(int, al_ustr_find_str, (const A5O_USTR *haystack, int start_pos,
      const A5O_USTR *needle));
AL_FUNC(int, al_ustr_find_cstr, (const A5O_USTR *haystack, int start_pos,
      const char *needle));
AL_FUNC(int, al_ustr_rfind_str, (const A5O_USTR *haystack, int start_pos,
      const A5O_USTR *needle));
AL_FUNC(int, al_ustr_rfind_cstr, (const A5O_USTR *haystack, int start_pos,
      const char *needle));
AL_FUNC(bool, al_ustr_find_replace, (A5O_USTR *us, int start_pos,
      const A5O_USTR *find, const A5O_USTR *replace));
AL_FUNC(bool, al_ustr_find_replace_cstr, (A5O_USTR *us, int start_pos,
      const char *find, const char *replace));

/* Compare */
AL_FUNC(bool, al_ustr_equal, (const A5O_USTR *us1, const A5O_USTR *us2));
AL_FUNC(int, al_ustr_compare, (const A5O_USTR *u, const A5O_USTR *v));
AL_FUNC(int, al_ustr_ncompare, (const A5O_USTR *us1, const A5O_USTR *us2,
      int n));
AL_FUNC(bool, al_ustr_has_prefix,(const A5O_USTR *u, const A5O_USTR *v));
AL_FUNC(bool, al_ustr_has_prefix_cstr, (const A5O_USTR *u, const char *s));
AL_FUNC(bool, al_ustr_has_suffix,(const A5O_USTR *u, const A5O_USTR *v));
AL_FUNC(bool, al_ustr_has_suffix_cstr,(const A5O_USTR *us1, const char *s));

/* Low level UTF-8 functions */
AL_FUNC(size_t, al_utf8_width, (int32_t c));
AL_FUNC(size_t, al_utf8_encode, (char s[], int32_t c));

/* UTF-16 */
AL_FUNC(A5O_USTR *, al_ustr_new_from_utf16, (uint16_t const *s));
AL_FUNC(size_t, al_ustr_size_utf16, (const A5O_USTR *us));
AL_FUNC(size_t, al_ustr_encode_utf16, (const A5O_USTR *us, uint16_t *s, size_t n));
AL_FUNC(size_t, al_utf16_width, (int c));
AL_FUNC(size_t, al_utf16_encode, (uint16_t s[], int32_t c));

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
