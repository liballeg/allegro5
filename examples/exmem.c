/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates the use of memory bitmaps. It creates
 *    a small temporary bitmap in memory, draws some circles onto it,
 *    and then blits lots of copies of it onto the screen.
 */


#include <allegro.h>



int main(void)
{
   BITMAP *memory_bitmap;
   int x, y;

   if (allegro_init() != 0)
      return 1;
   install_keyboard();

   if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
	 return 1;
      }
   }

   set_palette(desktop_palette);

   /* make a memory bitmap sized 20x20 */
   memory_bitmap = create_bitmap(20, 20);

   /* draw some circles onto it */
   clear_bitmap(memory_bitmap);
   for (x=0; x<16; x++)
      circle(memory_bitmap, 10, 10, x, palette_color[x]);

   /* blit lots of copies of it onto the screen */
   acquire_screen();

   for (y=0; y<SCREEN_H; y+=20)
      for (x=0; x<SCREEN_W; x+=20)
	 blit(memory_bitmap, screen, 0, 0, x, y, 20, 20);

   release_screen();

   /* free the memory bitmap */
   destroy_bitmap(memory_bitmap);

   readkey();
   return 0;
}

END_OF_MAIN()
