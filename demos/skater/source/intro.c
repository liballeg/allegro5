#include <allegro.h>
#include "../include/menus.h"
#include "../include/global.h"
#include "../include/defines.h"
#include "../include/music.h"


static int _id = DEMO_STATE_INTRO;
static float duration;
static float progress;
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
   set_palette(demo_data[DEMO_MENU_PALETTE].dat);
}


static int update(void)
{
   int c;

   progress += 1.0f / logic_framerate;

   if (progress >= duration) {
      return DEMO_STATE_MAIN_MENU;
   }

   if (keypressed()) {
      c = readkey() >> 8;
      clear_keybuf();

      if (c == KEY_ESC) {
         return DEMO_STATE_EXIT;
      }

      return DEMO_STATE_MAIN_MENU;
   }

   return id();
}


static void draw(BITMAP *canvas)
{
   int c, x, y, offx, offy;
   static char logo_text1[] = "Allegro";
   static char logo_text2[] = "";
   /* XXX commented out because the font doesn't contain the characters for
    * anything other than "Allegro 4.2"
    */
   /* static char logo_text2[] = "4.2"; */

   if (progress < 0.5f) {
      c = (int)(255 * progress / 0.5f);
      clear_to_color(canvas, makecol(c, c, c));
   } else {
      if (!already_played_midi) {
         play_music(DEMO_MIDI_INTRO, 0);
         already_played_midi = 1;
      }

      c = 255;
      clear_to_color(canvas, makecol(c, c, c));

      x = SCREEN_W / 2;
      y = SCREEN_H / 2 - 3 * text_height(demo_font_logo) / 2;

      offx = 0;
      if (progress < 1.0f) {
         offx =
            (int)(text_length(demo_font_logo, logo_text1) *
                  (1.0f - 2.0f * (progress - 0.5f)));
      }

      demo_textprintf_centre(canvas, demo_font_logo_m, x + 6 - offx,
                           y + 5, makecol(128, 128, 128), -1, logo_text1);
      demo_textprintf_centre(canvas, demo_font_logo, x - offx, y, -1, -1,
                           logo_text1);

      if (progress >= 1.5f) {
         y += 3 * text_height(demo_font_logo) / 2;
         offy = 0;
         if (progress < 2.0f) {
            offy = (int)((SCREEN_H - y) * (1.0f - 2.0f * (progress - 1.5f)));
         }

         demo_textprintf_centre(canvas, demo_font_logo_m, x + 6,
                              y + 5 + offy, makecol(128, 128, 128), -1,
                              logo_text2);
         demo_textprintf_centre(canvas, demo_font_logo, x, y + offy, -1, -1,
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
