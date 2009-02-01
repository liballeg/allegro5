#ifndef __al_included_utf8_h
#define __al_included_utf8_h

#include "allegro5/base.h"

#ifdef __cplusplus
   extern "C" {
#endif

typedef struct ALLEGRO_USTR ALLEGRO_USTR;
typedef const struct ALLEGRO_USTR_INFO ALLEGRO_USTR_INFO;

struct ALLEGRO_USTR {
   struct _al_tagbstring *b;  /* internal */
};

struct ALLEGRO_USTR_INFO {
   /* This struct needs to be at least as big as struct _al_tagbstring. */
   int __pad[4];
};

/* Creating strings */
AL_FUNC(ALLEGRO_USTR, al_ustr_new, (const char *s));
AL_FUNC(ALLEGRO_USTR, al_ustr_new_from_buffer, (const char *s, size_t size));
AL_FUNC(void, al_ustr_free, (ALLEGRO_USTR us));
AL_FUNC(const char *, al_cstr, (const ALLEGRO_USTR us));
AL_FUNC(char *, al_cstr_dup, (const ALLEGRO_USTR us));
AL_FUNC(ALLEGRO_USTR, al_ustr_dup, (const ALLEGRO_USTR us));
AL_FUNC(ALLEGRO_USTR, al_ustr_dup_substr, (const ALLEGRO_USTR us,
      int start_pos, int end_pos));

/* Predefined string */
AL_FUNC(ALLEGRO_USTR, al_ustr_empty_string, (void));

/* Reference strings */
AL_FUNC(ALLEGRO_USTR, al_ref_cstr, (ALLEGRO_USTR_INFO *info, const char *s));
AL_FUNC(ALLEGRO_USTR, al_ref_buffer, (ALLEGRO_USTR_INFO *info, const char *s,
      size_t size));
AL_FUNC(ALLEGRO_USTR, al_ref_ustr, (ALLEGRO_USTR_INFO *info,
      const ALLEGRO_USTR us, int start_pos, int end_pos));

/* Sizes and offsets */
AL_FUNC(size_t, al_ustr_size, (const ALLEGRO_USTR us));
AL_FUNC(size_t, al_ustr_length, (const ALLEGRO_USTR us));
AL_FUNC(int, al_ustr_offset, (const ALLEGRO_USTR us, int index));
AL_FUNC(bool, al_ustr_next, (const ALLEGRO_USTR us, int *pos));
AL_FUNC(bool, al_ustr_prev, (const ALLEGRO_USTR us, int *pos));

/* Get codepoints */
AL_FUNC(int32_t, al_ustr_get, (const ALLEGRO_USTR us, int pos));
AL_FUNC(int32_t, al_ustr_get_next, (const ALLEGRO_USTR us, int *pos));
AL_FUNC(int32_t, al_ustr_prev_get, (const ALLEGRO_USTR us, int *pos));

/* Insert */
AL_FUNC(bool, al_ustr_insert, (ALLEGRO_USTR us1, int pos,
      const ALLEGRO_USTR us2));
AL_FUNC(bool, al_ustr_insert_cstr, (ALLEGRO_USTR us, int pos,
      const char *us2));
AL_FUNC(size_t, al_ustr_insert_chr, (ALLEGRO_USTR us, int pos, int32_t c));

/* Append */
AL_FUNC(bool, al_ustr_append, (ALLEGRO_USTR us1, const ALLEGRO_USTR us2));
AL_FUNC(bool, al_ustr_append_cstr, (ALLEGRO_USTR us, const char *s));
AL_FUNC(size_t, al_ustr_append_chr, (ALLEGRO_USTR us, int32_t c));

/* Remove */
AL_FUNC(bool, al_ustr_remove_range, (ALLEGRO_USTR us, int start_pos,
      int end_pos));
AL_FUNC(bool, al_ustr_truncate, (ALLEGRO_USTR us, int start_pos));
AL_FUNC(bool, al_ustr_ltrim_ws, (ALLEGRO_USTR us));
AL_FUNC(bool, al_ustr_rtrim_ws, (ALLEGRO_USTR us));
AL_FUNC(bool, al_ustr_trim_ws, (ALLEGRO_USTR us));

/* Searching */
AL_FUNC(int, al_ustr_find_chr, (const ALLEGRO_USTR us, int start_pos,
      int32_t c));
AL_FUNC(int, al_ustr_rfind_chr, (const ALLEGRO_USTR us, int start_pos,
      int32_t c));
AL_FUNC(int, al_ustr_find_set, (const ALLEGRO_USTR us, int start_pos,
      const ALLEGRO_USTR accept));
AL_FUNC(int, al_ustr_find_set_cstr, (const ALLEGRO_USTR us, int start_pos,
      const char *accept));
AL_FUNC(int, al_ustr_find_cset, (const ALLEGRO_USTR us, int start_pos,
      const ALLEGRO_USTR reject));
AL_FUNC(int, al_ustr_find_cset_cstr, (const ALLEGRO_USTR us, int start_pos,
      const char *reject));
AL_FUNC(int, al_ustr_find_str, (const ALLEGRO_USTR haystack, int start_pos,
      const ALLEGRO_USTR needle));
AL_FUNC(int, al_ustr_find_cstr, (const ALLEGRO_USTR haystack, int start_pos,
      const char *needle));
AL_FUNC(int, al_ustr_rfind_str, (const ALLEGRO_USTR haystack, int start_pos,
      const ALLEGRO_USTR needle));
AL_FUNC(int, al_ustr_rfind_cstr, (const ALLEGRO_USTR haystack, int start_pos,
      const char *needle));

/* Compare */
AL_FUNC(bool, al_ustr_equal, (const ALLEGRO_USTR us1, const ALLEGRO_USTR us2));

/* Low level UTF-8 functions */
AL_FUNC(size_t, al_utf8_width, (int32_t c));
AL_FUNC(size_t, al_utf8_encode, (char s[], int32_t c));

/* To be added:

CREATE

   al_ustr_newf

APPEND

   al_ustr_appendf
   al_ustr_vappendf

REMOVE

   al_ustr_remove_chr

REPLACE

   al_ustr_set_char
   al_ustr_replace_from
   al_ustr_replace_range
   al_ustr_assign
   al_ustr_assignf
   al_ustr_assign_substr   (too similar to replace_range?)

COMPARE

   al_ustr_compare
   al_ustr_ncompare
   al_ustr_has_prefix
   al_ustr_has_suffix

*/

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
