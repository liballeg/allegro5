#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define A5O_UNSTABLE
#include "allegro5/allegro.h"
#include "allegro5/allegro_primitives.h"

#include "common.c"

typedef struct
{
   float x, y;
   float nx, ny, nz;
} CUSTOM_VERTEX;

#define RING_SIZE 25
#define SPHERE_RADIUS 150.1
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define NUM_RINGS (SCREEN_WIDTH / RING_SIZE + 1)
#define NUM_SEGMENTS 64
#define NUM_VERTICES (NUM_RINGS * NUM_SEGMENTS * 6)
#define FIRST_OUTSIDE_RING ((int)(SPHERE_RADIUS / RING_SIZE))

static void setup_vertex(CUSTOM_VERTEX* vtx, int ring, int segment, bool inside)
{
   float len;
   float x, y, z;
   x = ring * RING_SIZE * cosf(2 * A5O_PI * segment / NUM_SEGMENTS);
   y = ring * RING_SIZE * sinf(2 * A5O_PI * segment / NUM_SEGMENTS);
   vtx->x = x + SCREEN_WIDTH / 2;
   vtx->y = y + SCREEN_HEIGHT / 2;

   if (inside) {
      /* This comes from the definition of the normal vector as the
       * gradient of the 3D surface. */
      z = sqrtf(SPHERE_RADIUS * SPHERE_RADIUS - x * x - y * y);
      vtx->nx = x / z;
      vtx->ny = y / z;
   }
   else {
      vtx->nx = 0;
      vtx->ny = 0;
   }
   vtx->nz = 1.0;

   len = sqrtf(vtx->nx * vtx->nx + vtx->ny * vtx->ny + vtx->nz * vtx->nz);
   vtx->nx /= len;
   vtx->ny /= len;
   vtx->nz /= len;
}

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   bool redraw = true;
   A5O_SHADER *shader;
   A5O_VERTEX_DECL *vertex_decl;
   A5O_VERTEX_ELEMENT vertex_elems[] = {
      {A5O_PRIM_POSITION, A5O_PRIM_FLOAT_2, offsetof(CUSTOM_VERTEX, x)},
      {A5O_PRIM_USER_ATTR, A5O_PRIM_FLOAT_3, offsetof(CUSTOM_VERTEX, nx)},
      {0, 0, 0}
   };
   CUSTOM_VERTEX vertices[NUM_VERTICES];
   bool quit = false;
   const char* vertex_shader_file;
   const char* pixel_shader_file;
   int vertex_idx = 0;
   int ring, segment;
   float diffuse_color[4] = {0.1, 0.1, 0.7, 1.0};
   float light_position[3] = {0, 0, 100};

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_mouse();
   al_install_keyboard();
   al_install_touch_input();
   if (!al_init_primitives_addon()) {
      abort_example("Could not init primitives addon.\n");
   }
   init_platform_specific();
   al_set_new_display_flags(A5O_PROGRAMMABLE_PIPELINE);
   display = al_create_display(SCREEN_WIDTH, SCREEN_HEIGHT);
   if (!display) {
      abort_example("Error creating display.\n");
   }

   vertex_decl = al_create_vertex_decl(vertex_elems, sizeof(CUSTOM_VERTEX));
   if (!vertex_decl) {
      abort_example("Error creating vertex declaration.\n");
   }

   /* Computes a "spherical" bump ring. The z coordinate is not actually set
    * appropriately as this is a 2D example, but the normal vectors are computed
    * correctly for the light shading effect. */
   for (ring = 0; ring < NUM_RINGS; ring++) {
      for (segment = 0; segment < NUM_SEGMENTS; segment++) {
         bool inside = ring < FIRST_OUTSIDE_RING;
         setup_vertex(&vertices[vertex_idx + 0], ring + 0, segment + 0, inside);
         setup_vertex(&vertices[vertex_idx + 1], ring + 0, segment + 1, inside);
         setup_vertex(&vertices[vertex_idx + 2], ring + 1, segment + 0, inside);
         setup_vertex(&vertices[vertex_idx + 3], ring + 1, segment + 0, inside);
         setup_vertex(&vertices[vertex_idx + 4], ring + 0, segment + 1, inside);
         setup_vertex(&vertices[vertex_idx + 5], ring + 1, segment + 1, inside);
         vertex_idx += 6;
      }
   }

   shader = al_create_shader(A5O_SHADER_AUTO);

   if (!shader) {
      abort_example("Failed to create shader.");
   }

   if (al_get_shader_platform(shader) == A5O_SHADER_GLSL) {
      vertex_shader_file = "data/ex_prim_shader_vertex.glsl";
      pixel_shader_file = "data/ex_prim_shader_pixel.glsl";
   }
   else {
      vertex_shader_file = "data/ex_prim_shader_vertex.hlsl";
      pixel_shader_file = "data/ex_prim_shader_pixel.hlsl";
   }

   if (!al_attach_shader_source_file(shader, A5O_VERTEX_SHADER, vertex_shader_file)) {
      abort_example("al_attach_shader_source_file for vertex shader failed: %s\n",
         al_get_shader_log(shader));
   }
   if (!al_attach_shader_source_file(shader, A5O_PIXEL_SHADER, pixel_shader_file)) {
      abort_example("al_attach_shader_source_file for pixel shader failed: %s\n",
         al_get_shader_log(shader));
   }

   if (!al_build_shader(shader)) {
      abort_example("al_build_shader for link failed: %s\n", al_get_shader_log(shader));
   }

   al_use_shader(shader);
   al_set_shader_float_vector("diffuse_color", 4, diffuse_color, 1);
   /* alpha controls shininess, and 25 is very shiny */
   al_set_shader_float("alpha", 25);

   timer = al_create_timer(1.0 / 60);
   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   if (al_is_touch_input_installed()) {
      al_register_event_source(queue,
         al_get_touch_input_mouse_emulation_event_source());
   }
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_start_timer(timer);

   while (!quit) {
      A5O_EVENT event;
      al_wait_for_event(queue, &event);

      switch (event.type) {
         case A5O_EVENT_DISPLAY_CLOSE:
            quit = true;
            break;
         case A5O_EVENT_MOUSE_AXES:
            light_position[0] = event.mouse.x;
            light_position[1] = event.mouse.y;
            break;
         case A5O_EVENT_KEY_CHAR:
            if (event.keyboard.keycode == A5O_KEY_ESCAPE)
               quit = true;
            break;
         case A5O_EVENT_TIMER:
            redraw = true;
            break;
      }

      if (redraw && al_is_event_queue_empty(queue)) {
         al_clear_to_color(al_map_rgb_f(0, 0, 0));

         al_set_shader_float_vector("light_position", 3, light_position, 1);
         al_draw_prim(vertices, vertex_decl, NULL, 0, NUM_VERTICES, A5O_PRIM_TRIANGLE_LIST);

         al_flip_display();
         redraw = false;
      }
   }

   al_use_shader(NULL);
   al_destroy_shader(shader);
   al_destroy_vertex_decl(vertex_decl);

   return 0;
}


/* vim: set sts=3 sw=3 et: */
