#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_color.h"

/* Note: It should be easy using Cg or HLSL instead of GLSL - for the
 * 5.2 release we should make this a config option and implement all 3
 * in any shader examples.
 */
#include "allegro5/allegro_opengl.h"

#include "common.c"

int pal_hex[256];

typedef struct {
   float x, y, angle, t;
   int flags, i, j;
} Sprite;

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_BITMAP *bitmap, *background, *pal_bitmap;
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   bool redraw = true;
   bool show_pal = false;
   A5O_SHADER *shader;
   float pals[7][3 * 256];
   int i, j;
   float t = 0;
   Sprite sprite[8];

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_mouse();
   al_install_keyboard();
   al_init_image_addon();
   init_platform_specific();

   al_set_new_display_flags(A5O_PROGRAMMABLE_PIPELINE |
      A5O_OPENGL);
   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display\n");
   }

   al_set_new_bitmap_format(A5O_PIXEL_FORMAT_SINGLE_CHANNEL_8);
   bitmap = al_load_bitmap_flags("data/alexlogo.bmp", A5O_KEEP_INDEX);
   if (!bitmap) {
      abort_example("alexlogo not found or failed to load\n");
   }

   /* Create 8 sprites. */
   for (i = 0; i < 8; i++) {
      Sprite *s = sprite + i;
      s->angle = A5O_PI * 2 * i / 8;
      s->x = 320 + sin(s->angle) * (64 + i * 16);
      s->y = 240 - cos(s->angle) * (64 + i * 16);
      s->flags = (i % 2) ? A5O_FLIP_HORIZONTAL : 0;
      s->t = i / 8.0;
      s->i = i % 6;
      s->j = (s->i + 1) % 6;
   }

   background = al_load_bitmap("data/bkg.png");
   if (!bitmap) {
      abort_example("background not found or failed to load\n");
   }

   /* Create 7 palettes with changed hue. */
   for (j = 0; j < 7; j++) {
      for (i = 0; i < 256; i++) {
         float r, g, b, h, s, l;
         r = (pal_hex[i] >> 16) / 255.0;
         g = ((pal_hex[i] >> 8) & 255) / 255.0;
         b = (pal_hex[i] & 255) / 255.0;
         
         al_color_rgb_to_hsl(r, g, b, &h, &s, &l);
         h += j * 50;
         al_color_hsl_to_rgb(h, s, l, &r, &g, &b);

         pals[j][i * 3 + 0] = r;
         pals[j][i * 3 + 1] = g;
         pals[j][i * 3 + 2] = b;
      }
   }

   al_set_new_bitmap_format(A5O_PIXEL_FORMAT_ANY);
   pal_bitmap = al_create_bitmap(255, 7);
   al_set_target_bitmap(pal_bitmap);
   for (int y = 0; y < 7; y++) {
      for (int x = 0; x < 256; x++) {
         float r = pals[y][x * 3 + 0] * 255.0;
         float g = pals[y][x * 3 + 1] * 255.0;
         float b = pals[y][x * 3 + 2] * 255.0;
         al_put_pixel(x, y, al_map_rgb(r,g,b));
      }
   }
   al_set_target_backbuffer(display);

   shader = al_create_shader(A5O_SHADER_GLSL);
   if (!al_attach_shader_source(shader, A5O_VERTEX_SHADER,
         al_get_default_shader_source(A5O_SHADER_AUTO, A5O_VERTEX_SHADER))) {
      abort_example("al_attach_shader_source for vertex shader failed: %s\n", al_get_shader_log(shader));
   }
   if (!al_attach_shader_source_file(shader, A5O_PIXEL_SHADER, "data/ex_shader_palette_pixel.glsl")) {
      abort_example("al_attach_shader_source_file for pixel shader failed: %s\n", al_get_shader_log(shader));
   }
   if (!al_build_shader(shader))
      abort_example("al_build_shader failed: %s\n", al_get_shader_log(shader));

   al_use_shader(shader);

   timer = al_create_timer(1.0 / 60);
   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_start_timer(timer);

   al_set_shader_sampler("pal_tex", pal_bitmap, 1);

   log_printf("%s\n", "Press P to toggle displaying the palette");

   while (1) {
      A5O_EVENT event;
      al_wait_for_event(queue, &event);
      if (event.type == A5O_EVENT_DISPLAY_CLOSE)
         break;
      if (event.type == A5O_EVENT_KEY_CHAR) {
         if (event.keyboard.keycode == A5O_KEY_ESCAPE)
            break;
         if (event.keyboard.keycode == A5O_KEY_P)
            show_pal = !show_pal;
         
      }
      if (event.type == A5O_EVENT_TIMER) {
         redraw = true;
         t++;
         for (i = 0; i < 8; i++) {
            Sprite *s = sprite + i;
            int dir = s->flags ? 1 : -1;
            s->x += cos(s->angle) * 2 * dir;
            s->y += sin(s->angle) * 2 * dir;
            s->angle += A5O_PI / 180.0 * dir;
         }
      }
         
      if (redraw && al_is_event_queue_empty(queue)) {
         float pos = (int)t % 60 / 60.0;
         int p1 = (int)(t / 60) % 3;
         int p2 = (p1 + 1) % 3;

         redraw = false;
         al_clear_to_color(al_map_rgb_f(0, 0, 0));

         al_set_shader_float("pal_set_1", p1 * 2);
         al_set_shader_float("pal_set_2", p2 * 2);
         al_set_shader_float("pal_interp", pos);

         if (background)
            al_draw_bitmap(background, 0, 0, 0);

         for (i = 0; i < 8; i++) {
            Sprite *s = sprite + 7 - i;
            float pos = (1 + sin((t / 60 + s->t) * 2 * A5O_PI)) / 2;
            al_set_shader_float("pal_set_1", s->i);
            al_set_shader_float("pal_set_2", s->j);
            al_set_shader_float("pal_interp", pos);
            al_draw_rotated_bitmap(bitmap,
               64, 64, s->x, s->y, s->angle, s->flags);
         }
         
         {
            float sc = 0.5;
            al_set_shader_float("pal_set_1", (int)t % 20 > 15 ? 6 : 0);
            al_set_shader_float("pal_interp", 0);

            #define D al_draw_scaled_rotated_bitmap
            D(bitmap, 0, 0,   0,   0,  sc,  sc, 0, 0);
            D(bitmap, 0, 0, 640,   0, -sc,  sc, 0, 0);
            D(bitmap, 0, 0,   0, 480,  sc, -sc, 0, 0);
            D(bitmap, 0, 0, 640, 480, -sc, -sc, 0, 0);
         }

         if (show_pal) {
            al_use_shader(NULL);
            al_draw_scaled_bitmap(pal_bitmap, 0, 0, 255, 7,
               0, 0, 255, 7*12, 0);
            al_use_shader(shader);
         }
         
         al_flip_display();
      }
   }

   al_use_shader(NULL);

   al_destroy_bitmap(bitmap);
   al_destroy_shader(shader);

   return 0;
}

int pal_hex[256] = {
   0xFF00FF, 0x000100, 0x060000, 0x040006, 0x000200,
   0x000306, 0x010400, 0x030602, 0x02090C, 0x070A06,
   0x020C14, 0x030F1A, 0x0F0E03, 0x0D0F0C, 0x071221,
   0x0D1308, 0x0D1214, 0x121411, 0x12170E, 0x151707,
   0x0A182B, 0x171816, 0x131B0C, 0x1A191C, 0x171D08,
   0x081D35, 0x1A200E, 0x1D1F1C, 0x1D2013, 0x0E2139,
   0x06233F, 0x17230E, 0x1C270E, 0x21260F, 0x0D2845,
   0x0A294C, 0x1F2A12, 0x252724, 0x232B19, 0x222D15,
   0x0C2F51, 0x0D2F57, 0x263012, 0x2B2D2B, 0x233314,
   0x273617, 0x0D3764, 0x17355E, 0x2C3618, 0x2E3623,
   0x333432, 0x2C3A15, 0x093D70, 0x333B17, 0x163C6A,
   0x2F3D18, 0x323D24, 0x383A38, 0x30401B, 0x2F431C,
   0x1E4170, 0x12447D, 0x154478, 0x3F403E, 0x34471A,
   0x3D482C, 0x134B8B, 0x3A4D20, 0x184D86, 0x474846,
   0x3A511D, 0x13549A, 0x3D5420, 0x195595, 0x0F57A3,
   0x4E504D, 0x415925, 0x435B27, 0x485837, 0x125DA9,
   0x485E24, 0x175FB2, 0x235DA3, 0x555754, 0x0565BD,
   0x1C61B5, 0x2163B7, 0x2164B1, 0x49662A, 0x1268C1,
   0x2365B9, 0x1769C3, 0x5E605D, 0x196BBE, 0x55673D,
   0x1B6BC5, 0x2968BC, 0x246BB8, 0x526D2A, 0x0E73CC,
   0x0E74C6, 0x246FC9, 0x2470C4, 0x56712E, 0x666865,
   0x007DCE, 0x537530, 0x2A72CC, 0x55762B, 0x1B77D0,
   0x1F77D8, 0x1E79CC, 0x2E74CF, 0x58782D, 0x2E75CA,
   0x59792E, 0x2279D3, 0x5A7A2F, 0x3276D2, 0x6D6F6C,
   0x1081D3, 0x137FDF, 0x237DC9, 0x5B7C30, 0x637848,
   0x2A7DD7, 0x5E7F33, 0x2C7DDE, 0x2A80CD, 0x1D82E2,
   0x1A85D1, 0x2B80D5, 0x747673, 0x2D82CF, 0x2F84D1,
   0x3381E3, 0x2289D5, 0x3285D2, 0x2986EE, 0x2189ED,
   0x4782C5, 0x3884DF, 0x4083D2, 0x3487D4, 0x278BD7,
   0x298ADD, 0x67883B, 0x7B7D7A, 0x2A8CD9, 0x6C8653,
   0x3289E2, 0x3889D7, 0x2C8DDA, 0x2E8FDB, 0x3D8CDA,
   0x2F90DC, 0x338EE8, 0x3191DD, 0x3E8EDE, 0x3392DE,
   0x838582, 0x709145, 0x3593E0, 0x4191D9, 0x3794E1,
   0x698AB1, 0x4590E5, 0x3B93E6, 0x789158, 0x4594DC,
   0x3C97E4, 0x4896DE, 0x4397EA, 0x3D9AE1, 0x8B8E8B,
   0x409CE3, 0x4B99E1, 0x439CEA, 0x539AD6, 0x5898E2,
   0x439EE5, 0x4E9BE4, 0x439FEC, 0x809C5F, 0x7C9E57,
   0x45A0E7, 0x509FE1, 0x47A1E8, 0x599EDB, 0x48A2E9,
   0x80A153, 0x4AA4EB, 0x959794, 0x5CA1DE, 0x51A3EF,
   0x59A3E3, 0x4DA6ED, 0x4FA7EF, 0x51A8F0, 0x87A763,
   0x5AA8EA, 0x53AAF2, 0x9C9E9B, 0x49AFF5, 0x56ACF5,
   0x55AFF0, 0x8CAD67, 0x64ACE8, 0x60ADF0, 0x59AFF7,
   0x6EACE2, 0x79A9E1, 0x63AFF2, 0x59B2F3, 0x90B162,
   0xA6A8A5, 0x60B5F4, 0x94B56D, 0x99BC72, 0xAEB0AD,
   0x74BBF2, 0x8DB8ED, 0x94B7E3, 0x8ABEEA, 0xA0C379,
   0x82C0F2, 0xB6B8B5, 0xA3C77C, 0xA5C97E, 0xA9CA79,
   0x8FC7F3, 0xBEC0BD, 0xA1C6E9, 0x97C9F0, 0xADD07E,
   0xC8CAC7, 0xACD1F0, 0xB6CFF0, 0xB9D5ED, 0xD1D3D0,
   0xBEDAF4, 0xD9DBD8, 0xC7E2FB, 0xCDE3F6, 0xE1E3E0,
   0xE4E9EC, 0xDBEBF9, 0xEAECE9, 0xE7EFF8, 0xF1F3F0,
   0xECF4FD, 0xF2F7FA, 0xF6F8F5, 0xF7FCFF, 0xFAFCF8,
   0xFDFFFC,};

/* vim: set sts=4 sw=4 et: */
