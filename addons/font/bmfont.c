#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include <stdio.h>

#include "allegro5/internal/aintern_font.h"

#include "font.h"
#include "xml.h"

A5O_DEBUG_CHANNEL("font")

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
   int pages_count;
   A5O_BITMAP **pages;
   BMFONT_RANGE *range_first;
   int base;
   int line_height;
   int flags;

   int kerning_pairs;
   BMFONT_KERNING *kerning;
} BMFONT_DATA;

typedef struct {
   A5O_FONT *font;
   A5O_USTR *tag;
   A5O_USTR *attribute;
   BMFONT_CHAR *c;
   A5O_PATH *path;
} BMFONT_PARSER;

static void reallocate(BMFONT_RANGE *range) {
   range->characters = al_realloc(range->characters,
      range->count * sizeof *range->characters);
}

static void prepend_char(BMFONT_PARSER *parser, BMFONT_RANGE *range) {
   range->first--;
   range->count++;
   reallocate(range);
   memmove(range->characters + 1, range->characters,
      (range->count - 1) * sizeof *range->characters);
   range->characters[0] = parser->c;
}

static void append_char(BMFONT_PARSER *parser, BMFONT_RANGE *range) {
   range->count++;
   reallocate(range);
   range->characters[range->count - 1] = parser->c;
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

static void insert_new_range(BMFONT_PARSER *parser, BMFONT_RANGE *prev,
      int codepoint) {
   BMFONT_RANGE *range = al_calloc(1, sizeof *range);
   range->first = codepoint;
   range->count = 1;
   reallocate(range);
   range->characters[0] = parser->c;
   if (prev) {
      range->next = prev->next;
      prev->next = range;
   }
   else {
      A5O_FONT *font = parser->font;
      BMFONT_DATA *data = font->data;
      range->next = data->range_first;
      data->range_first = range;
   }
}

/* Adding a new codepoint creates either a new range consisting of that
 * single codepoint, appends the codepoint to the end of a range,
 * prepends it to the beginning of a range, or merges two ranges.
 */
static void add_codepoint(BMFONT_PARSER *parser, int codepoint) {
   A5O_FONT *font = parser->font;
   BMFONT_DATA *data = font->data;
   BMFONT_RANGE *prev = NULL;
   BMFONT_RANGE *range = data->range_first;
   while (range) {
      if (codepoint == range->first - 1) {
         prepend_char(parser, range);
         return;
      }
      if (codepoint < range->first) {
         insert_new_range(parser, prev, codepoint);
         return;
      }
      if (codepoint == range->first + range->count) {
         append_char(parser, range);
         BMFONT_RANGE *range2 = range->next;
         if (range2 != NULL && codepoint == range2->first - 1) {
            combine_ranges(data, range, range2);
         }
         return;
      }
      prev = range;
      range = range->next;
   }
   insert_new_range(parser, prev, codepoint);
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

static void add_page(BMFONT_PARSER *parser, char const *filename) {
   A5O_FONT *font = parser->font;
   BMFONT_DATA *data = font->data;
   data->pages_count++;
   data->pages = al_realloc(data->pages, data->pages_count *
      sizeof *data->pages);
   al_set_path_filename(parser->path, filename);
   A5O_BITMAP *page = al_load_bitmap_flags(
      al_path_cstr(parser->path, '/'), data->flags);
   data->pages[data->pages_count - 1] = page;
}

static bool tag_is(BMFONT_PARSER *parser, char const *str) {
   return strcmp(al_cstr(parser->tag), str) == 0;
}

static bool attribute_is(BMFONT_PARSER *parser, char const *str) {
   return strcmp(al_cstr(parser->attribute), str) == 0;
}

static int get_int(char const *value) {
   return strtol(value, NULL, 10);
}

static int xml_callback(XmlState state, char const *value, void *u)
{
   BMFONT_PARSER *parser = u;
   A5O_FONT *font = parser->font;
   BMFONT_DATA *data = font->data;
   if (state == ElementName) {
      al_ustr_assign_cstr(parser->tag, value);
      if (tag_is(parser, "char")) {
         parser->c = al_calloc(1, sizeof *parser->c);
      }
      else if (tag_is(parser, "kerning")) {
         data->kerning_pairs++;
         data->kerning = al_realloc(data->kerning, data->kerning_pairs *
            sizeof *data->kerning);
      }
   }
   if (state == AttributeName) {
      al_ustr_assign_cstr(parser->attribute, value);
   }
   if (state == AttributeValue) {
      if (tag_is(parser, "char")) {
         if (attribute_is(parser, "x")) parser->c->x = get_int(value);
         else if (attribute_is(parser, "y")) parser->c->y = get_int(value);
         else if (attribute_is(parser, "xoffset")) parser->c->xoffset = get_int(value);
         else if (attribute_is(parser, "yoffset")) parser->c->yoffset = get_int(value);
         else if (attribute_is(parser, "width")) parser->c->width = get_int(value);
         else if (attribute_is(parser, "height")) parser->c->height = get_int(value);
         else if (attribute_is(parser, "page")) parser->c->page = get_int(value);
         else if (attribute_is(parser, "xadvance")) parser->c->xadvance = get_int(value);
         else if (attribute_is(parser, "chnl")) parser->c->chnl = get_int(value);
         else if (attribute_is(parser, "id")) {
            add_codepoint(parser, get_int(value));
         }
      }
      else if (tag_is(parser, "page")) {
         if (attribute_is(parser, "file")) {
            add_page(parser, value);
         }
      }
      else if (tag_is(parser, "common")) {
         if (attribute_is(parser, "lineHeight")) data->line_height = get_int(value);
         else if (attribute_is(parser, "base")) data->base = get_int(value);
      }
      else if (tag_is(parser, "kerning")) {
         BMFONT_KERNING *k = data->kerning + data->kerning_pairs - 1;
         if (attribute_is(parser, "first")) k->first = get_int(value);
         else if (attribute_is(parser, "second")) k->second = get_int(value);
         else if (attribute_is(parser, "amount")) k->amount = get_int(value);
      }
   }
   return 0;
}

static int font_height(const A5O_FONT *f) {
   BMFONT_DATA *data = f->data;
   return data->line_height;
}

static int font_ascent(const A5O_FONT *f) {
   BMFONT_DATA *data = f->data;
   return data->base;
}

static int font_descent(const A5O_FONT *f) {
   BMFONT_DATA *data = f->data;
   return data->line_height - data->base;
}

static int get_kerning(BMFONT_CHAR *prev, int c) {
   if (!prev) return 0;
   int i;
   for (i = 0; i < prev->kerning_pairs; i++) {
      if (prev->kerning[i].second == c) {
         int a = prev->kerning[i].amount;
         return a;
      }
   }
   return 0;
}

static int each_character(const A5O_FONT *f, A5O_COLOR color,
      const A5O_USTR *text, float x, float y, A5O_GLYPH *glyph,
      int (*cb)(const A5O_FONT *f, A5O_COLOR color, int ch,
      float x, float y, A5O_GLYPH *glyph)) {
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

static int measure_char(const A5O_FONT *f, A5O_COLOR color,
      int ch, float x, float y, A5O_GLYPH *glyph) {
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

static bool get_glyph_dimensions(const A5O_FONT *f,
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

static int get_glyph_advance(const A5O_FONT *f,
      int codepoint1, int codepoint2) {
   BMFONT_DATA *data = f->data;
   BMFONT_CHAR *c = find_codepoint(data, codepoint1);
   int kerning = 0;
   if (codepoint1 == A5O_NO_KERNING) {
      return 0;
   }

   if (!c) {
      if (!f->fallback) return 0;
      return al_get_glyph_advance(f->fallback, codepoint1, codepoint2);
   }

   if (codepoint2 != A5O_NO_KERNING)
      kerning = get_kerning(c, codepoint2);

   return c->xadvance + kerning;
}

static bool get_glyph(const A5O_FONT *f, int prev_codepoint,
      int codepoint, A5O_GLYPH *glyph) {
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

static int char_length(const A5O_FONT *f, int ch) {
   A5O_COLOR color = {0, 0, 0, 0};
   return measure_char(f, color, ch, 0, 0, NULL);
}

static int text_length(const A5O_FONT *f, const A5O_USTR *text) {
   A5O_COLOR color = {0, 0, 0, 0};
   return each_character(f, color, text, 0, 0, NULL, measure_char);
}

static int render_char(const A5O_FONT *f, A5O_COLOR color, int ch, float x, float y) {
   BMFONT_DATA *data = f->data;
   BMFONT_CHAR *c = find_codepoint(data, ch);
   if (!c) {
      if (f->fallback) return f->fallback->vtable->render_char(
         f->fallback, color, ch, x, y);
      return 0;
   }
   A5O_BITMAP *page = data->pages[c->page];
   al_draw_tinted_bitmap_region(page, color, c->x, c->y, c->width, c->height,
      x + c->xoffset, y + c->yoffset, 0);
   return c->xadvance;
}

static int render_char_cb(const A5O_FONT *f, A5O_COLOR color,
      int ch, float x, float y, A5O_GLYPH *glyph) {
   (void)glyph;
   return render_char(f, color, ch, x, y);
}

static int render(const A5O_FONT *f, A5O_COLOR color,
      const A5O_USTR *text, float x, float y) {
   return each_character(f, color, text, x, y, NULL, render_char_cb);
}

static void destroy_range(BMFONT_RANGE *range) {
   int i;
   for (i = 0; i < range->count; i++) {
      BMFONT_CHAR *c = range->characters[i];
      al_free(c->kerning);
      al_free(c);
   }
   al_free(range);
}

static void destroy(A5O_FONT *f) {
   BMFONT_DATA *data = f->data;
   BMFONT_RANGE *range = data->range_first;
   while (range) {
      BMFONT_RANGE *next = range->next;
      destroy_range(range);
      range = next;
   }

   int i;
   for (i = 0; i < data->pages_count; i++) {
      al_destroy_bitmap(data->pages[i]);
   }
   al_free(data->pages);
   
   al_free(data->kerning);
   al_free(f);
}

static void get_text_dimensions(const A5O_FONT *f,
      const A5O_USTR *text, int *bbx, int *bby, int *bbw, int *bbh) {
   A5O_COLOR color = {0, 0, 0, 0};
   A5O_GLYPH glyph;
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

static int get_font_ranges(A5O_FONT *f,
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

static A5O_FONT_VTABLE _al_font_vtable_xml = {
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

A5O_FONT *_al_load_bmfont_xml(const char *filename, int size,
      int font_flags)
{
   (void)size;
   A5O_FILE *f = al_fopen(filename, "r");
   if (!f) {
      A5O_DEBUG("Could not open %s.\n", filename);
      return NULL;
   }

   BMFONT_DATA *data = al_calloc(1, sizeof *data);
   BMFONT_PARSER _parser;
   BMFONT_PARSER *parser = &_parser;
   parser->tag = al_ustr_new("");
   parser->attribute = al_ustr_new("");
   parser->path = al_create_path(filename);
   data->flags = font_flags;

   A5O_FONT *font = al_calloc(1, sizeof *font);
   font->vtable = &_al_font_vtable_xml;
   font->data = data;
   parser->font = font;

   _al_xml_parse(f, xml_callback, parser);

   int i;
   for (i = 0; i < data->kerning_pairs; i++) {
      BMFONT_KERNING *k = data->kerning + i;
      BMFONT_CHAR *c = find_codepoint(data, k->first);
      c->kerning_pairs++;
      c->kerning = al_realloc(c->kerning, c->kerning_pairs *
         sizeof *c->kerning);
      c->kerning[c->kerning_pairs - 1] = *k;
   }

   al_ustr_free(parser->tag);
   al_ustr_free(parser->attribute);
   al_destroy_path(parser->path);
   
   return font;
}
