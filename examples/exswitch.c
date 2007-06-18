/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program shows how to control the console switching mode, and
 *    let your program run in the background. These functions don't apply
 *    to every platform and driver, for example you can't control the
 *    switching mode from a DOS program.
 *
 *    Yes, I know the fractal drawing is very slow: that's the point!
 *    This is so you can easily check whether it goes on working in the
 *    background after you switch away from the app.
 *
 *    Depending on the type of selected switching mode, you will see
 *    whether the contents of the screen are preserved or not.
 */


#include <math.h>

#include <allegro.h>



/* there is no particular reason to use sub-bitmaps here: I just do it as a
 * stress-test, to make sure the switching code will handle them correctly.
 */
BITMAP *text_area;
BITMAP *graphics_area;

int in_callback = 0;
int out_callback = 0;



/* timer callbacks should go on running when we are in background mode */
volatile int counter = 0;


void increment_counter(void)
{
   counter++;
}

END_OF_FUNCTION(increment_counter)



/* displays a text message in the scrolling part of the screen */
void show_msg(char *msg)
{
   acquire_bitmap(text_area);

   blit(text_area, text_area, 0, 8, 0, 0, text_area->w, text_area->h-8);
   rectfill(text_area, 0, text_area->h-8, text_area->w, text_area->h,
	    palette_color[0]);

   if (msg)
      textout_centre_ex(text_area, font, msg, text_area->w/2, text_area->h-8,
			palette_color[255], palette_color[0]);

   release_bitmap(text_area);
}



/* displays the current switch mode setting */
void show_switch_mode(void)
{
   switch (get_display_switch_mode()) {

      case SWITCH_NONE:
	 show_msg("Current mode is SWITCH_NONE");
	 break;

      case SWITCH_PAUSE:
	 show_msg("Current mode is SWITCH_PAUSE");
	 break;

      case SWITCH_AMNESIA:
	 show_msg("Current mode is SWITCH_AMNESIA");
	 break;

      case SWITCH_BACKGROUND:
	 show_msg("Current mode is SWITCH_BACKGROUND");
	 break;

      case SWITCH_BACKAMNESIA:
	 show_msg("Current mode is SWITCH_BACKAMNESIA");
	 break;

      default:
	 show_msg("Eeek! Unknown switch mode...");
	 break;
   }
}



/* callback for switching back to our program */
void switch_in_callback(void)
{
   in_callback++;
}



/* callback for switching away from our program */
void switch_out_callback(void)
{
   out_callback++;
}



/* changes the display switch mode */
void set_sw_mode(int mode)
{
   if (set_display_switch_mode(mode) != 0) {
      show_msg("Error changing switch mode");
      show_msg(NULL);
      return;
   }

   show_switch_mode();

   if (set_display_switch_callback(SWITCH_IN, switch_in_callback) == 0)
      show_msg("SWITCH_IN callback activated");
   else
      show_msg("SWITCH_IN callback not available");

   if (set_display_switch_callback(SWITCH_OUT, switch_out_callback) == 0)
      show_msg("SWITCH_OUT callback activated");
   else
      show_msg("SWITCH_OUT callback not available");

   show_msg(NULL);
}



/* draws some graphics, for no particular reason at all */
void draw_pointless_graphics(void)
{
   static int x=0, y=0;
   float zr, zi, cr, ci, tr, ti;
   int c;

   if ((!x) && (!y))
      clear_to_color(graphics_area, palette_color[255]);

   cr = ((float)x / (float)graphics_area->w - 0.75) * 2.0;
   ci = ((float)y / (float)graphics_area->h - 0.5) * 1.8;

   zr = 0;
   zi = 0;

   for (c=0; c<100; c++) {
      tr = zr*zr - zi*zi;
      ti = zr*zi*2;

      zr = tr + cr;
      zi = ti + ci;
      if ((zr < -10) || (zr > 10) || (zi < -10) || (zi > 10))
	 break;
   }

   if ((zi != zi) || (zr != zr))
      c = 0;
   else if ((zi <= -1) || (zi >= 1) || (zr <= -1) || (zr >= 1))
      c = 255;
   else {
      c = sqrt(zi*zi + zr*zr) * 256;
      if (c > 255)
	 c = 255;
   }

   putpixel(graphics_area, x, y, makecol(c, c, c));

   x++;
   if (x >= graphics_area->w) {
      x = 0;
      y++;
      if (y >= graphics_area->h)
	 y = 0;
   }
}



int main(void)
{
   PALETTE pal;
   int finished = FALSE;
   int last_counter = 0;
   int c = GFX_AUTODETECT;
   int w, h, bpp, i;

   if (allegro_init() != 0)
      return 1;
   install_keyboard();
   install_mouse();
   install_timer();

   if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
      return 1;
   }
   set_palette(desktop_palette);

   w = SCREEN_W;
   h = SCREEN_H;
   bpp = bitmap_color_depth(screen);
   if (!gfx_mode_select_ex(&c, &w, &h, &bpp)) {
      allegro_exit();
      return 1;
   }

   set_color_depth(bpp);

   if (set_gfx_mode(c, w, h, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error setting graphics mode\n%s\n", allegro_error);
      return 1;
   }

   for (i=0; i<256; i++)
      pal[i].r = pal[i].g = pal[i].b = i/4;

   set_palette(pal);

   text_area = create_sub_bitmap(screen, 0, 0, SCREEN_W, SCREEN_H/2);
   graphics_area = create_sub_bitmap(screen, 0, SCREEN_H/2, SCREEN_W/2,
				     SCREEN_H/2);
   if ((!text_area) || (!graphics_area)) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Out of memory!\n");
      return 1;
   }

   LOCK_VARIABLE(counter);
   LOCK_FUNCTION(increment_counter);

   install_int(increment_counter, 10);

   show_msg("Console switching test");
   show_msg("Press 1-5 to change mode");
   show_msg(NULL);

   show_switch_mode();
   show_msg(NULL);

   while (!finished) {
      if (counter != last_counter) {
	 last_counter = counter;

	 acquire_screen();
	 textprintf_centre_ex(screen, font, SCREEN_W*3/4, SCREEN_H*3/4,
			      palette_color[255], palette_color[0],
			      "Time: %d", last_counter);
	 release_screen();

	 acquire_bitmap(graphics_area);
	 for (i=0; i<10; i++)
	    draw_pointless_graphics();
	 release_bitmap(graphics_area);
      }

      if (keypressed()) {
	 switch (readkey() & 255) {

	    case '1':
	       set_sw_mode(SWITCH_NONE);
	       break;

	    case '2':
	       set_sw_mode(SWITCH_PAUSE);
	       break;

	    case '3':
	       set_sw_mode(SWITCH_AMNESIA);
	       break;

	    case '4':
	       set_sw_mode(SWITCH_BACKGROUND);
	       break;

	    case '5':
	       set_sw_mode(SWITCH_BACKAMNESIA);
	       break;

	    case 27:
	       finished = TRUE;
	       break;
	 }
      }

      while (in_callback > 0) {
	 in_callback--;
	 show_msg("SWITCH_IN callback");
	 show_msg(NULL);
      }

      while (out_callback > 0) {
	 out_callback--;
	 show_msg("SWITCH_OUT callback");
	 show_msg(NULL);
      }
   }

   destroy_bitmap(text_area);
   destroy_bitmap(graphics_area);

   return 0;
}

END_OF_MAIN()
