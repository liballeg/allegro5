#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_primitives.h"
#include <stdio.h>

#include "allegro5/internal/aintern_font.h"

#include "font.h"
#include "xml.h"

ALLEGRO_DEBUG_CHANNEL("font")

typedef struct {
   int first;
   int second;
   int amount;
} BMFONT_KERNING;

typedef struct {
   int page;
   int x, y;
   int width, height;
   int xoffset, yoffset;
   int xadvance;
   int chnl;
   int kerning_pairs;
   BMFONT_KERNING *kerning;
} BMFONT_CHAR;

typedef struct BMFONT_RANGE BMFONT_RANGE;

struct BMFONT_RANGE {
   int first;
   int count;
   BMFONT_CHAR **characters;
   BMFONT_RANGE *next;
};

typedef struct {
   ALLEGRO_USTR *tag;
   ALLEGRO_USTR *attribute;
   int pages_count;
   ALLEGRO_BITMAP **pages;
   BMFONT_RANGE *range_first;
   ALLEGRO_PATH *path;
   int base;
   int line_height;
   int flags;

   int kerning_pairs;
   BMFONT_KERNING *kerning;

   BMFONT_CHAR *c;
} BMFONT_DATA;

static void reallocate(BMFONT_RANGE *range) {
   range->characters = al_realloc(range->characters,
      range->count * sizeof *range->characters);
}

static void prepend_char(BMFONT_DATA *data, BMFONT_RANGE *range) {
   range->first--;
   range->count++;
   reallocate(range);
   memmove(range->characters + 1, range->characters,
      (range->count - 1) * sizeof *range->characters);
   range->characters[0] = data->c;
}

static void append_char(BMFONT_DATA *data, BMFONT_RANGE *range) {
   range->count++;
   reallocate(range);
   range->characters[range->count - 1] = data->c;
}

static void combine_ranges(BMFONT_DATA *data,
      BMFONT_RANGE *range, BMFONT_RANGE *range2) {
   (void)data;
   range->count += range2->count;
   reallocate(range);
   memmove(range->characters + range->count - range2->count,
      range2->characters,
      range2->count * sizeof *range->characters);
   range->next = range2->next;
   al_free(range2->characters);
   al_free(range2);
}

static void insert_new_range(BMFONT_DATA *data, BMFONT_RANGE *prev,
      int codepoint) {
   BMFONT_RANGE *range = al_calloc(1, sizeof *range);
   range->first = codepoint;
   range->count = 1;
   reallocate(range);
   range->characters[0] = data->c;
   if (prev) {
      range->next = prev->next;
      prev->next = range;
   }
   else {
      range->next = data->range_first;
      data->range_first = range;
   }
}

/* Adding a new codepoint creates either a new range consisting of that
 * single codepoint, appends the codepoint to the end of a range,
 * prepends it to the beginning of a range, or merges two ranges.
 */
static void add_codepoint(BMFONT_DATA *data, int codepoint) {
   BMFONT_RANGE *prev = NULL;
   BMFONT_RANGE *range = data->range_first;
   while (range) {
      if (codepoint == range->first - 1) {
         prepend_char(data, range);
         return;
      }
      if (codepoint < range->first) {
         insert_new_range(data, prev, codepoint);
         return;
      }
      if (codepoint == range->first + range->count) {
         append_char(data, range);
         BMFONT_RANGE *range2 = range->next;
         if (range2 != NULL && codepoint == range2->first - 1) {
            combine_ranges(data, range, range2);
         }
         return;
      }
      prev = range;
      range = range->next;
   }
   insert_new_range(data, prev, codepoint);
}

static BMFONT_CHAR *find_codepoint(BMFONT_DATA *data, int codepoint) {
   BMFONT_RANGE *range = data->range_first;
   while (range) {
      if (codepoint >= range->first &&
            codepoint < range->first + range->count) {
         return range->characters[codepoint - range->first];
      }
      range = range->next;
   }
   return NULL;
}

static void add_page(BMFONT_DATA *data, char const *filename) {
   data->pages_count++;
   data->pages = al_realloc(data->pages, data->pages_count *
      sizeof *data->pages);
   al_set_path_filename(data->path, filename);
   ALLEGRO_BITMAP *page = al_load_bitmap_flags(
      al_path_cstr(data->path, '/'), data->flags);
   data->pages[data->pages_count - 1] = page;
}

static bool tag_is(BMFONT_DATA *data, char const *str) {
   return strcmp(al_cstr(data->tag), str) == 0;
}

static bool attribute_is(BMFONT_DATA *data, char const *str) {
   return strcmp(al_cstr(data->attribute), str) == 0;
}

static int get_int(char const *value) {
   return strtol(value, NULL, 0);
}

static int xml_callback(XmlState state, char const *value, void *u)
{
   ALLEGRO_FONT *font = u;
   BMFONT_DATA *data = font->data;
   if (state == ElementName) {
      al_ustr_assign_cstr(data->tag, value);
      if (tag_is(data, "char")) {
         data->c = al_calloc(1, sizeof *data->c);
      }
      else if (tag_is(data, "kerning")) {
         data->kerning_pairs++;
         data->kerning = al_realloc(data->kerning, data->kerning_pairs *
            sizeof *data->kerning);
      }
   }
   if (state == AttributeName) {
      al_ustr_assign_cstr(data->attribute, value);
   }
   if (state == AttributeValue) {
      if (tag_is(data, "char")) {
         if (attribute_is(data, "x")) data->c->x = get_int(value);
         else if (attribute_is(data, "y")) data->c->y = get_int(value);
         else if (attribute_is(data, "xoffset")) data->c->xoffset = get_int(value);
         else if (attribute_is(data, "yoffset")) data->c->yoffset = get_int(value);
         else if (attribute_is(data, "width")) data->c->width = get_int(value);
         else if (attribute_is(data, "height")) data->c->height = get_int(value);
         else if (attribute_is(data, "page")) data->c->page = get_int(value);
         else if (attribute_is(data, "xadvance")) data->c->xadvance = get_int(value);
         else if (attribute_is(data, "chnl")) data->c->chnl = get_int(value);
         else if (attribute_is(data, "id")) {
            add_codepoint(data, get_int(value));
         }
      }
      else if (tag_is(data, "page")) {
         if (attribute_is(data, "file")) {
            add_page(data, value);
         }
      }
      else if (tag_is(data, "common")) {
         if (attribute_is(data, "lineHeight")) data->line_height = get_int(value);
         else if (attribute_is(data, "base")) data->base = get_int(value);
      }
      else if (tag_is(data, "kerning")) {
         BMFONT_KERNING *k = data->kerning + data->kerning_pairs - 1;
         if (attribute_is(data, "first")) k->first = get_int(value);
         else if (attribute_is(data, "second")) k->second = get_int(value);
         else if (attribute_is(data, "amount")) k->amount = get_int(value);
      }
   }
   return 0;
}

static int font_height(const ALLEGRO_FONT *f) {
   BMFONT_DATA *data = f->data;
   return data->line_height;
}

static int font_ascent(const ALLEGRO_FONT *f) {
   BMFONT_DATA *data = f->data;
   return data->base;
}

static int font_descent(const ALLEGRO_FONT *f) {
   BMFONT_DATA *data = f->data;
   return data->line_height - data->base;
}

static int get_kerning(BMFONT_CHAR *prev, int c) {
   if (!prev) return 0;
   for (int i = 0; i < prev->kerning_pairs; i++) {
      if (prev->kerning[i].second == c) {
         int a = prev->kerning[i].amount;
         return a;
      }
   }
   return 0;
}

static int each_character(const ALLEGRO_FONT *f, ALLEGRO_COLOR color,
      const ALLEGRO_USTR *text, float x, float y, ALLEGRO_GLYPH *glyph,
      int (*cb)(const ALLEGRO_FONT *f, ALLEGRO_COLOR color, int ch,
      float x, float y, ALLEGRO_GLYPH *glyph)) {
   BMFONT_DATA *data = f->data;
   int pos = 0;
   int advance = 0;
   int prev = 0;
   while (true) {
      int c = al_ustr_get_next(text, &pos);
      if (c < 0) break;
      if (prev) {
         BMFONT_CHAR *pc = find_codepoint(data, prev);
         advance += get_kerning(pc, c);
      }
      advance += cb(f, color, c, x + advance, y, glyph);
      prev = c;
   }
   return advance;
}

static int measure_char(const ALLEGRO_FONT *f, ALLEGRO_COLOR color,
      int ch, float x, float y, ALLEGRO_GLYPH *glyph) {
   (void)color;
   (void)y;
   BMFONT_DATA *data = f->data;
   BMFONT_CHAR *c = find_codepoint(data, ch);
   int advance = 0;
   int xo = 0, yo = 0, w = 0, h = 0;
   if (c) {
      advance = c->xadvance;
      xo = c->xoffset;
      yo = c->yoffset;
      w = c->width;
      h = c->height;
   }
   else {
      if (!f->fallback) return 0;
      advance = al_get_glyph_width(f->fallback, ch);
      al_get_glyph_dimensions(f->fallback, ch, &xo, &yo, &w, &h);
   }
   if (glyph) {
      if (glyph->x == INT_MAX) glyph->x = xo;
      if (yo < glyph->y) glyph->y = yo;
      if (yo + h > glyph->h) glyph->h = yo + h;
      if (x + xo + w > glyph->w) glyph->w = x + xo + w;
   }
   return advance;
}

static bool get_glyph_dimensions(const ALLEGRO_FONT *f,
      int codepoint, int *bbx, int *bby, int *bbw, int *bbh) {
   BMFONT_DATA *data = f->data;
   BMFONT_CHAR *c = find_codepoint(data, codepoint);
   if (!c) {
      if (!f->fallback) return false;
      return al_get_glyph_dimensions(f->fallback, codepoint,
         bbx, bby, bbw, bbh);
   }
   *bbx = c->xoffset;
   *bby = c->yoffset;
   *bbw = c->width;
   *bbh = c->height;
   return true;
}

static int get_glyph_advance(const ALLEGRO_FONT *f,
      int codepoint1, int codepoint2) {
   BMFONT_DATA *data = f->data;
   BMFONT_CHAR *prev = find_codepoint(data, codepoint1);
   BMFONT_CHAR *c = find_codepoint(data, codepoint2);
   if (!c) {
      if (!f->fallback) return 0;
      return al_get_glyph_advance(f->fallback, codepoint1, codepoint2);
   }
   return c->xadvance + get_kerning(prev, codepoint2);
}

static bool get_glyph (const ALLEGRO_FONT *f, int prev_codepoint,
      int codepoint, ALLEGRO_GLYPH *glyph) {
   BMFONT_DATA *data = f->data;
   BMFONT_CHAR *prev = find_codepoint(data, prev_codepoint);
   BMFONT_CHAR *c = find_codepoint(data, codepoint);
   if (c) {
      glyph->bitmap = data->pages[c->page];
      glyph->x = c->x;
      glyph->y = c->y;
      glyph->w = c->width;
      glyph->h = c->height;
      glyph->kerning = get_kerning(prev, codepoint);
      glyph->offset_x = c->xoffset;
      glyph->offset_y = c->yoffset;
      glyph->advance = c->xadvance + glyph->kerning;
      return true;
   }
   if (f->fallback) {
      return al_get_glyph(f->fallback, prev_codepoint, codepoint,
         glyph);
   }
   return false;
}

static int char_length(const ALLEGRO_FONT *f, int ch) {
   ALLEGRO_COLOR color = {};
   return measure_char(f, color, ch, 0, 0, NULL);
}

static int text_length(const ALLEGRO_FONT *f, const ALLEGRO_USTR *text) {
   ALLEGRO_COLOR color = {};
   return each_character(f, color, text, 0, 0, NULL, measure_char);
}

static int render_char(const ALLEGRO_FONT *f, ALLEGRO_COLOR color, int ch, float x, float y) {
   BMFONT_DATA *data = f->data;
   BMFONT_CHAR *c = find_codepoint(data, ch);
   if (!c) {
      if (f->fallback) return f->fallback->vtable->render_char(
         f->fallback, color, ch, x, y);
      return 0;
   }
   ALLEGRO_BITMAP *page = data->pages[c->page];
   al_draw_tinted_bitmap_region(page, color, c->x, c->y, c->width, c->height,
      x + c->xoffset, y + c->yoffset, 0);
   return c->xadvance;
}

static int render_char_cb(const ALLEGRO_FONT *f, ALLEGRO_COLOR color,
      int ch, float x, float y, ALLEGRO_GLYPH *glyph) {
   (void)glyph;
   return render_char(f, color, ch, x, y);
}

static int render(const ALLEGRO_FONT *f, ALLEGRO_COLOR color,
      const ALLEGRO_USTR *text, float x, float y) {
   return each_character(f, color, text, x, y, NULL, render_char_cb);
}

static void destroy_range(BMFONT_RANGE *range) {
   for (int i = 0; i < range->count; i++) {
      BMFONT_CHAR *c = range->characters[i];
      al_free(c->kerning);
      al_free(c);
   }
   al_free(range);
}

static void destroy(ALLEGRO_FONT *f) {
   BMFONT_DATA *data = f->data;
   BMFONT_RANGE *range = data->range_first;
   while (range) {
      BMFONT_RANGE *next = range->next;
      destroy_range(range);
      range = next;
   }

   for (int i = 0; i < data->pages_count; i++) {
      al_destroy_bitmap(data->pages[i]);
   }
   al_free(data->pages);
   
   al_free(data->kerning);
   al_free(f);
}

static void get_text_dimensions(const ALLEGRO_FONT *f,
      const ALLEGRO_USTR *text, int *bbx, int *bby, int *bbw, int *bbh) {
   ALLEGRO_COLOR color = {};
   ALLEGRO_GLYPH glyph;
   glyph.x = INT_MAX;
   glyph.y = INT_MAX;
   glyph.w = INT_MIN;
   glyph.h = INT_MIN;
   each_character(f, color, text, 0, 0, &glyph, measure_char);
   *bbx = glyph.x;
   *bby = glyph.y;
   *bbw = glyph.w - glyph.x;
   *bbh = glyph.h - glyph.y;
}

static int get_font_ranges(ALLEGRO_FONT *f,
      int ranges_count, int *ranges) {
   BMFONT_DATA *data = f->data;
   BMFONT_RANGE *range = data->range_first;
   int i = 0;
   while (range) {
      if (i < ranges_count) {
         ranges[i * 2 + 0] = range->first;
         ranges[i * 2 + 1] = range->first + range->count - 1;
      }
      range = range->next;
      i++;
   }
   return i;
}

static ALLEGRO_FONT_VTABLE _al_font_vtable_xml = {
   font_height,
   font_ascent, 
   font_descent,
   char_length,
   text_length,
   render_char,
   render,
   destroy, 
   get_text_dimensions,
   get_font_ranges,
   get_glyph_dimensions,
   get_glyph_advance,
   get_glyph
};

ALLEGRO_FONT *_al_load_bmfont_xml(const char *filename, int size,
      int font_flags)
{
   (void)size;
   ALLEGRO_FILE *f = al_fopen(filename, "r");
   if (!f) {
      ALLEGRO_DEBUG("Could not open %s.\n", filename);
      return NULL;
   }

   BMFONT_DATA *data = al_calloc(1, sizeof *data);
   data->tag = al_ustr_new("");
   data->attribute = al_ustr_new("");
   data->path = al_create_path(filename);
   data->flags = font_flags;

   ALLEGRO_FONT *font = al_calloc(1, sizeof *font);
   font->vtable = &_al_font_vtable_xml;
   font->data = data;

   _al_xml_parse(f, xml_callback, font);

   for (int i = 0; i < data->kerning_pairs; i++) {
      BMFONT_KERNING *k = data->kerning + i;
      BMFONT_CHAR *c = find_codepoint(data, k->first);
      c->kerning_pairs++;
      c->kerning = al_realloc(c->kerning, c->kerning_pairs *
         sizeof *c->kerning);
      c->kerning[c->kerning_pairs - 1] = *k;
   }

   al_ustr_free(data->tag);
   al_ustr_free(data->attribute);
   al_destroy_path(data->path);
   
   return font;
}
