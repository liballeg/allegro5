/*
 *    Example program for the Allegro library, by Trent Gamblin.
 *
 *    This example demonstrates rotate_scaled_sprite functions.
 */


#include <allegro.h>


enum MODES {
   NORMAL,
   ALPHA,
   LIT
};


int main(void)
{
   BITMAP *dbuf;
   BITMAP *b;
   BITMAP *b2;
   int mode = NORMAL;
   float fangle = 0;
   float fscale = 1;
   float inc = 0.002;
   int dir = -1;

   if (allegro_init() != 0) {
      return 1;
   }
   set_color_depth(32);
   install_keyboard();
   if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0) != 0) {
      allegro_message("Could not set graphics mode\n");
      return 1;
   }

   dbuf = create_bitmap(SCREEN_W, SCREEN_H);
   if (!dbuf) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Could not create double buffer\n");
      return 1;
   }

   /* Load the alpha bitmap. This is used for alpha and normal modes */
   set_color_conversion(COLORCONV_KEEP_ALPHA);
   b = load_bitmap("inkblot.tga", NULL);
   if (!b) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Could not load inkblot.tga\n");
      return 1;
   }

   /* Make a non-alpha copy for drawing lit */
   b2 = create_bitmap(b->w, b->h);
   if (!b2) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error creating bitmap\n");
      return 1;
   }
   clear_to_color(b2, makeacol(255, 255, 255, 255));
   set_alpha_blender();
   draw_trans_sprite(b2, b, 0, 0);

   while (!key[KEY_ESC]) {
      fixed angle = ftofix(fangle);
      fixed scale = ftofix(fscale);
      float x = (SCREEN_W-(b->w*fscale))/2;
      float y = (SCREEN_H-(b->h*fscale))/2;

      /* XXX flickers badly due to no buffering */
      clear_to_color(dbuf, makecol(255, 255, 255));
      textout_centre_ex(dbuf, font, "Press 'n' for next mode", SCREEN_W/2, SCREEN_H-30, makecol(0, 0, 0), -1);
      switch (mode) {
         case NORMAL:
            rotate_scaled_sprite(dbuf, b, x, y, angle, scale);
            textout_centre_ex(dbuf, font, "Normal", SCREEN_W/2, SCREEN_H-20, makecol(0, 0, 0), -1);
            break;
         case ALPHA:
            set_alpha_blender();
            rotate_scaled_sprite_trans(dbuf, b, x, y, angle, scale);
            textout_centre_ex(dbuf, font, "Alpha", SCREEN_W/2, SCREEN_H-20, makecol(0, 0, 0), -1);
            break;
         case LIT:
            set_trans_blender(255, 0, 0, 0);
            rotate_scaled_sprite_lit(dbuf, b2, x, y, angle, scale, 128);
            textout_centre_ex(dbuf, font, "Lit", SCREEN_W/2, SCREEN_H-20, makecol(0, 0, 0), -1);
            break;
      }
      blit(dbuf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

      fangle += 1;
      fscale += dir*inc;
      if (fscale < 0.5) {
         dir = -dir;
         fscale = 0.5;
      }
      else if (fscale > 1) {
         dir = -dir;
         fscale = 1;
      }

      rest(5);

      if (keypressed()) {
         int k = readkey();
         switch (k >> 8) {
            case KEY_N:
               mode++;
               mode %= 3;
               break;
         }
      }
   }

   destroy_bitmap(b);
   destroy_bitmap(b2);
   destroy_bitmap(dbuf);
   return 0;
}
END_OF_MAIN()

