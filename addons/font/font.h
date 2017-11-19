#ifndef __al_included_allegro5_font_h
#define __al_included_allegro5_font_h

#include "allegro5/internal/aintern_font.h"

extern ALLEGRO_FONT_VTABLE _al_font_vtable_color;

typedef struct ALLEGRO_FONT_COLOR_DATA
{
   int begin, end;                   /* first char and one-past-the-end char */
   ALLEGRO_BITMAP *glyphs;           /* our glyphs */
   ALLEGRO_BITMAP **bitmaps;         /* sub bitmaps pointing to our glyphs */
   struct ALLEGRO_FONT_COLOR_DATA *next;  /* linked list structure */
} ALLEGRO_FONT_COLOR_DATA;

ALLEGRO_FONT *_al_load_bitmap_font(const char *filename,
   int size, int flags);
ALLEGRO_FONT *_al_load_bmfont_xml(const char *filename,
   int size, int flags);


#endif
