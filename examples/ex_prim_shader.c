#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/allegro_shader.h"

#include "common.c"

typedef struct
{
   float x, y;
   ALLEGRO_COLOR color;
   float r, g, b;
} CUSTOM_VERTEX;

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   bool redraw = true;
   ALLEGRO_SHADER *shader;
   ALLEGRO_VERTEX_DECL *vertex_decl;
   ALLEGRO_VERTEX_ELEMENT vertex_elems[] = {
      {ALLEGRO_PRIM_POSITION, ALLEGRO_PRIM_FLOAT_2, offsetof(CUSTOM_VERTEX, x)},
      {ALLEGRO_PRIM_COLOR_ATTR, 0, offsetof(CUSTOM_VERTEX, color)},
      {ALLEGRO_PRIM_USER_ATTR, ALLEGRO_PRIM_FLOAT_3, offsetof(CUSTOM_VERTEX, r)},
      {0, 0, 0}
   };
   CUSTOM_VERTEX vertices[4];
   bool quit = false;
   float mouse_pos[2];
   const char* vertex_shader_file;
   const char* pixel_shader_file;
   bool opengl = true;

   mouse_pos[0] = 0;
   mouse_pos[1] = 0;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_mouse();
   al_install_keyboard();
   if (!al_init_primitives_addon()) {
      abort_example("Could not init primitives addon.\n");
   }
   al_set_new_display_flags(ALLEGRO_USE_PROGRAMMABLE_PIPELINE);
   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display.\n");
   }
   
   opengl = al_get_display_flags(display) & ALLEGRO_OPENGL;

   vertex_decl = al_create_vertex_decl(vertex_elems, sizeof(CUSTOM_VERTEX));
   if (!vertex_decl) {
      abort_example("Error creating vertex declaration.\n");
   }

   vertices[0].x = 0;
   vertices[0].y = 0;
   vertices[0].r = 0;
   vertices[0].g = 1;
   vertices[0].b = 1;
   vertices[0].color = al_map_rgb_f(0.1, 0.1, 0.1);

   vertices[1].x = al_get_display_width(display);
   vertices[1].y = 0;
   vertices[1].r = 0;
   vertices[1].g = 0;
   vertices[1].b = 1;
   vertices[1].color = al_map_rgb_f(0.1, 0.1, 0.1);

   vertices[2].x = al_get_display_width(display);
   vertices[2].y = al_get_display_height(display);
   vertices[2].r = 1;
   vertices[2].g = 0;
   vertices[2].b = 0;
   vertices[2].color = al_map_rgb_f(0.1, 0.1, 0.1);

   vertices[3].x = 0;
   vertices[3].y = al_get_display_height(display);
   vertices[3].r = 1;
   vertices[3].g = 1;
   vertices[3].b = 0;
   vertices[3].color = al_map_rgb_f(0.1, 0.1, 0.1);

   shader = al_create_shader(opengl ? ALLEGRO_SHADER_GLSL : ALLEGRO_SHADER_HLSL);
   
   vertex_shader_file = opengl ? "data/ex_prim_shader_vertex.glsl" : "data/ex_prim_shader_vertex.hlsl";
   pixel_shader_file = opengl ? "data/ex_prim_shader_pixel.glsl" : "data/ex_prim_shader_pixel.hlsl";

   if (!al_attach_shader_source_file(shader, ALLEGRO_VERTEX_SHADER, vertex_shader_file)) {
      abort_example("al_attach_shader_source_file for vertex shader failed: %s\n",
         al_get_shader_log(shader));
   }
   if (!al_attach_shader_source_file(shader, ALLEGRO_PIXEL_SHADER, pixel_shader_file)) {
      abort_example("al_attach_shader_source_file for pixel shader failed: %s\n",
         al_get_shader_log(shader));
   }

   if (!al_link_shader(shader)) {
      abort_example("al_link_shader for link failed: %s\n", al_get_shader_log(shader));
   }
   al_set_shader(display, shader);

   timer = al_create_timer(1.0 / 60);
   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_start_timer(timer);

   while (!quit) {
      ALLEGRO_EVENT event;
      al_wait_for_event(queue, &event);

      switch (event.type) {
         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            quit = true;
            break;
         case ALLEGRO_EVENT_MOUSE_AXES:
            mouse_pos[0] = event.mouse.x;
            mouse_pos[1] = event.mouse.y;
            break;
         case ALLEGRO_EVENT_KEY_CHAR:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
               quit = true;
            break;
         case ALLEGRO_EVENT_TIMER:
            redraw = true;
            break;
      }

      if (redraw && al_is_event_queue_empty(queue)) {
         al_clear_to_color(al_map_rgb_f(0, 0, 0));

         al_set_shader_float_vector(shader, "mouse_pos", 2, mouse_pos, 1);
         al_use_shader(shader, true);
         al_draw_prim(vertices, vertex_decl, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_FAN);
         al_use_shader(shader, false);

         al_flip_display();
         redraw = false;
      }
   }

   al_destroy_shader(shader);
   al_destroy_vertex_decl(vertex_decl);

   return 0;
}


/* vim: set sts=4 sw=4 et: */
