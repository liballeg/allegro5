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


#include "allegro5/a5_font.h"


/* textout_ex:
 *  Writes the null terminated string str onto bmp at position x, y, using 
 *  the specified font, foreground color and background color (-1 is trans).
 *  If color is -1 and a proportional font is in use, it will be drawn
 *  using the colors from the original font bitmap (the one imported into
 *  the grabber program), which allows multicolored text output.
 */
void a5font_textout(AL_CONST A5FONT_FONT *f, AL_CONST char *str, int x, int y)
{
   ASSERT(f);
   ASSERT(str);
   f->vtable->render(f, str, x, y);
}



/* textout_centre_ex:
 *  Like textout_ex(), but uses the x coordinate as the centre rather than 
 *  the left of the string.
 */
void a5font_textout_centre(AL_CONST A5FONT_FONT *f, AL_CONST char *str, int x, int y)
{
   int len;
   ASSERT(f);
   ASSERT(str);

   len = a5font_text_length(f, str);
   f->vtable->render(f, str, x - len/2, y);
}



/* textout_right_ex:
 *  Like textout_ex(), but uses the x coordinate as the right rather than 
 *  the left of the string.
 */
void a5font_textout_right(AL_CONST A5FONT_FONT *f, AL_CONST char *str, int x, int y)
{
   int len;
   ASSERT(f);
   ASSERT(str);

   len = a5font_text_length(f, str);
   f->vtable->render(f, str, x - len, y);
}



/* textout_justify_ex:
 *  Like textout_ex(), but justifies the string to the specified area.
 */
#define MAX_TOKEN  128

void a5font_textout_justify(AL_CONST A5FONT_FONT *f, AL_CONST char *str, int x1, int x2, int y, int diff)
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
      f->vtable->render(f, str, x1, y);
      return;
   }

   minlen = 0;
   last = 0;
   tok[last] = ustrtok_r(strbuf, toks, &strlast);

   while (tok[last]) {
      minlen += a5font_text_length(f, tok[last]);
      if (++last == MAX_TOKEN)
         break;
      tok[last] = ustrtok_r(NULL, toks, &strlast);
   }

   /* amount of room for space between words */
   space = x2 - x1 - minlen;

   if ((space <= 0) || (space > diff) || (last < 2)) {
      /* can't justify */
      _AL_FREE(strbuf);
      f->vtable->render(f, str, x1, y);
      return; 
   }

   /* distribute space left evenly between words */
   fleft = (float)x1;
   finc = (float)space / (float)(last-1);
   for (i=0; i<last; i++) {
      f->vtable->render(f, tok[i], (int)fleft, y);
      fleft += (float)a5font_text_length(f, tok[i]) + finc;
   }

   _AL_FREE(strbuf);
}



/* textprintf_ex:
 *  Formatted text output, using a printf() style format string.
 */
void a5font_textprintf(AL_CONST A5FONT_FONT *f, int x, int y, AL_CONST char *format, ...)
{
   char buf[512];
   va_list ap;
   ASSERT(f);
   ASSERT(format);

   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   a5font_textout(f, buf, x, y);
}



/* textprintf_centre_ex:
 *  Like textprintf_ex(), but uses the x coordinate as the centre rather than 
 *  the left of the string.
 */
void a5font_textprintf_centre(AL_CONST A5FONT_FONT *f, int x, int y, AL_CONST char *format, ...)
{
   char buf[512];
   va_list ap;
   ASSERT(f);
   ASSERT(format);

   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   a5font_textout_centre(f, buf, x, y);
}



/* textprintf_right_ex:
 *  Like textprintf_ex(), but uses the x coordinate as the right rather than 
 *  the left of the string.
 */
void a5font_textprintf_right(AL_CONST A5FONT_FONT *f, int x, int y, AL_CONST char *format, ...)
{
   char buf[512];
   va_list ap;
   ASSERT(f);
   ASSERT(format);

   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   a5font_textout_right(f, buf, x, y);
}



/* textprintf_justify_ex:
 *  Like textprintf_ex(), but right justifies the string to the specified area.
 */
void a5font_textprintf_justify(AL_CONST A5FONT_FONT *f, int x1, int x2, int y, int diff, AL_CONST char *format, ...)
{
   char buf[512];
   va_list ap;
   ASSERT(f);
   ASSERT(format);

   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   a5font_textout_justify(f, buf, x1, x2, y, diff);
}



/* text_length:
 *  Calculates the length of a string in a particular font.
 */
int a5font_text_length(AL_CONST A5FONT_FONT *f, AL_CONST char *str)
{
   ASSERT(f);
   ASSERT(str);
   return f->vtable->text_length(f, str);
}



/* text_height:
 *  Returns the height of a character in the specified font.
 */
int a5font_text_height(AL_CONST A5FONT_FONT *f)
{
   ASSERT(f);
   return f->vtable->font_height(f);
}



/* destroy_font:
 *  Frees the memory being used by a font structure.
 *  This is now wholly handled in the vtable.
 */
void a5font_destroy_font(A5FONT_FONT *f)
{
   ASSERT(f);
   f->vtable->destroy(f);
}

