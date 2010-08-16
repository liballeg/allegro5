#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_image.h>

#define MAX_BITMAPS  8

typedef struct {
   ALLEGRO_USTR   *name;
   ALLEGRO_BITMAP *bitmap;
} Bitmap;

ALLEGRO_DISPLAY   *display;
Bitmap            bitmaps[MAX_BITMAPS];
float             delay = 0.0;
bool              verbose = false;
int               total_tests = 0;
int               passed_tests = 0;
int               failed_tests = 0;

#define streq(a, b)  (0 == strcmp((a), (b)))

void error(char const *msg, ...)
{
   va_list ap;

   va_start(ap, msg);
   fprintf(stderr, "test_driver: ");
   vfprintf(stderr, msg, ap);
   fprintf(stderr, "\n");
   va_end(ap);
   exit(EXIT_FAILURE);
}

void load_bitmaps(ALLEGRO_CONFIG const *cfg, const char *section)
{
   int i = 0;
   void *iter;
   char const *key;
   char const *value;
   ALLEGRO_PATH *path;
   ALLEGRO_PATH *tail;
   char const *path_str;

   key = al_get_first_config_entry(cfg, section, &iter);
   while (key && i < MAX_BITMAPS) {
      value = al_get_config_value(cfg, section, key);

      path = al_get_standard_path(ALLEGRO_PROGRAM_PATH);
      tail = al_create_path(value);
      al_join_paths(path, tail);
      path_str = al_path_cstr(path, '/');

      bitmaps[i].name = al_ustr_new(key);
      bitmaps[i].bitmap = al_load_bitmap(path_str);
      if (!bitmaps[i].bitmap) {
         error("failed to load %s", path_str);
      }

      al_destroy_path(path);
      al_destroy_path(tail);

      key = al_get_next_config_entry(&iter);
      i++;
   }

   if (i == MAX_BITMAPS)
      error("bitmap limit reached");
}

char const *resolve_var(ALLEGRO_CONFIG const *cfg, char const *section,
   char const *v)
{
   char const *vv = al_get_config_value(cfg, section, v);
   return (vv) ? vv : v;
}

ALLEGRO_COLOR get_color(char const *value)
{
   int r, g, b, a;

   if (sscanf(value, "#%02x%02x%02x%02x", &r, &g, &b, &a) == 4)
      return al_map_rgba(r, g, b, a);
   if (sscanf(value, "#%02x%02x%02x", &r, &g, &b) == 3)
      return al_map_rgb(r, g, b);
   return al_color_name(value);
}

ALLEGRO_BITMAP *get_bitmap(char const *value)
{
   int i;

   for (i = 0; i < MAX_BITMAPS; i++) {
      if (bitmaps[i].name && streq(al_cstr(bitmaps[i].name), value))
         return bitmaps[i].bitmap;
   }

   return NULL;
}

int get_draw_bitmap_flag(char const *value)
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

int get_blender_op(char const *value)
{
   return streq(value, "ALLEGRO_ADD") ? ALLEGRO_ADD
      : streq(value, "ALLEGRO_DEST_MINUS_SRC") ? ALLEGRO_DEST_MINUS_SRC
      : streq(value, "ALLEGRO_SRC_MINUS_DEST") ? ALLEGRO_SRC_MINUS_DEST
      : atoi(value);
}

int get_blend_factor(char const *value)
{
   return streq(value, "ALLEGRO_ZERO") ? ALLEGRO_ZERO
      : streq(value, "ALLEGRO_ONE") ? ALLEGRO_ONE
      : streq(value, "ALLEGRO_ALPHA") ? ALLEGRO_ALPHA
      : streq(value, "ALLEGRO_INVERSE_ALPHA") ? ALLEGRO_INVERSE_ALPHA
      : atoi(value);
}

uint32_t hash_bitmap(ALLEGRO_BITMAP *bmp)
{
   /* FNV-1a algorithm, parameters from:
    * http://www.isthe.com/chongo/tech/comp/fnv/index.html
    */
   ALLEGRO_LOCKED_REGION *lr;
   int x, y, w, h;
   uint32_t hash;

   w = al_get_bitmap_width(bmp);
   h = al_get_bitmap_height(bmp);
   hash = 2166136261UL;

   lr = al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_RGBA_8888,
      ALLEGRO_LOCK_READONLY);

   for (y = 0; y < h; y++) {
      char const *data = ((char const *)lr->data) + y*lr->pitch;
      for (x = 0; x < w*4; x++) {
         hash ^= data[x];
         hash *= 16777619;
      }
   }

   al_unlock_bitmap(bmp);

   return hash;
}

void check_hash(ALLEGRO_CONFIG const *cfg, char const *testname,
   ALLEGRO_BITMAP *bmp)
{
   char hash[16];
   char const *exp;

   sprintf(hash, "%08x", hash_bitmap(bmp));

   exp = al_get_config_value(cfg, testname, "hash");
   if (!exp) {
      printf("NEW  %s - hash is %s\n", testname, hash);
   }
   else if (streq(hash, exp)) {
      printf("OK   %s\n", testname);
      passed_tests++;
   }
   else {
      printf("FAIL %s - hash is %s; expected %s\n", testname, hash, exp);
      failed_tests++;
   }

   total_tests++;
}

void do_test(ALLEGRO_CONFIG const *cfg, char const *testname)
{
#define MAXBUF    80
#define PAT1      " %80[A-Za-z0-9_-.$|#] "
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
#define V(a)      resolve_var(cfg, testname, arg[(a)])
#define I(a)      atoi(V(a))
#define F(a)      atof(V(a))
#define C(a)      get_color(V(a))
#define B(a)      get_bitmap(V(a))
#define SCAN(fn, arity) \
                  (sscanf(stmt, fn " (" PAT##arity " )", ARGS##arity) == arity)

   int op;
   char const *stmt;
   char buf[MAXBUF];
   char arg[10][MAXBUF];

   /* Reset to defaults. */
   {
      ALLEGRO_BITMAP *target = al_get_backbuffer(display);
      ALLEGRO_TRANSFORM ident;

      /* Maybe we should be drawing to a memory buffer? */
      al_set_target_bitmap(target);
      al_clear_to_color(al_map_rgb(0, 0, 0));
      al_set_clipping_rectangle(0, 0,
         al_get_bitmap_width(target),
         al_get_bitmap_height(target));
      al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
      al_identity_transform(&ident);
      al_use_transform(&ident);
   }

   for (op = 0; ; op++) {
      sprintf(buf, "op%d", op);
      stmt = al_get_config_value(cfg, testname, buf);
      if (!stmt)
         break;

      if (verbose)
         printf("# %s\n", stmt);

      if (streq(stmt, "nop"))
         continue;

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

      if (SCAN("al_clear_to_color", 1)) {
         al_clear_to_color(C(0));
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
            get_draw_bitmap_flag(V(9)));
         continue;
      }

      if (SCAN("al_draw_pixel", 3)) {
         al_draw_pixel(I(0), I(1), C(2));
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

      error("statement didn't scan: %s", stmt);
   }

   check_hash(cfg, testname, al_get_target_bitmap());

   al_flip_display();
   al_rest(delay);

#undef B
#undef C
#undef F
#undef I
#undef SCAN
}

static bool section_exists(ALLEGRO_CONFIG const *cfg, char const *section)
{
   void *iter;

   return al_get_first_config_entry(cfg, section, &iter);
}

static void merge_config_sections(
   ALLEGRO_CONFIG *targ_cfg, char const *targ_section,
   ALLEGRO_CONFIG const *src_cfg, char const *src_section)
{
   char const *key;
   char const *value;
   void *iter;

   value = al_get_config_value(src_cfg, src_section, "extend");
   if (value) {
      if (streq(value, src_section)) {
         error("section cannot extend itself: %s "
            "(did you forget to rename a section?)", src_section);
      }
      merge_config_sections(targ_cfg, targ_section, src_cfg, value);
   }

   key = al_get_first_config_entry(src_cfg, src_section, &iter);
   if (!key) {
      error("missing section: %s", src_section);
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
      error("section not found: %s", section);
   }

   extend = al_get_config_value(cfg, section, "extend");
   if (!extend) {
      do_test(cfg, section);
   }
   else {
      cfg2 = al_create_config();
      merge_config_sections(cfg2, section, cfg, section);
      do_test(cfg2, section);
      al_destroy_config(cfg2);
   }
}

static void run_matching_tests(ALLEGRO_CONFIG const *cfg, const char *prefix)
{
   void *iter;
   char const *section;

   for (section = al_get_first_config_section(cfg, &iter);
         section != NULL;
         section = al_get_next_config_section(&iter)) {
      if (0 == strncmp(section, prefix, strlen(prefix))) {
         run_test(cfg, section);
      }
   }
}

static void partial_tests(ALLEGRO_CONFIG const *cfg,
   int argc, char const *argv[])
{
   ALLEGRO_USTR *name = al_ustr_new("");

   for (; argc > 0; argc--, argv++) {
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
   }

   al_ustr_free(name);
}

int main(int argc, char const *argv[])
{
   ALLEGRO_CONFIG *cfg;

   if (argc == 1) {
      error("requires config file argument");
   }
   argc--;
   argv++;

   if (!al_init()) {
      error("failed to initialise Allegro");
   }
   al_init_image_addon();

   for (; argc > 0; argc--, argv++) {
      if (streq(argv[0], "-d") || streq(argv[0], "--delay")) {
         delay = 1.0;
      }
      else if (streq(argv[0], "-v") || streq(argv[0], "--verbose")) {
         verbose = true;
      }
      else {
         break;
      }
   }

   cfg = al_load_config_file(argv[0]);
   if (!cfg)
      error("failed to load config file %s", argv[0]);
   argc--;
   argv++;

   display = al_create_display(640, 480);
   if (!display) {
      error("failed to create display");
   }

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);

   load_bitmaps(cfg, "bitmaps");

   if (argc == 0)
      run_matching_tests(cfg, "test ");
   else
      partial_tests(cfg, argc, argv);

   al_destroy_config(cfg);

   printf("\n");
   printf("total tests:  %d\n", total_tests);
   printf("passed tests: %d\n", passed_tests);
   printf("failed tests: %d\n", failed_tests);

   return !!failed_tests;
}

/* vim: set sts=3 sw=3 et: */
