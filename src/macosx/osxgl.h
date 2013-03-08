#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/platform/aintosx.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_opengl.h"

#include "allegro5/opengl/gl_ext.h"


/* Number of pixel format attributes we can set */
#define AL_OSX_NUM_PFA  64

/* This is our version of ALLEGRO_DISPLAY with driver specific extra data. */
typedef struct ALLEGRO_DISPLAY_OSX_WIN {
	ALLEGRO_DISPLAY parent;
	int depth;
	NSOpenGLContext* ctx;
   NSOpenGLPixelFormatAttribute attributes[AL_OSX_NUM_PFA];
	NSWindow* win;
   NSCursor* cursor;
   CGDirectDisplayID display_id;
   BOOL show_cursor;
   NSTrackingArea *tracking;
   unsigned int display_group;
   BOOL in_fullscreen;
} ALLEGRO_DISPLAY_OSX_WIN;

/* This is our version of ALLEGRO_MOUSE_CURSOR */
typedef struct ALLEGRO_MOUSE_CURSOR_OSX
{
   NSCursor *cursor;
} ALLEGRO_MOUSE_CURSOR_OSX;

