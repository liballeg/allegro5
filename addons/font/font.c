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
 *
 *      See readme.txt for copyright information.
 */

#include <string.h>
#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_vector.h"
#include "allegro5/a5_font.h"
#include "allegro5/a5_iio.h"


#include "font.h"

typedef struct
{
   ALLEGRO_USTR *extension;
   ALLEGRO_FONT *(*load_font)(char const *filename, int size, int flags);
} HANDLER;

static _AL_VECTOR handlers;

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



static void color_get_text_dimensions(ALLEGRO_FONT const *f,
   const ALLEGRO_USTR *text,
   int *bbx, int *bby, int *bbw, int *bbh, int *ascent,
   int *descent)
{
   /* Dummy implementation - for A4-style bitmap fonts the bounding
    * box of text is its width and line-height. And since we have no
    * idea about the baseline position, ascent is the full height and
    * descent is 0.
    */
   int h = al_get_font_line_height(f);
   if (bbx) *bbx = 0;
   if (bby) *bby = 0;
   if (bbw) *bbw = length(f, text);
   if (bbh) *bbh = h;
   if (ascent) *ascent = h;
   if (descent) *descent = 0;
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
    ALLEGRO_BITMAP *glyphs = NULL;

    if(!f) return;

    cf = (ALLEGRO_FONT_COLOR_DATA*)(f->data);
    
    if (cf)
        glyphs = cf->glyphs;

    while(cf) {
        ALLEGRO_FONT_COLOR_DATA* next = cf->next;
        int i = 0;

        for(i = cf->begin; i < cf->end; i++) al_destroy_bitmap(cf->bitmaps[i - cf->begin]);
        /* Each range might point to the same bitmap. */
        if (cf->glyphs != glyphs) al_destroy_bitmap(cf->glyphs);

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
    color_destroy,
    color_get_text_dimensions
};

ALLEGRO_FONT_VTABLE* al_font_vtable_color = &_al_font_vtable_color;



/* Function: al_init_font_addon
 */
void al_init_font_addon(void)
{
   _al_vector_init(&handlers, sizeof(HANDLER));
   al_init_iio_addon(); /* we depend on the iio addon */
   
   al_register_font_extension(".bmp", _al_load_bitmap_font);
   al_register_font_extension(".jpg", _al_load_bitmap_font);
   al_register_font_extension(".pcx", _al_load_bitmap_font);
   al_register_font_extension(".png", _al_load_bitmap_font);
   al_register_font_extension(".tga", _al_load_bitmap_font);
}



static HANDLER *find_extension(char const *extension)
{
   int i;
   /* Go backwards so a handler registered later for the same extension
    * has precedence.
    */
   for (i = _al_vector_size(&handlers) - 1; i >= 0 ; i--) {
      HANDLER *handler = _al_vector_ref(&handlers, i);
      if (0 == strcmp(al_cstr(handler->extension), extension))
         return handler;
   }
   return NULL;
}



/* Function: al_register_font_extension
 */
bool al_register_font_extension(char const *extension,
   ALLEGRO_FONT *(*load_font)(char const *filename, int size, int flags))
{
   HANDLER *handler = find_extension(extension);
   if (!handler) {
      if (!load_font) return false; /* Nothing to remove. */
      handler = _al_vector_alloc_back(&handlers);
   }
   else {
      if (!load_font)
         return _al_vector_find_and_delete(&handlers, handler);
      al_ustr_free(handler->extension);
   }
   handler->extension = al_ustr_new(extension);
   handler->load_font = load_font;
   return true;
}



/* Function: al_load_font
 */
ALLEGRO_FONT *al_load_font(char const *filename, int size, int flags)
{
   int i;
   ALLEGRO_PATH *path = al_path_create(filename);
   HANDLER *handler = find_extension(al_path_get_extension(path));
   if (handler)
      return handler->load_font(filename, size, flags);

   /* No handler for the extension was registered - try to load with
    * all registered handlers and see if one works. So if the user
    * does:
    * 
    * al_init_font_addon()
    * al_init_ttf_addon()
    * 
    * This will first try to load an unknown (let's say Type1) font file
    * with Freetype (and load it successfully in this case), then try
    * to load it as a bitmap font.
    */
   for (i = _al_vector_size(&handlers) - 1; i >= 0 ; i--) {
      HANDLER *handler = _al_vector_ref(&handlers, i);
      ALLEGRO_FONT *try = handler->load_font(filename, size, flags);
      if (try)
         return try;
   }
   return NULL;
}



/* vim: set sts=4 sw=4 et: */
