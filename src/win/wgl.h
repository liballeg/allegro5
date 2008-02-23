#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_bitmap.h"

#include <windows.h>

#include "win_new.h"

typedef struct ALLEGRO_DISPLAY_WGL ALLEGRO_DISPLAY_WGL;

struct ALLEGRO_DISPLAY_WGL
{
   ALLEGRO_DISPLAY display; /* This must be the first member. */

   /* Driver specifics */
   HWND window;
   HDC dc;
   HGLRC glrc;
};
