#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_ttf.h>

#include "common.c"

/* How far has the text been scrolled. */
static float scroll_y;

/* Total length of the scrolling text in pixels. */
static int text_length;

/* The Alex logo. */
static A5O_BITMAP *logo;

/* The star particle. */
static A5O_BITMAP *particle;

/* The font we use for everything. */
static A5O_FONT *font;


/* Local pseudo-random number generator. */
static int rnd(int *seed)
{
   *seed = (*seed + 1) * 1103515245 + 12345;
   return ((*seed >> 16) & 0xffff);
}


/* Load a bitmap or font and exit with a message if it's missing. */
static void *load(char const *path, char const *type, ...)
{
   void *data = NULL;
   va_list args;
   va_start(args, type);
   if (!strcmp(type, "bitmap")) {
      data = al_load_bitmap(path);
   }
   else if (!strcmp(type, "font")) {
      int size = va_arg(args, int);
      int flags = va_arg(args, int);
      data = al_load_font(path, size, flags);
   }
   va_end(args);
   if (!data) {
      abort_example("Could not load %s %s", type, path);
   }
   return data;
}


/* Print fading text. */
static int print(A5O_FONT *font, float x, float y, float r, float g,
   float b, float fade, char const *text)
{
   float c = 1 + (y - fade) / 360 / 2.0;
   if (c > 1)
      c = 1;
   if (c < 0)
      c = 0;
   al_draw_text(font, al_map_rgba_f(c * r, c * g, c * b, c), x, y,
      A5O_ALIGN_CENTER, text);
   return y + al_get_font_line_height(font);
}


/* Set up a perspective transform. We make the screen span
 * 180 vertical units with square pixel aspect and 90° vertical
 * FoV.
 */
static void setup_3d_projection(A5O_TRANSFORM *projection)
{
   A5O_DISPLAY *display = al_get_current_display();
   int dw = al_get_display_width(display);
   int dh = al_get_display_height(display);
   al_perspective_transform(projection, -180 * dw / dh, -180, 180,
      180 * dw / dh, 180, 3000);
   al_use_projection_transform(projection);
}


/* 3D transformations make it very easy to draw a starfield. */
static void draw_stars(void)
{
   A5O_TRANSFORM projection;
   int seed;
   int i;

   al_set_blender(A5O_ADD, A5O_ONE, A5O_ONE);

   seed = 0;
   for (i = 0; i < 100; i++) {
      const int x = rnd(&seed);
      const int y = rnd(&seed);
      const int z = rnd(&seed);

      al_identity_transform(&projection);
      al_translate_transform_3d(&projection, 0, 0,
         -2000 + ((int)scroll_y * 1000 / text_length + z) % 2000 - 180);
      setup_3d_projection(&projection);
      al_draw_bitmap(particle, x % 4000 - 2000, y % 2000 - 1000, 0);
   }
}


/* The main part of this example. */
static void draw_scrolling_text(void)
{
   A5O_TRANSFORM projection;
   int bw = al_get_bitmap_width(logo);
   int bh = al_get_bitmap_height(logo);
   float x, y, c;

   al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);

   al_identity_transform(&projection);

   /* First, we scroll the text in in the y direction (inside the x/z
    * plane) and move it away from the camera (in the z direction).
    * We move it as far as half the display height to get a vertical
    * FOV of 90 degrees.
    */
   al_translate_transform_3d(&projection, 0, -scroll_y, -180);

   /* Then we tilt it backwards 30 degrees. */
   al_rotate_transform_3d(&projection, 1, 0, 0,
      30 * A5O_PI / 180.0);

   /* And finally move it down so the 0 position ends up
    * at the bottom of the screen.
    */
   al_translate_transform_3d(&projection, 0, 180, 0);

   setup_3d_projection(&projection);

   x = 0;
   y = 0;
   c = 1 + (y - scroll_y) / 360 / 2.0;
   if (c < 0)
      c = 0;
   al_draw_tinted_bitmap(logo, al_map_rgba_f(c, c, c, c),
      x - bw / 2, y, 0);
   y += bh;

#define T(str) (y = print(font, x, y, 1, 0.9, 0.3, scroll_y, (str)));

   T("Allegro 5")
   T("")
   T("It is a period of game programming.")
   T("Game coders have won their first")
   T("victory against the evil")
   T("General Protection Fault.")
   T("")
   T("During the battle, hackers managed")
   T("to steal the secret source to the")
   T("General's ultimate weapon,")
   T("the ACCESS VIOLATION, a kernel")
   T("exception with enough power to")
   T("destroy an entire program.")
   T("")
   T("Pursued by sinister bugs the")
   T("Allegro developers race home")
   T("aboard their library to save")
   T("all game programmers and restore")
   T("freedom to the open source world.")

#undef T
}

static void draw_intro_text(void)
{
   A5O_TRANSFORM projection;
   int fade;
   int fh = al_get_font_line_height(font);

   if (scroll_y < 50)
      fade = (50 - scroll_y) * 12;
   else
      fade = (scroll_y - 50) * 4;
   
   al_identity_transform(&projection);
   al_translate_transform_3d(&projection, 0, -scroll_y / 3, -181);
   setup_3d_projection(&projection);

   print(font, 0, 0, 0, 0.9, 1, fade, "A long time ago, in a galaxy");
   print(font, 0, 0 + fh, 0, 0.9, 1, fade, "not too far away...");
}


int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   int redraw = 0;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_init_image_addon();
   al_init_font_addon();
   al_init_ttf_addon();
   init_platform_specific();
   al_install_keyboard();

   al_set_new_display_flags(A5O_RESIZABLE);
   display = al_create_display(640, 360);
   if (!display) {
      abort_example("Error creating display\n");
   }

   al_set_new_bitmap_flags(A5O_MIN_LINEAR | A5O_MAG_LINEAR |
      A5O_MIPMAP);

   font = load("data/DejaVuSans.ttf", "font", 40, 0);
   logo = load("data/alexlogo.png", "bitmap");
   particle = load("data/haiku/air_effect.png", "bitmap");

   al_convert_mask_to_alpha(logo, al_map_rgb(255, 0, 255));

   text_length = al_get_bitmap_height(logo) +
      19 * al_get_font_line_height(font);

   timer = al_create_timer(1.0 / 60);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));

   al_start_timer(timer);
   while (true) {
      A5O_EVENT event;

      al_wait_for_event(queue, &event);
      if (event.type == A5O_EVENT_DISPLAY_CLOSE)
         break;
      else if (event.type == A5O_EVENT_DISPLAY_RESIZE) {
         al_acknowledge_resize(display);
      }
      else if (event.type == A5O_EVENT_KEY_DOWN &&
            event.keyboard.keycode == A5O_KEY_ESCAPE) {
         break;
      }
      else if (event.type == A5O_EVENT_TIMER) {
         scroll_y++;
         if (scroll_y > text_length * 2)
            scroll_y -= text_length * 2;

         redraw = 1;
      }

      if (redraw  && al_is_event_queue_empty(queue)) {
         A5O_COLOR black = al_map_rgba_f(0, 0, 0, 1);

         al_clear_to_color(black);

         draw_stars();

         draw_scrolling_text();

         draw_intro_text();

         al_flip_display();
         redraw = 0;
      }
   }

   return 0;
}

/* vim: set sts=3 sw=3 et: */
