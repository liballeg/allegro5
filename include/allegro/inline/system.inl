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
 *      System inline functions (generic C).
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_SYSTEM_INL
#define ALLEGRO_SYSTEM_INL

#ifdef __cplusplus
   extern "C" {
#endif

#include "allegro/debug.h"


AL_INLINE(void, set_window_title, (char *name),
{
   ASSERT(system_driver);

   if (system_driver->set_window_title)
      system_driver->set_window_title(name);
})


#define ALLEGRO_WINDOW_CLOSE_MESSAGE                                         \
   "Warning: forcing program shutdown may lead to data loss and unexpected " \
   "results. It is preferable to use the exit command inside the window. "   \
   "Proceed anyway?"


AL_INLINE(int, set_window_close_button, (int enable),
{
   ASSERT(system_driver);

   if (system_driver->set_window_close_button)
      return system_driver->set_window_close_button(enable);

   return -1;
})


AL_INLINE(void, set_window_close_hook, (AL_METHOD(void, proc, (void))),
{
   ASSERT(system_driver);

   if (system_driver->set_window_close_hook)
      system_driver->set_window_close_hook(proc);
})


AL_INLINE(int, desktop_color_depth, (void),
{
   ASSERT(system_driver);

   if (system_driver->desktop_color_depth)
      return system_driver->desktop_color_depth();
   else
      return 0;
})


AL_INLINE(int, get_desktop_resolution, (int *width, int *height),
{
   ASSERT(system_driver);

   if (system_driver->get_desktop_resolution)
      return system_driver->get_desktop_resolution(width, height);
   else
      return -1;
})


AL_INLINE(void, yield_timeslice, (void),
{
   ASSERT(system_driver);

   if (system_driver->yield_timeslice)
      system_driver->yield_timeslice();
})


#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_SYSTEM_INL */


