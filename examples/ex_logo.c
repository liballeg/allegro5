/* Demo program which creates a logo by direct pixel manipulation of
 * bitmaps. Also uses alpha blending to create a real-time flash
 * effect (likely not visible with displays using memory bitmaps as it
 * is too slow).
 * 
 * FIXME: Fix few FIXMEs, and document.
 * 
 * TODO: Not sure this should be in the "examples" directory, it's not
 * really much of an example demonstrating a particular A5 feature.
 */
#include <stdio.h>
#include <math.h>
#include "allegro5/a5_ttf.h"
#include "allegro5/a5_iio.h"
#include <allegro5/a5_primitives.h>

static ALLEGRO_BITMAP *logo, *logo_flash;
static int logo_x, logo_y;
static ALLEGRO_FONT *font;
static int cursor;
static int selection;
static bool regenerate = 0, editing = 0;
static ALLEGRO_CONFIG *config;
static ALLEGRO_COLOR white;
static double anim;

static float clamp(float x)
{
   if (x < 0)
      return 0;
   if (x > 1)
      return 1;
   return x;
}

static char const *param_names[] = {
   "text", "font", "size", "shadow", "blur", "factor", "red", "green",
   "blue", NULL
};

/* Note: To regenerate something close to the official Allegro logo,
 * you need to obtain the non-free "Utopia Regular Italic" font. Then
 * replace "DejaVuSans.ttf" with "putri.pfa" below.
 */
static char param_values[][256] = {
   "Allegro", "data/DejaVuSans.ttf", "140", "10", "2", "0.5", "1.1",
   "1.5", "5"
};

/* Generates a bitmap with transparent background and the logo text.
 * The bitmap will have screen size. If 'bumpmap' is not NULL, it will
 * contain another bitmap which is a white, blurred mask of the logo
 * which we use for the flash effect.
 */
static ALLEGRO_BITMAP *generate_logo(char const *text,
                                     char const *fontname,
                                     int font_size,
                                     float shadow_offset,
                                     float blur_radius,
                                     float blur_factor,
                                     float light_red,
                                     float light_green,
                                     float light_blue,
                                     ALLEGRO_BITMAP **bumpmap)
{
   ALLEGRO_COLOR transparent = al_map_rgba_f(0, 0, 0, 0);
   int xp, yp, w, h, as, de, i, j, x, y, br, bw, dw, dh;
   ALLEGRO_COLOR c;
   ALLEGRO_FONT *logofont;
   ALLEGRO_STATE state;
   ALLEGRO_BITMAP *blur, *light, *logo;
   int left, right, top, bottom;
   ALLEGRO_LOCKED_REGION lock1, lock2;
   float cx, cy;

   dw = al_get_display_width();
   dh = al_get_display_height();

   cx = dw * 0.5;
   cy = dh * 0.5;

   logofont = al_ttf_load_font(fontname, -font_size, 0);
   al_ttf_get_text_dimensions(logofont, text, -1, &xp, &yp, &w, &h, &as, &de);

   al_store_state(&state, ALLEGRO_STATE_TARGET_BITMAP | ALLEGRO_STATE_BLENDER);

   /* Cheap blur effect to create a bump map. */
   blur = al_create_bitmap(dw, dh);
   al_set_target_bitmap(blur);
   al_clear(transparent);
   br = blur_radius;
   bw = br * 2 + 1;
   c = al_map_rgba_f(1, 1, 1, 1.0 / (bw * bw * blur_factor));
   al_set_separate_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
                           ALLEGRO_ONE, ALLEGRO_ONE, c);
   for (i = -br; i <= br; i++) {
      for (j = -br; j <= br; j++) {
         al_font_textout(logofont,
                         cx - xp * 0.5 - w * 0.5 + i,
                         cy - yp * 0.5 - h * 0.5 + j, text, -1);
      }
   }

   left = cx - xp * 0.5 - w * 0.5 - br + xp;
   top = cy - yp * 0.5 - h * 0.5 - br + yp;
   right = left + w + br * 2;
   bottom = top + h + br * 2;

   /* Cheap light effect. */
   light = al_create_bitmap(dw, dh);
   al_set_target_bitmap(light);
   al_clear(transparent);
   al_lock_bitmap(blur, &lock1, ALLEGRO_LOCK_READONLY);
   // FIXME: ALLEGRO_LOCK_WRITEONLY is broken right now with OpenGL
   al_lock_bitmap_region(light, left, top, 1 + right - left,
                         1 + bottom - top, &lock2, 0);
   for (y = top; y < bottom; y++) {
      for (x = left; x < right; x++) {
         float r1, g1, b1, a1;
         float r2, g2, b2, a2;
         float r, g, b, a;
         float d;
         ALLEGRO_COLOR c = al_get_pixel(blur, x, y);
         ALLEGRO_COLOR c1 = al_get_pixel(blur, x - 1, y - 1);
         ALLEGRO_COLOR c2 = al_get_pixel(blur, x + 1, y + 1);
         al_unmap_rgba_f(c, &r, &g, &b, &a);
         al_unmap_rgba_f(c1, &r1, &g1, &b1, &a1);
         al_unmap_rgba_f(c2, &r2, &g2, &b2, &a2);

         d = r2 - r1 + 0.5;
         r = clamp(d * light_red);
         g = clamp(d * light_green);
         b = clamp(d * light_blue);

         c = al_map_rgba_f(r, g, b, a);
         al_put_pixel(x, y, c);
      }
   }
   al_unlock_bitmap(light);
   al_unlock_bitmap(blur);

   if (bumpmap)
      *bumpmap = blur;
   else
      al_destroy_bitmap(blur);

   /* Create final logo */
   logo = al_create_bitmap(dw, dh);
   al_set_target_bitmap(logo);
   al_clear(transparent);

   /* Draw a shadow. */
   c = al_map_rgba_f(0, 0, 0, 0.5 / 9);
   al_set_separate_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
                           ALLEGRO_ONE, ALLEGRO_ONE, c);
   for (i = -1; i <= 1; i++)
      for (j = -1; j <= 1; j++)
         al_font_textout(logofont,
                         cx - xp * 0.5 - w * 0.5 + shadow_offset + i,
                         cy - yp * 0.5 - h * 0.5 + shadow_offset + j,
                         text, -1);

   /* Then draw the lit text we made before on top. */
   al_set_separate_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
                           ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA, white);
   al_draw_bitmap(light, 0, 0, 0);
   al_destroy_bitmap(light);

   al_restore_state(&state);
   al_font_destroy_font(logofont);

   return logo;
}

/* Draw the checkerboard background. */
static void draw_background(void)
{
   ALLEGRO_COLOR c[2];
   int i, j;
   c[0] = al_map_rgba(0xaa, 0xaa, 0xaa, 0xff);
   c[1] = al_map_rgba(0x99, 0x99, 0x99, 0xff);

   for (i = 0; i < 640 / 16; i++) {
      for (j = 0; j < 480 / 16; j++) {
         al_draw_filled_rectangle(i * 16, j * 16,
                           i * 16 + 16, j * 16 + 16,
                           c[(i + j) & 1]);
      }
   }
}

/* Print out the current logo parameters. */
static void print_parameters(void)
{
   int i;
   ALLEGRO_STATE state;
   ALLEGRO_COLOR normal = al_map_rgba_f(0, 0, 0, 1);
   ALLEGRO_COLOR light = al_map_rgba_f(0, 0, 1, 1);
   ALLEGRO_COLOR label = al_map_rgba_f(0.2, 0.2, 0.2, 1);
   int th;

   al_store_state(&state, ALLEGRO_STATE_BLENDER);

   th = al_font_text_height(font) + 3;
   for (i = 0; param_names[i]; i++) {
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, label);
      al_font_textprintf(font, 2, 2 + i * th, "%s", param_names[i]);
   }
   for (i = 0; param_names[i]; i++) {
      int y = 2 + i * th;
      // FIXME: additive blending seems broken here when using
      // memory blenders (i.e. no FBO available)
      // al_set_blender(ALLEGRO_ONE, ALLEGRO_ONE, white)
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, white);
      al_draw_filled_rectangle(75, y, 375, y + th - 2,
                        al_map_rgba_f(1, 1, 1, 0.5));
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
                     i == selection ? light : normal);
      al_font_textprintf(font, 75, y, "%s", param_values[i]);
      if (i == selection && editing &&
         (((int)(al_current_time() * 2)) & 1)) {
         int x = 75 + al_font_text_width(font, param_values[i], -1);
         al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, normal);
         al_draw_line(x, y, x, y + th, white, 0);
      }
   }

   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, normal);
   al_font_textprintf(font, 400, 2, "%s", "R - Randomize");
   al_font_textprintf(font, 400, 2 + th, "%s", "S - Save as logo.png");

   al_font_textprintf(font, 2, 480 - th * 2 - 2, "%s",
                      "To modify, press Return, then enter value, "
                      "then Return again.");
   al_font_textprintf(font, 2, 480 - th - 2, "%s",
                      "Use cursor up/down to "
                      "select the value to modify.");
   al_restore_state(&state);
}

static char const *rnum(float min, float max)
{
   static char s[256];
   double x = rand() / (double)RAND_MAX;
   x = min + x * (max - min);
   uszprintf(s, sizeof s, "%.1f", x);
   return s;
}

static void randomize(void)
{
   ustrcpy(param_values[3], rnum(2, 12));
   ustrcpy(param_values[4], rnum(1, 8));
   ustrcpy(param_values[5], rnum(0.1, 1));
   ustrcpy(param_values[6], rnum(0, 5));
   ustrcpy(param_values[7], rnum(0, 5));
   ustrcpy(param_values[8], rnum(0, 5));
   regenerate = true;
}

static void save(void)
{
   al_iio_save("logo.png", logo);
}

static void mouse_click(int x, int y)
{
   int th = al_font_text_height(font) + 3;
   int sel = (y - 2) / th;
   int i;
   if (x < 400) {
      for (i = 0; param_names[i]; i++) {
         if (sel == i) {
            selection = i;
            cursor = 0;
            editing = true;
         }
      }
   }
   else if (x < 500) {
      if (sel == 0)
         randomize();
      if (sel == 1)
         save();
   }
}

static void render(void)
{
   double t = al_current_time();
   if (regenerate) {
      al_destroy_bitmap(logo);
      al_destroy_bitmap(logo_flash);
      logo = NULL;
      regenerate = false;
   }
   if (!logo) {
      /* Generate a new logo. */
      ALLEGRO_BITMAP *fullflash;
      ALLEGRO_BITMAP *fulllogo = generate_logo(param_values[0],
         param_values[1],
         strtol(param_values[2], NULL, 10),
         strtod(param_values[3], NULL),
         strtod(param_values[4], NULL),
         strtod(param_values[5], NULL),
         strtod(param_values[6], NULL),
         strtod(param_values[7], NULL),
         strtod(param_values[8], NULL),
         &fullflash);
      ALLEGRO_BITMAP *crop;
      ALLEGRO_LOCKED_REGION lock;
      int x, y, left = 640, top = 480, right = -1, bottom = -1;
      /* Crop out the non-transparent part. */
      al_lock_bitmap(fulllogo, &lock, ALLEGRO_LOCK_READONLY);
      for (y = 0; y < 480; y++) {
         for (x = 0; x < 640; x++) {
            ALLEGRO_COLOR c = al_get_pixel(fulllogo, x, y);
            float r, g, b, a;
            al_unmap_rgba_f(c, &r, &g, &b, &a);
            if (a > 0) {
               if (x < left)
                  left = x;
               if (y < top)
                  top = y;
               if (x > right)
                  right = x;
               if (y > bottom)
                  bottom = y;
            }
         }
      }
      al_unlock_bitmap(fulllogo);

      crop = al_create_sub_bitmap(fulllogo, left, top,
                                  1 + right - left, 1 + bottom - top);
      logo = al_clone_bitmap(crop);
      al_destroy_bitmap(crop);
      al_destroy_bitmap(fulllogo);

      crop = al_create_sub_bitmap(fullflash, left, top,
                                  1 + right - left, 1 + bottom - top);
      logo_flash = al_clone_bitmap(crop);
      al_destroy_bitmap(crop);
      al_destroy_bitmap(fullflash);

      logo_x = left;
      logo_y = top;

      anim = t;
   }
   draw_background();

   /* For half a second, display our flash animation. */
   if (t - anim < 0.5) {
      ALLEGRO_STATE state;
      int w, h, i, j;
      float f = sin(ALLEGRO_PI * ((t - anim) / 0.5));
      ALLEGRO_COLOR c = al_map_rgb_f(f * 0.3, f * 0.3, f * 0.3);
      w = al_get_bitmap_width(logo);
      h = al_get_bitmap_height(logo);
      al_store_state(&state, ALLEGRO_STATE_BLENDER);
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
                     al_map_rgba_f(1, 1, 1, 1 - f));
      al_draw_bitmap(logo, logo_x, logo_y, 0);
      al_set_blender(ALLEGRO_ONE, ALLEGRO_ONE, c);
      for (j = -2; j <= 2; j += 2) {
         for (i = -2; i <= 2; i += 2) {
            al_draw_bitmap(logo_flash, logo_x + i, logo_y + j, 0);
         }
      }
      al_restore_state(&state);
   }
   else
      al_draw_bitmap(logo, logo_x, logo_y, 0);


   print_parameters();
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   int redraw = 0, i;
   bool quit = false;

   al_init();
   al_install_mouse();
   al_font_init();
   srand(time(NULL));

   white = al_map_rgba_f(1, 1, 1, 1);

   display = al_create_display(640, 480);
   al_set_window_title("Allegro Logo Generator");
   al_install_keyboard();

   /* Read logo parameters from logo.ini (if it exists). */
   config = al_config_read("logo.ini");
   if (!config)
      config = al_config_create();
   for (i = 0; param_names[i]; i++) {
      char const *value = al_config_get_value(config, "logo",
                                              param_names[i]);
      if (value)
         ustrzcpy(param_values[i], sizeof(param_values[i]), value);
   }

   font = al_ttf_load_font("data/DejaVuSans.ttf", 12, 0);

   timer = al_install_timer(1.0 / 60);

   queue = al_create_event_queue();
   al_register_event_source(queue, (void *)al_get_keyboard());
   al_register_event_source(queue, (void *)al_get_mouse());
   al_register_event_source(queue, (void *)display);
   al_register_event_source(queue, (void *)timer);

   al_start_timer(timer);
   while (!quit) {
      ALLEGRO_EVENT event;
      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
         break;
      if (event.type == ALLEGRO_EVENT_KEY_DOWN ||
          event.type == ALLEGRO_EVENT_KEY_REPEAT) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
            quit = true;
         else if (event.keyboard.keycode == ALLEGRO_KEY_ENTER) {
            if (editing) {
               regenerate = true;
               editing = false;
            }
            else {
               cursor = 0;
               editing = true;
            }
         }
         else if (event.keyboard.keycode == ALLEGRO_KEY_UP) {
            if (selection > 0) {
               selection--;
               cursor = 0;
               editing = false;
            }
         }
         else if (event.keyboard.keycode == ALLEGRO_KEY_DOWN) {
            if (param_names[selection + 1]) {
               selection++;
               cursor = 0;
               editing = false;
            }
         }
         else {
            int c = event.keyboard.unichar;
            if (editing) {
               if (c >= 32) {
                  usetat(param_values[selection], cursor, c);
                  cursor++;
                  usetat(param_values[selection], cursor, 0);
               }
            }
            else {
               if (c == 'r')
                  randomize();
               if (c == 's')
                  save();
            }
         }
      }
      if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
         if (event.mouse.button == 1) {
            mouse_click(event.mouse.x, event.mouse.y);
         }
      }
      if (event.type == ALLEGRO_EVENT_TIMER)
         redraw++;

      if (redraw && al_event_queue_is_empty(queue)) {
         redraw = 0;

         render();

         al_flip_display();
      }
   }

   /* Write modified parameters back to logo.ini. */
   for (i = 0; param_names[i]; i++) {
      al_config_set_value(config, "logo", param_names[i],
         param_values[i]);
   }
   al_config_write(config, "logo.ini");
   al_config_destroy(config);

   return 0;
}
END_OF_MAIN()
