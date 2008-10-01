/*
 *    Example program for the Allegro library, by Lennart Steinke.
 *
 *    This is a very simple program showing how to use the Allegro
 *    configuration (ini file) routines. A first look at the example
 *    shows nothing more than a static graphic and the wait for
 *    a key press. However, the way this graphic is displayed is
 *    configured through a custom exconfig.ini file which is loaded
 *    manually. From this file the example obtains parameters like
 *    fullscreen/windowed mode, a specific graphic resolution to set
 *    up, which graphic to show, how to blit it on the screen, etc.
 */


#include <allegro.h>


int main(void)
{
   int w,h,bpp;
   int windowed;
   int count;
   char **data;

   char *title;
   char *filename;
   int  r,g,b;

   BITMAP *background;
   int     display;
   RGB     pal[256];

   int    x, y;

   /* you should always do this at the start of Allegro programs */
   if (allegro_init() != 0)
      return 1;
   /* set up the keyboard handler */
   install_keyboard(); 

   /* save the current ini file, then set the program specific one */
   push_config_state();
   set_config_file("exconfig.ini");

   /* the gfx mode is stored like this:
    *    640  480 16
    * the get_config_argv() function returns a pointer to a char
    * array, and stores the size of the char array in an int
    */
   data = get_config_argv("graphics", "mode", &count);
   if (count != 3) {
      /* We expect only 3 parameters */
      allegro_message("Found %i parameters in graphics.mode instead of "
		      "the 3 expected.\n", count);
      w = 320;
      h = 200;
      bpp = 8;
   }
   else {
      w = atoi(data[0]);
      h = atoi(data[1]);
      bpp = atoi(data[2]);       
   }

   /* Should we use a windowed mode? 
    * In the config file this is stored as either FALSE or TRUE.
    * So we need to read a string and see what it contains.
    * If the entry is not found, we use "FALSE" by default
    */
   if (ustricmp(get_config_string("graphics", "windowed", "FALSE"), "FALSE") == 0)
      windowed = GFX_AUTODETECT_FULLSCREEN;
   else
      windowed = GFX_AUTODETECT_WINDOWED;

   /* the title string 
    * The string returned is stored inside of the config system
    * and would be lost if we call pop_config_state(), so we create
    * a copy of it.
    */
   title = ustrdup(get_config_string("content", "headline", "<no headline>"));

   /* the title color 
    * once again this is stored as three ints in one line
    */
   data = get_config_argv("content", "headercolor", &count);
   if (count != 3) {
      /* We expect only 3 parameters */
      allegro_message("Found %i parameters in content.headercolor "
		      "instead of the 3 expected.\n", count);
      r = g = b = 255;
   }
   else {
      r = atoi(data[0]);
      g = atoi(data[1]);
      b = atoi(data[2]);       
   }

   /* The image file to read 
    * The string returned is stored inside of the config system
    * and would be lost if we call pop_config_state(), so we create
    * a copy of it.
    */   
   filename = ustrdup(get_config_string("content", "image", "mysha.pcx"));

   /* and it's tiling mode */
   display = get_config_int("content", "display", 0);
   if (display <0 || display > 2) {
      allegro_message("content.display must be within 0..2\n");
      display = 0; 
   } 

   /* restore the old config file */
   pop_config_state();


   /* set the graphics mode */
   set_color_depth(bpp);
   if (set_gfx_mode(windowed, w, h, 0, 0) != 0) {
      allegro_message("Unable to set mode %ix%i with %ibpp\n", w, h, bpp);
      free(filename);
      free(title);
      exit(-1);           
   }         

   /* Clear the screen */
   clear_bitmap(screen);   

   /* load the image */
   background = load_bitmap(filename, pal);
   if (background != NULL) {
      set_palette(pal);

      switch (display) {

	 case 0: /* stretch */
	    stretch_blit(background, screen, 0, 0, background->w,
			 background->h, 0, 0, SCREEN_W, SCREEN_H);
	 break;

	 case 1: /* center */
	    blit(background, screen, 0, 0, (SCREEN_W - background->w)/2,
		 (SCREEN_H - background->h)/2, background->w, background->h);
	 break;

	 case 2: /* tile */
	    for (y = 0; y < SCREEN_H; y += background->h)
	       for (x = 0; x < SCREEN_W; x += background->w)
		  blit(background, screen, 0, 0, x, y,
		       background->w, background->h);
         break; 
      }

      destroy_bitmap(background);
   }
   else {
      textprintf_centre_ex(screen, font, SCREEN_W/2, SCREEN_H/2,
			   makecol(r,g,b), -1, "%s not found", filename);
   }

   textout_centre_ex(screen, font, title, SCREEN_W/2, 20, makecol(r,g,b), -1);

   readkey();

   free(filename);
   free(title);

   return 0;
}
END_OF_MAIN()
