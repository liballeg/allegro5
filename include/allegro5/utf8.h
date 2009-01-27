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
AL_FUNC(void, al_ustr_free, (ALLEGRO_USTR us));
AL_FUNC(const char *, al_cstr, (ALLEGRO_USTR us));

/* Predefined string */
AL_FUNC(ALLEGRO_USTR, al_ustr_empty_string, (void));

/* Reference strings */
AL_FUNC(ALLEGRO_USTR, al_ref_cstr, (ALLEGRO_USTR_INFO *info, const char *s));
AL_FUNC(ALLEGRO_USTR, al_ref_buffer, (ALLEGRO_USTR_INFO *info, const char *s,
      size_t size));
AL_FUNC(ALLEGRO_USTR, al_ref_ustr, (ALLEGRO_USTR_INFO *info, ALLEGRO_USTR us,
      int start_pos, int end_pos));

/* Sizes and offsets */
AL_FUNC(size_t, al_ustr_size, (ALLEGRO_USTR us));

/* To be added:

UTF-8 HELPERS

   al_utf8_width
   al_utf8_encode

CREATE

   al_ustr_newf
   al_ustr_dup
   al_ustr_dup_substr
   al_cstr_dup

LENGTH AND OFFSET

   al_ustr_length
   al_ustr_offset
   al_ustr_next
   al_ustr_prev

GET CODE POINTS

   al_ustr_get
   al_ustr_get_next
   al_ustr_prev_get

INSERT

   al_ustr_insert_chr
   al_ustr_insert
   al_ustr_insert_cstr

APPEND

   al_ustr_append_chr
   al_ustr_append
   al_ustr_append_cstr
   al_ustr_appendf
   al_ustr_vappendf

REMOVE

   al_ustr_remove_chr
   al_ustr_remove_range
   al_ustr_truncate
   al_ustr_ltrim_ws
   al_ustr_rtrim_ws
   al_ustr_trim_ws

REPLACE

   al_ustr_set_char
   al_ustr_replace_from
   al_ustr_replace_range
   al_ustr_assign
   al_ustr_assign_mid   (too similar to replace_range?)

SEARCHING

   al_ustr_find_chr
   al_ustr_rfind_chr
   al_ustr_find_any
   al_ustr_find_str
   al_ustr_find_span
   al_ustr_find_cspan

COMPARE

   al_ustr_compare
   al_ustr_ncompare
   al_ustr_equal
   al_ustr_has_prefix
   al_ustr_has_suffix

*/

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
