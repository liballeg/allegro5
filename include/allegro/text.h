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
 *      Text output routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_TEXT_H
#define ALLEGRO_TEXT_H

#include "base.h"

#ifdef __cplusplus
   extern "C" {
#endif

struct BITMAP;

typedef struct FONT_GLYPH           /* a single monochrome font character */
{
   short w, h;
   ZERO_SIZE_ARRAY(unsigned char, dat);
} FONT_GLYPH;


struct FONT_VTABLE;

typedef struct FONT
{
   void *data;
   int height;
   struct FONT_VTABLE *vtable;
} FONT;

AL_VAR(FONT *, font);
AL_VAR(int, allegro_404_char);
AL_FUNC(void, textout_ex, (struct BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x, int y, int color, int bg));
AL_FUNC(void, textout_centre_ex, (struct BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x, int y, int color, int bg));
AL_FUNC(void, textout_right_ex, (struct BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x, int y, int color, int bg));
AL_FUNC(void, textout_justify_ex, (struct BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x1, int x2, int y, int diff, int color, int bg));
AL_PRINTFUNC(void, textprintf_ex, (struct BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, int bg, AL_CONST char *format, ...), 7, 8);
AL_PRINTFUNC(void, textprintf_centre_ex, (struct BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, int bg, AL_CONST char *format, ...), 7, 8);
AL_PRINTFUNC(void, textprintf_right_ex, (struct BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, int bg, AL_CONST char *format, ...), 7, 8);
AL_PRINTFUNC(void, textprintf_justify_ex, (struct BITMAP *bmp, AL_CONST FONT *f, int x1, int x2, int y, int diff, int color, int bg, AL_CONST char *format, ...), 9, 10);
AL_FUNC(int, text_length, (AL_CONST FONT *f, AL_CONST char *str));
AL_FUNC(int, text_height, (AL_CONST FONT *f));
AL_FUNC(void, destroy_font, (FONT *f));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_TEXT_H */


