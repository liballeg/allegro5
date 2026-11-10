/*
 * Wayland backend test bed.
 *
 * This intentionally exercises the Wayland-specific cursor and EGL paths
 * that ex_wayland does not cover:
 *   - ALLEGRO_VSYNC at display creation time
 *   - named system cursors
 *   - custom wl_shm bitmap cursors
 *   - hiding/showing the cursor with wl_pointer.set_cursor(NULL)
 *   - al_wait_for_vsync's current backend result
 *
 * Usage:
 *   ex_wayland2 [0|1|2]
 *
 * The optional argument selects ALLEGRO_VSYNC: 0 = compositor default,
 * 1 = force on, 2 = force off.
 *
 * Keys:
 *   [ / ]     previous/next system cursor
 *   C         use the custom bitmap cursor
 *   H / S     hide/show cursor
 *   W         call al_wait_for_vsync
 *   F         toggle fullscreen
 *   M         toggle maximized
 *   B         toggle frameless mode
 *   R         request a resize
 *   T         cycle the window title
 *   Q / ESC   quit
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_primitives.h"

#include "common.c"

#define LOG_LINES 12
#define LOG_LENGTH 160

static char log_lines[LOG_LINES][LOG_LENGTH];

static void add_log(const char *format, ...)
{
   va_list args;

   memmove(log_lines[1], log_lines[0],
      (LOG_LINES - 1) * sizeof log_lines[0]);
   va_start(args, format);
   vsnprintf(log_lines[0], sizeof log_lines[0], format, args);
   va_end(args);
}

typedef struct CursorChoice {
   ALLEGRO_SYSTEM_MOUSE_CURSOR id;
   const char *name;
} CursorChoice;

static const CursorChoice cursor_choices[] = {
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT, "DEFAULT" },
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_ARROW, "ARROW" },
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_BUSY, "BUSY" },
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_QUESTION, "QUESTION" },
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_EDIT, "EDIT" },
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_MOVE, "MOVE" },
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_N, "RESIZE_N" },
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_W, "RESIZE_W" },
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_S, "RESIZE_S" },
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_E, "RESIZE_E" },
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_NW, "RESIZE_NW" },
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_SW, "RESIZE_SW" },
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_SE, "RESIZE_SE" },
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_NE, "RESIZE_NE" },
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_PROGRESS, "PROGRESS" },
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_PRECISION, "PRECISION" },
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_LINK, "LINK" },
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_ALT_SELECT, "ALT_SELECT" },
   { ALLEGRO_SYSTEM_MOUSE_CURSOR_UNAVAILABLE, "UNAVAILABLE" },
};

#define NUM_CURSOR_CHOICES ((int)(sizeof cursor_choices / sizeof cursor_choices[0]))

typedef struct TestState {
   ALLEGRO_DISPLAY *display;
   ALLEGRO_FONT *font;
   ALLEGRO_MOUSE_CURSOR *custom_cursor;
   int cursor_index;
   const char *cursor_name;
   bool custom_active;
   bool cursor_hidden;
   bool fullscreen;
   bool maximized;
   bool frameless;
   int title_index;
   int resize_index;
   int requested_vsync;
} TestState;

static void make_custom_cursor(TestState *state)
{
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_BITMAP *old_target = al_get_target_bitmap();

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   bitmap = al_create_bitmap(24, 24);
   if (!bitmap) {
      add_log("custom cursor bitmap creation failed");
      return;
   }

   al_set_target_bitmap(bitmap);
   al_clear_to_color(al_map_rgba(0, 0, 0, 0));
   al_draw_filled_triangle(2, 2, 2, 21, 20, 13,
      al_map_rgba(255, 220, 40, 255));
   al_draw_line(2, 2, 2, 21, al_map_rgb(20, 20, 20), 2);
   al_draw_line(2, 2, 20, 13, al_map_rgb(20, 20, 20), 2);
   al_draw_line(2, 21, 20, 13, al_map_rgb(20, 20, 20), 2);
   al_draw_filled_circle(8, 8, 2, al_map_rgb(255, 255, 255));

   state->custom_cursor = al_create_mouse_cursor(bitmap, 2, 2);
   if (state->custom_cursor)
      add_log("created custom 24x24 wl_shm cursor");
   else
      add_log("custom cursor creation failed");

   al_set_target_bitmap(old_target);
   al_destroy_bitmap(bitmap);
}

static void select_system_cursor(TestState *state, int direction)
{
   bool ok;

   state->cursor_index = (state->cursor_index + direction + NUM_CURSOR_CHOICES)
      % NUM_CURSOR_CHOICES;
   ok = al_set_system_mouse_cursor(state->display,
      cursor_choices[state->cursor_index].id);
   state->custom_active = false;
   state->cursor_name = cursor_choices[state->cursor_index].name;
   add_log("system cursor %s -> %s", state->cursor_name,
      ok ? "true" : "false");
}

static void select_custom_cursor(TestState *state)
{
   bool ok = state->custom_cursor
      && al_set_mouse_cursor(state->display, state->custom_cursor);
   if (ok) {
      state->custom_active = true;
      state->cursor_name = "CUSTOM_BITMAP";
   }
   add_log("custom bitmap cursor -> %s", ok ? "true" : "false");
}

static void draw_ui(TestState *state, double now)
{
   ALLEGRO_DISPLAY *display = state->display;
   ALLEGRO_FONT *font = state->font;
   int w = al_get_display_width(display);
   int h = al_get_display_height(display);
   ALLEGRO_COLOR bg = al_map_rgb_f(0.08, 0.10, 0.14);
   ALLEGRO_COLOR panel = al_map_rgba_f(0, 0, 0, 0.55);
   ALLEGRO_COLOR white = al_map_rgb_f(0.92, 0.95, 1.0);
   ALLEGRO_COLOR dim = al_map_rgb_f(0.58, 0.64, 0.74);
   ALLEGRO_COLOR cyan = al_map_rgb_f(0.20, 0.90, 1.0);
   ALLEGRO_COLOR orange = al_map_rgb_f(1.0, 0.55, 0.10);
   ALLEGRO_MOUSE_STATE mouse_state;
   int i;

   al_get_mouse_state(&mouse_state);
   al_clear_to_color(bg);

   for (i = 0; i < w; i += 40)
      al_draw_line(i + 0.5, 0, i + 0.5, h,
         al_map_rgba_f(1, 1, 1, 0.035), 1);
   for (i = 0; i < h; i += 40)
      al_draw_line(0, i + 0.5, w, i + 0.5,
         al_map_rgba_f(1, 1, 1, 0.035), 1);

   al_draw_filled_rounded_rectangle(10, 10, w - 10, 48, 6, 6, panel);
   al_draw_text(font, white, 22, 17, 0, "Wayland backend test bed");
   al_draw_textf(font, cyan, 330, 17, 0,
      "vsync request %d / effective %d",
      state->requested_vsync,
      al_get_display_option(display, ALLEGRO_VSYNC));

   al_draw_filled_rounded_rectangle(10, 60, 430, 280, 6, 6, panel);
   al_draw_text(font, cyan, 22, 70, 0, "Cursor tests");
   al_draw_textf(font, white, 22, 92, 0, "active: %s", state->cursor_name);
   al_draw_textf(font, dim, 22, 110, 0, "hidden: %s",
      state->cursor_hidden ? "yes" : "no");
   al_draw_text(font, dim, 22, 140, 0, "[ / ]  cycle system cursors");
   al_draw_text(font, dim, 22, 158, 0, "C      custom wl_shm bitmap");
   al_draw_text(font, dim, 22, 176, 0, "H / S  hide / show cursor");
   al_draw_textf(font, dim, 22, 205, 0, "mouse: %d,%d buttons 0x%x",
      mouse_state.x, mouse_state.y, mouse_state.buttons);
   al_draw_textf(font, dim, 22, 223, 0, "wheel: z=%d w=%d",
      mouse_state.z, mouse_state.w);

   al_draw_text(font, cyan, 22, 252, 0, "Window / EGL tests");
   al_draw_text(font, dim, 22, 272, 0, "W  al_wait_for_vsync");
   al_draw_text(font, dim, 22, 290, 0, "F/M/B  fullscreen/maximize/frameless");
   al_draw_text(font, dim, 22, 308, 0, "R/T  resize/title");

   al_draw_filled_rounded_rectangle(450, 60, w - 10, 340, 6, 6, panel);
   al_draw_text(font, cyan, 462, 70, 0, "Event log");
   for (i = 0; i < LOG_LINES; i++) {
      if (log_lines[i][0])
         al_draw_text(font, i == 0 ? white : dim, 462, 92 + i * 18,
            0, log_lines[i]);
   }

   al_draw_filled_circle(w - 70 + sin(now * 2) * 25, h - 50, 18, orange);
   al_draw_text(font, dim, 22, h - 28, 0,
      "ESC/Q quit | pointer movement tests enter/leave and cursor reapply");
   al_flip_display();
}

static void toggle_flag(TestState *state, int flag, const char *name,
   bool *value)
{
   bool requested = !*value;
   bool ok = al_set_display_flag(state->display, flag, requested);
   add_log("%s %d -> %s", name, requested, ok ? "true" : "false");
   if (ok)
      *value = requested;
}

int main(int argc, char **argv)
{
   TestState state;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_TIMER *timer;
   bool quit = false;
   bool redraw = true;
   int i;

   memset(&state, 0, sizeof state);
   state.requested_vsync = argc > 1 ? atoi(argv[1]) : 0;
   if (state.requested_vsync < 0 || state.requested_vsync > 2)
      state.requested_vsync = 0;

   if (!al_init())
      abort_example("Could not init Allegro.\n");
   al_init_font_addon();
   al_init_primitives_addon();
   if (!al_install_keyboard() || !al_install_mouse())
      abort_example("Could not install keyboard/mouse.\n");

   al_set_new_display_option(ALLEGRO_VSYNC, state.requested_vsync,
      ALLEGRO_SUGGEST);
   al_set_new_display_flags(ALLEGRO_RESIZABLE);
   state.display = al_create_display(960, 600);
   if (!state.display)
      abort_example("Could not create display.\n");
   state.font = al_create_builtin_font();
   if (!state.font)
      abort_example("Could not create font.\n");

   make_custom_cursor(&state);
   state.cursor_name = cursor_choices[0].name;
   al_set_system_mouse_cursor(state.display, cursor_choices[0].id);
   add_log("display created: %dx%d", al_get_display_width(state.display),
      al_get_display_height(state.display));
   add_log("effective vsync option: %d",
      al_get_display_option(state.display, ALLEGRO_VSYNC));

   queue = al_create_event_queue();
   timer = al_create_timer(1.0 / 60.0);
   if (!queue || !timer)
      abort_example("Could not create event queue/timer.\n");
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_register_event_source(queue, al_get_display_event_source(state.display));
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   al_start_timer(timer);

   while (!quit) {
      ALLEGRO_EVENT event;
      al_wait_for_event(queue, &event);

      switch (event.type) {
         case ALLEGRO_EVENT_TIMER:
            redraw = true;
            break;

         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            quit = true;
            break;

         case ALLEGRO_EVENT_DISPLAY_RESIZE:
            add_log("resize event %dx%d; acknowledging",
               event.display.width, event.display.height);
            al_acknowledge_resize(state.display);
            redraw = true;
            break;

         case ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY:
            add_log("mouse entered display");
            redraw = true;
            break;

         case ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY:
            add_log("mouse left display");
            redraw = true;
            break;

         case ALLEGRO_EVENT_KEY_DOWN:
            switch (event.keyboard.keycode) {
               case ALLEGRO_KEY_ESCAPE:
               case ALLEGRO_KEY_Q:
                  quit = true;
                  break;
               case ALLEGRO_KEY_OPENBRACE:
                  select_system_cursor(&state, -1);
                  break;
               case ALLEGRO_KEY_CLOSEBRACE:
                  select_system_cursor(&state, 1);
                  break;
               case ALLEGRO_KEY_C:
                  select_custom_cursor(&state);
                  break;
               case ALLEGRO_KEY_H:
                  state.cursor_hidden = al_hide_mouse_cursor(state.display);
                  add_log("hide cursor -> %s",
                     state.cursor_hidden ? "true" : "false");
                  break;
               case ALLEGRO_KEY_S:
                  state.cursor_hidden = !al_show_mouse_cursor(state.display);
                  add_log("show cursor -> %s",
                     state.cursor_hidden ? "false" : "true");
                  break;
               case ALLEGRO_KEY_W: {
                  bool ok = al_wait_for_vsync();
                  add_log("al_wait_for_vsync -> %s", ok ? "true" : "false");
                  break;
               }
               case ALLEGRO_KEY_F:
                  toggle_flag(&state, ALLEGRO_FULLSCREEN, "fullscreen",
                     &state.fullscreen);
                  break;
               case ALLEGRO_KEY_M:
                  toggle_flag(&state, ALLEGRO_MAXIMIZED, "maximized",
                     &state.maximized);
                  break;
               case ALLEGRO_KEY_B:
                  toggle_flag(&state, ALLEGRO_FRAMELESS, "frameless",
                     &state.frameless);
                  break;
               case ALLEGRO_KEY_R: {
                  static const int sizes[][2] = {
                     {640, 400}, {960, 600}, {1280, 720},
                  };
                  state.resize_index = (state.resize_index + 1) % 3;
                  i = state.resize_index;
                  add_log("resize %dx%d -> %s", sizes[i][0], sizes[i][1],
                     al_resize_display(state.display, sizes[i][0], sizes[i][1])
                        ? "true" : "false");
                  break;
               }
               case ALLEGRO_KEY_T: {
                  static const char *titles[] = {
                     "Wayland test bed", "Cursor surface test",
                     "EGL and Wayland probes",
                  };
                  state.title_index = (state.title_index + 1) % 3;
                  al_set_window_title(state.display,
                     titles[state.title_index]);
                  add_log("title -> %s", titles[state.title_index]);
                  break;
               }
            }
            redraw = true;
            break;
      }

      if (redraw && al_is_event_queue_empty(queue)) {
         draw_ui(&state, al_get_time());
         redraw = false;
      }
   }

   al_destroy_timer(timer);
   al_destroy_event_queue(queue);
   al_destroy_mouse_cursor(state.custom_cursor);
   al_destroy_font(state.font);
   al_destroy_display(state.display);
   return 0;
}

/* vim: set sw=3 sts=3 et: */
