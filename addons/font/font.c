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
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_vector.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/internal/aintern_bitmap.h"

#include "font.h"


typedef struct
{
   ALLEGRO_USTR *extension;
   ALLEGRO_FONT *(*load_font)(char const *filename, int size, int flags);
} FONT_HANDLER;


static _AL_VECTOR font_handlers;


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


static ALLEGRO_FONT_COLOR_DATA *_al_font_find_page(
   ALLEGRO_FONT_COLOR_DATA *cf, int ch)
{
    while(cf) {
        if(ch >= cf->begin && ch < cf->end)
            return cf;
        cf = cf->next;
    }
    return NULL;
}


/* _color_find_glyph:
 *  Helper for color vtable entries, below.
 */
static ALLEGRO_BITMAP* _al_font_color_find_glyph(const ALLEGRO_FONT* f, int ch)
{
    ALLEGRO_FONT_COLOR_DATA* cf = (ALLEGRO_FONT_COLOR_DATA*)(f->data);

    cf = _al_font_find_page(cf, ch);
    if (cf) {
        return cf->bitmaps[ch - cf->begin];
    }

    /* if we don't find the character, then search for the missing
       glyph, but don't get stuck in a loop. */
    if(ch != al_font_404_character)
        return _al_font_color_find_glyph(f, al_font_404_character);
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

static int quick_process_char(const ALLEGRO_FONT *f, int ch, int x, int y,
   ALLEGRO_VERTEX *verts)
{
    ALLEGRO_BITMAP *g;
    int w, h;

    g = _al_font_color_find_glyph(f, ch);
    w = al_get_bitmap_width(g);
    h = al_get_bitmap_height(g);

    if(g) {
        int y1 = y + ((float)h - al_get_bitmap_height(g))/2.0f;
        ALLEGRO_PRIM_COLOR color = al_get_prim_color(*_al_get_blend_color());

        float tu_s = g->xofs;
        float tv_s = g->yofs;
        float tu_e = tu_s + w;
        float tv_e = tv_s + h;
        int x2 = x + w;
        int y2 = y + h;

        verts[0].x = x;
        verts[0].y = y1;
        verts[0].u = tu_s;
        verts[0].v = tv_s;
        verts[0].color = color;

        verts[1].x = x2;
        verts[1].y = y2;
        verts[1].u = tu_e;
        verts[1].v = tv_e;
        verts[1].color = color;

        verts[2].x = x;
        verts[2].y = y2;
        verts[2].u = tu_s;
        verts[2].v = tv_e;
        verts[2].color = color;

        verts[3].x = x;
        verts[3].y = y1;
        verts[3].u = tu_s;
        verts[3].v = tv_s;
        verts[3].color = color;

        verts[4].x = x2;
        verts[4].y = y1;
        verts[4].u = tu_e;
        verts[4].v = tv_s;
        verts[4].color = color;

        verts[5].x = x2;
        verts[5].y = y2;
        verts[5].u = tu_e;
        verts[5].v = tv_e;
        verts[5].color = color;
    }

    return w;
}

static int quick_color_render(const ALLEGRO_FONT *f, const ALLEGRO_USTR *text,
   int x0, int y)
{
    int pos = 0;
    int x = 0;
    int32_t ch;
    int i, j;
    int total = 0;
    int length = al_ustr_length(text);
    const int MAX_CHARS = 100;
    ALLEGRO_VERTEX verts[6*MAX_CHARS];
   
    while (total < length) {
        i = j = 0;
        while ((ch = al_ustr_get_next(text, &pos)) >= 0 && j < MAX_CHARS) {
            x += quick_process_char(f, ch, x+x0, y, &verts[i]);
            i += 6;
            j++;
            total++;
        }

        ch = al_ustr_get(text, 0);
         if (ch >= 0) {
                 ALLEGRO_FONT_COLOR_DATA *cf = (ALLEGRO_FONT_COLOR_DATA *)f->data;
                 cf = _al_font_find_page(cf, ch);

                 if (cf) {
                         al_draw_prim(verts, 0, cf->glyphs, 0, i,
                                  ALLEGRO_PRIM_TRIANGLE_LIST);
                 }

         }
    }

    return x - x0;
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

    /*
     * If all of the characters are on the same glyph page,
     * we can draw them all at once with the primitives addon
     */
    ALLEGRO_FONT_COLOR_DATA *cf = (ALLEGRO_FONT_COLOR_DATA *)f->data;
    bool started = false;
    bool can_draw_quickly = true;

    /* This slows down on short text, so set a arbitrary minimum */
    if (al_ustr_length(text) <= 6) {
        can_draw_quickly = false;
    }

    while (can_draw_quickly && (ch = al_ustr_get_next(text, &pos)) >= 0) {
        if (!started) {
            started = true;
            cf = _al_font_find_page(cf, ch);
            if (!cf) {
                can_draw_quickly = false;
                break;
            }
        }
        else {
            if (cf != _al_font_find_page(cf, ch)) {
                can_draw_quickly = false;
                break;
            }
		  }
    }
    if (can_draw_quickly) {
        return quick_color_render(f, text, x0, y);
    }

    pos = 0;

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

    if(!f)
        return;

    cf = (ALLEGRO_FONT_COLOR_DATA*)(f->data);
    
    if (cf)
        glyphs = cf->glyphs;

    while(cf) {
        ALLEGRO_FONT_COLOR_DATA* next = cf->next;
        int i = 0;

        for(i = cf->begin; i < cf->end; i++) al_destroy_bitmap(cf->bitmaps[i - cf->begin]);
        /* Each range might point to the same bitmap. */
        if (cf->glyphs != glyphs) {
            al_destroy_bitmap(cf->glyphs);
            cf->glyphs = NULL;
        }

        if (!next && cf->glyphs)
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
    color_destroy,
    color_get_text_dimensions
};

ALLEGRO_FONT_VTABLE* al_font_vtable_color = &_al_font_vtable_color;


static void font_shutdown(void)
{
    while (!_al_vector_is_empty(&font_handlers)) {
       FONT_HANDLER *h = _al_vector_ref_back(&font_handlers);
       al_ustr_free(h->extension);
       _al_vector_delete_at(&font_handlers, _al_vector_size(&font_handlers)-1);
    }
    _al_vector_free(&font_handlers);
}


/* Function: al_init_font_addon
 */
void al_init_font_addon(void)
{
   _al_vector_init(&font_handlers, sizeof(FONT_HANDLER));
   al_init_image_addon(); /* we depend on the iio addon */
   
   al_register_font_loader(".bmp", _al_load_bitmap_font);
   al_register_font_loader(".jpg", _al_load_bitmap_font);
   al_register_font_loader(".pcx", _al_load_bitmap_font);
   al_register_font_loader(".png", _al_load_bitmap_font);
   al_register_font_loader(".tga", _al_load_bitmap_font);

   _al_add_exit_func(font_shutdown, "font_shutdown");
}


/* Function: al_shutdown_font_addon
 */
void al_shutdown_font_addon(void)
{
   font_shutdown();
}


static FONT_HANDLER *find_extension(char const *extension)
{
   int i;
   /* Go backwards so a handler registered later for the same extension
    * has precedence.
    */
   for (i = _al_vector_size(&font_handlers) - 1; i >= 0 ; i--) {
      FONT_HANDLER *handler = _al_vector_ref(&font_handlers, i);
      if (0 == stricmp(al_cstr(handler->extension), extension))
         return handler;
   }
   return NULL;
}



/* Function: al_register_font_loader
 */
bool al_register_font_loader(char const *extension,
   ALLEGRO_FONT *(*load_font)(char const *filename, int size, int flags))
{
   FONT_HANDLER *handler = find_extension(extension);
   if (!handler) {
      if (!load_font)
         return false; /* Nothing to remove. */
      handler = _al_vector_alloc_back(&font_handlers);
      handler->extension = al_ustr_new(extension);
   }
   else {
      if (!load_font)
         return _al_vector_find_and_delete(&font_handlers, handler);
   }
   handler->load_font = load_font;
   return true;
}



/* Function: al_load_font
 */
ALLEGRO_FONT *al_load_font(char const *filename, int size, int flags)
{
   int i;
   const char *ext;
   FONT_HANDLER *handler;

   ASSERT(filename);

   ext = strrchr(filename, '.');
   if (!ext)
      return NULL;
   handler = find_extension(ext);
   if (handler)
      return handler->load_font(filename, size, flags);

   /* No handler for the extension was registered - try to load with
    * all registered font_handlers and see if one works. So if the user
    * does:
    * 
    * al_init_font_addon()
    * al_init_ttf_addon()
    * 
    * This will first try to load an unknown (let's say Type1) font file
    * with Freetype (and load it successfully in this case), then try
    * to load it as a bitmap font.
    */
   for (i = _al_vector_size(&font_handlers) - 1; i >= 0 ; i--) {
      FONT_HANDLER *handler = _al_vector_ref(&font_handlers, i);
      ALLEGRO_FONT *try = handler->load_font(filename, size, flags);
      if (try)
         return try;
   }

   return NULL;
}



/* Function: al_get_allegro_font_version
 */
uint32_t al_get_allegro_font_version(void)
{
   return ALLEGRO_VERSION_INT;
}


/* vim: set sts=4 sw=4 et: */
 
