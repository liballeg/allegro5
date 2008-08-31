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
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
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
static int font_height(AL_CONST ALLEGRO_FONT *f)
{
   ASSERT(f);
   return f->height;
}



/* length:
 *  (mono and color vtable entry)
 *  Returns the length, in pixels, of a string as rendered in a font.
 */
static int length(AL_CONST ALLEGRO_FONT* f, AL_CONST char* text, int count)
{
    int ch = 0, w = 0, i;
    AL_CONST char* p = text;
    ASSERT(text);
    ASSERT(f);

    for (i = 0; i < count; i++) {
        ch = ugetxc(&p);
        w += f->vtable->char_length(f, ch);
    }

    return w;
}



/* _color_find_glyph:
 *  Helper for color vtable entries, below.
 */
ALLEGRO_BITMAP* _al_font_color_find_glyph(AL_CONST ALLEGRO_FONT* f, int ch)
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
static int color_char_length(AL_CONST ALLEGRO_FONT* f, int ch)
{
    ALLEGRO_BITMAP* g = _al_font_color_find_glyph(f, ch);
    return g ? g->w : 0;
}



/* color_render_char:
 *  (color vtable entry)
 *  Renders a color character onto a bitmap, at the specified location, using
 *  the specified colors. If fg == -1, render as color, else render as
 *  mono; if bg == -1, render as transparent, else render as opaque.
 *  Returns the character width, in pixels.
 */
static int color_render_char(AL_CONST ALLEGRO_FONT* f, int ch, int x, int y)
{
    int w = 0;
    int h = f->vtable->font_height(f);
    ALLEGRO_BITMAP *g;

    g = _al_font_color_find_glyph(f, ch);
    if(g) {
        al_draw_bitmap(g, x, y + ((float)h-g->h)/2.0f, 0);

	w = g->w;
    }

    return w;
}



/* color_render:
 *  (color vtable entry)
 *  Renders a color font onto a bitmap, at the specified location, using
 *  the specified colors. If fg == -1, render as color, else render as
 *  mono; if bg == -1, render as transparent, else render as opaque.
 */
static void color_render(AL_CONST ALLEGRO_FONT* f, AL_CONST char* text, int x, int y, int count)
{
    AL_CONST char* p = text;
    int i = 0, ch;

    for (; i < count; i++) {
        ch = ugetxc(&p);
        x += f->vtable->render_char(f, ch, x, y);
    }
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


#if 0
/* color_get_font_ranges:
 *  (color vtable entry)
 *  Returns the number of character ranges in a font, or -1 if that information
 *   is not available.
 */
static int color_get_font_ranges(ALLEGRO_FONT *f)
{
    ALLEGRO_FONT_COLOR_DATA* cf = 0;
    int ranges = 0;

    if (!f) 
        return -1;

    cf = (ALLEGRO_FONT_COLOR_DATA*)(f->data);

    while(cf) {
        ALLEGRO_FONT_COLOR_DATA* next = cf->next;
        
        ranges++;
        if (!next)
            return ranges;
         cf = next;
    }

    return -1;
}



/* color_get_font_range_begin:
 *  (color vtable entry)
 *  Get first character for font.
 */
static int color_get_font_range_begin(ALLEGRO_FONT* f, int range)
{
   ALLEGRO_FONT_COLOR_DATA* cf = 0;
   int n;

   if (!f || !f->data) 
      return -1;
      
   if (range < 0)
      range = 0;
   n = 0;

   cf = (ALLEGRO_FONT_COLOR_DATA*)(f->data);
   while(cf && n<=range) {
      ALLEGRO_FONT_COLOR_DATA* next = cf->next;
        
      if (!next || range == n)
         return cf->begin;
      cf = next;
      n++;
   }

   return -1;
}



/* color_get_font_range_end:
 *  (color vtable entry)
 *  Get last character for font range.
 */
static int color_get_font_range_end(ALLEGRO_FONT* f, int range)
{
   ALLEGRO_FONT_COLOR_DATA* cf = 0;
   int n;

   if (!f) 
      return -1;

   n = 0;

   cf = (ALLEGRO_FONT_COLOR_DATA*)(f->data);

   while(cf && (n<=range || range==-1)) {
      ALLEGRO_FONT_COLOR_DATA* next = cf->next;
      if (!next || range == n)
         return cf->end - 1;
      cf = next;
      n++;
   }

   return -1;
}



/* color_copy_glyph_range:
 *  Colour font helper function. Copies (part of) a glyph range
 */
static ALLEGRO_FONT_COLOR_DATA *color_copy_glyph_range(ALLEGRO_FONT_COLOR_DATA *cf, int begin, int end)
{
   ALLEGRO_FONT_COLOR_DATA *newcf;
   ALLEGRO_BITMAP **gl;
   ALLEGRO_BITMAP *g;
   int num, c;
   int src_mode, dst_mode;
   ALLEGRO_COLOR blend_color, white;
   
   if (begin<cf->begin || end>cf->end)
      return NULL;
   
   newcf = _AL_MALLOC(sizeof *newcf);

   if (!newcf)
      return NULL;

   newcf->begin = begin;
   newcf->end = end;
   newcf->next = NULL;
   num = end - begin;

   _al_push_new_bitmap_parameters();
   //al_set_new_bitmap_flags(0);
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);
   al_get_blender(&src_mode, &dst_mode, &blend_color);
   white = al_map_rgb(255, 255, 255);
   al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, white);
   _al_push_target_bitmap();

   gl = newcf->bitmaps = _AL_MALLOC(num * sizeof *gl);
   for (c=0; c<num; c++) {
      g = cf->bitmaps[begin - cf->begin + c];
      gl[c] = al_create_bitmap(g->w, g->h);
      al_set_target_bitmap(gl[c]);
      al_draw_bitmap(g, 0, 0, 0);
   }

   _al_pop_new_bitmap_parameters();
   al_set_blender(src_mode, dst_mode, blend_color);
   _al_pop_target_bitmap();

   return newcf;
}



/* color_extract_font_range:
 *  (color vtable entry)
 *  Extract a range of characters from a color font 
 */
static ALLEGRO_FONT *color_extract_font_range(ALLEGRO_FONT *f, int begin, int end)
{
   ALLEGRO_FONT *fontout = NULL;
   ALLEGRO_FONT_COLOR_DATA *cf, *cfin;
   int first, last;

   if (!f)
      return NULL;

   /* Special case: copy entire font */
   if (begin==-1 && end==-1) {
   }
   /* Copy from the beginning */
   else if (begin == -1 && end > color_get_font_range_begin(f, -1)) {
   }
   /* Copy to the end */
   else if (end == -1 && begin <= color_get_font_range_end(f, -1)) {
   }
   /* begin cannot be bigger than end */
   else if (begin <= end && begin != -1 && end != -1) {
   }
   else {
      return NULL;
   }

   /* Get output font */
   fontout = _AL_MALLOC(sizeof *fontout);

   fontout->height = f->height;
   fontout->vtable = f->vtable;
   fontout->data = NULL;

   /* Get real character ranges */
   first = MAX(begin, color_get_font_range_begin(f, -1));
   last = (end>-1) ? MIN(end, color_get_font_range_end(f, -1)) : color_get_font_range_end(f, -1);
   last++;

   cf = NULL;
   cfin = f->data;
   while (cfin) {
      /* Find the range that is covered by the requested range. */
      /* Check if the requested and processed ranges at least overlap */
      if (((first >= cfin->begin && first < cfin->end) || (last <= cfin->end && last > cfin->begin))
      /* Check if the requested range wraps processed ranges */
      || (first < cfin->begin && last > cfin->end)) {
         int local_begin, local_end;
         
         local_begin = MAX(cfin->begin, first);
         local_end = MIN(cfin->end, last);

         if (cf) {
            cf->next = color_copy_glyph_range(cfin, local_begin, local_end);
            cf = cf->next;
         }
         else {
            cf = color_copy_glyph_range(cfin, local_begin, local_end);
            fontout->data = cf;
         }
      }
      cfin = cfin->next;
   }

   return fontout;
}



/* color_merge_fonts:
 *  (color vtable entry)
 *  Merges font2 with font1 and returns a new font
 */
static ALLEGRO_FONT *color_merge_fonts(ALLEGRO_FONT *font1, ALLEGRO_FONT *font2)
{
   ALLEGRO_FONT *fontout = NULL, *font2_upgr = NULL;
   ALLEGRO_FONT_COLOR_DATA *cf, *cf1, *cf2;
   
   if (!font1 || !font2)
      return NULL;
      
   font2_upgr = font2;

   /* Get output font */
   fontout = _AL_MALLOC(sizeof *fontout);
   fontout->height = MAX(font1->height, font2->height);
   fontout->vtable = font1->vtable;
   cf = fontout->data = NULL;
   
   cf1 = font1->data;
   cf2 = font2_upgr->data;
   while (cf1 || cf2) {
      if (cf1 && (!cf2 ||  (cf1->begin < cf2->begin))) {
         if (cf) {
            cf->next = color_copy_glyph_range(cf1, cf1->begin, cf1->end);
            cf = cf->next;
         }
         else {
            cf = color_copy_glyph_range(cf1, cf1->begin, cf1->end);
            fontout->data = cf;
         }
         cf1 = cf1->next;
      }
      else {
         if (cf) {
            cf->next = color_copy_glyph_range(cf2, cf2->begin, cf2->end);
            cf = cf->next;
         }
         else {
            cf = color_copy_glyph_range(cf2, cf2->begin, cf2->end);
            fontout->data = cf;
         }
         cf2 = cf2->next;
      }
   }

   if (font2_upgr != font2)
      al_font_destroy_font(font2_upgr);

   return fontout;
}



/* color_transpose_font:
 *  (color vtable entry)
 *  Transpose all glyphs in a font
 */
static int color_transpose_font(ALLEGRO_FONT* f, int drange)
{
   ALLEGRO_FONT_COLOR_DATA* cf = 0;

   if (!f) 
      return -1;

   cf = (ALLEGRO_FONT_COLOR_DATA*)(f->data);

   while(cf) {
      ALLEGRO_FONT_COLOR_DATA* next = cf->next;
      
      cf->begin += drange;
      cf->end += drange;
      cf = next;
   }

   return 0;
}

#endif

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
    /*
    color_get_font_ranges,
    color_get_font_range_begin,
    color_get_font_range_end,
    color_extract_font_range,
    color_merge_fonts,
    color_transpose_font
    */
};

ALLEGRO_FONT_VTABLE* al_font_vtable_color = &_al_font_vtable_color;


/* al_font_is_compatibe_font:
 *  returns non-zero if the two fonts are of similar type
 */
int al_font_is_compatible_font(ALLEGRO_FONT *f1, ALLEGRO_FONT *f2)
{
   ASSERT(f1);
   ASSERT(f2);
   return f1->vtable == f2->vtable;
}


#if 0
/* al_font_extract_font_range:
 *  Extracts a character range from a font f, and returns a new font containing
 *   only the extracted characters.
 * Returns NULL if the character range could not be extracted.
 */
ALLEGRO_FONT *al_font_extract_font_range(ALLEGRO_FONT *f, int begin, int end)
{
   if (f->vtable->extract_font_range)
      return f->vtable->extract_font_range(f, begin, end);
   return NULL;
}



/* al_font_merge_fonts:
 *  Merges two fonts. May convert the two fonts to compatible types before
 *   merging, in which case converting the type of f2 to f1 is tried first.
 */
ALLEGRO_FONT *al_font_merge_fonts(ALLEGRO_FONT *f1, ALLEGRO_FONT *f2)
{
   ALLEGRO_FONT *f = NULL;
   
   if (f1->vtable->merge_fonts)
      f = f1->vtable->merge_fonts(f1, f2);
   
   if (!f && f2->vtable->merge_fonts)
      f = f2->vtable->merge_fonts(f2, f1);
      
   return f;
}



/* al_font_get_font_ranges:
 *  Returns the number of character ranges in a font, or -1 if that information
 *   is not available.
 */
int al_font_get_font_ranges(ALLEGRO_FONT *f)
{
   if (f->vtable->get_font_ranges)
      return f->vtable->get_font_ranges(f);
   
   return -1;
}



/* al_font_get_font_range_begin:
 *  Returns the starting character for the font in question, or -1 if that
 *   information is not available.
 */
int al_font_get_font_range_begin(ALLEGRO_FONT *f, int range)
{
   if (f->vtable->get_font_range_begin)
      return f->vtable->get_font_range_begin(f, range);
   
   return -1;
}



/* al_font_get_font_range_end:
 *  Returns the last character for the font in question, or -1 if that
 *  information is not available.
 */
int al_font_get_font_range_end(ALLEGRO_FONT *f, int range)
{
   if (f->vtable->get_font_range_end)
      return f->vtable->get_font_range_end(f, range);

   return -1;
}




/* al_font_transpose_font:
 *  Transposes all the glyphs in a font over a range drange. Returns 0 on
 *   success, or -1 on failure.
 */
int al_font_transpose_font(ALLEGRO_FONT *f, int drange)
{
   if (f->vtable->transpose_font)
      return f->vtable->transpose_font(f, drange);
   
   return -1;
}
#endif

void al_font_init(void)
{
   al_iio_init(); /* we depend on the iio addon */
   //_register_font_file_type_init();
}

