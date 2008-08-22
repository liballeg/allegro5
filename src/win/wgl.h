#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/a5_opengl.h"

#include <windows.h>

#include "win_new.h"


typedef struct ALLEGRO_DISPLAY_WGL
{
   ALLEGRO_DISPLAY_WIN win_display; /* This must be the first member. */

   /* Driver specifics */
   HDC dc;
   HGLRC glrc;

   bool thread_ended;
   bool end_thread;
} ALLEGRO_DISPLAY_WGL;

