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



/* Function: al_font_textout
 */
void al_font_textout(const ALLEGRO_FONT *f, int x, int y,
   const char *str, int count)
{
   ALLEGRO_USTR_INFO str_info;
   ALLEGRO_USTR *ustr;
   ASSERT(f);
   ASSERT(str);
   ASSERT(count >= -1);

   ustr = al_ref_cstr(&str_info, str);
   if (count > -1) {
      ustr = al_ref_buffer(&str_info, str, al_ustr_offset(ustr, count));
   }

   f->vtable->render(f, ustr, x, y);
}



/* Function: al_font_textout_centre
 */
void al_font_textout_centre(const ALLEGRO_FONT *f, int x, int y,
   const char *str, int count)
{
   ALLEGRO_USTR_INFO str_info;
   ALLEGRO_USTR *ustr;
   int len;
   ASSERT(f);
   ASSERT(str);
   ASSERT(count >= -1);

   ustr = al_ref_cstr(&str_info, str);
   if (count > -1) {
      ustr = al_ref_buffer(&str_info, str, al_ustr_offset(ustr, count));
   }

   len = f->vtable->text_length(f, ustr);
   f->vtable->render(f, ustr, x - len/2, y);
}



/* Function: al_font_textout_right
 */
void al_font_textout_right(const ALLEGRO_FONT *f, int x, int y,
   const char *str, int count)
{
   ALLEGRO_USTR_INFO str_info;
   ALLEGRO_USTR *ustr;
   int len;
   ASSERT(f);
   ASSERT(str);
   ASSERT(count >= -1);

   ustr = al_ref_cstr(&str_info, str);
   if (count > -1) {
      ustr = al_ref_buffer(&str_info, str, al_ustr_offset(ustr, count));
   }

   len = f->vtable->text_length(f, ustr);
   f->vtable->render(f, ustr, x - len, y);
}



/* Function: al_font_textout_justify
 */
void al_font_textout_justify(const ALLEGRO_FONT *f, int x1, int x2, int y,
   int diff, const char *str)
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

   ustr = al_ref_cstr(&str_info, str);

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
      minlen += f->vtable->text_length(f, word);
      num_words++;

      pos1 = pos2;
   }

   /* amount of room for space between words */
   space = x2 - x1 - minlen;

   if ((space <= 0) || (space > diff) || (num_words < 2)) {
      /* can't justify */
      f->vtable->render(f, ustr, x1, y);
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
      fleft += f->vtable->render(f, word, (int)fleft, y);
      fleft += finc;

      pos1 = pos2;
   }
}



/* Function: al_font_textprintf
 */
void al_font_textprintf(const ALLEGRO_FONT *f, int x, int y,
   const char *format, ...)
{
   ALLEGRO_USTR *buf;
   va_list ap;
   ASSERT(f);
   ASSERT(format);

   va_start(ap, format);
   buf = al_ustr_new("");
   al_ustr_vappendf(buf, format, ap);
   va_end(ap);

   al_font_textout(f, x, y, al_cstr(buf), -1);

   al_ustr_free(buf);
}



/* Function: al_font_textprintf_centre
 */
void al_font_textprintf_centre(const ALLEGRO_FONT *f, int x, int y,
   const char *format, ...)
{
   ALLEGRO_USTR *buf;
   va_list ap;
   ASSERT(f);
   ASSERT(format);

   va_start(ap, format);
   buf = al_ustr_new("");
   al_ustr_vappendf(buf, format, ap);
   va_end(ap);

   al_font_textout_centre(f, x, y, al_cstr(buf), -1);

   al_ustr_free(buf);
}



/* Function: al_font_textprintf_right
 */
void al_font_textprintf_right(const ALLEGRO_FONT *f, int x, int y,
   const char *format, ...)
{
   ALLEGRO_USTR *buf;
   va_list ap;
   ASSERT(f);
   ASSERT(format);

   va_start(ap, format);
   buf = al_ustr_new("");
   al_ustr_vappendf(buf, format, ap);
   va_end(ap);

   al_font_textout_right(f, x, y, al_cstr(buf), -1);

   al_ustr_free(buf);
}



/* Function: al_font_textprintf_justify
 */
void al_font_textprintf_justify(const ALLEGRO_FONT *f, int x1, int x2, int y,
   int diff, const char *format, ...)
{
   ALLEGRO_USTR *buf;
   va_list ap;
   ASSERT(f);
   ASSERT(format);

   va_start(ap, format);
   buf = al_ustr_new("");
   al_ustr_vappendf(buf, format, ap);
   va_end(ap);

   al_font_textout_justify(f, x1, x2, y, diff, al_cstr(buf));

   al_ustr_free(buf);
}



/* Function: al_font_text_width
 */
int al_font_text_width(const ALLEGRO_FONT *f, const char *str, int count)
{
   ALLEGRO_USTR_INFO str_info;
   ALLEGRO_USTR *ustr;
   ASSERT(f);
   ASSERT(str);
   ASSERT(count >= -1);

   ustr = al_ref_cstr(&str_info, str);
   if (count > -1) {
      ustr = al_ref_buffer(&str_info, str, al_ustr_offset(ustr, count));
   }

   return f->vtable->text_length(f, ustr);
}



/* Function: al_font_text_height
 */
int al_font_text_height(const ALLEGRO_FONT *f)
{
   ASSERT(f);
   return f->vtable->font_height(f);
}



/* Function: al_font_destroy_font
 */
void al_font_destroy_font(ALLEGRO_FONT *f)
{
   ASSERT(f);
   f->vtable->destroy(f);
}


/* vim: set sts=3 sw=3 et: */
