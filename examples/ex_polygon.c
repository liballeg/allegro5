/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    This program tests the polygon routines in the primitives addon.
 */

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <ctype.h>

#include "common.c"

#define MAX_VERTICES 64
#define RADIUS       5

enum {
   MODE_POLYLINE = 0,
   MODE_POLYGON,
   MODE_FILLED_POLYGON,
   MODE_MAX
};

typedef struct Vertex Vertex;
struct Vertex {
   float x;
   float y;
};

struct Example {
   ALLEGRO_DISPLAY   *display;
   ALLEGRO_FONT      *font;
   ALLEGRO_FONT      *fontbmp;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_BITMAP    *dbuf;
   ALLEGRO_COLOR     bg;
   ALLEGRO_COLOR     fg;
   Vertex            vertices[MAX_VERTICES];
   int               vertex_count;
   int               cur_vertex; /* -1 = none */
   int               mode;
   ALLEGRO_LINE_CAP  cap_style;
   ALLEGRO_LINE_JOIN join_style;
   float             thickness;
   float             miter_limit;
   bool              software;
};

static struct Example ex;

static void reset(void)
{
   ex.vertex_count = 0;
   ex.cur_vertex = -1;
   ex.mode = MODE_POLYLINE;
   ex.cap_style = ALLEGRO_LINE_CAP_NONE;
   ex.join_style = ALLEGRO_LINE_JOIN_NONE;
   ex.thickness = 1.0f;
   ex.miter_limit = 1.0f;
   ex.software = false;
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
   }
}

static void rclick(int mx, int my)
{
   const int i = hit_vertex(mx, my);
   if (i >= 0) {
      ex.vertex_count--;
      memmove(&ex.vertices[i], &ex.vertices[i + 1],
         sizeof(Vertex) * (ex.vertex_count - i));
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

static const char *join_style_to_string(ALLEGRO_LINE_JOIN x)
{
   switch (x) {
      case ALLEGRO_LINE_JOIN_NONE:
         return "NONE";
      case ALLEGRO_LINE_JOIN_BEVEL:
         return "BEVEL";
      case ALLEGRO_LINE_JOIN_ROUND:
         return "ROUND";
      case ALLEGRO_LINE_JOIN_MITER:
         return "MITER";
      default:
         return "unknown";
   }
}

static const char *cap_style_to_string(ALLEGRO_LINE_CAP x)
{
   switch (x) {
      case ALLEGRO_LINE_CAP_NONE:
         return "NONE";
      case ALLEGRO_LINE_CAP_SQUARE:
         return "SQUARE";
      case ALLEGRO_LINE_CAP_ROUND:
         return "ROUND";
      case ALLEGRO_LINE_CAP_TRIANGLE:
         return "TRIANGLE";
      case ALLEGRO_LINE_CAP_CLOSED:
         return "CLOSED";
      default:
         return "unknown";
   }
}

static ALLEGRO_FONT *choose_font(void)
{
   return (ex.software) ? ex.fontbmp : ex.font;
}

static void draw_vertices(void)
{
   ALLEGRO_FONT *f = choose_font();
   ALLEGRO_COLOR vertc = al_map_rgba_f(0.7, 0, 0, 0.7);
   ALLEGRO_COLOR textc = al_map_rgba_f(0, 0, 0, 0.7);
   int i;

   for (i = 0; i < ex.vertex_count; i++) {
      float x = ex.vertices[i].x;
      float y = ex.vertices[i].y;

      al_draw_filled_circle(x, y, RADIUS, vertc);
      al_draw_textf(f, textc, x + RADIUS, y + RADIUS, 0, "%d", i);
   }
}

static void draw_all(void)
{
   ALLEGRO_FONT *f = choose_font();
   ALLEGRO_COLOR textc = al_map_rgb(0, 0, 0);
   float texth = al_get_font_line_height(f) * 1.5;
   float textx = 5;
   float texty = 5;

   al_clear_to_color(ex.bg);

   if (ex.mode == MODE_POLYLINE) {
      if (ex.vertex_count >= 2) {
         al_draw_polyline_ex(
            (float *)ex.vertices, sizeof(Vertex), ex.vertex_count,
            ex.join_style, ex.cap_style, ex.fg, ex.thickness, ex.miter_limit);
      }
      al_draw_textf(f, textc, textx, texty, 0,
         "al_draw_polyline (SPACE)");
      texty += texth;
   }
   else if (ex.mode == MODE_FILLED_POLYGON) {
      if (ex.vertex_count >= 2) {
         al_draw_filled_polygon(
            (float *)ex.vertices, ex.vertex_count, ex.fg);
      }
      al_draw_textf(f, textc, textx, texty, 0,
         "al_draw_filled_polygon (SPACE)");
      texty += texth;
   }
   else if (ex.mode == MODE_POLYGON) {
      if (ex.vertex_count >= 2) {
         al_draw_polygon(
            (float *)ex.vertices, ex.vertex_count,
            ex.join_style, ex.fg, ex.thickness, ex.miter_limit);
      }
      al_draw_textf(f, textc, textx, texty, 0,
         "al_draw_polygon (SPACE)");
      texty += texth;
   }

   draw_vertices();

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
      "%s (S)", (ex.software ? "Software rendering" : "Hardware rendering"));
   texty += texth;

   al_draw_textf(f, textc, textx, texty, 0,
      "Reset (R)");
   texty += texth;
}

/* Print vertices in a format for the test suite. */
static void print_vertices(void)
{
   int i;

   for (i = 0; i < ex.vertex_count; i++) {
      printf("v%-2d= %.2f, %.2f\n",
         i, ex.vertices[i].x, ex.vertices[i].y);
   }
   printf("\n");
}

int main(void)
{
   ALLEGRO_EVENT event;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   if (!al_init_primitives_addon()) {
      abort_example("Could not init primitives.\n");
   }
   al_init_image_addon();
   al_init_font_addon();
   al_install_mouse();
   al_install_keyboard();

   ex.display = al_create_display(800, 600);
   if (!ex.display) {
      abort_example("Error creating display\n");
   }

   ex.font = al_load_font("data/a4_font.tga", 0, 0);
   if (!ex.font) {
      abort_example("Error loading data/a4_font.tga\n");
   }

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   ex.dbuf = al_create_bitmap(800, 600);
   ex.fontbmp = al_load_font("data/a4_font.tga", 0, 0);
   if (!ex.fontbmp) {
      abort_example("Error loading data/a4_font.tga\n");
   }

   ex.queue = al_create_event_queue();
   al_register_event_source(ex.queue, al_get_keyboard_event_source());
   al_register_event_source(ex.queue, al_get_mouse_event_source());
   al_register_event_source(ex.queue, al_get_display_event_source(ex.display));

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
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
         break;
      if (event.type == ALLEGRO_EVENT_KEY_CHAR) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
            break;
         switch (toupper(event.keyboard.unichar)) {
            case ' ':
               if (++ex.mode >= MODE_MAX)
                  ex.mode = 0;
               break;
            case 'J':
               if (++ex.join_style > ALLEGRO_LINE_JOIN_MITER)
                  ex.join_style = ALLEGRO_LINE_JOIN_NONE;
               break;
            case 'C':
               if (++ex.cap_style > ALLEGRO_LINE_CAP_CLOSED)
                  ex.cap_style = ALLEGRO_LINE_CAP_NONE;
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
         }
      }
      if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
         if (event.mouse.button == 1)
            lclick(event.mouse.x, event.mouse.y);
         if (event.mouse.button == 2)
            rclick(event.mouse.x, event.mouse.y);
      }
      if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
         ex.cur_vertex = -1;
      }
      if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
         drag(event.mouse.x, event.mouse.y);
      }
   }

   return 0;
}

/* vim: set sts=3 sw=3 et: */
