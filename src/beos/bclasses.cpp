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
 *      Stuff for BeOS.
 *
 *      By Jason Wilkins.
 *
 *      See readme.txt for copyright information.
 */

#include "bealleg.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintbeos.h"

#ifndef ALLEGRO_BEOS
#error something is wrong with the makefile
#endif



/* BeAllegroView::BeAllegroView:
 */
BeAllegroView::BeAllegroView(BRect frame, const char *name,
   uint32 resizingMode, uint32 flags)
   : BView(frame, name, resizingMode, flags)
{
}



/* BeAllegroView::~BeAllegroView:
 */
BeAllegroView::~BeAllegroView()
{
}



/* BeAllegroView::AttachedToWindow:
 */
void BeAllegroView::AttachedToWindow()
{
}



/* BeAllegroView::MessageReceived:
 */
void BeAllegroView::MessageReceived(BMessage *message)
{
   switch (message->what) {
      case B_SIMPLE_DATA:
         break;

      default:
         BView::MessageReceived(message);
         break;
   }
}
