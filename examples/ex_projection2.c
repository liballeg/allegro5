#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

#include <math.h>

#include "common.c"

static void draw_pyramid(A5O_BITMAP* texture, float x, float y, float z, float theta)
{
   A5O_COLOR c = al_map_rgb_f(1, 1, 1);
   A5O_TRANSFORM t;
   A5O_VERTEX vtx[5] = {
   /*   x   y   z   u   v  c  */
      { 0,  1,  0,  0, 64, c},
      {-1, -1, -1,  0,  0, c},
      { 1, -1, -1, 64, 64, c},
      { 1, -1,  1, 64,  0, c},
      {-1, -1,  1, 64, 64, c},
   };
   int indices[12] = {
      0, 1, 2,
      0, 2, 3,
      0, 3, 4,
      0, 4, 1
   };

   al_identity_transform(&t);
   al_rotate_transform_3d(&t, 0, 1, 0, theta);
   al_translate_transform_3d(&t, x, y, z);
   al_use_transform(&t);
   al_draw_indexed_prim(vtx, NULL, texture, indices, 12, A5O_PRIM_TRIANGLE_LIST);
}

static void set_perspective_transform(A5O_BITMAP* bmp)
{
   A5O_TRANSFORM p;
   float aspect_ratio = (float)al_get_bitmap_height(bmp) / al_get_bitmap_width(bmp);
   al_set_target_bitmap(bmp);
   al_identity_transform(&p);
   al_perspective_transform(&p, -1, aspect_ratio, 1, 1, -aspect_ratio, 1000);
   al_use_projection_transform(&p);
}

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   A5O_BITMAP *texture;
   A5O_BITMAP *display_sub_persp;
   A5O_BITMAP *display_sub_ortho;
   A5O_BITMAP *buffer;
   A5O_FONT *font;
   bool redraw = false;
   bool quit = false;
   bool fullscreen = false;
   bool background = false;
   int display_flags = A5O_RESIZABLE;
   float theta = 0;

   if (argc > 1) {
      if(strcmp(argv[1], "--use-shaders") == 0) {
         display_flags |= A5O_PROGRAMMABLE_PIPELINE;
      }
      else {
         abort_example("Unknown command line argument: %s\n", argv[1]);
      }
   }

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_init_image_addon();
   al_init_primitives_addon();
   al_init_font_addon();
   init_platform_specific();
   al_install_keyboard();

   al_set_new_display_flags(display_flags);
   al_set_new_display_option(A5O_DEPTH_SIZE, 16, A5O_SUGGEST);
   /* Load everything as a POT bitmap to make sure the projection stuff works
    * with mismatched backing texture and bitmap sizes. */
   al_set_new_display_option(A5O_SUPPORT_NPOT_BITMAP, 0, A5O_REQUIRE);
   display = al_create_display(800, 600);
   if (!display) {
      abort_example("Error creating display\n");
   }
   al_set_window_constraints(display, 256, 512, 0, 0);
   al_apply_window_constraints(display, true);
   set_perspective_transform(al_get_backbuffer(display));

   /* This bitmap is a sub-bitmap of the display, and has a perspective transformation. */
   display_sub_persp = al_create_sub_bitmap(al_get_backbuffer(display), 0, 0, 256, 256);
   set_perspective_transform(display_sub_persp);

   /* This bitmap is a sub-bitmap of the display, and has a orthographic transformation. */
   display_sub_ortho = al_create_sub_bitmap(al_get_backbuffer(display), 0, 0, 256, 512);

   /* This bitmap has a perspective transformation, purposefully non-POT */
   buffer = al_create_bitmap(200, 200);
   set_perspective_transform(buffer);

   timer = al_create_timer(1.0 / 60);
   font = al_create_builtin_font();

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));

   al_set_new_bitmap_flags(A5O_MIN_LINEAR | A5O_MAG_LINEAR |
      A5O_MIPMAP);

   texture = al_load_bitmap("data/bkg.png");
   if (!texture) {
      abort_example("Could not load data/bkg.png");
   }

   al_start_timer(timer);
   while (!quit) {
      A5O_EVENT event;

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case A5O_EVENT_DISPLAY_CLOSE:
            quit = true;
            break;
         case A5O_EVENT_DISPLAY_RESIZE:
            al_acknowledge_resize(display);
            set_perspective_transform(al_get_backbuffer(display));
            break;
         case A5O_EVENT_KEY_DOWN:
            switch (event.keyboard.keycode) {
               case A5O_KEY_ESCAPE:
                  quit = true;
                  break;
               case A5O_KEY_SPACE:
                  fullscreen = !fullscreen;
                  al_set_display_flag(display, A5O_FULLSCREEN_WINDOW, fullscreen);
                  set_perspective_transform(al_get_backbuffer(display));
                  break;
            }
            break;
         case A5O_EVENT_TIMER:
            redraw = true;
            theta = fmod(theta + 0.05, 2 * A5O_PI);
            break;
         case A5O_EVENT_DISPLAY_HALT_DRAWING:
            background = true;
            al_acknowledge_drawing_halt(display);
            al_stop_timer(timer);
            break;
         case A5O_EVENT_DISPLAY_RESUME_DRAWING:
            background = false;
            al_acknowledge_drawing_resume(display);
            al_start_timer(timer);
            break;
      }

      if (!background && redraw && al_is_event_queue_empty(queue)) {
         al_set_target_backbuffer(display);
         al_set_render_state(A5O_DEPTH_TEST, 1);
         al_clear_to_color(al_map_rgb_f(0, 0, 0));
         al_clear_depth_buffer(1000);
         draw_pyramid(texture, 0, 0, -4, theta);

         al_set_target_bitmap(buffer);
         al_set_render_state(A5O_DEPTH_TEST, 1);
         al_clear_to_color(al_map_rgb_f(0, 0.1, 0.1));
         al_clear_depth_buffer(1000);
         draw_pyramid(texture, 0, 0, -4, theta);

         al_set_target_bitmap(display_sub_persp);
         al_set_render_state(A5O_DEPTH_TEST, 1);
         al_clear_to_color(al_map_rgb_f(0, 0, 0.25));
         al_clear_depth_buffer(1000);
         draw_pyramid(texture, 0, 0, -4, theta);

         al_set_target_bitmap(display_sub_ortho);
         al_set_render_state(A5O_DEPTH_TEST, 0);
         al_draw_text(font, al_map_rgb_f(1, 1, 1), 128, 16, A5O_ALIGN_CENTER,
                      "Press Space to toggle fullscreen");
         al_draw_bitmap(buffer, 0, 256, 0);

         al_flip_display();
         redraw = false;
      }
   }
   return 0;
}
