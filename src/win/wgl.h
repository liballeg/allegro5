#include "allegro5/internal/aintern_display.h"
#include "allegro5/platform/aintwin.h"

#include <windows.h>


typedef struct ALLEGRO_DISPLAY_WGL
{
   ALLEGRO_DISPLAY_WIN win_display; /* This must be the first member. */

   /* Driver specifics */
   HDC dc;
   HGLRC glrc;
} ALLEGRO_DISPLAY_WGL;

int _al_win_determine_adapter(void);
