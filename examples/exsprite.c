/*
 *    Example program for the Allegro library, by Grzegorz Ludorowski.
 *
 *    This example demonstrates how to use datafiles, various sprite
 *    drawing routines and flicker-free animation.
 *
 *    Why is the animate() routine coded in that way?  As you
 *    probably know, VIDEO RAM is much slower than "normal"
 *    RAM, so it's advisable to reduce VRAM blits to a minimum.
 *    Drawing sprite on the screen (meaning in VRAM) and then
 *    clearing a background for it is not very fast. This example
 *    uses a different method which is much faster, but require a
 *    bit more memory.
 *
 *    First the buffer is cleared (it's a normal BITMAP), then the
 *    sprite is drawn on it, and when the drawing is finished this
 *    buffer is copied directly to the screen. So the end result is
 *    that there is a single VRAM blit instead of blitting/clearing
 *    the background and drawing a sprite on it.  It's a good method
 *    even when you have to restore the background. And of course,
 *    it completely removes any flickering effect.
 *
 *    When one uses a big (ie. 800x600 background) and draws
 *    something on it, it's wise to use a copy of background
 *    somewhere in memory and restore background using this
 *    "virtual background". When blitting from VRAM in SVGA modes,
 *    it's probably, that drawing routines have to switch banks on
 *    video card. I think, I don't have to remind how slow is it.
 *
 *    Note that on modern systems, the above isn't true anymore, and
 *    you usually get the best performance by caching all your
 *    animations in video ram and doing only VRAM->VRAM blits, so
 *    there is no more RAM->VRAM transfer at all anymore. And usually,
 *    such transfers can run in parallel on the graphics card's
 *    processor as well, costing virtually no main cpu time at all.
 *    See the exaccel example for an example of this.
 */

#include <math.h>
#include <allegro.h>
#include "running.h"



#define FRAMES_PER_SECOND 30

/* set up a timer for animation */
volatile int ticks = 0;
void ticker(void)
{
    ticks++;
}
END_OF_FUNCTION(ticker)

/* pointer to data file */
DATAFILE *running_data;

/* current sprite frame number */
int frame, frame_number = 0;

/* pointer to a sprite buffer, where sprite will be drawn */
BITMAP *sprite_buffer;

/* a boolean - if true, skip to next part */
int next;



void animate(void)
{
   /* Wait for animation timer. */
   while (frame > ticks) {
       /* Avoid busy wait. */
       rest(1);
   }

   /* Ideally, instead of using a timer, we would set the monitor refresh rate
    * to a multiple of the animation speed, and synchronize with the vertical
    * blank interrupt (vsync) - to get a completely smooth animation. But this
    * doesn't work on all setups (e.g. in X11 windowed modes), so should only
    * be used after performing some tests first or letting the user enable it.
    * Too much for this simple example
    */

   frame++;

   /* blits sprite buffer to screen */
   blit(sprite_buffer, screen, 0, 0, (SCREEN_W - sprite_buffer->w) / 2,
	(SCREEN_H - sprite_buffer->h) / 2, sprite_buffer->w, sprite_buffer->h);

   /* clears sprite buffer with color 0 */
   clear_bitmap(sprite_buffer);

   /* if key pressed set a next flag */
   if (keypressed())
      next = TRUE;
   else
      next = FALSE;

   if (frame_number == 0)
      play_sample(running_data[SOUND_01].dat, 128, 128, 1000, FALSE);

   /* increase frame number, or if it's equal 9 (last frame) set it to 0 */
   if (frame_number == 9)
      frame_number = 0;
   else
      frame_number++;
}



int main(int argc, char *argv[])
{
   char datafile_name[256];
   int angle = 0;
   int x, y;
   int text_y;
   int color;

   if (allegro_init() != 0)
      return 1;
   install_keyboard();
   install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL);
   install_timer();
   LOCK_FUNCTION(ticker);
   LOCK_VARIABLE(ticks);
   install_int_ex(ticker, BPS_TO_TIMER(30));

   if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to set any graphic mode\n%s\n",
			 allegro_error);
	 return 1;
      }
   }

   /* loads datafile and sets user palette saved in datafile */
   replace_filename(datafile_name, argv[0], "running.dat",
		    sizeof(datafile_name));
   running_data = load_datafile(datafile_name);
   if (!running_data) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error loading %s!\n", datafile_name);
      return 1;
   }

   /* select the palette which was loaded from the datafile */
   set_palette(running_data[PALETTE_001].dat);

   /* create and clear a bitmap for sprite buffering, big
    * enough to hold the diagonal(sqrt(2)) when rotating */
   sprite_buffer = create_bitmap((int)(82 * sqrt(2) + 2),
      (int)(82 * sqrt(2) + 2));
   clear_bitmap(sprite_buffer);

   x = (sprite_buffer->w - 82) / 2;
   y = (sprite_buffer->h - 82) / 2;
   color = makecol(0, 80, 0);
   text_y = SCREEN_H - 10 - text_height(font);

   frame = ticks;

   /* write current sprite drawing method */
   textout_centre_ex(screen, font, "Press a key for next part...",
	      SCREEN_W / 2, 10, palette_color[1], -1);
   textout_centre_ex(screen, font, "Using draw_sprite",
		     SCREEN_W / 2, text_y, palette_color[15], -1);

   do {
      hline(sprite_buffer, 0, y + 82, sprite_buffer->w - 1, color);
      draw_sprite(sprite_buffer, running_data[frame_number].dat, x, y);
      animate();
   } while (!next);

   clear_keybuf();
   rectfill(screen, 0, text_y, SCREEN_W, SCREEN_H, 0);
   textout_centre_ex(screen, font, "Using draw_sprite_h_flip",
		     SCREEN_W / 2, text_y, palette_color[15], -1);

   do {
      hline(sprite_buffer, 0, y + 82, sprite_buffer->w - 1, color);
      draw_sprite_h_flip(sprite_buffer, running_data[frame_number].dat, x, y);
      animate();
   } while (!next);

   clear_keybuf();
   rectfill(screen, 0, text_y, SCREEN_W, SCREEN_H, 0);
   textout_centre_ex(screen, font, "Using draw_sprite_v_flip",
		     SCREEN_W / 2, text_y, palette_color[15], -1);

   do {
      hline(sprite_buffer, 0, y - 1, sprite_buffer->w - 1, color);
      draw_sprite_v_flip(sprite_buffer, running_data[frame_number].dat, x, y);
      animate();
   } while (!next);

   clear_keybuf();
   rectfill(screen, 0, text_y, SCREEN_W, SCREEN_H, 0);
   textout_centre_ex(screen, font, "Using draw_sprite_vh_flip",
		     SCREEN_W / 2, text_y, palette_color[15], -1);

   do {
      hline(sprite_buffer, 0, y - 1, sprite_buffer->w - 1, color);
      draw_sprite_vh_flip(sprite_buffer, running_data[frame_number].dat, x, y);
      animate();
   } while (!next);

   clear_keybuf();
   rectfill(screen, 0, text_y, SCREEN_W, SCREEN_H, 0);
   textout_centre_ex(screen, font, "Now with rotating - pivot_sprite",
		     SCREEN_W / 2, text_y, palette_color[15], -1);

   do {
      /* The last argument to pivot_sprite() is a fixed point type,
       * so I had to use itofix() routine (integer to fixed).
       */
      circle(sprite_buffer, x + 41, y + 41, 47, color);
      pivot_sprite(sprite_buffer, running_data[frame_number].dat, sprite_buffer->w / 2,
	 sprite_buffer->h / 2, 41, 41, itofix(angle));
      animate();
      angle -= 4;
   } while (!next);

   clear_keybuf();
   rectfill(screen, 0, text_y, SCREEN_W, SCREEN_H, 0);
   textout_centre_ex(screen, font, "Now using pivot_sprite_v_flip",
		     SCREEN_W / 2, text_y, palette_color[15], -1);

   do {
      /* The last argument to pivot_sprite_v_flip() is a fixed point type,
       * so I had to use itofix() routine (integer to fixed).
       */
      circle(sprite_buffer, x + 41, y + 41, 47, color);
      pivot_sprite_v_flip(sprite_buffer, running_data[frame_number].dat,
	 sprite_buffer->w / 2, sprite_buffer->h / 2, 41, 41, itofix(angle));
      animate();
      angle += 4;
   } while (!next);

   unload_datafile(running_data);
   destroy_bitmap(sprite_buffer);
   return 0;
}

END_OF_MAIN()
