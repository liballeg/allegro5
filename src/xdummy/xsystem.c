/* This is only a dummy driver, not implementing most required things,
 * it's just here to give me some understanding of the base framework of a
 * system driver.
 */
#include <stdlib.h>
#include <string.h>

#include "allegro.h"
#include "platform/aintunix.h"
#include "internal/aintern.h"
#include "internal/aintern2.h"
#include "internal/system_new.h"

#include "xdummy.h"

static AL_SYSTEM_INTERFACE *vt;

/* Create a new system object for the dummy X11 driver. */
static AL_SYSTEM *initialize(int flags)
{
   AL_SYSTEM_XDUMMY *s = _AL_MALLOC(sizeof *s);
   memset(s, 0, sizeof *s);

   s->system.vt = vt;

   /* Get an X11 display handle. */
   s->xdisplay = XOpenDisplay(0);

   TRACE("xsystem: XDummy driver connected to X11.\n");

   return &s->system;
}

// FIXME: This is just for now, the real way is of course a list of
// available display drivers. Possibly such drivers can be attached at runtime
// to the system driver, so addons could provide additional drivers.
AL_DISPLAY_INTERFACE *get_display_driver(void)
{
    return _al_display_xdummy_driver();
}

/* Internal function to get a reference to this driver. */
AL_SYSTEM_INTERFACE *_al_system_xdummy_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->initialize = initialize;
   vt->get_display_driver = get_display_driver;

   return vt;
}

