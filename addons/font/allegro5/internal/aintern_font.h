#ifndef __al_included_allegro_aintern_font_h
#define __al_included_allegro_aintern_font_h

#include "allegro5/internal/aintern_list.h"

typedef struct A5O_FONT_VTABLE A5O_FONT_VTABLE;

struct A5O_FONT
{
   void *data;
   int height;
   A5O_FONT *fallback;
   A5O_FONT_VTABLE *vtable;
   _AL_LIST_ITEM *dtor_item;
};

/* text- and font-related stuff */
struct A5O_FONT_VTABLE
{
   A5O_FONT_METHOD(int, font_height, (const A5O_FONT *f));
   A5O_FONT_METHOD(int, font_ascent, (const A5O_FONT *f));
   A5O_FONT_METHOD(int, font_descent, (const A5O_FONT *f));
   A5O_FONT_METHOD(int, char_length, (const A5O_FONT *f, int ch));
   A5O_FONT_METHOD(int, text_length, (const A5O_FONT *f, const A5O_USTR *text));
   A5O_FONT_METHOD(int, render_char, (const A5O_FONT *f, A5O_COLOR color, int ch, float x, float y));
   A5O_FONT_METHOD(int, render, (const A5O_FONT *f, A5O_COLOR color, const A5O_USTR *text, float x, float y));
   A5O_FONT_METHOD(void, destroy, (A5O_FONT *f));
   A5O_FONT_METHOD(void, get_text_dimensions, (const A5O_FONT *f,
      const A5O_USTR *text, int *bbx, int *bby, int *bbw, int *bbh));
   A5O_FONT_METHOD(int, get_font_ranges, (A5O_FONT *font,
      int ranges_count, int *ranges));

   A5O_FONT_METHOD(bool, get_glyph_dimensions, (const A5O_FONT *f,
      int codepoint, int *bbx, int *bby, int *bbw, int *bbh));
   A5O_FONT_METHOD(int, get_glyph_advance, (const A5O_FONT *font,
      int codepoint1, int codepoint2));

   A5O_FONT_METHOD(bool, get_glyph, (const A5O_FONT *f, int prev_codepoint, int codepoint, A5O_GLYPH *glyph));
};

#endif
