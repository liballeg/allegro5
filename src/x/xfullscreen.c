#include <X11/Xlib.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xdisplay.h"
#include "allegro5/internal/aintern_xfullscreen.h"
#include "allegro5/internal/aintern_xsystem.h"

ALLEGRO_DEBUG_CHANNEL("display")

/* globals - this might be better in ALLEGRO_SYSTEM_XGLX */
_ALLEGRO_XGLX_MMON_INTERFACE _al_xglx_mmon_interface;

/* generic multi-head x */
int _al_xsys_mheadx_get_default_adapter(ALLEGRO_SYSTEM_XGLX *s)
{
   int i;
 
   ALLEGRO_DEBUG("mhead get default adapter\n");
   
   if (ScreenCount(s->x11display) == 1)
      return 0;

   _al_mutex_lock(&s->lock);
   
   Window focus;
   int revert_to = 0;
   XWindowAttributes attr;
   Screen *focus_screen;

   if (!XGetInputFocus(s->x11display, &focus, &revert_to)) {
      ALLEGRO_ERROR("XGetInputFocus failed!");
      _al_mutex_unlock(&s->lock);
      return 0;
   }
   
   if (focus == None) {
      ALLEGRO_ERROR("XGetInputFocus returned None!\n");
      _al_mutex_unlock(&s->lock);
      return 0;
   }
   else if (focus == PointerRoot) {
      ALLEGRO_DEBUG("XGetInputFocus returned PointerRoot.\n");
      /* XXX TEST THIS >:( */
      Window root, child;
      int root_x, root_y;
      int win_x, win_y;
      unsigned int mask;
      
      if (XQueryPointer(s->x11display, focus, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask) == False) {
         ALLEGRO_ERROR("XQueryPointer failed :(");
         _al_mutex_unlock(&s->lock);
         return 0;
      }
      
      focus = root;
   }
   else {
      ALLEGRO_DEBUG("XGetInputFocus returned %i!\n", (int)focus);
   }
   
   XGetWindowAttributes(s->x11display, focus, &attr);
   focus_screen = attr.screen;
   
   int ret = 0;
   for (i = 0; i < ScreenCount(s->x11display); i++) {
      if (ScreenOfDisplay(s->x11display, i) == focus_screen) {
         _al_mutex_unlock(&s->lock);
         ret = i;
         break;
      }
   }
   
   _al_mutex_unlock(&s->lock);
   return ret;
}

/* in pure multi-head mode, allegro's virtual adapters map directly to X Screens. */
int _al_xsys_mheadx_get_xscreen(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   (void)s;
   ALLEGRO_DEBUG("mhead get screen %i\n", adapter);
   return adapter;
}

/*
Returns the parent window of "window" (i.e. the ancestor of window
that is a direct child of the root, or window itself if it is a direct child).
If window is the root window, returns window.
*/
static Window get_toplevel_parent(ALLEGRO_SYSTEM_XGLX *s, Window window)
{
   Window parent;
   Window root;
   Window * children;
   unsigned int num_children;

   while (1) {
      /* XXX enlightenment shows some very strange errors here,
       * for some reason 'window' isn't valid when the mouse happens
       * to be over the windeco when this is called.. */
      if (0 == XQueryTree(s->x11display, window, &root, &parent, &children, &num_children)) {
         ALLEGRO_ERROR("XQueryTree error\n");
         return None;
      }
      if (children) { /* must test for NULL */
         XFree(children);
      }
      if (window == root || parent == root) {
         return window;
      }
      else {
         window = parent;
      }
   }
   
   return None;
}

/* used for xinerama and pure xrandr modes */
void _al_xsys_get_active_window_center(ALLEGRO_SYSTEM_XGLX *s, int *x, int *y)
{
   Window focus;
   int revert_to = 0;
   
   _al_mutex_lock(&s->lock);
   
   if (!XGetInputFocus(s->x11display, &focus, &revert_to)) {
      ALLEGRO_ERROR("XGetInputFocus failed!\n");
      _al_mutex_unlock(&s->lock);
      return;
   }
   
   if (focus == None || focus == PointerRoot) {
      ALLEGRO_DEBUG("XGetInputFocus returned special window, selecting default root!\n");
      focus = DefaultRootWindow(s->x11display);
   }
   else {
      /* this horribleness is due to toolkits like GTK (and probably Qt) creating
       * a 1x1 window under the window you're looking at that actually accepts
       * all input, so we need to grab the top level parent window rather than
       * whatever happens to have focus */
   
      focus = get_toplevel_parent(s, focus);
   }
   
   ALLEGRO_DEBUG("XGetInputFocus returned %i\n", (int)focus);
   
   XWindowAttributes attr;
   
   if (XGetWindowAttributes(s->x11display, focus, &attr) == 0) {
      ALLEGRO_ERROR("XGetWindowAttributes failed :(\n");
      _al_mutex_unlock(&s->lock);
      return;
   }

   _al_mutex_unlock(&s->lock);
   
   /* check the center of the window with focus
    * might be a bit more useful than just checking the top left */
   ALLEGRO_DEBUG("focus geom: %ix%i %ix%i\n", attr.x, attr.y, attr.width, attr.height);
   *x = (attr.x + (attr.x + attr.width)) / 2;
   *y = (attr.y + (attr.y + attr.height)) / 2;  
}

/*---------------------------------------------------------------------------
 *
 * Xinerama
 *
 */

#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA

static void xinerama_init(ALLEGRO_SYSTEM_XGLX *s)
{
   int event_base = 0;
   int error_base = 0;

   /* init xinerama info to defaults */
   s->xinerama_available = 0;
   s->xinerama_screen_count = 0;
   s->xinerama_screen_info = NULL;

   _al_mutex_lock(&s->lock);

   if (XineramaQueryExtension(s->x11display, &event_base, &error_base)) {
      int minor_version = 0, major_version = 0;
      int status = XineramaQueryVersion(s->x11display, &major_version, &minor_version);
      ALLEGRO_INFO("Xinerama version: %i.%i\n", major_version, minor_version);

      if (status && !XineramaIsActive(s->x11display)) {
         ALLEGRO_WARN("Xinerama is not active\n");
      }
      else {
         s->xinerama_screen_info = XineramaQueryScreens(s->x11display, &s->xinerama_screen_count);
         if (!s->xinerama_screen_info) {
            ALLEGRO_ERROR("Xinerama failed to query screens.\n");
         }
         else {
            s->xinerama_available = 1;
            ALLEGRO_INFO("Xinerama is active\n");
         }
      }
   }

   if (!s->xinerama_available) {
      ALLEGRO_WARN("Xinerama extension is not available.\n");
   }

   _al_mutex_unlock(&s->lock);
}

static void xinerama_exit(ALLEGRO_SYSTEM_XGLX *s)
{
   if (!s->xinerama_available)
      return;

   ALLEGRO_DEBUG("xfullscreen: xinerama exit.\n");
   if (s->xinerama_screen_info)
      XFree(s->xinerama_screen_info);

   s->xinerama_available = 0;
   s->xinerama_screen_count = 0;
   s->xinerama_screen_info = NULL;
}

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE

static void xinerama_get_display_offset(ALLEGRO_SYSTEM_XGLX *s, int adapter, int *x, int *y)
{
   ALLEGRO_ASSERT(adapter >= 0 && adapter < s->xinerama_screen_count);
   *x = s->xinerama_screen_info[adapter].x_org;
   *y = s->xinerama_screen_info[adapter].y_org;
   ALLEGRO_DEBUG("xinerama dpy off %ix%i\n", *x, *y);
}

static bool xinerama_get_monitor_info(ALLEGRO_SYSTEM_XGLX *s, int adapter, ALLEGRO_MONITOR_INFO *mi)
{
   if (adapter < 0 || adapter >= s->xinerama_screen_count)
      return false;

   mi->x1 = s->xinerama_screen_info[adapter].x_org;
   mi->y1 = s->xinerama_screen_info[adapter].y_org;
   mi->x2 = mi->x1 + s->xinerama_screen_info[adapter].width;
   mi->y2 = mi->y1 + s->xinerama_screen_info[adapter].height;
   return true;
}

static ALLEGRO_DISPLAY_MODE *xinerama_get_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter, int i, ALLEGRO_DISPLAY_MODE *mode)
{
   if (adapter < 0 || adapter >= s->xinerama_screen_count)
      return NULL;
   
   if (i != 0)
      return NULL;
   
   mode->width = s->xinerama_screen_info[adapter].width;
   mode->height = s->xinerama_screen_info[adapter].height;
   mode->format = 0;
   mode->refresh_rate = 0;
   
   return mode;
}

static int xinerama_get_default_adapter(ALLEGRO_SYSTEM_XGLX *s)
{
   int center_x = 0, center_y = 0;
   ALLEGRO_DEBUG("xinerama get default adapter\n");
   
   _al_xsys_get_active_window_center(s, &center_x, &center_y);
   ALLEGRO_DEBUG("xinerama got active center: %ix%i\n", center_x, center_y);
   
   int i;
   for (i = 0; i < s->xinerama_screen_count; i++) {
      if (center_x >= s->xinerama_screen_info[i].x_org && center_x <= s->xinerama_screen_info[i].x_org + s->xinerama_screen_info[i].width &&
         center_y >= s->xinerama_screen_info[i].y_org && center_y <= s->xinerama_screen_info[i].y_org + s->xinerama_screen_info[i].height)
      {
         ALLEGRO_DEBUG("center is inside (%i) %ix%i %ix%i\n", i, s->xinerama_screen_info[i].x_org, s->xinerama_screen_info[i].y_org, s->xinerama_screen_info[i].width, s->xinerama_screen_info[i].height);
         return i;
      }
   }
   
   ALLEGRO_DEBUG("xinerama returning default 0\n");
   return 0;
}

/* similar to multi-head x, but theres only one X Screen, so we return 0 always */
static int xinerama_get_xscreen(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   (void)s;
   (void)adapter;
   return 0;
}

#endif /* ALLEGRO_XWINDOWS_WITH_XF86VIDMODE */

#endif /* ALLEGRO_XWINDOWS_WITH_XINERAMA */



/*---------------------------------------------------------------------------
 *
 * XF86VidMode
 *
 */

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE

// XXX retest under multi-head!
static int xfvm_get_num_modes(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   if (s->xinerama_available && s->xinerama_screen_count != s->xfvm_screen_count) {
      if (adapter < 0 || adapter > s->xinerama_screen_count)
         return 0;
    
      /* due to braindeadedness of the NVidia binary driver we can't know what an individual
       * monitor's modes are, as the NVidia binary driver only reports combined "BigDesktop"
       * or "TwinView" modes to user-space. There is no way to set modes on individual screens.
       * As such, we can only do one thing here and report one single mode,
       * which will end up being the xinerama size for the requested adapter */
      return 1;
   }
#endif

   if (adapter < 0 || adapter > s->xfvm_screen_count)
      return 0;
   
   return s->xfvm_screen[adapter].mode_count;
}

static ALLEGRO_DISPLAY_MODE *xfvm_get_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter, int i, ALLEGRO_DISPLAY_MODE *mode)
{
   int denom;

#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   /* TwinView gives us one large screen via xfvm, and no way to
    * properly change modes on individual monitors, so we want to query
    * xinerama for the lone mode. */
   if (s->xinerama_available && s->xfvm_screen_count != s->xinerama_screen_count) {
      return xinerama_get_mode(s, adapter, i, mode);
   }
#endif

   if (adapter < 0 || adapter > s->xfvm_screen_count)
      return NULL;
   
   if (i < 0 || i > s->xfvm_screen[adapter].mode_count)
      return NULL;
   
   mode->width = s->xfvm_screen[adapter].modes[i]->hdisplay;
   mode->height = s->xfvm_screen[adapter].modes[i]->vdisplay;
   mode->format = 0;
   denom = s->xfvm_screen[adapter].modes[i]->htotal * s->xfvm_screen[adapter].modes[i]->vtotal;
   if (denom > 0)
      mode->refresh_rate = s->xfvm_screen[adapter].modes[i]->dotclock * 1000L / denom;
   else
      mode->refresh_rate = 0;

   return mode;
}

static bool xfvm_set_mode(ALLEGRO_SYSTEM_XGLX *s, ALLEGRO_DISPLAY_XGLX *d, int w, int h, int format, int refresh_rate)
{
   int mode_idx = -1;
   int adapter = _al_xglx_get_adapter(s, d, false);
   
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   /* TwinView workarounds, nothing to do here, since we can't really change or restore modes */
   if (s->xinerama_available && s->xinerama_screen_count != s->xfvm_screen_count) {
      /* at least pretend we set a mode if its the current mode */
      if (s->xinerama_screen_info[adapter].width != w || s->xinerama_screen_info[adapter].height != h)
         return false;
      
      return true;
   }
#endif

   mode_idx = _al_xglx_fullscreen_select_mode(s, adapter, w, h, format, refresh_rate);
   if (mode_idx == -1)
      return false;
   
   if (!XF86VidModeSwitchToMode(s->x11display, adapter, s->xfvm_screen[adapter].modes[mode_idx])) {
      ALLEGRO_ERROR("xfullscreen: XF86VidModeSwitchToMode failed\n");
      return false;
   }

   return true;
}

static void xfvm_store_video_mode(ALLEGRO_SYSTEM_XGLX *s)
{
   int n;

   ALLEGRO_DEBUG("xfullscreen: xfvm_store_video_mode\n");

#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   /* TwinView workarounds, nothing to do here, since we can't really change or restore modes */
   if (s->xinerama_available && s->xinerama_screen_count != s->xfvm_screen_count) {
      return;
   }
#endif

   // save all original modes
   int i;
   for (i = 0; i < s->xfvm_screen_count; i++) {
      n = xfvm_get_num_modes(s, i);
      if (n == 0) {
         /* XXX what to do here? */
         continue;
      }

      s->xfvm_screen[i].original_mode = s->xfvm_screen[i].modes[0];

      int j;
      for (j = 0; j <  s->xfvm_screen[i].mode_count; j++) {
         ALLEGRO_DEBUG("xfvm: screen[%d] mode[%d] = (%d, %d)\n",
            i, j, s->xfvm_screen[i].modes[j]->hdisplay, s->xfvm_screen[i].modes[j]->vdisplay);
      }
      ALLEGRO_INFO("xfvm: screen[%d] original mode = (%d, %d)\n",
         i, s->xfvm_screen[i].original_mode->hdisplay, s->xfvm_screen[i].original_mode->vdisplay);
   }
}

static void xfvm_restore_video_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   Bool ok;
   
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   /* TwinView workarounds, nothing to do here, since we can't really change or restore modes */
   if (s->xinerama_available && s->xinerama_screen_count != s->xfvm_screen_count) {
      return;
   }
#endif

   if (adapter < 0 || adapter > s->xfvm_screen_count)
      return;

   ASSERT(s->xfvm_screen[adapter].original_mode);
   ALLEGRO_DEBUG("xfullscreen: xfvm_restore_video_mode (%d, %d)\n",
      s->xfvm_screen[adapter].original_mode->hdisplay, s->xfvm_screen[adapter].original_mode->vdisplay);

   ok = XF86VidModeSwitchToMode(s->x11display, adapter, s->xfvm_screen[adapter].original_mode);
   if (!ok) {
      ALLEGRO_ERROR("xfullscreen: XF86VidModeSwitchToMode failed\n");
   }

   if (s->mouse_grab_display) {
      XUngrabPointer(s->gfxdisplay, CurrentTime);
      s->mouse_grab_display = NULL;
   }

   /* This is needed, at least on my machine, or the program may terminate
    * before the screen mode is actually reset. --pw
    */
   /* can we move this into shutdown_system? It could speed up mode restores -TF */
   XFlush(s->gfxdisplay);
}

static void xfvm_get_display_offset(ALLEGRO_SYSTEM_XGLX *s, int adapter, int *x, int *y)
{
   int tmp_x = 0, tmp_y = 0;

#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   if (s->xinerama_available) {
      xinerama_get_display_offset(s, adapter, &tmp_x, &tmp_y);
   } //else 
#else
   (void)s;
   (void)adapter;
#endif
   /* don't set the output params if function fails */
   /* XXX I don't think this part makes sense at all.
    * in multi-head mode, the origin is always 0x0
    * in Xinerama, its caught by xinerama, and xfvm is NEVER
    * used when xrandr is active -TF */
   //if (!XF86VidModeGetViewPort(s->x11display, adapter, &tmp_x, &tmp_y))
   //   return;

   *x = tmp_x;
   *y = tmp_y;
   
   ALLEGRO_DEBUG("xfvm dpy off %ix%i\n", *x, *y);
}

static int xfvm_get_num_adapters(ALLEGRO_SYSTEM_XGLX *s)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   if (s->xinerama_available) {
      return s->xinerama_screen_count;
   }
#endif
   return s->xfvm_screen_count;
}

static bool xfvm_get_monitor_info(ALLEGRO_SYSTEM_XGLX *s, int adapter, ALLEGRO_MONITOR_INFO *mi)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   if (s->xinerama_available) {
      return xinerama_get_monitor_info(s, adapter, mi);
   }
#endif

   if (adapter < 0 || adapter > s->xfvm_screen_count)
      return false;

   XWindowAttributes xwa;
   Window root;

   _al_mutex_lock(&s->lock);
   root = RootWindow(s->x11display, adapter);
   XGetWindowAttributes(s->x11display, root, &xwa);
   _al_mutex_unlock(&s->lock);

   /* under plain X, each screen has its own origin,
      and theres no way to figure out orientation
      or relative position */
   mi->x1 = 0;
   mi->y1 = 0;
   mi->x2 = xwa.width;
   mi->y2 = xwa.height;
   return true;
}

static int xfvm_get_default_adapter(ALLEGRO_SYSTEM_XGLX *s)
{
   ALLEGRO_DEBUG("xfvm get default adapter\n");
   
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   if (s->xinerama_available) {
      return xinerama_get_default_adapter(s);
   }
#endif

   return _al_xsys_mheadx_get_default_adapter(s);
}

static int xfvm_get_xscreen(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   ALLEGRO_DEBUG("xfvm get xscreen for adapter %i\n", adapter);
   
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   if (s->xinerama_available) {
      return xinerama_get_xscreen(s, adapter);
   }
#endif

   return _al_xsys_mheadx_get_xscreen(s, adapter);
}

static void xfvm_post_setup(ALLEGRO_SYSTEM_XGLX *s,
   ALLEGRO_DISPLAY_XGLX *d)
{
   int x = 0, y = 0;
   XWindowAttributes xwa;
   
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   /* TwinView workarounds, nothing to do here, since we can't really change or restore modes */
   if (s->xinerama_available && s->xinerama_screen_count != s->xfvm_screen_count) {
      return;
   }
#endif
   
   int adapter = _al_xglx_get_adapter(s, d, false);
   
   XGetWindowAttributes(s->x11display, d->window, &xwa);
   xfvm_get_display_offset(s, adapter, &x, &y);
   
   /* some window managers like to move our window even if we explicitly tell it not to
    * so we need to get the correct offset here */
   x = xwa.x - x;
   y = xwa.y - y;
   
   ALLEGRO_DEBUG("xfvm set view port: %ix%i\n", x, y);
   
   XF86VidModeSetViewPort(s->x11display, adapter, x, y);
}


static void xfvm_init(ALLEGRO_SYSTEM_XGLX *s)
{
   int event_base = 0;
   int error_base = 0;

   /* init xfvm info to defaults */
   s->xfvm_available = 0;
   s->xfvm_screen_count = 0;
   s->xfvm_screen = NULL;

   _al_mutex_lock(&s->lock);

   if (XF86VidModeQueryExtension(s->x11display, &event_base, &error_base)) {
      int minor_version = 0, major_version = 0;
      int status = XF86VidModeQueryVersion(s->x11display, &major_version, &minor_version);
      ALLEGRO_INFO("XF86VidMode version: %i.%i\n", major_version, minor_version);

      if (!status) {
         ALLEGRO_WARN("XF86VidMode not available, XF86VidModeQueryVersion failed.\n");
      }
      else {
         // I don't actually know what versions are required here, just going to assume any is ok for now.
         ALLEGRO_INFO("XF86VidMode %i.%i is active\n", major_version, minor_version);
         s->xfvm_available = 1;
      }
   }
   else {
      ALLEGRO_WARN("XF86VidMode extension is not available.\n");
   }

   if (s->xfvm_available) {
      int num_screens;
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
      /* This is some fun stuff right here, if XRANDR is available, we can't use the xinerama screen count
       * and we really want the xrandr init in xglx_initialize to come last so it overrides xf86vm if available
       * and I really don't want to add more XRRQuery* or #ifdefs here.
       * I don't think XRandR can be disabled once its loaded,
       * so just seeing if its in the extension list should be fine. */
      /* interesting thing to note is if XRandR is available, that means we have TwinView,
       * and not multi-head Xinerama mode, as True Xinerama disables XRandR on my NVidia.
       * which means all of those xfvm_screen_count != xinerama_screen_count tests
       * only apply to TwinView. As this code below sets xfvm_screen_count to xinerama_screen_count
       * making all those compares fail, and make us fall back to the normal xfvm multi-head code. */
      /* second note, if FakeXinerama is disabled on TwinView setups, we will end up using
       * XRandR, as there is no other way to detect TwinView outside of libNVCtrl */
      
      int ext_op, ext_evt, ext_err;
      Bool ext_ret = XQueryExtension(s->x11display, "RANDR", &ext_op, &ext_evt, &ext_err);

      if (s->xinerama_available && ext_ret == False) {
         num_screens = s->xinerama_screen_count;
      }
      else
#endif
      {
         num_screens = ScreenCount(s->x11display);
      }

      ALLEGRO_DEBUG("XF86VidMode Got %d screens.\n", num_screens);
      s->xfvm_screen_count = num_screens;

      s->xfvm_screen = al_calloc(num_screens, sizeof(*s->xfvm_screen));
      if (!s->xfvm_screen) {
         ALLEGRO_ERROR("XF86VidMode: failed to allocate screen array.\n");
         s->xfvm_available = 0;
      }
      else {
         int i;
         for (i = 0; i < num_screens; i++) {
            ALLEGRO_DEBUG("XF86VidMode GetAllModeLines on screen %d.\n", i);
            if (!XF86VidModeGetAllModeLines(s->x11display, i, &(s->xfvm_screen[i].mode_count), &(s->xfvm_screen[i].modes))) {
               /* XXX what to do here? */
            }
         }

         _al_xglx_mmon_interface.get_num_display_modes = xfvm_get_num_modes;
         _al_xglx_mmon_interface.get_display_mode      = xfvm_get_mode;
         _al_xglx_mmon_interface.set_mode              = xfvm_set_mode;
         _al_xglx_mmon_interface.store_mode            = xfvm_store_video_mode;
         _al_xglx_mmon_interface.restore_mode          = xfvm_restore_video_mode;
         _al_xglx_mmon_interface.get_display_offset    = xfvm_get_display_offset;
         _al_xglx_mmon_interface.get_num_adapters      = xfvm_get_num_adapters;
         _al_xglx_mmon_interface.get_monitor_info      = xfvm_get_monitor_info;
         _al_xglx_mmon_interface.get_default_adapter   = xfvm_get_default_adapter;
         _al_xglx_mmon_interface.get_xscreen           = xfvm_get_xscreen;
         _al_xglx_mmon_interface.post_setup            = xfvm_post_setup;
      }
   }

   _al_mutex_unlock(&s->lock);
}

static void xfvm_exit(ALLEGRO_SYSTEM_XGLX *s)
{
   int adapter;
   ALLEGRO_DEBUG("xfullscreen: XFVM exit\n");

   for (adapter = 0; adapter < s->xfvm_screen_count; adapter++) {
      if (s->xfvm_screen[adapter].mode_count > 0) {
         int i;
         for (i = 0; i < s->xfvm_screen[adapter].mode_count; i++) {
            if (s->xfvm_screen[adapter].modes[i]->privsize > 0) {
               //XFree(s->xfvm_screen[adapter].modes[i]->private);
            }
         }
         //XFree(s->xfvm_screen[adapter].modes);
      }

      s->xfvm_screen[adapter].mode_count = 0;
      s->xfvm_screen[adapter].modes = NULL;
      s->xfvm_screen[adapter].original_mode = NULL;

      ALLEGRO_DEBUG("xfullscreen: XFVM freed adapter %d.\n", adapter);
   }

   al_free(s->xfvm_screen);
   s->xfvm_screen = NULL;
}

#endif /* ALLEGRO_XWINDOWS_WITH_XF86VIDMODE */



/*---------------------------------------------------------------------------
 *
 * Generic multi-monitor interface
 *
 */

static bool init_mmon_interface(ALLEGRO_SYSTEM_XGLX *s)
{
   if (s->x11display == NULL) {
      ALLEGRO_WARN("Not connected to X server.\n");
      return false;
   }

   if (s->mmon_interface_inited)
      return true;

   /* Shouldn't we avoid initing any more of these than we need? */
   /* nope, no way to tell which is going to be used on any given system
    * this way, xrandr always overrides everything else should it succeed.
    * And when xfvm is chosen, it needs xinerama inited,
    * incase there are multiple screens.
    */

#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   xinerama_init(s);
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   xfvm_init(s);
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_XRANDR
   _al_xsys_xrandr_init(s);
#endif

   if (_al_xglx_mmon_interface.store_mode)
      _al_xglx_mmon_interface.store_mode(s);

   s->mmon_interface_inited = true;

   return true;
}

void _al_xsys_mmon_exit(ALLEGRO_SYSTEM_XGLX *s)
{
   if (!s->mmon_interface_inited)
      return;

#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   xinerama_exit(s);
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   xfvm_exit(s);
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_XRANDR
   _al_xsys_xrandr_exit(s);
#endif

   s->mmon_interface_inited = false;
}

int _al_xglx_get_num_display_modes(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   if (!init_mmon_interface(s))
      return 0;

   if (adapter < 0)
      adapter = _al_xglx_get_default_adapter(s);

   if (!_al_xglx_mmon_interface.get_num_display_modes) {
      if (adapter != 0)
         return 0;

      return 1;
   }

   return _al_xglx_mmon_interface.get_num_display_modes(s, adapter);
}

ALLEGRO_DISPLAY_MODE *_al_xglx_get_display_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter, int index,
   ALLEGRO_DISPLAY_MODE *mode)
{
   if (!init_mmon_interface(s))
      return NULL;

   if (adapter < 0)
      adapter = _al_xglx_get_default_adapter(s);

   if (!_al_xglx_mmon_interface.get_display_mode) {
      mode->width = DisplayWidth(s->x11display, DefaultScreen(s->x11display));
      mode->height = DisplayHeight(s->x11display, DefaultScreen(s->x11display));
      mode->format = 0;
      mode->refresh_rate = 0;
      return NULL;
   }

   return _al_xglx_mmon_interface.get_display_mode(s, adapter, index, mode);
}

int _al_xglx_fullscreen_select_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter, int w, int h, int format, int refresh_rate)
{
   /* GCC complains about mode being uninitialized without this: */
   ALLEGRO_DISPLAY_MODE mode = mode;
   ALLEGRO_DISPLAY_MODE mode2;
   int i;
   int n;

   if (!init_mmon_interface(s))
      return -1;
   
   if (adapter < 0)
      adapter = _al_xglx_get_default_adapter(s);

   n = _al_xglx_get_num_display_modes(s, adapter);
   if (!n)
      return -1;

   /* Find all modes with correct parameters. */
   int possible_modes[n];
   int possible_count = 0;
   for (i = 0; i < n; i++) {
      if (!_al_xglx_get_display_mode(s, adapter, i, &mode)) {
         continue;
      }
      if (mode.width == w && mode.height == h &&
         (format == 0 || mode.format == format) &&
         (refresh_rate == 0 || mode.refresh_rate == refresh_rate))
      {
         possible_modes[possible_count++] = i;
      }
   }
   if (!possible_count)
      return -1;

   /* Choose mode with highest refresh rate. */
   int best_mode = possible_modes[0];
   _al_xglx_get_display_mode(s, adapter, best_mode, &mode);
   for (i = 1; i < possible_count; i++) {
      if (!_al_xglx_get_display_mode(s, adapter, possible_modes[i], &mode2))  {
         continue;
      }
      if (mode2.refresh_rate > mode.refresh_rate) {
         mode = mode2;
         best_mode = possible_modes[i];
      }
   }

   ALLEGRO_INFO("best mode [%d] = (%d, %d)\n", best_mode, mode.width, mode.height);

   return best_mode;
}

bool _al_xglx_fullscreen_set_mode(ALLEGRO_SYSTEM_XGLX *s,
   ALLEGRO_DISPLAY_XGLX *d, int w, int h, int format, int refresh_rate)
{
   if (!init_mmon_interface(s))
      return false;

   if (!_al_xglx_mmon_interface.set_mode)
      return false;

   return _al_xglx_mmon_interface.set_mode(s, d, w, h, format, refresh_rate);
}

void _al_xglx_fullscreen_to_display(ALLEGRO_SYSTEM_XGLX *s,
   ALLEGRO_DISPLAY_XGLX *d)
{
   if (!init_mmon_interface(s))
      return;
   
   if (!_al_xglx_mmon_interface.post_setup)
      return;
   
   _al_xglx_mmon_interface.post_setup(s, d);
   
}

void _al_xglx_store_video_mode(ALLEGRO_SYSTEM_XGLX *s)
{
   if (!init_mmon_interface(s))
      return;

   if (!_al_xglx_mmon_interface.store_mode)
      return;

   _al_xglx_mmon_interface.store_mode(s);
}

void _al_xglx_restore_video_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   if (!init_mmon_interface(s))
      return;

   if (!_al_xglx_mmon_interface.restore_mode)
      return;

   _al_xglx_mmon_interface.restore_mode(s, adapter);
}

void _al_xglx_get_display_offset(ALLEGRO_SYSTEM_XGLX *s, int adapter, int *x, int *y)
{
   if (!init_mmon_interface(s))
      return;

   if (!_al_xglx_mmon_interface.get_display_offset)
      return;

   _al_xglx_mmon_interface.get_display_offset(s, adapter, x, y);
}

bool _al_xglx_get_monitor_info(ALLEGRO_SYSTEM_XGLX *s, int adapter, ALLEGRO_MONITOR_INFO *info)
{
   if (!init_mmon_interface(s))
      return false;

   if (!_al_xglx_mmon_interface.get_monitor_info) {
      _al_mutex_lock(&s->lock);
      info->x1 = 0;
      info->y1 = 0;
      info->x2 = DisplayWidth(s->x11display, DefaultScreen(s->x11display));
      info->y2 = DisplayHeight(s->x11display, DefaultScreen(s->x11display));
      _al_mutex_unlock(&s->lock);
      return true;
   }

   return _al_xglx_mmon_interface.get_monitor_info(s, adapter, info);
}

int _al_xglx_get_num_video_adapters(ALLEGRO_SYSTEM_XGLX *s)
{
   if (!init_mmon_interface(s))
      return 0;

   if (!_al_xglx_mmon_interface.get_num_adapters)
      return 1;

   return _al_xglx_mmon_interface.get_num_adapters(s);
}

int _al_xglx_get_default_adapter(ALLEGRO_SYSTEM_XGLX *s)
{
   ALLEGRO_DEBUG("get default adapter\n");
   
   if (!init_mmon_interface(s))
      return 0;

   if (!_al_xglx_mmon_interface.get_default_adapter)
      return 0;

   return _al_xglx_mmon_interface.get_default_adapter(s);
}

int _al_xglx_get_xscreen(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   ALLEGRO_DEBUG("get xscreen\n");
   
   if (!init_mmon_interface(s))
      return 0;

   if (!_al_xglx_mmon_interface.get_xscreen)
      return 0;
   
   return _al_xglx_mmon_interface.get_xscreen(s, adapter);
}

int _al_xglx_get_adapter(ALLEGRO_SYSTEM_XGLX *s, ALLEGRO_DISPLAY_XGLX *d, bool recalc)
{
   if (!init_mmon_interface(s))
      return 0;
      
   if (d->adapter >= 0 && !recalc)
      return d->adapter;
   
   if (!_al_xglx_mmon_interface.get_adapter)
      return 0;
   
   return _al_xglx_mmon_interface.get_adapter(s, d);
}

void _al_xglx_handle_mmon_event(ALLEGRO_SYSTEM_XGLX *s, ALLEGRO_DISPLAY_XGLX *d, XEvent *e)
{
   ALLEGRO_DEBUG("got event %i\n", e->type);
   // if we haven't setup the mmon interface, just bail
   if (!s->mmon_interface_inited)
      return;
   
   // bail if the current mmon interface doesn't implement the handle_xevent method
   if (!_al_xglx_mmon_interface.handle_xevent)
      return;
   
   _al_xglx_mmon_interface.handle_xevent(s, d, e);
}

/* vim: set sts=3 sw=3 et: */
