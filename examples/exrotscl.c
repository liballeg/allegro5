#include <allegro.h>

enum MODES {
   NORMAL,
   ALPHA,
   LIT
};

int main(void)
{
   int mode = 0;

   allegro_init();
   set_color_depth(32);
   install_keyboard();
   set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0);

   /* Load the alpha bitmap. This is used for alpha and normal modes */
   set_color_conversion(COLORCONV_KEEP_ALPHA);
   BITMAP *b = load_bitmap("inkblot.tga", NULL);

   /* Make a non-alpha copy for drawing lit */
   BITMAP *b2 = create_bitmap(b->w, b->h);
   clear_to_color(b2, makeacol(255, 255, 255, 255));
   set_alpha_blender();
   draw_trans_sprite(b2, b, 0, 0);

   float fangle = 0;
   float fscale = 1;
   float inc = 0.002;
   int dir = -1;

   while (!key[KEY_ESC]) {
      acquire_screen();
      clear_to_color(screen, makecol(255, 255, 255));
      textout_centre_ex(screen, font, "Press 'n' for next mode", SCREEN_W/2, SCREEN_H-30, makecol(0, 0, 0), -1);
      fixed angle = ftofix(fangle);
      fixed scale = ftofix(fscale);
      float x = (SCREEN_W-(b->w*fscale))/2;
      float y = (SCREEN_H-(b->h*fscale))/2;
      switch (mode) {
         case NORMAL:
            rotate_scaled_sprite(screen, b, x, y, angle, scale);
            textout_centre_ex(screen, font, "Normal", SCREEN_W/2, SCREEN_H-20, makecol(0, 0, 0), -1);
            break;
         case ALPHA:
            set_alpha_blender();
            rotate_scaled_sprite_trans(screen, b, x, y, angle, scale);
            textout_centre_ex(screen, font, "Alpha", SCREEN_W/2, SCREEN_H-20, makecol(0, 0, 0), -1);
            break;
         case LIT:
            set_trans_blender(255, 0, 0, 0);
            rotate_scaled_sprite_lit(screen, b2, x, y, angle, scale,
               128);//makecol(255, 0, 0));
            textout_centre_ex(screen, font, "Lit", SCREEN_W/2, SCREEN_H-20, makecol(0, 0, 0), -1);
            break;
      }
      release_screen();
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
}
END_OF_MAIN()

