#include "xdummy.h"

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE

static int check(AL_SYSTEM_XDUMMY *s)
{
   int x, y;
   return XF86VidModeQueryExtension(s->xdisplay, &x, &y)
      && XF86VidModeQueryVersion(s->xdisplay, &x, &y);
}

int get_num_display_modes(AL_SYSTEM_XDUMMY *s)
{
   if (!check(s)) return 0;

   if (!XF86VidModeGetAllModeLines(s->xdisplay, 0, &s->modes_count, &s->modes))
      return 0;

   return s->modes_count;
}

int _al_xdummy_get_num_display_modes(void)
{
   return get_num_display_modes((void *)al_system_driver());
}

AL_DISPLAY_MODE *get_display_mode(AL_SYSTEM_XDUMMY *s,
   int i, AL_DISPLAY_MODE *mode)
{
   if (!s->modes) return NULL;

   mode->width = s->modes[i]->hdisplay;
   mode->height = s->modes[i]->vdisplay;
   mode->format = 0;
   mode->refresh_rate = 0;

   return mode;
}

AL_DISPLAY_MODE *_al_xdummy_get_display_mode(
   int i, AL_DISPLAY_MODE *mode)
{
   return get_display_mode((void *)al_system_driver(), i, mode);
}

bool _al_xdummy_fullscreen_set_mode(AL_SYSTEM_XDUMMY *s,
   int w, int h, int format, int refresh_rate)
{
   int i;
   AL_DISPLAY_MODE mode, mode2;
   int n = get_num_display_modes(s);
   if (!n) return false;

   /* Find all modes with correct parameters. */
   int possible_modes[n];
   int possible_count = 0;
   for (i = 0; i < n; i++) {
      get_display_mode(s, i, &mode);
      if (mode.width == w && mode.height == h &&
         (format == 0 || mode.format == format) &&
         (refresh_rate == 0 || mode.refresh_rate == refresh_rate)) {
         possible_modes[possible_count++] = i;
      }
   }
   if (!possible_count) return false;

   /* Choose mode with highest refresh rate. */
   int best_mode = possible_modes[0];
   get_display_mode(s, best_mode, &mode);
   for (i = 1; i < possible_count; i++) {
      get_display_mode(s, possible_modes[i], &mode2);
      if (mode2.refresh_rate > mode.refresh_rate) {
         mode = mode2;
         best_mode = possible_modes[i];
      }
   }

   if (!XF86VidModeSwitchToMode(s->xdisplay, 0, s->modes[best_mode]))
      return false;

   return true;
}

void _al_xdummy_fullscreen_set_origin(AL_SYSTEM_XDUMMY *s, int x, int y)
{
   XF86VidModeSetViewPort(s->xdisplay, 0, x, y);
}

void _al_xdummy_store_video_mode(AL_SYSTEM_XDUMMY *s)
{
   get_num_display_modes(s);
   s->original_mode = s->modes[0];
}

void _al_xdummy_restore_video_mode(AL_SYSTEM_XDUMMY *s)
{
   XF86VidModeSwitchToMode(s->xdisplay, 0, s->original_mode);
}
#else
int _al_xdummy_get_num_display_modes(void)
{
   return 0;
}

AL_DISPLAY_MODE *_al_xdummy_get_display_mode(int index, AL_DISPLAY_MODE *mode)
{
   return NULL;
}

bool _al_xdummy_set_mode(int w, int h, int format, int refresh_rate)
{
   return false;
}

void _al_xdummy_store_video_mode(AL_SYSTEM_XDUMMY *s)
{
}

void _al_xdummy_restore_video_mode(AL_SYSTEM_XDUMMY *s)
{
}
#endif
