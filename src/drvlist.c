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
 *      Dynamic driver list helpers.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"



/* count_drivers:
 *  Returns the number of drivers in a list, not including the sentinel.
 */
static int count_drivers(_DRIVER_INFO *drvlist)
{
   _DRIVER_INFO *start = drvlist;
   while (drvlist->driver) drvlist++;
   return drvlist - start;
}



/* _create_driver_list:
 *  Creates a new driver list and returns it. Returns NULL on error.
 */
_DRIVER_INFO *_create_driver_list()
{
   _DRIVER_INFO *drv;
	
   drv = malloc(sizeof(struct _DRIVER_INFO));

   if (drv) {
      drv->id = 0;
      drv->driver = NULL;
      drv->autodetect = FALSE;
   }

   return drv;
}



/* _destroy_driver_list:
 *  Frees a driver list.
 */
void _destroy_driver_list(_DRIVER_INFO *drvlist)
{
   free(drvlist);
}



/* _driver_list_add_driver:
 *  Adds a driver to the end of a driver list. Returns the new driver list.
 */
_DRIVER_INFO *_driver_list_add_driver(_DRIVER_INFO *drvlist, int id, void *driver, int autodetect)
{
   _DRIVER_INFO *drv;
   int c;
    
   ASSERT(drvlist);

   c = count_drivers(drvlist);

   drv = realloc(drvlist, sizeof(_DRIVER_INFO) * (c+2));
   if (!drv)
      return drvlist;

   drv[c].id = id;
   drv[c].driver = driver;
   drv[c].autodetect = autodetect;
   drv[c+1].id = 0;
   drv[c+1].driver = NULL;
   drv[c+1].autodetect = FALSE;
   return drv;
}



/* _driver_list_add_list:
 *  Add drivers from another list, and return the new list.
 */
_DRIVER_INFO *_driver_list_add_list(_DRIVER_INFO *drvlist, _DRIVER_INFO *srclist)
{
   ASSERT(drvlist);
   ASSERT(srclist);

   while (srclist->driver) {
      drvlist = _driver_list_add_driver(drvlist, srclist->id, srclist->driver, srclist->autodetect);
      srclist++;
   }

   return drvlist;
}
