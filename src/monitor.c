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
 *      Monitor queries.
 *
 *      See LICENSE.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_system.h"


/* Function: al_get_num_video_adapters
 */
int al_get_num_video_adapters(void)
{
   A5O_SYSTEM *system = al_get_system_driver();

   if (system && system->vt && system->vt->get_num_video_adapters) {
      return system->vt->get_num_video_adapters();
   }

   return 0;
}

/* Function: al_get_monitor_refresh_rate
 */
int al_get_monitor_refresh_rate(int adapter)
{
   A5O_SYSTEM *system = al_get_system_driver();

   if (adapter < al_get_num_video_adapters()) {
      if (system && system->vt && system->vt->get_monitor_refresh_rate) {
         return system->vt->get_monitor_refresh_rate(adapter);
      }
   }

   return 0;
}


/* Function: al_get_monitor_info
 */
bool al_get_monitor_info(int adapter, A5O_MONITOR_INFO *info)
{
   A5O_SYSTEM *system = al_get_system_driver();

   if (adapter < al_get_num_video_adapters()) {
      if (system && system->vt && system->vt->get_monitor_info) {
         return system->vt->get_monitor_info(adapter, info);
      }
   }

   info->x1 = info->y1 = info->x2 = info->y2 = INT_MAX;
   return false;
}

/* Function: al_get_monitor_dpi
 */
int al_get_monitor_dpi(int adapter)
{
   A5O_SYSTEM *system = al_get_system_driver();

   if (adapter < al_get_num_video_adapters()) {
       if (system && system->vt && system->vt->get_monitor_dpi) {
           return system->vt->get_monitor_dpi(adapter);
       }
   }

   return 0;
}

/* vim: set sts=3 sw=3 et: */
