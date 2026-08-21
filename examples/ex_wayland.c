/* Showcase for the in-progress Allegro 5 Wayland backend.
 *
 * Demonstrates what the backend currently supports:
 *   - EGL rendering via the primitives addon + al_flip_display
 *   - live window title updates (al_set_window_title)
 *   - resizable windows: compositor-driven resize + al_acknowledge_resize,
 *     and programmatic resizes via al_resize_display
 *   - keyboard events: key down/up/char, auto-repeat, keycode names, modifiers
 *   - mouse events: enter/leave, axes, buttons, wheel
 *   - adapter / monitor info (from wl_output)
 *   - display options reported from the EGL config (GL version, samples, ...)
 *
 * Keys:
 *   ESC / Q    quit
 *   Space      pause/resume the animation
 *   R          programmatic resize (960x600 <-> 1280x720)
 *   T          cycle window titles
 *   M          toggle the monitor-info panel
 *   D          toggle the display-options panel
 *   C          warp the mouse to the window center (al_set_mouse_xy)
 *   Z          fake wheel scroll: al_set_mouse_z(state.z + 100)
 *   N          cycle window-constraint presets (al_set_window_constraints)
 *   V          toggle constraints on/off (al_apply_window_constraints)
 *   + / -      ball speed
 *   F / X / B  feature probes: ALLEGRO_FULLSCREEN / ALLEGRO_MAXIMIZED /
 *              ALLEGRO_FRAMELESS via al_set_display_flag (reports the honest
 *              return value; the backend returns false until these are
 *              implemented)
 *   mouse wheel also changes ball speed
 *
 * Deliberately NOT called here: al_set_mouse_cursor / al_set_system_mouse_cursor
 * / al_show_mouse_cursor / al_hide_mouse_cursor / al_create_mouse_cursor /
 * al_get_num_display_modes / al_get_display_mode -- those vtable slots are
 * still NULL in the Wayland backend and the core wrappers ASSERT or
 * dereference them (crash), so they cannot be exercised yet.
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_primitives.h"

#include "common.c"

#define MAX_LOG 14
#define LOG_LEN 160

static char log_lines[MAX_LOG][LOG_LEN];

static void add_log(char const *fmt, ...)
{
   va_list args;
   memmove(log_lines[1], log_lines[0], (MAX_LOG - 1) * LOG_LEN);
   va_start(args, fmt);
   vsnprintf(log_lines[0], LOG_LEN, fmt, args);
   va_end(args);
}

static const char *mods_str(unsigned int m)
{
   static char buf[80];
   const char *sep = "";
   buf[0] = '\0';
#define ADD_MOD(bit, name) \
   if (m & (bit)) { strncat(buf, sep, sizeof buf - strlen(buf) - 1); \
                    strncat(buf, name, sizeof buf - strlen(buf) - 1); \
                    sep = "+"; }
   ADD_MOD(ALLEGRO_KEYMOD_SHIFT, "shift");
   ADD_MOD(ALLEGRO_KEYMOD_CTRL, "ctrl");
   ADD_MOD(ALLEGRO_KEYMOD_ALT, "alt");
   ADD_MOD(ALLEGRO_KEYMOD_ALTGR, "altgr");
   ADD_MOD(ALLEGRO_KEYMOD_LWIN, "lwin");
   ADD_MOD(ALLEGRO_KEYMOD_RWIN, "rwin");
   ADD_MOD(ALLEGRO_KEYMOD_CAPSLOCK, "caps");
   ADD_MOD(ALLEGRO_KEYMOD_NUMLOCK, "num");
#undef ADD_MOD
   if (!buf[0])
      strcpy(buf, "-");
   return buf;
}

typedef struct Demo Demo;
struct Demo {
   ALLEGRO_DISPLAY *display;
   ALLEGRO_FONT *font;
   bool paused;
   bool show_monitors;
   bool show_options;
   bool constraints_on;
   int title_mode;      /* 0 = auto (fps/size), else custom title index */
   int fps;
   float speed;         /* ball speed, px/s */
   float bx, by, bvx, bvy;  /* ball position and velocity (px/s) */
   double last_tick;
   int target_w, target_h;  /* target size for the R key toggle */
};

static const char *custom_titles[] = {
   NULL, /* auto */
   "Hello from libdecor!",
   "Allegro 5 on Wayland",
   "Drag the window edges to resize",
};
#define NUM_TITLES (int)(sizeof custom_titles / sizeof custom_titles[0])

static void set_auto_title(Demo *d)
{
   char buf[128];
   snprintf(buf, sizeof buf, "Wayland demo - %dx%d @ %d fps",
      al_get_display_width(d->display),
      al_get_display_height(d->display), d->fps);
   al_set_window_title(d->display, buf);
}

static void update_scene(Demo *d, double now)
{
   double dt = now - d->last_tick;
   d->last_tick = now;
   if (dt > 0.05)
      dt = 0.05; /* clamp after stalls */

   if (d->paused)
      return;

   int w = al_get_display_width(d->display);
   int h = al_get_display_height(d->display);

   d->bx += d->bvx * d->speed * dt;
   d->by += d->bvy * d->speed * dt;

   if (d->bx < 24) { d->bx = 24; d->bvx = -d->bvx; }
   if (d->bx > w - 24) { d->bx = w - 24; d->bvx = -d->bvx; }
   if (d->by < 24) { d->by = 24; d->bvy = -d->bvy; }
   if (d->by > h - 24) { d->by = h - 24; d->bvy = -d->bvy; }
}

static void draw_scene(Demo *d, double now)
{
   ALLEGRO_DISPLAY *display = d->display;
   ALLEGRO_FONT *font = d->font;
   int w = al_get_display_width(display);
   int h = al_get_display_height(display);
   int x, y;
   int i;

   ALLEGRO_COLOR bg = al_map_rgb_f(0.10, 0.12, 0.16);
   ALLEGRO_COLOR panel = al_map_rgba_f(0.0, 0.0, 0.0, 0.55);
   ALLEGRO_COLOR grid = al_map_rgba_f(1, 1, 1, 0.05);
   ALLEGRO_COLOR white = al_map_rgb_f(0.92, 0.95, 1.0);
   ALLEGRO_COLOR dim = al_map_rgb_f(0.55, 0.6, 0.7);
   ALLEGRO_COLOR orange = al_map_rgb_f(1.0, 0.6, 0.1);
   ALLEGRO_COLOR cyan = al_map_rgb_f(0.2, 0.9, 1.0);

   al_clear_to_color(bg);

   /* Background grid. */
   for (x = 0; x < w; x += 40)
      al_draw_line(x + 0.5, 0, x + 0.5, h, grid, 1.0);
   for (y = 0; y < h; y += 40)
      al_draw_line(0, y + 0.5, w, y + 0.5, grid, 1.0);

   /* Sweeping scan line. */
   float sx = fmod(now * 90.0, (double)w + 80) - 40;
   al_draw_filled_rectangle(sx, 0, sx + 6, h,
      al_map_rgba_f(0.2, 0.9, 1.0, 0.06));

   /* Bouncing ball with a pulsing ring. */
   float pulse = 6 + 4 * sin(now * 4.0);
   al_draw_circle(d->bx, d->by, 24 + pulse, cyan, 1.5);
   al_draw_filled_circle(d->bx, d->by, 20, orange);
   al_draw_filled_circle(d->bx - 7, d->by - 7, 4.5,
      al_map_rgba_f(1, 1, 1, 0.7));

   /* A rotating diamond (cheap rotation via the square formula). */
   float a = now * 1.2;
   float r = 30;
   float cxs[4], cys[4];
   for (i = 0; i < 4; i++) {
      float ang = a + i * ALLEGRO_PI / 2;
      cxs[i] = w - 90 + cosf(ang) * r;
      cys[i] = h - 90 + sinf(ang) * r;
   }
   al_draw_filled_triangle(cxs[0], cys[0], cxs[1], cys[1],
      cxs[2], cys[2], al_map_rgba_f(0.4, 0.8, 0.4, 0.8));
   al_draw_filled_triangle(cxs[0], cys[0], cxs[2], cys[2],
      cxs[3], cys[3], al_map_rgba_f(0.4, 0.8, 0.4, 0.8));

   /* --- Header panel --- */
   al_draw_filled_rounded_rectangle(8, 8, w - 8, 44, 6, 6, panel);
   al_draw_textf(font, white, 18, 14, ALLEGRO_ALIGN_LEFT,
      "Allegro 5 Wayland backend demo");
   al_draw_filled_rectangle(18, 28, 18 + 10 * 22, 30, cyan);
   al_draw_textf(font, dim, 320, 22, ALLEGRO_ALIGN_LEFT,
      "ESC quit | R resize | T title | N constraints | V on/off | "
      "M monitors | D options | F/X/B probes | C warp | Z wheel");

   /* --- Stats panel (top right) --- */
   ALLEGRO_MOUSE_STATE ms;
   al_get_mouse_state(&ms);
   al_draw_filled_rounded_rectangle(w - 260, 54, w - 8, 158, 6, 6, panel);
   al_draw_textf(font, white, w - 250, 60, 0, "fps %d    %dx%d", d->fps, w, h);
   al_draw_textf(font, dim, w - 250, 76, 0, "ball speed %d px/s%s",
      (int)d->speed, d->paused ? "  (paused)" : "");
   al_draw_textf(font, dim, w - 250, 92, 0, "mouse %d,%d  buttons 0x%x",
      ms.x, ms.y, ms.buttons);
   al_draw_textf(font, dim, w - 250, 108, 0, "wheel z=%d w=%d  adapters %d",
      ms.z, ms.w, al_get_num_video_adapters());
   al_draw_textf(font, cyan, w - 250, 124, 0,
      "wheel / +/- change ball speed");
   {
      int mnw, mnh, mxw, mxh;
      if (al_get_window_constraints(d->display, &mnw, &mnh, &mxw, &mxh)) {
         al_draw_textf(font, dim, w - 250, 140, 0,
            "constraints min %dx%d max %dx%d %s",
            mnw, mnh, mxw, mxh, d->constraints_on ? "(on)" : "(off)");
      }
   }

   /* --- Event log (left column) --- */
   al_draw_filled_rounded_rectangle(8, 54, 420, 54 + MAX_LOG * 15 + 10,
      6, 6, panel);
   al_draw_text(font, dim, 18, 60, 0, "Event log (newest on top)");
   for (i = 0; i < MAX_LOG; i++) {
      if (!log_lines[i][0])
         continue;
      al_draw_textf(font, i == 0 ? white : dim, 18, 78 + i * 15, 0,
         "%s", log_lines[i]);
   }

   /* --- Monitor info panel --- */
   if (d->show_monitors) {
      al_draw_filled_rounded_rectangle(8, 54 + MAX_LOG * 15 + 20,
         420, 54 + MAX_LOG * 15 + 20 + 40 + 16 * 4, 6, 6, panel);
      al_draw_text(font, cyan, 18, 54 + MAX_LOG * 15 + 24, 0,
         "Video adapters (wl_output)");
      int n = al_get_num_video_adapters();
      al_draw_textf(font, dim, 18, 54 + MAX_LOG * 15 + 40, 0,
         "total: %d", n);
      for (i = 0; i < n && i < 3; i++) {
         ALLEGRO_MONITOR_INFO mi;
         if (al_get_monitor_info(i, &mi)) {
            al_draw_textf(font, dim, 18, 54 + MAX_LOG * 15 + 56 + 16 * i, 0,
               "adapter %d: %dx%d at (%d,%d)", i,
               mi.x2 - mi.x1, mi.y2 - mi.y1, mi.x1, mi.y1);
         }
      }
   }

   /* --- Display options panel --- */
   if (d->show_options) {
      al_draw_filled_rounded_rectangle(8, 54 + MAX_LOG * 15 + 20,
         420, 54 + MAX_LOG * 15 + 20 + 40 + 16 * 4, 6, 6, panel);
      al_draw_text(font, cyan, 18, 54 + MAX_LOG * 15 + 24, 0,
         "Display options (EGL config)");
      al_draw_textf(font, dim, 18, 54 + MAX_LOG * 15 + 40, 0,
         "GL %d.%d  samples %d  depth %d  stencil %d  vsync %d",
         al_get_display_option(display, ALLEGRO_OPENGL_MAJOR_VERSION),
         al_get_display_option(display, ALLEGRO_OPENGL_MINOR_VERSION),
         al_get_display_option(display, ALLEGRO_SAMPLES),
         al_get_display_option(display, ALLEGRO_DEPTH_SIZE),
         al_get_display_option(display, ALLEGRO_STENCIL_SIZE),
         al_get_display_option(display, ALLEGRO_VSYNC));
      al_draw_textf(font, dim, 18, 54 + MAX_LOG * 15 + 56, 0,
         "compatible %d  double buffer %s",
         al_get_display_option(display, ALLEGRO_COMPATIBLE_DISPLAY),
         al_get_display_option(display, ALLEGRO_SINGLE_BUFFER) ? "no" : "yes");
   }

   al_flip_display();
}

/* Probe a window state flag and report the honest result. */
static void probe_flag(Demo *d, int flag, const char *name, bool *state)
{
   bool on = !*state;
   bool ok = al_set_display_flag(d->display, flag, on);
   add_log("%s %s: al_set_display_flag(%s, %d) -> %s",
      ok ? "OK  " : "----", name, name, on,
      ok ? "true" : "false (not implemented yet)");
   if (ok)
      *state = on;
}

int main(int argc, char **argv)
{
   (void)argc;
   (void)argv;

   al_set_config_value(al_get_system_config(), "trace", "level", "debug");

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_init_font_addon();
   al_init_primitives_addon();
   al_install_keyboard();
   al_install_mouse();

   al_set_new_display_flags(ALLEGRO_RESIZABLE);
   Demo d;
   memset(&d, 0, sizeof d);

   d.display = al_create_display(960, 600);
   if (!d.display) {
      abort_example("Error creating display.\n");
   }
   d.font = al_create_builtin_font();
   if (!d.font) {
      abort_example("Error creating builtin font.\n");
   }

   d.paused = false;
   d.speed = 120.0f;
   d.bx = 200;
   d.by = 300;
   d.bvx = 1.0f;
   d.bvy = 0.8f;
   d.last_tick = al_get_time();
   d.target_w = 1280;
   d.target_h = 720;

   al_set_window_title(d.display, "Allegro 5 Wayland demo");
   add_log("display created 960x600 (resizable)");

   ALLEGRO_TIMER *timer = al_create_timer(1.0 / 60.0);
   ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_register_event_source(queue, al_get_display_event_source(d.display));
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   al_start_timer(timer);

   bool redraw = true;
   bool quit = false;
   int frames = 0;
   double fps_time = al_get_time();
   bool maximized = false;
   bool frameless = false;
   bool fullscreen = false;

   while (!quit) {
      ALLEGRO_EVENT event;
      al_wait_for_event(queue, &event);

      switch (event.type) {
         case ALLEGRO_EVENT_TIMER:
            redraw = true;
            break;

         case ALLEGRO_EVENT_DISPLAY_RESIZE:
            add_log("DISPLAY_RESIZE %dx%d (acknowledging)",
               event.display.width, event.display.height);
            al_acknowledge_resize(d.display);
            redraw = true;
            break;

         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            add_log("DISPLAY_CLOSE - quitting");
            quit = true;
            break;

         case ALLEGRO_EVENT_KEY_DOWN:
            add_log("KEY_DOWN %s (code %d) [%s]",
               al_keycode_to_name(event.keyboard.keycode),
               event.keyboard.keycode,
               mods_str(event.keyboard.modifiers));
            switch (event.keyboard.keycode) {
               case ALLEGRO_KEY_ESCAPE:
               case ALLEGRO_KEY_Q:
                  quit = true;
                  break;
               case ALLEGRO_KEY_SPACE:
                  d.paused = !d.paused;
                  add_log("animation %s", d.paused ? "paused" : "resumed");
                  break;
               case ALLEGRO_KEY_R: {
                  int nw = al_get_display_width(d.display) == d.target_w
                     ? 960 : d.target_w;
                  int nh = nw == 960 ? 600 : d.target_h;
                  bool ok = al_resize_display(d.display, nw, nh);
                  add_log("al_resize_display(%dx%d) -> %s", nw, nh,
                     ok ? "true" : "false");
                  break;
               }
               case ALLEGRO_KEY_T:
                  d.title_mode = (d.title_mode + 1) % NUM_TITLES;
                  if (custom_titles[d.title_mode]) {
                     al_set_window_title(d.display, custom_titles[d.title_mode]);
                     add_log("set_window_title -> \"%s\"",
                        custom_titles[d.title_mode]);
                  }
                  else {
                     set_auto_title(&d);
                     add_log("set_window_title -> auto (fps/size)");
                  }
                  break;
               case ALLEGRO_KEY_Z: {
                  ALLEGRO_MOUSE_STATE ms;
                  al_get_mouse_state(&ms);
                  bool ok = al_set_mouse_z(ms.z + 100);
                  add_log("al_set_mouse_z(%d) -> %s", ms.z + 100,
                     ok ? "true" : "false");
                  break;
               }
               case ALLEGRO_KEY_C: {
                  int cx = al_get_display_width(d.display) / 2;
                  int cy = al_get_display_height(d.display) / 2;
                  bool ok = al_set_mouse_xy(d.display, cx, cy);
                  add_log("al_set_mouse_xy(%d,%d) -> %s", cx, cy,
                     ok ? "true" : "false");
                  break;
               }
               case ALLEGRO_KEY_M:
                  d.show_monitors = !d.show_monitors;
                  add_log("monitor panel %s", d.show_monitors ? "on" : "off");
                  break;
               case ALLEGRO_KEY_D:
                  d.show_options = !d.show_options;
                  add_log("options panel %s", d.show_options ? "on" : "off");
                  break;
               case ALLEGRO_KEY_EQUALS:
               case ALLEGRO_KEY_PAD_PLUS:
                  d.speed += 20;
                  add_log("speed %d px/s", (int)d.speed);
                  break;
               case ALLEGRO_KEY_MINUS:
               case ALLEGRO_KEY_PAD_MINUS:
                  d.speed -= 20;
                  if (d.speed < 20)
                     d.speed = 20;
                  add_log("speed %d px/s", (int)d.speed);
                  break;
               case ALLEGRO_KEY_F:
                  probe_flag(&d, ALLEGRO_FULLSCREEN, "FULLSCREEN",
                     &fullscreen);
                  break;
               case ALLEGRO_KEY_X:
                  probe_flag(&d, ALLEGRO_MAXIMIZED, "MAXIMIZED", &maximized);
                  break;
               case ALLEGRO_KEY_B:
                  probe_flag(&d, ALLEGRO_FRAMELESS, "FRAMELESS", &frameless);
                  break;
               case ALLEGRO_KEY_N: {
                  /* Presets for window constraints; 0 = unconstrained.
                   * al_set_window_constraints stores them, then
                   * al_apply_window_constraints pushes them to the
                   * compositor and re-resizes (drag the window edges to
                   * feel the clamp). */
                  static const struct { int mnw, mnh, mxw, mxh; } presets[] = {
                     {0, 0, 0, 0},        /* none */
                     {320, 240, 0, 0},    /* min only */
                     {320, 240, 800, 600},
                     {480, 360, 1024, 768},
                  };
                  static int preset = 0;
                  preset = (preset + 1) %
                     (int)(sizeof presets / sizeof presets[0]);
                  bool ok = al_set_window_constraints(d.display,
                     presets[preset].mnw, presets[preset].mnh,
                     presets[preset].mxw, presets[preset].mxh);
                  add_log("constraints preset %d: min %dx%d max %dx%d -> %s",
                     preset, presets[preset].mnw, presets[preset].mnh,
                     presets[preset].mxw, presets[preset].mxh,
                     ok ? "true" : "false");
                  if (ok) {
                     al_apply_window_constraints(d.display, true);
                     d.constraints_on = true;
                     add_log("constraints applied (drag the edges)");
                  }
                  break;
               }
               case ALLEGRO_KEY_V:
                  d.constraints_on = !d.constraints_on;
                  al_apply_window_constraints(d.display, d.constraints_on);
                  add_log("constraints %s",
                     d.constraints_on ? "applied" : "released");
                  break;
            }
            redraw = true;
            break;

         case ALLEGRO_EVENT_KEY_UP:
            add_log("KEY_UP   %s (code %d)",
               al_keycode_to_name(event.keyboard.keycode),
               event.keyboard.keycode);
            redraw = true;
            break;

         case ALLEGRO_EVENT_KEY_CHAR: {
            unsigned int uc = event.keyboard.unichar;
            char printable = (uc >= 32 && uc < 127) ? (char)uc : '.';
            add_log("KEY_CHAR '%c' (u+%04x) repeat=%d", printable, uc,
               event.keyboard.repeat);
            redraw = true;
            break;
         }

         case ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY:
            add_log("MOUSE_ENTER_DISPLAY at %d,%d",
               event.mouse.x, event.mouse.y);
            redraw = true;
            break;

         case ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY:
            add_log("MOUSE_LEAVE_DISPLAY");
            redraw = true;
            break;

         case ALLEGRO_EVENT_MOUSE_AXES:
            if (event.mouse.dz) {
               d.speed += event.mouse.dz * 10;
               if (d.speed < 20)
                  d.speed = 20;
               if (d.speed > 1500)
                  d.speed = 1500;
            }
            add_log("MOUSE_AXES %d,%d  d(%d,%d,%d,%d)",
               event.mouse.x, event.mouse.y,
               event.mouse.dx, event.mouse.dy,
               event.mouse.dz, event.mouse.dw);
            redraw = true;
            break;

         case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            add_log("MOUSE_BUTTON_DOWN button %d", event.mouse.button);
            redraw = true;
            break;

         case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            add_log("MOUSE_BUTTON_UP   button %d", event.mouse.button);
            redraw = true;
            break;
      }

      if (redraw && al_is_event_queue_empty(queue)) {
         double now = al_get_time();
         update_scene(&d, now);

         /* FPS + auto title, updated twice a second. */
         frames++;
         if (now - fps_time >= 0.5) {
            d.fps = (int)(frames / (now - fps_time));
            frames = 0;
            fps_time = now;
            if (d.title_mode == 0)
               set_auto_title(&d);
         }

         draw_scene(&d, now);
         redraw = false;
      }
   }

   add_log("shutting down");
   al_destroy_timer(timer);
   al_destroy_event_queue(queue);
   al_destroy_font(d.font);
   al_destroy_display(d.display);

   return 0;
}

/* vim: set sw=3 sts=3 et: */
