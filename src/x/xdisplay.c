#include <X11/Xatom.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xcursor.h"
#include "allegro5/internal/aintern_xdisplay.h"
#include "allegro5/internal/aintern_xfullscreen.h"
#include "allegro5/internal/aintern_xglx_config.h"
#include "allegro5/internal/aintern_xsystem.h"
#include "allegro5/internal/aintern_xwindow.h"
#include "allegro5/platform/aintxglx.h"

ALLEGRO_DEBUG_CHANNEL("display")

static ALLEGRO_DISPLAY_INTERFACE xdpy_vt;


/* Helper to set a window icon.  We use the _NET_WM_ICON property which is
 * supported by modern window managers.
 *
 * The old method is XSetWMHints but the (antiquated) ICCCM talks about 1-bit
 * pixmaps.  For colour icons, perhaps you're supposed use the icon_window,
 * and draw the window yourself?
 */
static bool xdpy_set_icon_inner(Display *x11display, Window window,
   ALLEGRO_BITMAP *bitmap, int prop_mode)
{
   int w, h;
   int data_size;
   unsigned long *data; /* Yes, unsigned long, even on 64-bit platforms! */
   ALLEGRO_LOCKED_REGION *lr;
   bool ret;

   w = al_get_bitmap_width(bitmap);
   h = al_get_bitmap_height(bitmap);
   data_size = 2 + w * h;
   data = al_malloc(data_size * sizeof(data[0]));
   if (!data)
      return false;

   lr = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA,
      ALLEGRO_LOCK_READONLY);
   if (lr) {
      int x, y;
      ALLEGRO_COLOR c;
      unsigned char r, g, b, a;
      Atom _NET_WM_ICON;

      data[0] = w;
      data[1] = h;
      for (y = 0; y < h; y++) {
         for (x = 0; x < w; x++) {
            c = al_get_pixel(bitmap, x, y);
            al_unmap_rgba(c, &r, &g, &b, &a);
            data[2 + y*w + x] = (a << 24) | (r << 16) | (g << 8) | b;
         }
      }

      _NET_WM_ICON = XInternAtom(x11display, "_NET_WM_ICON", False);
      XChangeProperty(x11display, window, _NET_WM_ICON, XA_CARDINAL, 32,
         prop_mode, (unsigned char *)data, data_size);

      al_unlock_bitmap(bitmap);
      ret = true;
   }
   else {
      ret = false;
   }

   al_free(data);

   return ret;
}

static void xdpy_set_icons(ALLEGRO_DISPLAY *d,
   int num_icons, ALLEGRO_BITMAP *bitmaps[])
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;
   int prop_mode = PropModeReplace;
   int i;

   _al_mutex_lock(&system->lock);

   for (i = 0; i < num_icons; i++) {
      if (xdpy_set_icon_inner(system->x11display, glx->window, bitmaps[i],
            prop_mode)) {
         prop_mode = PropModeAppend;
      }
   }

   _al_mutex_unlock(&system->lock);
}



static void xdpy_set_window_position(ALLEGRO_DISPLAY *display, int x, int y)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
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



static void xdpy_set_frame(ALLEGRO_DISPLAY *display, bool frame_on)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   Display *x11 = system->x11display;
   Atom hints;

   _al_mutex_lock(&system->lock);

#if 1
   /* This code is taken from the GDK sources. So it works perfectly in Gnome,
    * no idea if it will work anywhere else. X11 documentation itself only
    * describes a way how to make the window completely unmanaged, but that
    * would also require special care in the event handler.
    */
   hints = XInternAtom(x11, "_MOTIF_WM_HINTS", True);
   if (hints) {
      struct {
         unsigned long flags;
         unsigned long functions;
         unsigned long decorations;
         long input_mode;
         unsigned long status;
      } motif = {2, 0, frame_on, 0, 0};
      XChangeProperty(x11, glx->window, hints, hints, 32, PropModeReplace,
         (void *)&motif, sizeof motif / 4);

      if (frame_on)
         display->flags &= ~ALLEGRO_FRAMELESS;
      else
         display->flags |= ALLEGRO_FRAMELESS;
   }
#endif

   _al_mutex_unlock(&system->lock);
}



static void xdpy_set_fullscreen_window(ALLEGRO_DISPLAY *display, bool onoff)
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
      _al_mutex_unlock(&system->lock);
   }
}



static bool xdpy_set_display_flag(ALLEGRO_DISPLAY *display, int flag,
   bool flag_onoff)
{
   switch (flag) {
      case ALLEGRO_FRAMELESS:
         /* The ALLEGRO_FRAMELESS flag is backwards. */
         xdpy_set_frame(display, !flag_onoff);
         return true;
      case ALLEGRO_FULLSCREEN_WINDOW:
         xdpy_set_fullscreen_window(display, flag_onoff);
         return true;
   }
   return false;
}

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

static void xdpy_destroy_display(ALLEGRO_DISPLAY *d);


/* Create a new X11 display, which maps directly to a GLX window. */
static ALLEGRO_DISPLAY *xdpy_create_display(int w, int h)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   int adapter = al_get_new_display_adapter();
   
   if (system->x11display == NULL) {
      ALLEGRO_WARN("Not connected to X server.\n");
      return NULL;
   }

   if (w <= 0 || h <= 0) {
      ALLEGRO_ERROR("Invalid window size %dx%d\n", w, h);
      return NULL;
   }

   _al_mutex_lock(&system->lock);

   ALLEGRO_DISPLAY_XGLX *d = al_calloc(1, sizeof *d);
   ALLEGRO_DISPLAY *display = (void*)d;
   ALLEGRO_OGL_EXTRAS *ogl = al_calloc(1, sizeof *ogl);
   display->ogl_extras = ogl;

   int major, minor;
   glXQueryVersion(system->x11display, &major, &minor);
   d->glx_version = major * 100 + minor * 10;
   ALLEGRO_INFO("GLX %.1f.\n", d->glx_version / 100.f);

   display->w = w;
   display->h = h;
   display->vt = _al_display_xglx_driver();
   display->refresh_rate = al_get_new_display_refresh_rate();
   display->flags = al_get_new_display_flags();

   // FIXME: default? Is this the right place to set this?
   display->flags |= ALLEGRO_OPENGL;

   // store our initial virtual adapter, used by fullscreen and positioning code
   d->adapter = adapter;
   ALLEGRO_DEBUG("selected adapter %i\n", adapter);
   if (d->adapter < 0) {
      d->adapter = _al_xglx_get_default_adapter(system);
   }

   _al_xglx_use_adapter(system, d->adapter);
   
   /* if we're in multi-head X mode, bail if we try to use more than one display
    * as there are bugs in X/glX that cause us to hang in X if we try to use more than one. */
   /* if we're in real xinerama mode, also bail, x makes mouse use evil */
   
   bool true_xinerama_active = false;
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   bool xrandr_active = false;

#ifdef ALLEGRO_XWINDOWS_WITH_XRANDR
   xrandr_active = system->xrandr_available;
#endif

   true_xinerama_active = !xrandr_active && system->xinerama_available;
#endif
   
   if ((true_xinerama_active || ScreenCount(system->x11display) > 1) && system->adapter_use_count) {
      uint32_t i, adapter_use_count = 0;
      for (i = 0; i < 32; i++) {
         if (system->adapter_map[i])
            adapter_use_count++;
      }
      
      if (adapter_use_count > 1) {
         ALLEGRO_ERROR("Use of more than one adapter at once in multi-head X or X with true Xinerama active is not possible.\n");
         al_free(d);
         al_free(ogl);
         _al_mutex_unlock(&system->lock);
         return NULL;
      }
   }
   ALLEGRO_DEBUG("xdpy: selected adapter %i\n", d->adapter);
   
   // store or initial X Screen, used by window creation, fullscreen, and glx visual code
   d->xscreen = _al_xglx_get_xscreen(system, d->adapter);
   
   ALLEGRO_DEBUG("xdpy: selected xscreen %i\n", d->xscreen);
   
   d->is_mapped = false;
   _al_cond_init(&d->mapped);

   d->resize_count = 0;
   d->programmatic_resize = false;

   _al_xglx_config_select_visual(d);

   if (!d->xvinfo) {
      ALLEGRO_ERROR("FIXME: Need better visual selection.\n");
      ALLEGRO_ERROR("No matching visual found.\n");
      al_free(d);
      al_free(ogl);
      _al_mutex_unlock(&system->lock);
      return NULL;
   }

   ALLEGRO_INFO("Selected X11 visual %lu.\n", d->xvinfo->visualid);

   /* Add ourself to the list of displays. */
   ALLEGRO_DISPLAY_XGLX **add;
   add = _al_vector_alloc_back(&system->system.displays);
   *add = d;

   /* Each display is an event source. */
   _al_event_source_init(&display->es);

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
      FocusChangeMask |
      ExposureMask |
      PropertyChangeMask |
      ButtonPressMask |
      ButtonReleaseMask |
      PointerMotionMask;

   /* For a non-compositing window manager, a black background can look
    * less broken if the application doesn't react to expose events fast
    * enough. However in some cases like resizing, the black background
    * causes horrible flicker.
    */
   if (!(display->flags & ALLEGRO_RESIZABLE)) {
      mask |= CWBackPixel;
      swa.background_pixel = BlackPixel(system->x11display, d->xvinfo->screen);
   }

   int x_off = INT_MAX, y_off = INT_MAX;
   if (display->flags & ALLEGRO_FULLSCREEN) {
      _al_xglx_get_display_offset(system, d->adapter, &x_off, &y_off);
   }
   else {
      /* we want new_display_adapter's offset to add to the new_window_position */
      int xscr_x = 0, xscr_y = 0;

      al_get_new_window_position(&x_off, &y_off);
      
      if (adapter >= 0) {
         /* non default adapter. I'm assuming this means the user wants the window to be placed on the adapter offset by new display pos */
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
      
   // Try to set full screen mode if requested, fail if we can't
   if (display->flags & ALLEGRO_FULLSCREEN) {
      /* According to the spec, the window manager is supposed to disable
       * window decorations when _NET_WM_STATE_FULLSCREEN is in effect.
       * However, some WMs may not be fully compliant, e.g. Fluxbox.
       */
      
      xdpy_set_frame(display, false);

      _al_xglx_set_above(display, 1);
      
      if (!_al_xglx_fullscreen_set_mode(system, d, w, h, 0, display->refresh_rate)) {
         ALLEGRO_DEBUG("xdpy: failed to set fullscreen mode.\n");
         xdpy_destroy_display(display);
         _al_mutex_unlock(&system->lock);
         return NULL;
      }
      //XSync(system->x11display, False);
   }

   if (display->flags & ALLEGRO_FRAMELESS) {
      xdpy_set_frame(display, false);
   }

   ALLEGRO_DEBUG("X11 window created.\n");
   
   _al_xwin_set_size_hints(display, x_off, y_off);
   
   XLockDisplay(system->x11display);
   
   d->wm_delete_window_atom = XInternAtom(system->x11display,
      "WM_DELETE_WINDOW", False);
   XSetWMProtocols(system->x11display, d->window, &d->wm_delete_window_atom, 1);

   XMapWindow(system->x11display, d->window);
   ALLEGRO_DEBUG("X11 window mapped.\n");
   
   XUnlockDisplay(system->x11display);

   /* Send the pending request to the X server. */
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
      _al_xglx_set_above(display, 1);

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
   
   if (!_al_xglx_config_create_context(d)) {
      xdpy_destroy_display(display);
      _al_mutex_unlock(&system->lock);
      return NULL;
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

   if (display->ogl_extras->ogl_info.version < _ALLEGRO_OPENGL_VERSION_1_2) {
      ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds = _al_get_new_display_settings();
      if (eds->required & (1<<ALLEGRO_COMPATIBLE_DISPLAY)) {
         ALLEGRO_ERROR("Allegro requires at least OpenGL version 1.2 to work.\n");
         xdpy_destroy_display(display);
         _al_mutex_unlock(&system->lock);
         return NULL;
      }
      display->extra_settings.settings[ALLEGRO_COMPATIBLE_DISPLAY] = 0;
   }
#if 0
   // Apparently, you can get a OpenGL 3.0 context without specifically creating
   // it with glXCreateContextAttribsARB, and not every OpenGL 3.0 is evil, but we
   // can't tell the difference at this stage.
   else if (display->ogl_extras->ogl_info.version > _ALLEGRO_OPENGL_VERSION_2_1) {
      /* We don't have OpenGL3 a driver. */
      display->extra_settings.settings[ALLEGRO_COMPATIBLE_DISPLAY] = 0;
   }
#endif

   if (display->extra_settings.settings[ALLEGRO_COMPATIBLE_DISPLAY])
      _al_ogl_setup_gl(display);

   /* vsync */

   /* Fill in the user setting. */
   display->extra_settings.settings[ALLEGRO_VSYNC] =
      _al_get_new_display_settings()->settings[ALLEGRO_VSYNC];

   /* We set the swap interval to 0 if vsync is forced off, and to 1
    * if it is forced on.
    * http://www.opengl.org/registry/specs/SGI/swap_control.txt
    * If the option is set to 0, we simply use the system default. The
    * above extension specifies vsync on as default though, so in the
    * end with GLX we can't force vsync on, just off.
    */
   ALLEGRO_DEBUG("requested vsync=%d.\n",
      display->extra_settings.settings[ALLEGRO_VSYNC]);
   if (display->extra_settings.settings[ALLEGRO_VSYNC]) {
      if (display->ogl_extras->extension_list->ALLEGRO_GLX_SGI_swap_control) {
         int x = 1;
         if (display->extra_settings.settings[ALLEGRO_VSYNC] == 2)
            x = 0;
         if (glXSwapIntervalSGI(x)) {
            ALLEGRO_WARN("glXSwapIntervalSGI(%d) failed.\n", x);
         }
      }
      else {
         ALLEGRO_WARN("no vsync, GLX_SGI_swap_control missing.\n");
         /* According to the specification that means it's on, but
          * the driver might have disabled it. So we do not know.
          */
         display->extra_settings.settings[ALLEGRO_VSYNC] = 0;
      }
   }

   d->invisible_cursor = None; /* Will be created on demand. */
   d->current_cursor = None; /* Initially, we use the root cursor. */
   d->cursor_hidden = false;

   d->icon = None;
   d->icon_mask = None;

   _al_mutex_unlock(&system->lock);

   return display;
}


static void xdpy_destroy_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_XGLX *s = (void *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (void *)d;
   ALLEGRO_OGL_EXTRAS *ogl = d->ogl_extras;

   ALLEGRO_DEBUG("destroy display.\n");

   /* If we're the last display, convert all bitmpas to display independent
    * (memory) bitmaps. */
   if (s->system.displays._size == 1) {
      while (d->bitmaps._size > 0) {
         ALLEGRO_BITMAP **bptr = _al_vector_ref_back(&d->bitmaps);
         ALLEGRO_BITMAP *b = *bptr;
         _al_convert_to_memory_bitmap(b);
      }
   }
   else {
      /* Pass all bitmaps to any other living display. (We assume all displays
       * are compatible.) */
      size_t i;
      ALLEGRO_DISPLAY **living = NULL;
      ASSERT(s->system.displays._size > 1);

      for (i = 0; i < s->system.displays._size; i++) {
         living = _al_vector_ref(&s->system.displays, i);
         if (*living != d)
            break;
      }

      for (i = 0; i < d->bitmaps._size; i++) {
         ALLEGRO_BITMAP **add = _al_vector_alloc_back(&(*living)->bitmaps);
         ALLEGRO_BITMAP **ref = _al_vector_ref(&d->bitmaps, i);
         *add = *ref;
         (*add)->display = *living;
      }
   }

   _al_xglx_unuse_adapter(s, glx->adapter);
   
   _al_ogl_unmanage_extensions(d);
   ALLEGRO_DEBUG("unmanaged extensions.\n");

   _al_mutex_lock(&s->lock);
   _al_vector_find_and_delete(&s->system.displays, &d);
   XDestroyWindow(s->x11display, glx->window);

   if (s->mouse_grab_display == d) {
      s->mouse_grab_display = NULL;
   }

   ALLEGRO_DEBUG("destroy window.\n");

   if (d->flags & ALLEGRO_FULLSCREEN) {
      size_t i;
      ALLEGRO_DISPLAY **living = NULL;
      bool last_fullscreen = true;
      /* If any other fullscreen display is still active on the same adapter,
       * we must not touch the video mode.
       */
      for (i = 0; i < s->system.displays._size; i++) {
         living = _al_vector_ref(&s->system.displays, i);
         ALLEGRO_DISPLAY_XGLX *living_glx = (void*)*living;
         
         if (*living == d) continue;
         
         /* check for fullscreen displays on the same adapter */
         if (living_glx->adapter == glx->adapter &&
             al_get_display_flags(*living) & ALLEGRO_FULLSCREEN)
         {
            last_fullscreen = false;
         }
      }
      
      if (last_fullscreen) {
         ALLEGRO_DEBUG("restore modes.\n");
         _al_xglx_restore_video_mode(s, glx->adapter);
      }
      else {
         ALLEGRO_DEBUG("*not* restoring modes.\n");
      }
   }

   if (ogl->backbuffer) {
      _al_ogl_destroy_backbuffer(ogl->backbuffer);
      ogl->backbuffer = NULL;
      ALLEGRO_DEBUG("destroy backbuffer.\n");
   }

   if (glx->context) {
      glXDestroyContext(s->gfxdisplay, glx->context);
      glx->context = NULL;
      ALLEGRO_DEBUG("destroy context.\n");
   }

   /* XXX quick pre-release hack */
   /* In multi-window programs these result in a double-free bugs. */
#if 0
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
#endif

   _al_cond_destroy(&glx->mapped);

   _al_vector_free(&d->bitmaps);
   _al_event_source_free(&d->es);

   al_free(d->ogl_extras);
   al_free(d->vertex_cache);
   al_free(d);

   _al_mutex_unlock(&s->lock);

   ALLEGRO_DEBUG("destroy display finished.\n");
}



static bool xdpy_set_current_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;
   ALLEGRO_OGL_EXTRAS *ogl = d->ogl_extras;

   /* Make our GLX context current for reading and writing in the current
    * thread.
    */
   if (glx->fbc) {
      if (!glXMakeContextCurrent(system->gfxdisplay, glx->glxwindow,
                                glx->glxwindow, glx->context))
         return false;
   }
   else {
      if (!glXMakeCurrent(system->gfxdisplay, glx->glxwindow, glx->context))
         return false;
   }

   _al_ogl_set_extensions(ogl->extension_api);

   return true;
}



static void xdpy_unset_current_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   (void)d;

   glXMakeContextCurrent(system->gfxdisplay, None, None, NULL);
}



/* Dummy implementation of flip. */
static void xdpy_flip_display(ALLEGRO_DISPLAY *d)
{
   int e = glGetError();
   if (e) {
      ALLEGRO_ERROR("OpenGL error was not 0: %d\n", e);
   }

   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;
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



static bool xdpy_resize_display(ALLEGRO_DISPLAY *d, int w, int h)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;
   XWindowAttributes xwa;
   int attempts;
   bool ret = false;

   /* A fullscreen-window can't be resized. */
   if (d->flags & ALLEGRO_FULLSCREEN_WINDOW)
      return false;

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
      _al_xglx_set_above(d, 1);
      _al_xglx_fullscreen_to_display(system, glx);
      ALLEGRO_DEBUG("xdpy: resize fullscreen?\n");
   }

   _al_mutex_unlock(&system->lock);
   return ret;
}


void _al_xglx_display_configure(ALLEGRO_DISPLAY *d, int x, int y,
   int width, int height, bool setglxy)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;

   ALLEGRO_EVENT_SOURCE *es = &glx->display.es;
   _al_event_source_lock(es);

   /* Generate a resize event if the size has changed non-programmtically.
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

   /* We receive two configure events when toggling the window frame.
    * We ignore the first one as it has bogus coordinates.
    * The only way to tell them apart seems to be the send_event field.
    * Unfortunately, we also end up ignoring the only event we receive in
    * response to a XMoveWindow request so we have to compensate for that.
    */
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
   /* Anything but NotifyNormal seem to indicate the switch is not "real".
    * TODO: Find out details?
    */
   if (xevent->mode != NotifyNormal)
      return;

   ALLEGRO_EVENT_SOURCE *es = &display->es;
   _al_event_source_lock(es);
   if (_al_event_source_needs_to_generate_event(es)) {
      ALLEGRO_EVENT event;
      if (xevent->type == FocusOut)
         event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_OUT;
      else
         event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_IN;
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



static void xdpy_set_window_title(ALLEGRO_DISPLAY *display, const char *title)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;

   _al_mutex_lock(&system->lock);
   Atom WM_NAME = XInternAtom(system->x11display, "WM_NAME", False);
   Atom _NET_WM_NAME = XInternAtom(system->x11display, "_NET_WM_NAME", False);
   char *list[] = {(void *)title};
   XTextProperty property;
   Xutf8TextListToTextProperty(system->x11display, list, 1, XUTF8StringStyle,
      &property);
   XSetTextProperty(system->x11display, glx->window, &property, WM_NAME);
   XSetTextProperty(system->x11display, glx->window, &property, _NET_WM_NAME);
   XFree(property.value);
   XClassHint hint;
   hint.res_name = strdup(title); // Leak? (below too...)
   hint.res_class = strdup(title);
   XSetClassHint(system->x11display, glx->window, &hint);
   _al_mutex_unlock(&system->lock);
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
   xdpy_vt.create_sub_bitmap = _al_ogl_create_sub_bitmap;
   xdpy_vt.get_backbuffer = _al_ogl_get_backbuffer;
   xdpy_vt.set_target_bitmap = _al_ogl_set_target_bitmap;
   xdpy_vt.is_compatible_bitmap = xdpy_is_compatible_bitmap;
   xdpy_vt.resize_display = xdpy_resize_display;
   xdpy_vt.set_icons = xdpy_set_icons;
   xdpy_vt.set_window_title = xdpy_set_window_title;
   xdpy_vt.set_window_position = xdpy_set_window_position;
   xdpy_vt.get_window_position = xdpy_get_window_position;
   xdpy_vt.set_display_flag = xdpy_set_display_flag;
   xdpy_vt.wait_for_vsync = xdpy_wait_for_vsync;

   _al_xwin_add_cursor_functions(&xdpy_vt);
   _al_ogl_add_drawing_functions(&xdpy_vt);

   return &xdpy_vt;
}

#define X11_ATOM(x)  XInternAtom(x11, #x, False);

void _al_xglx_set_above(ALLEGRO_DISPLAY *display, int value)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   Display *x11 = system->x11display;

   ALLEGRO_DEBUG("Toggling _NET_WM_STATE_ABOVE hint: %d\n", value);

   XEvent xev;
   xev.xclient.type = ClientMessage;
   xev.xclient.serial = 0;
   xev.xclient.send_event = True;
   xev.xclient.message_type = X11_ATOM(_NET_WM_STATE);
   xev.xclient.window = glx->window;
   xev.xclient.format = 32;

   // Note: It seems 0 is not reliable except when mapping a window -
   // 2 is all we need though.
   xev.xclient.data.l[0] = value; /* 0 = off, 1 = on, 2 = toggle */

   xev.xclient.data.l[1] = X11_ATOM(_NET_WM_STATE_ABOVE);
   xev.xclient.data.l[2] = 0;
   xev.xclient.data.l[3] = 0;
   xev.xclient.data.l[4] = 1;

   XSendEvent(x11, DefaultRootWindow(x11), False,
      SubstructureRedirectMask | SubstructureNotifyMask, &xev);
}

/* vi: set sts=3 sw=3 et: */
