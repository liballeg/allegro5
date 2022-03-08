/*
 *    SPEED - by Shawn Hargreaves, 1999
 *
 *    Main program body, setup code, action pump, etc.
 */

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

#include "speed.h"



/* are we cheating? */
int cheat = FALSE; 



/* low detail mode? */
int low_detail = FALSE; 



/* disable the grid? */
int no_grid = FALSE; 



/* disable the music? */
int no_music = FALSE; 



/* how many points did we get? */
int score; 



/* the main game function */
static int play_game()
{ 
   ALLEGRO_TIMER *inc_counter;
   int gameover = 0;
   int cyclenum = 0;
   int redraw = 1;

   /* init */
   score = 0;

   init_view();
   init_player();
   init_badguys();
   init_bullets();
   init_explode();
   init_message();

   #define TIMER_SPEED  ALLEGRO_BPS_TO_SECS(30*(cyclenum+2))

   inc_counter = al_create_timer(TIMER_SPEED);
   al_start_timer(inc_counter);

   while (!gameover) {

      /* move everyone */
      while ((al_get_timer_count(inc_counter) > 0) && (!gameover)) {
         update_view();
         update_bullets();
         update_explode();
         update_message();

         if (update_badguys()) {
            if (advance_view()) {
               cyclenum++;
               al_set_timer_count(inc_counter, 0);
               lay_attack_wave(TRUE);
               advance_player(TRUE);
            }
            else {
               lay_attack_wave(FALSE);
               advance_player(FALSE);
            }
         }

         gameover = update_player();

         al_set_timer_count(inc_counter, al_get_timer_count(inc_counter)-1);
         redraw = 1;
      }

      /* take a screenshot? */
      if (key[ALLEGRO_KEY_PRINTSCREEN]) {
         static int ss_count = 0;

         char fname[80];

         sprintf(fname, "speed%03d.tga", ++ss_count);

         al_save_bitmap(fname, al_get_backbuffer(screen));

         while (key[ALLEGRO_KEY_PRINTSCREEN])
            poll_input_wait();

         al_set_timer_count(inc_counter, 0);
      }

      /* toggle fullscreen window */
      if (key[ALLEGRO_KEY_F]) {
         int flags = al_get_display_flags(screen);
         al_set_display_flag(screen, ALLEGRO_FULLSCREEN_WINDOW,
            !(flags & ALLEGRO_FULLSCREEN_WINDOW));

         while (key[ALLEGRO_KEY_F])
            poll_input_wait();
      }

      /* draw everyone */
      if (redraw) {
         draw_view();
         redraw = 0;
      }
      else {
         rest(1);
      }
   }

   /* cleanup */
   al_destroy_timer(inc_counter);

   shutdown_view();
   shutdown_player();
   shutdown_badguys();
   shutdown_bullets();
   shutdown_explode();
   shutdown_message();

   if (gameover < 0) {
      sfx_ping(1);
      return FALSE;
   }

   return TRUE;
}



/* display a commandline usage message */
static void usage()
{
   printf(
      "\n"
      "SPEED - by Shawn Hargreaves, 1999\n"
      "Allegro 5 port, 2010\n"
      "\n"
      "Usage: speed w h [options]\n"
      "\n"
      "The w and h values set your desired screen resolution.\n"
      "What modes are available will depend on your hardware.\n"
      "\n"
      "Available options:\n"
      "\n"
      "\t-fullscreen enables full screen mode. (w and h are optional)\n"
      "\t-cheat makes you invulnerable.\n"
      "\t-simple turns off the more expensive graphics effects.\n"
      "\t-nogrid turns off the wireframe background grid.\n"
      "\t-nomusic turns off my most excellent background music.\n"
      "\t-www invokes the built-in web browser.\n"
      "\n"
      "Example usage:\n"
      "\n"
      "\tspeed 640 480\n"
   );
}



/* the main program body */
int main(int argc, char *argv[])
{
   int w = 0, h = 0;
   int www = FALSE;
   int i, n;
   int display_flags = ALLEGRO_GENERATE_EXPOSE_EVENTS;

   srand(time(NULL));
   
   al_set_org_name("liballeg.org");
   al_set_app_name("SPEED");

   if (!al_init()) {
      fprintf(stderr, "Could not initialise Allegro.\n");
      return 1;
   }
   al_init_primitives_addon();

   /* parse the commandline */
   for (i=1; i<argc; i++) {
      if (strcmp(argv[i], "-cheat") == 0) {
         cheat = TRUE;
      }
      else if (strcmp(argv[i], "-simple") == 0) {
         low_detail = TRUE;
      }
      else if (strcmp(argv[i], "-nogrid") == 0) {
         no_grid = TRUE;
      }
      else if (strcmp(argv[i], "-nomusic") == 0) {
         no_music = TRUE;
      }
      else if (strcmp(argv[i], "-www") == 0) {
         www = TRUE;
      }
      else if (strcmp(argv[i], "-fullscreen") == 0) {
         /* if no width is specified, assume fullscreen_window */
         display_flags |= w ? ALLEGRO_FULLSCREEN : ALLEGRO_FULLSCREEN_WINDOW;
      }
      else {
         n = atoi(argv[i]);

         if (!n) {
            usage();
            return 1;
         }

         if (!w) {
            w = n;
            if (display_flags & ALLEGRO_FULLSCREEN_WINDOW) {
               /* toggle from fullscreen_window to fullscreen */
               display_flags &= ~ALLEGRO_FULLSCREEN_WINDOW;
               display_flags |= ALLEGRO_FULLSCREEN;
            }
         }
         else if (!h) {
            h = n;
         }
         else {
            usage();
            return 1;
         }
      }
   }

   /* it's a real shame that I had to take this out! */
   if (www) {
      printf(
         "\n"
         "Unfortunately the built-in web browser feature had to be removed.\n"
         "\n"
         "I did get it more or less working as of Saturday evening (forms and\n"
         "Java were unsupported, but tables and images were mostly rendering ok),\n"
         "but the US Department of Justice felt that this was an unacceptable\n"
         "monopolistic attempt to tie in web browsing functionality to an\n"
         "unrelated product, so they threatened me with being sniped at from\n"
         "the top of tall buildings by guys with high powered rifles unless I\n"
         "agreed to disable this code.\n"
         "\n"
         "We apologise for any inconvenience that this may cause you.\n"
      );

      return 1;
   }
   
   if (!w || !h) {
      if (argc == 1 || (display_flags & ALLEGRO_FULLSCREEN_WINDOW)) {
         w = 640;
         h = 480;
      }
      else {
         usage();
         return 1;
      }
   }

   /* set the screen mode */
   al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
   al_set_new_display_option(ALLEGRO_SAMPLES, 4, ALLEGRO_SUGGEST);

   al_set_new_display_flags(display_flags);
   screen = al_create_display(w, h);
   if (!screen) {
      fprintf(stderr, "Error setting %dx%d display mode\n", w, h);
      return 1;
   }

   /* To avoid performance problems on graphics drivers that don't support
    * drawing to textures, we build up transition screens on memory bitmaps.
    * We need a font loaded into a memory bitmap for those, then a font
    * loaded into a video bitmap for the game view. Blech!
    */
   al_init_font_addon();
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   font = al_create_builtin_font();
   if (!font) {
      fprintf(stderr, "Error creating builtin font\n");
      return 1;
   }

   al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
   font_video = al_create_builtin_font();
   if (!font_video) {
      fprintf(stderr, "Error creating builtin font\n");
      return 1;
   }

   /* set up everything else */
   al_install_keyboard();
   al_install_joystick();
   if (al_install_audio()) {
      if (!al_reserve_samples(8))
         al_uninstall_audio();
   }

   init_input();
   init_sound();
   init_hiscore();

   /* the main program body */
   while (title_screen()) {
      if (play_game()) {
         show_results();
         score_table();
      }
   }

   /* time to go away now */
   shutdown_hiscore();
   shutdown_sound();

   goodbye();

   shutdown_input();

   al_destroy_font(font);
   al_destroy_font(font_video);

   return 0;
}
