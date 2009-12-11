/*
 *    Example program for the Allegro library, by Grzegorz Ludorowski.
 *
 *    This example demonstrates how to use PCX files, palettes and stretch
 *    blits. It loads a PCX file, sets its palette and does some random
 *    stretch_blits. Don't worry - it's VERY slowed down using vsync().
 */


#include <allegro.h>



int main(int argc, char *argv[])
{
   PALETTE my_palette;
   BITMAP *scr_buffer;
   char pcx_name[256];

   if (allegro_init() != 0)
      return 1;
   install_keyboard();

   if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 640, 480, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
	 return 1;
      }
   }

   replace_filename(pcx_name, argv[0], "mysha.pcx", sizeof(pcx_name));
   scr_buffer = load_pcx(pcx_name, my_palette);
   if (!scr_buffer) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error loading %s!\n", pcx_name);
      return 1;
   }

   set_palette(my_palette);
   blit(scr_buffer, screen, 0, 0, 0, 0, scr_buffer->w, scr_buffer->h);

   while (!keypressed()) {
      stretch_blit(scr_buffer, screen, 0, 0, AL_RAND()%scr_buffer->w,
		   AL_RAND()%scr_buffer->h, AL_RAND()%SCREEN_W, AL_RAND()%SCREEN_H,
		   AL_RAND()%SCREEN_W, AL_RAND()%SCREEN_H);
      vsync();
   }

   destroy_bitmap(scr_buffer);
   return 0;
}

END_OF_MAIN()
