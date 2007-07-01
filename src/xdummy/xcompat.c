#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/internal/aintern_bitmap.h"

/* This is just a temporary gfx driver for the compatibility layer. It will
 * provide a screen BITMAP to old Allegro programs, which then is blitted to
 * the new X11 driver on updates. The idea is taken from Trent's D3D driver.
 *
 * Once the 5.0 API is done, this driver will not be needed any longer, as the
 * compatibility layer then will be reduced to a set of wrapper functions
 * implementing the Allegro 4.0 API with 5.0 functions (which has the advantage
 * that it can then benefit from HW acceleration).
 */
GFX_DRIVER _al_xdummy_gfx_driver = {
    0,
    "Dummy X11 compatibility driver",
    "Dummy X11 compatibility driver",
    "Dummy X11 compatibility driver",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    0, 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

void _al_xdummy_display_upload_compat_screen(BITMAP *bitmap, int x, int y, int w, int h)
{
   AL_LOCKED_REGION locked;
   al_lock_bitmap_region(al_get_backbuffer(), x, y, w, h, &locked, 0);
   _al_convert_compat_bitmap(bitmap, locked.data, locked.format, locked.pitch,
      x, y, 0, 0, w, h);
   al_unlock_bitmap(al_get_backbuffer());
}
