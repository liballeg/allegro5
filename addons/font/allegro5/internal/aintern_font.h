#ifndef __al_included_allegro_aintern_font_h
#define __al_included_allegro_aintern_font_h

#include "allegro5/internal/aintern_list.h"

typedef struct ALLEGRO_FONT_VTABLE ALLEGRO_FONT_VTABLE;

struct ALLEGRO_FONT
{
   void *data;
   int height;
   ALLEGRO_FONT *fallback;
   ALLEGRO_FONT_VTABLE *vtable;
   _AL_LIST_ITEM *dtor_item;
};

/* text- and font-related stuff */
struct ALLEGRO_FONT_VTABLE
{
   ALLEGRO_FONT_METHOD(int, font_height, (const ALLEGRO_FONT *f));
   ALLEGRO_FONT_METHOD(int, font_ascent, (const ALLEGRO_FONT *f));
   ALLEGRO_FONT_METHOD(int, font_descent, (const ALLEGRO_FONT *f));
   ALLEGRO_FONT_METHOD(int, char_length, (const ALLEGRO_FONT *f, int ch));
   ALLEGRO_FONT_METHOD(int, text_length, (const ALLEGRO_FONT *f, const ALLEGRO_USTR *text));
   ALLEGRO_FONT_METHOD(int, render_char, (const ALLEGRO_FONT *f, ALLEGRO_COLOR color, int ch, float x, float y));
   ALLEGRO_FONT_METHOD(int, render_char_scaled, (const ALLEGRO_FONT *f, ALLEGRO_COLOR color, int ch, float x, float y, float scaleX, float scaleY));
   ALLEGRO_FONT_METHOD(int, render, (const ALLEGRO_FONT *f, ALLEGRO_COLOR color, const ALLEGRO_USTR *text, float x, float y));
   ALLEGRO_FONT_METHOD(int, render_scaled, (const ALLEGRO_FONT *f, ALLEGRO_COLOR color, const ALLEGRO_USTR *text, float x, float y, float scaleX, float scaleY));
   ALLEGRO_FONT_METHOD(void, destroy, (ALLEGRO_FONT *f));
   ALLEGRO_FONT_METHOD(void, get_text_dimensions, (const ALLEGRO_FONT *f,
      const ALLEGRO_USTR *text, int *bbx, int *bby, int *bbw, int *bbh));
   ALLEGRO_FONT_METHOD(int, get_font_ranges, (ALLEGRO_FONT *font,
      int ranges_count, int *ranges));

   ALLEGRO_FONT_METHOD(bool, get_glyph_dimensions, (const ALLEGRO_FONT *f,
      int codepoint, int *bbx, int *bby, int *bbw, int *bbh));
   ALLEGRO_FONT_METHOD(int, get_glyph_advance, (const ALLEGRO_FONT *font,
      int codepoint1, int codepoint2));

   ALLEGRO_FONT_METHOD(bool, get_glyph, (const ALLEGRO_FONT *f, int prev_codepoint, int codepoint, ALLEGRO_GLYPH *glyph));
};

#endif
