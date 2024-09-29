/*
 * ex_window_maximized.c - create the maximized, resizable window.
 * Press SPACE to change constraints.
 */

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

#include "common.c"


#define DISPLAY_W 640
#define DISPLAY_H 480

/* comment out to use GTK backend */
/* #define USE_GTK */

static void
draw_information(A5O_DISPLAY *display,
   A5O_FONT *font, A5O_COLOR color);

static bool use_constraints;


extern int
main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_EVENT_QUEUE *queue;
   A5O_FONT *font;
   bool done = false;
   bool redraw = true;
   use_constraints = true;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Failed to init Allegro.\n");
   }

   if (!al_init_primitives_addon()) {
      abort_example("Failed to init primitives addon.\n");
   }

   if (!al_init_image_addon()) {
      abort_example("Failed to init image addon.\n");
   }

   if (!al_init_font_addon()) {
      abort_example("Failed to init font addon.\n");
   }

   if (!al_init_native_dialog_addon()) {
      abort_example("Failed to init native dialog addon.\n");
   }

   al_set_new_display_flags(A5O_WINDOWED
#if defined(USE_GTK) && !defined(_WIN32)
      | A5O_GTK_TOPLEVEL
#endif
      | A5O_RESIZABLE | A5O_MAXIMIZED
      | A5O_GENERATE_EXPOSE_EVENTS);

   /* creating really small display */
   display = al_create_display(DISPLAY_W / 3, DISPLAY_H / 3);
   if (!display) {
      abort_example("Error creating display.\n");
   }

   /* set lower limits for constraints only */
   if(!al_set_window_constraints(display, DISPLAY_W / 2, DISPLAY_H / 2, 0, 0))
      abort_example("Unable to set window constraints.\n");
   al_apply_window_constraints(display, use_constraints);

   if (!al_install_keyboard()) {
      abort_example("Error installing keyboard.\n");
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));

   A5O_COLOR color_1 = al_map_rgb(255, 127, 0);
   A5O_COLOR color_2 = al_map_rgb(0, 255, 0);
   A5O_COLOR *color = &color_1;
   A5O_COLOR color_text = al_map_rgb(0, 0, 0);

   font = al_create_builtin_font();

   while (!done) {
      A5O_EVENT event;

      if (redraw && al_is_event_queue_empty(queue)) {
         redraw = false;
         int x2 = al_get_display_width(display) - 10;
         int y2 = al_get_display_height(display) - 10;
         al_clear_to_color(al_map_rgb(0, 0, 0));
         al_draw_filled_rectangle(10, 10, x2, y2, *color);
         draw_information(display, font, color_text);
         al_flip_display();
      }

      al_wait_for_event(queue, &event);

      switch (event.type) {
      case A5O_EVENT_KEY_DOWN:
         if (event.keyboard.keycode == A5O_KEY_ESCAPE)
            done = true;
         break;

      case A5O_EVENT_KEY_UP:
         if (event.keyboard.keycode == A5O_KEY_SPACE) {
            redraw = true;

            if (color == &color_1) {
               if (!al_set_window_constraints(display,
                  0, 0,
                  DISPLAY_W, DISPLAY_H))
               {
                  abort_example("Unable to set window constraints.\n");
               }

               color = &color_2;
            }
            else {
               if (!al_set_window_constraints(display,
                  DISPLAY_W / 2, DISPLAY_H / 2,
                  0, 0))
               {
                  abort_example("Unable to set window constraints.\n");
               }

               color = &color_1;
            }

            al_apply_window_constraints(display, use_constraints);
         }
         else if (event.keyboard.keycode == A5O_KEY_ENTER) {
            redraw = true;
            use_constraints = !use_constraints;
            al_apply_window_constraints(display, use_constraints);
         }
         break;

      case A5O_EVENT_DISPLAY_RESIZE:
         al_acknowledge_resize(event.display.source);
         redraw = true;
         break;

      case A5O_EVENT_DISPLAY_EXPOSE:
         redraw = true;
         break;

      case A5O_EVENT_DISPLAY_CLOSE:
         done = true;
         break;
      } /* switch (event.type) { */
   }

   al_destroy_font(font);

   return 0;
}


static void
draw_information(A5O_DISPLAY *display,
   A5O_FONT *font, A5O_COLOR color)
{
   int min_w, min_h, max_w, max_h;

   al_draw_textf(font, color, 20, 20, 0, "Hotkeys:");
   al_draw_textf(font, color, 30, 30, 0, "enabled/disable constraints: ENTER");
   al_draw_textf(font, color, 30, 40, 0, "change constraints: SPACE");

   al_draw_textf(font, color, 20, 50, 0,
      "Resolution: %dx%d",
      al_get_display_width(display),
      al_get_display_height(display));

   if (al_get_window_constraints(display, &min_w, &min_h, &max_w, &max_h)) {
      al_draw_textf(font, color, 20, 60, 0, "Constraints: %s",
         use_constraints ? "Enabled" : "Disabled");
      al_draw_textf(font, color, 20, 70, 0, "min_w = %d min_h = %d",
         min_w, min_h);
      al_draw_textf(font, color, 20, 80, 0, "max_w = %d max_h = %d",
         max_w, max_h);
   }
}

/* vim: set sts=3 sw=3 et: */

