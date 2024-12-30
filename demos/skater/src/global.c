#include <allegro5/allegro.h>
#include <stdio.h>
#include <ctype.h>
#include "background_scroller.h"
#include "demodata.h"
#include "game.h"
#include "global.h"
#include "music.h"
#include "framework.h"

/* Default values of some config varables */
#ifdef ALLEGRO_IPHONE
int fullscreen = 1;
int controller_id = 1;
#else
int fullscreen = 0;
int controller_id = 0;
#endif
int bit_depth = 32;
int screen_width = 640;
int screen_height = 480;
int screen_orientation = ALLEGRO_DISPLAY_ORIENTATION_0_DEGREES;
int window_width = 640;
int window_height = 480;
int screen_samples = 1;
int use_vsync = 0;
int logic_framerate = 100;
int max_frame_skip = 5;
int limit_framerate = 1;
int display_framerate = 1;
int reduce_cpu_usage = 1;
int sound_volume = 8;
int music_volume = 8;

int shadow_offset = 2;

VCONTROLLER *controller[2];

char config_path[DEMO_PATH_LENGTH + 1];
char data_path[DEMO_PATH_LENGTH + 1];
DATA_ENTRY *demo_data;

ALLEGRO_DISPLAY *screen;

char *GameError;

int load_data(void);

const char *demo_error(int id)
{
   switch (id) {
      case DEMO_ERROR_ALLEGRO:
         return "Allegro error";
      case DEMO_ERROR_GFX:
         return "can't find suitable screen update driver";
      case DEMO_ERROR_MEMORY:
         return "ran out of memory";
      case DEMO_ERROR_VIDEOMEMORY:
         return "not enough VRAM";
      case DEMO_ERROR_TRIPLEBUFFER:
         return "triple buffering not supported";
      case DEMO_ERROR_DATA:
         return "can't load menu data";
      case DEMO_ERROR_GAMEDATA:
         return GameError;
      case DEMO_OK:
         return "OK";
      default:
         return "unknown";
   };
}


int get_config_int(const ALLEGRO_CONFIG *cfg, const char *section,
                   const char *name, int def)
{
   const char *v = al_get_config_value(cfg, section, name);

   return (v) ? atoi(v) : def;
}


void set_config_int(ALLEGRO_CONFIG *cfg, const char *section, const char *name,
                    int val)
{
   char buf[32];

   sprintf(buf, "%d", val);
   al_set_config_value(cfg, section, name, buf);
}


/* read_config:
 * Load settings from the configuration file, providing default values
 */
void read_global_config(const char *config)
{
   ALLEGRO_CONFIG *c = al_load_config_file(config);
   if (!c) c = al_create_config();

   fullscreen = get_config_int(c, "GFX", "fullscreen", fullscreen);
   bit_depth = get_config_int(c, "GFX", "bit_depth", bit_depth);
   screen_width = get_config_int(c, "GFX", "screen_width", screen_width);
   screen_height = get_config_int(c, "GFX", "screen_height", screen_height);
   window_width = get_config_int(c, "GFX", "window_width", window_height);
   window_height = get_config_int(c, "GFX", "window_height", screen_height);
   screen_samples = get_config_int(c, "GFX", "samples", screen_samples);
   use_vsync = get_config_int(c, "GFX", "vsync", use_vsync);

   logic_framerate =
      get_config_int(c, "TIMING", "logic_framerate", logic_framerate);
   limit_framerate =
      get_config_int(c, "TIMING", "limit_framerate", limit_framerate);
   max_frame_skip =
      get_config_int(c, "TIMING", "max_frame_skip", max_frame_skip);
   display_framerate =
      get_config_int(c, "TIMING", "display_framerate", display_framerate);
   reduce_cpu_usage =
      get_config_int(c, "TIMING", "reduce_cpu_usage", reduce_cpu_usage);

   sound_volume = get_config_int(c, "SOUND", "sound_volume", sound_volume);
   music_volume = get_config_int(c, "SOUND", "music_volume", music_volume);

   set_sound_volume(sound_volume / 10.0);
   set_music_volume(music_volume / 10.0);

   controller_id = get_config_int(c, "CONTROLS", "controller_id", controller_id);

   al_destroy_config(c);
}


void write_global_config(const char *config)
{
   ALLEGRO_CONFIG *c = al_load_config_file(config);
   if (!c) c = al_create_config();

   set_config_int(c, "GFX", "fullscreen", fullscreen);
   set_config_int(c, "GFX", "bit_depth", bit_depth);
   set_config_int(c, "GFX", "screen_width", screen_width);
   set_config_int(c, "GFX", "screen_height", screen_height);
   set_config_int(c, "GFX", "window_width", window_width);
   set_config_int(c, "GFX", "window_height", window_height);
   set_config_int(c, "GFX", "samples", screen_samples);
   set_config_int(c, "GFX", "vsync", use_vsync);

   set_config_int(c, "TIMING", "logic_framerate", logic_framerate);
   set_config_int(c, "TIMING", "max_frame_skip", max_frame_skip);
   set_config_int(c, "TIMING", "limit_framerate", limit_framerate);
   set_config_int(c, "TIMING", "display_framerate", display_framerate);
   set_config_int(c, "TIMING", "reduce_cpu_usage", reduce_cpu_usage);

   set_config_int(c, "SOUND", "sound_volume", sound_volume);
   set_config_int(c, "SOUND", "music_volume", music_volume);

   set_config_int(c, "CONTROLS", "controller_id", controller_id);

   al_save_config_file(config, c);
   al_destroy_config(c);
}

static bool load(DATA_ENTRY *d, int id, char const *type, char const *path,
   char const *subfolder, char const *name, char const *ext, int size)
{
   static char spath[1024];
   sprintf(spath, "%s/%s/%s.%s", path, subfolder, name, ext);
   printf("Loading %s...\n", spath);
   if (!strcmp(type, "font")) d[id].dat = al_load_font(spath, size, 0);
   if (!strcmp(type, "bitmap")) d[id].dat = al_load_bitmap(spath);
   if (!strcmp(type, "sample")) d[id].dat = al_load_sample(spath);
   if (!strcmp(type, "music")) d[id].dat = al_load_audio_stream(spath, 2, 4096);
   if (d[id].dat == NULL) {
      printf("Failed loading %s.\n", name);
   }
   d[id].type = type;
   d[id].path = strdup(path);
   d[id].subfolder = strdup(subfolder);
   d[id].name = strdup(name);
   d[id].ext = strdup(ext);
   d[id].size = size;
   return d[id].dat != NULL;
}

int change_gfx_mode(void)
{
   int ret = DEMO_OK;
   int flags = 0;

   /* Select appropriate (fullscreen or windowed) gfx mode driver. */
   if (fullscreen == 0) {
      flags |= ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE;
      screen_width = window_width;
      screen_height = window_height;
   } else if (fullscreen == 1) {
      flags |= ALLEGRO_FULLSCREEN_WINDOW;
   } else {
      flags |= ALLEGRO_FULLSCREEN;
   }

   if (screen) {
      al_destroy_display(screen);
   }

   al_set_new_display_flags(flags);

   // May be a good idea, but need to add a border to textures for it.
   // al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);

   if (screen_samples > 1) {
      al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
      al_set_new_display_option(ALLEGRO_SAMPLES, screen_samples, ALLEGRO_SUGGEST);
   }
   else {
      al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 0, ALLEGRO_SUGGEST);
      al_set_new_display_option(ALLEGRO_SAMPLES, 0, ALLEGRO_SUGGEST);
   }

   al_set_new_display_option(ALLEGRO_SUPPORTED_ORIENTATIONS,
                             ALLEGRO_DISPLAY_ORIENTATION_LANDSCAPE, ALLEGRO_SUGGEST);

   /* Attempt to set the selected colour depth and gfx mode. */
   screen = al_create_display(screen_width, screen_height);
   if (!screen) {
      return DEMO_ERROR_ALLEGRO;
   }
   al_set_window_constraints(screen, 320, 320, 0, 0);

   screen_width = al_get_display_width(screen);
   screen_height = al_get_display_height(screen);
   screen_orientation = ALLEGRO_DISPLAY_ORIENTATION_90_DEGREES;

   al_register_event_source(event_queue, al_get_display_event_source(screen));

   /* blank display now, before doing any more complicated stuff */
   al_clear_to_color(al_map_rgb(0, 0, 0));

   /* Attempt to load game data. */
   ret = load_data();

   /* If loading was successful, initialize the background scroller module. */
   if (ret == DEMO_OK) {
      init_background();
   }

   return ret;
}

static DATA_ENTRY *load_data_entries(char const *path)
{
   DATA_ENTRY *d = calloc(DEMO_DATA_COUNT + 1, sizeof *d);
   load(d, DEMO_BMP_BACK,        "bitmap",   path, "menu",     "back", "png", 0);
   load(d, DEMO_FONT,            "font",     path, "menu",     "cancunsmall", "png", 0);
   load(d, DEMO_FONT_LOGO,       "font",     path, "menu",     "logofont", "png", 0);
   load(d, DEMO_MIDI_INGAME,     "music",    path, "menu",     "skate2", "ogg", 0);
   load(d, DEMO_MIDI_INTRO,      "music",    path, "menu",     "intro_music", "ogg", 0);
   load(d, DEMO_MIDI_MENU,       "music",    path, "menu",     "menu_music", "ogg", 0);
   load(d, DEMO_MIDI_SUCCESS,    "music",    path, "menu",     "endoflevel", "ogg", 0);
   load(d, DEMO_SAMPLE_BUTTON,   "sample",   path, "menu",     "button", "ogg", 0);
   load(d, DEMO_SAMPLE_WELCOME,  "sample",   path, "menu",     "welcome", "ogg", 0);
   load(d, DEMO_SAMPLE_SKATING,  "sample",   path, "audio",    "skating", "ogg", 0);
   load(d, DEMO_SAMPLE_WAVE,     "sample",   path, "audio",    "wave", "ogg", 0);
   load(d, DEMO_SAMPLE_DING,     "sample",   path, "audio",    "ding", "ogg", 0);
   load(d, DEMO_SAMPLE_DOOROPEN, "sample",   path, "audio",    "dooropen", "ogg", 0);
   load(d, DEMO_SAMPLE_POP,      "sample",   path, "audio",    "pop", "ogg", 0);

   load(d, DEMO_BMP_BANANAS,     "bitmap",   path, "graphics", "bananas", "png", 0);
   load(d, DEMO_BMP_CHERRIES,    "bitmap",   path, "graphics", "cherries", "png", 0);
   load(d, DEMO_BMP_CLOUD,       "bitmap",   path, "graphics", "cloud", "png", 0);
   load(d, DEMO_BMP_DOOROPEN,    "bitmap",   path, "graphics", "dooropen", "png", 0);
   load(d, DEMO_BMP_DOORSHUT,    "bitmap",   path, "graphics", "doorshut", "png", 0);
   load(d, DEMO_BMP_EXITSIGN,    "bitmap",   path, "graphics", "exitsign", "png", 0);
   load(d, DEMO_BMP_GRASS,       "bitmap",   path, "graphics", "grass", "png", 0);
   load(d, DEMO_BMP_ICECREAM,    "bitmap",   path, "graphics", "icecream", "png", 0);
   load(d, DEMO_BMP_ICE,         "bitmap",   path, "graphics", "ice", "png", 0);
   load(d, DEMO_BMP_ICETIP,      "bitmap",   path, "graphics", "icetip", "png", 0);
   load(d, DEMO_BMP_ORANGE,      "bitmap",   path, "graphics", "orange", "png", 0);
   load(d, DEMO_BMP_SKATEFAST,   "bitmap",   path, "graphics", "skatefast", "png", 0);
   load(d, DEMO_BMP_SKATEMED,    "bitmap",   path, "graphics", "skatemed", "png", 0);
   load(d, DEMO_BMP_SKATER1,     "bitmap",   path, "graphics", "skater1", "png", 0);
   load(d, DEMO_BMP_SKATER2,     "bitmap",   path, "graphics", "skater2", "png", 0);
   load(d, DEMO_BMP_SKATER3,     "bitmap",   path, "graphics", "skater3", "png", 0);
   load(d, DEMO_BMP_SKATER4,     "bitmap",   path, "graphics", "skater4", "png", 0);
   load(d, DEMO_BMP_SKATESLOW,   "bitmap",   path, "graphics", "skateslow", "png", 0);
   load(d, DEMO_BMP_SOIL,        "bitmap",   path, "graphics", "soil", "png", 0);
   load(d, DEMO_BMP_SWEET,       "bitmap",   path, "graphics", "sweet", "png", 0);
   load(d, DEMO_BMP_WATER,       "bitmap",   path, "graphics", "water", "png", 0);

   return d;
}

void unload_data_entries(DATA_ENTRY *data)
{
    while (data && data->dat) {
        if (!strcmp(data->type, "bitmap")) al_destroy_bitmap(data->dat);
        if (!strcmp(data->type, "font")) al_destroy_font(data->dat);
        if (!strcmp(data->type, "sample")) al_destroy_sample(data->dat);
        if (!strcmp(data->type, "music")) al_destroy_audio_stream(data->dat);
        data++;
    }
}

int load_data(void)
{
   if (demo_data)
      return DEMO_OK;

   /* Load the data for the game menus. */
   demo_data = load_data_entries(data_path);
   if (demo_data == 0) {
      return DEMO_ERROR_DATA;
   }

   /* Load other game resources. */
   if ((GameError = load_game_resources(data_path)))
      return DEMO_ERROR_GAMEDATA;

   return DEMO_OK;
}


void unload_data(void)
{
   if (demo_data != 0) {
      unload_data_entries(demo_data);
      unload_game_resources();
      demo_data = 0;
   }
}


void demo_textout(const ALLEGRO_FONT *f, const char *s, int x, int y,
                  ALLEGRO_COLOR color)
{
   demo_textprintf(f, x, y, color, "%s", s);
}


void demo_textout_right(const ALLEGRO_FONT *f, const char *s, int x, int y,
                  ALLEGRO_COLOR color)
{
   demo_textprintf_right(f, x, y, color, "%s", s);
}


void demo_textout_centre(const ALLEGRO_FONT *f, const char *s, int x, int y,
                  ALLEGRO_COLOR color)
{
   demo_textprintf_centre(f, x, y, color, "%s", s);
}


void demo_textprintf_centre(const ALLEGRO_FONT *font, int x, int y,
                     ALLEGRO_COLOR col, const char *format, ...)
{
   char buf[512];

   va_list ap;

   va_start(ap, format);
   vsnprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   demo_textprintf_ex(font, x, y, col, 2, "%s", buf);
}


void demo_textprintf_right(const ALLEGRO_FONT *font, int x, int y,
                     ALLEGRO_COLOR col, const char *format, ...)
{
   char buf[512];

   va_list ap;

   va_start(ap, format);
   vsnprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   demo_textprintf_ex(font, x, y, col, 1, "%s", buf);
}


void demo_textprintf(const ALLEGRO_FONT *font, int x, int y,
                     ALLEGRO_COLOR col, const char *format, ...)
{
   char buf[512];

   va_list ap;

   va_start(ap, format);
   vsnprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   demo_textprintf_ex(font, x, y, col, 0, "%s", buf);
}

void demo_textprintf_ex(const ALLEGRO_FONT *font, int x, int y,
                     ALLEGRO_COLOR col, int align, const char *format, ...)
{
   char buf[512];

   va_list ap;

   va_start(ap, format);
   vsnprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   switch (align) {
      case 0:
         al_draw_textf(font, col, x, y, ALLEGRO_ALIGN_LEFT, "%s", buf);
         break;
      case 1:
         al_draw_textf(font, col, x, y, ALLEGRO_ALIGN_RIGHT, "%s", buf);
         break;
      case 2:
         al_draw_textf(font, col, x, y, ALLEGRO_ALIGN_CENTRE, "%s", buf);
         break;
   };

}


void shadow_textprintf(ALLEGRO_FONT *font, int x, int y,
                     ALLEGRO_COLOR col, int align, const char *format, ...)
{
   char buf[512];

   va_list ap;

   va_start(ap, format);
   vsnprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   demo_textprintf_ex(font, x + shadow_offset, y + shadow_offset, al_map_rgba(0, 0, 0, 128),
                   align, "%s", buf);
   demo_textprintf_ex(font, x, y, col, align, "%s", buf);
}

int my_stricmp(const char *s1, const char *s2)
{
    char c1, c2;
    int v;

    while(1) {
        c1 = *s1++;
        c2 = *s2++;
        v = tolower(c1) - tolower(c2);
        if (v != 0 || c1 == 0 || c2 == 0) break;
    }

    return v;
}
