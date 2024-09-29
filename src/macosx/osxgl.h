#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/platform/aintosx.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_opengl.h"

#include "allegro5/opengl/gl_ext.h"


/* Number of pixel format attributes we can set */
#define AL_OSX_NUM_PFA  64

@interface ALWindow : NSWindow {
   A5O_DISPLAY* display;
}

@property A5O_DISPLAY *display;

@end

/* This is our version of A5O_DISPLAY with driver specific extra data. */
typedef struct A5O_DISPLAY_OSX_WIN {
   A5O_DISPLAY parent;
   int depth;
   NSOpenGLContext* ctx;
   NSOpenGLPixelFormatAttribute attributes[AL_OSX_NUM_PFA];
   ALWindow* win;
   NSView* view;
   NSCursor* cursor;
   CGDirectDisplayID display_id;
   BOOL show_cursor;
   NSTrackingArea *tracking;
   unsigned int display_group;
   BOOL in_fullscreen;
   BOOL single_buffer;
   CGDisplayModeRef original_mode;
   BOOL send_halt_events;
   A5O_MUTEX *halt_mutex;
   A5O_COND *halt_cond;
   BOOL halt_event_acknowledged;
   /* For new (10.14+) vsyncing. */
   CVDisplayLinkRef display_link;
   A5O_MUTEX *flip_mutex;
   A5O_COND *flip_cond;
   int num_flips;
   /* These two store the old window size when restoring from a FS window. */
   int old_w;
   int old_h;
} A5O_DISPLAY_OSX_WIN;

/* This is our version of A5O_MOUSE_CURSOR */
typedef struct A5O_MOUSE_CURSOR_OSX
{
   NSCursor *cursor;
} A5O_MOUSE_CURSOR_OSX;

