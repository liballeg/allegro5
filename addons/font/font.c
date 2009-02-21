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
 *      The default 8x8 font, and mono and color font vtables.
 *
 *      Contains characters:
 *
 *          ASCII          (0x0020 to 0x007F)
 *          Latin-1        (0x00A1 to 0x00FF)
 *          Extended-A     (0x0100 to 0x017F)
 *          Euro           (0x20AC)
 *
 *      Elias Pschernig added the Euro character.
 *
 *      See readme.txt for copyright information.
 */

#include <string.h>
#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/a5_font.h"
#include "allegro5/a5_iio.h"


#include "font.h"


/* al_font_404_character:
 *  This is what we render missing glyphs as.
 */
static int al_font_404_character = '^';



/* font_height:
 *  (mono and color vtable entry)
 *  Returns the height, in pixels of the font.
 */
static int font_height(const ALLEGRO_FONT *f)
{
   ASSERT(f);
   return f->height;
}



/* length:
 *  (mono and color vtable entry)
 *  Returns the length, in pixels, of a string as rendered in a font.
 */
static int length(const ALLEGRO_FONT* f, const ALLEGRO_USTR *text)
{
    int ch = 0, w = 0;
    int pos = 0;
    ASSERT(f);

    while ((ch = al_ustr_get_next(text, &pos)) >= 0) {
        w += f->vtable->char_length(f, ch);
    }

    return w;
}



/* _color_find_glyph:
 *  Helper for color vtable entries, below.
 */
ALLEGRO_BITMAP* _al_font_color_find_glyph(const ALLEGRO_FONT* f, int ch)
{
    ALLEGRO_FONT_COLOR_DATA* cf = (ALLEGRO_FONT_COLOR_DATA*)(f->data);

    while(cf) {
        if(ch >= cf->begin && ch < cf->end) return cf->bitmaps[ch - cf->begin];
        cf = cf->next;
    }

    /* if we don't find the character, then search for the missing
       glyph, but don't get stuck in a loop. */
    if(ch != al_font_404_character) return _al_font_color_find_glyph(f, al_font_404_character);
    return 0;
}



/* color_char_length:
 *  (color vtable entry)
 *  Returns the length of a character, in pixels, as it would be rendered
 *  in this font.
 */
static int color_char_length(const ALLEGRO_FONT* f, int ch)
{
    ALLEGRO_BITMAP* g = _al_font_color_find_glyph(f, ch);
    return g ? al_get_bitmap_width(g) : 0;
}



/* color_render_char:
 *  (color vtable entry)
 *  Renders a color character onto a bitmap, at the specified location, using
 *  the specified colors. If fg == -1, render as color, else render as
 *  mono; if bg == -1, render as transparent, else render as opaque.
 *  Returns the character width, in pixels.
 */
static int color_render_char(const ALLEGRO_FONT* f, int ch, int x, int y)
{
    int w = 0;
    int h = f->vtable->font_height(f);
    ALLEGRO_BITMAP *g;

    g = _al_font_color_find_glyph(f, ch);
    if(g) {
        al_draw_bitmap(g, x, y + ((float)h - al_get_bitmap_height(g))/2.0f, 0);

        w = al_get_bitmap_width(g);
    }

    return w;
}



/* color_render:
 *  (color vtable entry)
 *  Renders a color font onto a bitmap, at the specified location, using
 *  the specified colors. If fg == -1, render as color, else render as
 *  mono; if bg == -1, render as transparent, else render as opaque.
 */
static int color_render(const ALLEGRO_FONT* f, const ALLEGRO_USTR *text,
    int x0, int y)
{
    int pos = 0;
    int x = x0;
    int32_t ch;

    while ((ch = al_ustr_get_next(text, &pos)) >= 0) {
        x += f->vtable->render_char(f, ch, x, y);
    }
    return x - x0;
}



/* color_destroy:
 *  (color vtable entry)
 *  Destroys a color font.
 */
static void color_destroy(ALLEGRO_FONT* f)
{
    ALLEGRO_FONT_COLOR_DATA* cf;

    if(!f) return;

    cf = (ALLEGRO_FONT_COLOR_DATA*)(f->data);

    while(cf) {
        ALLEGRO_FONT_COLOR_DATA* next = cf->next;
        int i = 0;

        for(i = cf->begin; i < cf->end; i++) al_destroy_bitmap(cf->bitmaps[i - cf->begin]);
        al_destroy_bitmap(cf->glyphs);

        _AL_FREE(cf->bitmaps);
        _AL_FREE(cf);

        cf = next;
    }

    _AL_FREE(f);
}


/********
 * vtable declarations
 ********/

ALLEGRO_FONT_VTABLE _al_font_vtable_color = {  
    font_height,
    color_char_length,
    length,
    color_render_char,
    color_render,
    color_destroy
};

ALLEGRO_FONT_VTABLE* al_font_vtable_color = &_al_font_vtable_color;



/* Function: al_font_is_compatible_font
 *  Returns non-zero if the two fonts are of similar type.
 */
int al_font_is_compatible_font(ALLEGRO_FONT *f1, ALLEGRO_FONT *f2)
{
   ASSERT(f1);
   ASSERT(f2);
   return f1->vtable == f2->vtable;
}



/* Function: al_font_init
 */
void al_font_init(void)
{
   al_iio_init(); /* we depend on the iio addon */
}


/* vim: set sts=4 sw=4 et: */
