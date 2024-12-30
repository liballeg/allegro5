#include "demo.h"
#include "data.h"
#include "expl.h"
#include "title.h"
#include "game.h"
#include <allegro5/allegro_ttf.h>

/* command line options */
int cheat = FALSE;
int jumpstart = FALSE;

int max_fps = FALSE;
PALETTE *palette;

ALLEGRO_COLOR get_palette(int p) {
   if (!palette) return makecol(0, 0, 0);
   return palette->rgb[p];
}

void set_palette(PALETTE *p) {
   palette = p;
}



void fade_out(int skip)
{
   ALLEGRO_BITMAP *capture = al_create_bitmap(SCREEN_W, SCREEN_H);
   al_set_target_bitmap(capture);
   al_draw_bitmap(al_get_backbuffer(screen), 0, 0, 0);
   al_set_target_backbuffer(screen);

   int steps = 128 / skip;
   if (steps < 1) steps = 1;
   double t0 = al_get_time();
   for (int i = 0; i < steps; i++) {
      al_rest(t0 + (i + 1) / 120.0 - al_get_time());
      ALLEGRO_COLOR fade = al_map_rgba_f(0, 0, 0, 1.0 * (i + 1) / steps);
      al_draw_bitmap(capture, 0, 0, 0);
      al_draw_filled_rectangle(0, 0, SCREEN_W, SCREEN_H, fade);
      al_flip_display();
      poll_input();
      if (keypressed()) break;
   }
   al_destroy_bitmap(capture);
}


static void intro_screen(void)
{
   play_sample(data[INTRO_SPL].dat, 255, 128, 1000, FALSE);

   int t0 = al_get_time();
   for (int i = 0; i < 51; i++) {
      int x = i % 8;
      int y = i / 8;
      stretch_blit(data[INTRO_ANIM].dat, x * 320, y * 200, 320, 200, 0, 0, SCREEN_W, SCREEN_H);
      al_flip_display();
      double dt = t0 + i * 0.050 - al_get_time();
      al_rest(dt);
   }

   rest(1000);
   fade_out(1);
}



int main(int argc, char *argv[])
{
   int c;
   int w, h;

   for (c = 1; c < argc; c++) {
      if (stricmp(argv[c], "-cheat") == 0)
         cheat = TRUE;

      if (stricmp(argv[c], "-jumpstart") == 0)
         jumpstart = TRUE;
   }

   srand(time(NULL));
   if (!allegro_init())
      return 1;

   ALLEGRO_MONITOR_INFO info;
   al_get_monitor_info(0, &info);
   w = (info.x2 - info.x1) * 0.75;
   h = (info.y2 - info.y1) * 0.75;

   al_set_new_display_flags(ALLEGRO_RESIZABLE);
   screen = al_create_display(w, h);
   if (!screen) {
      allegro_message("Error setting graphics mode\n");
      exit(1);
   }
   al_init_image_addon();
   al_init_acodec_addon();
   install_keyboard();
   install_mouse();
   install_timer();
   al_init_font_addon();
   al_init_ttf_addon();
   al_init_primitives_addon();

   font = al_create_builtin_font();

   if (!install_sound()) {
      allegro_message("Error initialising sound\n%s\n", allegro_error);
   }

   //if (install_joystick(JOY_TYPE_AUTODETECT) != 0) {
      //allegro_message("Error initialising joystick\n%s\n", allegro_error);
      //install_joystick(JOY_TYPE_NONE);
   //}

   data_load();

   if (!jumpstart) {
      intro_screen();
   }

   generate_explosions();

   while (title_screen())
      play_game();

   destroy_explosions();

   stop_midi();

   return 0;
}
