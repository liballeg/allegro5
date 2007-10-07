#include <allegro.h>
#include <alleggl.h>


#define MAX_IMAGES      256


/* structure to hold the current position and velocity of an image */
typedef struct IMAGE
{
   float x, y;
   float dx, dy;
} IMAGE;


int blender = 0;
char *blender_name = "alpha";


/* initialises an image structure to a random position and velocity */
void init_image(IMAGE *image)
{
   image->x = (float)(AL_RAND() % 704);
   image->y = (float)(AL_RAND() % 568);
   image->dx = (float)((AL_RAND() % 255) - 127) / 32.0;
   image->dy = (float)((AL_RAND() % 255) - 127) / 32.0;
}


volatile int chrono = 0;

void the_timer(void) {
        chrono++;
} END_OF_FUNCTION(the_timer);


/* called once per frame to bounce an image around the screen */
void update_image(IMAGE *image)
{
   image->x += image->dx;
   image->y += image->dy;

   if (((image->x < 0) && (image->dx < 0)) ||
       ((image->x > SCREEN_W-300) && (image->dx > 0)))
      image->dx *= -1;

   if (((image->y < 0) && (image->dy < 0)) ||
       ((image->y > SCREEN_H-200) && (image->dy > 0)))
      image->dy *= -1;
}



int main(int argc, char *argv[])
{
   BITMAP *tmp, *image;
   BITMAP *vimage;
   PALETTE pal;
   FONT *agl_font;
   IMAGE images[MAX_IMAGES];
   int num_images = 4;
   int done = FALSE;
   int i, x, y;
   int frame_count = 0, frame_count_time = 0;
   float fps_rate = 0.0;

   if (allegro_init() != 0)
      return 1;
   if (install_allegro_gl() != 0)
       return 1;
   install_keyboard();
   install_timer();

   LOCK_FUNCTION(the_timer);
   LOCK_VARIABLE(chrono);

   install_int(the_timer, 5);

   allegro_gl_set(AGL_DOUBLEBUFFER, 1);
   allegro_gl_set(AGL_WINDOWED, TRUE);
   allegro_gl_set(AGL_COLOR_DEPTH, 32);
   allegro_gl_set(AGL_SUGGEST, AGL_DOUBLEBUFFER | AGL_WINDOWED | AGL_COLOR_DEPTH);
   if(set_gfx_mode(GFX_OPENGL, 800, 600, 0, 0)) {
       allegro_message ("Error setting OpenGL graphics mode:\n%s\n"
                        "Allegro GL error : %s\n",
                        allegro_error, allegro_gl_error);
       exit(0);
   }

   /* Convert Allegro font */
   agl_font = allegro_gl_convert_allegro_font_ex(font,
               AGL_FONT_TYPE_TEXTURED, -1.0, GL_INTENSITY8);

   /* read in the source graphic */
   set_color_conversion(COLORCONV_NONE);
   tmp = load_bitmap("mysha.pcx", pal);
   set_palette(pal);

   image = create_bitmap_ex(32, tmp->w, tmp->h);
   blit(tmp, image, 0, 0, 0, 0, tmp->w, tmp->h);

   /* Generate alpha channel from greyscale of the image. */
   for (y = 0; y < image->h; ++y) {
        for (x = 0; x < image->w; ++x){
            int col = _getpixel32(image, x, y);
            int a = getr32(col) + getg32(col) + getb32(col);
            a = MID(0, a/2-128, 255);
            _putpixel32(image, x, y, makeacol32(getr32(col), getg32(col), getb32(col), a));
        }
   }

   /* initialise the images to random positions */
   for (i=0; i<MAX_IMAGES; i++)
      init_image(images+i);

   /* create a video memory bitmap to store our picture */
   allegro_gl_set_video_bitmap_color_depth(32);
   vimage = create_video_bitmap(image->w, image->h);
   blit(image, vimage, 0, 0, 0, 0, image->w, image->h);

   set_alpha_blender();

   allegro_gl_set_projection();
   glMatrixMode(GL_MODELVIEW);

   while (!done) {
      glClear(GL_COLOR_BUFFER_BIT);
      glLoadIdentity();

      /* Draw background pattern. */
      for (y = 0; y < SCREEN_H; y += 50) {
         for (x = 0; x < SCREEN_W; x += 50) {
            int sx = (x / 50) & 1;
            int sy = (y / 50) & 1;
            float c = sx ^ sy;
            glColor3f(c, c, c);
            glBegin(GL_QUADS);
            glVertex2d(x, y);
            glVertex2d(x + 50, y);
            glVertex2d(x + 50, y + 50);
            glVertex2d(x, y + 50);
            glEnd();
         }
      }

      /* draw onto screen */
      for (i=0; i<num_images; i++)
         draw_trans_sprite(screen, vimage, images[i].x, images[i].y);

      /* deal with keyboard input */
      while (keypressed()) {
         switch (readkey()>>8) {

            case KEY_UP:
            case KEY_RIGHT:
               if (num_images < MAX_IMAGES)
               num_images++;
            break;

            case KEY_DOWN:
            case KEY_LEFT:
               if (num_images > 0)
               num_images--;
            break;

            case KEY_SPACE:
               if (blender < 6)
                  blender++;
               else
                  blender = 0;

               switch (blender) {
                  case 0:
                     set_alpha_blender();
                     blender_name = "alpha";
                  break;
                  case 1:
                     set_trans_blender(128, 128, 128, 128);
                     blender_name = "trans";
                  break;
                  case 2:
                     set_add_blender(128, 128, 128, 128);
                     blender_name = "add";
                  break;
                  case 3:
                     set_burn_blender(128, 128, 128, 128);
                     blender_name = "burn";
                  break;
                  case 4:
                     set_dodge_blender(128, 128, 128, 128);
                     blender_name = "dodge";
                  break;
                  case 5:
                     set_invert_blender(128, 128, 128, 128);
                     blender_name = "invert";
                  break;
                  case 6:
                     set_multiply_blender(128, 128, 128, 128);
                     blender_name = "multiply";
                  break;
               }
            break;

            case KEY_ESC:
               done = TRUE;
            break;
         }
      }

      /* bounce the images around the screen */
      for (i=0; i<num_images; i++)
         update_image(images+i);

      /* calculate and display framerate */
      frame_count++;
      if (frame_count >= 20) {
         if (chrono > frame_count_time)
             fps_rate = frame_count * 200.0 / (chrono - frame_count_time);
         else
             fps_rate = frame_count * 200.0;

         frame_count_time = chrono;
         frame_count = 0;
      }

      glBegin(GL_QUADS);
         glColor3ub(0, 0, 0);
         glVertex2f(0, 0);
         glVertex2f(0, 50);
         glVertex2f(300, 50);
         glVertex2f(300, 0);
      glEnd();
      glEnable(GL_TEXTURE_2D);
      allegro_gl_printf(agl_font, 0, 0, 0, makecol(255, 255, 255), "FPS: %.2f", fps_rate);
      allegro_gl_printf(agl_font, 0, 10, 0, makecol(255, 255, 255), "image count: %i (arrow keys to change)", num_images);
      allegro_gl_printf(agl_font, 0, 20, 0, makecol(255, 255, 255), "using %s blender (space key to change)", blender_name);
      glDisable(GL_TEXTURE_2D);

      allegro_gl_flip();
   }

   destroy_bitmap(tmp);
   destroy_bitmap(image);
   destroy_bitmap(vimage);
   destroy_font(agl_font);

   return 0;
}

END_OF_MAIN()
