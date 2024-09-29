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
 *      Clipboard handling.
 *
 *      By Beoran.
 *
 *      See readme.txt for copyright information.
 */

/* Title: Event sources
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_display.h"


/* Function: al_get_clipboard_text
 */
char *al_get_clipboard_text(A5O_DISPLAY *display)
{
   if (!display)
      display = al_get_current_display();

   if (!display)
      return NULL;

   if (!display->vt->get_clipboard_text)
      return NULL;

   return display->vt->get_clipboard_text(display);
}


/* Function: al_set_clipboard_text
 */
bool al_set_clipboard_text(A5O_DISPLAY *display, const char *text)
{
   if (!display)
      display = al_get_current_display();

   if (!display)
      return false;

   if (!display->vt->set_clipboard_text)
      return false;

   return display->vt->set_clipboard_text(display, text);

}


/* Function: al_clipboard_has_text
 */
bool al_clipboard_has_text(A5O_DISPLAY *display)
{
   if (!display)
      display = al_get_current_display();

   if (!display)
      return false;

   if (!display->vt->has_clipboard_text)
      return false;

  return display->vt->has_clipboard_text(display);
}


/* vim: set sts=3 sw=3 et: */
