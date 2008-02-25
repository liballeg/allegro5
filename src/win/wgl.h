#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/a5_opengl.h"

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

   /* Mandatory members */
   /* A list of extensions supported by Allegro, for this context. */
   ALLEGRO_OGL_EXT_LIST *extension_list;
   /* A list of extension API, loaded by Allegro, for this context. */
   ALLEGRO_OGL_EXT_API *extension_api;
   /* Various info about OpenGL implementation. */
   OPENGL_INFO ogl_info;
};

