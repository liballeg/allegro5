#ifdef DEBUG_X11
extern int _Xdebug; /* part of Xlib */
#endif

#include <sys/time.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xcursor.h"
#include "allegro5/internal/aintern_xembed.h"
#include "allegro5/internal/aintern_xevents.h"
#include "allegro5/internal/aintern_xfullscreen.h"
#include "allegro5/internal/aintern_xkeyboard.h"
#include "allegro5/internal/aintern_xmouse.h"
#include "allegro5/internal/aintern_xsystem.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/platform/aintxglx.h"

#include "allegro5/allegro_x.h"

#include "xicon.h"

ALLEGRO_DEBUG_CHANNEL("system")

static ALLEGRO_SYSTEM_INTERFACE *xglx_vt;

char **x11_xpm = NULL;

#ifdef ALLEGRO_XWINDOWS_WITH_XPM
#include <stdio.h>
#include "icon.xpm"

static bool x11_xpm_set;
static int x11_xpm_rows;

static char **bitmap_to_xpm(ALLEGRO_BITMAP *bitmap, int *nrows_ret)
{
   _AL_VECTOR v;
   int w, h, x, y;
   int ncolors, nrows;
   char **xpm;
   char buf[100];
   int i;

   ALLEGRO_LOCKED_REGION *lr = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READONLY);
   if (lr == NULL)
      return NULL;

   _al_vector_init(&v, sizeof(uint32_t));

   w = al_get_bitmap_width(bitmap);
   h = al_get_bitmap_height(bitmap);

   for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++) {
         uint32_t c = *(uint32_t *)((((char *)lr->data) + lr->pitch * y + x * 4));
         int alpha = (c >> 24) & 0xff;
         if (alpha != 255) {
                 c = 0;
         }
         int sz = _al_vector_size(&v);
         bool found = false;
         for (i = 0; i < sz; i++) {
            if (*((uint32_t *)_al_vector_ref(&v, i)) == c) {
               found = true;
               break;
            }
         }
         if (found == false) {
            uint32_t *p = _al_vector_alloc_back(&v);
            *p = c;
         }
      }
   }

   ncolors = _al_vector_size(&v);
   nrows = 2 + ncolors + h;

   xpm = malloc(nrows * sizeof(char *));
   if (xpm == NULL)
      return NULL;

   snprintf(buf, 100, "%d %d %d 8", w, h, ncolors + 1);

   xpm[0] = strdup(buf);

   xpm[1] = strdup("00000000\tc None");

   for (i = 0; i < ncolors; i++) {
        uint32_t c = *((uint32_t *)_al_vector_ref(&v, i));
        int r = c & 0xff;
        int g = (c >> 8) & 0xff;
        int b = (c >> 16) & 0xff;
        snprintf(buf, 100, "%08x\tc #%02x%02x%02x", i+1, r, g, b);
        xpm[i+2] = strdup(buf);
   }

   for (y = 0; y < h; y++) {
        int row = y + 2 + ncolors;
        xpm[row] = malloc(8 * w + 1);
        xpm[row][8 * w] = 0;
        uint32_t *p = (uint32_t *)(((char *)lr->data) + lr->pitch * y);
        for (x = 0; x < w; x++) {
                uint32_t pixel = *p;
                int alpha = (pixel >> 24) & 0xff;
                if (alpha != 255) {
                   snprintf(buf, 100, "%s", "00000000");
                }
                else {
                   for (i = 0; i < (int)_al_vector_size(&v); i++) {
                      uint32_t pixel2 = *((uint32_t *)_al_vector_ref(&v, i));
                      if (pixel == pixel2)
                         break;
                   }
                   snprintf(buf, 100, "%08x", i+1);
                }
                for (i = 0; i < 8; i++) {
                        xpm[row][8*x+i] = buf[i];
                }
                p++;
        }
   }

   _al_vector_free(&v);

   *nrows_ret = nrows;

   al_unlock_bitmap(bitmap);

   // debug
   /*
   for (i = 0; i < nrows; i++) {
      printf("%s\n", xpm[i]);
   }
   */

   return xpm;
}
#endif

/* Function: al_x_set_initial_icon
 */
bool al_x_set_initial_icon(ALLEGRO_BITMAP *bitmap)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XPM
   if (x11_xpm_set) {
      int i;
      for (i = 0; i < x11_xpm_rows; i++) {
         free(x11_xpm[i]);
      }
      free(x11_xpm);
      x11_xpm_set = false;
   }
   x11_xpm = bitmap_to_xpm(bitmap, &x11_xpm_rows);
   if (x11_xpm == NULL)
      return false;
   x11_xpm_set = true;
   return true;
#else
   (void)bitmap;
   return false;
#endif
}

static ALLEGRO_SYSTEM *xglx_initialize(int flags)
{
   Display *x11display;
   Display *gfxdisplay;
   ALLEGRO_SYSTEM_XGLX *s;

   (void)flags;

#ifdef DEBUG_X11
   _Xdebug = 1;
#endif

   XInitThreads();

   /* Get an X11 display handle. */
   x11display = XOpenDisplay(0);
   if (x11display) {
      /* Never ask. */
      gfxdisplay = XOpenDisplay(0);
      if (!gfxdisplay) {
         ALLEGRO_ERROR("XOpenDisplay failed second time.\n");
         XCloseDisplay(x11display);
         return NULL;
      }
   }
   else {
      ALLEGRO_INFO("XOpenDisplay failed; assuming headless mode.\n");
      gfxdisplay = NULL;
   }
   
   _al_unix_init_time();

   s = al_calloc(1, sizeof *s);

   _al_mutex_init_recursive(&s->lock);
   _al_cond_init(&s->resized);
   s->inhibit_screensaver = false;

   _al_vector_init(&s->system.displays, sizeof (ALLEGRO_DISPLAY_XGLX *));

   s->system.vt = xglx_vt;

   s->gfxdisplay = gfxdisplay;
   s->x11display = x11display;

   if (s->x11display) {
      ALLEGRO_INFO("XGLX driver connected to X11 (%s %d).\n",
         ServerVendor(s->x11display), VendorRelease(s->x11display));
      ALLEGRO_INFO("X11 protocol version %d.%d.\n",
         ProtocolVersion(s->x11display), ProtocolRevision(s->x11display));

      /* We need to put *some* atom into the ClientMessage we send for
       * faking mouse movements with al_set_mouse_xy - so let's ask X11
       * for one here.
       */
      s->AllegroAtom = XInternAtom(x11display, "AllegroAtom", False);

      /* Message type for XEmbed protocol. */
      s->XEmbedAtom = XInternAtom(x11display, "_XEMBED", False);

      _al_thread_create(&s->xevents_thread, _al_xwin_background_thread, s);
      s->have_xevents_thread = true;
      ALLEGRO_INFO("events thread spawned.\n");
   }

   const char *binding = al_get_config_value(al_get_system_config(),
         "keyboard", "toggle_mouse_grab_key");
   if (binding) {
      s->toggle_mouse_grab_keycode = _al_parse_key_binding(binding,
         &s->toggle_mouse_grab_modifiers);
      if (s->toggle_mouse_grab_keycode) {
         ALLEGRO_DEBUG("Toggle mouse grab key: '%s'\n", binding);
      }
      else {
         ALLEGRO_WARN("Cannot parse key binding '%s'\n", binding);
      }
   }

#ifdef ALLEGRO_XWINDOWS_WITH_XPM
   x11_xpm = icon_xpm;
#endif

   return &s->system;
}

static void xglx_shutdown_system(void)
{
   ALLEGRO_SYSTEM *s = al_get_system_driver();
   ALLEGRO_SYSTEM_XGLX *sx = (void *)s;

   ALLEGRO_INFO("shutting down.\n");

   if (sx->have_xevents_thread) {
      _al_thread_join(&sx->xevents_thread);
      sx->have_xevents_thread = false;
   }

   /* Close all open displays. */
   while (_al_vector_size(&s->displays) > 0) {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&s->displays, 0);
      ALLEGRO_DISPLAY *d = *dptr;
      al_destroy_display(d);
   }
   _al_vector_free(&s->displays);

   // Makes sure we wait for any commands sent to the X server when destroying the displays.
   // Should make sure we don't shutdown before modes are restored.
   if (sx->x11display) {
      XSync(sx->x11display, False);
   }

   _al_xsys_mmon_exit(sx);

   if (sx->x11display) {
      XCloseDisplay(sx->x11display);
      sx->x11display = None;
      ALLEGRO_DEBUG("xsys: close x11display.\n");
   }

   if (sx->gfxdisplay) {
      XCloseDisplay(sx->gfxdisplay);
      sx->gfxdisplay = None;
   }

   al_free(sx);
}

static ALLEGRO_DISPLAY_INTERFACE *xglx_get_display_driver(void)
{
   return _al_display_xglx_driver();
}

static ALLEGRO_KEYBOARD_DRIVER *xglx_get_keyboard_driver(void)
{
   return _al_xwin_keyboard_driver();
}

static ALLEGRO_MOUSE_DRIVER *xglx_get_mouse_driver(void)
{
   return _al_xwin_mouse_driver();
}

static ALLEGRO_JOYSTICK_DRIVER *xglx_get_joystick_driver(void)
{
   return _al_joystick_driver_list[0].driver;
}

static ALLEGRO_HAPTIC_DRIVER *xglx_get_haptic_driver(void)
{
   return _al_haptic_driver_list[0].driver;
}

static ALLEGRO_TOUCH_INPUT_DRIVER *xglx_get_touch_driver(void)
{
   return _al_touch_input_driver_list[0].driver;
}

static int xglx_get_num_video_adapters(void)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   return _al_xglx_get_num_video_adapters(system);
}

static bool xglx_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO *info)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   return _al_xglx_get_monitor_info(system, adapter, info);
}

static bool xglx_get_cursor_position(int *ret_x, int *ret_y)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   Window root = RootWindow(system->x11display, 0);
   Window child;
   int wx, wy;
   unsigned int mask;

   _al_mutex_lock(&system->lock);
   XQueryPointer(system->x11display, root, &root, &child, ret_x, ret_y,
      &wx, &wy, &mask);
   _al_mutex_unlock(&system->lock);
   return true;
}

static bool xglx_inhibit_screensaver(bool inhibit)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   system->inhibit_screensaver = inhibit;
   return true;
}

static int xglx_get_num_display_modes(void)
{
   int adapter = al_get_new_display_adapter();
   ALLEGRO_SYSTEM_XGLX *s = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();

   return _al_xglx_get_num_display_modes(s, adapter);
}

static ALLEGRO_DISPLAY_MODE *xglx_get_display_mode(int mode, ALLEGRO_DISPLAY_MODE *dm)
{
   int adapter = al_get_new_display_adapter();
   ALLEGRO_SYSTEM_XGLX *s = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();

   return _al_xglx_get_display_mode(s, adapter, mode, dm);
}

/* Internal function to get a reference to this driver. */
ALLEGRO_SYSTEM_INTERFACE *_al_system_xglx_driver(void)
{
   if (xglx_vt)
      return xglx_vt;

   xglx_vt = al_calloc(1, sizeof *xglx_vt);

   xglx_vt->initialize = xglx_initialize;
   xglx_vt->get_display_driver = xglx_get_display_driver;
   xglx_vt->get_keyboard_driver = xglx_get_keyboard_driver;
   xglx_vt->get_mouse_driver = xglx_get_mouse_driver;
   xglx_vt->get_joystick_driver = xglx_get_joystick_driver;
   xglx_vt->get_haptic_driver = xglx_get_haptic_driver;
   xglx_vt->get_touch_input_driver = xglx_get_touch_driver;
   xglx_vt->get_num_display_modes = xglx_get_num_display_modes;
   xglx_vt->get_display_mode = xglx_get_display_mode;
   xglx_vt->shutdown_system = xglx_shutdown_system;
   xglx_vt->get_num_video_adapters = xglx_get_num_video_adapters;
   xglx_vt->get_monitor_info = xglx_get_monitor_info;
   xglx_vt->create_mouse_cursor = _al_xwin_create_mouse_cursor;
   xglx_vt->destroy_mouse_cursor = _al_xwin_destroy_mouse_cursor;
   xglx_vt->get_cursor_position = xglx_get_cursor_position;
   xglx_vt->grab_mouse = _al_xwin_grab_mouse;
   xglx_vt->ungrab_mouse = _al_xwin_ungrab_mouse;
   xglx_vt->get_path = _al_unix_get_path;
   xglx_vt->inhibit_screensaver = xglx_inhibit_screensaver;

   return xglx_vt;
}


/* vim: set sts=3 sw=3 et: */
