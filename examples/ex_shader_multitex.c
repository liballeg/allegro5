#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_opengl.h"

#include "common.c"

static A5O_BITMAP *load_bitmap(char const *filename)
{
   A5O_BITMAP *bitmap = al_load_bitmap(filename);
   if (!bitmap)
      abort_example("%s not found or failed to load\n", filename);
   return bitmap;
}

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_BITMAP *bitmap[2];
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   bool redraw = true;
   A5O_SHADER *shader;
   int t = 0;
   const char* pixel_file = NULL;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_mouse();
   al_install_keyboard();
   al_init_image_addon();
   init_platform_specific();

   al_set_new_bitmap_flags(A5O_MIN_LINEAR | A5O_MAG_LINEAR |
      A5O_MIPMAP);
   al_set_new_display_option(A5O_SAMPLE_BUFFERS, 1, A5O_SUGGEST);
   al_set_new_display_option(A5O_SAMPLES, 4, A5O_SUGGEST);
   al_set_new_display_flags(A5O_PROGRAMMABLE_PIPELINE);
   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display\n");
   }

   bitmap[0]= load_bitmap("data/mysha.pcx");
   bitmap[1]= load_bitmap("data/obp.jpg");

   shader = al_create_shader(A5O_SHADER_AUTO);
   if (!shader)
      abort_example("Error creating shader.\n");

   if (al_get_shader_platform(shader) == A5O_SHADER_GLSL) {
#ifdef A5O_CFG_SHADER_GLSL
      pixel_file = "data/ex_shader_multitex_pixel.glsl";
#endif
   }
   else {
#ifdef A5O_CFG_SHADER_HLSL
      pixel_file = "data/ex_shader_multitex_pixel.hlsl";
#endif
   }

   if (!pixel_file) {
      abort_example("No shader source\n");
   }
   if (!al_attach_shader_source(shader, A5O_VERTEX_SHADER,
         al_get_default_shader_source(A5O_SHADER_AUTO, A5O_VERTEX_SHADER))) {
      abort_example("al_attach_shader_source for vertex shader failed: %s\n", al_get_shader_log(shader));
   }
   if (!al_attach_shader_source_file(shader, A5O_PIXEL_SHADER, pixel_file))
      abort_example("al_attach_shader_source_file for pixel shader failed: %s\n", al_get_shader_log(shader));
   if (!al_build_shader(shader))
      abort_example("al_build_shader failed: %s\n", al_get_shader_log(shader));

   al_use_shader(shader);

   timer = al_create_timer(1.0 / 60);
   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_start_timer(timer);

   while (1) {
      A5O_EVENT event;
      al_wait_for_event(queue, &event);
      if (event.type == A5O_EVENT_DISPLAY_CLOSE)
         break;
      if (event.type == A5O_EVENT_KEY_CHAR) {
         if (event.keyboard.keycode == A5O_KEY_ESCAPE)
            break;
         
      }
      if (event.type == A5O_EVENT_TIMER) {
         redraw = true;
         t++;
      }

      if (redraw && al_is_event_queue_empty(queue)) {
         int dw, dh;
         double scale = 1 + 100 * (1 + sin(t * A5O_PI * 2 / 60 / 10));
         double angle = A5O_PI * 2 * t / 60 / 15;
         double x = 120 - 20 * cos(A5O_PI * 2 * t / 60 / 25);
         double y = 120 - 20 * sin(A5O_PI * 2 * t / 60 / 25);
         
         dw = al_get_display_width(display);
         dh = al_get_display_height(display);

         redraw = false;
         al_clear_to_color(al_map_rgb_f(0, 0, 0));

         /* We set a second bitmap for texture unit 1. Unit 0 will have
          * the normal texture which al_draw_*_bitmap will set up for us.
          * We then draw the bitmap like normal, except it will use the
          * custom shader.
          */
         al_set_shader_sampler("tex2", bitmap[1], 1);
         al_draw_scaled_rotated_bitmap(bitmap[0], x, y, dw / 2, dh / 2,
            scale, scale, angle, 0);

         al_flip_display();
      }
   }

   al_use_shader(NULL);

   al_destroy_bitmap(bitmap[0]);
   al_destroy_bitmap(bitmap[1]);
   al_destroy_shader(shader);

   return 0;
}


/* vim: set sts=4 sw=4 et: */
