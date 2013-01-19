#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_shader.h"

#include "allegro5/allegro_opengl.h"
#include "allegro5/allegro_shader_glsl.h"

#include "common.c"

static ALLEGRO_BITMAP *load_bitmap(char const *filename)
{
   ALLEGRO_BITMAP *bitmap = al_load_bitmap(filename);
   if (!bitmap)
      abort_example("%s not found or failed to load\n", filename);
   return bitmap;
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *bitmap[2];
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   bool redraw = true;
   ALLEGRO_SHADER *shader;
   int t = 0;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_mouse();
   al_install_keyboard();
   al_init_image_addon();

   al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR |
      ALLEGRO_MIPMAP);
   al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
   al_set_new_display_option(ALLEGRO_SAMPLES, 4, ALLEGRO_SUGGEST);
   al_set_new_display_flags(ALLEGRO_USE_PROGRAMMABLE_PIPELINE |
      ALLEGRO_OPENGL);
   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display\n");
   }

   bitmap[0]= load_bitmap("data/mysha.pcx");
   bitmap[1]= load_bitmap("data/obp.jpg");

   shader = al_create_shader(ALLEGRO_SHADER_GLSL);
   
   al_attach_shader_source(shader, ALLEGRO_VERTEX_SHADER,
      al_get_default_glsl_vertex_shader());

   /* Simple fragment shader which uses the fractional texture coordinate to
    * look up the color of the second texture (scaled down by factor 100).
    */
   al_attach_shader_source(
      shader,
      ALLEGRO_PIXEL_SHADER,
      "#version 120\n"
      #ifdef ALLEGRO_CFG_OPENGLES
      "precision mediump float;\n"
      #endif
      "uniform sampler2D tex;\n"
      "uniform sampler2D tex2;\n"
      "varying vec2 varying_texcoord;\n"
      "void main()\n"
      "{\n"
      "    vec4 color = texture2D(tex2, fract(varying_texcoord * 100));\n"
      "    gl_FragColor = color * texture2D(tex, varying_texcoord);\n"
      "}\n"
   );
   
   al_link_shader(shader);
   al_set_shader(display, shader);

   timer = al_create_timer(1.0 / 60);
   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_start_timer(timer);

   while (1) {
      ALLEGRO_EVENT event;
      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
         break;
      if (event.type == ALLEGRO_EVENT_KEY_CHAR) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
            break;
         
      }
      if (event.type == ALLEGRO_EVENT_TIMER) {
         redraw = true;
         t++;
      }

      if (redraw && al_is_event_queue_empty(queue)) {
         int dw, dh;
         double scale = 1 + 100 * (1 + sin(t * ALLEGRO_PI * 2 / 60 / 10));
         double angle = ALLEGRO_PI * 2 * t / 60 / 15;
         double x = 120 - 20 * cos(ALLEGRO_PI * 2 * t / 60 / 25);
         double y = 120 - 20 * sin(ALLEGRO_PI * 2 * t / 60 / 25);
         
         dw = al_get_display_width(display);
         dh = al_get_display_height(display);

         redraw = false;
         al_clear_to_color(al_map_rgb_f(0, 0, 0));

         /* We set a second bitmap for texture unit 1. Unit 0 will have
          * the normal texture which al_draw_*_bitmap will set up for us.
          * We then draw the bitmap like normal, except it will use the
          * custom shader.
          */
         al_set_shader_sampler(shader, "tex2", bitmap[1], 1);
         al_use_shader(shader, true);
         al_draw_scaled_rotated_bitmap(bitmap[0], x, y, dw / 2, dh / 2,
            scale, scale, angle, 0);

         al_flip_display();
      }
   }

   al_destroy_bitmap(bitmap[0]);
   al_destroy_bitmap(bitmap[1]);
   al_destroy_shader(shader);

   return 0;
}


/* vim: set sts=4 sw=4 et: */
