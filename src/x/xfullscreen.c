#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_xglx.h"

ALLEGRO_DEBUG_CHANNEL("display")

typedef struct _ALLEGRO_XGLX_MMON_INTERFACE _ALLEGRO_XGLX_MMON_INTERFACE;

struct _ALLEGRO_XGLX_MMON_INTERFACE {
    int (*get_num_display_modes)(ALLEGRO_SYSTEM_XGLX *s, int adapter);
    ALLEGRO_DISPLAY_MODE *(*get_display_mode)(ALLEGRO_SYSTEM_XGLX *s, int, int, ALLEGRO_DISPLAY_MODE*);
    bool (*set_mode)(ALLEGRO_SYSTEM_XGLX *, ALLEGRO_DISPLAY_XGLX *, int, int, int, int);
    void (*store_mode)(ALLEGRO_SYSTEM_XGLX *);
    void (*restore_mode)(ALLEGRO_SYSTEM_XGLX *, int);
    void (*get_display_offset)(ALLEGRO_SYSTEM_XGLX *, int, int *, int *);
    int (*get_num_adapters)(ALLEGRO_SYSTEM_XGLX *);
    void (*get_monitor_info)(ALLEGRO_SYSTEM_XGLX *, int, ALLEGRO_MONITOR_INFO *);
};


/* globals - this might be better in ALLEGRO_SYSTEM_XGLX */
static _ALLEGRO_XGLX_MMON_INTERFACE mmon_interface;



/*---------------------------------------------------------------------------
 *
 * Xinerama
 *
 */

#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA

static void _al_xsys_xinerama_init(ALLEGRO_SYSTEM_XGLX *s)
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

static void _al_xsys_xinerama_exit(ALLEGRO_SYSTEM_XGLX *s)
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

static void _al_xsys_xinerama_get_display_offset(ALLEGRO_SYSTEM_XGLX *s, int adapter, int *x, int *y)
{
   ALLEGRO_ASSERT(adapter >= 0 && adapter < s->xinerama_screen_count);
   *x = s->xinerama_screen_info[adapter].x_org;
   *y = s->xinerama_screen_info[adapter].y_org;
}

static void _al_xsys_xinerama_get_monitor_info(ALLEGRO_SYSTEM_XGLX *s, int adapter, ALLEGRO_MONITOR_INFO *mi)
{
   if (adapter < 0 || adapter >= s->xinerama_screen_count)
      return;
   
   mi->x1 = s->xinerama_screen_info[adapter].x_org;
   mi->y1 = s->xinerama_screen_info[adapter].y_org;
   mi->x2 = mi->x1 + s->xinerama_screen_info[adapter].width;
   mi->y2 = mi->y1 + s->xinerama_screen_info[adapter].height;
}

#endif /* ALLEGRO_XWINDOWS_WITH_XINERAMA */



/*---------------------------------------------------------------------------
 *
 * RandR
 *
 */

#ifdef ALLEGRO_XWINDOWS_WITH_XRANDR

static bool _al_xsys_xrandr_query(ALLEGRO_SYSTEM_XGLX *s)
{
   XRRScreenResources *res = NULL;
   bool ret = 0;

   if (s->xrandr_res) {
      XRRFreeScreenResources(s->xrandr_res);
   }

   if (s->xrandr_outputs) {
      int i;
      for (i = 0; i < s->xrandr_output_count; i++) {
         XRRFreeOutputInfo(s->xrandr_outputs[i]);
         s->xrandr_outputs[i] = NULL;
      }
      al_free(s->xrandr_outputs);
      s->xrandr_outputs = NULL;
      s->xrandr_output_count = 0;
   }

   /* XXX This causes an annoying flicker with the intel driver and a secondary
    * display (at least) so should be deferred until required.
    */
   res = s->xrandr_res = XRRGetScreenResources (s->x11display, XRootWindow(s->x11display, DefaultScreen(s->x11display)));
   if (res && res->nmode) {
      s->xrandr_output_count = 0; // just in case

      // XXX this may not behave correctly if there are any cloned outputs
      int ncrtc;
      for (ncrtc = 0; ncrtc < res->ncrtc; ncrtc++) {
         XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(s->x11display, res, res->crtcs[ncrtc]);
         if (!crtc_info) {
            ALLEGRO_WARN("xrandr: failed to fetch crtc %i\n", ncrtc);
            continue;
         }

         ALLEGRO_DEBUG("xrandr: got crtc %d.\n", ncrtc);

         if (crtc_info->noutput > 0) {
            XRROutputInfo **new_output_info = s->xrandr_outputs;
            int new_output_count = s->xrandr_output_count + crtc_info->noutput;

            new_output_info = al_realloc(new_output_info, sizeof(XRROutputInfo *) * new_output_count);
            if (new_output_info) {
               memset(new_output_info+s->xrandr_output_count, 0, sizeof(XRROutputInfo*)*crtc_info->noutput);

               int nout;
               for (nout = 0; nout < crtc_info->noutput; nout++) {
                  XRROutputInfo *info = XRRGetOutputInfo(s->x11display, res, crtc_info->outputs[nout]);
                  if (!info) {
                     ALLEGRO_WARN("xrandr: failed to fetch output %i for crtc %i\n", nout, ncrtc);
                     continue;
                  }

                  ALLEGRO_DEBUG("xrandr: added new output[%d].\n", s->xrandr_output_count + nout);
                  new_output_info[s->xrandr_output_count + nout] = info;
               }

               s->xrandr_outputs = new_output_info;
               s->xrandr_output_count = new_output_count;

            }
            else {
               ALLEGRO_ERROR("xrandr: failed to allocate array for output structures.\n");
               continue;
            }
         }

         XRRFreeCrtcInfo(crtc_info);
      }

      if (s->xrandr_output_count > 0) {
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
         if (s->xinerama_available && s->xinerama_screen_count != s->xrandr_output_count) {
            ALLEGRO_WARN("XRandR and Xinerama seem to disagree on how many screens there are (%i vs %i), going to ignore XRandR.\n", s->xrandr_output_count, s->xinerama_screen_count);
            // only actually going to ignore the output count, and setting of modes on the extra xinerama screens.
         }
#else
         // XXX verify xrandr isn't borked here because of stupidity
         // XXX  like the nvidia binary driver only implementing XRandR 1.1
#endif
         ALLEGRO_DEBUG("xrandr: found %d outputs.\n",s->xrandr_output_count);
         ret = 1;
      }
      else {
         ALLEGRO_WARN("XRandR has no outputs.\n");
         ret = 0;
      }
   }

   return ret;
}

// XXX I think this is supposed to re query Xrandr
// XXX  that can wait till Xrandr change events are handled by allegro.
static int _al_xsys_xrandr_get_num_modes(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   if (adapter < 0 || adapter >= s->xrandr_output_count)
      return 0;

   return s->xrandr_outputs[adapter]->nmode;
}

static ALLEGRO_DISPLAY_MODE *_al_xsys_xrandr_get_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter, int id, ALLEGRO_DISPLAY_MODE *mode)
{
   XRRModeInfo *mi = NULL;

   if (adapter < 0 || adapter >= s->xrandr_output_count)
      return NULL;
      
   int i;
   for (i = 0; i < s->xrandr_res->nmode; i++) {
      if (s->xrandr_res->modes[i].id == s->xrandr_outputs[adapter]->modes[id]) {
         mi = &s->xrandr_res->modes[i];
         break;
      }
   }

   if (!mi)
      return NULL;

   mode->width = mi->width;
   mode->height = mi->height;
   mode->format = 0;
   if (mi->hTotal && mi->vTotal) {
      mode->refresh_rate = ((float) mi->dotClock / ((float) mi->hTotal * (float) mi->vTotal));
   }
   else {
      mode->refresh_rate = 0;
   }

   ALLEGRO_INFO("XRandR got mode: %dx%d@%d\n", mode->width, mode->height, mode->refresh_rate);

   return mode;
}

static bool _al_xsys_xrandr_set_mode(ALLEGRO_SYSTEM_XGLX *s, ALLEGRO_DISPLAY_XGLX *d, int w, int h, int format, int refresh)
{
   int mode_idx = -1;
   XRRModeInfo *mi = NULL;

   if (d->xscreen < 0 || d->xscreen >= s->xrandr_output_count)
      return false;

   XRRModeInfo *cur_mode = s->xrandr_stored_modes[d->xscreen];
   int cur_refresh_rate;
   if (cur_mode->hTotal && cur_mode->vTotal)
      cur_refresh_rate = ((float) cur_mode->dotClock / ((float) cur_mode->hTotal * (float) cur_mode->vTotal));
   else
      cur_refresh_rate = 0;
   if ((int)cur_mode->width == w && (int)cur_mode->height == h && cur_refresh_rate == refresh) {
      ALLEGRO_DEBUG("xrandr: mode already set, we're good to go.\n");
      return true;
   }
   else {
      ALLEGRO_DEBUG("xrandr: new mode: %dx%d@%d old mode: %dx%d@%d.\n", w,h,refresh, cur_mode->width, cur_mode->height, cur_refresh_rate);
   }
   
   mode_idx = _al_xglx_fullscreen_select_mode(s, d->xscreen, w, h, format, refresh);
   if (mode_idx == -1) {
      ALLEGRO_DEBUG("xrandr: mode %dx%d@%d not found.\n", w,h,refresh);
      return false;
   }

   mi = &s->xrandr_res->modes[mode_idx];

   _al_mutex_lock(&s->lock);

   // grab the Crtc info so we can get the info we ARENT changing to pass back to SetCrtcConfig
   XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(s->x11display, s->xrandr_res, s->xrandr_outputs[d->xscreen]->crtc);

   // actually changes the mode, what a name.
   // XXX actually check for failure...
   int ok = XRRSetCrtcConfig(s->x11display, s->xrandr_res,
                    s->xrandr_outputs[d->xscreen]->crtc,
                    // proves to XRandR that our info is up to date, or it'll just ignore us
                    crtc_info->timestamp,
                    // we don't want to change any of the config prefixed by crtc_info
                    crtc_info->x,
                    crtc_info->y,
                    mi->id, // Set the Mode!
                    crtc_info->rotation,
                    crtc_info->outputs,
                    crtc_info->noutput);

   XRRFreeCrtcInfo(crtc_info);

   XFlush(s->x11display);
   _al_mutex_unlock(&s->lock);

   if (ok != 0) {
      ALLEGRO_ERROR("XRandR failed to set mode.\n");
      return false;
   }

   return true;
}

static void _al_xsys_xrandr_store_modes(ALLEGRO_SYSTEM_XGLX *s)
{
   al_free(s->xrandr_stored_modes);
   s->xrandr_stored_modes = al_calloc(s->xrandr_output_count, sizeof(XRRModeInfo*));
   if (!s->xrandr_stored_modes) {
      ALLEGRO_ERROR("XRandR failed to allocate memory for stored modes array.\n");
      return;
   }

   // XXX this might be better placed in xrandr_query, it'd save a bunch of XRRGetCrtcInfo calls.
   int i;
   for (i = 0; i < s->xrandr_output_count; i++) {
      XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(s->x11display, s->xrandr_res, s->xrandr_outputs[i]->crtc);

      int j;
      for (j = 0; j < s->xrandr_res->nmode; j++) {
         if (s->xrandr_res->modes[j].id == crtc_info->mode) {
            s->xrandr_stored_modes[i] = &s->xrandr_res->modes[j];
         }
      }

      XRRFreeCrtcInfo(crtc_info);

      if (!s->xrandr_stored_modes[i]) {
         ALLEGRO_WARN("XRandR failed to store mode for adapter %d.\n", i);
      }
   }
}

static void _al_xsys_xrandr_restore_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   if (adapter < 0 || adapter >= s->xrandr_output_count)
      return;
   
   ASSERT(s->xrandr_stored_modes[adapter]);
   ALLEGRO_DEBUG("xfullscreen: _al_xsys_xrandr_restore_mode (%d, %d)\n",
                 s->xrandr_stored_modes[adapter]->width, s->xrandr_stored_modes[adapter]->height);

   XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(s->x11display, s->xrandr_res, s->xrandr_outputs[adapter]->crtc);
   if (!crtc_info) {
      ALLEGRO_ERROR("xfullscreen: XRRGetCrtcInfo failed.\n");
      return;
   }

   if (s->xrandr_stored_modes[adapter]->width == crtc_info->width &&
      s->xrandr_stored_modes[adapter]->height == crtc_info->height) {
      ALLEGRO_INFO("xfullscreen: mode already restored.\n");
      return;
   }
   // actually changes the mode, what a name.
   int ok = XRRSetCrtcConfig(s->x11display,
                    s->xrandr_res,
                    s->xrandr_outputs[adapter]->crtc,
                    // proves to XRandR that our info is up to date, or it'll just ignore us
                    crtc_info->timestamp,
                    // we don't want to change any of the config prefixed by crtc_info
                    crtc_info->x,
                    crtc_info->y,
                    s->xrandr_stored_modes[adapter]->id, // Set the Mode!
                    crtc_info->rotation,
                    crtc_info->outputs,
                    crtc_info->noutput);

   XRRFreeCrtcInfo(crtc_info);
    
   if (ok != 0) {
      ALLEGRO_ERROR("XRandR failed to restore mode.\n");
   }

   XFlush(s->x11display);
}

static void _al_xsys_xrandr_get_display_offset(ALLEGRO_SYSTEM_XGLX *s, int adapter, int *x, int *y)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   if (s->xinerama_available) {
      _al_xsys_xinerama_get_display_offset(s, adapter, x, y);
   } else
#endif
   {
      XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(s->x11display, s->xrandr_res, s->xrandr_outputs[adapter]->crtc);
      if (!crtc_info) {
         ALLEGRO_ERROR("XRandR failed to get CrtcInfo, can't get display offset.\n");
         return;
      }

      *x = crtc_info->x;
      *y = crtc_info->y;
      XRRFreeCrtcInfo(crtc_info);
   }
}

static int _al_xsys_xrandr_get_num_adapters(ALLEGRO_SYSTEM_XGLX *s)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   if (s->xinerama_available) {
      return s->xinerama_screen_count;
   }
#endif

   return s->xrandr_output_count;
}

static void _al_xsys_xrandr_get_monitor_info(ALLEGRO_SYSTEM_XGLX *s, int adapter, ALLEGRO_MONITOR_INFO *monitor_info)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   if (s->xinerama_available) {
      _al_xsys_xinerama_get_monitor_info(s, adapter, monitor_info);
      return;
   }
#endif

   XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(s->x11display, s->xrandr_res, s->xrandr_outputs[adapter]->crtc);
   if (!crtc_info) {
      ALLEGRO_ERROR("XRandR failed to get CrtcInfo, can't get monitor info.\n");
      return;
   }

   monitor_info->x1 = crtc_info->x;
   monitor_info->y1 = crtc_info->y;
   monitor_info->x2 = crtc_info->x + crtc_info->width;
   monitor_info->y2 = crtc_info->y + crtc_info->height;

   XRRFreeCrtcInfo(crtc_info);
}

static void _al_xsys_xrandr_init(ALLEGRO_SYSTEM_XGLX *s)
{
   int event_base = 0;
   int error_base = 0;

   /* init xrandr info to defaults */
   s->xrandr_available = 0;
   s->xrandr_output_count = 0;
   s->xrandr_outputs = NULL;

   _al_mutex_lock(&s->lock);

   if (XRRQueryExtension(s->x11display, &event_base, &error_base)) {
      int minor_version = 0, major_version = 0;
      int status = XRRQueryVersion(s->x11display, &major_version, &minor_version);
      ALLEGRO_INFO("XRandR version: %i.%i\n", major_version, minor_version);

      if (!status) {
         ALLEGRO_WARN("XRandR not available, XRRQueryVersion failed.\n");
      }
      else if (major_version == 1 && minor_version < 2) {
         /* this is unlikely to happen, unless the user has an ancient Xorg,
         Xorg will just emulate the latest version and return that
         instead of what the driver actually supports, stupid. */
         ALLEGRO_WARN("XRandR not available, unsupported version: %i.%i\n", major_version, minor_version);
      }
      else {
         bool ret = _al_xsys_xrandr_query(s);
         if (ret) {
            ALLEGRO_INFO("XRandR is active\n");
            s->xrandr_available = 1;
         }
         else {
            ALLEGRO_INFO("XRandR is not active\n");
         }
      }
   }
   else {
      ALLEGRO_WARN("XRandR extension is not available.\n");
   }

   if (s->xrandr_available) {
      mmon_interface.get_num_display_modes = _al_xsys_xrandr_get_num_modes;
      mmon_interface.get_display_mode      = _al_xsys_xrandr_get_mode;
      mmon_interface.set_mode              = _al_xsys_xrandr_set_mode;
      mmon_interface.store_mode            = _al_xsys_xrandr_store_modes;
      mmon_interface.restore_mode          = _al_xsys_xrandr_restore_mode;
      mmon_interface.get_display_offset    = _al_xsys_xrandr_get_display_offset;
      mmon_interface.get_num_adapters      = _al_xsys_xrandr_get_num_adapters;
      mmon_interface.get_monitor_info      = _al_xsys_xrandr_get_monitor_info;
   }

   _al_mutex_unlock(&s->lock);
}

static void _al_xsys_xrandr_exit(ALLEGRO_SYSTEM_XGLX *s)
{
   int i;
   ALLEGRO_DEBUG("xfullscreen: XRandR exiting.\n");

   for (i = 0; i < s->xrandr_output_count; i++) {
   //   XRRFreeOutputInfo(s->xrandr_outputs[i]);
   }

   ALLEGRO_DEBUG("xfullscreen: XRRFreeScreenResources\n");
   //if (s->xrandr_res)
   //   XRRFreeScreenResources(s->xrandr_res);

   al_free(s->xrandr_outputs);
   al_free(s->xrandr_stored_modes);

   s->xrandr_available = 0;
   s->xrandr_output_count = 0;
   s->xrandr_outputs = NULL;

   ALLEGRO_DEBUG("xfullscreen: XRandR exit finished.\n");
}

#endif /* ALLEGRO_XWINDOWS_WITH_XRANDR */



/*---------------------------------------------------------------------------
 *
 * XF86VidMode
 *
 */

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE

// XXX This code is completely untested. Preferably it should be tested on a machine that doesn't have any XRandR support.
static int _al_xsys_xfvm_get_num_modes(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   return s->xfvm_screen[adapter].mode_count;
}

static ALLEGRO_DISPLAY_MODE *_al_xsys_xfvm_get_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter, int i, ALLEGRO_DISPLAY_MODE *mode)
{
   int denom;

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

static bool _al_xsys_xfvm_set_mode(ALLEGRO_SYSTEM_XGLX *s, ALLEGRO_DISPLAY_XGLX *d, int w, int h, int format, int refresh_rate)
{
   int mode_idx = -1;

   mode_idx = _al_xglx_fullscreen_select_mode(s, d->xscreen, w, h, format, refresh_rate);
   if (mode_idx == -1)
      return false;

   if (!XF86VidModeSwitchToMode(s->gfxdisplay, d->xscreen, s->xfvm_screen[d->xscreen].modes[mode_idx])) {
      ALLEGRO_ERROR("xfullscreen: XF86VidModeSwitchToMode failed\n");
      return false;
   }

   return true;
}

/* no longer used, remove later when tested
static void _al_xsys_xfvm_fullscreen_to_display(ALLEGRO_SYSTEM_XGLX *s,
   ALLEGRO_DISPLAY_XGLX *d)
{
   int x, y;
   Window child;
   XWindowAttributes xwa;



   // grab the root window that contains d->window
   XGetXwindowAttributes(s->gfxdisplay, d->window, &xwa);
   XTranslateCoordinates(s->gfxdisplay, d->window,
      xwa.root, 0, 0, &x, &y, &child);

   XF86VidModeSetViewPort(s->gfxdisplay, d->xscreen, x, y);
}
*/

static void _al_xsys_xfvm_store_video_mode(ALLEGRO_SYSTEM_XGLX *s)
{
   int n;

   ALLEGRO_DEBUG("xfullscreen: _al_xsys_xfvm_store_video_mode\n");

   // save all original modes
   int i;
   for (i = 0; i < s->xfvm_screen_count; i++) {
      n = _al_xsys_xfvm_get_num_modes(s, i);
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

static void _al_xsys_xfvm_restore_video_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   Bool ok;

   ASSERT(s->xfvm_screen[adapter].original_mode);
   ALLEGRO_DEBUG("xfullscreen: _al_xsys_xfvm_restore_video_mode (%d, %d)\n",
      s->xfvm_screen[adapter].original_mode->hdisplay, s->xfvm_screen[adapter].original_mode->vdisplay);

   ok = XF86VidModeSwitchToMode(s->gfxdisplay, adapter, s->xfvm_screen[adapter].original_mode);
   if (!ok) {
      ALLEGRO_ERROR("xfullscreen: XF86VidModeSwitchToMode failed\n");
   }

   if (s->pointer_grabbed) {
      XUngrabPointer(s->gfxdisplay, CurrentTime);
      s->pointer_grabbed = false;
   }

   /* This is needed, at least on my machine, or the program may terminate
    * before the screen mode is actually reset. --pw
    */
   XFlush(s->gfxdisplay);
}

/* XXX this might replace fullscreen_to_display alltogeter */
static void _al_xsys_xfvm_get_display_offset(ALLEGRO_SYSTEM_XGLX *s, int adapter, int *x, int *y)
{
   int tmp_x = 0, tmp_y = 0;

#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   if (s->xinerama_available) {
      _al_xsys_xinerama_get_display_offset(s, adapter, &tmp_x, &tmp_y);
   } else
#endif
   /* don't set the output params if function fails */
   if (!XF86VidModeGetViewPort(s->x11display, adapter, &tmp_x, &tmp_y))
      return;

   *x = tmp_x;
   *y = tmp_y;
}

static int _al_xsys_xfvm_get_num_adapters(ALLEGRO_SYSTEM_XGLX *s)
{
   return s->xfvm_screen_count;
}

static void _al_xsys_xfvm_get_monitor_info(ALLEGRO_SYSTEM_XGLX *s, int adapter, ALLEGRO_MONITOR_INFO *mi)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   if (s->xinerama_available) {
      _al_xsys_xinerama_get_monitor_info(s, adapter, mi);
      return;
   }
#endif

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
}

static void _al_xsys_xfvm_init(ALLEGRO_SYSTEM_XGLX *s)
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
      // This is some fun stuff right here, if XRANDR is available, we can't use the xinerama screen count
      // and we really want the xrandr init in xglx_initialize to come last so it overrides xf86vm if available
      // and I really don't want to add more XRRQuery* or #ifdefs here.
      // I don't think XRandR can be disabled once its loaded,
      // so just seeing if its in the extension list should be fine.
      int ext_op, ext_evt, ext_err;
      Bool ext_ret = XQueryExtension(s->x11display, "RANDR", &ext_op, &ext_evt, &ext_err);

      if (s->xinerama_available && ext_ret == False) {
         num_screens = s->xinerama_screen_count;
      } else
#endif
      num_screens = ScreenCount(s->x11display);

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

         mmon_interface.get_num_display_modes = _al_xsys_xfvm_get_num_modes;
         mmon_interface.get_display_mode      = _al_xsys_xfvm_get_mode;
         mmon_interface.set_mode              = _al_xsys_xfvm_set_mode;
         mmon_interface.store_mode            = _al_xsys_xfvm_store_video_mode;
         mmon_interface.restore_mode          = _al_xsys_xfvm_restore_video_mode;
         mmon_interface.get_display_offset    = _al_xsys_xfvm_get_display_offset;
         mmon_interface.get_num_adapters      = _al_xsys_xfvm_get_num_adapters;
         mmon_interface.get_monitor_info      = _al_xsys_xfvm_get_monitor_info;
      }
   }

   _al_mutex_unlock(&s->lock);
}

static void _al_xsys_xfvm_exit(ALLEGRO_SYSTEM_XGLX *s)
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

#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   _al_xsys_xinerama_init(s);
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   _al_xsys_xfvm_init(s);
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_XRANDR
   _al_xsys_xrandr_init(s);
#endif

   if (mmon_interface.store_mode)
      mmon_interface.store_mode(s);

   s->mmon_interface_inited = true;

   return true;
}

void _al_xsys_mmon_exit(ALLEGRO_SYSTEM_XGLX *s)
{
   if (!s->mmon_interface_inited)
      return;

#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   _al_xsys_xinerama_exit(s);
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   _al_xsys_xfvm_exit(s);
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_XRANDR
   _al_xsys_xrandr_exit(s);
#endif

   s->mmon_interface_inited = false;
}

static int get_default_adapter(void)
{
   /* FIXME: the default adapter is not necessarily 0. */
   return 0;
}

int _al_xglx_get_num_display_modes(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   if (!init_mmon_interface(s))
      return 0;

   if (adapter == -1)
      adapter = get_default_adapter();

   if (!mmon_interface.get_num_display_modes) {
      if (adapter != 0)
         return 0;

      return 1;
   }

   return mmon_interface.get_num_display_modes(s, adapter);
}

ALLEGRO_DISPLAY_MODE *_al_xglx_get_display_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter, int index,
   ALLEGRO_DISPLAY_MODE *mode)
{
   if (!init_mmon_interface(s))
      return NULL;

   if (adapter == -1)
      adapter = get_default_adapter();

   if (!mmon_interface.get_display_mode) {
      mode->width = DisplayWidth(s->x11display, DefaultScreen(s->x11display));
      mode->height = DisplayHeight(s->x11display, DefaultScreen(s->x11display));
      mode->format = 0;
      mode->refresh_rate = 0;
      return NULL;
   }

   return mmon_interface.get_display_mode(s, adapter, index, mode);
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
   
   if (adapter == -1)
      adapter = get_default_adapter();

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

   if (!mmon_interface.set_mode)
      return false;

   return mmon_interface.set_mode(s, d, w, h, format, refresh_rate);
}

void _al_xglx_fullscreen_to_display(ALLEGRO_SYSTEM_XGLX *s,
   ALLEGRO_DISPLAY_XGLX *d)
{
   /* First, make sure the mouse stays inside the window. */
   XGrabPointer(s->gfxdisplay, d->window, False,
      PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
      GrabModeAsync, GrabModeAsync, d->window, None, CurrentTime);
   //FIXME: handle possible errors here
   s->pointer_grabbed = true;
}

void _al_xglx_store_video_mode(ALLEGRO_SYSTEM_XGLX *s)
{
   if (!init_mmon_interface(s))
      return;

   if (!mmon_interface.store_mode)
      return;

   mmon_interface.store_mode(s);
}

void _al_xglx_restore_video_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   if (!init_mmon_interface(s))
      return;

   if (!mmon_interface.restore_mode)
      return;

   mmon_interface.restore_mode(s, adapter);
}

void _al_xglx_get_display_offset(ALLEGRO_SYSTEM_XGLX *s, int adapter, int *x, int *y)
{
   if (!init_mmon_interface(s))
      return;

   if (!mmon_interface.get_display_offset)
      return;

   mmon_interface.get_display_offset(s, adapter, x, y);
}

void _al_xglx_get_monitor_info(ALLEGRO_SYSTEM_XGLX *s, int adapter, ALLEGRO_MONITOR_INFO *info)
{
   if (!init_mmon_interface(s))
      return;

   if (!mmon_interface.get_monitor_info) {
      _al_mutex_lock(&s->lock);
      info->x1 = 0;
      info->y2 = 0;
      info->x2 = DisplayWidth(s->x11display, DefaultScreen(s->x11display));
      info->y2 = DisplayHeight(s->x11display, DefaultScreen(s->x11display));
      _al_mutex_unlock(&s->lock);
      return;
   }

   mmon_interface.get_monitor_info(s, adapter, info);
}

int _al_xglx_get_num_video_adapters(ALLEGRO_SYSTEM_XGLX *s)
{
   if (!init_mmon_interface(s))
      return 0;

   if (!mmon_interface.get_num_adapters)
      return 1;

   return mmon_interface.get_num_adapters(s);
}



#define X11_ATOM(x)  XInternAtom(x11, #x, False);

/* Note: The system mutex must be locked (exactly once) before
 * calling this as we call _al_display_xglx_await_resize.
 */
void _al_xglx_toggle_fullscreen_window(ALLEGRO_DISPLAY *display, int value)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   Display *x11 = system->x11display;
   int old_resize_count = glx->resize_count;

   ALLEGRO_DEBUG("Toggling _NET_WM_STATE_FULLSCREEN hint: %d\n", value);

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

   xev.xclient.data.l[1] = X11_ATOM(_NET_WM_STATE_FULLSCREEN);
   xev.xclient.data.l[2] = 0;
   xev.xclient.data.l[3] = 0;
   xev.xclient.data.l[4] = 1;

   XSendEvent(x11, DefaultRootWindow(x11), False,
      SubstructureRedirectMask | SubstructureNotifyMask, &xev);
   
   if (value == 2) {
      /* Only wait for a resize if toggling. */
      _al_display_xglx_await_resize(display, old_resize_count, true);
   }
}

/* vim: set sts=3 sw=3 et: */
