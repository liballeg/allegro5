/* Demo program which creates a logo by direct pixel manipulation of
 * bitmaps. Also uses alpha blending to create a real-time flash
 * effect (likely not visible with displays using memory bitmaps as it
 * is too slow).
 */

#include <stdio.h>
#include <math.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>

#include "common.c"

#ifdef A5O_MSVC
   #define snprintf _snprintf
#endif

static A5O_BITMAP *logo, *logo_flash;
static int logo_x, logo_y;
static A5O_FONT *font;
static int cursor;
static int selection;
static bool regenerate = 0, editing = 0;
static A5O_CONFIG *config;
static A5O_COLOR white;
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
static A5O_BITMAP *generate_logo(char const *text,
                                     char const *fontname,
                                     int font_size,
                                     float shadow_offset,
                                     float blur_radius,
                                     float blur_factor,
                                     float light_red,
                                     float light_green,
                                     float light_blue,
                                     A5O_BITMAP **bumpmap)
{
   A5O_COLOR transparent = al_map_rgba_f(0, 0, 0, 0);
   int xp, yp, w, h, i, j, x, y, br, bw, dw, dh;
   A5O_COLOR c;
   A5O_FONT *logofont;
   A5O_STATE state;
   A5O_BITMAP *blur, *light, *logo;
   int left, right, top, bottom;
   float cx, cy;

   dw = al_get_bitmap_width(al_get_target_bitmap());
   dh = al_get_bitmap_height(al_get_target_bitmap());

   cx = dw * 0.5;
   cy = dh * 0.5;

   logofont = al_load_font(fontname, -font_size, 0);
   al_get_text_dimensions(logofont, text, &xp, &yp, &w, &h);

   al_store_state(&state, A5O_STATE_TARGET_BITMAP | A5O_STATE_BLENDER);

   /* Cheap blur effect to create a bump map. */
   blur = al_create_bitmap(dw, dh);
   al_set_target_bitmap(blur);
   al_clear_to_color(transparent);
   br = blur_radius;
   bw = br * 2 + 1;
   c = al_map_rgba_f(1, 1, 1, 1.0 / (bw * bw * blur_factor));
   al_set_separate_blender(A5O_ADD, A5O_ALPHA, A5O_INVERSE_ALPHA,
                           A5O_ADD, A5O_ONE, A5O_ONE);
   for (i = -br; i <= br; i++) {
      for (j = -br; j <= br; j++) {
         al_draw_text(logofont, c,
                         cx - xp * 0.5 - w * 0.5 + i,
                         cy - yp * 0.5 - h * 0.5 + j, 0, text);
      }
   }

   left = cx - xp * 0.5 - w * 0.5 - br + xp;
   top = cy - yp * 0.5 - h * 0.5 - br + yp;
   right = left + w + br * 2;
   bottom = top + h + br * 2;

   if (left < 0)
      left = 0;
   if (top < 0)
      top = 0;
   if (right > dw - 1)
      right = dw - 1;
   if (bottom > dh - 1)
      bottom = dh - 1;

   /* Cheap light effect. */
   light = al_create_bitmap(dw, dh);
   al_set_target_bitmap(light);
   al_clear_to_color(transparent);
   al_lock_bitmap(blur, A5O_PIXEL_FORMAT_ANY, A5O_LOCK_READONLY);
   al_lock_bitmap_region(light, left, top,
      1 + right - left, 1 + bottom - top,
      A5O_PIXEL_FORMAT_ANY, A5O_LOCK_WRITEONLY);
   for (y = top; y <= bottom; y++) {
      for (x = left; x <= right; x++) {
         float r1, g1, b1, a1;
         float r2, g2, b2, a2;
         float r, g, b, a;
         float d;
         A5O_COLOR c = al_get_pixel(blur, x, y);
         A5O_COLOR c1 = al_get_pixel(blur, x - 1, y - 1);
         A5O_COLOR c2 = al_get_pixel(blur, x + 1, y + 1);
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
   al_clear_to_color(transparent);

   /* Draw a shadow. */
   c = al_map_rgba_f(0, 0, 0, 0.5 / 9);
   al_set_separate_blender(A5O_ADD, A5O_ALPHA, A5O_INVERSE_ALPHA,
                           A5O_ADD, A5O_ONE, A5O_ONE);
   for (i = -1; i <= 1; i++)
      for (j = -1; j <= 1; j++)
         al_draw_text(logofont, c,
                         cx - xp * 0.5 - w * 0.5 + shadow_offset + i,
                         cy - yp * 0.5 - h * 0.5 + shadow_offset + j,
                         0, text);

   /* Then draw the lit text we made before on top. */
   al_set_separate_blender(A5O_ADD, A5O_ALPHA, A5O_INVERSE_ALPHA,
                           A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);
   al_draw_bitmap(light, 0, 0, 0);
   al_destroy_bitmap(light);

   al_restore_state(&state);
   al_destroy_font(logofont);

   return logo;
}

/* Draw the checkerboard background. */
static void draw_background(void)
{
   A5O_COLOR c[2];
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
   A5O_STATE state;
   A5O_COLOR normal = al_map_rgba_f(0, 0, 0, 1);
   A5O_COLOR light = al_map_rgba_f(0, 0, 1, 1);
   A5O_COLOR label = al_map_rgba_f(0.2, 0.2, 0.2, 1);
   int th;

   al_store_state(&state, A5O_STATE_BLENDER);

   th = al_get_font_line_height(font) + 3;
   for (i = 0; param_names[i]; i++) {
      al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);
      al_draw_textf(font, label, 2, 2 + i * th, 0, "%s", param_names[i]);
   }
   for (i = 0; param_names[i]; i++) {
      int y = 2 + i * th;
      al_draw_filled_rectangle(75, y, 375, y + th - 2,
                        al_map_rgba_f(0.5, 0.5, 0.5, 0.5));
      al_draw_textf(font, i == selection ? light : normal, 75, y, 0, "%s", param_values[i]);
      if (i == selection && editing &&
         (((int)(al_get_time() * 2)) & 1)) {
         int x = 75 + al_get_text_width(font, param_values[i]);
         al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);
         al_draw_line(x, y, x, y + th, white, 0);
      }
   }

   al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);
   al_draw_textf(font, normal, 400, 2, 0, "%s", "R - Randomize");
   al_draw_textf(font, normal, 400, 2 + th, 0, "%s", "S - Save as logo.png");

   al_draw_textf(font, normal, 2, 480 - th * 2 - 2, 0, "%s",
                      "To modify, press Return, then enter value, "
                      "then Return again.");
   al_draw_textf(font, normal, 2, 480 - th - 2, 0, "%s",
                      "Use cursor up/down to "
                      "select the value to modify.");
   al_restore_state(&state);
}

static char const *rnum(float min, float max)
{
   static char s[256];
   double x = rand() / (double)RAND_MAX;
   x = min + x * (max - min);
   snprintf(s, sizeof s, "%.1f", x);
   return s;
}

static void randomize(void)
{
   strcpy(param_values[3], rnum(2, 12));
   strcpy(param_values[4], rnum(1, 8));
   strcpy(param_values[5], rnum(0.1, 1));
   strcpy(param_values[6], rnum(0, 5));
   strcpy(param_values[7], rnum(0, 5));
   strcpy(param_values[8], rnum(0, 5));
   regenerate = true;
}

static void save(void)
{
   al_save_bitmap("logo.png", logo);
}

static void mouse_click(int x, int y)
{
   int th = al_get_font_line_height(font) + 3;
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
   double t = al_get_time();
   if (regenerate) {
      al_destroy_bitmap(logo);
      al_destroy_bitmap(logo_flash);
      logo = NULL;
      regenerate = false;
   }
   if (!logo) {
      /* Generate a new logo. */
      A5O_BITMAP *fullflash;
      A5O_BITMAP *fulllogo = generate_logo(param_values[0],
         param_values[1],
         strtol(param_values[2], NULL, 10),
         strtod(param_values[3], NULL),
         strtod(param_values[4], NULL),
         strtod(param_values[5], NULL),
         strtod(param_values[6], NULL),
         strtod(param_values[7], NULL),
         strtod(param_values[8], NULL),
         &fullflash);
      A5O_BITMAP *crop;
      int x, y, left = 640, top = 480, right = -1, bottom = -1;
      /* Crop out the non-transparent part. */
      al_lock_bitmap(fulllogo, A5O_PIXEL_FORMAT_ANY, A5O_LOCK_READONLY);
      for (y = 0; y < 480; y++) {
         for (x = 0; x < 640; x++) {
            A5O_COLOR c = al_get_pixel(fulllogo, x, y);
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

      if (left == 640)
         left = 0;
      if (top == 480)
         top = 0;
      if (right < left)
         right = left;
      if (bottom < top)
         bottom = top;

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
      A5O_STATE state;
      int i, j;
      float f = sin(A5O_PI * ((t - anim) / 0.5));
      A5O_COLOR c = al_map_rgb_f(f * 0.3, f * 0.3, f * 0.3);
      al_store_state(&state, A5O_STATE_BLENDER);
      al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);
      al_draw_tinted_bitmap(logo, al_map_rgba_f(1, 1, 1, 1 - f), logo_x, logo_y, 0);
      al_set_blender(A5O_ADD, A5O_ONE, A5O_ONE);
      for (j = -2; j <= 2; j += 2) {
         for (i = -2; i <= 2; i += 2) {
            al_draw_tinted_bitmap(logo_flash, c, logo_x + i, logo_y + j, 0);
         }
      }
      al_restore_state(&state);
   }
   else
      al_draw_bitmap(logo, logo_x, logo_y, 0);


   print_parameters();
}

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   int redraw = 0, i;
   bool quit = false;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not initialise Allegro\n");
   }
   al_init_primitives_addon();
   al_install_mouse();
   al_init_image_addon();
   al_init_font_addon();
   al_init_ttf_addon();
   init_platform_specific();
   srand(time(NULL));

   white = al_map_rgba_f(1, 1, 1, 1);

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Could not create display\n");
   }
   al_set_window_title(display, "Allegro Logo Generator");
   al_install_keyboard();

   /* Read logo parameters from logo.ini (if it exists). */
   config = al_load_config_file("logo.ini");
   if (!config)
      config = al_create_config();
   for (i = 0; param_names[i]; i++) {
      char const *value = al_get_config_value(config, "logo", param_names[i]);
      if (value)
         strncpy(param_values[i], value, sizeof(param_values[i]));
   }

   font = al_load_font("data/DejaVuSans.ttf", 12, 0);
   if (!font) {
      abort_example("Could not load font\n");
   }

   timer = al_create_timer(1.0 / 60);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));

   al_start_timer(timer);
   while (!quit) {
      A5O_EVENT event;
      al_wait_for_event(queue, &event);
      if (event.type == A5O_EVENT_DISPLAY_CLOSE)
         break;
      if (event.type == A5O_EVENT_KEY_CHAR) {
         if (event.keyboard.keycode == A5O_KEY_ESCAPE) {
            quit = true;
         }
         else if (event.keyboard.keycode == A5O_KEY_ENTER) {
            if (editing) {
               regenerate = true;
               editing = false;
            }
            else {
               cursor = strlen(param_values[selection]);
               editing = true;
            }
         }
         else if (event.keyboard.keycode == A5O_KEY_UP) {
            if (selection > 0) {
               selection--;
               cursor = strlen(param_values[selection]);
               editing = false;
            }
         }
         else if (event.keyboard.keycode == A5O_KEY_DOWN) {
            if (param_names[selection + 1]) {
               selection++;
               cursor = strlen(param_values[selection]);
               editing = false;
            }
         }
         else {
            int c = event.keyboard.unichar;
            if (editing) {
               if (event.keyboard.keycode == A5O_KEY_BACKSPACE) {
                  if (cursor > 0) {
                     A5O_USTR *u = al_ustr_new(param_values[selection]);
                     if (al_ustr_prev(u, &cursor)) {
                        al_ustr_remove_chr(u, cursor);
                        strncpy(param_values[selection], al_cstr(u),
                           sizeof param_values[selection]);
                     }
                     al_ustr_free(u);
                  }
               }
               else if (c >= 32) {
                  A5O_USTR *u = al_ustr_new(param_values[selection]);
                  cursor += al_ustr_set_chr(u, cursor, c);
                  al_ustr_set_chr(u, cursor, 0);
                  strncpy(param_values[selection], al_cstr(u),
                     sizeof param_values[selection]);
                  al_ustr_free(u);
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
      if (event.type == A5O_EVENT_MOUSE_BUTTON_DOWN) {
         if (event.mouse.button == 1) {
            mouse_click(event.mouse.x, event.mouse.y);
         }
      }
      if (event.type == A5O_EVENT_TIMER)
         redraw++;

      if (redraw && al_is_event_queue_empty(queue)) {
         redraw = 0;

         render();

         al_flip_display();
      }
   }

   /* Write modified parameters back to logo.ini. */
   for (i = 0; param_names[i]; i++) {
      al_set_config_value(config, "logo", param_names[i],
         param_values[i]);
   }
   al_save_config_file("logo.ini", config);
   al_destroy_config(config);

   return 0;
}
