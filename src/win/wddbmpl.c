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
 *      DirectDraw video bitmap list.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#include "wddraw.h"



BMP_EXTRA_INFO *directx_bmp_list = NULL;



/* register_directx_bitmap:
 *  add surface to the linked list
 */
void register_directx_bitmap(struct BITMAP *bmp)
{
   BMP_EXTRA_INFO *item;

   if (bmp) {
      _enter_critical();

      /* create item */
      item = BMP_EXTRA(bmp);
      if (item) {
	 /* add to list */
	 item->next = directx_bmp_list;
	 item->prev = NULL;
	 if (directx_bmp_list)
	    directx_bmp_list->prev = item;
	 directx_bmp_list = item;
      }

      _exit_critical();
   }
}



/* unregister_directx_bitmap:
 *  remove surface from linked list
 */
void unregister_directx_bitmap(struct BITMAP *bmp)
{
   BMP_EXTRA_INFO *item;
   BMP_EXTRA_INFO *searched;

   if (bmp) {
      /* find item */
      _enter_critical();

      item = directx_bmp_list;
      searched = BMP_EXTRA(bmp);
      if (searched) {
	 while (item) {
	    if (item == searched) {
	       /* surface found, unlink now */
	       if (item->next)
		  item->next->prev = item->prev;
	       if (item->prev)
		  item->prev->next = item->next;
	       if (directx_bmp_list == item)
		  directx_bmp_list = item->next;

	       item->next = NULL;
	       item->prev = NULL;

	       _exit_critical();
	       return;
	    }

	    item = item->next;
	 }
      }

      _exit_critical();
   }
}



/* unregister_all_directx_bitmaps:
 *  remove all surfaces from linked list
 */
void unregister_all_directx_bitmaps(void)
{
   BMP_EXTRA_INFO *item;
   BMP_EXTRA_INFO *next_item;

   _enter_critical();

   next_item = directx_bmp_list;
   while (next_item) {
      item = next_item;
      next_item = next_item->next;
      item->next = NULL;
      item->prev = NULL;
   }

   directx_bmp_list = NULL;

   _exit_critical();
}
