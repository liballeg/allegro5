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
#include "allegro/aintern.h"



/* include bitmap data for the default font */
#include "font.h"

FONT *font = &_default_font;

int _textmode = 0;



/* find_range:
 *  Searches a font for a specific character.
 */
static INLINE AL_CONST FONT *find_range(AL_CONST FONT *f, int c)
{
   while (f) {
      if ((c >= f->start) && (c <= f->end))
	 return f;

      f = f->next;
   }

   return NULL;
}



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



/* masked_character:
 *  Helper routine for masked multicolor output of proportional fonts.
 */
static void masked_character(BITMAP *bmp, BITMAP *sprite, int x, int y, int color)
{
   bmp->vtable->draw_256_sprite(bmp, sprite, x, y);
}



/* opaque_character:
 *  Helper routine for opaque multicolor output of proportional fonts.
 */
static void opaque_character(BITMAP *bmp, BITMAP *sprite, int x, int y, int color)
{
   blit(sprite, bmp, 0, 0, x, y, sprite->w, sprite->h);
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
   void (*putter)() = NULL;
   AL_CONST FONT *range = NULL;
   int c;

   acquire_bitmap(bmp);

   while ((c = ugetx(&str)) != 0) {
      if ((!range) || (c < range->start) || (c > range->end)) {
	 /* search for a suitable character range */
	 range = find_range(f, c);

	 if (!range)
	    range = find_range(f, (c = '^'));

	 if (range) {
	    if (range->renderhook)
	       putter = NULL;
	    else if (range->mono)
	       putter = bmp->vtable->draw_glyph;
	    else if (color >= 0)
	       putter = bmp->vtable->draw_character;
	    else if (_textmode < 0)
	       putter = masked_character;
	    else
	       putter = opaque_character; 
	 }
      }

      if (range) {
	 /* draw the character */
	 c -= range->start;

	 if (range->renderhook)
	    range->renderhook(bmp, range->glyphs, c, x, y, color);
	 else
	    putter(bmp, range->glyphs[c], x, y, color);

	 if (range->widthhook)
	    x += range->widthhook(range->glyphs, c);
	 else if (range->mono)
	    x += ((FONT_GLYPH *)(range->glyphs[c]))->w;
	 else
	    x += ((BITMAP *)(range->glyphs[c]))->w;

	 if (x >= bmp->cr) {
	    release_bitmap(bmp);
	    return;
	 }
      }
   }

   release_bitmap(bmp);
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
void textout_justify(BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x1, int x2, int y, int diff, int color)
{
   char toks[32];
   char *tok[128];
   char strbuf[512];
   int i, minlen, last, space;
   float fleft, finc;

   i = usetc(toks, ' ');
   i += usetc(toks+i, '\t');
   i += usetc(toks+i, '\n');
   i += usetc(toks+i, '\r');
   usetc(toks+i, 0);

   /* count words and measure min length (without spaces) */ 
   minlen = 0;
   ustrcpy(strbuf, str);
   last = 0;

   for (tok[last] = ustrtok(strbuf, toks); tok[last]; tok[last] = ustrtok(NULL, toks)) {
      minlen += text_length(f, tok[last]);
      last++; 
   }

   /* amount of room for space between words */
   space = x2 - x1 - minlen;

   if ((space <= 0) || (space > diff) || (last < 2)) {
      /* can't justify */
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
}



/* textprintf:
 *  Formatted text output, using a printf() style format string.
 */
void textprintf(BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, AL_CONST char *format, ...)
{
   char buf[512];

   va_list ap;
   va_start(ap, format);
   uvsprintf(buf, format, ap);
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
   uvsprintf(buf, format, ap);
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
   uvsprintf(buf, format, ap);
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
   uvsprintf(buf, format, ap);
   va_end(ap);

   textout_justify(bmp, f, buf, x1, x2, y, diff, color);
}



/* text_length:
 *  Calculates the length of a string in a particular font.
 */
int text_length(AL_CONST FONT *f, AL_CONST char *str)
{
   AL_CONST FONT *range;
   int c, len;

   len = 0;

   while ((c = ugetx(&str)) != 0) {
      range = find_range(f, c);

      if (!range)
	 range = find_range(f, (c = '^'));

      if (range) {
	 c -= range->start;

	 if (range->widthhook)
	    len += range->widthhook(range->glyphs, c);
	 else if (range->mono)
	    len += ((FONT_GLYPH *)(range->glyphs[c]))->w;
	 else
	    len += ((BITMAP *)(range->glyphs[c]))->w;
      }
   }

   return len;
}



/* text_height:
 *  Returns the height of a character in the specified font.
 */
int text_height(AL_CONST FONT *f)
{
   if (f->heighthook)
      return f->heighthook(f->glyphs);
   else if (f->mono)
      return ((FONT_GLYPH *)(f->glyphs[0]))->h;
   else
      return ((BITMAP *)(f->glyphs[0]))->h;
}



/* destroy_font:
 *  Frees the memory being used by a font structure.
 */
void destroy_font(FONT *f)
{
   FONT *next;
   int c;

   while (f) {
      if (f->glyphs) {
	 if(f->destroyhook)
	    f->destroyhook(f->glyphs, f->start, f->end);
	 else {
	    for (c=0; c<=f->end-f->start; c++) {
	       if (f->glyphs[c]) {
		  if (f->mono)
		     free(f->glyphs[c]);
		  else
		     destroy_bitmap(f->glyphs[c]);
	       }
	    }
	 }

	 free(f->glyphs);
      }

      next = f->next;
      free(f);
      f = next;
   }
}

