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
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_memory.h"

#include "allegro5/a5_font.h"

/* If you call this, you're probably making a mistake. */
#define strlen(s)   __are_you_sure__



/* al_font_textout:
 *  Writes the null terminated string str onto bmp at position x, y, using 
 *  the specified font, foreground color and background color (-1 is trans).
 *  If color is -1 and a proportional font is in use, it will be drawn
 *  using the colors from the original font bitmap (the one imported into
 *  the grabber program), which allows multicolored text output.
 *  If count is not -1, write only the first 'count' code points.
 */
void al_font_textout(const ALLEGRO_FONT *f, int x, int y,
   const char *str, int count)
{
   ASSERT(f);
   ASSERT(str);

   if (count == -1) {
      count = ustrlen(str);
   }
   ASSERT(count >= 0);

   f->vtable->render(f, str, x, y, count);
}



/* al_font_textout_centre:
 *  Like <al_font_textout>, but uses the x coordinate as the centre rather than 
 *  the left of the string.
 */
void al_font_textout_centre(const ALLEGRO_FONT *f, int x, int y,
   const char *str, int count)
{
   int len;
   ASSERT(f);
   ASSERT(str);

   if (count == -1) {
      count = ustrlen(str);
   }
   ASSERT(count >= 0);

   len = al_font_text_width(f, str, count);
   f->vtable->render(f, str, x - len/2, y, count);
}



/* al_font_textout_right:
 *  Like <al_font_textout>, but uses the x coordinate as the right rather than 
 *  the left of the string.
 */
void al_font_textout_right(const ALLEGRO_FONT *f, int x, int y,
   const char *str, int count)
{
   int len;
   ASSERT(f);
   ASSERT(str);

   if (count == -1) {
      count = ustrlen(str);
   }
   ASSERT(count >= 0);

   len = al_font_text_width(f, str, count);
   f->vtable->render(f, str, x - len, y, count);
}



/* al_font_textout_justify:
 *  Like textout_ex(), but justifies the string to the specified area.
 */
#define MAX_TOKEN  128

/* XXX this looks pretty inefficient */
void al_font_textout_justify(const ALLEGRO_FONT *f, int x1, int x2, int y,
   int diff, const char *str)
{
   char toks[32];
   char *tok[MAX_TOKEN];
   char *strbuf, *strlast;
   int i, minlen, last, space;
   float fleft, finc;
   ASSERT(f);
   ASSERT(str);

   i = usetc(toks, ' ');
   i += usetc(toks+i, '\t');
   i += usetc(toks+i, '\n');
   i += usetc(toks+i, '\r');
   usetc(toks+i, 0);

   /* count words and measure min length (without spaces) */ 
   strbuf = ustrdup(str);
   if (!strbuf) {
      /* Can't justify ! */
      f->vtable->render(f, str, x1, y, ustrlen(str));
      return;
   }

   minlen = 0;
   last = 0;
   tok[last] = ustrtok_r(strbuf, toks, &strlast);

   while (tok[last]) {
      minlen += al_font_text_width(f, tok[last], -1);
      if (++last == MAX_TOKEN)
         break;
      tok[last] = ustrtok_r(NULL, toks, &strlast);
   }

   /* amount of room for space between words */
   space = x2 - x1 - minlen;

   if ((space <= 0) || (space > diff) || (last < 2)) {
      /* can't justify */
      _AL_FREE(strbuf);
      f->vtable->render(f, str, x1, y, ustrlen(str));
      return; 
   }

   /* distribute space left evenly between words */
   fleft = (float)x1;
   finc = (float)space / (float)(last-1);
   for (i=0; i<last; i++) {
      f->vtable->render(f, tok[i], (int)fleft, y, ustrlen(tok[i]));
      fleft += (float)al_font_text_width(f, tok[i], -1) + finc;
   }

   _AL_FREE(strbuf);
}



/* al_font_textprintf:
 *  Formatted text output, using a printf() style format string.
 */
void al_font_textprintf(const ALLEGRO_FONT *f, int x, int y, const char *format, ...)
{
   char buf[512];
   va_list ap;
   ASSERT(f);
   ASSERT(format);

   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   al_font_textout(f, x, y, buf, -1);
}



/* al_font_textprintf_centre:
 *  Like <al_font_textprintf>, but uses the x coordinate as the centre rather
 *  than the left of the string.
 */
void al_font_textprintf_centre(const ALLEGRO_FONT *f, int x, int y, const char *format, ...)
{
   char buf[512];
   va_list ap;
   ASSERT(f);
   ASSERT(format);

   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   al_font_textout_centre(f, x, y, buf, -1);
}



/* al_font_textprintf_right:
 *  Like <al_font_textprintf>, but uses the x coordinate as the right rather
 *  than the left of the string.
 */
void al_font_textprintf_right(const ALLEGRO_FONT *f, int x, int y, const char *format, ...)
{
   char buf[512];
   va_list ap;
   ASSERT(f);
   ASSERT(format);

   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   al_font_textout_right(f, x, y, buf, -1);
}



/* al_font_textprintf_justify:
 *  Like <al_font_textprintf>, but right justifies the string to the specified
 *  area.
 */
void al_font_textprintf_justify(const ALLEGRO_FONT *f, int x1, int x2, int y, int diff, const char *format, ...)
{
   char buf[512];
   va_list ap;
   ASSERT(f);
   ASSERT(format);

   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   al_font_textout_justify(f, x1, x2, y, diff, buf);
}



/* al_font_text_width:
 *  Calculates the length of a string in a particular font.
 */
int al_font_text_width(const ALLEGRO_FONT *f, const char *str, int count)
{
   ASSERT(f);
   ASSERT(str);

   if (count == -1) {
      count = ustrlen(str);
   }
   ASSERT(count >= 0);

   return f->vtable->text_length(f, str, count);
}



/* text_height:
 *  Returns the height of a character in the specified font.
 */
int al_font_text_height(const ALLEGRO_FONT *f)
{
   ASSERT(f);
   return f->vtable->font_height(f);
}



/* destroy_font:
 *  Frees the memory being used by a font structure.
 *  This is now wholly handled in the vtable.
 */
void al_font_destroy_font(ALLEGRO_FONT *f)
{
   ASSERT(f);
   f->vtable->destroy(f);
}


/* vim: set sts=3 sw=3 et: */
