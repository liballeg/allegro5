/* An example comparing different ways of drawing polygons, including usage of
 * vertex buffers.
 */

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <math.h>

#include "common.c"

#define FPS 60
#define FRAME_TAU 60
#define NUM_VERTICES 4096

typedef struct METHOD METHOD;

struct METHOD
{
   float x;
   float y;
   ALLEGRO_VERTEX_BUFFER *vbuff;
   ALLEGRO_VERTEX *vertices;
   const char *name;
   int flags;
   double frame_average;
};

static ALLEGRO_COLOR get_color(size_t ii)
{
   double t = al_get_time();
   double frac = (float)(ii) / NUM_VERTICES;
   #define THETA(period) ((t / (period) + frac) * 2 * ALLEGRO_PI)
   return al_map_rgb_f(sin(THETA(5.0)) / 2 + 1, cos(THETA(1.1)) / 2 + 1, sin(THETA(3.4) / 2) / 2 + 1);
}

static void draw_method(METHOD *md, ALLEGRO_FONT *font, ALLEGRO_VERTEX* new_vertices)
{
   ALLEGRO_TRANSFORM t;
   double start_time;
   double new_fps;
   ALLEGRO_COLOR c;

   al_identity_transform(&t);
   al_translate_transform(&t, md->x, md->y);
   al_use_transform(&t);

   al_draw_textf(font, al_map_rgb_f(1, 1, 1), 0, -50, ALLEGRO_ALIGN_CENTRE, "%s%s", md->name,
                 md->flags & ALLEGRO_PRIM_BUFFER_READWRITE ? "+read/write" : "+write-only");

   start_time = al_get_time();
   if (md->vbuff) {
      if (new_vertices) {
         void* lock_mem = al_lock_vertex_buffer(md->vbuff, 0, NUM_VERTICES, ALLEGRO_LOCK_WRITEONLY);
         memcpy(lock_mem, new_vertices, sizeof(ALLEGRO_VERTEX) * NUM_VERTICES);
         al_unlock_vertex_buffer(md->vbuff);
      }
      al_draw_vertex_buffer(md->vbuff, 0, 0, NUM_VERTICES, ALLEGRO_PRIM_TRIANGLE_STRIP);
   }
   else if (md->vertices) {
      al_draw_prim(md->vertices, 0, 0, 0, NUM_VERTICES, ALLEGRO_PRIM_TRIANGLE_STRIP);
   }

   /* Force the completion of the previous commands by reading from screen */
   c = al_get_pixel(al_get_backbuffer(al_get_current_display()), 0, 0);
   (void)c;

   new_fps = 1.0 / (al_get_time() - start_time);
   md->frame_average += (new_fps - md->frame_average) / FRAME_TAU;

   if (md->vbuff || md->vertices) {
      al_draw_textf(font, al_map_rgb_f(1, 0, 0), 0, 0, ALLEGRO_ALIGN_CENTRE, "%.1e FPS", md->frame_average);
   }
   else {
      al_draw_text(font, al_map_rgb_f(1, 0, 0), 0, 0, ALLEGRO_ALIGN_CENTRE, "N/A");
   }

   al_identity_transform(&t);
   al_use_transform(&t);
}

int main(int argc, char **argv)
{
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_DISPLAY *display;
   ALLEGRO_FONT *font;
   int w = 640, h = 480;
   bool done = false;
   bool need_redraw = true;
   bool background = false;
   bool dynamic_buffers = false;
   size_t num_methods;
   int num_x = 3;
   int num_y = 3;
   int spacing_x = 200;
   int spacing_y = 150;
   size_t ii;
   ALLEGRO_VERTEX* vertices;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Failed to init Allegro.\n");
   }

   al_init_font_addon();
   al_init_primitives_addon();

   #ifdef ALLEGRO_IPHONE
   al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);
   #endif
   al_set_new_display_option(ALLEGRO_SUPPORTED_ORIENTATIONS,
                             ALLEGRO_DISPLAY_ORIENTATION_ALL, ALLEGRO_SUGGEST);
   display = al_create_display(w, h);
   if (!display) {
      abort_example("Error creating display.\n");
   }

   w = al_get_display_width(display);
   h = al_get_display_height(display);

   al_install_keyboard();

   font = al_create_builtin_font();
   timer = al_create_timer(1.0 / FPS);
   queue = al_create_event_queue();

   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_register_event_source(queue, al_get_display_event_source(display));

   vertices = al_malloc(sizeof(ALLEGRO_VERTEX) * NUM_VERTICES);
   al_calculate_arc(&vertices[0].x, sizeof(ALLEGRO_VERTEX), 0, 0, 80, 30, 0, 2 * ALLEGRO_PI, 10, NUM_VERTICES / 2);
   for (ii = 0; ii < NUM_VERTICES; ii++) {
      vertices[ii].z = 0;
      vertices[ii].color = get_color(ii);
   }

   {
      #define GETX(n) ((w - (num_x - 1) * spacing_x) / 2 + n * spacing_x)
      #define GETY(n) ((h - (num_y - 1) * spacing_y) / 2 + n * spacing_y)

      METHOD methods[] =
      {
         {GETX(1), GETY(0), 0, vertices, "No buffer", 0, 0},
         {GETX(0), GETY(1), 0, 0, "STREAM", ALLEGRO_PRIM_BUFFER_STREAM | ALLEGRO_PRIM_BUFFER_READWRITE, 0},
         {GETX(1), GETY(1), 0, 0, "STATIC", ALLEGRO_PRIM_BUFFER_STATIC | ALLEGRO_PRIM_BUFFER_READWRITE, 0},
         {GETX(2), GETY(1), 0, 0, "DYNAMIC", ALLEGRO_PRIM_BUFFER_STREAM | ALLEGRO_PRIM_BUFFER_READWRITE, 0},
         {GETX(0), GETY(2), 0, 0, "STREAM", ALLEGRO_PRIM_BUFFER_STREAM, 0},
         {GETX(1), GETY(2), 0, 0, "STATIC", ALLEGRO_PRIM_BUFFER_STATIC, 0},
         {GETX(2), GETY(2), 0, 0, "DYNAMIC", ALLEGRO_PRIM_BUFFER_DYNAMIC, 0}
      };

      num_methods = sizeof(methods) / sizeof(METHOD);

      for (ii = 0; ii < num_methods; ii++) {
         METHOD* md = &methods[ii];
         md->frame_average = 1;
         if (!md->vertices)
            md->vbuff = al_create_vertex_buffer(0, vertices, NUM_VERTICES, md->flags);
      }

      al_start_timer(timer);

      while (!done) {
         ALLEGRO_EVENT event;
         if (!background && need_redraw && al_is_event_queue_empty(queue)) {
            al_clear_to_color(al_map_rgb_f(0, 0, 0.2));

            for (ii = 0; ii < num_methods; ii++)
               draw_method(&methods[ii], font, dynamic_buffers ? vertices : 0);

            al_draw_textf(font, al_map_rgb_f(1, 1, 1), 10, 10, 0, "Dynamic (D): %s", dynamic_buffers ? "yes" : "no");

            al_flip_display();
            need_redraw = false;
         }

         al_wait_for_event(queue, &event);
         switch (event.type) {
            case ALLEGRO_EVENT_KEY_DOWN:
               if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
                  done = true;
               else if(event.keyboard.keycode == ALLEGRO_KEY_D)
                  dynamic_buffers = !dynamic_buffers;
               break;
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
               done = true;
               break;
            case ALLEGRO_EVENT_DISPLAY_HALT_DRAWING:
               background = true;
               al_acknowledge_drawing_halt(event.display.source);
               break;
            case ALLEGRO_EVENT_DISPLAY_RESIZE:
               al_acknowledge_resize(event.display.source);
               break;
            case ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING:
               background = false;
               break;
            case ALLEGRO_EVENT_TIMER:
               for (ii = 0; ii < NUM_VERTICES; ii++)
                  vertices[ii].color = get_color(ii);
               need_redraw = true;
               break;
         }
      }

      for (ii = 0; ii < num_methods; ii++)
         al_destroy_vertex_buffer(methods[ii].vbuff);

      al_destroy_font(font);
      al_free(vertices);
   }

   return 0;
}
