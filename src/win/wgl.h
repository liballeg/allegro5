#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/a5_opengl.h"

#include <windows.h>

#include "win_new.h"


typedef struct ALLEGRO_DISPLAY_WGL
{
   ALLEGRO_DISPLAY_OGL ogl_display; /* This must be the first member. */

   /* Driver specifics */
   HWND window;
   HDC dc;
   HGLRC glrc;

   int mouse_range_x1;
   int mouse_range_y1;
   int mouse_range_x2;
   int mouse_range_y2;

   HCURSOR mouse_selected_hcursor;
   bool mouse_cursor_shown;

   bool thread_ended;
   bool end_thread;
} ALLEGRO_DISPLAY_WGL;

