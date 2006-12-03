#include <X11/Xlib.h>

typedef struct AL_SYSTEM_XDUMMY AL_SYSTEM_XDUMMY;

/* This is our version of AL_SYSTEM with driver specific extra data. */
struct AL_SYSTEM_XDUMMY
{
   AL_SYSTEM system; /* This must be the first member, we "derive" from it. */

   Display *xdisplay; /* The X11 display. */
};
