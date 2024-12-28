#define ALLEGRO_INTERNAL_UNSTABLE

#include "allegro5/allegro.h"
#ifdef ALLEGRO_CFG_OPENGL
#include "allegro5/allegro_opengl.h"
#endif
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_vector.h"

#include "allegro5/allegro_ttf.h"
#include "allegro5/internal/aintern_font.h"
#include "allegro5/internal/aintern_ttf_cfg.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_exitfunc.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H

#include <stdlib.h>

ALLEGRO_DEBUG_CHANNEL("font")


/* Some low-end drivers enable automatic S3TC compression, which
 * requires glTexSubImage2D to only work on multiples of aligned
 * 4x4 pixel blocks with some buggy OpenGL drivers.
 * There's not much we can do about that in general - if the user
 * locks a portion of a bitmap not conformin to this it will fail
 * with such a driver.
 *
 * However in many programs this is no problem at all safe for rendering
 * glyphs and simply aligning to 4 pixels here fixes it.
 */
#define ALIGN_TO_4_PIXEL


#define RANGE_SIZE   128


typedef struct REGION
{
   short x;
   short y;
   short w;
   short h;
} REGION;


typedef struct ALLEGRO_TTF_GLYPH_DATA
{
   ALLEGRO_BITMAP *page_bitmap;
   REGION region;
   short offset_x;
   short offset_y;
   short advance;
} ALLEGRO_TTF_GLYPH_DATA;


typedef struct ALLEGRO_TTF_GLYPH_RANGE
{
   int32_t range_start;
   ALLEGRO_TTF_GLYPH_DATA *glyphs;  /* [RANGE_SIZE] */
} ALLEGRO_TTF_GLYPH_RANGE;


typedef struct ALLEGRO_TTF_FONT_DATA
{
   FT_Face face;
   int flags;
   _AL_VECTOR glyph_ranges;  /* sorted array of of ALLEGRO_TTF_GLYPH_RANGE */

   _AL_VECTOR page_bitmaps;  /* of ALLEGRO_BITMAP pointers */
   int page_pos_x;
   int page_pos_y;
   int page_line_height;
   ALLEGRO_LOCKED_REGION *page_lr;

   FT_StreamRec stream;
   ALLEGRO_FILE *file;
   unsigned long base_offset;
   unsigned long offset;

   int bitmap_format;
   int bitmap_flags;

   int min_page_size;
   int max_page_size;

   bool skip_cache_misses;
} ALLEGRO_TTF_FONT_DATA;


/* globals */
static bool ttf_inited;
static FT_Library ft;
static ALLEGRO_FONT_VTABLE vt;


static INLINE int align4(int x)
{
#ifdef ALIGN_TO_4_PIXEL
   return (x + 3) & ~3;
#else
   return x;
#endif
}


/* Returns false if the glyph is invalid.
 */
static bool get_glyph(ALLEGRO_TTF_FONT_DATA *data,
   int ft_index, ALLEGRO_TTF_GLYPH_DATA **glyph)
{
   ALLEGRO_TTF_GLYPH_RANGE *range;
   int32_t range_start;
   int lo, hi, mid;
   ASSERT(glyph);

   range_start = ft_index - (ft_index % RANGE_SIZE);

   /* Binary search for the range. */
   lo = 0;
   hi = _al_vector_size(&data->glyph_ranges);
   mid = (hi + lo)/2;
   range = NULL;

   while (lo < hi) {
      ALLEGRO_TTF_GLYPH_RANGE *r = _al_vector_ref(&data->glyph_ranges, mid);
      if (r->range_start == range_start) {
         range = r;
         break;
      }
      else if (r->range_start < range_start) {
         lo = mid + 1;
      }
      else {
         hi = mid;
      }
      mid = (hi + lo)/2;
   }

   if (!range) {
      range = _al_vector_alloc_mid(&data->glyph_ranges, mid);
      range->range_start = range_start;
      range->glyphs = al_calloc(RANGE_SIZE, sizeof(ALLEGRO_TTF_GLYPH_DATA));
   }
   
   *glyph = &range->glyphs[ft_index - range_start]; 
   
   /* If we're skipping cache misses and it isn't already cached, return it as invalid. */
   if (data->skip_cache_misses && !(*glyph)->page_bitmap && (*glyph)->region.x >= 0) {
      return false;
   }

   return ft_index != 0;
}


static void unlock_current_page(ALLEGRO_TTF_FONT_DATA *data)
{
   if (data->page_lr) {
      ALLEGRO_BITMAP **back = _al_vector_ref_back(&data->page_bitmaps);
      ASSERT(al_is_bitmap_locked(*back));
      al_unlock_bitmap(*back);
      data->page_lr = NULL;
      ALLEGRO_DEBUG("Unlocking page: %p\n", *back);
   }
}


static ALLEGRO_BITMAP *push_new_page(ALLEGRO_TTF_FONT_DATA *data, int glyph_size)
{
    ALLEGRO_BITMAP **back;
    ALLEGRO_BITMAP *page;
    ALLEGRO_STATE state;
    int page_size = 1;
    /* 16 seems to work well. A particular problem are fixed width fonts which
     * take an inordinate amount of space. */
    while (page_size < 16 * glyph_size) {
      page_size *= 2;
    }
    if (page_size < data->min_page_size) {
      page_size = data->min_page_size;
    }
    if (page_size > data->max_page_size) {
      page_size = data->max_page_size;
    }
    if (glyph_size > page_size) {
      ALLEGRO_ERROR("Unable create new page, glyph too large: %d > %d\n",
         glyph_size, page_size);
      return NULL;
    }

    unlock_current_page(data);

    /* The bitmap will be destroyed when the parent font is destroyed so
     * it is not safe to register a destructor for it.
     */
    _al_push_destructor_owner();
    al_store_state(&state, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
    al_set_new_bitmap_format(data->bitmap_format);
    al_set_new_bitmap_flags(data->bitmap_flags);
    page = al_create_bitmap(page_size, page_size);
    al_restore_state(&state);
    _al_pop_destructor_owner();

    if (page) {
       back = _al_vector_alloc_back(&data->page_bitmaps);
       *back = page;

       data->page_pos_x = 0;
       data->page_pos_y = 0;
       data->page_line_height = 0;
    }

    return page;
}


static unsigned char *alloc_glyph_region(ALLEGRO_TTF_FONT_DATA *data,
   int ft_index, int w, int h, bool new, ALLEGRO_TTF_GLYPH_DATA *glyph,
   bool lock_whole_page)
{
   ALLEGRO_BITMAP *page;
   int w4 = align4(w);
   int h4 = align4(h);
   int glyph_size = w4 > h4 ? w4 : h4;
   bool lock = false;

   if (_al_vector_is_empty(&data->page_bitmaps) || new) {
      page = push_new_page(data, glyph_size);
      if (!page) {
         ALLEGRO_ERROR("Failed to create a new page for glyph %d.\n", ft_index);
         return NULL;
      }
   }
   else {
      ALLEGRO_BITMAP **back = _al_vector_ref_back(&data->page_bitmaps);
      page = *back;
   }

   ALLEGRO_DEBUG("Glyph %d: %dx%d (%dx%d)%s\n",
      ft_index, w, h, w4, h4, new ? " new" : "");

   if (data->page_pos_x + w4 > al_get_bitmap_width(page)) {
      data->page_pos_y += data->page_line_height;
      data->page_pos_y = align4(data->page_pos_y);
      data->page_pos_x = 0;
      data->page_line_height = 0;
   }

   if (data->page_pos_y + h4 > al_get_bitmap_height(page)) {
      return alloc_glyph_region(data, ft_index, w, h, true, glyph, lock_whole_page);
   }

   glyph->page_bitmap = page;
   glyph->region.x = data->page_pos_x;
   glyph->region.y = data->page_pos_y;
   glyph->region.w = w;
   glyph->region.h = h;

   data->page_pos_x = align4(data->page_pos_x + w4);
   if (h > data->page_line_height) {
      data->page_line_height = h4;
   }

   REGION lock_rect;
   if (lock_whole_page) {
      lock_rect.x = 0;
      lock_rect.y = 0;
      lock_rect.w = al_get_bitmap_width(page);
      lock_rect.h = al_get_bitmap_height(page);
      if (!data->page_lr) {
         lock = true;
         ALLEGRO_DEBUG("Locking whole page: %p\n", page);
      }
   }
   else {
      /* Unlock just in case... */
      unlock_current_page(data);
      lock_rect.x = glyph->region.x;
      lock_rect.y = glyph->region.y;
      lock_rect.w = w4;
      lock_rect.h = h4;
      lock = true;
      ALLEGRO_DEBUG("Locking glyph region: %p %d %d %d %d\n", page,
         lock_rect.x, lock_rect.y, lock_rect.w, lock_rect.h);
   }

   if (lock) {
      unsigned char *ptr;
      int i;

      data->page_lr = al_lock_bitmap_region(page,
         lock_rect.x, lock_rect.y, lock_rect.w, lock_rect.h,
         ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);

      if (!data->page_lr) {
         ALLEGRO_ERROR("Failed to lock page.\n");
         return NULL;
      }

      if (data->flags & ALLEGRO_NO_PREMULTIPLIED_ALPHA) {
         /* Fill in border with "transparent white"
          * to make anti-aliasing work well when rotation
          * and/or scaling transforms are in effect
          * FIXME We could clear just the border
          */ 
         for (i = 0; i < lock_rect.h; i++) {
            ptr = (unsigned char *)(data->page_lr->data) + (i * data->page_lr->pitch);
            int j;
            for (j = 0; j < lock_rect.w; ++j) {
               *ptr++ = 255;
               *ptr++ = 255;
               *ptr++ = 255;
               *ptr++ = 0;
            }
         }
      }
      else {
         /* Clear the data so we don't get garbage when using filtering
          * FIXME We could clear just the border but I'm not convinced that
          * would be faster (yet)
          */
         for (i = 0; i < lock_rect.h; i++) {
            ptr = (unsigned char *)(data->page_lr->data) + (i * data->page_lr->pitch);
            memset(ptr, 0, lock_rect.w * 4);
         }
      }
   }

   ASSERT(data->page_lr);

   /* Copy a displaced pointer for the glyph. */
   return (unsigned char *)data->page_lr->data
      + ((glyph->region.y + 2) - lock_rect.y) * data->page_lr->pitch
      + ((glyph->region.x + 2) - lock_rect.x) * sizeof(int32_t);
}


static void copy_glyph_mono(ALLEGRO_TTF_FONT_DATA *font_data, FT_Face face,
   unsigned char *glyph_data)
{
   int pitch = font_data->page_lr->pitch;
   int x, y;

   for (y = 0; y < (int)face->glyph->bitmap.rows; y++) {
      unsigned char const *ptr = face->glyph->bitmap.buffer + face->glyph->bitmap.pitch * y;
      unsigned char *dptr = glyph_data + pitch * y;
      int bit = 0;

      if (face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) {
         for (x = 0; x < (int)face->glyph->bitmap.width; x++) {
            unsigned char set = ((ptr[3] >> (7-bit)) & 1) ? 255 : 0;
            *dptr++ = set;
            *dptr++ = set;
            *dptr++ = set;
            *dptr++ = set;
            ptr += 4;
         }
      }
      else if (font_data->flags & ALLEGRO_NO_PREMULTIPLIED_ALPHA) {
         /* FIXME We could just set the alpha byte since the
          * region was cleared above when allocated
          */
         for (x = 0; x < (int)face->glyph->bitmap.width; x++) {
            unsigned char set = ((*ptr >> (7-bit)) & 1) ? 255 : 0;
            *dptr++ = 255;
            *dptr++ = 255;
            *dptr++ = 255;
            *dptr++ = set;
            bit = (bit + 1) & 7;
            if (bit == 0) {
               ptr++;
            }
         }
      }
      else {
         for (x = 0; x < (int)face->glyph->bitmap.width; x++) {
            unsigned char set = ((*ptr >> (7-bit)) & 1) ? 255 : 0;
            *dptr++ = set;
            *dptr++ = set;
            *dptr++ = set;
            *dptr++ = set;
            bit = (bit + 1) & 7;
            if (bit == 0) {
               ptr++;
            }
         }
      }
   }
}


static void copy_glyph_color(ALLEGRO_TTF_FONT_DATA *font_data, FT_Face face,
   unsigned char *glyph_data)
{
   int pitch = font_data->page_lr->pitch;
   int x, y;

   for (y = 0; y < (int)face->glyph->bitmap.rows; y++) {
      unsigned char const *ptr = face->glyph->bitmap.buffer + face->glyph->bitmap.pitch * y;
      unsigned char *dptr = glyph_data + pitch * y;

      if (face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) {
         for (x = 0; x < (int)face->glyph->bitmap.width; x++) {
            *dptr++ = ptr[2];
            *dptr++ = ptr[1];
            *dptr++ = ptr[0];
            *dptr++ = ptr[3];
            ptr += 4;
         }
      }
      else if (font_data->flags & ALLEGRO_NO_PREMULTIPLIED_ALPHA) {
         /* FIXME We could just set the alpha byte since the
          * region was cleared above when allocated
          */
         for (x = 0; x < (int)face->glyph->bitmap.width; x++) {
            unsigned char c = *ptr;
            *dptr++ = 255;
            *dptr++ = 255;
            *dptr++ = 255;
            *dptr++ = c;
            ptr++;
         }
      }
      else {
         for (x = 0; x < (int)face->glyph->bitmap.width; x++) {
            unsigned char c = *ptr;
            *dptr++ = c;
            *dptr++ = c;
            *dptr++ = c;
            *dptr++ = c;
            ptr++;
         }
      }
   }
}


/* NOTE: this function may disable the bitmap hold drawing state
 * and leave the current page bitmap locked.
 * 
 * NOTE: We have previously tried to be more clever about caching multiple
 * glyphs during incidental cache misses, but found that approach to be slower.
 */
static void cache_glyph(ALLEGRO_TTF_FONT_DATA *font_data, FT_Face face,
   int ft_index, ALLEGRO_TTF_GLYPH_DATA *glyph, bool lock_whole_page)
{
    FT_Int32 ft_load_flags;
    FT_Error e;
    int w, h;
    unsigned char *glyph_data;

    if (glyph->page_bitmap || glyph->region.x < 0)
        return;
   
    /* We shouldn't ever get here, as cache misses
     * should have been set to ft_index = 0. */
    ASSERT(!(font_data->skip_cache_misses && !lock_whole_page));

    // FIXME: make this a config setting? FT_LOAD_FORCE_AUTOHINT

    ft_load_flags = FT_LOAD_RENDER;
    if (face->num_fixed_sizes) {
       // This is a bitmap font with fixed sizes, let's load them in
       // color if available.
       ft_load_flags |= FT_LOAD_COLOR;
    }
    else {
       // FIXME: Investigate why some fonts don't work without the
       // NO_BITMAP flags. Supposedly using that flag makes small sizes
       // look bad so ideally we would not used it.
       ft_load_flags |= FT_LOAD_NO_BITMAP;
    }

    if (font_data->flags & ALLEGRO_TTF_MONOCHROME)
       ft_load_flags |= FT_LOAD_TARGET_MONO;
    if (font_data->flags & ALLEGRO_TTF_NO_AUTOHINT)
       ft_load_flags |= FT_LOAD_NO_AUTOHINT;

    e = FT_Load_Glyph(face, ft_index, ft_load_flags);
    if (e) {
       ALLEGRO_WARN("Failed loading glyph %d from.\n", ft_index);
    }

    glyph->offset_x = face->glyph->bitmap_left;
    glyph->offset_y = (face->size->metrics.ascender >> 6) - face->glyph->bitmap_top;
    glyph->advance = face->glyph->advance.x >> 6;

    w = face->glyph->bitmap.width;
    h = face->glyph->bitmap.rows;

    if (w == 0 || h == 0) {
       /* Mark this glyph so we won't try to cache it next time. */
       glyph->region.x = -1;
       glyph->region.y = -1;
       glyph->page_bitmap = NULL;
       /* Even though this glyph has no "region", include the 2-pixel border in the size */
       glyph->region.w = w + 4;
       glyph->region.h = h + 4;
       ALLEGRO_DEBUG("Glyph %d has zero size. (pixel mode %d)\n", ft_index, face->glyph->bitmap.pixel_mode);
       return;
    }

    /* Each glyph has a 2-pixel border all around. Note: The border is kept
     * even against the outer bitmap edge, to ensure consistent rendering.
     */
    glyph_data = alloc_glyph_region(font_data, ft_index,
       w + 4, h + 4, false, glyph, lock_whole_page);

    if (glyph_data == NULL) {
       return;
    }

    if (font_data->flags & ALLEGRO_TTF_MONOCHROME)
       copy_glyph_mono(font_data, face, glyph_data);
    else
       copy_glyph_color(font_data, face, glyph_data);

    if (!lock_whole_page) {
       unlock_current_page(font_data);
    }
}

/* WARNING: It is only valid to call this function when the current page is empty
 * (or already locked), otherwise it will gibberify the current glyphs on that page.
 * 
 * This leaves the current page unlocked.
 */
static void cache_glyphs(ALLEGRO_TTF_FONT_DATA *data, const char *text, size_t text_size)
{
   ALLEGRO_USTR_INFO info;
   const ALLEGRO_USTR* ustr = al_ref_buffer(&info, text, text_size);
   FT_Face face = data->face;
   int pos = 0;
   int32_t ch;  

   while ((ch = al_ustr_get_next(ustr, &pos)) >= 0) {
      ALLEGRO_TTF_GLYPH_DATA *glyph;
      int ft_index = FT_Get_Char_Index(face, ch);
      get_glyph(data, ft_index, &glyph);
      cache_glyph(data, face, ft_index, glyph, true);
   }
}


static int get_kerning(ALLEGRO_TTF_FONT_DATA const *data, FT_Face face,
   int prev_ft_index, int ft_index)
{
   /* Do kerning? */
   if (!(data->flags & ALLEGRO_TTF_NO_KERNING) && prev_ft_index != -1) {
      FT_Vector delta;
      FT_Get_Kerning(face, prev_ft_index, ft_index,
         FT_KERNING_DEFAULT, &delta);
      return delta.x >> 6;
   }

   return 0;
}


static bool ttf_get_glyph_worker(ALLEGRO_FONT const *f, int prev_ft_index, int ft_index, int prev_codepoint, int codepoint, ALLEGRO_GLYPH *info)
{
   ALLEGRO_TTF_FONT_DATA *data = f->data;
   FT_Face face = data->face;
   ALLEGRO_TTF_GLYPH_DATA *glyph;
   int advance = 0;

   if (!get_glyph(data, ft_index, &glyph)) {
      if (f->fallback)
         return f->fallback->vtable->get_glyph(f->fallback, prev_codepoint, codepoint, info);
      else {
         get_glyph(data, 0, &glyph);
         ft_index = 0;
      }
   }

   cache_glyph(data, face, ft_index, glyph, false);

   advance += get_kerning(data, face, prev_ft_index, ft_index);

   info->bitmap = glyph->page_bitmap;
   /* the adjustments below remove the 2-pixel border from the glyph */
   if (info->bitmap) {
      info->x = glyph->region.x + 2;
      info->y = glyph->region.y + 2;
   }
   else {
      info->x = -1;
      info->y = -1;
   }
   info->w = glyph->region.w - 4;
   info->h = glyph->region.h - 4;
   info->kerning = advance;
   info->offset_x = glyph->offset_x;
   info->offset_y = glyph->offset_y;
   info->advance = glyph->advance + advance;

   return ft_index != 0;
}


static bool ttf_get_glyph(ALLEGRO_FONT const *f, int prev_codepoint, int codepoint, ALLEGRO_GLYPH *glyph)
{
   ALLEGRO_TTF_FONT_DATA *data = f->data;
   FT_Face face = data->face;
   int prev_ft_index = (prev_codepoint == -1) ? -1 : (int)FT_Get_Char_Index(face, prev_codepoint);
   int ft_index = FT_Get_Char_Index(face, codepoint);
   return ttf_get_glyph_worker(f, prev_ft_index, ft_index, prev_codepoint, codepoint, glyph);
}


static int render_glyph(ALLEGRO_FONT const *f, ALLEGRO_COLOR color,
   int prev_ft_index, int ft_index, int32_t prev_ch, int32_t ch, float xpos, float ypos)
{
   ALLEGRO_GLYPH glyph;

   if (ttf_get_glyph_worker(f, prev_ft_index, ft_index, prev_ch, ch, &glyph) == false)
      return 0;

   if (glyph.bitmap != NULL) {
      /*
       * Include 1 pixel of the 2-pixel border when drawing glyph.
       * This improves render results when rotating and scaling.
       */
      al_draw_tinted_bitmap_region(
         glyph.bitmap, color,
         glyph.x - 1, glyph.y - 1, glyph.w + 2, glyph.h + 2,
         xpos + glyph.offset_x + glyph.kerning - 1,
         ypos + glyph.offset_y - 1,
         0
      );
   }

   return glyph.advance;
}


static int ttf_font_height(ALLEGRO_FONT const *f)
{
   ASSERT(f);
   return f->height;
}


static int ttf_font_ascent(ALLEGRO_FONT const *f)
{
    ALLEGRO_TTF_FONT_DATA *data;
    FT_Face face;

    ASSERT(f);

    data = f->data;
    face = data->face;

    return face->size->metrics.ascender >> 6;
}


static int ttf_font_descent(ALLEGRO_FONT const *f)
{
    ALLEGRO_TTF_FONT_DATA *data;
    FT_Face face;

    ASSERT(f);

    data = f->data;
    face = data->face;

    return (-face->size->metrics.descender) >> 6;
}


static int ttf_render_char(ALLEGRO_FONT const *f, ALLEGRO_COLOR color,
   int ch, float xpos, float ypos)
{
   ALLEGRO_TTF_FONT_DATA *data = f->data;
   FT_Face face = data->face;
   int advance = 0;
   int32_t ch32 = (int32_t) ch;

   int ft_index = FT_Get_Char_Index(face, ch32);
   advance = render_glyph(f, color, -1, ft_index, -1, ch, xpos, ypos);

   return advance;
}


static int ttf_char_length(ALLEGRO_FONT const *f, int ch)
{
   int result;
   ALLEGRO_TTF_FONT_DATA *data = f->data;
   ALLEGRO_TTF_GLYPH_DATA *glyph;
   FT_Face face = data->face;
   int ft_index = FT_Get_Char_Index(face, ch);
   if (!get_glyph(data, ft_index, &glyph)) {
      if (f->fallback) {
         return al_get_glyph_width(f->fallback, ch);
      }
      else {
         get_glyph(data, 0, &glyph);
         ft_index = 0;
      }
   }
   cache_glyph(data, face, ft_index, glyph, false);
   result = glyph->region.w - 4;    /* Remove 2-pixel border from width */

   return result;
}


static int ttf_render(ALLEGRO_FONT const *f, ALLEGRO_COLOR color,
   const ALLEGRO_USTR *text, float x, float y)
{
   ALLEGRO_TTF_FONT_DATA *data = f->data;
   FT_Face face = data->face;
   int pos = 0;
   int advance = 0;
   int prev_ft_index = -1;
   int32_t prev_ch = -1;
   int32_t ch;
   bool hold;

   hold = al_is_bitmap_drawing_held();
   al_hold_bitmap_drawing(true);

   while ((ch = al_ustr_get_next(text, &pos)) >= 0) {
      int ft_index = FT_Get_Char_Index(face, ch);
      advance += render_glyph(f, color, prev_ft_index, ft_index, prev_ch, ch,
         x + advance, y);
      prev_ft_index = ft_index;
      prev_ch = ch;
   }

   al_hold_bitmap_drawing(hold);

   return advance;
}


static int ttf_text_length(ALLEGRO_FONT const *f, const ALLEGRO_USTR *text)
{
   int pos = 0;
   int x = 0;
   int32_t ch, nch;

   nch = al_ustr_get_next(text, &pos);
   while (nch >= 0) {
      ch = nch;
      nch = al_ustr_get_next(text, &pos);

      x += al_get_glyph_advance(f, ch, nch < 0 ?
         ALLEGRO_NO_KERNING : nch);
   }

   return x;
}


static void ttf_get_text_dimensions(ALLEGRO_FONT const *f,
   ALLEGRO_USTR const *text,
   int *bbx, int *bby, int *bbw, int *bbh)
{
   int pos = 0;
   bool first = true;
   int x = 0;
   int32_t ch, nch;
   int ymin = f->height;
   int ymax = 0;
   *bbx = 0;

   nch = al_ustr_get_next(text, &pos);
   while (nch >= 0) {
      int gx, gy, gw, gh;
      ch = nch;
      nch = al_ustr_get_next(text, &pos);
      if (!al_get_glyph_dimensions(f, ch, &gx, &gy, &gw, &gh)) {
         continue;
      }

      if (nch < 0) {
         x += gx + gw;
      }
      else {
         x += al_get_glyph_advance(f, ch, nch);
      }

      if (gy < ymin) {
         ymin = gy;
      }

      if (gh+gy > ymax) {
         ymax = gh + gy;
      }

      if (first) {
         *bbx = gx;
         first = false;
      }
   }

   *bby = ymin;
   *bbw = x - *bbx;
   *bbh = ymax - ymin;
}


#ifdef DEBUG_CACHE
#include "allegro5/allegro_image.h"
static void debug_cache(ALLEGRO_FONT *f)
{
   ALLEGRO_TTF_FONT_DATA *data = f->data;
   _AL_VECTOR *v = &data->page_bitmaps;
   static int j = 0;
   int i;

   al_init_image_addon();

   for (i = 0; i < (int)_al_vector_size(v); i++) {
      ALLEGRO_BITMAP **bmp = _al_vector_ref(v, i);
      ALLEGRO_USTR *u = al_ustr_newf("font%d_%d.png", j, i);
      al_save_bitmap(al_cstr(u), *bmp);
      al_ustr_free(u);
   }
   j++;
}
#endif


static void ttf_destroy(ALLEGRO_FONT *f)
{
   ALLEGRO_TTF_FONT_DATA *data = f->data;
   int i;

   unlock_current_page(data);

#ifdef DEBUG_CACHE
   debug_cache(f);
#endif

   FT_Done_Face(data->face);
   for (i = _al_vector_size(&data->glyph_ranges) - 1; i >= 0; i--) {
      ALLEGRO_TTF_GLYPH_RANGE *range = _al_vector_ref(&data->glyph_ranges, i);
      al_free(range->glyphs);
   }
   _al_vector_free(&data->glyph_ranges);
   for (i = _al_vector_size(&data->page_bitmaps) - 1; i >= 0; i--) {
      ALLEGRO_BITMAP **bmp = _al_vector_ref(&data->page_bitmaps, i);
      al_destroy_bitmap(*bmp);
   }
   _al_vector_free(&data->page_bitmaps);
   al_free(data);
   al_free(f);
}


static unsigned long ftread(FT_Stream stream, unsigned long offset,
    unsigned char *buffer, unsigned long count)
{
    ALLEGRO_TTF_FONT_DATA *data = stream->pathname.pointer;
    unsigned long bytes;

    if (count == 0)
       return 0;

    if (offset != data->offset)
       al_fseek(data->file, data->base_offset + offset, ALLEGRO_SEEK_SET);
    bytes = al_fread(data->file, buffer, count);
    data->offset = offset + bytes;
    return bytes;
}


static void ftclose(FT_Stream  stream)
{
    ALLEGRO_TTF_FONT_DATA *data = stream->pathname.pointer;
    al_fclose(data->file);
    data->file = NULL;
}

/* Function: al_load_ttf_font_f
 */
ALLEGRO_FONT *al_load_ttf_font_f(ALLEGRO_FILE *file,
    char const *filename, int size, int flags)
{
    return al_load_ttf_font_stretch_f(file, filename, 0, size, flags);
}


/* Function: al_load_ttf_font_stretch_f
 */
ALLEGRO_FONT *al_load_ttf_font_stretch_f(ALLEGRO_FILE *file,
    char const *filename, int w, int h, int flags)
{
    FT_Face face;
    ALLEGRO_TTF_FONT_DATA *data;
    ALLEGRO_FONT *f;
    ALLEGRO_PATH *path;
    FT_Open_Args args;
    int result;
    ALLEGRO_CONFIG* system_cfg = al_get_system_config();
    const char* min_page_size_str =
      al_get_config_value(system_cfg, "ttf", "min_page_size");
    const char* max_page_size_str =
      al_get_config_value(system_cfg, "ttf", "max_page_size");
    const char* cache_str =
      al_get_config_value(system_cfg, "ttf", "cache_text");
    const char* skip_cache_misses_str =
      al_get_config_value(system_cfg, "ttf", "skip_cache_misses");

    if ((h > 0 && w < 0) || (h < 0 && w > 0)) {
       ALLEGRO_ERROR("Height/width have opposite signs (w = %d, h = %d).\n", w, h);
       return NULL;
    }

    data = al_calloc(1, sizeof *data);
    data->stream.read = ftread;
    data->stream.close = ftclose;
    data->stream.pathname.pointer = data;
    data->base_offset = al_ftell(file);
    data->stream.size = al_fsize(file);
    data->file = file;
    data->bitmap_format = al_get_new_bitmap_format();
    data->bitmap_flags = al_get_new_bitmap_flags();
    data->min_page_size = 256;
    data->max_page_size = 8192;

    if (min_page_size_str) {
      int min_page_size = atoi(min_page_size_str);
      if (min_page_size > 0) {
         data->min_page_size = min_page_size;
      }
    }

    if (max_page_size_str) {
      int max_page_size = atoi(max_page_size_str);
      if (max_page_size > 0 && max_page_size >= data->min_page_size) {
         data->max_page_size = max_page_size;
      }
    }

    if (skip_cache_misses_str && !strcmp(skip_cache_misses_str, "true")) {
       data->skip_cache_misses = true;
    }

    memset(&args, 0, sizeof args);
    args.flags = FT_OPEN_STREAM;
    args.stream = &data->stream;

    if ((result = FT_Open_Face(ft, &args, 0, &face)) != 0) {
        ALLEGRO_ERROR("Reading %s failed. Freetype error code %d\n", filename,
          result);
        // Note: Freetype already closed the file for us.
        al_free(data);
        return NULL;
    }

    // FIXME: The below doesn't use Allegro's streaming.
    /* Small hack for Type1 fonts which store kerning information in
     * a separate file - and we try to guess the name of that file.
     */
    path = al_create_path(filename);
    if (!strcmp(al_get_path_extension(path), ".pfa")) {
        const char *helper;
        ALLEGRO_DEBUG("Type1 font assumed for %s.\n", filename);

        al_set_path_extension(path, ".afm");
        helper = al_path_cstr(path, '/');
        FT_Attach_File(face, helper);
        ALLEGRO_DEBUG("Guessed afm file %s.\n", helper);

        al_set_path_extension(path, ".tfm");
        helper = al_path_cstr(path, '/');
        FT_Attach_File(face, helper);
        ALLEGRO_DEBUG("Guessed tfm file %s.\n", helper);
    }
    al_destroy_path(path);

    if (face->num_fixed_sizes) {
        // TODO: we always pick the first size 
        FT_Select_Size(face, 0);
    } else if (h > 0) {
       FT_Set_Pixel_Sizes(face, w, h);
    }
    else {
       /* Set the "real dimension" of the font to be the passed size,
        * in pixels.
        */
       FT_Size_RequestRec req;
       ASSERT(w <= 0);
       ASSERT(h <= 0);
       req.type = FT_SIZE_REQUEST_TYPE_REAL_DIM;
       req.width = (-w) << 6;
       req.height = (-h) << 6;
       req.horiResolution = 0;
       req.vertResolution = 0;
       FT_Request_Size(face, &req);
    }

    ALLEGRO_DEBUG("Font %s loaded with pixel size %d x %d.\n", filename,
        w, h);
    ALLEGRO_DEBUG("    ascent=%.1f, descent=%.1f, height=%.1f\n",
        face->size->metrics.ascender / 64.0,
        face->size->metrics.descender / 64.0,
        face->size->metrics.height / 64.0);

    data->face = face;
    data->flags = flags;

    _al_vector_init(&data->glyph_ranges, sizeof(ALLEGRO_TTF_GLYPH_RANGE));
    _al_vector_init(&data->page_bitmaps, sizeof(ALLEGRO_BITMAP*));

    if (data->skip_cache_misses) {
       cache_glyphs(data, "\0", 1);
    }
    if (cache_str) {
       cache_glyphs(data, cache_str, strlen(cache_str));
    }
    unlock_current_page(data);

    f = al_calloc(sizeof *f, 1);
    f->height = face->size->metrics.height >> 6;
    f->vtable = &vt;
    f->data = data;

    f->dtor_item = _al_register_destructor(_al_dtor_list, "ttf_font", f,
       (void (*)(void *))al_destroy_font);

    return f;
}


/* Function: al_load_ttf_font
 */
ALLEGRO_FONT *al_load_ttf_font(char const *filename, int size, int flags)
{
   return al_load_ttf_font_stretch(filename, 0, size, flags);
}


/* Function: al_load_ttf_font_stretch
 */
ALLEGRO_FONT *al_load_ttf_font_stretch(char const *filename, int w, int h,
   int flags)
{
   ALLEGRO_FILE *f;
   ALLEGRO_FONT *font;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_ERROR("Unable to open file for reading: %s\n", filename);
      return NULL;
   }

   /* The file handle is owned by the function and the file is usually only
    * closed when the font is destroyed, in case Freetype has to load data
    * at a later time.
    */
   font = al_load_ttf_font_stretch_f(f, filename, w, h, flags);

   return font;
}


static int ttf_get_font_ranges(ALLEGRO_FONT *font, int ranges_count,
   int *ranges)
{
   ALLEGRO_TTF_FONT_DATA *data = font->data;
   FT_UInt g;
   FT_ULong unicode = FT_Get_First_Char(data->face, &g);
   int i = 0;
   if (i < ranges_count) {
      ranges[i * 2 + 0] = unicode;
      ranges[i * 2 + 1] = unicode;
   }
   while (g) {
      FT_ULong unicode2 = FT_Get_Next_Char(data->face, unicode, &g);
      if (unicode + 1 != unicode2) {
         if (i < ranges_count) {
            ranges[i * 2 + 1] = unicode;
            if (i + 1 < ranges_count) {
               ranges[(i + 1) * 2 + 0] = unicode2;
            }
         }
         i++;
      }
      if (i < ranges_count) {
         ranges[i * 2 + 1] = unicode2;
      }
      unicode = unicode2;
   }
   return i;
}

static bool ttf_get_glyph_dimensions(ALLEGRO_FONT const *f,
   int codepoint,
   int *bbx, int *bby, int *bbw, int *bbh)
{
   ALLEGRO_TTF_FONT_DATA *data = f->data;
   ALLEGRO_TTF_GLYPH_DATA *glyph;
   FT_Face face = data->face;
   int ft_index = FT_Get_Char_Index(face, codepoint);
   if (!get_glyph(data, ft_index, &glyph)) {
      if (f->fallback) {
         return al_get_glyph_dimensions(f->fallback, codepoint,
            bbx, bby, bbw, bbh);
      }
      else {
         get_glyph(data, 0, &glyph);
         ft_index = 0;
      }
   }
   cache_glyph(data, face, ft_index, glyph, false);
   *bbx = glyph->offset_x;
   *bbw = glyph->region.w - 4;
   *bbh = glyph->region.h - 4;
   *bby = glyph->offset_y;

   return true;
}

static int ttf_get_glyph_advance(ALLEGRO_FONT const *f, int codepoint1,
   int codepoint2)
{
   ALLEGRO_TTF_FONT_DATA *data = f->data;
   FT_Face face = data->face;
   int ft_index = FT_Get_Char_Index(face, codepoint1);
   ALLEGRO_TTF_GLYPH_DATA *glyph;
   int kerning = 0;
   int advance = 0;

   if (codepoint1 == ALLEGRO_NO_KERNING) {
      return 0;
   }

   if (!get_glyph(data, ft_index, &glyph)) {
      if (f->fallback) {
         return al_get_glyph_advance(f->fallback, codepoint1, codepoint2);
      }
      else {
         get_glyph(data, 0, &glyph);
         ft_index = 0;
      }
   }
   cache_glyph(data, face, ft_index, glyph, false);

   if (codepoint2 != ALLEGRO_NO_KERNING) {
      int ft_index1 = FT_Get_Char_Index(face, codepoint1);
      int ft_index2 = FT_Get_Char_Index(face, codepoint2);
      kerning = get_kerning(data, face, ft_index1, ft_index2);
   }

   advance = glyph->advance;
   return advance + kerning;
}



/* Function: al_init_ttf_addon
 */
bool al_init_ttf_addon(void)
{
   if (ttf_inited) {
      ALLEGRO_WARN("TTF addon already initialised.\n");
      return true;
   }

   FT_Init_FreeType(&ft);
   vt.font_height = ttf_font_height;
   vt.font_ascent = ttf_font_ascent;
   vt.font_descent = ttf_font_descent;
   vt.char_length = ttf_char_length;
   vt.text_length = ttf_text_length;
   vt.render_char = ttf_render_char;
   vt.render = ttf_render;
   vt.destroy = ttf_destroy;
   vt.get_text_dimensions = ttf_get_text_dimensions;
   vt.get_font_ranges = ttf_get_font_ranges;
   vt.get_glyph_dimensions = ttf_get_glyph_dimensions;
   vt.get_glyph_advance = ttf_get_glyph_advance;
   vt.get_glyph = ttf_get_glyph;

   al_register_font_loader(".ttf", al_load_ttf_font);

   _al_add_exit_func(al_shutdown_ttf_addon, "al_shutdown_ttf_addon");

   /* Can't fail right now - in the future we might dynamically load
    * the FreeType DLL here and/or initialize FreeType (which both
    * could fail and would cause a false return).
    */
   ttf_inited = true;
   return ttf_inited;
}


/* Function: al_is_ttf_addon_initialized
 */
bool al_is_ttf_addon_initialized(void)
{
   return ttf_inited;
}


/* Function: al_shutdown_ttf_addon
 */
void al_shutdown_ttf_addon(void)
{
   if (!ttf_inited) {
      ALLEGRO_ERROR("TTF addon not initialised.\n");
      return;
   }

   al_register_font_loader(".ttf", NULL);

   FT_Done_FreeType(ft);

   ttf_inited = false;
}


/* Function: al_get_allegro_ttf_version
 */
uint32_t al_get_allegro_ttf_version(void)
{
   return ALLEGRO_VERSION_INT;
}

/* vim: set sts=3 sw=3 et: */
