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


#include <math.h>
#include "allegro5/allegro.h"

#include "allegro5/allegro_font.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_system.h"

/* If you call this, you're probably making a mistake. */
/*
#define strlen(s)   __are_you_sure__
*/
/* Removed the above define since some compilers seem to use some
 * preprocessor magic when calling strcmp() that inserts a call to strlen.
 * There might be a better way to do this.
 */



/* Text usually looks best when aligned to pixels -
 * but if x is 0.5 it may very well end up at an integer
 * position if the current transformation scales by 2 or
 * translated x by 0.5. So we simply apply the transformation,
 * round to nearest integer, and backtransform that.
 */
// TODO: In case someone actually *wants* to draw their text to
// sub-pixel positions, for example because multisampling is used,
// this should not be done. We most likely want either a new alignment
// flag to disable this, or even a per-font or per-display setting to
// do so.
static void align_to_integer_pixel(float *x, float *y)
{
   ALLEGRO_TRANSFORM const *t;
   ALLEGRO_TRANSFORM inverse;
   t = al_get_current_transform();
   al_transform_coordinates(t, x, y);
   *x = floor(*x + 0.5);
   *y = floor(*y + 0.5);
   al_copy_transform(&inverse, t);
   al_invert_transform(&inverse);
   al_transform_coordinates(&inverse, x, y);
}



/* Function: al_draw_ustr
 */
void al_draw_ustr(const ALLEGRO_FONT *font,
   ALLEGRO_COLOR color, float x, float y, int flags,
   const ALLEGRO_USTR *ustr) 
{
   ASSERT(font);
   ASSERT(ustr);

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

   align_to_integer_pixel(&x, &y);

   font->vtable->render(font, color, ustr, x, y);
}



/* Function: al_draw_text
 */
void al_draw_text(const ALLEGRO_FONT *font,
   ALLEGRO_COLOR color, float x, float y, int flags,
   char const *text) 
{
   ALLEGRO_USTR_INFO info;
   ASSERT(text);
   al_draw_ustr(font, color, x, y, flags, al_ref_cstr(&info, text));
}



/* Function: al_draw_justified_ustr
 */
void al_draw_justified_ustr(const ALLEGRO_FONT *font,
   ALLEGRO_COLOR color, float x1, float x2,
   float y, float diff, int flags, const ALLEGRO_USTR *ustr)
{
   const char *whitespace = " \t\n\r";
   ALLEGRO_USTR_INFO word_info;
   const ALLEGRO_USTR *word;
   int pos1, pos2;
   int minlen;
   int num_words;
   int space;
   float fleft, finc;

   (void)flags;
   
   ASSERT(font);

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
      font->vtable->render(font, color, ustr, x1, y);
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
      fleft += font->vtable->render(font, color, word, (int)fleft, y);
      fleft += finc;

      pos1 = pos2;
   }
}


/* Function: al_draw_justified_text
 */
void al_draw_justified_text(const ALLEGRO_FONT *font,
   ALLEGRO_COLOR color, float x1, float x2,
   float y, float diff, int flags, const char *text)
{
   ALLEGRO_USTR_INFO info;
   ASSERT(text);
   al_draw_justified_ustr(font, color, x1, x2, y, diff, flags,
      al_ref_cstr(&info, text));
}


/* Function: al_draw_textf
 */
void al_draw_textf(const ALLEGRO_FONT *font, ALLEGRO_COLOR color,
   float x, float y, int flags,
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
      al_draw_text(font, color, x, y, flags, s);
      va_end(ap);
      return;
   }

   va_start(ap, format);
   buf = al_ustr_new("");
   al_ustr_vappendf(buf, format, ap);
   va_end(ap);

   al_draw_text(font, color, x, y, flags, al_cstr(buf));

   al_ustr_free(buf);
}



/* Function: al_draw_justified_textf
 */
void al_draw_justified_textf(const ALLEGRO_FONT *f,
   ALLEGRO_COLOR color, float x1, float x2, float y,
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

   al_draw_justified_text(f, color, x1, x2, y, diff, flags,
      al_cstr(buf));

   al_ustr_free(buf);
}



/* Function: al_get_ustr_width
 */
int al_get_ustr_width(const ALLEGRO_FONT *f, ALLEGRO_USTR const *ustr)
{
   ASSERT(f);
   ASSERT(ustr);

   return f->vtable->text_length(f, ustr);
}



/* Function: al_get_text_width
 */
int al_get_text_width(const ALLEGRO_FONT *f, const char *str)
{
   ALLEGRO_USTR_INFO str_info;
   const ALLEGRO_USTR *ustr;
   ASSERT(f);
   ASSERT(str);

   ustr = al_ref_cstr(&str_info, str);

   return f->vtable->text_length(f, ustr);
}



/* Function: al_get_font_line_height
 */
int al_get_font_line_height(const ALLEGRO_FONT *f)
{
   ASSERT(f);
   return f->vtable->font_height(f);
}



/* Function: al_get_font_ascent
 */
int al_get_font_ascent(const ALLEGRO_FONT *f)
{
   ASSERT(f);
   return f->vtable->font_ascent(f);
}



/* Function: al_get_font_descent
 */
int al_get_font_descent(const ALLEGRO_FONT *f)
{
   ASSERT(f);
   return f->vtable->font_descent(f);
}



/* Function: al_get_ustr_dimensions
 */
void al_get_ustr_dimensions(const ALLEGRO_FONT *f,
   ALLEGRO_USTR const *ustr,
   int *bbx, int *bby, int *bbw, int *bbh)
{
   ASSERT(f);
   ASSERT(ustr);
   f->vtable->get_text_dimensions(f, ustr, bbx, bby,
      bbw, bbh);
}



/* Function: al_get_text_dimensions
 */
void al_get_text_dimensions(const ALLEGRO_FONT *f,
   char const *text,
   int *bbx, int *bby, int *bbw, int *bbh)
{
   ALLEGRO_USTR_INFO info;
   ASSERT(f);
   ASSERT(text);

   f->vtable->get_text_dimensions(f, al_ref_cstr(&info, text), bbx, bby,
      bbw, bbh);
}



/* Function: al_destroy_font
 */
void al_destroy_font(ALLEGRO_FONT *f)
{
   if (!f)
      return;

   _al_unregister_destructor(_al_dtor_list, f);

   f->vtable->destroy(f);
}


/* vim: set sts=3 sw=3 et: */
