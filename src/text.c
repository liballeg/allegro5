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
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



int _textmode = 0;



/* text_mode:
 *  Sets the mode in which text will be drawn. If mode is positive, text
 *  output will be opaque and the background will be set to mode. If mode
 *  is negative, text will be drawn transparently (ie. the background will
 *  not be altered). The default is a mode of zero.
 *  Returns previous mode.
 */
int text_mode(int mode)
{
   int old_mode = _textmode;

   if (mode < 0)
      _textmode = -1;
   else
      _textmode = mode;

   return old_mode;
}



/* textout:
 *  Writes the null terminated string str onto bmp at position x, y, using 
 *  the current text mode and the specified font and foreground color.
 *  If color is -1 and a proportional font is in use, it will be drawn
 *  using the colors from the original font bitmap (the one imported into
 *  the grabber program), which allows multicolored text output.
 */
void textout(BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x, int y, int color)
{
   f->vtable->render(f, str, color, _textmode, bmp, x, y);
}



/* textout_centre:
 *  Like textout(), but uses the x coordinate as the centre rather than 
 *  the left of the string.
 */
void textout_centre(BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x, int y, int color)
{
   int len;

   len = text_length(f, str);
   textout(bmp, f, str, x - len/2, y, color);
}



/* textout_right:
 *  Like textout(), but uses the x coordinate as the right rather than 
 *  the left of the string.
 */
void textout_right(BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x, int y, int color)
{
   int len;

   len = text_length(f, str);
   textout(bmp, f, str, x - len, y, color);
}



/* textout_justify:
 *  Like textout(), but justifies the string to the specified area.
 */
#define MAX_TOKEN  128

void textout_justify(BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x1, int x2, int y, int diff, int color)
{
   char toks[32];
   char *tok[MAX_TOKEN];
   char *strbuf, *strlast;
   int i, minlen, last, space;
   float fleft, finc;

   i = usetc(toks, ' ');
   i += usetc(toks+i, '\t');
   i += usetc(toks+i, '\n');
   i += usetc(toks+i, '\r');
   usetc(toks+i, 0);

   /* count words and measure min length (without spaces) */ 
   strbuf = ustrdup(str);
   if (!strbuf) {
      /* Can't justify ! */
      textout(bmp, f, str, x1, y, color);
      return;
   }

   minlen = 0;
   last = 0;
   tok[last] = ustrtok_r(strbuf, toks, &strlast);

   while (tok[last]) {
      minlen += text_length(f, tok[last]);
      if (++last == MAX_TOKEN)
         break;
      tok[last] = ustrtok_r(NULL, toks, &strlast);
   }

   /* amount of room for space between words */
   space = x2 - x1 - minlen;

   if ((space <= 0) || (space > diff) || (last < 2)) {
      /* can't justify */
      free(strbuf);
      textout(bmp, f, str, x1, y, color);
      return; 
   }

   /* distribute space left evenly between words */
   fleft = (float)x1;
   finc = (float)space / (float)(last-1);
   for (i=0; i<last; i++) {
      textout(bmp, f, tok[i], (int)fleft, y, color);
      fleft += (float)text_length(f, tok[i]) + finc;
   }

   free(strbuf);
}



/* textprintf:
 *  Formatted text output, using a printf() style format string.
 */
void textprintf(BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, AL_CONST char *format, ...)
{
   char buf[512];

   va_list ap;
   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   textout(bmp, f, buf, x, y, color);
}



/* textprintf_centre:
 *  Like textprintf(), but uses the x coordinate as the centre rather than 
 *  the left of the string.
 */
void textprintf_centre(BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, AL_CONST char *format, ...)
{
   char buf[512];

   va_list ap;
   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   textout_centre(bmp, f, buf, x, y, color);
}



/* textprintf_right:
 *  Like textprintf(), but uses the x coordinate as the right rather than 
 *  the left of the string.
 */
void textprintf_right(BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, AL_CONST char *format, ...)
{
   char buf[512];

   va_list ap;
   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   textout_right(bmp, f, buf, x, y, color);
}



/* textprintf_justify:
 *  Like textprintf(), but right justifies the string to the specified area.
 */
void textprintf_justify(BITMAP *bmp, AL_CONST FONT *f, int x1, int x2, int y, int diff, int color, AL_CONST char *format, ...)
{
   char buf[512];

   va_list ap;
   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   textout_justify(bmp, f, buf, x1, x2, y, diff, color);
}



/* text_length:
 *  Calculates the length of a string in a particular font.
 */
int text_length(AL_CONST FONT *f, AL_CONST char *str)
{
   return f->vtable->text_length(f, str);
}



/* text_height:
 *  Returns the height of a character in the specified font.
 */
int text_height(AL_CONST FONT *f)
{
   return f->vtable->font_height(f);
}



/* destroy_font:
 *  Frees the memory being used by a font structure.
 *  This is now wholly handled in the vtable.
 */
void destroy_font(FONT *f)
{
   f->vtable->destroy(f);
}

