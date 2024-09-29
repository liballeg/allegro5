#ifndef __al_included_allegro5_font_h
#define __al_included_allegro5_font_h

#include "allegro5/internal/aintern_font.h"

extern A5O_FONT_VTABLE _al_font_vtable_color;

typedef struct A5O_FONT_COLOR_DATA
{
   int begin, end;                   /* first char and one-past-the-end char */
   A5O_BITMAP *glyphs;           /* our glyphs */
   A5O_BITMAP **bitmaps;         /* sub bitmaps pointing to our glyphs */
   struct A5O_FONT_COLOR_DATA *next;  /* linked list structure */
} A5O_FONT_COLOR_DATA;

A5O_FONT *_al_load_bitmap_font(const char *filename,
   int size, int flags);
A5O_FONT *_al_load_bmfont_xml(const char *filename,
   int size, int flags);


#endif
