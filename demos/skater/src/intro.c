#include "global.h"
#include "menus.h"
#include "defines.h"
#include "music.h"
#include "mouse.h"


static int _id = DEMO_STATE_INTRO;
static double duration;
static double progress;
static int already_played_midi;

static int id(void)
{
   return _id;
}


static void init(void)
{
   duration = 4.0f;
   progress = 0.0f;
   already_played_midi = 0;
}


static int update(void)
{
   progress += 1.0f / logic_framerate;

   if (progress >= duration) {
      return DEMO_STATE_MAIN_MENU;
   }

   if (key_pressed(ALLEGRO_KEY_ESCAPE)) return DEMO_STATE_MAIN_MENU;
   if (key_pressed(ALLEGRO_KEY_SPACE)) return DEMO_STATE_MAIN_MENU;
   if (key_pressed(ALLEGRO_KEY_ENTER)) return DEMO_STATE_MAIN_MENU;
   if (mouse_button_pressed(1)) return DEMO_STATE_MAIN_MENU;

   return id();
}


static void draw(void)
{
   int x, y, offx, offy;
   float c = 1;
   static char logo_text1[] = "Allegro";
   static char logo_text2[] = "5.0";
   ALLEGRO_COLOR color;

   if (progress < 0.5f) {
      c = progress / 0.5f;
      al_clear_to_color(al_map_rgb_f(c, c, c));
   } else {
      int alpha = (progress - 0.5) / 0.5;
      if (alpha > 1) alpha = 1;
        
      if (progress > 2.5) {
         float r, g, b;
         al_color_hsv_to_rgb(120 + (progress - 2.5) / 0.5 * 360 * (progress - 1.5), 1, 1, &r, &g, &b);
         color = al_map_rgba_f(r * alpha, g * alpha, b * alpha, alpha);
         c = 1 - (progress - 2.5) / 10.0;
      }
      else {
         color = al_map_rgba_f(0, 0, 0.8 * alpha, alpha);
      }
      
      if (!already_played_midi) {
         play_music(DEMO_MIDI_INTRO, 0);
         already_played_midi = 1;
      }

      al_clear_to_color(al_map_rgb_f(c, c, c));

      x = screen_width / 2;
      y = screen_height / 2 - 3 * al_get_font_line_height(demo_font_logo) / 2;

      offx = 0;
      if (progress < 1.0f) {
         offx =
            (int)(al_get_text_width(demo_font_logo, logo_text1) *
                  (1.0f - 2.0f * (progress - 0.5f)));
      }

      demo_textprintf_centre(demo_font_logo, x + 6 - offx,
                           y + 5, al_map_rgba(128, 128, 128, 64), logo_text1);
      demo_textprintf_centre(demo_font_logo, x - offx, y,
         color,
         logo_text1);

      if (progress >= 1.5f) {
         y += 3 * al_get_font_line_height(demo_font_logo) / 2;
         offy = 0;
         if (progress < 2.0f) {
            offy = (int)((screen_height - y) * (1.0f - 2.0f * (progress - 1.5f)));
         }

         demo_textprintf_centre(demo_font_logo, x + 6,
                              y + 5 + offy, al_map_rgba(128, 128, 128, 64),
                              logo_text2);
         demo_textprintf_centre(demo_font_logo, x, y + offy, color,
                              logo_text2);
      }
   }
}


void create_intro(GAMESTATE * state)
{
   state->id = id;
   state->init = init;
   state->update = update;
   state->draw = draw;
}
