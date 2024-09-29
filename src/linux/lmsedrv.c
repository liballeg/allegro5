/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Linux console mouse driver list.
 *
 *      By George Foot.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_driver.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/platform/aintlnx.h"

#ifdef A5O_RASPBERRYPI
#include "allegro5/internal/aintern_raspberrypi.h"
#endif

/* list the available drivers */
_AL_DRIVER_INFO _al_linux_mouse_driver_list[] =
{
   /* These drivers have not been updated for the new mouse API.
    * They may be updated as required, although the evdev driver
    * should be fine on modern kernels --pw
    */
/* {  MOUSEDRV_LINUX_GPMDATA,  &mousedrv_linux_gpmdata,  true  },*/
/* {  MOUSEDRV_LINUX_MS,       &mousedrv_linux_ms,       true  },*/
/* {  MOUSEDRV_LINUX_IMS,      &mousedrv_linux_ims,      true  },*/
/* {  MOUSEDRV_LINUX_PS2,      &mousedrv_linux_ps2,      true  },*/
/* {  MOUSEDRV_LINUX_IPS2,     &mousedrv_linux_ips2,     true  },*/
#if defined A5O_HAVE_LINUX_INPUT_H || defined A5O_RASPBERRYPI
   {  AL_MOUSEDRV_LINUX_EVDEV, &_al_mousedrv_linux_evdev, true  },
#endif
   {  0,                       NULL,                     0     }
};

