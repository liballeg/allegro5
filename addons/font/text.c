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


#define ALLEGRO_INTERNAL_UNSTABLE

#include <math.h>
#include <ctype.h>
#include "allegro5/allegro.h"

#include "allegro5/allegro_font.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_font.h"
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
static void align_to_integer_pixel_inner(
   ALLEGRO_TRANSFORM const *fwd,
   ALLEGRO_TRANSFORM const *inv,
   float *x, float *y)
{
   al_transform_coordinates(fwd, x, y);
   *x = floorf(*x + 0.5f);
   *y = floorf(*y + 0.5f);
   al_transform_coordinates(inv, x, y);
}

static void align_to_integer_pixel(float *x, float *y)
{
   ALLEGRO_TRANSFORM const *fwd;
   ALLEGRO_TRANSFORM inv;

   fwd = al_get_current_transform();
   al_copy_transform(&inv, fwd);
   al_invert_transform(&inv);
   align_to_integer_pixel_inner(fwd, &inv, x, y);
}



/* Function: al_draw_ustr
 */
void al_draw_ustr(const ALLEGRO_FONT *font,
   ALLEGRO_COLOR color, float x, float y, int flags,
   const ALLEGRO_USTR *ustr)
{
   ASSERT(font);
   ASSERT(ustr);

   if (flags & ALLEGRO_ALIGN_CENTRE) {
      /* Use integer division to avoid introducing a fractional
       * component to an integer x value.
       */
      x -= font->vtable->text_length(font, ustr) / 2;
   }
   else if (flags & ALLEGRO_ALIGN_RIGHT) {
      x -= font->vtable->text_length(font, ustr);
   }

   if (flags & ALLEGRO_ALIGN_INTEGER)
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
   int advance;
   ALLEGRO_TRANSFORM const *fwd = NULL;
   ALLEGRO_TRANSFORM inv;

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
      if (flags & ALLEGRO_ALIGN_INTEGER)
         align_to_integer_pixel(&x1, &y);
      font->vtable->render(font, color, ustr, x1, y);
      return;
   }

   /* distribute space left evenly between words */
   fleft = (float)x1;
   finc = (float)space / (float)(num_words-1);
   pos1 = 0;

   if (flags & ALLEGRO_ALIGN_INTEGER) {
      fwd = al_get_current_transform();
      al_copy_transform(&inv, fwd);
      al_invert_transform(&inv);
   }

   for (;;) {
      pos1 = al_ustr_find_cset_cstr(ustr, pos1, whitespace);
      if (pos1 == -1)
         break;
      pos2 = al_ustr_find_set_cstr(ustr, pos1, whitespace);
      if (pos2 == -1)
         pos2 = al_ustr_size(ustr);

      word = al_ref_ustr(&word_info, ustr, pos1, pos2);
      if (flags & ALLEGRO_ALIGN_INTEGER) {
         float drawx = fleft;
         float drawy = y;
         align_to_integer_pixel_inner(fwd, &inv, &drawx, &drawy);
         advance = font->vtable->render(font, color, word, drawx, drawy);
      }
      else {
         advance = font->vtable->render(font, color, word, fleft, y);
      }
      fleft += advance + finc;
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

   al_draw_ustr(font, color, x, y, flags, buf);

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

   al_draw_justified_ustr(f, color, x1, x2, y, diff, flags, buf);

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

   _al_unregister_destructor(_al_dtor_list, f->dtor_item);

   f->vtable->destroy(f);
}


/* Function: al_get_font_ranges
 */
int al_get_font_ranges(ALLEGRO_FONT *f, int ranges_count, int *ranges)
{
   return f->vtable->get_font_ranges(f, ranges_count, ranges);
}

/* Function: al_draw_glyph
 */
void al_draw_glyph(const ALLEGRO_FONT *f, ALLEGRO_COLOR color, float x, float y,
   int codepoint)
{
   f->vtable->render_char(f, color, codepoint, x, y);
};

/* Function: al_get_glyph_width
 */
int al_get_glyph_width(const ALLEGRO_FONT *f, int codepoint)
{
   return f->vtable->char_length(f, codepoint);
}

/* Function: al_get_glyph_dimensions
 */
bool al_get_glyph_dimensions(const ALLEGRO_FONT *f,
   int codepoint, int *bbx, int *bby, int *bbw, int *bbh)
{
   return f->vtable->get_glyph_dimensions(f, codepoint, bbx, bby, bbw, bbh);
}

/* Function: al_get_glyph_advance
 */
int al_get_glyph_advance(const ALLEGRO_FONT *f, int codepoint1, int codepoint2)
{
   return f->vtable->get_glyph_advance(f, codepoint1, codepoint2);
}

/* Function: al_get_glyph
 */
bool al_get_glyph(const ALLEGRO_FONT *f, int prev_codepoint, int codepoint, ALLEGRO_GLYPH *glyph)
{
   return f->vtable->get_glyph(f, prev_codepoint, codepoint, glyph);
};


/* This helper function helps splitting an ustr in several delimited parts.
 * It returns an ustr that refers to the next part of the string that
 * is delimited by the delimiters in delimiter.
 * Returns NULL at the end of the string.
 * Pos is updated to byte index of character after the delimiter or
 * to the end of the string.
 */
static const ALLEGRO_USTR *ustr_split_next(const ALLEGRO_USTR *ustr,
   ALLEGRO_USTR_INFO *info, int *pos, const char *delimiter)
{
   const ALLEGRO_USTR *result;
   int end, size;

   size = al_ustr_size(ustr);
   if (*pos >= size) {
      return NULL;
   }

   end = al_ustr_find_set_cstr(ustr, *pos, delimiter);
   if (end == -1)
      end = size;

   result = al_ref_ustr(info, ustr, *pos, end);
   /* Set pos to character AFTER delimiter */
   al_ustr_next(ustr, &end);
   (*pos) = end;
   return result;
}



/* This returns the next "soft" line of text from ustr
 * that will fit in max_with using the font font, starting at pos *pos.
 * These are "soft" lines because they are broken up if needed at a space
 * or tab character.
 * This function updates pos if needed, and returns the next "soft" line,
 * or NULL if no more soft lines.
 * The soft line will not include the trailing space where the
 * line was split, but pos will be set to point to after that trailing
 * space so iteration can continue easily.
 */
static const ALLEGRO_USTR *get_next_soft_line(const ALLEGRO_USTR *ustr,
   ALLEGRO_USTR_INFO *info, int *pos,
   const ALLEGRO_FONT *font, float max_width)
{
   const ALLEGRO_USTR *result = NULL;
   const char *whitespace = " \t";
   int old_end = 0;
   int end = 0;
   int size = al_ustr_size(ustr);
   bool first_word = true;

   if (*pos >= size) {
      return NULL;
   }

   end = *pos;
   old_end = end;
   do {
      /* On to the next word. */
      end = al_ustr_find_set_cstr(ustr, end, whitespace);
      if (end < 0)
         end = size;

      /* Reference to the line that is being built. */
      result = al_ref_ustr(info, ustr, *pos, end);

      /* Check if the line is too long. If it is, return a soft line. */
      if (al_get_ustr_width(font, result) > max_width) {
         /* Corner case: a single word may not even fit the line.
          * In that case, return the word/line anyway as the "soft line",
          * the user can set a clip rectangle to cut it. */

         if (first_word) {
            /* Set pos to character AFTER end to allow easy iteration. */
            al_ustr_next(ustr, &end);
            *pos = end;
            return result;
         }
         else {
            /* Not first word, return old end position without the new word */
            result = al_ref_ustr(info, ustr, *pos, old_end);
            /* Set pos to character AFTER end to allow easy iteration. */
            al_ustr_next(ustr, &old_end);
            *pos = old_end;
            return result;
         }
      }
      first_word = false;
      old_end    = end;
      /* Skip the character at end which normally is whitespace. */
      al_ustr_next(ustr, &end);
   } while (end < size);

   /* If we get here the whole ustr will fit.*/
   result = al_ref_ustr(info, ustr, *pos, size);
   *pos = size;
   return result;
}


/* Function: al_do_multiline_ustr
 */
void al_do_multiline_ustr(const ALLEGRO_FONT *font, float max_width,
   const ALLEGRO_USTR *ustr,
   bool (*cb)(int line_num, const ALLEGRO_USTR * line, void *extra),
   void *extra)
{
   const char *linebreak  = "\n";
   const ALLEGRO_USTR *hard_line, *soft_line;
   ALLEGRO_USTR_INFO hard_line_info, soft_line_info;
   int hard_line_pos = 0, soft_line_pos = 0;
   int line_num = 0;
   bool proceed;

   /* For every "hard" line separated by a newline character... */
   hard_line = ustr_split_next(ustr, &hard_line_info, &hard_line_pos,
      linebreak);
   while (hard_line) {
      /* For every "soft" line in the "hard" line... */
      soft_line_pos = 0;
      soft_line =
      get_next_soft_line(hard_line, &soft_line_info, &soft_line_pos, font,
         max_width);
      /* No soft line here because it's an empty hard line. */
      if (!soft_line) {
         /* Call the callback with empty string to indicate an empty line. */
         proceed = cb(line_num, al_ustr_empty_string(), extra);
         if (!proceed) return;
         line_num ++;
      }
      while(soft_line) {
         /* Call the callback on the next soft line. */
         proceed = cb(line_num, soft_line, extra);
         if (!proceed) return;
         line_num++;

         soft_line = get_next_soft_line(hard_line, &soft_line_info,
            &soft_line_pos, font, max_width);
      }
      hard_line = ustr_split_next(ustr, &hard_line_info, &hard_line_pos,
         linebreak);
   }
}



/* Helper struct for al_do_multiline_text. */
typedef struct DO_MULTILINE_TEXT_EXTRA {
   bool (*callback)(int line_num, const char *line, int size, void *extra);
   void *extra;
} DO_MULTILINE_TEXT_EXTRA;



/* The functions do_multiline_text_cb is the helper callback
 * that "adapts" al_do_multiline_ustr to al_do_multiline_text.
 */
static bool do_multiline_text_cb(int line_num, const ALLEGRO_USTR *line,
   void *extra) {
   DO_MULTILINE_TEXT_EXTRA *s = extra;

   return s->callback(line_num, al_cstr(line), al_ustr_size(line), s->extra);
}



/* Function: al_do_multiline_text
 */
void al_do_multiline_text(const ALLEGRO_FONT *font,
   float max_width, const char *text,
   bool (*cb)(int line_num, const char *line, int size, void *extra),
   void *extra)
{
   ALLEGRO_USTR_INFO info;
   DO_MULTILINE_TEXT_EXTRA extra2;
   ASSERT(font);
   ASSERT(text);

   extra2.callback = cb;
   extra2.extra = extra;
   al_do_multiline_ustr(font, max_width, al_ref_cstr(&info, text),
      do_multiline_text_cb, &extra2);
}



/* Helper struct for al_draw_multiline_ustr. */
typedef struct DRAW_MULTILINE_USTR_EXTRA {
   const ALLEGRO_FONT *font;
   ALLEGRO_COLOR color;
   float x;
   float y;
   float line_height;
   int flags;
} DRAW_MULTILINE_USTR_EXTRA;



/* The function draw_multiline_ustr_cb is the helper callback
 * that implements the actual drawing for al_draw_multiline_ustr.
 */
static bool draw_multiline_ustr_cb(int line_num, const ALLEGRO_USTR *line,
   void *extra) {
   DRAW_MULTILINE_USTR_EXTRA *s = extra;
   float y;

   y  = s->y + (s->line_height * line_num);
   al_draw_ustr(s->font, s->color, s->x, y, s->flags, line);
   return true;
}



/* Function: al_draw_multiline_ustr
 */
void al_draw_multiline_ustr(const ALLEGRO_FONT *font,
     ALLEGRO_COLOR color, float x, float y, float max_width, float line_height,
     int flags, const ALLEGRO_USTR *ustr)
{
   DRAW_MULTILINE_USTR_EXTRA extra;
   ASSERT(font);
   ASSERT(ustr);

   extra.font = font;
   extra.color = color;
   extra.x = x;
   extra.y = y;
   if (line_height < 1) {
      extra.line_height =  al_get_font_line_height(font);
   }
   else {
      extra.line_height = line_height;
   }
   extra.flags = flags;

   al_do_multiline_ustr(font, max_width, ustr, draw_multiline_ustr_cb, &extra);
}



/* Function: al_draw_multiline_text
 */
void al_draw_multiline_text(const ALLEGRO_FONT *font,
     ALLEGRO_COLOR color, float x, float y, float max_width, float line_height,
     int flags, const char *text)
{
   ALLEGRO_USTR_INFO info;
   ASSERT(font);
   ASSERT(text);

   al_draw_multiline_ustr(font, color, x, y, max_width, line_height, flags,
      al_ref_cstr(&info, text));
}



/* Function: al_draw_multiline_textf
 */
void al_draw_multiline_textf(const ALLEGRO_FONT *font,
     ALLEGRO_COLOR color, float x, float y, float max_width, float line_height,
     int flags, const char *format, ...)
{
   ALLEGRO_USTR *buf;
   va_list ap;
   ASSERT(font);
   ASSERT(format);

   va_start(ap, format);
   buf = al_ustr_new("");
   al_ustr_vappendf(buf, format, ap);
   va_end(ap);

   al_draw_multiline_ustr(font, color, x, y, max_width, line_height, flags,
      buf);

   al_ustr_free(buf);
}


/* Function: al_set_fallback_font
 */
void al_set_fallback_font(ALLEGRO_FONT *font, ALLEGRO_FONT *fallback)
{
   font->fallback = fallback;
}

/* Function: al_get_fallback_font
 */
ALLEGRO_FONT *al_get_fallback_font(ALLEGRO_FONT *font)
{
   return font->fallback;
}


/* vim: set sts=3 sw=3 et: */
