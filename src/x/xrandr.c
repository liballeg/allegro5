#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xdisplay.h"
#include "allegro5/internal/aintern_xfullscreen.h"
#include "allegro5/internal/aintern_xsystem.h"

#ifdef ALLEGRO_XWINDOWS_WITH_XRANDR

ALLEGRO_DEBUG_CHANNEL("xrandr")

typedef struct xrandr_screen xrandr_screen;
typedef struct xrandr_crtc xrandr_crtc;
typedef struct xrandr_output xrandr_output;
typedef struct xrandr_mode xrandr_mode;

struct xrandr_screen {
   int id;
   Time timestamp;
   Time configTimestamp;
   _AL_VECTOR crtcs; // xrandr_crtc
   _AL_VECTOR outputs; // xrandr_output
   _AL_VECTOR modes; // xrandr_mode
   
   XRRScreenResources *res;
};

enum xrandr_crtc_position {
   CRTC_POS_NONE = 0,
   CRTC_POS_ABOVE,
   CRTC_POS_LEFTOF,
   CRTC_POS_BELOW,
   CRTC_POS_RIGHTOF
};

struct xrandr_crtc {
   RRCrtc id;
   Time timestamp;
   int x, y;
   unsigned int width, height;
   RRMode mode;
   Rotation rotation;
   _AL_VECTOR connected;
   _AL_VECTOR possible;
   
   RRMode original_mode;
   int original_xoff;
   int original_yoff;
   RRCrtc align_to;
   int align;
};

struct xrandr_output {
   RROutput id;
   Time timestamp;
   RRCrtc crtc;
   char *name;
   int namelen;
   unsigned long mm_width;
   unsigned long mm_height;
   Connection connection;
   SubpixelOrder subpixel_order;
   _AL_VECTOR crtcs; // RRCrtc
   _AL_VECTOR clones; // RROutput
   RRMode prefered_mode;
   _AL_VECTOR modes; // RRMode
};

struct xrandr_mode {
   RRMode id;
   unsigned int width;
   unsigned int height;
   unsigned int refresh;
};

struct xrandr_rect {
   int x1;
   int y1;
   int x2;
   int y2;
};

static void xrandr_copy_mode(xrandr_mode *mode, XRRModeInfo *rrmode)
{
   mode->id     = rrmode->id;
   mode->width  = rrmode->width;
   mode->height = rrmode->height;
   
   if (rrmode->hTotal && rrmode->vTotal) {
      mode->refresh = ((float) rrmode->dotClock / ((float) rrmode->hTotal * (float) rrmode->vTotal));
   }
   else {
      mode->refresh = 0;
   }
}

static void xrandr_clear_fake_refresh_rates(xrandr_mode *modes, int nmode)
{
   int i;

   if (nmode < 2)
      return;

   /* The Nvidia proprietary driver may return fake refresh rates when
    * DynamicTwinView is enabled, so that all modes are unique.  The user has
    * no use for that wrong information so zero it out if we detect it.
    */

   for (i = 1; i < nmode; i++) {
      if (modes[i].refresh != modes[i-1].refresh + 1) {
         return;
      }
   }

   ALLEGRO_WARN("Zeroing out fake refresh rates from nvidia proprietary driver.\n");
   ALLEGRO_WARN("Disable the DynamicTwinView driver option to avoid this.\n");

   for (i = 0; i < nmode; i++) {
      modes[i].refresh = 0;
   }
}

static void xrandr_copy_output(xrandr_output *output, RROutput id, XRROutputInfo *rroutput)
{
   output->id             = id;
   output->timestamp      = rroutput->timestamp;
   output->crtc           = rroutput->crtc;
   output->name           = strdup(rroutput->name);
   output->namelen        = rroutput->nameLen;
   output->mm_width       = rroutput->mm_width;
   output->mm_height      = rroutput->mm_height;
   output->connection     = rroutput->connection;
   output->subpixel_order = rroutput->subpixel_order;
   
   ALLEGRO_DEBUG("output[%s] %s on crtc %i.\n", output->name, output->connection == RR_Connected ? "Connected" : "Not Connected", (int)output->crtc);
   
   _al_vector_init(&output->crtcs, sizeof(RRCrtc));
   if(rroutput->ncrtc) {
      _al_vector_append_array(&output->crtcs, rroutput->ncrtc, rroutput->crtcs);
   }
   
   _al_vector_init(&output->clones, sizeof(RROutput));
   if(rroutput->nclone) {
      _al_vector_append_array(&output->clones, rroutput->nclone, rroutput->clones);
   }
   
   _al_vector_init(&output->modes, sizeof(RRMode));
   if(rroutput->nmode) {
      _al_vector_append_array(&output->modes, rroutput->nmode, rroutput->modes);
   }

   /* npreferred isn't the prefered mode index, it's the number of prefered modes
    * starting from 0. So just load up the first prefered mode.
    * We don't actually use it yet anyway, so it's not really important.
    */
   if(rroutput->npreferred) {
      output->prefered_mode = rroutput->modes[0];
   }
}

static void xrandr_copy_crtc(xrandr_crtc *crtc, RRCrtc id, XRRCrtcInfo *rrcrtc)
{
   crtc->id        = id;
   crtc->timestamp = rrcrtc->timestamp;
   crtc->x         = rrcrtc->x;
   crtc->y         = rrcrtc->y;
   crtc->width     = rrcrtc->width;
   crtc->height    = rrcrtc->height;
   crtc->mode      = rrcrtc->mode;
   crtc->rotation  = rrcrtc->rotation;
   
   _al_vector_init(&crtc->connected, sizeof(RROutput));
   if(rrcrtc->noutput) {
      _al_vector_append_array(&crtc->connected, rrcrtc->noutput, rrcrtc->outputs);
   }
   
   ALLEGRO_DEBUG("found %i outputs.\n", rrcrtc->noutput);
   
   _al_vector_init(&crtc->possible, sizeof(RROutput));
   if(rrcrtc->npossible) {
      _al_vector_append_array(&crtc->possible, rrcrtc->npossible, rrcrtc->possible);

      int i;
      for(i = 0; i < rrcrtc->npossible; i++) {
         ALLEGRO_DEBUG("output[%i] %i.\n", i, (int)rrcrtc->possible[i]);
      }
   }
   
   crtc->original_mode = crtc->mode;
   crtc->original_xoff = crtc->x;
   crtc->original_yoff = crtc->y;
   crtc->align_to = 0;
   crtc->align = CRTC_POS_NONE;
}

static void xrandr_copy_screen(ALLEGRO_SYSTEM_XGLX *s, xrandr_screen *screen, XRRScreenResources *res)
{
   int j;

   _al_vector_init(&screen->modes, sizeof(xrandr_mode));
   if(res->nmode) {
      for(j = 0; j < res->nmode; j++) {
         xrandr_mode *mode = _al_vector_alloc_back(&screen->modes);
         xrandr_copy_mode(mode, &res->modes[j]);
      }

      xrandr_clear_fake_refresh_rates(_al_vector_ref_front(&screen->modes), res->nmode);
   }

   _al_vector_init(&screen->crtcs, sizeof(xrandr_crtc));
   if(res->ncrtc) {
      ALLEGRO_DEBUG("found %i crtcs.\n", res->ncrtc);
      for(j = 0; j < res->ncrtc; j++) {
         ALLEGRO_DEBUG("crtc[%i] %i.\n", j, (int)res->crtcs[j]);
         xrandr_crtc *crtc = _al_vector_alloc_back(&screen->crtcs);
         XRRCrtcInfo *rrcrtc = XRRGetCrtcInfo(s->x11display, res, res->crtcs[j]);
      
         xrandr_copy_crtc(crtc, res->crtcs[j], rrcrtc);
      
         XRRFreeCrtcInfo(rrcrtc);
      }
   }
   
   _al_vector_init(&screen->outputs, sizeof(xrandr_output));
   if(res->noutput) {
      ALLEGRO_DEBUG("found %i outputs.\n", res->noutput);
      for(j = 0; j < res->noutput; j++) {
         ALLEGRO_DEBUG("output[%i] %i.\n", j, (int)res->outputs[j]);
         xrandr_output *output = _al_vector_alloc_back(&screen->outputs);
         XRROutputInfo *rroutput = XRRGetOutputInfo(s->x11display, res, res->outputs[j]);
      
         xrandr_copy_output(output, res->outputs[j], rroutput);
      
         XRRFreeOutputInfo(rroutput);
         XSync(s->x11display, False);
      }
   }
}

static xrandr_crtc *xrandr_fetch_crtc(ALLEGRO_SYSTEM_XGLX *s, int sid, RRCrtc id)
{
   unsigned int i;
   
   xrandr_screen *screen = _al_vector_ref(&s->xrandr_screens, sid);
   
   for(i = 0; i < _al_vector_size(&screen->crtcs); i++) {
      xrandr_crtc *crtc = _al_vector_ref(&screen->crtcs, i);
      if(crtc->id == id)
         return crtc;
   }
   
   return NULL;
}

static xrandr_output *xrandr_fetch_output(ALLEGRO_SYSTEM_XGLX *s, int sid, RROutput id)
{
   unsigned int i;
   
   xrandr_screen *screen = _al_vector_ref(&s->xrandr_screens, sid);
   
   for(i = 0; i < _al_vector_size(&screen->outputs); i++) {
      xrandr_output *output = _al_vector_ref(&screen->outputs, i);
      if(output->id == id)
         return output;
   }
   
   return NULL;
}

static xrandr_mode *xrandr_fetch_mode(ALLEGRO_SYSTEM_XGLX *s, int sid, RRMode id)
{
   unsigned int i;
   
   xrandr_screen *screen = _al_vector_ref(&s->xrandr_screens, sid);

   for(i = 0; i < _al_vector_size(&screen->modes); i++) {
      xrandr_mode *mode = _al_vector_ref(&screen->modes, i);
      if(mode->id == id)
         return mode;
   }
   
   return NULL;
}

static inline xrandr_crtc *xrandr_map_to_crtc(ALLEGRO_SYSTEM_XGLX *s, int sid, int adapter)
{
   return xrandr_fetch_crtc(s, sid, *(RRCrtc*)_al_vector_ref(&s->xrandr_adaptermap, adapter));
}

static inline xrandr_output *xrandr_map_adapter(ALLEGRO_SYSTEM_XGLX *s, int sid, int adapter)
{
   xrandr_crtc *crtc = xrandr_map_to_crtc(s, sid, adapter);
   return xrandr_fetch_output(s, sid, *(RROutput*)_al_vector_ref(&crtc->connected, 0));
}

static void xrandr_combine_output_rect(struct xrandr_rect *rect, xrandr_crtc *crtc)
{
   if(rect->x1 > crtc->x)
      rect->x1 = crtc->x;
   
   if(rect->y1 > crtc->y)
      rect->y1 = crtc->y;
   
   if(crtc->x + (int)crtc->width > rect->x2)
      rect->x2 = crtc->x + crtc->width;
   
   if(crtc->y + (int)crtc->height > rect->y2)
      rect->y2 = crtc->y + crtc->height;
}

/* begin vtable methods */

static int xrandr_get_xscreen(ALLEGRO_SYSTEM_XGLX *s, int adapter);

static bool xrandr_query(ALLEGRO_SYSTEM_XGLX *s)
{
   int screen_count = ScreenCount(s->x11display);
   int i;
   bool ret = true;
   
   _al_vector_init(&s->xrandr_screens, sizeof(xrandr_screen));
   _al_vector_init(&s->xrandr_adaptermap, sizeof(RROutput));

   for(i = 0; i < screen_count; i++) {
      xrandr_screen *screen = _al_vector_alloc_back(&s->xrandr_screens);
      
      XRRScreenResources *res = XRRGetScreenResources(s->x11display, XRootWindow(s->x11display, i));
      if(!res) {
         ALLEGRO_DEBUG("failed to get screen resources for screen %i\n", i);
         continue;
      }
      
      if(!res->noutput) {
         ALLEGRO_DEBUG("screen %i doesn't have any outputs.\n", i);
         continue;
      }
      
      xrandr_copy_screen(s, screen, res);
      
      screen->res = res;
      
      /* Detect clones */
      int j;
      for(j = 0; j < (int)_al_vector_size(&screen->crtcs); j++) {
         xrandr_crtc *crtc = _al_vector_ref(&screen->crtcs, j);

         /* Skip crtc with no connected outputs. */
         if(_al_vector_size(&crtc->connected) < 1)
            continue;
            
         // XXX it might be nessesary to do something more clever about detecting clones
         // XXX for now I'm going with a plain origin test, it aught to work for most cases.
         // the other alternative is to check the crtc's connected outputs's clone's list...
         // sounds like pain to me.
         int k;
         bool not_clone = true;
         for(k = 0; k < j; k++) {
            xrandr_crtc *crtc_k = _al_vector_ref(&screen->crtcs, k);
            
            /* Skip crtc with no connected outputs. */
            if(_al_vector_size(&crtc_k->connected) < 1)
               continue;
            
            if(crtc->x == crtc_k->x && crtc->y == crtc_k->y)
               not_clone = false;
         }
         
         if(not_clone) {
            RRCrtc *crtc_ptr = _al_vector_alloc_back(&s->xrandr_adaptermap);
            ALLEGRO_DEBUG("Map Allegro Adadpter %i to RandR CRTC %i.\n", (int)(_al_vector_size(&s->xrandr_adaptermap)-1), (int)crtc->id);
            *crtc_ptr = crtc->id;
         }
         else {
            ALLEGRO_DEBUG("RandR CRTC %i is a clone, ignoring.\n", (int)crtc->id);
         }
      }
      
      int mask =  RRScreenChangeNotifyMask | 
                  RRCrtcChangeNotifyMask   | 
                  RROutputChangeNotifyMask | 
                  RROutputPropertyNotifyMask;
   
      XRRSelectInput( s->x11display, RootWindow(s->x11display, i), 0);
      XRRSelectInput( s->x11display, RootWindow(s->x11display, i), mask); 
   }
   
   /* map out crtc positions and alignments */
   /* this code makes Adapter 0 the root display, everything will hang off it */
   for(i = 1; i < (int)_al_vector_size(&s->xrandr_adaptermap); i++) {
      int xscreen = xrandr_get_xscreen(s, i);
      xrandr_crtc *crtc = xrandr_fetch_crtc(s, xscreen, *(RRCrtc*)_al_vector_ref(&s->xrandr_adaptermap, i));
      
      int j;
      for(j = 0; j < (int)_al_vector_size(&s->xrandr_adaptermap); j++) {
         int xscreen_j = xrandr_get_xscreen(s, j);
         xrandr_crtc *crtc_j = xrandr_fetch_crtc(s, xscreen_j, *(RRCrtc*)_al_vector_ref(&s->xrandr_adaptermap, j));
         
         if(crtc->x == crtc_j->x + (int)crtc_j->width) {
            crtc->align_to = crtc_j->id;
            crtc->align = CRTC_POS_RIGHTOF;
            ALLEGRO_DEBUG("Adapter %i is RightOf Adapter %i.\n", i, j);
         }
         else if(crtc->x + (int)crtc->width == crtc_j->x) {
            crtc->align_to = crtc_j->id;
            crtc->align = CRTC_POS_LEFTOF;
            ALLEGRO_DEBUG("Adapter %i is LeftOf Adapter %i.\n", i, j);
         }
         else if(crtc->y == crtc_j->y + (int)crtc_j->height) {
            crtc->align_to = crtc_j->id;
            crtc->align = CRTC_POS_BELOW;
            ALLEGRO_DEBUG("Adapter %i is Below Adapter %i.\n", i, j);
         }
         else if(crtc->y + (int)crtc->height == crtc_j->y) {
            crtc->align_to = crtc_j->id;
            crtc->align = CRTC_POS_ABOVE;
            ALLEGRO_DEBUG("Adapter %i is Above Adapter %i.\n", i, j);
         }
      }
   }
      
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   if (s->xinerama_available && s->xinerama_screen_count != (int)_al_vector_size(&s->xrandr_adaptermap)) {
      ALLEGRO_WARN("XRandR and Xinerama seem to disagree on how many screens there are (%i vs %i), going to ignore XRandR.\n", (int)_al_vector_size(&s->xrandr_adaptermap), s->xinerama_screen_count);
      // only actually going to ignore the output count, and setting of modes on the extra xinerama screens.
      ret = false;
   }
#endif

   return ret;
}

static int xrandr_get_num_modes(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   if(adapter < 0 || adapter >= (int)_al_vector_size(&s->xrandr_adaptermap))
      return 0;
   
   int xscreen = _al_xglx_get_xscreen(s, adapter);
   xrandr_output *output = xrandr_map_adapter(s, xscreen, adapter);
   return _al_vector_size(&output->modes);
}

static ALLEGRO_DISPLAY_MODE *xrandr_get_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter, int id, ALLEGRO_DISPLAY_MODE *amode)
{
   int xscreen = _al_xglx_get_xscreen(s, adapter);
   xrandr_output *output = xrandr_map_adapter(s, xscreen, adapter);
   
   if(id < 0 || id > (int)_al_vector_size(&output->modes))
      return NULL;
   
   xrandr_mode *mode = xrandr_fetch_mode(s, xscreen, *(RRMode*)_al_vector_ref(&output->modes, id));
   
   amode->width = mode->width;
   amode->height = mode->height;
   amode->format = 0;
   amode->refresh_rate = mode->refresh;
   
   return amode;
}

static bool xrandr_realign_crtc_origin(ALLEGRO_SYSTEM_XGLX *s, int xscreen, xrandr_crtc *crtc, int new_w, int new_h, int *x, int *y)
{
   bool ret;
   
   if(crtc->align_to == 0)
      return false;
   
   xrandr_crtc *align_to = xrandr_fetch_crtc(s, xscreen, crtc->align_to);

   switch(crtc->align) {
      case CRTC_POS_RIGHTOF:
         *x = align_to->x + align_to->width;
         *y = align_to->y;
         ret = true;
         break;
         
      case CRTC_POS_LEFTOF:
         *x = align_to->x - new_w;
         *y = align_to->y;
         ret = true;
         break;
         
      case CRTC_POS_BELOW:
         *x = align_to->x;
         *y = align_to->y + align_to->height;
         ret = true;
         break;
         
      case CRTC_POS_ABOVE:
         *x = align_to->x;
         *y = align_to->y - new_h;
         ret = true;
         break;
      
      default:
         ALLEGRO_WARN("unknown crtc alignment flag (%i)!", crtc->align);
         ret = false;
         break;
   }
   
   return ret;
}

static bool xrandr_set_mode(ALLEGRO_SYSTEM_XGLX *s, ALLEGRO_DISPLAY_XGLX *d, int w, int h, int format, int refresh)
{
   int adapter = _al_xglx_get_adapter(s, d, false);
   int xscreen = _al_xglx_get_xscreen(s, adapter);
   
   xrandr_screen *screen = _al_vector_ref(&s->xrandr_screens, xscreen);
   
   xrandr_crtc *crtc = xrandr_map_to_crtc(s, xscreen, adapter);
   xrandr_mode *cur_mode = xrandr_fetch_mode(s, xscreen, crtc->mode);
   
   if((int)cur_mode->width == w && (int)cur_mode->height == h && (refresh == 0 || refresh == (int)cur_mode->refresh)) {
      ALLEGRO_DEBUG("mode already set, good to go\n");
      return true;
   }
   else {
      ALLEGRO_DEBUG("new mode: %dx%d@%d old mode: %dx%d@%d.\n", w, h, refresh, cur_mode->width, cur_mode->height, cur_mode->refresh);
   }
   
   int mode_idx = _al_xglx_fullscreen_select_mode(s, adapter, w, h, format, refresh);
   if(mode_idx == -1) {
      ALLEGRO_DEBUG("mode %dx%d@%d not found\n", w, h, refresh);
      return false;
   }
   
   xrandr_output *output = xrandr_fetch_output(s, xscreen, *(RROutput*)_al_vector_ref(&crtc->connected, 0));
   xrandr_mode *mode = xrandr_fetch_mode(s, xscreen, *(RRMode*)_al_vector_ref(&output->modes, mode_idx));
   
   int new_x = crtc->x, new_y = crtc->y;
   
   xrandr_realign_crtc_origin(s, xscreen, crtc, w, h, &new_x, &new_y);
   
   ALLEGRO_DEBUG("xrandr: set mode %i+%i-%ix%i on adapter %i\n", new_x, new_y, w, h, adapter);
   
   _al_mutex_lock(&s->lock);
   
   int ok = XRRSetCrtcConfig(
      s->x11display,
      screen->res,
      crtc->id,
      crtc->timestamp,
      new_x,
      new_y,
      mode->id,
      crtc->rotation,
      _al_vector_ref_front(&crtc->connected),
      _al_vector_size(&crtc->connected)
   );
   
   if (ok != RRSetConfigSuccess) {
      ALLEGRO_ERROR("XRandR failed to set mode.\n");
      _al_mutex_unlock(&s->lock);
      return false;
   }
   
   /* make sure the virtual screen size is large enough after setting our mode */
   int i;
   struct xrandr_rect rect = { 0, 0, 0, 0 };
   for(i = 0; i < (int)_al_vector_size(&screen->crtcs); i++) {
      xrandr_crtc *c = _al_vector_ref(&screen->crtcs, i);
      if(_al_vector_size(&c->connected) > 0) {
         xrandr_combine_output_rect(&rect, crtc);
      }
   }
   
   int new_width = rect.x2 - rect.x1;
   int new_height = rect.y2 - rect.y1;
   
   if(new_width > DisplayWidth(s->x11display, xscreen) ||
      new_height > DisplayHeight(s->x11display, xscreen))
   {
      XRRSetScreenSize(s->x11display,
                       RootWindow(s->x11display, xscreen),
                       new_width, new_height,
                       DisplayWidthMM(s->x11display, xscreen),
                       DisplayHeightMM(s->x11display, xscreen));
   }
   
   _al_mutex_unlock(&s->lock);
   
   return true;
}

static void xrandr_restore_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   int xscreen = _al_xglx_get_xscreen(s, adapter);
   xrandr_screen *screen = _al_vector_ref(&s->xrandr_screens, xscreen);
   xrandr_crtc *crtc = xrandr_map_to_crtc(s, xscreen, adapter);
   
   if(crtc->mode == crtc->original_mode) {
      ALLEGRO_DEBUG("current crtc mode (%i) equals the original mode (%i), not restoring.\n", (int)crtc->mode, (int)crtc->original_mode);
      return;
   }
   
   xrandr_mode *orig_mode = xrandr_fetch_mode(s, xscreen, crtc->original_mode);
   
   ALLEGRO_DEBUG("restore mode %i+%i-%ix%i@%i on adapter %i\n", crtc->original_xoff, crtc->original_yoff, orig_mode->width, orig_mode->height, orig_mode->refresh, adapter);
   
   _al_mutex_lock(&s->lock);
   
   int ok = XRRSetCrtcConfig
   (
      s->x11display,
      screen->res,
      crtc->id,
      crtc->timestamp,
      crtc->original_xoff,
      crtc->original_yoff,
      orig_mode->id,
      crtc->rotation,
      _al_vector_ref_front(&crtc->connected),
      _al_vector_size(&crtc->connected)
   );
   
   if(ok != RRSetConfigSuccess) {
      ALLEGRO_ERROR("failed to restore mode.\n");
   }
   
   // XSync(s->x11display, False);
   
   _al_mutex_unlock(&s->lock);
}

static void xrandr_get_display_offset(ALLEGRO_SYSTEM_XGLX *s, int adapter, int *x, int *y)
{
   int xscreen = _al_xglx_get_xscreen(s, adapter);
   
   xrandr_crtc *crtc = xrandr_map_to_crtc(s, xscreen, adapter);
   
   // XXX Should we always return original_[xy]off here?
   // When does a user want to query the offset after the modes are set?
   
   *x = crtc->x;
   *y = crtc->y;
   
   ALLEGRO_DEBUG("display offset: %ix%i.\n", *x, *y);
}

static int xrandr_get_num_adapters(ALLEGRO_SYSTEM_XGLX *s)
{
   return _al_vector_size(&s->xrandr_adaptermap);
}

static bool xrandr_get_monitor_info(ALLEGRO_SYSTEM_XGLX *s, int adapter, ALLEGRO_MONITOR_INFO *mi)
{
   if(adapter < 0 || adapter >= (int)_al_vector_size(&s->xrandr_adaptermap))
      return false;
   
   int xscreen = _al_xglx_get_xscreen(s, adapter);
   xrandr_output *output = xrandr_map_adapter(s, xscreen, adapter);
   
   xrandr_crtc *crtc = xrandr_fetch_crtc(s, xscreen, output->crtc);
   
   mi->x1 = crtc->x;
   mi->y1 = crtc->y;
   mi->x2 = crtc->x + crtc->width;
   mi->y2 = crtc->y + crtc->height;
   return true;
}

static int xrandr_get_default_adapter(ALLEGRO_SYSTEM_XGLX *s)
{
   // if we have more than one xrandr_screen, we're in multi-head x mode
   if(_al_vector_size(&s->xrandr_screens) > 1)
      return _al_xsys_mheadx_get_default_adapter(s);
   
   int center_x = 0, center_y = 0;
   _al_xsys_get_active_window_center(s, &center_x, &center_y);
   
   int i, default_adapter = 0;
   for(i = 0; i < (int)_al_vector_size(&s->xrandr_adaptermap); i++) {
      xrandr_crtc *crtc = xrandr_map_to_crtc(s, 0, i);
      
      if(center_x >= (int)crtc->x && center_x <= (int)(crtc->x + crtc->width) &&
         center_y >= (int)crtc->y && center_y <= (int)(crtc->y + crtc->height))
      {
         default_adapter = i;
         break;
      }
   }
   
   ALLEGRO_DEBUG("selected default adapter: %i.\n", default_adapter);
   
   return default_adapter;
}

static int xrandr_get_xscreen(ALLEGRO_SYSTEM_XGLX *s, int adapter)
{
   // more than one screen means we have multi-head x mode
   if(_al_vector_size(&s->xrandr_screens) > 1)
      return _al_xsys_mheadx_get_xscreen(s, adapter);
   
   // Normal XRandR will normally give us one X screen, so return 0 always.
   return 0;
}

static void xrandr_handle_xevent(ALLEGRO_SYSTEM_XGLX *s, ALLEGRO_DISPLAY_XGLX *d, XEvent *e)
{
   if(e->type == s->xrandr_event_base + RRNotify) {
      XRRNotifyEvent *rre = (XRRNotifyEvent*)e;
      if(rre->subtype == RRNotify_CrtcChange) {
         XRRCrtcChangeNotifyEvent *rrce = (XRRCrtcChangeNotifyEvent*)rre;
         ALLEGRO_DEBUG("RRNotify_CrtcChange!\n");
         
         xrandr_crtc *crtc = xrandr_fetch_crtc(s, d->xscreen, rrce->crtc);
         if(!crtc) {
            ALLEGRO_DEBUG("invalid RRCrtc(%i).\n", (int)rrce->crtc);
            return;
         }
         
         if(rrce->mode != crtc->mode) {
            ALLEGRO_DEBUG("mode changed from %i to %i.\n", (int)crtc->mode, (int)rrce->mode);
            crtc->mode = rrce->mode;
         }
         
         if(rrce->rotation != crtc->rotation) {
            ALLEGRO_DEBUG("rotation changed from %i to %i.\n", crtc->rotation, rrce->rotation);
            crtc->rotation = rrce->rotation;
         }
         
         if(rrce->x != crtc->x || rrce->y != crtc->y) {
            ALLEGRO_DEBUG("origin changed from %i+%i to %i+%i.\n", crtc->x, crtc->y, rrce->x, rrce->y);
            crtc->x = rrce->x;
            crtc->y = rrce->y;
         }
         
         if(rrce->width != crtc->width || rrce->height != crtc->height) {
            ALLEGRO_DEBUG("size changed from %ix%i to %ix%i.\n", crtc->width, crtc->height, rrce->width, rrce->height);
            crtc->width = rrce->width;
            crtc->height = rrce->height;
         }
         
         xrandr_screen *screen = _al_vector_ref(&s->xrandr_screens, d->xscreen);
         crtc->timestamp = screen->timestamp;
      }
      else if(rre->subtype == RRNotify_OutputChange) {
         XRROutputChangeNotifyEvent *rroe = (XRROutputChangeNotifyEvent*)rre;
                                                      
         xrandr_output *output = xrandr_fetch_output(s, d->xscreen, rroe->output);
         if(!output) {
            ALLEGRO_DEBUG("invalid RROutput(%i).\n", (int)rroe->output);
            return;
         }
         
         ALLEGRO_DEBUG("xrandr: RRNotify_OutputChange %s!\n", output->name);
         
         if(rroe->crtc != output->crtc) {
            ALLEGRO_DEBUG("crtc changed from %i to %i.\n", (int)output->crtc, (int)rroe->crtc);
            output->crtc = rroe->crtc;
         }
         
         // XXX I'm not sure how monitor connections are handled here,
         // that is, what happens when a monitor is connected, and disconnected...
         // IE: CHECK!
         if(rroe->connection != output->connection) {
            ALLEGRO_DEBUG("connection changed from %i to %i.\n", output->connection, rroe->connection);
            output->connection = rroe->connection;
         }
         
         xrandr_screen *screen = _al_vector_ref(&s->xrandr_screens, d->xscreen);
         output->timestamp = screen->timestamp;
      }
      else if(rre->subtype == RRNotify_OutputProperty) {
         ALLEGRO_DEBUG("xrandr: RRNotify_OutputProperty!\n");
      }
      else {
         ALLEGRO_DEBUG("xrandr: RRNotify_Unknown(%i)!\n", rre->subtype);  
      }
   }
   else if(e->type == s->xrandr_event_base + RRScreenChangeNotify) {
      XRRScreenChangeNotifyEvent *rre = (XRRScreenChangeNotifyEvent*)e;
      XRRUpdateConfiguration( e );
      
      ALLEGRO_DEBUG("RRScreenChangeNotify!\n");
      
      /* XXX I don't think we need to actually handle this event fully,
       * it only really deals with the virtual screen as a whole it seems
       * The interesting changes get sent with the RRNotify event.
       */

      /* update the timestamps */
      xrandr_screen *screen = _al_vector_ref(&s->xrandr_screens, d->xscreen);
      screen->timestamp = rre->timestamp;
      screen->configTimestamp = rre->config_timestamp;
   }
}

/* begin "public" ctor/dtor methods */

void _al_xsys_xrandr_init(ALLEGRO_SYSTEM_XGLX *s)
{
   int error_base = 0;
   
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
         bool ret = xrandr_query(s);
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
      memset(&_al_xglx_mmon_interface, 0, sizeof(_al_xglx_mmon_interface));
      _al_xglx_mmon_interface.get_num_display_modes = xrandr_get_num_modes;
      _al_xglx_mmon_interface.get_display_mode      = xrandr_get_mode;
      _al_xglx_mmon_interface.set_mode              = xrandr_set_mode;
      _al_xglx_mmon_interface.restore_mode          = xrandr_restore_mode;
      _al_xglx_mmon_interface.get_display_offset    = xrandr_get_display_offset;
      _al_xglx_mmon_interface.get_num_adapters      = xrandr_get_num_adapters;
      _al_xglx_mmon_interface.get_monitor_info      = xrandr_get_monitor_info;
      _al_xglx_mmon_interface.get_default_adapter   = xrandr_get_default_adapter;
      _al_xglx_mmon_interface.get_xscreen           = xrandr_get_xscreen;
      _al_xglx_mmon_interface.handle_xevent         = xrandr_handle_xevent;
   }

   _al_mutex_unlock(&s->lock);
}

void _al_xsys_xrandr_exit(ALLEGRO_SYSTEM_XGLX *s)
{
#if 0
   // int i;
   ALLEGRO_DEBUG("XRandR exiting.\n");
   
   // for (i = 0; i < s->xrandr_output_count; i++) {
   //   XRRFreeOutputInfo(s->xrandr_outputs[i]);
   // }

   // for (i = 0; i < s->xrandr_res_count; i++) {
   //    XRRFreeScreenResources(s->xrandr_res[i]);
   // }

   ALLEGRO_DEBUG("XRRFreeScreenResources\n");
   //if (s->xrandr_res)
   //   XRRFreeScreenResources(s->xrandr_res);

   al_free(s->xrandr_outputs);
   al_free(s->xrandr_res);
   
   s->xrandr_available = 0;
   s->xrandr_res_count = 0;
   s->xrandr_res = NULL;
   s->xrandr_output_count = 0;
   s->xrandr_outputs = NULL;

   ALLEGRO_DEBUG("XRandR exit finished.\n");
#endif /* 0 */

   int i;
   for(i = 0; i < (int)_al_vector_size(&s->xrandr_screens); i++) {
      xrandr_screen *screen = _al_vector_ref(&s->xrandr_screens, i);
      int j;

      for (j = 0; j < (int)_al_vector_size(&screen->crtcs); j++) {
         xrandr_crtc *crtc = _al_vector_ref(&screen->crtcs, j);
         _al_vector_free(&crtc->connected);
         _al_vector_free(&crtc->possible);
      }

      for(j = 0; j < (int)_al_vector_size(&screen->outputs); j++) {
         xrandr_output *output = _al_vector_ref(&screen->outputs, j);
         free(output->name);
         _al_vector_free(&output->crtcs);
         _al_vector_free(&output->clones);
         _al_vector_free(&output->modes);
      }

      _al_vector_free(&screen->crtcs);
      _al_vector_free(&screen->outputs);
      _al_vector_free(&screen->modes);
      
      XRRFreeScreenResources(screen->res);
      screen->res = NULL;
   }

   _al_vector_free(&s->xrandr_screens);
   _al_vector_free(&s->xrandr_adaptermap);
   
}

#endif /* ALLEGRO_XWINDOWS_WITH_XRANDR */

/* vim: set sts=3 sw=3 et: */
