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

#ifdef __cplusplus
   extern "C" {
#endif

#include "base.h"

struct BITMAP;

typedef struct FONT_GLYPH           /* a single monochrome font character */
{
   short w, h;
   unsigned char dat[ZERO_SIZE];
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
AL_FUNC(int, text_mode, (int mode));
AL_FUNC(void, textout, (struct BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x, int y, int color));
AL_FUNC(void, textout_centre, (struct BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x, int y, int color));
AL_FUNC(void, textout_right, (struct BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x, int y, int color));
AL_FUNC(void, textout_justify, (struct BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x1, int x2, int y, int diff, int color));
AL_PRINTFUNC(void, textprintf, (struct BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, AL_CONST char *format, ...), 6, 7);
AL_PRINTFUNC(void, textprintf_centre, (struct BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, AL_CONST char *format, ...), 6, 7);
AL_PRINTFUNC(void, textprintf_right, (struct BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, AL_CONST char *format, ...), 6, 7);
AL_PRINTFUNC(void, textprintf_justify, (struct BITMAP *bmp, AL_CONST FONT *f, int x1, int x2, int y, int diff, int color, AL_CONST char *format, ...), 8, 9);
AL_FUNC(int, text_length, (AL_CONST FONT *f, AL_CONST char *str));
AL_FUNC(int, text_height, (AL_CONST FONT *f));
AL_FUNC(void, destroy_font, (FONT *f));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_TEXT_H */


