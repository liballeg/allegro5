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
 *      Text drawing routines.
 *
 *      By Shawn Hargreaves.
 *
 *      textout_justify() by Seymour Shlien.
 *
 *      Laurence Withers added the textout_ex() function family.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern_memory.h"

#include "allegro5/a5_font.h"

/* If you call this, you're probably making a mistake. */
#define strlen(s)   __are_you_sure__



static ALLEGRO_USTR *ref_str(ALLEGRO_USTR_INFO *info, const char *text,
   int start, int end)
{
   ALLEGRO_USTR *ustr = al_ref_cstr(info, text);
   if (start == 0 && end == 0) return ustr;

   if (end == 0)
      end = al_ustr_length(ustr);

   ustr = al_ref_buffer(info, text + al_ustr_offset(ustr, start),
      al_ustr_offset(ustr, end));

   return ustr;
}



/* Function: al_draw_text
 */
void al_draw_text(const ALLEGRO_FONT *font, float x, float y, int flags,
   const char *text, int start, int end) 
{
   ALLEGRO_USTR_INFO str_info;
   ALLEGRO_USTR *ustr;
   ASSERT(font);
   ASSERT(text);

   ustr = ref_str(&str_info, text, start, end);

   switch (flags) {
      case ALLEGRO_ALIGN_CENTRE:
         x -= font->vtable->text_length(font, ustr) * 0.5;
         break;
      case ALLEGRO_ALIGN_RIGHT:
         x -= font->vtable->text_length(font, ustr);
         break;
      case ALLEGRO_ALIGN_LEFT:
      default:
         break;
   }
   font->vtable->render(font, ustr, x, y);
}



/* Function: al_draw_justified_text
 */
void al_draw_justified_text(const ALLEGRO_FONT *font, float x1, float x2,
   float y, float diff, int flags, const char *text, int start, int end)
{
   const char *whitespace = " \t\n\r";
   ALLEGRO_USTR_INFO str_info;
   ALLEGRO_USTR_INFO word_info;
   ALLEGRO_USTR *ustr;
   ALLEGRO_USTR *word;
   int pos1, pos2;
   int minlen;
   int num_words;
   int space;
   float fleft, finc;

   (void)flags;

   ustr = ref_str(&str_info, text, start, end);

   /* count words and measure min length (without spaces) */ 
   num_words = 0;
   minlen = 0;
   pos1 = 0;
   for (;;) {
      pos1 = al_ustr_find_cset_cstr(ustr, pos1, whitespace);
      if (pos1 == -1)
         break;
      pos2 = al_ustr_find_set_cstr(ustr, pos1, whitespace);
      if (pos2 == -1)
         pos2 = al_ustr_size(ustr);

      word = al_ref_ustr(&word_info, ustr, pos1, pos2);
      minlen += font->vtable->text_length(font, word);
      num_words++;

      pos1 = pos2;
   }

   /* amount of room for space between words */
   space = x2 - x1 - minlen;

   if ((space <= 0) || (space > diff) || (num_words < 2)) {
      /* can't justify */
      font->vtable->render(font, ustr, x1, y);
      return; 
   }

   /* distribute space left evenly between words */
   fleft = (float)x1;
   finc = (float)space / (float)(num_words-1);
   pos1 = 0;
   for (;;) {
      pos1 = al_ustr_find_cset_cstr(ustr, pos1, whitespace);
      if (pos1 == -1)
         break;
      pos2 = al_ustr_find_set_cstr(ustr, pos1, whitespace);
      if (pos2 == -1)
         pos2 = al_ustr_size(ustr);

      word = al_ref_ustr(&word_info, ustr, pos1, pos2);
      fleft += font->vtable->render(font, word, (int)fleft, y);
      fleft += finc;

      pos1 = pos2;
   }
}



/* Function: al_draw_textf
 */
void al_draw_textf(const ALLEGRO_FONT *font, float x, float y, int flags,
   const char *format, ...)
{
   ALLEGRO_USTR *buf;
   va_list ap;
   const char *s;
   ASSERT(font);
   ASSERT(format);

   /* Fast path for common case. */
   if (0 == strcmp(format, "%s")) {
      va_start(ap, format);
      s = va_arg(ap, const char *);
      al_draw_text(font, x, y, flags, s, 0, 0);
      va_end(ap);
      return;
   }

   va_start(ap, format);
   buf = al_ustr_new("");
   al_ustr_vappendf(buf, format, ap);
   va_end(ap);

   al_draw_text(font, x, y, flags, al_cstr(buf), 0, 0);

   al_ustr_free(buf);
}



/* Function: al_draw_justified_textf
 */
void al_draw_justified_textf(const ALLEGRO_FONT *f, float x1, float x2, float y,
   float diff, int flags, const char *format, ...)
{
   ALLEGRO_USTR *buf;
   va_list ap;
   ASSERT(f);
   ASSERT(format);

   va_start(ap, format);
   buf = al_ustr_new("");
   al_ustr_vappendf(buf, format, ap);
   va_end(ap);

   al_draw_justified_text(f, x1, x2, y, diff, flags, al_cstr(buf), 0, 0);

   al_ustr_free(buf);
}



/* Function: al_get_text_width
 */
int al_get_text_width(const ALLEGRO_FONT *f, const char *str, int start,
   int end)
{
   ALLEGRO_USTR_INFO str_info;
   ALLEGRO_USTR *ustr;
   ASSERT(f);
   ASSERT(str);

   ustr = ref_str(&str_info, str, start, end);

   return f->vtable->text_length(f, ustr);
}



/* Function: al_get_font_line_height
 */
int al_get_font_line_height(const ALLEGRO_FONT *f)
{
   ASSERT(f);
   return f->vtable->font_height(f);
}



/* Function: al_get_text_dimensions
 */
void al_get_text_dimensions(const ALLEGRO_FONT *f,
   char const *text, int start, int end,
   int *bbx, int *bby, int *bbw, int *bbh, int *ascent, int *descent)
{
   ALLEGRO_USTR_INFO str_info;
   ALLEGRO_USTR *ustr;
   ASSERT(f);
   ustr = ref_str(&str_info, text, start, end);
   f->vtable->get_text_dimensions(f, ustr, bbx, bby,
      bbw, bbh, ascent, descent);
}



/* Function: al_destroy_font
 */
void al_destroy_font(ALLEGRO_FONT *f)
{
   ASSERT(f);
   f->vtable->destroy(f);
}


/* vim: set sts=3 sw=3 et: */
