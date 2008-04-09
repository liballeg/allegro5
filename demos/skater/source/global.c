#include <allegro.h>
#ifdef DEMO_USE_ALLEGRO_GL
    #include <alleggl.h>
    #include "../include/oglflip.h"
#endif
#include "../include/backscrl.h"
#include "../include/dblbuf.h"
#include "../include/demodata.h"
#include "../include/game.h"
#include "../include/global.h"
#include "../include/pageflip.h"
#include "../include/tribuf.h"
#include "../include/updtedvr.h"

/* Default values of some config varables */
int fullscreen = 0;
int bit_depth = 32;
int screen_width = 640;
int screen_height = 480;
int update_driver_id = DEMO_DOUBLE_BUFFER;
int use_vsync = 0;
int logic_framerate = 100;
int max_frame_skip = 5;
int limit_framerate = 1;
int display_framerate = 1;
int reduce_cpu_usage = 1;
int sound_volume = 5;
int music_volume = 5;
int controller_id = 0;

int shadow_offset = 2;

VCONTROLLER *controller[2];

char config_path[DEMO_PATH_LENGTH];
char data_path[DEMO_PATH_LENGTH];
FONT *demo_font;
FONT *demo_font_logo;
FONT *demo_font_logo_m;
FONT *plain_font;
DATAFILE *demo_data = 0;

DEMO_SCREEN_UPDATE_DRIVER update_driver = { 0, 0, 0, 0 };

char *GameError;

int load_data(void);

const char *demo_error(int id)
{
   switch (id) {
      case DEMO_ERROR_ALLEGRO:
         return allegro_error;
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



/* read_config:
 * Load settings from the configuration file, providing default values
 */
void read_config(const char *config)
{
   push_config_state();
   set_config_file(config);

   fullscreen = get_config_int("GFX", "fullscreen", fullscreen);
   bit_depth = get_config_int("GFX", "bit_depth", bit_depth);
   screen_width = get_config_int("GFX", "screen_width", screen_width);
   screen_height = get_config_int("GFX", "screen_height", screen_height);
   update_driver_id =
      get_config_int("GFX", "update_driver_id", update_driver_id);
   use_vsync = get_config_int("GFX", "vsync", use_vsync);

   logic_framerate =
      get_config_int("TIMING", "logic_framerate", logic_framerate);
   limit_framerate =
      get_config_int("TIMING", "limit_framerate", limit_framerate);
   max_frame_skip =
      get_config_int("TIMING", "max_frame_skip", max_frame_skip);
   display_framerate =
      get_config_int("TIMING", "display_framerate", display_framerate);
   reduce_cpu_usage =
      get_config_int("TIMING", "reduce_cpu_usage", reduce_cpu_usage);

   sound_volume = get_config_int("SOUND", "sound_volume", sound_volume);
   music_volume = get_config_int("SOUND", "music_volume", music_volume);

   controller_id = get_config_int("CONTROLS", "controller_id", controller_id);

   pop_config_state();
}


void write_config(const char *config)
{
   push_config_state();
   set_config_file(config);

   set_config_int("GFX", "fullscreen", fullscreen);
   set_config_int("GFX", "bit_depth", bit_depth);
   set_config_int("GFX", "screen_width", screen_width);
   set_config_int("GFX", "screen_height", screen_height);
   set_config_int("GFX", "update_driver_id", update_driver_id);
   set_config_int("GFX", "vsync", use_vsync);

   set_config_int("TIMING", "logic_framerate", logic_framerate);
   set_config_int("TIMING", "max_frame_skip", max_frame_skip);
   set_config_int("TIMING", "limit_framerate", limit_framerate);
   set_config_int("TIMING", "display_framerate", display_framerate);
   set_config_int("TIMING", "reduce_cpu_usage", reduce_cpu_usage);

   set_config_int("SOUND", "sound_volume", sound_volume);
   set_config_int("SOUND", "music_volume", music_volume);

   set_config_int("CONTROLS", "controller_id", controller_id);

   pop_config_state();
}


int change_gfx_mode(void)
{
   int gfx_mode;
   int alternative;
   int ret = DEMO_OK;

   /* First unload all previously loaded data, select the direct screen
      update driver and go to text mode just in case. */
   unload_data();
   select_update_driver(DEMO_UPDATE_DIRECT);
   set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);

   /* In windowed mode the desktop color depth takes preference to the
      user specified colour depth for performance reasons. */
   if (fullscreen == 0) {
      bit_depth = desktop_color_depth();
   }

   /* Select appropriate (fullscreen or windowed) gfx mode driver. */
#ifdef DEMO_USE_ALLEGRO_GL
   gfx_mode = GFX_OPENGL;
#else
   if (fullscreen == 1) {
      gfx_mode = GFX_AUTODETECT_FULLSCREEN;
   } else {
      gfx_mode = GFX_AUTODETECT_WINDOWED;
   }
#endif


#ifdef DEMO_USE_ALLEGRO_GL
   /* AllegroGL has its own way to set GFX options. */
   allegro_gl_set(AGL_COLOR_DEPTH, bit_depth);
   allegro_gl_set(AGL_DOUBLEBUFFER, 1);
   allegro_gl_set(AGL_RENDERMETHOD, 1);
   allegro_gl_set(AGL_WINDOWED, !fullscreen);
   allegro_gl_set(AGL_SUGGEST, AGL_COLOR_DEPTH | AGL_DOUBLEBUFFER | AGL_RENDERMETHOD
                | AGL_WINDOWED);
#else
   set_color_depth(bit_depth);
#endif

   /* Attempt to set the selected colour depth and gfx mode. */
   if (set_gfx_mode(gfx_mode, screen_width, screen_height, 0, 0) != 0) {
      /* If setting gfx mode failed, try an alternative colour depth. 8bpp
         doesn't have an alternative, but 15 and 16 can be exchanged as well
         as 24 and 32. */
      switch (bit_depth) {
         case 8:
            alternative = 0;
            break;
         case 15:
            alternative = 16;
            break;
         case 16:
            alternative = 15;
            break;
         case 24:
            alternative = 32;
            break;
         case 32:
            alternative = 24;
            break;
         default:
            alternative = 16;
            break;
      }

      if (alternative != 0) {
         /* Try to set the alternative colour depth and gfx mode. */
         set_color_depth(alternative);
         if (set_gfx_mode(gfx_mode, screen_width, screen_height, 0, 0)
             != 0) {
            /* Won't go. Give up! */
            return DEMO_ERROR_ALLEGRO;
         }

         /* Alternative worked, so remember it for next time. */
         bit_depth = alternative;
      }
      else {
         /* Give up is there is no alternative color depth. */
         return DEMO_ERROR_ALLEGRO;
      }
   }

#ifdef DEMO_USE_ALLEGRO_GL
   /* Needed for Allegro drawnig functions to behave. Also sets the Allegro
      coordinate system. */
   allegro_gl_set_allegro_mode();
#endif

   /* blank display now, before doing any more complicated stuff */
   clear_to_color(screen, makecol(0, 0, 0));
   if (fullscreen) {
      show_os_cursor(MOUSE_CURSOR_NONE);
   }

#ifdef DEMO_USE_ALLEGRO_GL
   /* OpenGL doesn't let us choose the update mode. */
   select_update_driver(DEMO_OGL_FLIPPING);
#else
   /* select update driver */
   switch (update_driver_id) {
      case DEMO_TRIPLE_BUFFER:
         ret = select_update_driver(DEMO_TRIPLE_BUFFER);
         if (ret == DEMO_OK)
            break;
         /* fall through */

      case DEMO_PAGE_FLIPPING:
         ret = select_update_driver(DEMO_PAGE_FLIPPING);
         if (ret == DEMO_OK)
            break;
         /* fall through */

      case DEMO_DOUBLE_BUFFER:
         ret = select_update_driver(DEMO_DOUBLE_BUFFER);
         break;

      default:
         ret = DEMO_ERROR_GFX;
   };
#endif

   /* Attempt to load game data. */
   ret = load_data();

   /* If loading was successful, initialize the background scroller module. */
   if (ret == DEMO_OK) {
      init_background();
   }

   return ret;
}


int load_data(void)
{
   /* First unload any data that was previously loaded. We don't want any
      nasty memory leaks. */
   unload_data();

   /* Load the data for the game menus. */
   demo_data = load_datafile(data_path);
   if (demo_data == 0) {
      return DEMO_ERROR_DATA;
   }

#ifdef DEMO_USE_ALLEGRO_GL_FONT
   /* Convert Allegro FONT to AllegroGL FONT. */

   /* Note that we could use Allegro's FONT and text output routines in
      AllegroGL mode but it is proven to be very slow. */

   demo_font = allegro_gl_convert_allegro_font_ex(demo_data[DEMO_FONT].dat,
               AGL_FONT_TYPE_TEXTURED, -1.0, GL_ALPHA8);

   set_palette(demo_data[DEMO_MENU_PALETTE].dat);
   demo_font_logo = allegro_gl_convert_allegro_font_ex(
                    demo_data[DEMO_FONT_LOGO].dat, AGL_FONT_TYPE_TEXTURED,
                    -1.0, GL_RGBA8);

   /* Due to lack of the flexibility in AllegroGL text output routines we need
      an additional FONT that is equivalent to demo_font_logo - but monocrome.
      This allows tinting it to any color, which cannot be done with true color
      fonts (and paletted fonts are not supported by AllegroGL). */
   demo_font_logo_m = allegro_gl_convert_allegro_font_ex(
                    demo_data[DEMO_FONT_LOGO].dat, AGL_FONT_TYPE_TEXTURED,
                    -1.0, GL_ALPHA8);

   plain_font = allegro_gl_convert_allegro_font_ex(font,
                AGL_FONT_TYPE_TEXTURED, -1.0, GL_ALPHA8);
#else
   demo_font = demo_data[DEMO_FONT].dat;
   demo_font_logo = demo_data[DEMO_FONT_LOGO].dat;
   demo_font_logo_m = demo_font_logo;
   plain_font = font;
#endif

   /* Load other game resources. */
   if ((GameError = load_game_resources()))
      return DEMO_ERROR_GAMEDATA;

   return DEMO_OK;
}


void unload_data(void)
{
   if (demo_data != 0) {
      unload_datafile(demo_data);
      unload_game_resources();
      demo_data = 0;
   }

#ifdef DEMO_USE_ALLEGRO_GL_FONT
   if (demo_font)
      allegro_gl_destroy_font(demo_font);
   if (demo_font_logo)
      allegro_gl_destroy_font(demo_font_logo);
   if (demo_font_logo_m)
      allegro_gl_destroy_font(demo_font_logo_m);
   if (plain_font)
      allegro_gl_destroy_font(plain_font);

   demo_font = NULL;
   demo_font_logo = NULL;
   demo_font_logo_m = NULL;
   plain_font = NULL;
#endif
}


void demo_textout(BITMAP *bmp, const FONT *f, const char *s, int x, int y,
                  int color, int bg)
{
   demo_textprintf(bmp, f, x, y, color, bg, "%s", s);
}


void demo_textout_right(BITMAP *bmp, const FONT *f, const char *s, int x, int y,
                  int color, int bg)
{
   demo_textprintf_right(bmp, f, x, y, color, bg, "%s", s);
}


void demo_textout_centre(BITMAP *bmp, const FONT *f, const char *s, int x, int y,
                  int color, int bg)
{
   demo_textprintf_centre(bmp, f, x, y, color, bg, "%s", s);
}


void demo_textprintf_centre(BITMAP *canvas, const FONT *font, int x, int y,
                     int col, int bg, const char *format, ...)
{
   char buf[512];

   va_list ap;

   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   demo_textprintf_ex(canvas, font, x, y, col, bg, 2, "%s", buf);
}


void demo_textprintf_right(BITMAP *canvas, const FONT *font, int x, int y,
                     int col, int bg, const char *format, ...)
{
   char buf[512];

   va_list ap;

   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   demo_textprintf_ex(canvas, font, x, y, col, bg, 1, "%s", buf);
}


void demo_textprintf(BITMAP *canvas, const FONT *font, int x, int y,
                     int col, int bg, const char *format, ...)
{
   char buf[512];

   va_list ap;

   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   demo_textprintf_ex(canvas, font, x, y, col, bg, 0, "%s", buf);
}

void demo_textprintf_ex(BITMAP *canvas, const FONT *font, int x, int y,
                     int col, int bg, int align, const char *format, ...)
{
   char buf[512];

   va_list ap;

   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

#ifdef DEMO_USE_ALLEGRO_GL_FONT
   if (bg != -1) {
      rectfill(canvas, x, y,
               x + text_length(font, buf), y + text_height(font), bg);
   }

   switch (align) {
      case 1:
         x = x - text_length(font, buf);
         break;
      case 2:
         x = x - text_length(font, buf) / 2;
         break;
   };

   glEnable(GL_TEXTURE_2D);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   allegro_gl_printf(font, x, y, 0, col, "%s", buf);

   glDisable(GL_BLEND);
   glDisable(GL_TEXTURE_2D);

#else

   switch (align) {
      case 0:
         textprintf_ex(canvas, font, x, y, col, bg, "%s", buf);
         break;
      case 1:
         textprintf_right_ex(canvas, font, x, y, col, bg, "%s", buf);
         break;
      case 2:
         textprintf_centre_ex(canvas, font, x, y, col, bg, "%s", buf);
         break;
   };
#endif
}


void shadow_textprintf(BITMAP *canvas, FONT *font, int x, int y,
                     int col, int align, const char *format, ...)
{
   char buf[512];

   va_list ap;

   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   demo_textprintf_ex(canvas, font, x + shadow_offset, y + shadow_offset, 0, -1,
                   align, "%s", buf);
   demo_textprintf_ex(canvas, font, x, y, col, -1, align, "%s", buf);
}


int select_update_driver(int id)
{
   int error;
   DEMO_SCREEN_UPDATE_DRIVER new_driver = { 0, 0, 0, 0 };

   /* destroy the previous driver if any */
   if (update_driver.destroy) {
      update_driver.destroy();
   }

   /* select the create function */
   switch (id) {
      case DEMO_DOUBLE_BUFFER:
         select_double_buffer(&new_driver);
         break;

      case DEMO_PAGE_FLIPPING:
         select_page_flipping(&new_driver);
         break;

      case DEMO_TRIPLE_BUFFER:
         select_triple_buffer(&new_driver);
         break;
#ifdef DEMO_USE_ALLEGRO_GL
      case DEMO_OGL_FLIPPING:
         select_ogl_flipping(&new_driver);
         break;
#endif
   };

   if (!new_driver.create) {
      return DEMO_ERROR_GFX;
   }

   /* create the backbuffers, video pages, etc. */
   error = new_driver.create();
   if (error != DEMO_OK) {
      /* If it didn't work, fall back to the old driver, if any. */
      if (update_driver.create) {
         update_driver.create();
      }

      return error;
   }

   /* copy the object over */
   update_driver = new_driver;

   return DEMO_OK;
}
