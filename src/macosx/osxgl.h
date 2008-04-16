#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/platform/aintosx.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_opengl.h"

#include "allegro5/opengl/gl_ext.h"

/* This is our version of ALLEGRO_DISPLAY with driver specific extra data. */
typedef struct ALLEGRO_DISPLAY_OSX_WIN {
	ALLEGRO_DISPLAY_OGL parent;
	int gl_fmt, gl_datasize;
	NSOpenGLContext* ctx;
	NSWindow* win;
   NSCursor* cursor;
   BOOL show_cursor;
   NSTrackingRectTag tracking;
} ALLEGRO_DISPLAY_OSX_WIN;

