/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      FLI player test program for the Allegro library.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"



void usage(void)
{
   allegro_message("\nFLI player test program for Allegro " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR "\n"
		   "By Shawn Hargreaves, " ALLEGRO_DATE_STR "\n\n"
		   "Usage: playfli [options] filename.(fli|flc)\n\n"
		   "Options:\n"
		   "\t'-loop' cycles the animation until a key is pressed\n"
		   "\t'-step' selects single-step mode\n"
		   "\t'-mode screen_w screen_h' sets the screen mode\n");
}



int loop = FALSE;
int step = FALSE;



int key_checker(void)
{
   if (step) {
      if ((readkey() & 0xFF) == 27)
	 return 1;
      else
	 return 0;
   }
   else {
      if (keypressed())
	 return 1;
      else
	 return 0;
   }
}



int main(int argc, char *argv[])
{
   int w = 320;
   int h = 200;
   int c, ret;

   if (allegro_init() != 0)
      return 1;

   if (argc < 2) {
      usage();
      return 1;
   }

   for (c=1; c<argc-1; c++) {
      if (stricmp(argv[c], "-loop") == 0)
	 loop = TRUE;
      else if (stricmp(argv[c], "-step") == 0)
	 step = TRUE;
      else if ((stricmp(argv[c], "-mode") == 0) && (c<argc-3)) {
	 w = atoi(argv[c+1]);
	 h = atoi(argv[c+2]);
	 c += 2;
      }
      else {
	 usage();
	 return 1;
      }
   }

   install_keyboard();
   install_timer();

   if (set_gfx_mode(GFX_AUTODETECT, w, h, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error setting graphics mode\n%s\n", allegro_error);
      return 1;
   }

   ret = play_fli(argv[c], screen, loop, key_checker);

   if (ret < 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error playing FLI file\n");
      return 1;
   }

   return 0;
}

END_OF_MAIN()
