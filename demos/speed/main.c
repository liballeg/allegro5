/*
 *    SPEED - by Shawn Hargreaves, 1999
 *
 *    Main program body, setup code, action pump, etc.
 */

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <allegro.h>

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



/* timer for controlling game speed */
static volatile int counter;

static void inc_counter(void)
{
   counter++;
}

END_OF_STATIC_FUNCTION(inc_counter);



/* the main game function */
static int play_game()
{ 
   BITMAP *bmp = create_bitmap(SCREEN_W, SCREEN_H);

   int gameover = 0;
   int cyclenum = 0;

   /* init */
   score = 0;

   init_view();
   init_player();
   init_badguys();
   init_bullets();
   init_explode();
   init_message();

   LOCK_VARIABLE(counter);
   LOCK_FUNCTION(inc_counter);

   counter = 0;

   #define TIMER_SPEED  BPS_TO_TIMER(30*(cyclenum+2))

   install_int_ex(inc_counter, TIMER_SPEED);

   while (!gameover) {

      /* move everyone */
      while ((counter > 0) && (!gameover)) {
	 update_view();
	 update_bullets();
	 update_explode();
	 update_message();

	 if (update_badguys()) {
	    if (advance_view()) {
	       cyclenum++;
	       install_int_ex(inc_counter, TIMER_SPEED);
	       lay_attack_wave(TRUE);
	       advance_player(TRUE);
	    }
	    else {
	       lay_attack_wave(FALSE);
	       advance_player(FALSE);
	    }
	 }

	 gameover = update_player();

	 counter--;
      }

      /* take a screenshot? */
      if (key[KEY_PRTSCR]) {
	 static int ss_count = 0;

	 char fname[80];
	 PALETTE pal;
	 BITMAP *b;

	 sprintf(fname, "speed%03d.tga", ++ss_count);

	 get_palette(pal);
	 b = create_sub_bitmap(screen, 0, 0, SCREEN_W, SCREEN_H);
	 save_tga(fname, b, pal);
	 destroy_bitmap(b);

	 while (key[KEY_PRTSCR])
	    poll_keyboard();

	 counter = 0;
      }

      /* draw everyone */
      draw_view(bmp);
   }

   /* cleanup */
   remove_int(inc_counter);

   shutdown_view();
   shutdown_player();
   shutdown_badguys();
   shutdown_bullets();
   shutdown_explode();
   shutdown_message();

   destroy_bitmap(bmp);

   if (gameover < 0) {
      sfx_ping(1);
      return FALSE;
   }

   return TRUE;
}



/* for generating the 8 bit additive color lookup table */
static void add_blender8(PALETTE pal, int x, int y, RGB *rgb)
{
   int r, g, b;

   r = (int)pal[x].r + (int)pal[y].r;
   g = (int)pal[x].g + (int)pal[y].g;
   b = (int)pal[x].b + (int)pal[y].b;

   rgb->r = MIN(r, 63);
   rgb->g = MIN(g, 63);
   rgb->b = MIN(b, 63);
}



/* 15 bit additive color blender */
static unsigned long add_blender15(unsigned long x, unsigned long y, unsigned long n)
{
   int r = getr15(x) + getr15(y);
   int g = getg15(x) + getg15(y);
   int b = getb15(x) + getb15(y);

   r = MIN(r, 255);
   g = MIN(g, 255);
   b = MIN(b, 255);

   return makecol15(r, g, b);
}



/* 16 bit additive color blender */
static unsigned long add_blender16(unsigned long x, unsigned long y, unsigned long n)
{
   int r = getr16(x) + getr16(y);
   int g = getg16(x) + getg16(y);
   int b = getb16(x) + getb16(y);

   r = MIN(r, 255);
   g = MIN(g, 255);
   b = MIN(b, 255);

   return makecol16(r, g, b);
}



/* 24 bit additive color blender */
static unsigned long add_blender24(unsigned long x, unsigned long y, unsigned long n)
{
   int r = getr24(x) + getr24(y);
   int g = getg24(x) + getg24(y);
   int b = getb24(x) + getb24(y);

   r = MIN(r, 255);
   g = MIN(g, 255);
   b = MIN(b, 255);

   return makecol24(r, g, b);
}



/* display a commandline usage message */
static void usage()
{
   allegro_message(
      "\n"
      "SPEED - by Shawn Hargreaves, 1999\n"
      "\n"
      "Usage: speed w h bpp [options]\n"
      "\n"
      "The w and h values set your desired screen resolution.\n"
      "What modes are available will depend on your hardware.\n"
      "\n"
      "The bpp value sets the color depth: 8, 15, 16, 24, or 32.\n"
      "It runs _much_ faster in 8 bit mode.\n"
      "\n"
      "Available options:\n"
      "\n"
      "\t-cheat makes you invulnerable.\n"
      "\t-simple turns off the more expensive graphics effects.\n"
      "\t-nogrid turns off the wireframe background grid.\n"
      "\t-nomusic turns off my most excellent background music.\n"
      "\t-www invokes the built-in web browser.\n"
      "\n"
      "Example usage:\n"
      "\n"
      "\tspeed 640 480 8\n"
   );
}



/* the main program body */
int main(int argc, char *argv[])
{
   int w=0, h=0, bpp=0;
   int www = FALSE;
   PALETTE pal;
   int i, n;

   srand(time(NULL));

   allegro_init();

   /* parse the commandline */
   for (i=1; i<argc; i++) {
      if (stricmp(argv[i], "-cheat") == 0) {
	 cheat = TRUE;
      }
      else if (stricmp(argv[i], "-simple") == 0) {
	 low_detail = TRUE;
      }
      else if (stricmp(argv[i], "-nogrid") == 0) {
	 no_grid = TRUE;
      }
      else if (stricmp(argv[i], "-nomusic") == 0) {
	 no_music = TRUE;
      }
      else if (stricmp(argv[i], "-www") == 0) {
	 www = TRUE;
      }
      else {
	 n = atoi(argv[i]);

	 if (!n) {
	    usage();
	    return 1;
	 }

	 if (!w) {
	    w = n;
	 }
	 else if (!h) {
	    h = n;
	 }
	 else if (!bpp) {
	    bpp = n;
	 }
	 else {
	    usage();
	    return 1;
	 }
      }
   }

   /* it's a real shame that I had to take this out! */
   if (www) {
      allegro_message(
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

   if ((!w) || (!h) || (!bpp)) {
      usage();
      return 1;
   }

   /* set the screen mode */
   set_color_depth(bpp);

   if (set_gfx_mode(GFX_AUTODETECT, w, h, 0, 0) != 0) {
      allegro_message("Error setting %dx%d %d bpp display mode:\n%s\n",
		      w, h, bpp, allegro_error);
      return 1;
   }

   /* set up everything else */
   install_timer();
   install_keyboard();
   install_joystick(JOY_TYPE_AUTODETECT);
   install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL);

   init_sound();
   init_hiscore();

   /* additive color is cool */
   if (bpp == 8) {
      generate_332_palette(pal);

      pal[0].r = 0;
      pal[0].g = 0;
      pal[0].b = 0;

      set_palette(pal);

      rgb_map = malloc(sizeof(RGB_MAP));
      create_rgb_table(rgb_map, pal, NULL);

      color_map = malloc(sizeof(COLOR_MAP));
      create_color_table(color_map, pal, add_blender8, NULL);
   }
   else {
      set_blender_mode(add_blender15, add_blender16, add_blender24, 0, 0, 0, 0);
   }

   text_mode(-1);

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

   if (rgb_map) {
      free(rgb_map);
      rgb_map = NULL;
   }

   if (color_map) {
      free(color_map);
      color_map = NULL;
   }

   return 0;
}

END_OF_MAIN();
