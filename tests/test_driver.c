/*
 *    Automated testing driver program for Allegro.
 *
 *    By Peter Wang.
 */

#define ALLEGRO_UNSTABLE
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>

#define MAX_BITMAPS  128
#define MAX_TRANS    8
#define MAX_FONTS    16
#define MAX_VERTICES 100
#define MAX_POLYGONS 8

typedef struct {
   ALLEGRO_USTR   *name;
   ALLEGRO_BITMAP *bitmap[2];
} Bitmap;

typedef enum {
   SW = 0,
   HW = 1
} BmpType;

typedef struct {
   ALLEGRO_USTR   *name;
   ALLEGRO_TRANSFORM transform;
} Transform;

typedef struct {
   int            x;
   int            y;
   int            w;
   int            h;
   ALLEGRO_LOCKED_REGION *lr;
} LockRegion;

typedef struct {
   ALLEGRO_USTR   *name;
   ALLEGRO_FONT   *font;
} NamedFont;

int               argc;
char              **argv;
ALLEGRO_DISPLAY   *display;
ALLEGRO_BITMAP    *membuf;
Bitmap            bitmaps[MAX_BITMAPS];
LockRegion        lock_region;
Transform         transforms[MAX_TRANS];
NamedFont         fonts[MAX_FONTS];
ALLEGRO_VERTEX    vertices[MAX_VERTICES];
float             simple_vertices[2 * MAX_VERTICES];
int               num_simple_vertices;
int               vertex_counts[MAX_POLYGONS];
int               num_global_bitmaps;
float             delay = 0.0;
bool              save_outputs = false;
bool              save_on_failure = false;
bool              quiet = false;
bool              want_display = true;
bool              on_xvfb = true;
int               verbose = 0;
int               total_tests = 0;
int               passed_tests = 0;
int               failed_tests = 0;
int               skipped_tests = 0;

#define streq(a, b)  (0 == strcmp((a), (b)))

/* Helper macros for scanning statements. */
#define PAT       " %79[A-Za-z0-9_.$|#-] "
#define PAT1      PAT
#define PAT2      PAT1 "," PAT1
#define PAT3      PAT2 "," PAT1
#define PAT4      PAT3 "," PAT1
#define PAT5      PAT4 "," PAT1
#define PAT6      PAT5 "," PAT1
#define PAT7      PAT6 "," PAT1
#define PAT8      PAT7 "," PAT1
#define PAT9      PAT8 "," PAT1
#define PAT10     PAT9 "," PAT1
#define PAT11     PAT10 "," PAT1
#define PAT12     PAT11 "," PAT1
#define PAT13     PAT12 "," PAT1
#define PAT14     PAT13 "," PAT1
#define ARGS1     arg[0]
#define ARGS2     ARGS1, arg[1]
#define ARGS3     ARGS2, arg[2]
#define ARGS4     ARGS3, arg[3]
#define ARGS5     ARGS4, arg[4]
#define ARGS6     ARGS5, arg[5]
#define ARGS7     ARGS6, arg[6]
#define ARGS8     ARGS7, arg[7]
#define ARGS9     ARGS8, arg[8]
#define ARGS10    ARGS9, arg[9]
#define ARGS11    ARGS10, arg[10]
#define ARGS12    ARGS11, arg[11]
#define ARGS13    ARGS12, arg[12]
#define ARGS14    ARGS13, arg[13]
#define V(a)      resolve_var(cfg, section, arg[(a)])
#define I(a)      atoi(V(a))
#define F(a)      atof(V(a))
#define C(a)      get_color(V(a))
#define B(a)      get_bitmap(V(a), bmp_type, target)
#define SCAN0(fn) \
      (sscanf(stmt, fn " %79[(]" " )", ARGS1) == 1)
#define SCAN(fn, arity) \
      (sscanf(stmt, fn " (" PAT##arity " )", ARGS##arity) == arity)
#define SCANLVAL(fn, arity) \
      (sscanf(stmt, PAT " = " fn " (" PAT##arity " )", lval, ARGS##arity) \
         == 1 + arity)

static void fatal_error(char const *msg, ...)
{
   va_list ap;

   va_start(ap, msg);
   fprintf(stderr, "test_driver: ");
   vfprintf(stderr, msg, ap);
   fprintf(stderr, "\n");
   va_end(ap);
   exit(EXIT_FAILURE);
}

static char const *bmp_type_to_string(BmpType bmp_type)
{
   switch (bmp_type) {
      case SW: return "sw";
      case HW: return "hw";
   }
   return "error";
}

static ALLEGRO_BITMAP *create_fallback_bitmap(void)
{
   ALLEGRO_BITMAP *bmp = al_create_bitmap(256, 256);
   ALLEGRO_FONT *builtin_font = al_create_builtin_font();
   ALLEGRO_STATE state;

   if (!bmp) {
      fatal_error("couldn't create a backup bitmap");
   }
   if (!builtin_font) {
      fatal_error("couldn't create a builtin font");
   }
   al_store_state(&state, ALLEGRO_STATE_BITMAP);
   al_set_target_bitmap(bmp);
   al_clear_to_color(al_map_rgb_f(0.5, 0, 0));
   al_draw_text(builtin_font, al_map_rgb_f(1, 1, 1), 0, 0, 0, "fallback");
   al_restore_state(&state);
   al_destroy_font(builtin_font);

   return bmp;
}

static ALLEGRO_BITMAP *load_relative_bitmap(char const *filename, int flags)
{
   ALLEGRO_BITMAP *bmp;

   bmp = al_load_bitmap_flags(filename, flags);
   if (!bmp) {
      fprintf(stderr, "test_driver: failed to load %s\n", filename);
      bmp = create_fallback_bitmap();
   }
   return bmp;
}

static void load_bitmaps(ALLEGRO_CONFIG const *cfg, const char *section,
   BmpType bmp_type, int flags)
{
   int i = 0;
   ALLEGRO_CONFIG_ENTRY *iter;
   char const *key;
   char const *value;

   key = al_get_first_config_entry(cfg, section, &iter);
   while (key && i < MAX_BITMAPS) {
      value = al_get_config_value(cfg, section, key);

      bitmaps[i].name = al_ustr_new(key);
      bitmaps[i].bitmap[bmp_type] = load_relative_bitmap(value, flags);

      key = al_get_next_config_entry(&iter);
      i++;
   }

   if (i == MAX_BITMAPS)
      fatal_error("bitmap limit reached");

   num_global_bitmaps = i;
}

static ALLEGRO_BITMAP **reserve_local_bitmap(const char *name, BmpType bmp_type)
{
   int i;

   for (i = num_global_bitmaps; i < MAX_BITMAPS; i++) {
      if (!bitmaps[i].name) {
         bitmaps[i].name = al_ustr_new(name);
         return &bitmaps[i].bitmap[bmp_type];
      }
   }

   fatal_error("bitmap limit reached");
   return NULL;
}

static void unload_data(void)
{
   int i;

   for (i = 0; i < MAX_BITMAPS; i++) {
      al_ustr_free(bitmaps[i].name);
      al_destroy_bitmap(bitmaps[i].bitmap[0]);
      al_destroy_bitmap(bitmaps[i].bitmap[1]);
   }
   memset(bitmaps, 0, sizeof(bitmaps));

   for (i = 0; i < MAX_FONTS; i++) {
      al_ustr_free(fonts[i].name);
      al_destroy_font(fonts[i].font);
   }
   memset(fonts, 0, sizeof(fonts));

   num_global_bitmaps = 0;
}

static void set_target_reset(ALLEGRO_BITMAP *target)
{
   ALLEGRO_TRANSFORM ident;

   al_set_target_bitmap(target);
   al_clear_to_color(al_map_rgb(0, 0, 0));
   al_reset_clipping_rectangle();
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
   al_identity_transform(&ident);
   al_use_transform(&ident);
   al_orthographic_transform(&ident, 0, 0, -1, al_get_bitmap_width(target), al_get_bitmap_height(target), 1);
   al_use_projection_transform(&ident);
}

static char const *resolve_var(ALLEGRO_CONFIG const *cfg, char const *section,
   char const *v)
{
   char const *vv = al_get_config_value(cfg, section, v);
   return (vv) ? vv : v;
}

static bool get_bool(char const *value)
{
   return streq(value, "true") ? true
      : streq(value, "false") ? false
      : atoi(value);
}

static ALLEGRO_COLOR get_color(char const *value)
{
   int r, g, b, a;
   float rf, gf, bf;

   if (sscanf(value, "#%02x%02x%02x%02x", &r, &g, &b, &a) == 4)
      return al_map_rgba(r, g, b, a);
   if (sscanf(value, "#%02x%02x%02x", &r, &g, &b) == 3)
      return al_map_rgb(r, g, b);
   if (sscanf(value, "%f/%f/%f", &rf, &gf, &bf) == 3)
      return al_map_rgb_f(rf, gf, bf);
   return al_color_name(value);
}

static ALLEGRO_BITMAP *get_bitmap(char const *value, BmpType bmp_type,
   ALLEGRO_BITMAP *target)
{
   int i;

   for (i = 0; i < MAX_BITMAPS; i++) {
      if (bitmaps[i].name && streq(al_cstr(bitmaps[i].name), value))
         return bitmaps[i].bitmap[bmp_type];
   }

   if (streq(value, "target"))
      return target;

   if (streq(value, "0") || streq(value, "NULL"))
      return NULL;

   fatal_error("undefined bitmap: %s", value);
   return NULL;
}

static int get_load_bitmap_flag(char const *value)
{
   if (streq(value, "ALLEGRO_NO_PREMULTIPLIED_ALPHA"))
      return ALLEGRO_NO_PREMULTIPLIED_ALPHA;
   if (streq(value, "ALLEGRO_KEEP_INDEX"))
      return ALLEGRO_KEEP_INDEX;
   return atoi(value);
}

static int get_draw_bitmap_flag(char const *value)
{
   if (streq(value, "ALLEGRO_FLIP_HORIZONTAL"))
      return ALLEGRO_FLIP_HORIZONTAL;
   if (streq(value, "ALLEGRO_FLIP_VERTICAL"))
      return ALLEGRO_FLIP_VERTICAL;
   if (streq(value, "ALLEGRO_FLIP_VERTICAL|ALLEGRO_FLIP_HORIZONTAL"))
      return ALLEGRO_FLIP_VERTICAL|ALLEGRO_FLIP_HORIZONTAL;
   if (streq(value, "ALLEGRO_FLIP_HORIZONTAL|ALLEGRO_FLIP_VERTICAL"))
      return ALLEGRO_FLIP_HORIZONTAL|ALLEGRO_FLIP_VERTICAL;
   return atoi(value);
}

static int get_blender_op(char const *value)
{
   return streq(value, "ALLEGRO_ADD") ? ALLEGRO_ADD
      : streq(value, "ALLEGRO_DEST_MINUS_SRC") ? ALLEGRO_DEST_MINUS_SRC
      : streq(value, "ALLEGRO_SRC_MINUS_DEST") ? ALLEGRO_SRC_MINUS_DEST
      : atoi(value);
}

static int get_blend_factor(char const *value)
{
   return streq(value, "ALLEGRO_ZERO") ? ALLEGRO_ZERO
      : streq(value, "ALLEGRO_ONE") ? ALLEGRO_ONE
      : streq(value, "ALLEGRO_ALPHA") ? ALLEGRO_ALPHA
      : streq(value, "ALLEGRO_INVERSE_ALPHA") ? ALLEGRO_INVERSE_ALPHA
      : streq(value, "ALLEGRO_SRC_COLOR") ? ALLEGRO_SRC_COLOR
      : streq(value, "ALLEGRO_DEST_COLOR") ? ALLEGRO_DEST_COLOR
      : streq(value, "ALLEGRO_INVERSE_SRC_COLOR") ? ALLEGRO_INVERSE_SRC_COLOR
      : streq(value, "ALLEGRO_INVERSE_DEST_COLOR") ? ALLEGRO_INVERSE_DEST_COLOR
      : streq(value, "ALLEGRO_CONST_COLOR") ? ALLEGRO_CONST_COLOR
      : streq(value, "ALLEGRO_INVERSE_CONST_COLOR") ? ALLEGRO_INVERSE_CONST_COLOR
      : atoi(value);
}

static ALLEGRO_TRANSFORM *get_transform(const char *name)
{
   int i;

   for (i = 0; i < MAX_TRANS; i++) {
      if (!transforms[i].name) {
         transforms[i].name = al_ustr_new(name);
         al_identity_transform(&transforms[i].transform);
         return &transforms[i].transform;
      }

      if (transforms[i].name && streq(al_cstr(transforms[i].name), name))
         return &transforms[i].transform;
   }

   fatal_error("transforms limit reached");
   return NULL;
}

static int get_pixel_format(char const *v)
{
   int format = streq(v, "ALLEGRO_PIXEL_FORMAT_ANY") ? ALLEGRO_PIXEL_FORMAT_ANY
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA") ? ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA") ? ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA") ? ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA") ? ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA") ? ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA") ? ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA") ? ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA") ? ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ARGB_8888") ? ALLEGRO_PIXEL_FORMAT_ARGB_8888
      : streq(v, "ALLEGRO_PIXEL_FORMAT_RGBA_8888") ? ALLEGRO_PIXEL_FORMAT_RGBA_8888
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ARGB_4444") ? ALLEGRO_PIXEL_FORMAT_ARGB_4444
      : streq(v, "ALLEGRO_PIXEL_FORMAT_RGB_888") ? ALLEGRO_PIXEL_FORMAT_RGB_888
      : streq(v, "ALLEGRO_PIXEL_FORMAT_RGB_565") ? ALLEGRO_PIXEL_FORMAT_RGB_565
      : streq(v, "ALLEGRO_PIXEL_FORMAT_RGB_555") ? ALLEGRO_PIXEL_FORMAT_RGB_555
      : streq(v, "ALLEGRO_PIXEL_FORMAT_RGBA_5551") ? ALLEGRO_PIXEL_FORMAT_RGBA_5551
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ARGB_1555") ? ALLEGRO_PIXEL_FORMAT_ARGB_1555
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ABGR_8888") ? ALLEGRO_PIXEL_FORMAT_ABGR_8888
      : streq(v, "ALLEGRO_PIXEL_FORMAT_XBGR_8888") ? ALLEGRO_PIXEL_FORMAT_XBGR_8888
      : streq(v, "ALLEGRO_PIXEL_FORMAT_BGR_888") ? ALLEGRO_PIXEL_FORMAT_BGR_888
      : streq(v, "ALLEGRO_PIXEL_FORMAT_BGR_565") ? ALLEGRO_PIXEL_FORMAT_BGR_565
      : streq(v, "ALLEGRO_PIXEL_FORMAT_BGR_555") ? ALLEGRO_PIXEL_FORMAT_BGR_555
      : streq(v, "ALLEGRO_PIXEL_FORMAT_RGBX_8888") ? ALLEGRO_PIXEL_FORMAT_RGBX_8888
      : streq(v, "ALLEGRO_PIXEL_FORMAT_XRGB_8888") ? ALLEGRO_PIXEL_FORMAT_XRGB_8888
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ABGR_F32") ? ALLEGRO_PIXEL_FORMAT_ABGR_F32
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE") ? ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE
      : streq(v, "ALLEGRO_PIXEL_FORMAT_RGBA_4444") ? ALLEGRO_PIXEL_FORMAT_RGBA_4444
      : streq(v, "ALLEGRO_PIXEL_FORMAT_COMPRESSED_RGBA_DXT1") ? ALLEGRO_PIXEL_FORMAT_COMPRESSED_RGBA_DXT1
      : streq(v, "ALLEGRO_PIXEL_FORMAT_COMPRESSED_RGBA_DXT3") ? ALLEGRO_PIXEL_FORMAT_COMPRESSED_RGBA_DXT3
      : streq(v, "ALLEGRO_PIXEL_FORMAT_COMPRESSED_RGBA_DXT5") ? ALLEGRO_PIXEL_FORMAT_COMPRESSED_RGBA_DXT5
      : -1;
   if (format == -1)
      fatal_error("invalid format: %s", v);
   return format;
}

static int get_lock_bitmap_flags(char const *v)
{
   return streq(v, "ALLEGRO_LOCK_READWRITE") ? ALLEGRO_LOCK_READWRITE
      : streq(v, "ALLEGRO_LOCK_READONLY") ? ALLEGRO_LOCK_READONLY
      : streq(v, "ALLEGRO_LOCK_WRITEONLY") ? ALLEGRO_LOCK_WRITEONLY
      : atoi(v);
}

static int get_bitmap_flags(char const *v)
{
   return streq(v, "ALLEGRO_MEMORY_BITMAP") ? ALLEGRO_MEMORY_BITMAP
      : streq(v, "ALLEGRO_VIDEO_BITMAP") ? ALLEGRO_VIDEO_BITMAP
      : atoi(v);
}

static void fill_lock_region(LockRegion *lr, float alphafactor, bool blended)
{
   int x, y;
   float r, g, b, a;
   ALLEGRO_COLOR c;

   for (y = 0; y < lr->h; y++) {
      for (x = 0; x < lr->w; x++) {
         /* -1 to make the last pixel use the full color, allows easier
          * manual inspection of the test results as we have a fixed
          * color on each side.
          */
         r = (float)x / (lr->w - 1);
         b = (float)y / (lr->h - 1);
         g = r*b;
         a = r * alphafactor;
         c = al_map_rgba_f(r, g, b, a);
         if (blended)
            al_put_blended_pixel(lr->x + x, lr->y + y, c);
         else
            al_put_pixel(lr->x + x, lr->y + y, c);
      }
   }
}

static int get_load_font_flags(char const *v)
{
   return streq(v, "ALLEGRO_NO_PREMULTIPLIED_ALPHA") ? ALLEGRO_NO_PREMULTIPLIED_ALPHA
      : streq(v, "ALLEGRO_TTF_NO_KERNING") ? ALLEGRO_TTF_NO_KERNING
      : streq(v, "ALLEGRO_TTF_MONOCHROME") ? ALLEGRO_TTF_MONOCHROME
      : atoi(v);
}

static void load_fonts(ALLEGRO_CONFIG const *cfg, const char *section)
{
#define MAXBUF    80

   int i = 0;
   ALLEGRO_CONFIG_ENTRY *iter;
   char const *key;
   char arg[14][MAXBUF];

   key = al_get_first_config_entry(cfg, section, &iter);
   while (key && i < MAX_FONTS) {
      char const *stmt = al_get_config_value(cfg, section, key);
      ALLEGRO_FONT *font = NULL;
      bool load_stmt = false;

      if (SCAN("al_load_font", 3)) {
         font = al_load_font(V(0), I(1), get_load_font_flags(V(2)));
         load_stmt = true;
      }
      else if (SCAN("al_load_ttf_font", 3)) {
         font = al_load_ttf_font(V(0), I(1), get_load_font_flags(V(2)));
         load_stmt = true;
      }
      else if (SCAN("al_load_ttf_font_stretch", 4)) {
         font = al_load_ttf_font_stretch(V(0), I(1), I(2),
            get_load_font_flags(V(3)));
         load_stmt = true;
      }
      else if (SCAN0("al_create_builtin_font")) {
         font = al_create_builtin_font();
         load_stmt = true;
      }

      if (load_stmt) {
         if (!font) {
            fatal_error("failed to load font: %s", key);
         }
         fonts[i].name = al_ustr_new(key);
         fonts[i].font = font;
         i++;
      }

      key = al_get_next_config_entry(&iter);
   }

   if (i == MAX_FONTS)
      fatal_error("font limit reached");

#undef MAXBUF
}

static ALLEGRO_FONT *get_font(char const *name)
{
   int i;

   if (streq(name, "NULL"))
      return NULL;

   for (i = 0; i < MAX_FONTS; i++) {
      if (fonts[i].name && streq(al_cstr(fonts[i].name), name))
         return fonts[i].font;
   }

   fatal_error("undefined font: %s", name);
   return NULL;
}

static int get_font_align(char const *value)
{
   return streq(value, "ALLEGRO_ALIGN_LEFT") ? ALLEGRO_ALIGN_LEFT
      : streq(value, "ALLEGRO_ALIGN_CENTRE") ? ALLEGRO_ALIGN_CENTRE
      : streq(value, "ALLEGRO_ALIGN_RIGHT") ? ALLEGRO_ALIGN_RIGHT
      : streq(value, "ALLEGRO_ALIGN_INTEGER") ? ALLEGRO_ALIGN_INTEGER
      : streq(value, "ALLEGRO_ALIGN_LEFT|ALLEGRO_ALIGN_INTEGER")
         ? ALLEGRO_ALIGN_LEFT | ALLEGRO_ALIGN_INTEGER
      : streq(value, "ALLEGRO_ALIGN_RIGHT|ALLEGRO_ALIGN_INTEGER")
         ? ALLEGRO_ALIGN_RIGHT | ALLEGRO_ALIGN_INTEGER
      : streq(value, "ALLEGRO_ALIGN_CENTRE|ALLEGRO_ALIGN_INTEGER")
         ? ALLEGRO_ALIGN_CENTRE | ALLEGRO_ALIGN_INTEGER
      : atoi(value);
}

static void set_config_int(ALLEGRO_CONFIG *cfg, char const *section,
   char const *var, int value)
{
   char buf[40];
   sprintf(buf, "%d", value);
   al_set_config_value(cfg, section, var, buf);
}

static void set_config_float(ALLEGRO_CONFIG *cfg, char const *section,
   char const *var, float value)
{
   char buf[40];
   sprintf(buf, "%f", value);
   al_set_config_value(cfg, section, var, buf);
}

static void fill_vertices(ALLEGRO_CONFIG const *cfg, char const *name)
{
#define MAXBUF    80

   char const *value;
   char buf[MAXBUF];
   float x, y, z;
   float u, v;
   int i;

   memset(vertices, 0, sizeof(vertices));

   for (i = 0; i < MAX_VERTICES; i++) {
      sprintf(buf, "v%d", i);
      value = al_get_config_value(cfg, name, buf);
      if (!value)
         return;

      if (sscanf(value, " %f , %f , %f ; %f , %f ; %s",
            &x, &y, &z, &u, &v, buf) == 6) {
         vertices[i].x = x;
         vertices[i].y = y;
         vertices[i].z = z;
         vertices[i].u = u;
         vertices[i].v = v;
         vertices[i].color = get_color(buf);
      }
   }

#undef MAXBUF
}

static void fill_simple_vertices(ALLEGRO_CONFIG const *cfg, char const *name)
{
#define MAXBUF    80

   char const *value;
   char buf[MAXBUF];
   float x, y;
   int i;

   memset(simple_vertices, 0, sizeof(simple_vertices));

   for (i = 0; i < MAX_VERTICES; i++) {
      sprintf(buf, "v%d", i);
      value = al_get_config_value(cfg, name, buf);
      if (!value)
         break;

      if (sscanf(value, " %f , %f", &x, &y) == 2) {
         simple_vertices[2*i + 0] = x;
         simple_vertices[2*i + 1] = y;
      }
   }

   num_simple_vertices = i;

#undef MAXBUF
}

static void fill_vertex_counts(ALLEGRO_CONFIG const *cfg, char const *name)
{
#define MAXBUF    80

   char const *value;
   char buf[MAXBUF];
   int count;
   int i;

   memset(vertex_counts, 0, sizeof(vertex_counts));

   for (i = 0; i < MAX_POLYGONS; i++) {
      sprintf(buf, "p%d", i);
      value = al_get_config_value(cfg, name, buf);
      if (!value)
         break;

      if (sscanf(value, " %d ", &count) == 1) {
         vertex_counts[i] = count;
      }
   }

#undef MAXBUF
}

static int get_prim_type(char const *value)
{
   return streq(value, "ALLEGRO_PRIM_POINT_LIST") ? ALLEGRO_PRIM_POINT_LIST
      : streq(value, "ALLEGRO_PRIM_LINE_LIST") ? ALLEGRO_PRIM_LINE_LIST
      : streq(value, "ALLEGRO_PRIM_LINE_STRIP") ? ALLEGRO_PRIM_LINE_STRIP
      : streq(value, "ALLEGRO_PRIM_LINE_LOOP") ? ALLEGRO_PRIM_LINE_LOOP
      : streq(value, "ALLEGRO_PRIM_TRIANGLE_LIST") ? ALLEGRO_PRIM_TRIANGLE_LIST
      : streq(value, "ALLEGRO_PRIM_TRIANGLE_STRIP") ? ALLEGRO_PRIM_TRIANGLE_STRIP
      : streq(value, "ALLEGRO_PRIM_TRIANGLE_FAN") ? ALLEGRO_PRIM_TRIANGLE_FAN
      : atoi(value);
}

static int get_line_join(char const *value)
{
   return streq(value, "ALLEGRO_LINE_JOIN_NONE") ? ALLEGRO_LINE_JOIN_NONE
      : streq(value, "ALLEGRO_LINE_JOIN_BEVEL") ? ALLEGRO_LINE_JOIN_BEVEL
      : streq(value, "ALLEGRO_LINE_JOIN_ROUND") ? ALLEGRO_LINE_JOIN_ROUND
      : streq(value, "ALLEGRO_LINE_JOIN_MITER") ? ALLEGRO_LINE_JOIN_MITER
      : atoi(value);
}

static int get_line_cap(char const *value)
{
   return streq(value, "ALLEGRO_LINE_CAP_NONE") ? ALLEGRO_LINE_CAP_NONE
      : streq(value, "ALLEGRO_LINE_CAP_SQUARE") ? ALLEGRO_LINE_CAP_SQUARE
      : streq(value, "ALLEGRO_LINE_CAP_ROUND") ? ALLEGRO_LINE_CAP_ROUND
      : streq(value, "ALLEGRO_LINE_CAP_TRIANGLE") ? ALLEGRO_LINE_CAP_TRIANGLE
      : streq(value, "ALLEGRO_LINE_CAP_CLOSED") ? ALLEGRO_LINE_CAP_CLOSED
      : atoi(value);
}

/* FNV-1a algorithm, parameters from:
 * http://www.isthe.com/chongo/tech/comp/fnv/index.html
 */
#define FNV_OFFSET_BASIS   2166136261UL
#define FNV_PRIME          16777619

static uint32_t hash_bitmap(ALLEGRO_BITMAP *bmp)
{
   ALLEGRO_LOCKED_REGION *lr;
   int x, y, w, h;
   uint32_t hash;

   w = al_get_bitmap_width(bmp);
   h = al_get_bitmap_height(bmp);
   hash = FNV_OFFSET_BASIS;

   lr = al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,
      ALLEGRO_LOCK_READONLY);

   for (y = 0; y < h; y++) {
      /* Oops, I unintentially committed the first version of this with signed
       * chars and computing in BGRA order, so leave it like that so we don't
       * have to update a bunch of old hashes.
       */
      signed char const *data = ((signed char const *)lr->data) + y*lr->pitch;
      for (x = 0; x < w; x++) {
         hash ^= data[x*4 + 3]; hash *= FNV_PRIME;
         hash ^= data[x*4 + 2]; hash *= FNV_PRIME;
         hash ^= data[x*4 + 1]; hash *= FNV_PRIME;
         hash ^= data[x*4 + 0]; hash *= FNV_PRIME;
      }
   }

   al_unlock_bitmap(bmp);

   return hash;
}

/* Image "signature" I just made up:
 * We take the average intensity of 7x7 patches centred at 9x9 grid points on
 * the image.  Each of these values is reduced down to 6 bits so it can be
 * represented by one printable character in base64 encoding.  This gives a
 * reasonable signature length.
 */

#define SIG_GRID  9
#define SIG_LEN   (SIG_GRID * SIG_GRID)
#define SIG_LENZ  (SIG_LEN + 1)

static int patch_intensity(ALLEGRO_BITMAP *bmp, int cx, int cy)
{
   float sum = 0.0;
   int x, y;

   for (y = -3; y <= 3; y++) {
      for (x = -3; x <= 3; x++) {
         ALLEGRO_COLOR c = al_get_pixel(bmp, cx + x, cy + y);
         sum += c.r + c.g + c.b;
      }
   }

   return 255 * sum/(7*7*3);
}

static char const base64[64] =
   "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/";

static int base64_decode(char c)
{
   if (c >= '0' && c <= '9') return c - '0';
   if (c >= 'A' && c <= 'Z') return 10 + (c - 'A');
   if (c >= 'a' && c <= 'z') return 36 + (c - 'a');
   if (c == '+') return 62;
   if (c == '/') return 63;
   fatal_error("invalid base64 character: %c", c);
   return -1;
}

static void compute_signature(ALLEGRO_BITMAP *bmp, char sig[SIG_LENZ])
{
   int w = al_get_bitmap_width(bmp);
   int h = al_get_bitmap_height(bmp);
   int x, y;
   int n = 0;

   al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);

   for (y = 0; y < SIG_GRID; y++) {
      for (x = 0; x < SIG_GRID; x++) {
         int cx = (1 + x) * w/(1 + SIG_GRID);
         int cy = (1 + y) * h/(1 + SIG_GRID);
         int level = patch_intensity(bmp, cx, cy);
         sig[n] = base64[level >> 2];
         n++;
      }
   }

   sig[n] = '\0';

   al_unlock_bitmap(bmp);
}

static bool similar_signatures(char const sig1[SIG_LEN], char const sig2[SIG_LEN])
{
   int correct = 0;
   int i;

   for (i = 0; i < SIG_LEN; i++) {
      int q1 = base64_decode(sig1[i]);
      int q2 = base64_decode(sig2[i]);

      /* A difference of one quantisation level could be because two values
       * which were originally close by straddled a quantisation boundary.
       * A difference of two quantisation levels is a significant deviation.
       */
      if (abs(q1 - q2) > 1)
         return false;

      correct++;
   }

   return ((float)correct / SIG_LEN) > 0.95;
}

static bool check_hash(ALLEGRO_CONFIG const *cfg, char const *testname,
   ALLEGRO_BITMAP *bmp, BmpType bmp_type)
{
   char const *bt = bmp_type_to_string(bmp_type);
   char hash[16];
   char sig[SIG_LENZ];
   char const *exp;
   char const *sigexp;

   exp = al_get_config_value(cfg, testname, "hash");

   sigexp = al_get_config_value(cfg, testname, "sig");

   if (exp && streq(exp, "off")) {
      printf("OK   %s [%s] - hash check off\n", testname, bt);
      passed_tests++;
      return true;
   }

   sprintf(hash, "%08x", hash_bitmap(bmp));
   compute_signature(bmp, sig);

   if (verbose) {
      printf("hash=%s\n", hash);
      printf("sig=%s\n", sig);
   }

   if (!exp && !sigexp) {
      printf("NEW  %s [%s]\n\thash=%s\n\tsig=%s\n",
         testname, bt, hash, sig);
      return true;
   }

   if (exp && streq(hash, exp)) {
      printf("OK   %s [%s]\n", testname, bt);
      passed_tests++;
      return true;
   }

   if (sigexp && strlen(sigexp) != SIG_LEN) {
      printf("WARNING: ignoring bad signature: %s\n", sigexp);
      sigexp = NULL;
   }

   if (sigexp && similar_signatures(sig, sigexp)) {
      printf("OK   %s [%s] - by signature\n", testname, bt);
      passed_tests++;
      return true;
   }

   printf("FAIL %s [%s] - hash=%s\n", testname, bt, hash);
   failed_tests++;
   return false;
}

static double bitmap_dissimilarity(ALLEGRO_BITMAP *bmp1, ALLEGRO_BITMAP *bmp2)
{
   ALLEGRO_LOCKED_REGION *lr1;
   ALLEGRO_LOCKED_REGION *lr2;
   int x, y, w, h;
   double sqerr = 0.0;

   lr1 = al_lock_bitmap(bmp1, ALLEGRO_PIXEL_FORMAT_RGBA_8888,
      ALLEGRO_LOCK_READONLY);
   lr2 = al_lock_bitmap(bmp2, ALLEGRO_PIXEL_FORMAT_RGBA_8888,
      ALLEGRO_LOCK_READONLY);

   w = al_get_bitmap_width(bmp1);
   h = al_get_bitmap_height(bmp1);

   for (y = 0; y < h; y++) {
      char const *data1 = ((char const *)lr1->data) + y*lr1->pitch;
      char const *data2 = ((char const *)lr2->data) + y*lr2->pitch;

      for (x = 0; x < w*4; x++) {
         double err = (double)data1[x] - (double)data2[x];
         sqerr += err*err;
      }
   }

   al_unlock_bitmap(bmp1);
   al_unlock_bitmap(bmp2);

   return sqrt(sqerr / (w*h*4.0));
}

static bool check_similarity(ALLEGRO_CONFIG const *cfg,
   char const *testname,
   ALLEGRO_BITMAP *bmp1, ALLEGRO_BITMAP *bmp2, BmpType bmp_type, bool reliable)
{
   char const *bt = bmp_type_to_string(bmp_type);
   double rms = bitmap_dissimilarity(bmp1, bmp2);
   double tolerance;
   char const *value;
   
   if (bmp_type == HW) {
      char const *exp = al_get_config_value(cfg, testname, "hash_hw");
      char const *sigexp = al_get_config_value(cfg, testname, "sig_hw");
      char hash[16];
      char sig[SIG_LENZ];
      sprintf(hash, "%08x", hash_bitmap(bmp1));
      if (exp && streq(hash, exp)) {
         printf("OK   %s [%s]\n", testname, bt);
         passed_tests++;
         return true;
      }

      if (sigexp && strlen(sigexp) != SIG_LEN) {
         printf("WARNING: ignoring bad signature: %s\n", sigexp);
         sigexp = NULL;
      }

      compute_signature(bmp1, sig);

      if (sigexp && similar_signatures(sig, sigexp)) {
         printf("OK   %s [%s] - by signature\n", testname, bt);
         passed_tests++;
         return true;
      }
   }

   /* The default cutoff is "empirically determined" only. */
   if ((value = al_get_config_value(cfg, testname, "tolerance")))
      tolerance = atof(value);
   else
      tolerance = 17.5;

   if (rms <= tolerance) {
      if (reliable)
         printf("OK   %s [%s]\n", testname, bt);
      else
         printf("OK?  %s [%s]\n", testname, bt);
      passed_tests++;
      return true;
   }
   else {
      char const *exp = al_get_config_value(cfg, testname, "hash");
      char hash[16];
      char sig[SIG_LENZ];
      sprintf(hash, "%08x", hash_bitmap(bmp1));

      if (exp && streq(hash, exp)) {
         printf("OK   %s [%s]\n", testname, bt);
         passed_tests++;
         return true;
      }

      printf("FAIL %s [%s] - RMS error is %g\n", testname, bt, rms);
      printf("hash_hw=%s\n", hash);
      compute_signature(bmp1, sig);
      printf("sig_hw=%s\n", sig);
      failed_tests++;
      return false;
   }
}

static bool do_test(ALLEGRO_CONFIG *cfg, char const *testname,
   ALLEGRO_BITMAP *target, int bmp_type, bool reliable, bool do_check_hash)
{
#define MAXBUF    80

   const char *section = testname;
   int op;
   char const *stmt;
   char buf[MAXBUF];
   char arg[14][MAXBUF];
   char lval[MAXBUF];
   int i;

   if (verbose) {
      /* So in case it segfaults, we know which test to re-run. */
      printf("\nRunning %s [%s].\n", testname, bmp_type_to_string(bmp_type));
      fflush(stdout);
   }

   set_target_reset(target);

   for (op = 0; ; op++) {
      sprintf(buf, "op%d", op);
      stmt = al_get_config_value(cfg, testname, buf);
      if (!stmt) {
         /* Check for a common mistake. */
         sprintf(buf, "op%d", op+1);
         stmt = al_get_config_value(cfg, testname, buf);
         if (!stmt)
            break;
         printf("WARNING: op%d skipped, continuing at op%d\n", op, op+1);
         op++;
      }

      if (verbose > 1)
         printf("# %s\n", stmt);

      if (streq(stmt, ""))
         continue;

      if (SCAN("al_set_target_bitmap", 1)) {
         al_set_target_bitmap(B(0));
         continue;
      }

      if (SCAN("al_set_clipping_rectangle", 4)) {
         al_set_clipping_rectangle(I(0), I(1), I(2), I(3));
         continue;
      }

      if (SCAN("al_set_blender", 3)) {
         al_set_blender(
            get_blender_op(V(0)),
            get_blend_factor(V(1)),
            get_blend_factor(V(2)));
         continue;
      }

      if (SCAN("al_set_separate_blender", 6)) {
         al_set_separate_blender(
            get_blender_op(V(0)),
            get_blend_factor(V(1)),
            get_blend_factor(V(2)),
            get_blender_op(V(3)),
            get_blend_factor(V(4)),
            get_blend_factor(V(5)));
         continue;
      }

      if (SCAN("al_set_bitmap_blender", 3)) {
         al_set_bitmap_blender(
            get_blender_op(V(0)),
            get_blend_factor(V(1)),
            get_blend_factor(V(2)));
         continue;
      }

      if (SCAN0("al_reset_bitmap_blender")) {
         al_reset_bitmap_blender();
         continue;
      }

      if (SCAN("al_set_new_bitmap_flags", 1)) {
         al_set_new_bitmap_flags(get_bitmap_flags(V(0)));
         continue;
      }

      if (SCAN("al_set_new_bitmap_format", 1)) {
         al_set_new_bitmap_format(get_pixel_format(V(0)));
         continue;
      }

      if (SCAN("al_clear_to_color", 1)) {
         al_clear_to_color(C(0));
         continue;
      }

      if (SCANLVAL("al_clone_bitmap", 1)) {
         ALLEGRO_BITMAP **bmp = reserve_local_bitmap(lval, bmp_type);
         (*bmp) = al_clone_bitmap(B(0));
         continue;
      }

      if (SCAN("al_draw_bitmap", 4)) {
         al_draw_bitmap(B(0), F(1), F(2),
            get_draw_bitmap_flag(V(3)));
         continue;
      }

      if (SCAN("al_draw_tinted_bitmap", 5)) {
         al_draw_tinted_bitmap(B(0), C(1), F(2), F(3),
            get_draw_bitmap_flag(V(4)));
         continue;
      }

      if (SCAN("al_draw_bitmap_region", 8)) {
         al_draw_bitmap_region(B(0),
            F(1), F(2), F(3), F(4), F(5), F(6),
            get_draw_bitmap_flag(V(7)));
         continue;
      }

      if (SCAN("al_draw_tinted_bitmap_region", 9)) {
         al_draw_tinted_bitmap_region(B(0), C(1),
            F(2), F(3), F(4), F(5), F(6), F(7),
            get_draw_bitmap_flag(V(8)));
         continue;
      }

      if (SCAN("al_draw_rotated_bitmap", 7)) {
         al_draw_rotated_bitmap(B(0), F(1), F(2), F(3), F(4), F(5),
            get_draw_bitmap_flag(V(6)));
         continue;
      }

      if (SCAN("al_draw_tinted_rotated_bitmap", 8)) {
         al_draw_tinted_rotated_bitmap(B(0), C(1),
            F(2), F(3), F(4), F(5), F(6),
            get_draw_bitmap_flag(V(7)));
         continue;
      }

      if (SCAN("al_draw_scaled_bitmap", 10)) {
         al_draw_scaled_bitmap(B(0),
            F(1), F(2), F(3), F(4), F(5), F(6), F(7), F(8),
            get_draw_bitmap_flag(V(9)));
         continue;
      }

      if (SCAN("al_draw_tinted_scaled_bitmap", 11)) {
         al_draw_tinted_scaled_bitmap(B(0), C(1),
            F(2), F(3), F(4), F(5), F(6), F(7), F(8), F(9),
            get_draw_bitmap_flag(V(10)));
         continue;
      }

      if (SCAN("al_draw_scaled_rotated_bitmap", 9)) {
         al_draw_scaled_rotated_bitmap(B(0),
            F(1), F(2), F(3), F(4), F(5), F(6), F(7),
            get_draw_bitmap_flag(V(8)));
         continue;
      }

      if (SCAN("al_draw_tinted_scaled_rotated_bitmap", 10)) {
         al_draw_tinted_scaled_rotated_bitmap(B(0), C(1),
            F(2), F(3), F(4), F(5), F(6), F(7), F(8),
            get_draw_bitmap_flag(V(9)));
         continue;
      }
      
      if (SCAN("al_draw_tinted_scaled_rotated_bitmap_region", 14)) {
         al_draw_tinted_scaled_rotated_bitmap_region(B(0), F(1), F(2),
            F(3), F(4), C(5),
            F(6), F(7), F(8), F(9), F(10), F(11), F(12),
            get_draw_bitmap_flag(V(13)));
         continue;
      }

      if (SCAN("al_draw_pixel", 3)) {
         al_draw_pixel(F(0), F(1), C(2));
         continue;
      }

      if (SCAN("al_put_pixel", 3)) {
         al_put_pixel(I(0), I(1), C(2));
         continue;
      }

      if (SCAN("al_put_blended_pixel", 3)) {
         al_put_blended_pixel(I(0), I(1), C(2));
         continue;
      }

      if (SCANLVAL("al_create_bitmap", 2)) {
         ALLEGRO_BITMAP **bmp = reserve_local_bitmap(lval, bmp_type);
         (*bmp) = al_create_bitmap(I(0), I(1));
         continue;
      }

      if (SCANLVAL("al_create_sub_bitmap", 5)) {
         ALLEGRO_BITMAP **bmp = reserve_local_bitmap(lval, bmp_type);
         (*bmp) = al_create_sub_bitmap(B(0), I(1), I(2), I(3), I(4));
         continue;
      }

      if (SCANLVAL("al_load_bitmap", 1)) {
         ALLEGRO_BITMAP **bmp = reserve_local_bitmap(lval, bmp_type);
         (*bmp) = load_relative_bitmap(V(0), 0);
         continue;
      }
      if (SCANLVAL("al_load_bitmap_flags", 2)) {
         ALLEGRO_BITMAP **bmp = reserve_local_bitmap(lval, bmp_type);
         (*bmp) = load_relative_bitmap(V(0), get_load_bitmap_flag(V(1)));
         continue;
      }
      if (SCAN("al_save_bitmap", 2)) {
         if (!al_save_bitmap(V(0), B(1))) {
            fatal_error("failed to save %s", V(0));
         }
         continue;
      }
      if (SCANLVAL("al_identify_bitmap", 1)) {
         char const *ext = al_identify_bitmap(V(0));
         if (!ext)
            ext = "NULL";
         al_set_config_value(cfg, testname, lval, ext);
         continue;
      }

      if (SCAN("al_hold_bitmap_drawing", 1)) {
         al_hold_bitmap_drawing(get_bool(V(0)));
         continue;
      }

      /* Transformations */
      if (SCAN("al_copy_transform", 2)) {
         al_copy_transform(get_transform(V(0)), get_transform(V(1)));
         continue;
      }
      if (SCAN("al_use_transform", 1)) {
         al_use_transform(get_transform(V(0)));
         continue;
      }
      if (SCAN("al_build_transform", 6)) {
         al_build_transform(get_transform(V(0)), F(1), F(2), F(3), F(4), F(5));
         continue;
      }
      if (SCAN("al_translate_transform", 3)) {
         al_translate_transform(get_transform(V(0)), F(1), F(2));
         continue;
      }
      if (SCAN("al_scale_transform", 3)) {
         al_scale_transform(get_transform(V(0)), F(1), F(2));
         continue;
      }
      if (SCAN("al_rotate_transform", 2)) {
         al_rotate_transform(get_transform(V(0)), F(1));
         continue;
      }
      if (SCAN("al_compose_transform", 2)) {
         al_compose_transform(get_transform(V(0)), get_transform(V(1)));
         continue;
      }

      /* Conversion */
      if (SCAN("al_convert_bitmap", 1)) {
         ALLEGRO_BITMAP *bmp = B(0);
         al_convert_bitmap(bmp);
         continue;
      }

      /* Locking */
      if (SCAN("al_lock_bitmap", 3)) {
         ALLEGRO_BITMAP *bmp = B(0);
         lock_region.x = 0;
         lock_region.y = 0;
         lock_region.w = al_get_bitmap_width(bmp);
         lock_region.h = al_get_bitmap_height(bmp);
         lock_region.lr = al_lock_bitmap(bmp,
            get_pixel_format(V(1)),
            get_lock_bitmap_flags(V(2)));
         continue;
      }
      if (SCAN("al_lock_bitmap_region", 7)) {
         ALLEGRO_BITMAP *bmp = B(0);
         lock_region.x = I(1);
         lock_region.y = I(2);
         lock_region.w = I(3);
         lock_region.h = I(4);
         lock_region.lr = al_lock_bitmap_region(bmp,
            lock_region.x, lock_region.y,
            lock_region.w, lock_region.h,
            get_pixel_format(V(5)),
            get_lock_bitmap_flags(V(6)));
         continue;
      }
      if (SCAN("al_unlock_bitmap", 1)) {
         al_unlock_bitmap(B(0));
         lock_region.lr = NULL;
         continue;
      }
      if (SCAN("fill_lock_region", 2)) {
         fill_lock_region(&lock_region, F(0), get_bool(V(1)));
         continue;
      }

      /* Fonts */
      if (SCAN("al_draw_text", 6)) {
         al_draw_text(get_font(V(0)), C(1), F(2), F(3), get_font_align(V(4)),
            V(5));
         continue;
      }
      if (SCAN("al_draw_justified_text", 8)) {
         al_draw_justified_text(get_font(V(0)), C(1), F(2), F(3), F(4), F(5),
            get_font_align(V(6)), V(7));
         continue;
      }
      if (SCANLVAL("al_get_text_width", 2)) {
         int w = al_get_text_width(get_font(V(0)), V(1));
         set_config_int(cfg, testname, lval, w);
         continue;
      }
      if (SCANLVAL("al_get_font_line_height", 1)) {
         int h = al_get_font_line_height(get_font(V(0)));
         set_config_int(cfg, testname, lval, h);
         continue;
      }
      if (SCANLVAL("al_get_font_ascent", 1)) {
         int as = al_get_font_ascent(get_font(V(0)));
         set_config_int(cfg, testname, lval, as);
         continue;
      }
      if (SCANLVAL("al_get_font_descent", 1)) {
         int de = al_get_font_descent(get_font(V(0)));
         set_config_int(cfg, testname, lval, de);
         continue;
      }
      if (SCAN("al_get_text_dimensions", 6)) {
         int bbx, bby, bbw, bbh;
         al_get_text_dimensions(get_font(V(0)), V(1), &bbx, &bby, &bbw, &bbh);
         set_config_int(cfg, testname, V(2), bbx);
         set_config_int(cfg, testname, V(3), bby);
         set_config_int(cfg, testname, V(4), bbw);
         set_config_int(cfg, testname, V(5), bbh);
         continue;
      }
      if (SCAN("al_set_fallback_font", 2)) {
         al_set_fallback_font(get_font(V(0)), get_font(V(1)));
         continue;
      }

      /* Primitives */
      if (SCAN("al_draw_line", 6)) {
         al_draw_line(F(0), F(1), F(2), F(3), C(4), F(5));
         continue;
      }
      if (SCAN("al_draw_triangle", 8)) {
         al_draw_triangle(F(0), F(1), F(2), F(3), F(4), F(5),
            C(6), F(7));
         continue;
      }
      if (SCAN("al_draw_filled_triangle", 7)) {
         al_draw_filled_triangle(F(0), F(1), F(2), F(3), F(4), F(5), C(6));
         continue;
      }
      if (SCAN("al_draw_rectangle", 6)) {
         al_draw_rectangle(F(0), F(1), F(2), F(3), C(4), F(5));
         continue;
      }
      if (SCAN("al_draw_filled_rectangle", 5)) {
         al_draw_filled_rectangle(F(0), F(1), F(2), F(3), C(4));
         continue;
      }
      if (SCAN("al_draw_rounded_rectangle", 8)) {
         al_draw_rounded_rectangle(F(0), F(1), F(2), F(3), F(4), F(5), C(6),
            F(7));
         continue;
      }
      if (SCAN("al_draw_filled_rounded_rectangle", 7)) {
         al_draw_filled_rounded_rectangle(F(0), F(1), F(2), F(3), F(4), F(5),
            C(6));
         continue;
      }
      if (SCAN("al_draw_pieslice", 7)) {
         al_draw_pieslice(F(0), F(1), F(2), F(3), F(4), C(5), F(6));
         continue;
      }
      if (SCAN("al_draw_filled_pieslice", 6)) {
         al_draw_filled_pieslice(F(0), F(1), F(2), F(3), F(4), C(5));
         continue;
      }
      if (SCAN("al_draw_ellipse", 6)) {
         al_draw_ellipse(F(0), F(1), F(2), F(3), C(4), F(5));
         continue;
      }
      if (SCAN("al_draw_filled_ellipse", 5)) {
         al_draw_filled_ellipse(F(0), F(1), F(2), F(3), C(4));
         continue;
      }
      if (SCAN("al_draw_circle", 5)) {
         al_draw_circle(F(0), F(1), F(2), C(3), F(4));
         continue;
      }
      if (SCAN("al_draw_filled_circle", 4)) {
         al_draw_filled_circle(F(0), F(1), F(2), C(3));
         continue;
      }
      if (SCAN("al_draw_arc", 7)) {
         al_draw_arc(F(0), F(1), F(2), F(3), F(4), C(5), F(6));
         continue;
      }
      if (SCAN("al_draw_elliptical_arc", 8)) {
         al_draw_elliptical_arc(F(0), F(1), F(2), F(3), F(4), F(5), C(6), F(7));
         continue;
      }
      if (SCAN("al_draw_spline", 3)) {
         float pt[8];
         if (sscanf(V(0), "%f, %f, %f, %f, %f, %f, %f, %f",
               pt+0, pt+1, pt+2, pt+3, pt+4, pt+5, pt+6, pt+7) == 8) {
            al_draw_spline(pt, C(1), F(2));
            continue;
         }
      }
      if (SCAN("al_draw_prim", 6)) {
         fill_vertices(cfg, V(0));
         /* decl arg is ignored */
         al_draw_prim(vertices, NULL, B(2), I(3), I(4), get_prim_type(V(5)));
         continue;
      }

      /* Keep 5.0 and 5.1 functions separate for easier merging. */

      /* Primitives (5.1) */
      if (SCAN("al_draw_polyline", 6)) {
         fill_simple_vertices(cfg, V(0));
         al_draw_polyline(simple_vertices, 2 * sizeof(float),
            num_simple_vertices, get_line_join(V(1)), get_line_cap(V(2)),
            C(3), F(4), F(5));
         continue;
      }
      if (SCAN("al_draw_polygon", 5)) {
         fill_simple_vertices(cfg, V(0));
         al_draw_polygon(simple_vertices, num_simple_vertices,
            get_line_join(V(1)), C(2), F(3), F(4));
         continue;
      }
      if (SCAN("al_draw_filled_polygon", 2)) {
         fill_simple_vertices(cfg, V(0));
         al_draw_filled_polygon(simple_vertices, num_simple_vertices, C(1));
         continue;
      }
      if (SCAN("al_draw_filled_polygon_with_holes", 3)) {
         fill_simple_vertices(cfg, V(0));
         fill_vertex_counts(cfg, V(1));
         al_draw_filled_polygon_with_holes(simple_vertices, vertex_counts, C(2));
         continue;
      }

      /* Transformations (5.1) */
      if (SCAN("al_horizontal_shear_transform", 2)) {
         al_horizontal_shear_transform(get_transform(V(0)), F(1));
         continue;
      }
      if (SCAN("al_vertical_shear_transform", 2)) {
         al_vertical_shear_transform(get_transform(V(0)), F(1));
         continue;
      }
      if (SCAN("al_orthographic_transform", 7)) {
         al_orthographic_transform(get_transform(V(0)), F(1), F(2), F(3), F(4), F(5), F(6));
         continue;
      }
      if (SCAN("al_use_projection_transform", 1)) {
         al_use_projection_transform(get_transform(V(0)));
         continue;
      }
      if (SCAN("al_set_blend_color", 1)) {
         al_set_blend_color(C(0));
         continue;
      }

      /* Simple arithmetic, generally useful. (5.1) */
      if (SCANLVAL("isum", 2)) {
         int result  = I(0) + I(1);
         set_config_int(cfg, testname, lval, result);
         continue;
      }
      if (SCANLVAL("idif", 2)) {
         int result  = I(0) - I(1);
         set_config_int(cfg, testname, lval, result);
         continue;
      }
      if (SCANLVAL("imul", 2)) {
         int result  = I(0) * I(1);
         set_config_int(cfg, testname, lval, result);
         continue;
      }
      if (SCANLVAL("idiv", 2)) {
         int result  = I(0) / I(1);
         set_config_int(cfg, testname, lval, result);
         continue;
      }
      if (SCANLVAL("fsum", 2)) {
         float result  = F(0) + F(1);
         set_config_float(cfg, testname, lval, result);
         continue;
      }
      if (SCANLVAL("fdif", 2)) {
         float result  = F(0) - F(1);
         set_config_float(cfg, testname, lval, result);
         continue;
      }
      if (SCANLVAL("fmul", 2)) {
         float result  = F(0) * F(1);
         set_config_float(cfg, testname, lval, result);
         continue;
      }
      if (SCANLVAL("fdiv", 2)) {
         float result  = F(0) / F(1);
         set_config_float(cfg, testname, lval, result);
         continue;
      }
      if (SCANLVAL("round", 1)) {
         int result  = round(F(0));
         set_config_int(cfg, testname, lval, result);
         continue;
      }

      /* Dynamical variable initialisation, needed to properly initialize
       * variables (5.1)*/
      if (SCANLVAL("int", 1)) {
         float result  = F(0);
         set_config_float(cfg, testname, lval, result);
         continue;
      }
      if (SCANLVAL("float", 1)) {
         float result  = F(0);
         set_config_float(cfg, testname, lval, result);
         continue;
      }
       
      /* Fonts: per glyph text handling (5.1)  */
      if (SCAN("al_draw_glyph", 5)) {
         al_draw_glyph(get_font(V(0)), C(1), F(2), F(3), I(4));
         continue;
      }
      if (SCANLVAL("al_get_glyph_advance", 3)) {
         int kerning = al_get_glyph_advance(get_font(V(0)), I(1), I(2));
         set_config_int(cfg, testname, lval, kerning);
         continue;
      }
      if (SCANLVAL("al_get_glyph_width", 2)) {
         int w = al_get_glyph_width(get_font(V(0)), I(1));
         set_config_int(cfg, testname, lval, w);
         continue;
      }
      if (SCANLVAL("al_get_glyph_dimensions", 6)) {
         int bbx, bby, bbw, bbh;
         bool ok = al_get_glyph_dimensions(get_font(V(0)), I(1), &bbx, &bby, &bbw, &bbh);
         set_config_int(cfg, testname, V(2), bbx);
         set_config_int(cfg, testname, V(3), bby);
         set_config_int(cfg, testname, V(4), bbw);
         set_config_int(cfg, testname, V(5), bbh);
         set_config_int(cfg, testname, lval, ok);
         continue;
      }

      if (SCANLVAL("al_color_distance_ciede2000", 2)) {
         float d = al_color_distance_ciede2000(C(0), C(1));
         set_config_float(cfg, testname, lval, d);
         continue;
      }
      if (SCANLVAL("al_color_lab", 3)) {
         ALLEGRO_COLOR rgb = al_color_lab(F(0), F(1), F(2));
         char hex[100];
         sprintf(hex, "%f/%f/%f", rgb.r, rgb.g, rgb.b);
         al_set_config_value(cfg, testname, lval, hex);
         continue;
      }

      fatal_error("statement didn't scan: %s", stmt);
   }

   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);

   bool good;
   if (do_check_hash) {
      good = check_hash(cfg, testname, target, bmp_type);
   }
   else {
      good = check_similarity(cfg, testname, target, membuf, bmp_type, reliable);
   }

   total_tests++;

   if (save_outputs && (!save_on_failure || !good)) {
      ALLEGRO_USTR *filename = al_ustr_newf("%s [%s].png", testname,
         bmp_type_to_string(bmp_type));
      al_save_bitmap(al_cstr(filename), target);
      al_ustr_free(filename);
   }

   if (!quiet) {
      if (target != al_get_backbuffer(display)) {
         set_target_reset(al_get_backbuffer(display));
         al_draw_bitmap(target, 0, 0, 0);
      }

      al_flip_display();
      al_rest(delay);
   }

   /* Ensure we don't target a bitmap which is about to be destroyed. */
   al_set_target_bitmap(display ? al_get_backbuffer(display) : NULL);

   /* Destroy local bitmaps. */
   for (i = num_global_bitmaps; i < MAX_BITMAPS; i++) {
      if (bitmaps[i].name) {
         al_ustr_free(bitmaps[i].name);
         bitmaps[i].name = NULL;
         al_destroy_bitmap(bitmaps[i].bitmap[bmp_type]);
         bitmaps[i].bitmap[bmp_type] = NULL;
      }
   }

   /* Free transform names. */
   for (i = 0; i < MAX_TRANS; i++) {
      al_ustr_free(transforms[i].name);
      transforms[i].name = NULL;
   }

   return good;
#undef MAXBUF
}

static void sw_hw_test(ALLEGRO_CONFIG *cfg, char const *testname)
{
   bool reliable = true;
   char const *hw_only_str = al_get_config_value(cfg, testname, "hw_only");
   char const *sw_only_str = al_get_config_value(cfg, testname, "sw_only");
   char const *skip_on_xvfb_str = al_get_config_value(cfg, testname, "skip_on_xvfb");
   bool hw_only = hw_only_str && get_bool(hw_only_str);
   bool sw_only = sw_only_str && get_bool(sw_only_str);
   bool skip_on_xvfb = skip_on_xvfb_str && get_bool(skip_on_xvfb_str);

   if (skip_on_xvfb && on_xvfb) {
      skipped_tests++;
      return;
   }

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   if (!hw_only) {
      reliable = do_test(cfg, testname, membuf, SW, true, true);
   }

   if (sw_only) return;

   if (display) {
      al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
      do_test(cfg, testname, al_get_backbuffer(display), HW, reliable, hw_only);
   } else if (hw_only) {
      printf("WARNING: Skipping hardware-only test due to the --no-display flag: %s\n",
             testname);
      skipped_tests++;
   }
}

static bool section_exists(ALLEGRO_CONFIG const *cfg, char const *section)
{
   ALLEGRO_CONFIG_ENTRY *iter;

   return al_get_first_config_entry(cfg, section, &iter) != NULL;
}

static void merge_config_sections(
   ALLEGRO_CONFIG *targ_cfg, char const *targ_section,
   ALLEGRO_CONFIG const *src_cfg, char const *src_section)
{
   char const *key;
   char const *value;
   ALLEGRO_CONFIG_ENTRY *iter;

   value = al_get_config_value(src_cfg, src_section, "extend");
   if (value) {
      if (streq(value, src_section)) {
         fatal_error("section cannot extend itself: %s "
            "(did you forget to rename a section?)", src_section);
      }
      merge_config_sections(targ_cfg, targ_section, src_cfg, value);
   }

   key = al_get_first_config_entry(src_cfg, src_section, &iter);
   if (!key) {
      fatal_error("missing section: %s", src_section);
   }
   for (; key != NULL; key = al_get_next_config_entry(&iter)) {
      value = al_get_config_value(src_cfg, src_section, key);
      al_set_config_value(targ_cfg, targ_section, key, value);
   }
}

static void run_test(ALLEGRO_CONFIG const *cfg, char const *section)
{
   char const *extend;
   ALLEGRO_CONFIG *cfg2;

   if (!section_exists(cfg, section)) {
      fatal_error("section not found: %s", section);
   }

   cfg2 = al_create_config();
   al_merge_config_into(cfg2, cfg);
   extend = al_get_config_value(cfg, section, "extend");
   if (extend) {
      merge_config_sections(cfg2, section, cfg, section);
   }
   sw_hw_test(cfg2, section);
   al_destroy_config(cfg2);
}

static void run_matching_tests(ALLEGRO_CONFIG const *cfg, const char *prefix)
{
   ALLEGRO_CONFIG_SECTION *iter;
   char const *section;

   for (section = al_get_first_config_section(cfg, &iter);
         section != NULL;
         section = al_get_next_config_section(&iter)) {
      if (0 == strncmp(section, prefix, strlen(prefix))) {
         run_test(cfg, section);
      }
   }
}

static void partial_tests(ALLEGRO_CONFIG const *cfg, int n)
{
   ALLEGRO_USTR *name = al_ustr_new("");

   while (n > 0) {
      /* Automatically prepend "test" for convenience. */
      if (0 == strncmp(argv[0], "test ", 5)) {
         al_ustr_assign_cstr(name, argv[0]);
      }
      else {
         al_ustr_truncate(name, 0);
         al_ustr_appendf(name, "test %s", argv[0]);
      }

      /* Star suffix means run all matching tests. */
      if (al_ustr_has_suffix_cstr(name, "*")) {
         al_ustr_truncate(name, al_ustr_size(name) - 1);
         run_matching_tests(cfg, al_cstr(name));
      }
      else {
         run_test(cfg, al_cstr(name));
      }

      argc--;
      argv++;
      n--;
   }

   al_ustr_free(name);
}

static bool has_suffix(char const *s, char const *suf)
{
   return (strlen(s) >= strlen(suf))
      && streq(s + strlen(s) - strlen(suf), suf);
}

static void process_ini_files(void)
{
   ALLEGRO_CONFIG *cfg;
   int n;

   while (argc > 0) {
      if (!has_suffix(argv[0], ".ini"))
         fatal_error("expected .ini argument: %s\n", argv[0]);
      cfg = al_load_config_file(argv[0]);
      if (!cfg)
         fatal_error("failed to load config file %s", argv[0]);

      if (verbose)
         printf("Running %s\n", argv[0]);

      argc--;
      argv++;

      al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
      load_bitmaps(cfg, "bitmaps", SW, ALLEGRO_NO_PREMULTIPLIED_ALPHA);

      if (display) {
         al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
         load_bitmaps(cfg, "bitmaps", HW, ALLEGRO_NO_PREMULTIPLIED_ALPHA);
      }

      load_fonts(cfg, "fonts");

      for (n = 0; n < argc; n++) {
         if (has_suffix(argv[n], ".ini"))
            break;
      }

      if (n == 0)
         run_matching_tests(cfg, "test ");
      else
         partial_tests(cfg, n);

      unload_data();

      al_destroy_config(cfg);
   }
}

const char* help_str =
" [OPTION] CONFIG_FILE [TEST_NAME]... [CONFIG_FILE [TEST_NAME]...]...\n"
"\n"
"Run Allegro graphical output tests within one or more CONFIG_FILEs (each\n"
"having an .ini extension). By default this program runs all the tests in a\n"
"file, but individual TEST_NAMEs can be specified after each CONFIG_FILE.\n"
"\n"
"Options:\n"
" -d, --delay           duration (in sec) to wait between tests\n"
" --force-d3d           force using D3D (Windows only)\n"
" --force-opengl-1.2    force using OpenGL 1.2\n"
" --force-opengl-2.0    force using OpenGL 2.0\n"
" --force-opengl        force using OpenGL\n"
" -f, --save_on_failure save the output of failred tests in the current directory\n"
" -h, --help            display this message\n"
" -n, --no-display      do not create a display (hardware drawing is disabled)\n"
" -s, --save            save the output of each test in the current directory\n"
" --use-shaders         use the programmable pipeline for drawing\n"
" -v, --verbose         show additional information after each test\n"
" -q, --quiet           do not draw test output to the display\n"
" --xvfb                indicates that we're running on XVFB, skipping tests if necessary\n";

int main(int _argc, char *_argv[])
{
   int display_flags = 0;

   argc = _argc;
   argv = _argv;

   if (argc == 1) {
      fatal_error("requires config file argument.\nSee --help for usage");
   }
   argc--;
   argv++;

   if (!al_init()) {
      fatal_error("failed to initialise Allegro");
   }
   al_init_image_addon();
   al_init_font_addon();
   al_init_ttf_addon();
   al_init_primitives_addon();

   for (; argc > 0; argc--, argv++) {
      char const *opt = argv[0];
      if (streq(opt, "-d") || streq(opt, "--delay")) {
         delay = 1.0;
      }
      else if (streq(opt, "-s") || streq(opt, "--save")) {
         save_outputs = true;
      }
      else if (streq(opt, "-f") || streq(opt, "--save_on_failure")) {
         save_on_failure = true;
         save_outputs = true;
      }
      else if (streq(opt, "--xvfb")) {
         on_xvfb = true;
      }
      else if (streq(opt, "-q") || streq(opt, "--quiet")) {
         quiet = true;
      }
      else if (streq(opt, "-n") || streq(opt, "--no-display")) {
         want_display = false;
         quiet = true;
      }
      else if (streq(opt, "-v") || streq(opt, "--verbose")) {
         verbose++;
      }
      else if (streq(opt, "--force-opengl-1.2")) {
         ALLEGRO_CONFIG *cfg = al_get_system_config();
         al_set_config_value(cfg, "opengl", "force_opengl_version", "1.2");
         display_flags |= ALLEGRO_OPENGL;
      }
      else if (streq(opt, "--force-opengl-2.0")) {
         ALLEGRO_CONFIG *cfg = al_get_system_config();
         al_set_config_value(cfg, "opengl", "force_opengl_version", "2.0");
         display_flags |= ALLEGRO_OPENGL;
      }
      else if (streq(opt, "--force-opengl")) {
         display_flags |= ALLEGRO_OPENGL;
      }
      else if (streq(opt, "--force-d3d")) {
         /* Don't try this at home. */
         display_flags |= ALLEGRO_DIRECT3D_INTERNAL;
      }
      else if (streq(opt, "--use-shaders")) {
         display_flags |= ALLEGRO_PROGRAMMABLE_PIPELINE;
      }
      else if (streq(opt, "-h") || streq(opt, "--help")) {
         printf("Usage:\n%s%s", _argv[0], help_str);
         return 0;
      }
      else {
         break;
      }
   }

   if (want_display) {
      al_set_new_display_flags(display_flags);
      display = al_create_display(640, 480);
      if (!display) {
         fatal_error("failed to create display");
      }
   }

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);

   if (want_display) {
      membuf = al_create_bitmap(
         al_get_display_width(display),
         al_get_display_height(display));
   }
   else {
      membuf = al_create_bitmap(640, 480);
   }

   process_ini_files();

   printf("\n");
   printf("total tests:  %d\n", total_tests);
   printf("passed tests: %d\n", passed_tests);
   printf("failed tests: %d\n", failed_tests);
   printf("skipped tests: %d\n", skipped_tests);
   printf("\n");

   return !!failed_tests;
}

/* vim: set sts=3 sw=3 et: */
