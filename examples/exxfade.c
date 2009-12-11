/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates how to load and display bitmap files
 *    in truecolor video modes, and how to crossfade between them.
 *    You have to use this example from the command line to specify
 *    as parameters a number of graphic files. Use at least two
 *    files to see the graphical effect. The example will crossfade
 *    from one image to another with each key press until you press
 *    the ESC key.
 */


#include <allegro.h>


const char *default_argv[] = {
   "exxfade", "allegro.pcx", "mysha.pcx", "planet.pcx"};



int show(const char *name)
{
   BITMAP *bmp, *buffer;
   PALETTE pal;
   int alpha;

   /* load the file */
   bmp = load_bitmap(name, pal);
   if (!bmp)
      return -1;

   buffer = create_bitmap(SCREEN_W, SCREEN_H);
   blit(screen, buffer, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

   set_palette(pal);

   /* fade it in on top of the previous picture */
   for (alpha=0; alpha<256; alpha+=8) {
      set_trans_blender(0, 0, 0, alpha);
      draw_trans_sprite(buffer, bmp,
			(SCREEN_W-bmp->w)/2, (SCREEN_H-bmp->h)/2);
      vsync();
      blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
      if (keypressed()) {
	 destroy_bitmap(bmp);
	 destroy_bitmap(buffer);
	 if ((readkey() & 0xFF) == 27)
	    return 1;
	 else
	    return 0;
      }

      /* slow down animation for modern machines */
      rest(2);
   }

   blit(bmp, screen, 0, 0, (SCREEN_W-bmp->w)/2, (SCREEN_H-bmp->h)/2,
	bmp->w, bmp->h);

   destroy_bitmap(bmp);
   destroy_bitmap(buffer);

   if ((readkey() & 0xFF) == 27)
      return 1;
   else
      return 0;
}



int main(int argc, const char *argv[])
{
   int i;

   if (allegro_init() != 0)
      return 1;

   if (argc < 2) {
      /* use a default set of images if the user doesn't specify any */
      argc = sizeof(default_argv) / sizeof(default_argv[0]);
      argv = default_argv;
   }

   install_keyboard(); 

   /* set the best color depth that we can find */
   set_color_depth(16);
   if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
      set_color_depth(15);
      if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
	 set_color_depth(32);
	 if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
	    set_color_depth(24);
	    if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
	       set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	       allegro_message("Error setting graphics mode\n%s\n",
			       allegro_error);
	       return 1;
	    }
	 }
      }
   }

   /* load all images in the same color depth as the display */
   set_color_conversion(COLORCONV_TOTAL);

   /* process all the files on our command line */
   i=1;
   for (;;) {
      switch (show(argv[i])) {

	 case -1:
	    /* error */
	    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	    allegro_message("Error loading image file '%s'\n", argv[i]);
	    return 1;

	 case 0:
	    /* ok! */
	    break;

	 case 1:
	    /* quit */
	    allegro_exit();
	    return 0;
      }

      if (++i >= argc)
         i=1;
   }

   return 0;
}

END_OF_MAIN()
