#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/opengl/algl.h"

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

   /* A list of extensions supported by Allegro, for this context. */
   struct ALLEGRO_OGL_EXT_LIST *extension_list;

   /* A list of extension API, loaded by Allegro, for this context. */
   ALLEGRO_OGL_EXT_API *extension_api;

   OPENGL_INFO ogl_info;
};