/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates how to get mouse input.
 */


#include <stdio.h>

#include "allegro.h"



int main()
{
   int mickeyx = 0;
   int mickeyy = 0;
   BITMAP *custom_cursor;
   char msg[80];
   int c = 0;

   allegro_init();
   install_keyboard(); 
   install_mouse();
   install_timer();
   if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
      return 1;
   }
   set_palette(desktop_palette);
   text_mode(makecol(255, 255, 255));
   clear_to_color(screen, makecol(255, 255, 255));

   textprintf_centre(screen, font, SCREEN_W/2, 8, makecol(0, 0, 0),
		     "Driver: %s", mouse_driver->name);

   do {
      /* On most platforms (eg. DOS) things will still work correctly
       * without this call, but it is a good idea to include it in any
       * programs that you want to be portable, because on some platforms
       * you may not be able to get any mouse input without it.
       */ 
      poll_mouse();

      acquire_screen();

      /* the mouse position is stored in the variables mouse_x and mouse_y */
      sprintf(msg, "mouse_x = %-5d", mouse_x);
      textout(screen, font, msg, 16, 48, makecol(0, 0, 0));

      sprintf(msg, "mouse_y = %-5d", mouse_y);
      textout(screen, font, msg, 16, 64, makecol(0, 0, 0));

      /* or you can use this function to measure the speed of movement.
       * Note that we only call it every fourth time round the loop: 
       * there's no need for that other than to slow the numbers down 
       * a bit so that you will have time to read them...
       */
      c++;
      if ((c & 3) == 0)
	 get_mouse_mickeys(&mickeyx, &mickeyy);

      sprintf(msg, "mickey_x = %-7d", mickeyx);
      textout(screen, font, msg, 16, 88, makecol(0, 0, 0));

      sprintf(msg, "mickey_y = %-7d", mickeyy);
      textout(screen, font, msg, 16, 104, makecol(0, 0, 0));

      /* the mouse button state is stored in the variable mouse_b */
      if (mouse_b & 1)
	 textout(screen, font, "left button is pressed ", 16, 128, makecol(0, 0, 0));
      else
	 textout(screen, font, "left button not pressed", 16, 128, makecol(0, 0, 0));

      if (mouse_b & 2)
	 textout(screen, font, "right button is pressed ", 16, 144, makecol(0, 0, 0));
      else
	 textout(screen, font, "right button not pressed", 16, 144, makecol(0, 0, 0));

      if (mouse_b & 4)
	 textout(screen, font, "middle button is pressed ", 16, 160, makecol(0, 0, 0));
      else
	 textout(screen, font, "middle button not pressed", 16, 160, makecol(0, 0, 0));

      /* the wheel position is stored in the variable mouse_z */
      sprintf(msg, "mouse_z = %-5d", mouse_z);
      textout(screen, font, msg, 16, 184, makecol(0, 0, 0));

      release_screen();

      vsync();

   } while (!keypressed());

   clear_keybuf();

   /*  To display a mouse pointer, call show_mouse(). There are several 
    *  things you should be aware of before you do this, though. For one,
    *  it won't work unless you call install_timer() first. For another,
    *  you must never draw anything onto the screen while the mouse
    *  pointer is visible. So before you draw anything, be sure to turn 
    *  the mouse off with show_mouse(NULL), and turn it back on again when
    *  you are done.
    */
   clear_to_color(screen, makecol(255, 255, 255));
   textout_centre(screen, font, "Press a key to change cursor",
		  SCREEN_W/2, SCREEN_H/2, makecol(0, 0, 0));
   show_mouse(screen);
   readkey();
   show_mouse(NULL);

   /* create a custom mouse cursor bitmap... */
   custom_cursor = create_bitmap(32, 32);
   clear_to_color(custom_cursor, bitmap_mask_color(screen));
   for (c=0; c<8; c++)
      circle(custom_cursor, 16, 16, c*2, palette_color[c]);

   /* select the custom cursor and set the focus point to the middle of it */
   set_mouse_sprite(custom_cursor);
   set_mouse_sprite_focus(16, 16);

   clear_to_color(screen, makecol(255, 255, 255));
   textout_centre(screen, font, "Press a key to quit", SCREEN_W/2,
		  SCREEN_H/2, makecol(0, 0, 0));
   show_mouse(screen);
   readkey();
   show_mouse(NULL);

   destroy_bitmap(custom_cursor);

   return 0;
}

END_OF_MAIN();
