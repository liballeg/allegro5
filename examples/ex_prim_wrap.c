/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Bitmap wrapping options.
 *
 *      See readme.txt for copyright information.
 */
#define ALLEGRO_UNSTABLE

#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_primitives.h"

#include "common.c"

typedef enum {
   TRIANGLES,
   POINTS,
   LINES,
   NUM_TYPES,
} PRIM_TYPE;

static void draw_square(ALLEGRO_BITMAP* texture, float x, float y, float w, float h, int prim_type)
{
   ALLEGRO_COLOR c = al_map_rgb_f(1, 1, 1);

   switch (prim_type) {
      case TRIANGLES: {
         ALLEGRO_VERTEX vtxs[4] = {
         /*  x       y       z   u        v      c  */
            {x,          y,          0., -2 * w, -2 * h, c},
            {x + 5 * w,  y,          0.,  3 * w, -2 * h, c},
            {x + 5 * w,  y + 5 * h,  0.,  3 * w,  3 * h, c},
            {x,          y + 5 * h,  0., -2 * w,  3 * h, c},
         };
         al_draw_prim(vtxs, NULL, texture, 0, 4, ALLEGRO_PRIM_TRIANGLE_FAN);
         break;
      }
      case POINTS: {
#define POINTS_N 32
         float d = POINTS_N - 1;
         ALLEGRO_VERTEX vtxs[POINTS_N * POINTS_N];

         for (int iy = 0; iy < POINTS_N; iy++) {
            for (int ix = 0; ix < POINTS_N; ix++) {
               ALLEGRO_VERTEX *v = &vtxs[iy * POINTS_N + ix];
               v->x = x + 5. * w * (float)ix / d;
               v->y = y + 5. * h * (float)iy / d;
               v->z = 0.;
               v->u = -2 * w + 5. * w * (float)ix / d;
               v->v = -2 * h + 5. * h * (float)iy / d;
               v->color = c;
            }
         }
         
         al_draw_prim(vtxs, NULL, texture, 0, POINTS_N * POINTS_N, ALLEGRO_PRIM_POINT_LIST);
         break;
      }
      case LINES: {
#define LINES_N 32
         float d = LINES_N - 1;
         ALLEGRO_VERTEX vtxs[2 * LINES_N * 2];

         for (int iy = 0; iy < LINES_N; iy++) {
            for (int ix = 0; ix < 2; ix++) {
               ALLEGRO_VERTEX *v = &vtxs[ix + 2 * iy];
               v->x = x + 5. * w * (float)ix;
               v->y = y + 5. * h * (float)iy / d;
               v->z = 0.;
               v->u = -2 * w + 5. * w * (float)ix;
               v->v = -2 * h + 5. * h * (float)iy / d;
               v->color = c;
            }
         }
         for (int iy = 0; iy < 2; iy++) {
            for (int ix = 0; ix < LINES_N; ix++) {
               ALLEGRO_VERTEX *v = &vtxs[2 * ix + iy + 2 * LINES_N];
               v->x = x + 5. * w * (float)ix / d;
               v->y = y + 5. * h * (float)iy;
               v->z = 0.;
               v->u = -2 * w + 5. * w * (float)ix / d;
               v->v = -2 * h + 5. * h * (float)iy;
               v->color = c;
            }
         }
         al_draw_prim(vtxs, NULL, texture, 0, 4 * LINES_N, ALLEGRO_PRIM_LINE_LIST);
         break;
      }
   }
}

int main(int argc, char **argv)
{
   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_mouse();
   al_install_keyboard();
   al_init_image_addon();
   al_init_font_addon();
   al_init_primitives_addon();
   init_platform_specific();
   bool allow_shader = false;
   bool use_shader = false;
   if (argc > 1 && strcmp(argv[1], "--shader") == 0) {
      allow_shader = true;
   }

   if (allow_shader)
      al_set_new_display_flags(ALLEGRO_PROGRAMMABLE_PIPELINE);
   ALLEGRO_DISPLAY *display = al_create_display(1024, 1024);
   if (!display)
      abort_example("Error creating display\n");

   ALLEGRO_BITMAP *bitmap = al_load_bitmap("data/texture2.tga");
   if (!bitmap)
      abort_example("Could not load 'data/texture2.tga'.\n");

   ALLEGRO_BITMAP_WRAP wrap_settings[4] = {
      ALLEGRO_BITMAP_WRAP_DEFAULT,
      ALLEGRO_BITMAP_WRAP_REPEAT,
      ALLEGRO_BITMAP_WRAP_CLAMP,
      ALLEGRO_BITMAP_WRAP_MIRROR,
   };
   const char* wrap_names[4] = {
      "DEFAULT",
      "REPEAT",
      "CLAMP",
      "MIRROR",
   };
   const char* prim_names[NUM_TYPES] = {
      "TRIANGLES",
      "POINTS",
      "LINES",
   };

   ALLEGRO_BITMAP *bitmaps[4][4];
   for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
         al_set_new_bitmap_wrap(wrap_settings[i], wrap_settings[j]);
         bitmaps[i][j] = al_clone_bitmap(bitmap);
      }
   }

   ALLEGRO_FONT *font = al_create_builtin_font();

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   ALLEGRO_BITMAP *memory_buffer = al_create_bitmap(al_get_display_width(display),
      al_get_display_height(display));
   al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
   ALLEGRO_BITMAP *display_buffer = al_get_backbuffer(display);

   ALLEGRO_SHADER *shader = NULL;

   if (allow_shader) {
      shader = al_create_shader(ALLEGRO_SHADER_AUTO);
      if (!shader)
         abort_example("Error creating shader.\n");

      const char* pixel_file;
      if (al_get_shader_platform(shader) == ALLEGRO_SHADER_GLSL) {
   #ifdef ALLEGRO_CFG_SHADER_GLSL
         pixel_file = "data/ex_bitmap_wrap_pixel.glsl";
   #endif
      }
      else {
   #ifdef ALLEGRO_CFG_SHADER_HLSL
         pixel_file = "data/ex_bitmap_wrap_pixel.hlsl";
   #endif
      }

      if (!pixel_file) {
         abort_example("No shader source\n");
      }
      if (!al_attach_shader_source(shader, ALLEGRO_VERTEX_SHADER,
            al_get_default_shader_source(ALLEGRO_SHADER_AUTO, ALLEGRO_VERTEX_SHADER))) {
         abort_example("al_attach_shader_source for vertex shader failed: %s\n", al_get_shader_log(shader));
      }
      if (!al_attach_shader_source_file(shader, ALLEGRO_PIXEL_SHADER, pixel_file))
         abort_example("al_attach_shader_source_file for pixel shader failed: %s\n", al_get_shader_log(shader));
      if (!al_build_shader(shader))
         abort_example("al_build_shader failed: %s\n", al_get_shader_log(shader));
   }

   ALLEGRO_TIMER *timer = al_create_timer(1.0 / 60);
   ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_start_timer(timer);

   bool redraw = true;
   bool software = false;
   PRIM_TYPE prim_type = TRIANGLES;
   bool quit = false;

   while (!quit) {
      ALLEGRO_EVENT event;
      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
         break;
      if (event.type == ALLEGRO_EVENT_KEY_CHAR) {
         switch (event.keyboard.keycode) {
            case ALLEGRO_KEY_ESCAPE:
               quit = true;
               break;
            case ALLEGRO_KEY_S:
               software = !software;
               break;
            case ALLEGRO_KEY_R:
               if (allow_shader)
                  use_shader = !use_shader;
               break;
            case ALLEGRO_KEY_P:
               prim_type = (prim_type + 1) % NUM_TYPES;
               break;
         }
      }
      if (event.type == ALLEGRO_EVENT_TIMER) {
         redraw = true;
      }

      if (redraw && al_is_event_queue_empty(queue)) {
         redraw = false;
         al_clear_to_color(al_map_rgb_f(0, 0, 0));

         int offt_x = 64;
         int offt_y = 64;
         int spacing = 16;
         int bw = al_get_bitmap_width(bitmap);
         int bh = al_get_bitmap_height(bitmap);
         int w = spacing + 5 * bw;
         int h = spacing + 5 * bh;

         if (software && !use_shader) {
            al_set_target_bitmap(memory_buffer);
            al_clear_to_color(al_map_rgb_f(0, 0, 0));
         }
         else {
            al_set_target_bitmap(display_buffer);
         }

         al_draw_textf(font, al_map_rgb_f(1., 1., 0.5), 16, 16,
            ALLEGRO_ALIGN_LEFT, "(S)oftware: %s", (software && !use_shader) ? "On" : "Off");
         al_draw_textf(font, al_map_rgb_f(1., 1., 0.5), 16, 32,
            ALLEGRO_ALIGN_LEFT, "Shade(r) (--shader to enable): %s", use_shader ? "On" : "Off");
         al_draw_textf(font, al_map_rgb_f(1., 1., 0.5), 16, 48,
            ALLEGRO_ALIGN_LEFT, "(P)rimitive type: %s", prim_names[prim_type]);

         for (int i = 0; i < 4; i++) {
            al_draw_text(font, al_map_rgb_f(1., 0.5, 1.), offt_x + spacing + w * i + w / 2,
               64, ALLEGRO_ALIGN_CENTRE, wrap_names[i]);
            for (int j = 0; j < 4; j++) {
               if (i == 0) {
                  al_draw_text(font, al_map_rgb_f(1., 0.5, 1.), 16,
                     offt_y + spacing + h * j + h / 2, ALLEGRO_ALIGN_LEFT, wrap_names[j]);
               }
               if (use_shader) {
                  al_use_shader(shader);
                  // Note that this part will look different in D3D and OpenGL.
                  // In GL, the wrapping is attached to textures, so
                  // when sampling twice from the same texture they must
                  // share the same wrapping mode. This runs into an
                  // issue with the primitives addon which attempts to
                  // change it on the fly. To get consistent behavior,
                  // use the non-default wrapping mode.
                  al_set_shader_sampler("tex2", bitmaps[i][j], 1);
                  draw_square(bitmaps[i][j], offt_x + spacing + w * i, offt_y + spacing + h * j, bw, bh, prim_type);
                  al_use_shader(NULL);
               }
               else {
                  draw_square(bitmaps[i][j], offt_x + spacing + w * i, offt_y + spacing + h * j, bw, bh, prim_type);
               }
            }
         }

         if (software && !use_shader) {
            al_set_target_bitmap(display_buffer);
            al_draw_bitmap(memory_buffer, 0, 0, 0);
         }

         al_flip_display();
      }
   }

   al_destroy_bitmap(bitmap);
   al_destroy_bitmap(memory_buffer);
   for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
         al_destroy_bitmap(bitmaps[i][j]);
      }
   }
   al_destroy_font(font);

   return 0;
}


/* vim: set sts=4 sw=4 et: */
