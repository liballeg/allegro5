#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_xglx.h"

ALLEGRO_DEBUG_CHANNEL("display")

/* globals - this might be better in ALLEGRO_SYSTEM_XGLX */
_ALLEGRO_XGLX_MMON_INTERFACE mmon_interface;

/* generic multi-head x */
int _al_xsys_mheadx_get_default_adapter(ALLEGRO_SYSTEM_XGLX *s)
{
   int i;
 
   ALLEGRO_DEBUG("mhead get default adapter\n");
   
   if(ScreenCount(s->x11display) == 1)
      return 0;

   _al_mutex_lock(&s->lock);
   
   Window focus;
   int revert_to = 0;
   XWindowAttributes attr;
   Screen *focus_screen;

   if(!XGetInputFocus(s->x11display, &focus, &revert_to)) {
      ALLEGRO_ERROR("XGetInputFocus failed!");
		_al_mutex_unlock(&s->lock);
      return 0;
   }
   
   if(focus == None) {
      ALLEGRO_ERROR("XGetInputFocus returned None!\n");
		_al_mutex_unlock(&s->lock);
      return 0;
   } else
   if(focus == PointerRoot) {
      ALLEGRO_DEBUG("XGetInputFocus returned PointerRoot.\n");
      /* XXX TEST THIS >:( */
      Window root, child;
      int root_x, root_y;
      int win_x, win_y;
      unsigned int mask;
      
      if(XQueryPointer(s->x11display, focus, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask) == False) {
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
   for(i = 0; i < ScreenCount(s->x11display); i++) {
      if(ScreenOfDisplay(s->x11display, i) == focus_screen) {
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
Window _al_xsys_get_toplevel_parent(ALLEGRO_SYSTEM_XGLX *s, Window window)
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
   
   if(!XGetInputFocus(s->x11display, &focus, &revert_to)) {
      ALLEGRO_ERROR("XGetInputFocus failed!\n");
		_al_mutex_unlock(&s->lock);
      return;
   }
   
   if(focus == None || focus == PointerRoot) {
      ALLEGRO_DEBUG("XGetInputFocus returned special window, selecting default root!\n");
      focus = DefaultRootWindow(s->x11display);
   }
   else {
      /* this horribleness is due to toolkits like GTK (and probably Qt) creating
       * a 1x1 window under the window you're looking at that actually accepts
       * all input, so we need to grab the top level parent window rather than
       * whatever happens to have focus */
   
      focus = _al_xsys_get_toplevel_parent(s, focus);
   }
   
   ALLEGRO_DEBUG("XGetInputFocus returned %i\n", (int)focus);
   
   XWindowAttributes attr;
   
   if(XGetWindowAttributes(s->x11display, focus, &attr) == 0) {
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
   ALLEGRO_DEBUG("xinerama dpy off %ix%i\n", *x, *y);
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

static ALLEGRO_DISPLAY_MODE *_al_xsys_xinerama_get_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter, int i, ALLEGRO_DISPLAY_MODE *mode)
{
   if (adapter < 0 || adapter >= s->xinerama_screen_count)
      return NULL;
   
   if(i != 0)
      return NULL;
   
   mode->width = s->xinerama_screen_info[adapter].width;
   mode->height = s->xinerama_screen_info[adapter].height;
   mode->format = 0;
   mode->refresh_rate = 0;
   
   return mode;
}

static int _al_xsys_xinerama_get_default_adapter(ALLEGRO_SYSTEM_XGLX *s)
{
   int center_x = 0, center_y = 0;
   ALLEGRO_DEBUG("xinerama get default adapter\n");
   
   _al_xsys_get_active_window_center(s, &center_x, &center_y);
   ALLEGRO_DEBUG("xinerama got active center: %ix%i\n", center_x, center_y);
   
   int i;
   for(i = 0; i < s->xinerama_screen_count; i++) {
      if(center_x >= s->xinerama_screen_info[i].x_org && center_x <= s->xinerama_screen_info[i].x_org + s->xinerama_screen_info[i].width &&
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
static int _al_xsys_xinerama_get_xscreen(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   (void)s;
   (void)adapter;
   return 0;
}

#endif /* ALLEGRO_XWINDOWS_WITH_XINERAMA */



/*---------------------------------------------------------------------------
 *
 * RandR
 *
 */

#ifdef ALLEGRO_XWINDOWS_WITH_XRANDR

#if 0
static bool _al_xsys_xrandr_query(ALLEGRO_SYSTEM_XGLX *s)
{
   XRRScreenResources *res = NULL;
   bool ret = 0;
   int xscr = 0;
   int res_idx = 0;
   RRMode *modes = NULL;
   RRMode *set_modes = NULL;
   
   if (s->xrandr_outputs) {
      int i;
      modes = al_calloc(s->xrandr_output_count, sizeof(RRMode));
      if(!modes)
         return false;
      
      set_modes = al_calloc(s->xrandr_output_count, sizeof(RRMode));
      if(!set_modes)
         return false;
      
      for (i = 0; i < s->xrandr_output_count; i++) {
         XRRFreeOutputInfo(s->xrandr_outputs[i]->output);
         s->xrandr_outputs[i]->output = NULL;
         modes[i] = s->xrandr_outputs[i]->mode;
         set_modes[i] = s->xrandr_outputs[i]->set_mode;
      }
      al_free(s->xrandr_outputs);
      s->xrandr_outputs = NULL;
      s->xrandr_output_count = 0;
   }

   if(s->xrandr_res) {
      for(res_idx = 0; res_idx < s->xrandr_res_count; res_idx++) {
         XRRFreeScreenResources(s->xrandr_res[res_idx]);
      }
      free(s->xrandr_res);
   }
   
   /* XXX This causes an annoying flicker with the intel driver and a secondary
    * display (at least) so should be deferred until required.
    */

   s->xrandr_res = al_calloc(ScreenCount(s->x11display), sizeof(*s->xrandr_res));
   if(!s->xrandr_res) {
      ALLEGRO_ERROR("xrandr: failed to allocate array for XRRScreenResources structures.\n");
      return 0;
   }

   s->xrandr_res_count = ScreenCount(s->x11display);
   
   s->xrandr_output_count = 0; // just in case
   for(xscr = 0; xscr < ScreenCount(s->x11display); xscr++) {

      res = s->xrandr_res[xscr] = XRRGetScreenResources (s->x11display, XRootWindow(s->x11display, xscr));
      if (res && res->nmode) {
         
         // XXX this may not behave correctly if there are any cloned outputs
         int ncrtc;
         for (ncrtc = 0; ncrtc < res->ncrtc; ncrtc++) {
            XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(s->x11display, res, res->crtcs[ncrtc]);
            if (!crtc_info) {
               ALLEGRO_WARN("xrandr: failed to fetch crtc %i\n", ncrtc);
               continue;
            }

            ALLEGRO_DEBUG("xrandr: got crtc %d (%i+%i-%ix%i).\n", ncrtc, crtc_info->x, crtc_info->y, crtc_info->width, crtc_info->height);

            if (crtc_info->noutput > 0) {
               struct xrandr_output_s **new_output_info = s->xrandr_outputs;
               int new_output_count = s->xrandr_output_count + crtc_info->noutput;

               new_output_info = al_realloc(new_output_info, sizeof(s->xrandr_outputs) * new_output_count);
               if (new_output_info) {
                  memset(new_output_info+s->xrandr_output_count, 0, sizeof(s->xrandr_outputs)*crtc_info->noutput);

                  int nout;
                  for (nout = 0; nout < crtc_info->noutput; nout++) {
                     XRROutputInfo *info = XRRGetOutputInfo(s->x11display, res, crtc_info->outputs[nout]);
                     if (!info) {
                        ALLEGRO_WARN("xrandr: failed to fetch output %i for crtc %i\n", nout, ncrtc);
                        continue;
                     }

                     ALLEGRO_DEBUG("xrandr: added new output[%d].\n", s->xrandr_output_count + nout);
                     new_output_info[s->xrandr_output_count + nout] = al_calloc(1, sizeof(**s->xrandr_outputs));
                     if(!new_output_info[s->xrandr_output_count + nout]) {
                        ALLEGRO_ERROR("xrandr: failed to allocate output structure.\n");
                        continue;
                     }
                     
                     new_output_info[s->xrandr_output_count + nout]->output = info;
                     new_output_info[s->xrandr_output_count + nout]->res_id = xscr;
                     
                     if(modes) {
                        new_output_info[s->xrandr_output_count + nout]->mode = modes[s->xrandr_output_count + nout];
                     }
                     
                     if(set_modes) {
                        new_output_info[s->xrandr_output_count + nout]->set_mode = set_modes[s->xrandr_output_count + nout];
                     }
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
            
            ret = 1;
            
   #ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
            if (s->xinerama_available && s->xinerama_screen_count != s->xrandr_output_count) {
               ALLEGRO_WARN("XRandR and Xinerama seem to disagree on how many screens there are (%i vs %i), going to ignore XRandR.\n", s->xrandr_output_count, s->xinerama_screen_count);
               // only actually going to ignore the output count, and setting of modes on the extra xinerama screens.
               ret = 0;
            }
   #else
            // XXX verify xrandr isn't borked here because of stupidity
            // XXX  like the nvidia binary driver only implementing XRandR 1.1
   #endif

         }
         else {
            ALLEGRO_WARN("XRandR has no outputs.\n");
            ret = 0;
         }
      }

   }

   ALLEGRO_DEBUG("xrandr: found %d outputs.\n",s->xrandr_output_count);

   al_free(modes);
   al_free(set_modes);
   
   return ret;
}

static XRRModeInfo *_al_xsys_xrandr_fetch_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter, RRMode id)
{
   int i;
   XRRScreenResources *res = s->xrandr_res[s->xrandr_outputs[adapter]->res_id];
   
   for(i = 0; i < res->nmode; i++) {
      if(res->modes[i].id == id)
         return &res->modes[i];
   }
   
   return NULL;
}

// XXX I think this is supposed to re query Xrandr
// XXX  that can wait till Xrandr change events are handled by allegro.
static int _al_xsys_xrandr_get_num_modes(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   if (adapter < 0 || adapter >= s->xrandr_output_count)
      return 0;

   return s->xrandr_outputs[adapter]->output->nmode;
}

static ALLEGRO_DISPLAY_MODE *_al_xsys_xrandr_get_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter, int id, ALLEGRO_DISPLAY_MODE *mode)
{
   XRRModeInfo *mi = NULL;

   if (adapter < 0 || adapter >= s->xrandr_output_count)
      return NULL;
      
   int i;

   /* should we validate res_id here? only way it would be invalid is if the array got stomped on... so a crash would be imminent anyhow */
   XRRScreenResources *res = s->xrandr_res[s->xrandr_outputs[adapter]->res_id];
   
   for (i = 0; i < res->nmode; i++) {
      if (res->modes[i].id == s->xrandr_outputs[adapter]->output->modes[id]) {
         mi = &res->modes[i];
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

   //ALLEGRO_INFO("xrandr: got mode %dx%d@%d\n", mode->width, mode->height, mode->refresh_rate);

   return mode;
}

struct xrr_rect {
   int x1;
   int y1;
   int x2;
   int y2;
};

static void _al_xsys_xrandr_combine_output_rect(struct xrr_rect *r1, XRRCrtcInfo *crtc)
{
   if(r1->x1 > crtc->x)
      r1->x1 = crtc->x;
   
   if(r1->y1 > crtc->y)
      r1->y1 = crtc->y;
   
   if(crtc->x + crtc->width > r1->x2)
      r1->x2 = crtc->x + crtc->width;
   
   if(crtc->y + crtc->height > r1->y2)
      r1->y2 = crtc->y + crtc->height;

}

static bool _al_xsys_xrandr_set_mode(ALLEGRO_SYSTEM_XGLX *s, ALLEGRO_DISPLAY_XGLX *d, int w, int h, int format, int refresh)
{
   int mode_idx = -1;
   XRRModeInfo *mi = NULL;
   XRRScreenResources *res = NULL;
   
   int adapter = _al_xglx_get_adapter(s, d, false);
   if (adapter < 0 || adapter >= s->xrandr_output_count)
      return false;
   
   _al_mutex_lock(&s->lock);

   //XSync(s->x11display, False);
   
   /* if we have already set the mode once, look that up, if not, use the original mode */
   XRRModeInfo *cur_mode = _al_xsys_xrandr_fetch_mode(s, adapter, s->xrandr_outputs[adapter]->set_mode ? s->xrandr_outputs[adapter]->set_mode : s->xrandr_outputs[adapter]->mode);
   int cur_refresh_rate;
   if (cur_mode->hTotal && cur_mode->vTotal)
      cur_refresh_rate = ((float) cur_mode->dotClock / ((float) cur_mode->hTotal * (float) cur_mode->vTotal));
   else
      cur_refresh_rate = 0;
   if ((int)cur_mode->width == w && (int)cur_mode->height == h && (refresh == 0 || cur_refresh_rate == refresh)) {
      ALLEGRO_DEBUG("xrandr: mode already set, we're good to go.\n");
      _al_mutex_unlock(&s->lock);
      return true;
   }
   else {
      ALLEGRO_DEBUG("xrandr: new mode: %dx%d@%d old mode: %dx%d@%d.\n", w,h,refresh, cur_mode->width, cur_mode->height, cur_refresh_rate);
   }
   
   mode_idx = _al_xglx_fullscreen_select_mode(s, adapter, w, h, format, refresh);
   if (mode_idx == -1) {
      ALLEGRO_DEBUG("xrandr: mode %dx%d@%d not found.\n", w,h,refresh);
      _al_mutex_unlock(&s->lock);
      return false;
   }

   res = s->xrandr_res[s->xrandr_outputs[adapter]->res_id];

   int i;
   for (i = 0; i < res->nmode; i++) {
      if (res->modes[i].id == s->xrandr_outputs[adapter]->output->modes[mode_idx]) {
         mi = &res->modes[i];
         break;
      }
   }

   // grab the Crtc info so we can get the info we ARENT changing to pass back to SetCrtcConfig
   XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(s->x11display, res, s->xrandr_outputs[adapter]->output->crtc);

   ALLEGRO_DEBUG("xrandr: set mode %i+%i-%ix%i on adapter %i\n", crtc_info->x, crtc_info->y, w, h, adapter);
   
   int xscr = s->xrandr_outputs[adapter]->res_id;
   
   ALLEGRO_DEBUG("xrandr: screen size: %ix%i\n", DisplayWidth(s->x11display, xscr),
                     DisplayHeight(s->x11display, xscr));
   
   // actually changes the mode, what a name.
   // XXX actually check for failure...
   int ok = XRRSetCrtcConfig(s->x11display, res,
                    s->xrandr_outputs[adapter]->output->crtc,
                    // proves to XRandR that our info is up to date, or it'll just ignore us
                    crtc_info->timestamp,
                    // we don't want to change any of the config prefixed by crtc_info
                    crtc_info->x,
                    crtc_info->y,
                    mi->id, // Set the Mode!
                    crtc_info->rotation,
                    crtc_info->outputs,
                    crtc_info->noutput);
   
   /* now make sure the framebuffer is large enough */
   int output;
   struct xrr_rect rect = { 0, 0, 0, 0 };
   for(output = 0; output < s->xrandr_output_count; output++) {
      XRRCrtcInfo *crtc;
      XRRScreenResources *r = s->xrandr_res[s->xrandr_outputs[output]->res_id];
      if(output != adapter)
         crtc = XRRGetCrtcInfo(s->x11display, r, s->xrandr_outputs[adapter]->output->crtc);
      else
         crtc = crtc_info;
      
      _al_xsys_xrandr_combine_output_rect(&rect, crtc);
      
      XRRFreeCrtcInfo(crtc);
   }

   int new_width = rect.x2 - rect.x1;
   int new_height = rect.y2 - rect.y1;
   
   if(new_width > DisplayWidth(s->x11display, xscr) ||
      new_height > DisplayWidth(s->x11display, xscr))
   {
      XRRSetScreenSize (s->x11display,
                     RootWindow(s->x11display, xscr),
                     new_width, new_height,
                     DisplayWidthMM(s->x11display, xscr),
                     DisplayHeightMM(s->x11display, xscr));
   }
   
   ALLEGRO_DEBUG("xrandr: screen size: %ix%i\n", DisplayWidth(s->x11display, xscr),
                     DisplayHeight(s->x11display, xscr));
   
   //XSync(s->x11display, False);
   
   _al_mutex_unlock(&s->lock);

   if (ok != 0) {
      ALLEGRO_ERROR("XRandR failed to set mode.\n");
      return false;
   }
   
   s->xrandr_outputs[adapter]->set_mode = mi->id;
   
   return true;
}

static void _al_xsys_xrandr_store_modes(ALLEGRO_SYSTEM_XGLX *s)
{
   // XXX this might be better placed in xrandr_query, it'd save a bunch of XRRGetCrtcInfo calls.
   int i;
	
	_al_mutex_lock(&s->lock);
	
   for (i = 0; i < s->xrandr_output_count; i++) {
      XRRScreenResources *res = s->xrandr_res[s->xrandr_outputs[i]->res_id];
      
      XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(s->x11display, res, s->xrandr_outputs[i]->output->crtc);
      ALLEGRO_DEBUG("xrandr: got %i modes.\n",  res->nmode);
      
      int j;
      for (j = 0; j < res->nmode; j++) {
         ALLEGRO_DEBUG("xrandr: modecmp %i == %i.\n", (int)res->modes[j].id, (int)crtc_info->mode);
         if (res->modes[j].id == crtc_info->mode) {
            ALLEGRO_DEBUG("xrandr: matched mode!\n");
            s->xrandr_outputs[i]->mode = res->modes[j].id;
            break;
         }
      }

      XRRFreeCrtcInfo(crtc_info);

      if (!s->xrandr_outputs[i]->mode) {
         ALLEGRO_WARN("XRandR failed to store mode for adapter %d.\n", i);
      }
   }
   
   _al_mutex_unlock(&s->lock);
}

static void _al_xsys_xrandr_restore_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   if (adapter < 0 || adapter >= s->xrandr_output_count)
      return;
   
   XRRModeInfo *mode = _al_xsys_xrandr_fetch_mode(s, adapter, s->xrandr_outputs[adapter]->mode);
   ASSERT(mode);
   ALLEGRO_DEBUG("xfullscreen: _al_xsys_xrandr_restore_mode (%d, %d)\n", mode->width, mode->height);

   XRRScreenResources *res = s->xrandr_res[s->xrandr_outputs[adapter]->res_id];
	
	_al_mutex_lock(&s->lock);
	
   XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(s->x11display, res, s->xrandr_outputs[adapter]->output->crtc);
   if (!crtc_info) {
		_al_mutex_unlock(&s->lock);
      ALLEGRO_ERROR("xfullscreen: XRRGetCrtcInfo failed.\n");
      return;
   }
   
   ALLEGRO_DEBUG("xrandr: restore mode %i+%i-%ix%i on adapter %i\n", crtc_info->x, crtc_info->y, mode->width, mode->height, adapter);
   
   // actually changes the mode, what a name.
   int ok = XRRSetCrtcConfig(s->x11display,
                    res,
                    s->xrandr_outputs[adapter]->output->crtc,
                    // proves to XRandR that our info is up to date, or it'll just ignore us
                    crtc_info->timestamp,
                    // we don't want to change any of the config prefixed by crtc_info
                    crtc_info->x,
                    crtc_info->y,
                    mode->id, // Set the Mode!
                    crtc_info->rotation,
                    crtc_info->outputs,
                    crtc_info->noutput);

   XRRFreeCrtcInfo(crtc_info);
    
   if (ok != 0) {
      ALLEGRO_ERROR("XRandR failed to restore mode.\n");
   }

   XFlush(s->x11display);

   _al_mutex_unlock(&s->lock);
   
   s->xrandr_outputs[adapter]->set_mode = mode->id;
    
}

static void _al_xsys_xrandr_get_display_offset(ALLEGRO_SYSTEM_XGLX *s, int adapter, int *x, int *y)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   //if (s->xinerama_available) {
   //   _al_xsys_xinerama_get_display_offset(s, adapter, x, y);
   //} else
#endif
   {
      XRRScreenResources *res = s->xrandr_res[s->xrandr_outputs[adapter]->res_id];
		
		_al_mutex_lock(&s->lock);
		
      XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(s->x11display, res, s->xrandr_outputs[adapter]->output->crtc);
      if (!crtc_info) {
			_al_mutex_unlock(&s->lock);
         ALLEGRO_ERROR("XRandR failed to get CrtcInfo, can't get display offset.\n");
         return;
      }

		_al_mutex_unlock(&s->lock);
		
      *x = crtc_info->x;
      *y = crtc_info->y;
      ALLEGRO_DEBUG("xrandr: display offset: %ix%i.\n", *x, *y);
      XRRFreeCrtcInfo(crtc_info);
   }
}

static int _al_xsys_xrandr_get_num_adapters(ALLEGRO_SYSTEM_XGLX *s)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
  //if (s->xinerama_available) {
   //   return s->xinerama_screen_count;
   //}
#endif

   return s->xrandr_output_count;
}

static void _al_xsys_xrandr_get_monitor_info(ALLEGRO_SYSTEM_XGLX *s, int adapter, ALLEGRO_MONITOR_INFO *monitor_info)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   //if (s->xinerama_available) {
   //   _al_xsys_xinerama_get_monitor_info(s, adapter, monitor_info);
   //   return;
   //}
#endif

   XRRScreenResources *res = s->xrandr_res[s->xrandr_outputs[adapter]->res_id];
	
	_al_mutex_lock(&s->lock);
	
   XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(s->x11display, res, s->xrandr_outputs[adapter]->output->crtc);
   if (!crtc_info) {
		_al_mutex_unlock(&s->lock);
      ALLEGRO_ERROR("XRandR failed to get CrtcInfo, can't get monitor info.\n");
      return;
   }

	_al_mutex_unlock(&s->lock);

   monitor_info->x1 = crtc_info->x;
   monitor_info->y1 = crtc_info->y;
   monitor_info->x2 = crtc_info->x + crtc_info->width;
   monitor_info->y2 = crtc_info->y + crtc_info->height;

   XRRFreeCrtcInfo(crtc_info);
}

static int _al_xsys_xrandr_get_default_adapter(ALLEGRO_SYSTEM_XGLX *s)
{
   ALLEGRO_DEBUG("xrandr get default adapter\n");
   
   /* if we have more than one res, means we're in hybrid multi-head X + xrandr mode
    * use multi-head X code to get default adapter */
   if(s->xrandr_res_count > 1)
      return _al_xsys_mheadx_get_default_adapter(s);
   
   int center_x = 0, center_y = 0;
   _al_xsys_get_active_window_center(s, &center_x, &center_y);
   
   int i, default_adapter = 0;
	
	_al_mutex_lock(&s->lock);
	
   for(i = 0; i < s->xrandr_output_count; i++) {
      XRRScreenResources *res = s->xrandr_res[s->xrandr_outputs[i]->res_id];
      XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(s->x11display, res, s->xrandr_outputs[i]->output->crtc);
      if(center_x >= (int)crtc_info->x && center_x <= (int)(crtc_info->x + crtc_info->width) &&
         center_y >= (int)crtc_info->y && center_y <= (int)(crtc_info->y + crtc_info->height))
      {
         default_adapter = i;
			break;
      }
   }
   
   _al_mutex_unlock(&s->lock);
	
   ALLEGRO_DEBUG("xrandr selected default: %i\n", default_adapter);
   
   return default_adapter;
}

static int _al_xsys_xrandr_get_xscreen(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   ALLEGRO_DEBUG("xrandr get xscreen for adapter %i\n", adapter);
   if(s->xrandr_res_count > 1)
      return _al_xsys_mheadx_get_xscreen(s, adapter);
   
   ALLEGRO_DEBUG("xrandr selected default xscreen (ScreenCount:%i res_count:%i)\n", ScreenCount(s->x11display), s->xrandr_res_count);
   /* should always only be one X Screen with normal xrandr */
   return 0;
}

static void _al_xsys_xrandr_create_display(ALLEGRO_SYSTEM_XGLX *s, ALLEGRO_DISPLAY_XGLX *d)
{
   int mask = RRScreenChangeNotifyMask | 
              RRCrtcChangeNotifyMask   | 
              RROutputChangeNotifyMask | 
              RROutputPropertyNotifyMask;
   
   XRRSelectInput( s->x11display, d->window, 0);
   XRRSelectInput( s->x11display, d->window, mask); 
}

static void _al_xsys_xrandr_init(ALLEGRO_SYSTEM_XGLX *s)
{
   int event_base = 0;
   int error_base = 0;

   /* init xrandr info to defaults */
   s->xrandr_available = 0;
   s->xrandr_res_count = 0;
   s->xrandr_res = NULL;
   s->xrandr_output_count = 0;
   s->xrandr_outputs = NULL;

   _al_mutex_lock(&s->lock);

   if (XRRQueryExtension(s->x11display, &s->xrandr_event_base, &error_base)) {
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
      mmon_interface.get_default_adapter   = _al_xsys_xrandr_get_default_adapter;
      mmon_interface.get_xscreen           = _al_xsys_xrandr_get_xscreen;
      mmon_interface.create_display        = _al_xsys_xrandr_create_display;
   }

   _al_mutex_unlock(&s->lock);
}

static void _al_xsys_xrandr_exit(ALLEGRO_SYSTEM_XGLX *s)
{
   // int i;
   ALLEGRO_DEBUG("xfullscreen: XRandR exiting.\n");
   
   // for (i = 0; i < s->xrandr_output_count; i++) {
   //   XRRFreeOutputInfo(s->xrandr_outputs[i]);
   // }

   // for (i = 0; i < s->xrandr_res_count; i++) {
   //    XRRFreeScreenResources(s->xrandr_res[i]);
   // }

   ALLEGRO_DEBUG("xfullscreen: XRRFreeScreenResources\n");
   //if (s->xrandr_res)
   //   XRRFreeScreenResources(s->xrandr_res);

   al_free(s->xrandr_outputs);
   al_free(s->xrandr_res);
   
   s->xrandr_available = 0;
   s->xrandr_res_count = 0;
   s->xrandr_res = NULL;
   s->xrandr_output_count = 0;
   s->xrandr_outputs = NULL;

   ALLEGRO_DEBUG("xfullscreen: XRandR exit finished.\n");
}

#endif /* 0 */

void _al_xsys_xrandr_init(ALLEGRO_SYSTEM_XGLX *s);
void _al_xsys_xrandr_exit(ALLEGRO_SYSTEM_XGLX *s);

#endif /* ALLEGRO_XWINDOWS_WITH_XRANDR */



/*---------------------------------------------------------------------------
 *
 * XF86VidMode
 *
 */

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE

// XXX retest under multi-head!
static int _al_xsys_xfvm_get_num_modes(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   if(s->xinerama_available && s->xinerama_screen_count != s->xfvm_screen_count) {
      if(adapter < 0 || adapter > s->xinerama_screen_count)
         return 0;
    
      /* due to braindeadedness of the NVidia binary driver we can't know what an individual
       * monitor's modes are, as the NVidia binary driver only reports combined "BigDesktop"
       * or "TwinView" modes to user-space. There is no way to set modes on individual screens.
       * As such, we can only do one thing here and report one single mode,
       * which will end up being the xinerama size for the requested adapter */
      return 1;
   }
#endif

   if(adapter < 0 || adapter > s->xfvm_screen_count)
      return 0;
   
   return s->xfvm_screen[adapter].mode_count;
}

static ALLEGRO_DISPLAY_MODE *_al_xsys_xfvm_get_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter, int i, ALLEGRO_DISPLAY_MODE *mode)
{
   int denom;

#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   /* TwinView gives us one large screen via xfvm, and no way to
    * properly change modes on individual monitors, so we want to query
    * xinerama for the lone mode. */
   if (s->xinerama_available && s->xfvm_screen_count != s->xinerama_screen_count) {
      return _al_xsys_xinerama_get_mode(s, adapter, i, mode);
   }
#endif

   if(adapter < 0 || adapter > s->xfvm_screen_count)
      return NULL;
   
   if(i < 0 || i > s->xfvm_screen[adapter].mode_count)
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

static bool _al_xsys_xfvm_set_mode(ALLEGRO_SYSTEM_XGLX *s, ALLEGRO_DISPLAY_XGLX *d, int w, int h, int format, int refresh_rate)
{
   int mode_idx = -1;
   int adapter = _al_xglx_get_adapter(s, d, false);
   
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   /* TwinView work arrounds, nothing to do here, since we can't really change or restore modes */
   if (s->xinerama_available && s->xinerama_screen_count != s->xfvm_screen_count) {
      /* at least pretend we set a mode if its the current mode */
      if(s->xinerama_screen_info[adapter].width != w || s->xinerama_screen_info[adapter].height != h)
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

static void _al_xsys_xfvm_store_video_mode(ALLEGRO_SYSTEM_XGLX *s)
{
   int n;

   ALLEGRO_DEBUG("xfullscreen: _al_xsys_xfvm_store_video_mode\n");

#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   /* TwinView work arrounds, nothing to do here, since we can't really change or restore modes */
   if (s->xinerama_available && s->xinerama_screen_count != s->xfvm_screen_count) {
      return;
   }
#endif

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
   
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   /* TwinView work arrounds, nothing to do here, since we can't really change or restore modes */
   if (s->xinerama_available && s->xinerama_screen_count != s->xfvm_screen_count) {
      return;
   }
#endif

   if(adapter < 0 || adapter > s->xfvm_screen_count)
      return;

   ASSERT(s->xfvm_screen[adapter].original_mode);
   ALLEGRO_DEBUG("xfullscreen: _al_xsys_xfvm_restore_video_mode (%d, %d)\n",
      s->xfvm_screen[adapter].original_mode->hdisplay, s->xfvm_screen[adapter].original_mode->vdisplay);

   ok = XF86VidModeSwitchToMode(s->x11display, adapter, s->xfvm_screen[adapter].original_mode);
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
   /* can we move this into shutdown_system? It could speed up mode restores -TF */
   XFlush(s->gfxdisplay);
}

static void _al_xsys_xfvm_get_display_offset(ALLEGRO_SYSTEM_XGLX *s, int adapter, int *x, int *y)
{
   int tmp_x = 0, tmp_y = 0;

#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   if (s->xinerama_available) {
      _al_xsys_xinerama_get_display_offset(s, adapter, &tmp_x, &tmp_y);
   } //else 
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

static int _al_xsys_xfvm_get_num_adapters(ALLEGRO_SYSTEM_XGLX *s)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   if (s->xinerama_available) {
      return s->xinerama_screen_count;
   }
#endif
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

   if(adapter < 0 || adapter > s->xfvm_screen_count)
      return;

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

static int _al_xsys_xfvm_get_default_adapter(ALLEGRO_SYSTEM_XGLX *s)
{
   ALLEGRO_DEBUG("xfvm get default adapter\n");
   
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   if (s->xinerama_available) {
      return _al_xsys_xinerama_get_default_adapter(s);
   }
#endif

   return _al_xsys_mheadx_get_default_adapter(s);
}

static int _al_xsys_xfvm_get_xscreen(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   ALLEGRO_DEBUG("xfvm get xscreen for adapter %i\n", adapter);
   
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   if (s->xinerama_available) {
      return _al_xsys_xinerama_get_xscreen(s, adapter);
   }
#endif

   return _al_xsys_mheadx_get_xscreen(s, adapter);
}

static void _al_xsys_xfvm_post_setup(ALLEGRO_SYSTEM_XGLX *s,
   ALLEGRO_DISPLAY_XGLX *d)
{
   int x = 0, y = 0;
   XWindowAttributes xwa;
   
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   /* TwinView work arrounds, nothing to do here, since we can't really change or restore modes */
   if (s->xinerama_available && s->xinerama_screen_count != s->xfvm_screen_count) {
      return;
   }
#endif
   
   int adapter = _al_xglx_get_adapter(s, d, false);
   
   XGetWindowAttributes(s->x11display, d->window, &xwa);
   _al_xsys_xfvm_get_display_offset(s, adapter, &x, &y);
   
   /* some window managers like to move our window even if we explicitly tell it not to
    * so we need to get the correct offset here */
   x = xwa.x - x;
   y = xwa.y - y;
   
   ALLEGRO_DEBUG("xfvm set view port: %ix%i\n", x, y);
   
   XF86VidModeSetViewPort(s->x11display, adapter, x, y);
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
         mmon_interface.get_default_adapter   = _al_xsys_xfvm_get_default_adapter;
         mmon_interface.get_xscreen           = _al_xsys_xfvm_get_xscreen;
         mmon_interface.post_setup            = _al_xsys_xfvm_post_setup;
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
   /* nope, no way to tell which is going to be used on any given system
    * this way, xrandr always overrides everything else should it succeed.
    * And when xfvm is chosen, it needs xinerama inited,
    * incase there are multiple screens.
    */

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

int _al_xglx_get_num_display_modes(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   if (!init_mmon_interface(s))
      return 0;

   if (adapter == -1)
      adapter = _al_xglx_get_default_adapter(s);

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
      adapter = _al_xglx_get_default_adapter(s);

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

   if (!mmon_interface.set_mode)
      return false;

   return mmon_interface.set_mode(s, d, w, h, format, refresh_rate);
}

void _al_xglx_fullscreen_to_display(ALLEGRO_SYSTEM_XGLX *s,
   ALLEGRO_DISPLAY_XGLX *d)
{
   /* First, make sure the mouse stays inside the window. */
   /* XXX We don't want to do it this way, for say a dual monitor setup... */
   //XGrabPointer(s->x11display, d->window, False,
   //   PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
   //   GrabModeAsync, GrabModeAsync, d->window, None, CurrentTime);
   //FIXME: handle possible errors here
   s->pointer_grabbed = true;
   
   if (!init_mmon_interface(s))
      return;
   
   if (!mmon_interface.post_setup)
      return;
   
   mmon_interface.post_setup(s, d);
   
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

int _al_xglx_get_default_adapter(ALLEGRO_SYSTEM_XGLX *s)
{
   ALLEGRO_DEBUG("get default adapter\n");
   
   if (!init_mmon_interface(s))
      return 0;

   if (!mmon_interface.get_default_adapter)
      return 0;

   return mmon_interface.get_default_adapter(s);
}

int _al_xglx_get_xscreen(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   ALLEGRO_DEBUG("get xscreen\n");
   
   if (!init_mmon_interface(s))
      return 0;

   if (!mmon_interface.get_xscreen)
      return 0;
   
   return mmon_interface.get_xscreen(s, adapter);
}

int _al_xglx_get_adapter(ALLEGRO_SYSTEM_XGLX *s, ALLEGRO_DISPLAY_XGLX *d, bool recalc)
{
   if (!init_mmon_interface(s))
      return 0;
      
   if(d->adapter != -1 && !recalc)
      return d->adapter;
   
   if(!mmon_interface.get_adapter)
      return 0;
   
   return mmon_interface.get_adapter(s, d);
}

void _al_xglx_handle_xevent(ALLEGRO_SYSTEM_XGLX *s, ALLEGRO_DISPLAY_XGLX *d, XEvent *e)
{
   ALLEGRO_DEBUG("got event %i\n", e->type);
   // if we haven't setup the mmon interface, just bail
   if(!s->mmon_interface_inited)
      return;
   
   // bail if the current mmon interface doesn't implement the handle_xevent method
   if(!mmon_interface.handle_xevent)
      return;
   
   
   mmon_interface.handle_xevent(s, d, e);

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

   XSendEvent(x11, RootWindowOfScreen(ScreenOfDisplay(x11, glx->xscreen)), False,
      SubstructureRedirectMask | SubstructureNotifyMask, &xev);
   
   if (value == 2) {
      /* Only wait for a resize if toggling. */
      _al_display_xglx_await_resize(display, old_resize_count, true);
   }
}

void _al_xglx_toggle_above(ALLEGRO_DISPLAY *display, int value)
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

/* vim: set sts=3 sw=3 et: */
