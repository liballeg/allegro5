#include <X11/Xlib.h>
#include <GL/glx.h>

typedef struct AL_SYSTEM_XDUMMY AL_SYSTEM_XDUMMY;
typedef struct AL_DISPLAY_XDUMMY AL_DISPLAY_XDUMMY;

/* This is our version of AL_SYSTEM with driver specific extra data. */
struct AL_SYSTEM_XDUMMY
{
   AL_SYSTEM system; /* This must be the first member, we "derive" from it. */

   Display *xdisplay; /* The X11 display. */
   pthread_t thread; /* background thread. */
   
   _AL_VECTOR displays; /* Keep a list of all displays attached to us. */
};

/* This is our version of AL_DISPLAY with driver specific extra data. */
struct AL_DISPLAY_XDUMMY
{
   AL_DISPLAY display; /* This must be the first member. */

   Window window;
   GLXWindow glxwindow;
   GLXContext context;
    
   int is_mapped;
   pthread_mutex_t mapped_mutex;
   pthread_cond_t mapped_cond;
};

void _al_display_xdummy_resize(AL_DISPLAY *d, XEvent *event);
void _al_xwin_keyboard_handler(XKeyEvent *event, bool dga2_hack);
