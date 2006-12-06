#include <X11/Xlib.h>

typedef struct AL_SYSTEM_XDUMMY AL_SYSTEM_XDUMMY;

/* This is our version of AL_SYSTEM with driver specific extra data. */
struct AL_SYSTEM_XDUMMY
{
   AL_SYSTEM system; /* This must be the first member, we "derive" from it. */

   Display *xdisplay; /* The X11 display. */
   pthread_t thread; /* background thread. */

   // FIXME
   /* Hack by which the display driver can wait on the map event. Eventually,
    * this would be a proper way to install event listeners by making a list
    * of callbacks. Then if multiple windows are created at the same time,
    * each one would install its own callback.
    */
   void *event_cb_data;
   void (*event_cb)(AL_SYSTEM_XDUMMY *system, XEvent *event, void *data);
};

void _al_xwin_keyboard_handler(XKeyEvent *event, bool dga2_hack);
