/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    This program tests the polygon routines in the primitives addon.
 */

#define A5O_UNSTABLE
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <ctype.h>
#include <math.h>

#include "common.c"

#define MAX_VERTICES 64
#define MAX_POLYGONS 9
#define RADIUS       5

enum {
   MODE_POLYLINE = 0,
   MODE_POLYGON,
   MODE_FILLED_POLYGON,
   MODE_FILLED_HOLES,
   MODE_MAX
};

enum AddHole {
   NOT_ADDING_HOLE,
   NEW_HOLE,
   GROW_HOLE
};

typedef struct Vertex Vertex;
struct Vertex {
   float x;
   float y;
};

struct Example {
   A5O_DISPLAY   *display;
   A5O_FONT      *font;
   A5O_FONT      *fontbmp;
   A5O_EVENT_QUEUE *queue;
   A5O_BITMAP    *dbuf;
   A5O_COLOR     bg;
   A5O_COLOR     fg;
   Vertex            vertices[MAX_VERTICES];
   int               vertex_polygon[MAX_VERTICES];
   int               vertex_count;
   int               cur_vertex; /* -1 = none */
   int               cur_polygon;
   int               mode;
   A5O_LINE_CAP  cap_style;
   A5O_LINE_JOIN join_style;
   float             thickness;
   float             miter_limit;
   bool              software;
   float             zoom;
   float             scroll_x, scroll_y;
   enum AddHole      add_hole;
};

static struct Example ex;

static void reset(void)
{
   ex.vertex_count = 0;
   ex.cur_vertex = -1;
   ex.cur_polygon = 0;
   ex.cap_style = A5O_LINE_CAP_NONE;
   ex.join_style = A5O_LINE_JOIN_NONE;
   ex.thickness = 1.0f;
   ex.miter_limit = 1.0f;
   ex.software = false;
   ex.zoom = 1;
   ex.scroll_x = 0;
   ex.scroll_y = 0;
   ex.add_hole = NOT_ADDING_HOLE;
}

static void transform(float *x, float *y)
{
   *x /= ex.zoom;
   *y /= ex.zoom;
   *x -= ex.scroll_x;
   *y -= ex.scroll_y;
}

static int hit_vertex(int mx, int my)
{
   int i;

   for (i = 0; i < ex.vertex_count; i++) {
      int dx = ex.vertices[i].x - mx;
      int dy = ex.vertices[i].y - my;
      int dd = dx*dx + dy*dy;
      if (dd <= RADIUS * RADIUS)
         return i;
   }

   return -1;
}

static void lclick(int mx, int my)
{
   ex.cur_vertex = hit_vertex(mx, my);

   if (ex.cur_vertex < 0 && ex.vertex_count < MAX_VERTICES) {
      int i = ex.vertex_count++;
      ex.vertices[i].x = mx;
      ex.vertices[i].y = my;
      ex.cur_vertex = i;

      if (ex.add_hole == NEW_HOLE && ex.cur_polygon < MAX_POLYGONS) {
         ex.cur_polygon++;
         ex.add_hole = GROW_HOLE;
      }

      ex.vertex_polygon[i] = ex.cur_polygon;
   }
}

static void rclick(int mx, int my)
{
   const int i = hit_vertex(mx, my);
   if (i >= 0 && ex.add_hole == NOT_ADDING_HOLE) {
      ex.vertex_count--;
      memmove(&ex.vertices[i], &ex.vertices[i + 1],
         sizeof(Vertex) * (ex.vertex_count - i));
      memmove(&ex.vertex_polygon[i], &ex.vertex_polygon[i + 1],
         sizeof(int) * (ex.vertex_count - i));
   }
   ex.cur_vertex = -1;
}

static void drag(int mx, int my)
{
   if (ex.cur_vertex >= 0) {
      ex.vertices[ex.cur_vertex].x = mx;
      ex.vertices[ex.cur_vertex].y = my;
   }
}

static void scroll(int mx, int my)
{
   ex.scroll_x += mx;
   ex.scroll_y += my;
}

static const char *join_style_to_string(A5O_LINE_JOIN x)
{
   switch (x) {
      case A5O_LINE_JOIN_NONE:
         return "NONE";
      case A5O_LINE_JOIN_BEVEL:
         return "BEVEL";
      case A5O_LINE_JOIN_ROUND:
         return "ROUND";
      case A5O_LINE_JOIN_MITER:
         return "MITER";
      default:
         return "unknown";
   }
}

static const char *cap_style_to_string(A5O_LINE_CAP x)
{
   switch (x) {
      case A5O_LINE_CAP_NONE:
         return "NONE";
      case A5O_LINE_CAP_SQUARE:
         return "SQUARE";
      case A5O_LINE_CAP_ROUND:
         return "ROUND";
      case A5O_LINE_CAP_TRIANGLE:
         return "TRIANGLE";
      case A5O_LINE_CAP_CLOSED:
         return "CLOSED";
      default:
         return "unknown";
   }
}

static A5O_FONT *choose_font(void)
{
   return (ex.software) ? ex.fontbmp : ex.font;
}

static void draw_vertices(void)
{
   A5O_FONT *f = choose_font();
   A5O_COLOR vertc = al_map_rgba_f(0.7, 0, 0, 0.7);
   A5O_COLOR textc = al_map_rgba_f(0, 0, 0, 0.7);
   int i;

   for (i = 0; i < ex.vertex_count; i++) {
      float x = ex.vertices[i].x;
      float y = ex.vertices[i].y;

      al_draw_filled_circle(x, y, RADIUS, vertc);
      al_draw_textf(f, textc, x + RADIUS, y + RADIUS, 0, "%d", i);
   }
}

static void compute_polygon_vertex_counts(
   int polygon_vertex_count[MAX_POLYGONS + 1])
{
   int i;

   /* This also implicitly terminates the array with a zero. */
   memset(polygon_vertex_count, 0, sizeof(int) * (MAX_POLYGONS + 1));
   for (i = 0; i < ex.vertex_count; i++) {
      const int poly = ex.vertex_polygon[i];
      polygon_vertex_count[poly]++;
   }
}

static void draw_all(void)
{
   A5O_FONT *f = choose_font();
   A5O_COLOR textc = al_map_rgb(0, 0, 0);
   float texth = al_get_font_line_height(f) * 1.5;
   float textx = 5;
   float texty = 5;
   A5O_TRANSFORM t;
   A5O_COLOR holec;

   al_clear_to_color(ex.bg);

   al_identity_transform(&t);
   al_translate_transform(&t, ex.scroll_x, ex.scroll_y);
   al_scale_transform(&t, ex.zoom, ex.zoom);
   al_use_transform(&t);

   if (ex.mode == MODE_POLYLINE) {
      if (ex.vertex_count >= 2) {
         al_draw_polyline(
            (float *)ex.vertices, sizeof(Vertex), ex.vertex_count,
            ex.join_style, ex.cap_style, ex.fg, ex.thickness, ex.miter_limit);
      }
   }
   else if (ex.mode == MODE_FILLED_POLYGON) {
      if (ex.vertex_count >= 2) {
         al_draw_filled_polygon(
            (float *)ex.vertices, ex.vertex_count, ex.fg);
      }
   }
   else if (ex.mode == MODE_POLYGON) {
      if (ex.vertex_count >= 2) {
         al_draw_polygon(
            (float *)ex.vertices, ex.vertex_count,
            ex.join_style, ex.fg, ex.thickness, ex.miter_limit);
      }
   }
   else if (ex.mode == MODE_FILLED_HOLES) {
      if (ex.vertex_count >= 2) {
         int polygon_vertex_count[MAX_POLYGONS + 1];
         compute_polygon_vertex_counts(polygon_vertex_count);
         al_draw_filled_polygon_with_holes(
            (float *)ex.vertices, polygon_vertex_count, ex.fg);
      }
   }

   draw_vertices();

   al_identity_transform(&t);
   al_use_transform(&t);

   if (ex.mode == MODE_POLYLINE) {
      al_draw_textf(f, textc, textx, texty, 0,
         "al_draw_polyline (SPACE)");
      texty += texth;
   }
   else if (ex.mode == MODE_FILLED_POLYGON) {
      al_draw_textf(f, textc, textx, texty, 0,
         "al_draw_filled_polygon (SPACE)");
      texty += texth;
   }
   else if (ex.mode == MODE_POLYGON) {
      al_draw_textf(f, textc, textx, texty, 0,
         "al_draw_polygon (SPACE)");
      texty += texth;
   }
   else if (ex.mode == MODE_FILLED_HOLES) {
      al_draw_textf(f, textc, textx, texty, 0,
         "al_draw_filled_polygon_with_holes (SPACE)");
      texty += texth;
   }

   al_draw_textf(f, textc, textx, texty, 0,
      "Line join style: %s (J)", join_style_to_string(ex.join_style));
   texty += texth;

   al_draw_textf(f, textc, textx, texty, 0,
      "Line cap style:  %s (C)", cap_style_to_string(ex.cap_style));
   texty += texth;

   al_draw_textf(f, textc, textx, texty, 0,
      "Line thickness:  %.2f (+/-)", ex.thickness);
   texty += texth;

   al_draw_textf(f, textc, textx, texty, 0,
      "Miter limit:     %.2f ([/])", ex.miter_limit);
   texty += texth;

   al_draw_textf(f, textc, textx, texty, 0,
      "Zoom:            %.2f (wheel)", ex.zoom);
   texty += texth;

   al_draw_textf(f, textc, textx, texty, 0,
      "%s (S)", (ex.software ? "Software rendering" : "Hardware rendering"));
   texty += texth;

   al_draw_textf(f, textc, textx, texty, 0,
      "Reset (R)");
   texty += texth;

   if (ex.add_hole == NOT_ADDING_HOLE)
      holec = textc;
   else if (ex.add_hole == GROW_HOLE)
      holec = al_map_rgb(200, 0, 0);
   else
      holec = al_map_rgb(0, 200, 0);
   al_draw_textf(f, holec, textx, texty, 0,
      "Add Hole (%d) (H)", ex.cur_polygon);
   texty += texth;
}

/* Print vertices in a format for the test suite. */
static void print_vertices(void)
{
   int i;

   for (i = 0; i < ex.vertex_count; i++) {
      log_printf("v%-2d= %.2f, %.2f\n",
         i, ex.vertices[i].x, ex.vertices[i].y);
   }
   log_printf("\n");
}

int main(int argc, char **argv)
{
   A5O_EVENT event;
   bool have_touch_input;
   bool mdown = false;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   open_log();

   if (!al_init_primitives_addon()) {
      abort_example("Could not init primitives.\n");
   }
   al_init_font_addon();
   al_install_mouse();
   al_install_keyboard();
   have_touch_input = al_install_touch_input();

   ex.display = al_create_display(800, 600);
   if (!ex.display) {
      abort_example("Error creating display\n");
   }

   ex.font = al_create_builtin_font();
   if (!ex.font) {
      abort_example("Error creating builtin font\n");
   }

   al_set_new_bitmap_flags(A5O_MEMORY_BITMAP);
   ex.dbuf = al_create_bitmap(800, 600);
   ex.fontbmp = al_create_builtin_font();
   if (!ex.fontbmp) {
      abort_example("Error creating builtin font\n");
   }

   ex.queue = al_create_event_queue();
   al_register_event_source(ex.queue, al_get_keyboard_event_source());
   al_register_event_source(ex.queue, al_get_mouse_event_source());
   al_register_event_source(ex.queue, al_get_display_event_source(ex.display));
   if (have_touch_input) {
      al_register_event_source(ex.queue, al_get_touch_input_event_source());
      al_register_event_source(ex.queue, al_get_touch_input_mouse_emulation_event_source());
   }

   ex.bg = al_map_rgba_f(1, 1, 0.9, 1);
   ex.fg = al_map_rgba_f(0, 0.5, 1, 1);

   reset();

   for (;;) {
      if (al_is_event_queue_empty(ex.queue)) {
         if (ex.software) {
            al_set_target_bitmap(ex.dbuf);
            draw_all();
            al_set_target_backbuffer(ex.display);
            al_draw_bitmap(ex.dbuf, 0, 0, 0);
         }
         else {
            al_set_target_backbuffer(ex.display);
            draw_all();
         }
         al_flip_display();
      }

      al_wait_for_event(ex.queue, &event);
      if (event.type == A5O_EVENT_DISPLAY_CLOSE)
         break;
      if (event.type == A5O_EVENT_KEY_CHAR) {
         if (event.keyboard.keycode == A5O_KEY_ESCAPE)
            break;
         switch (toupper(event.keyboard.unichar)) {
            case ' ':
               if (++ex.mode >= MODE_MAX)
                  ex.mode = 0;
               break;
            case 'J':
               if (++ex.join_style > A5O_LINE_JOIN_MITER)
                  ex.join_style = A5O_LINE_JOIN_NONE;
               break;
            case 'C':
               if (++ex.cap_style > A5O_LINE_CAP_CLOSED)
                  ex.cap_style = A5O_LINE_CAP_NONE;
               break;
            case '+':
               ex.thickness += 0.25f;
               break;
            case '-':
               ex.thickness -= 0.25f;
               if (ex.thickness <= 0.0f)
                  ex.thickness = 0.0f;
               break;
            case '[':
               ex.miter_limit -= 0.1f;
               if (ex.miter_limit < 0.0f)
                  ex.miter_limit = 0.0f;
               break;
            case ']':
               ex.miter_limit += 0.1f;
               if (ex.miter_limit >= 10.0f)
                  ex.miter_limit = 10.0f;
               break;
            case 'S':
               ex.software = !ex.software;
               break;
            case 'R':
               reset();
               break;
            case 'P':
               print_vertices();
               break;
            case 'H':
               ex.add_hole = NEW_HOLE;
               break;
         }
      }
      if (event.type == A5O_EVENT_MOUSE_BUTTON_DOWN) {
         float x = event.mouse.x, y = event.mouse.y;
         transform(&x, &y);
         if (event.mouse.button == 1)
            lclick(x, y);
         if (event.mouse.button == 2)
            rclick(x, y);
         if (event.mouse.button == 3)
            mdown = true;
      }
      if (event.type == A5O_EVENT_MOUSE_BUTTON_UP) {
         ex.cur_vertex = -1;
         if (event.mouse.button == 3)
            mdown = false;
      }
      if (event.type == A5O_EVENT_MOUSE_AXES) {
         float x = event.mouse.x, y = event.mouse.y;
         transform(&x, &y);

         if (mdown)
            scroll(event.mouse.dx, event.mouse.dy);
         else
            drag(x, y);

         ex.zoom *= pow(0.9, event.mouse.dz);
      }
   }

   al_destroy_display(ex.display);
   close_log(true);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
