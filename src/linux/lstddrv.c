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
 *      Standard driver helpers for Linux Allegro.
 *
 *      By Marek Habersack, mangled by George Foot.
 *
 *      See readme.txt for copyright information.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintunix.h"
#include "linalleg.h"

#include <unistd.h>


/* List of standard drivers */
STD_DRIVER *std_drivers[N_STD_DRIVERS];


static void __async_disable_driver (STD_DRIVER *drv, int mode)
{
   switch (mode) {
      case ASYNC_BSD:
      {
         int   flags = fcntl(drv->fd, F_GETFL, 0);
         fcntl(drv->fd, F_SETFL, flags & ~FASYNC);
         break;
      }
      case ASYNC_OFF:
         break;
   }

   drv->private[PRIV_ENABLED] = 0;
   drv->suspend();
}

static void __async_enable_driver (STD_DRIVER *drv, int mode)
{
   drv->resume();
   drv->private[PRIV_ENABLED] = 1;

   switch (mode) {
      case ASYNC_BSD:
      {
         int flags = fcntl(drv->fd, F_GETFL, 0);
         fcntl(drv->fd, F_SETOWN, getpid());
         fcntl(drv->fd, F_SETFL, flags | FASYNC);
         break;
      }
      case ASYNC_OFF:
         break;
   }
}


/* __al_linux_add_standard_driver:
 *  Adds a standard driver; returns 0 on success, non-zero if the sanity 
 *  checks fail.
 */
int __al_linux_add_standard_driver (STD_DRIVER *spec)
{
   if (!spec) return 1;
   if (spec->type >= N_STD_DRIVERS) return 2;
   if (!spec->update) return 3;
   if (spec->fd < 0) return 4;

   spec->private[PRIV_ENABLED] = 0;
   std_drivers[spec->type] = spec;

   __async_enable_driver (spec, __al_linux_async_io_mode);

   return 0;
}

/* __al_linux_remove_standard_driver:
 *  Removes a standard driver, returning 0 on success or non-zero on
 *  failure.
 */
int __al_linux_remove_standard_driver (STD_DRIVER *spec)
{
   if (!spec) return 1;
   if (!__al_linux_async_io_mode) return 2;
   if (spec->type >= N_STD_DRIVERS) return 3;
   if (!std_drivers[spec->type]) return 4; 
   if (std_drivers[spec->type] != spec) return 5;

   __async_disable_driver (spec, __al_linux_async_io_mode);
   
   std_drivers[spec->type] = NULL;

   return 0;
}


/* __al_linux_update_standard_driver:
 *  Calls the driver's update method.
 */
int __al_linux_update_standard_driver (int type)
{
   if (type >= N_STD_DRIVERS) return -1;
   if (!std_drivers[type]) return -1;

   if (std_drivers[type]->private[PRIV_ENABLED])
      return std_drivers[type]->update();

   return 0;
}


/* __al_linux_update_standard_drivers:
 *  Updates all drivers.
 */
void __al_linux_update_standard_drivers (void)
{
   int i;
   for (i = 0; i < N_STD_DRIVERS; i++)
      if (std_drivers[i] && std_drivers[i]->private[PRIV_ENABLED])
         std_drivers[i]->update();
}


/* __al_linux_async_set_drivers:
 *  Enables/disables drivers, with async mode `mode' at the time --
 *  used when switching between async modes.
 */
void __al_linux_async_set_drivers (int mode, int on_off)
{
   int i;
   for (i = 0; i < N_STD_DRIVERS; i++)
      if (std_drivers[i]) {
         if (on_off)
            __async_enable_driver (std_drivers[i], mode);
         else
            __async_disable_driver (std_drivers[i], mode);
      }
}



void __al_linux_disable_standard_driver (int type)
{
   if (type >= N_STD_DRIVERS) return;
   if (!std_drivers[type]) return;

   __async_disable_driver(std_drivers[type], __al_linux_async_io_mode);
}

void __al_linux_enable_standard_driver (int type)
{
   if (type >= N_STD_DRIVERS) return;
   if (!std_drivers[type]) return;

   __async_enable_driver(std_drivers[type], __al_linux_async_io_mode);
}

