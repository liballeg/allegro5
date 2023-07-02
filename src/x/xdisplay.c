#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xcursor.h"
#include "allegro5/internal/aintern_xclipboard.h"
#include "allegro5/internal/aintern_xdisplay.h"
#include "allegro5/internal/aintern_xfullscreen.h"
#include "allegro5/internal/aintern_xglx_config.h"
#include "allegro5/internal/aintern_xsystem.h"
#include "allegro5/internal/aintern_xtouch.h"
#include "allegro5/internal/aintern_xwindow.h"
#include "allegro5/internal/aintern_xdnd.h"
#include "allegro5/platform/aintxglx.h"

#include <X11/Xatom.h>
#ifdef ALLEGRO_XWINDOWS_WITH_XINPUT2
#include <X11/extensions/XInput2.h>
#endif

#include "xicon.h"
#include "icon.inc"

ALLEGRO_DEBUG_CHANNEL("display")

static ALLEGRO_DISPLAY_INTERFACE xdpy_vt;
static const ALLEGRO_XWIN_DISPLAY_OVERRIDABLE_INTERFACE default_overridable_vt;
static const ALLEGRO_XWIN_DISPLAY_OVERRIDABLE_INTERFACE *gtk_override_vt = NULL;

static void xdpy_destroy_display(ALLEGRO_DISPLAY *d);
static bool xdpy_acknowledge_resize(ALLEGRO_DISPLAY *d);


/* XXX where does this belong? */
static void _al_xglx_use_adapter(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   ALLEGRO_DEBUG("use adapter %i\n", adapter);
   s->adapter_use_count++;
   s->adapter_map[adapter]++;
}


static void _al_xglx_unuse_adapter(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   ALLEGRO_DEBUG("unuse adapter %i\n", adapter);
   s->adapter_use_count--;
   s->adapter_map[adapter]--;
}


static bool check_adapter_use_count(ALLEGRO_SYSTEM_XGLX *system)
{
   /* If we're in multi-head X mode, bail out if we try to use more than one
    * display as there are bugs in X/glX that cause us to hang in X if we try
    * to use more than one.
    * If we're in real xinerama mode, also bail out, X makes mouse use evil.
    */
   bool true_xinerama_active = false;
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   bool xrandr_active = false;
#ifdef ALLEGRO_XWINDOWS_WITH_XRANDR
   xrandr_active = system->xrandr_available;
#endif
   true_xinerama_active = !xrandr_active && system->xinerama_available;
#endif

   if ((true_xinerama_active || ScreenCount(system->x11display) > 1)
         && system->adapter_use_count > 0)
   {
      int i, adapter_use_count = 0;

      /* XXX magic constant */
      for (i = 0; i < 32; i++) {
         if (system->adapter_map[i])
            adapter_use_count++;
      }

      if (adapter_use_count > 1) {
         ALLEGRO_ERROR("Use of more than one adapter at once in "
            "multi-head X or X with true Xinerama active is not possible.\n");
         return false;
      }
   }

   return true;
}


static int query_glx_version(ALLEGRO_SYSTEM_XGLX *system)
{
   int major, minor;
   int version;

   glXQueryVersion(system->x11display, &major, &minor);
   version = major * 100 + minor * 10;
   ALLEGRO_INFO("GLX %.1f.\n", version / 100.f);
   return version;
}


static int xdpy_swap_control(ALLEGRO_DISPLAY *display, int vsync_setting)
{
   /* We set the swap interval to 0 if vsync is forced off, and to 1
    * if it is forced on.
    * http://www.opengl.org/registry/specs/SGI/swap_control.txt
    * If the option is set to 0, we simply use the system default. The
    * above extension specifies vsync on as default though, so in the
    * end with GLX we can't force vsync on, just off.
    */
   ALLEGRO_DEBUG("requested vsync=%d.\n", vsync_setting);

   if (vsync_setting) {
      if (display->ogl_extras->extension_list->ALLEGRO_GLX_SGI_swap_control) {
         int x = (vsync_setting == 2) ? 0 : 1;
         if (glXSwapIntervalSGI(x)) {
            ALLEGRO_WARN("glXSwapIntervalSGI(%d) failed.\n", x);
         }
      }
      else {
         ALLEGRO_WARN("no vsync, GLX_SGI_swap_control missing.\n");
         /* According to the specification that means it's on, but
          * the driver might have disabled it. So we do not know.
          */
         vsync_setting = 0;
      }
   }

   return vsync_setting;
}

static bool should_bypass_compositor(int flags)
{
   const char* value = al_get_config_value(al_get_system_config(), "x11", "bypass_compositor");
   if (value && strcmp(value, "always") == 0) {
      return true;
   }
   if (value && strcmp(value, "never") == 0) {
      return false;
   }
   // default to "fullscreen_only"
   return (flags & ALLEGRO_FULLSCREEN) || (flags & ALLEGRO_FULLSCREEN_WINDOW);
}

static void set_compositor_bypass_flag(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   const long _NET_WM_BYPASS_COMPOSITOR_HINT_ON = should_bypass_compositor(display->flags);
   Atom _NET_WM_BYPASS_COMPOSITOR;

   _NET_WM_BYPASS_COMPOSITOR = XInternAtom(system->x11display,
                                           "_NET_WM_BYPASS_COMPOSITOR",
                                           False);
   XChangeProperty(system->x11display, glx->window, _NET_WM_BYPASS_COMPOSITOR,
                   XA_CARDINAL, 32, PropModeReplace,
                   (unsigned char *)&_NET_WM_BYPASS_COMPOSITOR_HINT_ON, 1);
}


static bool xdpy_create_display_window(ALLEGRO_SYSTEM_XGLX *system,
   ALLEGRO_DISPLAY_XGLX *d, int w, int h, int adapter)
{
   ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY *)d;

   /* Create a colormap. */
   Colormap cmap = XCreateColormap(system->x11display,
      RootWindow(system->x11display, d->xvinfo->screen),
      d->xvinfo->visual, AllocNone);

   /* Create an X11 window */
   XSetWindowAttributes swa;
   int mask = CWBorderPixel | CWColormap | CWEventMask;
   swa.colormap = cmap;
   swa.border_pixel = 0;
   swa.event_mask =
      KeyPressMask |
      KeyReleaseMask |
      StructureNotifyMask |
      EnterWindowMask |
      LeaveWindowMask |
      ExposureMask |
      PropertyChangeMask |
      ButtonPressMask |
      ButtonReleaseMask |
      PointerMotionMask;

   /* We handle these events via GTK-sent XEmbed events. */
   if (!(display->flags & ALLEGRO_GTK_TOPLEVEL_INTERNAL)) {
      swa.event_mask |= FocusChangeMask;
   }

   /* For a non-compositing window manager, a black background can look
    * less broken if the application doesn't react to expose events fast
    * enough. However in some cases like resizing, the black background
    * causes horrible flicker.
    */
   if (!(display->flags & ALLEGRO_RESIZABLE)) {
      mask |= CWBackPixel;
      swa.background_pixel = BlackPixel(system->x11display, d->xvinfo->screen);
   }

   int x_off = INT_MAX;
   int y_off = INT_MAX;
   if (display->flags & ALLEGRO_FULLSCREEN) {
      _al_xglx_get_display_offset(system, d->adapter, &x_off, &y_off);
   }
   else {
      /* We want new_display_adapter's offset to add to the
       * new_window_position.
       */
      int xscr_x = 0;
      int xscr_y = 0;
      al_get_new_window_position(&x_off, &y_off);

      if (adapter >= 0) {
         /* Non default adapter. I'm assuming this means the user wants the
          * window to be placed on the adapter offset by new display pos.
          */
         _al_xglx_get_display_offset(system, d->adapter, &xscr_x, &xscr_y);
         if (x_off != INT_MAX)
            x_off += xscr_x;
         if (y_off != INT_MAX)
            y_off += xscr_y;
      }
   }

   d->window = XCreateWindow(system->x11display,
      RootWindow(system->x11display, d->xvinfo->screen),
      x_off != INT_MAX ? x_off : 0,
      y_off != INT_MAX ? y_off : 0,
      w, h, 0, d->xvinfo->depth,
      InputOutput, d->xvinfo->visual, mask, &swa);

   ALLEGRO_DEBUG("Window ID: %ld\n", (long)d->window);

   /* Try to set full screen mode if requested, fail if we can't. */
   if (display->flags & ALLEGRO_FULLSCREEN) {
      /* According to the spec, the window manager is supposed to disable
       * window decorations when _NET_WM_STATE_FULLSCREEN is in effect.
       * However, some WMs may not be fully compliant, e.g. Fluxbox.
       */
      _al_xwin_set_frame(display, false);
      _al_xwin_set_above(display, 1);
      if (!_al_xglx_fullscreen_set_mode(system, d, w, h, 0,
            display->refresh_rate)) {
         ALLEGRO_DEBUG("xdpy: failed to set fullscreen mode.\n");
         return false;
      }
   }

   if (display->flags & ALLEGRO_FRAMELESS) {
      _al_xwin_set_frame(display, false);
   }

   if (display->flags & ALLEGRO_DRAG_AND_DROP) {
      _al_xwin_accept_drag_and_drop(display, true);
   }

   ALLEGRO_DEBUG("X11 window created.\n");

   /* Set the PID related to the window. */
   Atom _NET_WM_PID = XInternAtom(system->x11display, "_NET_WM_PID", False);
   int pid = getpid();
   XChangeProperty(system->x11display, d->window, _NET_WM_PID, XA_CARDINAL,
                   32, PropModeReplace, (unsigned char *)&pid, 1);

   _al_xwin_set_size_hints(display, x_off, y_off);

   /* Let the window manager know we're a "normal" window */
   Atom _NET_WM_WINDOW_TYPE;
   Atom _NET_WM_WINDOW_TYPE_NORMAL;

   _NET_WM_WINDOW_TYPE = XInternAtom(system->x11display, "_NET_WM_WINDOW_TYPE",
                                     False);
   _NET_WM_WINDOW_TYPE_NORMAL = XInternAtom(system->x11display,
                                            "_NET_WM_WINDOW_TYPE_NORMAL",
                                            False);
   XChangeProperty(system->x11display, d->window, _NET_WM_WINDOW_TYPE, XA_ATOM,
                   32, PropModeReplace,
                   (unsigned char *)&_NET_WM_WINDOW_TYPE_NORMAL, 1);

   /* This seems like a good idea */
   set_compositor_bypass_flag(display);

#ifdef ALLEGRO_XWINDOWS_WITH_XINPUT2
   /* listen for touchscreen events */
   XIEventMask event_mask;
   event_mask.deviceid = XIAllMasterDevices;
   event_mask.mask_len = XIMaskLen(XI_TouchEnd);
   event_mask.mask = (unsigned char*)al_calloc(3, sizeof(char));
   XISetMask(event_mask.mask, XI_TouchBegin);
   XISetMask(event_mask.mask, XI_TouchUpdate);
   XISetMask(event_mask.mask, XI_TouchEnd);

   XISelectEvents(system->x11display, d->window, &event_mask, 1);

   al_free(event_mask.mask);
#endif

   return true;
}


static ALLEGRO_DISPLAY_XGLX *xdpy_create_display_locked(
   ALLEGRO_SYSTEM_XGLX *system, int flags, int w, int h, int adapter)
{
   ALLEGRO_DISPLAY_XGLX *d = al_calloc(1, sizeof *d);
   ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY *)d;
   ALLEGRO_OGL_EXTRAS *ogl = al_calloc(1, sizeof *ogl);
   display->ogl_extras = ogl;

   d->glx_version = query_glx_version(system);

   display->w = w;
   display->h = h;
   display->vt = _al_display_xglx_driver();
   display->refresh_rate = al_get_new_display_refresh_rate();
   display->flags = flags;
   // FIXME: default? Is this the right place to set this?
   display->flags |= ALLEGRO_OPENGL;
#ifdef ALLEGRO_CFG_OPENGLES2
   display->flags |= ALLEGRO_PROGRAMMABLE_PIPELINE;
#endif
#ifdef ALLEGRO_CFG_OPENGLES
   display->flags |= ALLEGRO_OPENGL_ES_PROFILE;
#endif

   /* Store our initial virtual adapter, used by fullscreen and positioning
    * code.
    */
   ALLEGRO_DEBUG("selected adapter %i\n", adapter);
   if (adapter < 0)
      d->adapter = _al_xglx_get_default_adapter(system);
   else
      d->adapter = adapter;

   ALLEGRO_DEBUG("xdpy: selected adapter %i\n", d->adapter);
   _al_xglx_use_adapter(system, d->adapter);
   if (!check_adapter_use_count(system)) {
      goto EarlyError;
   }

   /* Store our initial X Screen, used by window creation, fullscreen, and glx
    * visual code.
    */
   d->xscreen = _al_xglx_get_xscreen(system, d->adapter);
   ALLEGRO_DEBUG("xdpy: selected xscreen %i\n", d->xscreen);

   d->wm_delete_window_atom = None;

   d->is_mapped = false;
   _al_cond_init(&d->mapped);
   
   d->is_selectioned = false;
   _al_cond_init(&d->selectioned);


   d->resize_count = 0;
   d->programmatic_resize = false;

   _al_xglx_config_select_visual(d);

   if (!d->xvinfo) {
      ALLEGRO_ERROR("FIXME: Need better visual selection.\n");
      ALLEGRO_ERROR("No matching visual found.\n");
      goto EarlyError;
   }

   ALLEGRO_INFO("Selected X11 visual %lu.\n", d->xvinfo->visualid);

   /* Add ourself to the list of displays. */
   ALLEGRO_DISPLAY_XGLX **add;
   add = _al_vector_alloc_back(&system->system.displays);
   *add = d;

   /* Each display is an event source. */
   _al_event_source_init(&display->es);

   if (!xdpy_create_display_window(system, d, w, h, adapter)) {
      goto LateError;
   }

   /* Send any pending requests to the X server.
    * This is necessary to make the window ID immediately valid
    * for a GtkSocket.
    */
   XSync(system->x11display, False);

   if (display->flags & ALLEGRO_GTK_TOPLEVEL_INTERNAL) {
      ASSERT(gtk_override_vt);
      if (!gtk_override_vt->create_display_hook(display, w, h)) {
         goto LateError;
      }
   }
   else {
      default_overridable_vt.set_window_title(display, al_get_new_window_title());
      if (!default_overridable_vt.create_display_hook(display, w, h)) {
         goto LateError;
      }
   }

   /* overridable_vt should be set by the create_display_hook. */
   ASSERT(d->overridable_vt);

   /* Send any pending requests to the X server. */
   XSync(system->x11display, False);

   /* To avoid race conditions where some X11 functions fail before the window
    * is mapped, we wait here until it is mapped. Note that the thread is
    * locked, so the event could not possibly have been processed yet in the
    * events thread. So as long as no other map events occur, the condition
    * should only be signalled when our window gets mapped.
    */
   while (!d->is_mapped) {
      _al_cond_wait(&d->mapped, &system->lock);
   }
   /* In tiling WMs, we might get resize events pretty much immediately after
    * Window creation. This location seems to catch them reliably, tested with
    * dwm, awesome, xmonad and i3. */
   if ((display->flags & ALLEGRO_RESIZABLE) && d->resize_count > 0) {
      xdpy_acknowledge_resize(display);
   }

   /* We can do this at any time, but if we already have a mapped
    * window when switching to fullscreen it will use the same
    * monitor (with the MetaCity version I'm using here right now).
    */
   if ((display->flags & ALLEGRO_FULLSCREEN_WINDOW)) {
      ALLEGRO_INFO("Toggling fullscreen flag for %d x %d window.\n",
         display->w, display->h);
      _al_xwin_reset_size_hints(display);
      _al_xwin_set_fullscreen_window(display, 2);
      _al_xwin_set_size_hints(display, INT_MAX, INT_MAX);

      XWindowAttributes xwa;
      XGetWindowAttributes(system->x11display, d->window, &xwa);
      display->w = xwa.width;
      display->h = xwa.height;
      ALLEGRO_INFO("Using ALLEGRO_FULLSCREEN_WINDOW of %d x %d\n",
         display->w, display->h);
   }

   if (display->flags & ALLEGRO_FULLSCREEN) {
      /* kwin wants these here */
      /* metacity wants these here too */
      /* XXX compiz is quiky, can't seem to find a combination of hints that
       * make sure we are layerd over panels, and are positioned properly */

      //_al_xwin_set_fullscreen_window(display, 1);
      _al_xwin_set_above(display, 1);

      _al_xglx_fullscreen_to_display(system, d);

      /* Grab mouse if we only have one display, ungrab it if we have more than
       * one.
       */
      if (_al_vector_size(&system->system.displays) == 1) {
         al_grab_mouse(display);
      }
      else if (_al_vector_size(&system->system.displays) > 1) {
         al_ungrab_mouse();
      }
   }

   if (flags & ALLEGRO_MAXIMIZED) {
      _al_xwin_maximize(display, true);
   }

   if (!_al_xglx_config_create_context(d)) {
      goto LateError;
   }

   /* Make our GLX context current for reading and writing in the current
    * thread.
    */
   if (d->fbc) {
      if (!glXMakeContextCurrent(system->gfxdisplay, d->glxwindow,
            d->glxwindow, d->context)) {
         ALLEGRO_ERROR("glXMakeContextCurrent failed\n");
      }
   }
   else {
      if (!glXMakeCurrent(system->gfxdisplay, d->glxwindow, d->context)) {
         ALLEGRO_ERROR("glXMakeCurrent failed\n");
      }
   }

   _al_ogl_manage_extensions(display);
   _al_ogl_set_extensions(ogl->extension_api);

   /* Print out OpenGL version info */
   ALLEGRO_INFO("OpenGL Version: %s\n", (const char*)glGetString(GL_VERSION));
   ALLEGRO_INFO("Vendor: %s\n", (const char*)glGetString(GL_VENDOR));
   ALLEGRO_INFO("Renderer: %s\n", (const char*)glGetString(GL_RENDERER));

   /* Fill in opengl version */
   const int v = display->ogl_extras->ogl_info.version;
   display->extra_settings.settings[ALLEGRO_OPENGL_MAJOR_VERSION] = (v >> 24) & 0xFF;
   display->extra_settings.settings[ALLEGRO_OPENGL_MINOR_VERSION] = (v >> 16) & 0xFF;

   if (display->ogl_extras->ogl_info.version < _ALLEGRO_OPENGL_VERSION_1_2) {
      ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds = _al_get_new_display_settings();
      if (eds->required & (1<<ALLEGRO_COMPATIBLE_DISPLAY)) {
         ALLEGRO_ERROR("Allegro requires at least OpenGL version 1.2 to work.\n");
         goto LateError;
      }
      display->extra_settings.settings[ALLEGRO_COMPATIBLE_DISPLAY] = 0;
   }

   if (display->extra_settings.settings[ALLEGRO_COMPATIBLE_DISPLAY])
      _al_ogl_setup_gl(display);

   /* vsync */
   int vsync_setting = _al_get_new_display_settings()->settings[ALLEGRO_VSYNC];
   vsync_setting = xdpy_swap_control(display, vsync_setting);
   display->extra_settings.settings[ALLEGRO_VSYNC] = vsync_setting;

   d->invisible_cursor = None; /* Will be created on demand. */
   d->current_cursor = None; /* Initially, we use the root cursor. */
   d->cursor_hidden = false;

   d->icon = None;
   d->icon_mask = None;

   return d;

EarlyError:
   al_free(d);
   al_free(ogl);
   return NULL;

LateError:
   xdpy_destroy_display(display);
   return NULL;
}


static bool xdpy_create_display_hook_default(ALLEGRO_DISPLAY *display,
   int w, int h)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *d = (ALLEGRO_DISPLAY_XGLX *)display;
   (void)w;
   (void)h;

   if (_al_xwin_initial_icon) {
      ALLEGRO_BITMAP *bitmaps[] = {_al_xwin_initial_icon};
      _al_xwin_set_icons(display, 1, bitmaps);
   } else {
      ALLEGRO_STATE state;
      al_store_state(&state, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
      ALLEGRO_BITMAP *bitmap = al_create_bitmap(ICON_WIDTH, ICON_HEIGHT);

      ALLEGRO_LOCKED_REGION *lr = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_RGBA_8888, ALLEGRO_LOCK_WRITEONLY);
      for (int y = 0; y < ICON_HEIGHT; y++) {
         memcpy((char*)lr->data + lr->pitch * y, &icon_data[ICON_WIDTH * y], ICON_WIDTH * 4);
      }
      al_unlock_bitmap(bitmap);

      ALLEGRO_BITMAP *bitmaps[] = {bitmap};
      _al_xwin_set_icons(display, 1, bitmaps);
      al_destroy_bitmap(bitmap);
   }

   XLockDisplay(system->x11display);

   XMapWindow(system->x11display, d->window);
   ALLEGRO_DEBUG("X11 window mapped.\n");

   d->wm_delete_window_atom = XInternAtom(system->x11display,
      "WM_DELETE_WINDOW", False);
   XSetWMProtocols(system->x11display, d->window, &d->wm_delete_window_atom, 1);

   XUnlockDisplay(system->x11display);

   d->overridable_vt = &default_overridable_vt;

   return true;
}


/* Create a new X11 display, which maps directly to a GLX window. */
static ALLEGRO_DISPLAY *xdpy_create_display(int w, int h)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *display;
   int flags;
   int adapter;

   if (system->x11display == NULL) {
      ALLEGRO_WARN("Not connected to X server.\n");
      return NULL;
   }

   if (w <= 0 || h <= 0) {
      ALLEGRO_ERROR("Invalid window size %dx%d\n", w, h);
      return NULL;
   }

   flags = al_get_new_display_flags();
   if (flags & ALLEGRO_GTK_TOPLEVEL_INTERNAL) {
      if (gtk_override_vt == NULL) {
         ALLEGRO_ERROR("GTK requested but unavailable\n");
         return NULL;
      }
      if (flags & ALLEGRO_FULLSCREEN) {
         ALLEGRO_ERROR("GTK incompatible with fullscreen\n");
         return NULL;
      }
   }

   _al_mutex_lock(&system->lock);

   adapter = al_get_new_display_adapter();
   display = xdpy_create_display_locked(system, flags, w, h, adapter);

   _al_mutex_unlock(&system->lock);

   return (ALLEGRO_DISPLAY *)display;
}


static void convert_display_bitmaps_to_memory_bitmap(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DEBUG("converting display bitmaps to memory bitmaps.\n");

   while (d->bitmaps._size > 0) {
      ALLEGRO_BITMAP **bptr = _al_vector_ref_back(&d->bitmaps);
      ALLEGRO_BITMAP *b = *bptr;
      _al_convert_to_memory_bitmap(b);
   }
}


static void transfer_display_bitmaps_to_any_other_display(
   ALLEGRO_SYSTEM_XGLX *s, ALLEGRO_DISPLAY *d)
{
   size_t i;
   ALLEGRO_DISPLAY *living = NULL;
   ASSERT(s->system.displays._size > 1);

   for (i = 0; i < s->system.displays._size; i++) {
      ALLEGRO_DISPLAY **slot = _al_vector_ref(&s->system.displays, i);
      living = *slot;
      if (living != d)
         break;
   }

   ALLEGRO_DEBUG("transferring display bitmaps to other display.\n");

   for (i = 0; i < d->bitmaps._size; i++) {
      ALLEGRO_BITMAP **add = _al_vector_alloc_back(&(living->bitmaps));
      ALLEGRO_BITMAP **ref = _al_vector_ref(&d->bitmaps, i);
      *add = *ref;
      (*add)->_display = living;
   }
}


static void restore_mode_if_last_fullscreen_display(ALLEGRO_SYSTEM_XGLX *s,
   ALLEGRO_DISPLAY_XGLX *d)
{
   bool last_fullscreen = true;
   size_t i;

   /* If any other fullscreen display is still active on the same adapter,
    * we must not touch the video mode.
    */
   for (i = 0; i < s->system.displays._size; i++) {
      ALLEGRO_DISPLAY_XGLX **slot = _al_vector_ref(&s->system.displays, i);
      ALLEGRO_DISPLAY_XGLX *living = *slot;

      if (living == d)
         continue;

      /* Check for fullscreen displays on the same adapter. */
      if (living->adapter == d->adapter
            && (living->display.flags & ALLEGRO_FULLSCREEN)) {
         last_fullscreen = false;
      }
   }

   if (last_fullscreen) {
      ALLEGRO_DEBUG("restore mode.\n");
      _al_xglx_restore_video_mode(s, d->adapter);
   }
   else {
      ALLEGRO_DEBUG("*not* restoring mode.\n");
   }
}


static void xdpy_destroy_display_hook_default(ALLEGRO_DISPLAY *d, bool is_last)
{
   ALLEGRO_SYSTEM_XGLX *s = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;
   (void)is_last;

   if (glx->context) {
      glXDestroyContext(s->gfxdisplay, glx->context);
      glx->context = NULL;
      ALLEGRO_DEBUG("destroy context.\n");
   }

   if (glx->fbc) {
      al_free(glx->fbc);
      glx->fbc = NULL;
      XFree(glx->xvinfo);
      glx->xvinfo = NULL;
   }
   else if (glx->xvinfo) {
      al_free(glx->xvinfo);
      glx->xvinfo = NULL;
   }

   if ((glx->glxwindow) && (glx->glxwindow != glx->window)) {
      glXDestroyWindow(s->x11display, glx->glxwindow);
      glx->glxwindow = 0;
      ALLEGRO_DEBUG("destroy glx window\n");
   }

   _al_cond_destroy(&glx->mapped);
   _al_cond_destroy(&glx->selectioned);

   ALLEGRO_DEBUG("destroy window.\n");
   XDestroyWindow(s->x11display, glx->window);

   _al_xglx_unuse_adapter(s, glx->adapter);

   if (d->flags & ALLEGRO_FULLSCREEN) {
      restore_mode_if_last_fullscreen_display(s, glx);
   }
}


static void xdpy_destroy_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_XGLX *s = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;
   ALLEGRO_OGL_EXTRAS *ogl = d->ogl_extras;
   bool is_last;

   ALLEGRO_DEBUG("destroying display.\n");

   /* If we're the last display, convert all bitmaps to display independent
    * (memory) bitmaps. Otherwise, pass all bitmaps to any other living
    * display. We assume all displays are compatible.)
    */
   is_last = (s->system.displays._size == 1);
   if (is_last)
      convert_display_bitmaps_to_memory_bitmap(d);
   else
      transfer_display_bitmaps_to_any_other_display(s, d);

   _al_ogl_unmanage_extensions(d);
   ALLEGRO_DEBUG("unmanaged extensions.\n");

   _al_mutex_lock(&s->lock);
   _al_vector_find_and_delete(&s->system.displays, &d);

   if (ogl->backbuffer) {
      _al_ogl_destroy_backbuffer(ogl->backbuffer);
      ogl->backbuffer = NULL;
      ALLEGRO_DEBUG("destroy backbuffer.\n");
   }

   if (glx->overridable_vt) {
      glx->overridable_vt->destroy_display_hook(d, is_last);
   }

   if (s->mouse_grab_display == d) {
      s->mouse_grab_display = NULL;
   }

   _al_vector_free(&d->bitmaps);
   _al_event_source_free(&d->es);

   al_free(d->ogl_extras);
   al_free(d->vertex_cache);
   al_free(d);

   _al_mutex_unlock(&s->lock);

   ALLEGRO_DEBUG("destroy display finished.\n");
}


static bool xdpy_make_current(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;

   /* Make our GLX context current for reading and writing in the current
    * thread.
    */
   if (glx->fbc) {
      return glXMakeContextCurrent(system->gfxdisplay, glx->glxwindow,
         glx->glxwindow, glx->context);
   }
   else {
      return glXMakeCurrent(system->gfxdisplay, glx->glxwindow, glx->context);
   }
}


static bool xdpy_set_current_display(ALLEGRO_DISPLAY *d)
{
   bool rc;

   rc = xdpy_make_current(d);
   if (rc) {
      ALLEGRO_OGL_EXTRAS *ogl = d->ogl_extras;
      _al_ogl_set_extensions(ogl->extension_api);
      _al_ogl_update_render_state(d);
   }

   return rc;
}


static void xdpy_unset_current_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   glXMakeContextCurrent(system->gfxdisplay, None, None, NULL);
   (void)d;
}


static void xdpy_flip_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;

   int e = glGetError();
   if (e) {
      ALLEGRO_ERROR("OpenGL error was not 0: %s\n", _al_gl_error_string(e));
   }

   if (d->extra_settings.settings[ALLEGRO_SINGLE_BUFFER])
      glFlush();
   else
      glXSwapBuffers(system->gfxdisplay, glx->glxwindow);
}


static void xdpy_update_display_region(ALLEGRO_DISPLAY *d, int x, int y,
   int w, int h)
{
   (void)x;
   (void)y;
   (void)w;
   (void)h;
   xdpy_flip_display(d);
}


static bool xdpy_acknowledge_resize(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;
   XWindowAttributes xwa;
   unsigned int w, h;

   _al_mutex_lock(&system->lock);

   /* glXQueryDrawable is GLX 1.3+. */
   /*
   glXQueryDrawable(system->x11display, glx->glxwindow, GLX_WIDTH, &w);
   glXQueryDrawable(system->x11display, glx->glxwindow, GLX_HEIGHT, &h);
   */

   XGetWindowAttributes(system->x11display, glx->window, &xwa);
   w = xwa.width;
   h = xwa.height;

   if ((int)w != d->w || (int)h != d->h) {
      d->w = w;
      d->h = h;

      ALLEGRO_DEBUG("xdpy: acknowledge_resize (%d, %d)\n", d->w, d->h);

      /* No context yet means this is a stray call happening during
       * initialization.
       */
      if (glx->context) {
         _al_ogl_setup_gl(d);
      }

      _al_xwin_check_maximized(d);
   }

   _al_mutex_unlock(&system->lock);

   return true;
}


/* Note: The system mutex must be locked (exactly once) so when we
 * wait for the condition variable it gets auto-unlocked. For a
 * nested lock that would not be the case.
 */
void _al_display_xglx_await_resize(ALLEGRO_DISPLAY *d, int old_resize_count,
   bool delay_hack)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;
   ALLEGRO_TIMEOUT timeout;

   ALLEGRO_DEBUG("Awaiting resize event\n");

   XSync(system->x11display, False);

   /* Wait until we are actually resized.
    * Don't wait forever if an event never comes.
    */
   al_init_timeout(&timeout, 1.0);
   while (old_resize_count == glx->resize_count) {
      if (_al_cond_timedwait(&system->resized, &system->lock, &timeout) == -1) {
         ALLEGRO_ERROR("Timeout while waiting for resize event.\n");
         return;
      }
   }

   /* XXX: This hack helps when toggling between fullscreen windows and not,
    * on various window managers.
    */
   if (delay_hack) {
      al_rest(0.2);
   }

   xdpy_acknowledge_resize(d);
}


static bool xdpy_resize_display_default(ALLEGRO_DISPLAY *d, int w, int h)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;
   XWindowAttributes xwa;
   int attempts;
   bool ret = false;

   _al_mutex_lock(&system->lock);

   /* It seems some X servers will treat the resize as a no-op if the window is
    * already the right size, so check for it to avoid a deadlock later.
    */
   XGetWindowAttributes(system->x11display, glx->window, &xwa);
   if (xwa.width == w && xwa.height == h) {
      _al_mutex_unlock(&system->lock);
      return false;
   }

   if (d->flags & ALLEGRO_FULLSCREEN) {
      _al_xwin_set_fullscreen_window(d, 0);
      if (!_al_xglx_fullscreen_set_mode(system, glx, w, h, 0, 0)) {
         ret = false;
         goto skip_resize;
      }
      attempts = 3;
   }
   else {
      attempts = 1;
   }

   /* Hack: try multiple times to resize the window, with delays.  KDE reacts
    * slowly to the video mode change, and won't resize our window until a
    * while after.  It would be better to wait for some sort of event rather
    * than just waiting some amount of time, but I didn't manage to find that
    * event. --pw
    */
   for (; attempts >= 0; attempts--) {
      const int old_resize_count = glx->resize_count;
      ALLEGRO_DEBUG("calling XResizeWindow, attempts=%d\n", attempts);
      _al_xwin_reset_size_hints(d);
      glx->programmatic_resize = true;
      XResizeWindow(system->x11display, glx->window, w, h);
      _al_display_xglx_await_resize(d, old_resize_count,
         (d->flags & ALLEGRO_FULLSCREEN));
      glx->programmatic_resize = false;
      _al_xwin_set_size_hints(d, INT_MAX, INT_MAX);

      if (d->w == w && d->h == h) {
         ret = true;
         break;
      }

      /* Wait before trying again. */
      al_rest(0.333);
   }

   if (attempts == 0) {
      ALLEGRO_ERROR("XResizeWindow didn't work; giving up\n");
   }

skip_resize:

   if (d->flags & ALLEGRO_FULLSCREEN) {
      _al_xwin_set_fullscreen_window(d, 1);
      _al_xwin_set_above(d, 1);
      _al_xglx_fullscreen_to_display(system, glx);
      ALLEGRO_DEBUG("xdpy: resize fullscreen?\n");
   }

   _al_mutex_unlock(&system->lock);
   return ret;
}


static bool xdpy_resize_display(ALLEGRO_DISPLAY *d, int w, int h)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;

   /* A fullscreen-window can't be resized. */
   if (d->flags & ALLEGRO_FULLSCREEN_WINDOW)
      return false;

   return glx->overridable_vt->resize_display(d, w, h);
}


void _al_xglx_display_configure(ALLEGRO_DISPLAY *d, int x, int y,
   int width, int height, bool setglxy)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;

   ALLEGRO_EVENT_SOURCE *es = &glx->display.es;
   _al_event_source_lock(es);

   /* Generate a resize event if the size has changed non-programmatically.
    * We cannot asynchronously change the display size here yet, since the user
    * will only know about a changed size after receiving the resize event.
    * Here we merely add the event to the queue.
    */
   if (!glx->programmatic_resize &&
         (d->w != width ||
          d->h != height)) {
      if (_al_event_source_needs_to_generate_event(es)) {
         ALLEGRO_EVENT event;
         event.display.type = ALLEGRO_EVENT_DISPLAY_RESIZE;
         event.display.timestamp = al_get_time();
         event.display.x = x;
         event.display.y = y;
         event.display.width = width;
         event.display.height = height;
         _al_event_source_emit_event(es, &event);
      }
   }

   if (setglxy) {
      glx->x = x;
      glx->y = y;
   }

   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX*)al_get_system_driver();
   ALLEGRO_MONITOR_INFO mi;
   int center_x = (glx->x + (glx->x + width)) / 2;
   int center_y = (glx->y + (glx->y + height)) / 2;

   _al_xglx_get_monitor_info(system, glx->adapter, &mi);

   ALLEGRO_DEBUG("xconfigure event! %ix%i\n", x, y);

   /* check if we're no longer inside the stored adapter */
   if ((center_x < mi.x1 && center_x > mi.x2) ||
      (center_y < mi.y1 && center_y > mi.x2))
   {
      int new_adapter = _al_xglx_get_adapter(system, glx, true);
      if (new_adapter != glx->adapter) {
         ALLEGRO_DEBUG("xdpy: adapter change!\n");
         _al_xglx_unuse_adapter(system, glx->adapter);
         if (d->flags & ALLEGRO_FULLSCREEN)
            _al_xglx_restore_video_mode(system, glx->adapter);
         glx->adapter = new_adapter;
         _al_xglx_use_adapter(system, glx->adapter);
      }

   }

   _al_xwin_check_maximized(d);

   _al_event_source_unlock(es);
}


/* Handle an X11 configure event. [X11 thread]
 * Only called from the event handler with the system locked.
 */
void _al_xglx_display_configure_event(ALLEGRO_DISPLAY *d, XEvent *xevent)
{
   /* We receive two configure events when toggling the window frame.
    * We ignore the first one as it has bogus coordinates.
    * The only way to tell them apart seems to be the send_event field.
    * Unfortunately, we also end up ignoring the only event we receive in
    * response to a XMoveWindow request so we have to compensate for that.
    */
   bool setglxy = (xevent->xconfigure.send_event);
   _al_xglx_display_configure(d, xevent->xconfigure.x, xevent->xconfigure.y,
      xevent->xconfigure.width, xevent->xconfigure.height, setglxy);
}



/* Handle X11 switch event. [X11 thread]
 */
void _al_xwin_display_switch_handler(ALLEGRO_DISPLAY *display,
   XFocusChangeEvent *xevent)
{
   /* Mouse click in/out tend to set NotifyNormal events. For Alt-Tab,
    * there are also accompanying NotifyGrab/NotifyUngrab events which we don't
    * care about. At this point, some WMs either send NotifyNormal (KDE) or
    * NotifyWhileGrabbed (GNOME).
    */
   if (xevent->mode == NotifyNormal || xevent->mode == NotifyWhileGrabbed)
      _al_xwin_display_switch_handler_inner(display, (xevent->type == FocusIn));
}



/* Handle X11 switch event. [X11 thread]
 */
void _al_xwin_display_switch_handler_inner(ALLEGRO_DISPLAY *display, bool focus_in)
{
   ALLEGRO_EVENT_SOURCE *es = &display->es;
   _al_event_source_lock(es);
   if (_al_event_source_needs_to_generate_event(es)) {
      ALLEGRO_EVENT event;
      if (focus_in)
         event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_IN;
      else
         event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_OUT;
      event.display.timestamp = al_get_time();
      _al_event_source_emit_event(es, &event);
   }
   _al_event_source_unlock(es);
}



void _al_xwin_display_expose(ALLEGRO_DISPLAY *display,
   XExposeEvent *xevent)
{
   ALLEGRO_EVENT_SOURCE *es = &display->es;
   _al_event_source_lock(es);
   if (_al_event_source_needs_to_generate_event(es)) {
      ALLEGRO_EVENT event;
      event.display.type = ALLEGRO_EVENT_DISPLAY_EXPOSE;
      event.display.timestamp = al_get_time();
      event.display.x = xevent->x;
      event.display.y = xevent->y;
      event.display.width = xevent->width;
      event.display.height = xevent->height;
      _al_event_source_emit_event(es, &event);
   }
   _al_event_source_unlock(es);
}


static bool xdpy_is_compatible_bitmap(ALLEGRO_DISPLAY *display,
   ALLEGRO_BITMAP *bitmap)
{
   /* All GLX bitmaps are compatible. */
   (void)display;
   (void)bitmap;
   return true;
}


static void xdpy_set_window_title_default(ALLEGRO_DISPLAY *display, const char *title)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;

   {
      Atom WM_NAME = XInternAtom(system->x11display, "WM_NAME", False);
      Atom _NET_WM_NAME = XInternAtom(system->x11display, "_NET_WM_NAME", False);
      char *list[1] = { (char *) title };
      XTextProperty property;

      Xutf8TextListToTextProperty(system->x11display, list, 1, XUTF8StringStyle,
         &property);
      XSetTextProperty(system->x11display, glx->window, &property, WM_NAME);
      XSetTextProperty(system->x11display, glx->window, &property, _NET_WM_NAME);
      XSetTextProperty(system->x11display, glx->window, &property, XA_WM_NAME);
      XFree(property.value);
   }
   {
      XClassHint *hint = XAllocClassHint();
      if (hint) {
         ALLEGRO_PATH *exepath = al_get_standard_path(ALLEGRO_EXENAME_PATH);
         // hint doesn't use a const char*, so we use strdup to create a non const string
         hint->res_name = strdup(al_get_path_basename(exepath));
         hint->res_class = strdup(al_get_path_basename(exepath));
         XSetClassHint(system->x11display, glx->window, hint);
         free(hint->res_name);
         free(hint->res_class);
         XFree(hint);
         al_destroy_path(exepath);
      }
   }
}


static void xdpy_set_window_title(ALLEGRO_DISPLAY *display, const char *title)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;

   _al_mutex_lock(&system->lock);
   glx->overridable_vt->set_window_title(display, title);
   _al_mutex_unlock(&system->lock);
}


static void xdpy_set_window_position_default(ALLEGRO_DISPLAY *display,
   int x, int y)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   Window root, parent, child, *children;
   unsigned int n;

   _al_mutex_lock(&system->lock);

   /* To account for the window border, we have to find the parent window which
    * draws the border. If the parent is the root though, then we should not
    * translate.
    */
   XQueryTree(system->x11display, glx->window, &root, &parent, &children, &n);
   if (parent != root) {
      XTranslateCoordinates(system->x11display, parent, glx->window,
         x, y, &x, &y, &child);
   }

   XMoveWindow(system->x11display, glx->window, x, y);
   XFlush(system->x11display);

   /* We have to store these immediately, as we will ignore the XConfigureEvent
    * that we receive in response.  _al_display_xglx_configure() knows why.
    */
   glx->x = x;
   glx->y = y;

   _al_mutex_unlock(&system->lock);
}


static void xdpy_set_window_position(ALLEGRO_DISPLAY *display, int x, int y)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   glx->overridable_vt->set_window_position(display, x, y);
}


static void xdpy_get_window_position(ALLEGRO_DISPLAY *display, int *x, int *y)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   /* We could also query the X11 server, but it just would take longer, and
    * would not be synchronized to our events. The latter can be an advantage
    * or disadvantage.
    */
   *x = glx->x;
   *y = glx->y;
}


static bool xdpy_set_window_constraints_default(ALLEGRO_DISPLAY *display,
   int min_w, int min_h, int max_w, int max_h)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;

   glx->display.min_w = min_w;
   glx->display.min_h = min_h;
   glx->display.max_w = max_w;
   glx->display.max_h = max_h;

   return true;
}


static bool xdpy_set_window_constraints(ALLEGRO_DISPLAY *display,
   int min_w, int min_h, int max_w, int max_h)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   return glx->overridable_vt->set_window_constraints(display,
      min_w, min_h, max_w, max_h);
}


static bool xdpy_get_window_constraints(ALLEGRO_DISPLAY *display,
   int *min_w, int *min_h, int *max_w, int * max_h)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;

   *min_w = glx->display.min_w;
   *min_h = glx->display.min_h;
   *max_w = glx->display.max_w;
   *max_h = glx->display.max_h;

   return true;
}


static void xdpy_apply_window_constraints(ALLEGRO_DISPLAY *display,
   bool onoff)
{
   int posX;
   int posY;
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();

   _al_mutex_lock(&system->lock);

   if (onoff) {
      al_get_window_position(display, &posX, &posY);
      _al_xwin_set_size_hints(display, posX, posY);
   }
   else {
      _al_xwin_reset_size_hints(display);
   }

   _al_mutex_unlock(&system->lock);
   al_resize_display(display, display->w, display->h);
}


static void xdpy_set_fullscreen_window_default(ALLEGRO_DISPLAY *display, bool onoff)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   if (onoff == !(display->flags & ALLEGRO_FULLSCREEN_WINDOW)) {
      _al_mutex_lock(&system->lock);
      _al_xwin_reset_size_hints(display);
      _al_xwin_set_fullscreen_window(display, 2);
      /* XXX Technically, the user may fiddle with the _NET_WM_STATE_FULLSCREEN
       * property outside of Allegro so this flag may not be in sync with
       * reality.
       */
      display->flags ^= ALLEGRO_FULLSCREEN_WINDOW;
      _al_xwin_set_size_hints(display, INT_MAX, INT_MAX);

      set_compositor_bypass_flag(display);
      _al_mutex_unlock(&system->lock);
   }
}


static void xdpy_set_fullscreen_window(ALLEGRO_DISPLAY *display, bool onoff)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   glx->overridable_vt->set_fullscreen_window(display, onoff);
}


static bool xdpy_set_display_flag(ALLEGRO_DISPLAY *display, int flag,
   bool flag_onoff)
{
   switch (flag) {
      case ALLEGRO_FRAMELESS:
         /* The ALLEGRO_FRAMELESS flag is backwards. */
         _al_xwin_set_frame(display, !flag_onoff);
         return true;
      case ALLEGRO_FULLSCREEN_WINDOW:
         xdpy_set_fullscreen_window(display, flag_onoff);
         return true;
      case ALLEGRO_MAXIMIZED:
         _al_xwin_maximize(display, flag_onoff);
         return true;
   }
   return false;
}


static bool xdpy_wait_for_vsync(ALLEGRO_DISPLAY *display)
{
   (void) display;

   if (al_get_opengl_extension_list()->ALLEGRO_GLX_SGI_video_sync) {
      unsigned int count;
      glXGetVideoSyncSGI(&count);
      glXWaitVideoSyncSGI(2, (count+1) & 1, &count);
      return true;
   }

   return false;
}


/* Obtain a reference to this driver. */
ALLEGRO_DISPLAY_INTERFACE *_al_display_xglx_driver(void)
{
   if (xdpy_vt.create_display)
      return &xdpy_vt;

   xdpy_vt.create_display = xdpy_create_display;
   xdpy_vt.destroy_display = xdpy_destroy_display;
   xdpy_vt.set_current_display = xdpy_set_current_display;
   xdpy_vt.unset_current_display = xdpy_unset_current_display;
   xdpy_vt.flip_display = xdpy_flip_display;
   xdpy_vt.update_display_region = xdpy_update_display_region;
   xdpy_vt.acknowledge_resize = xdpy_acknowledge_resize;
   xdpy_vt.create_bitmap = _al_ogl_create_bitmap;
   xdpy_vt.get_backbuffer = _al_ogl_get_backbuffer;
   xdpy_vt.set_target_bitmap = _al_ogl_set_target_bitmap;
   xdpy_vt.is_compatible_bitmap = xdpy_is_compatible_bitmap;
   xdpy_vt.resize_display = xdpy_resize_display;
   xdpy_vt.set_icons = _al_xwin_set_icons;
   xdpy_vt.set_window_title = xdpy_set_window_title;
   xdpy_vt.set_window_position = xdpy_set_window_position;
   xdpy_vt.get_window_position = xdpy_get_window_position;
   xdpy_vt.set_window_constraints = xdpy_set_window_constraints;
   xdpy_vt.get_window_constraints = xdpy_get_window_constraints;
   xdpy_vt.apply_window_constraints = xdpy_apply_window_constraints;
   xdpy_vt.set_display_flag = xdpy_set_display_flag;
   xdpy_vt.wait_for_vsync = xdpy_wait_for_vsync;
   xdpy_vt.update_render_state = _al_ogl_update_render_state;

   _al_xwin_add_cursor_functions(&xdpy_vt);
   _al_xwin_add_clipboard_functions(&xdpy_vt);
   _al_ogl_add_drawing_functions(&xdpy_vt);

   return &xdpy_vt;
}


static const ALLEGRO_XWIN_DISPLAY_OVERRIDABLE_INTERFACE default_overridable_vt =
{
   xdpy_create_display_hook_default,
   xdpy_destroy_display_hook_default,
   xdpy_resize_display_default,
   xdpy_set_window_title_default,
   xdpy_set_fullscreen_window_default,
   xdpy_set_window_position_default,
   xdpy_set_window_constraints_default
};


bool _al_xwin_set_gtk_display_overridable_interface(uint32_t check_version,
   const ALLEGRO_XWIN_DISPLAY_OVERRIDABLE_INTERFACE *vt)
{
   /* The version of the native dialogs addon must exactly match the core
    * library version.
    */
   if (vt && check_version == ALLEGRO_VERSION_INT) {
      ALLEGRO_DEBUG("GTK vtable made available\n");
      gtk_override_vt = vt;
      return true;
   }

   ALLEGRO_DEBUG("GTK vtable reset\n");
   gtk_override_vt = NULL;
   return (vt == NULL);
}


/* vim: set sts=3 sw=3 et: */
