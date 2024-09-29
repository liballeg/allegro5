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
 *      MacOS X clipboard handling.
 *
 *      By Beoran.
 *
 *      See readme.txt for copyright information.
 */


#include <allegro5/allegro.h>
#include <allegro5/allegro_opengl.h>
#include <allegro5/internal/aintern_iphone.h>
#include <allegro5/internal/aintern_opengl.h>
#include <allegro5/internal/aintern_vector.h>
#include <allegro5/internal/aintern.h>
#include <math.h>

#include "iphone.h"
#include "allegroAppDelegate.h"
#include <MobileCoreServices/MobileCoreServices.h>

A5O_DEBUG_CHANNEL("iphone")

#ifndef A5O_IPHONE
#error Something is wrong with the makefile
#endif

/* Ensure that we have the right version number available. */
#ifndef NSAppKitVersionNumber10_6
#define NSAppKitVersionNumber10_6 1038
#endif


static char *iphone_get_clipboard_text(A5O_DISPLAY *display)
{
   const char *utf8;
   NSString *pbtext;
   size_t size;
   char *text;

   pbtext = [[UIPasteboard generalPasteboard] string];
   if (pbtext == nil)
      return NULL;

   utf8 = [pbtext UTF8String];
   size = strlen(utf8);
   text = al_malloc(size+1);
   text = _al_sane_strncpy(text, utf8, size+1);

   return text;
}

static bool iphone_set_clipboard_text(A5O_DISPLAY *display, const char *text)
{
   NSData *data = [NSData dataWithBytes:text length:strlen(text)];
   [[UIPasteboard generalPasteboard] setData:data forPasteboardType:(NSString *)kUTTypeUTF8PlainText];
   return true;
}

static bool iphone_has_clipboard_text(A5O_DISPLAY *display)
{
   NSString *pbtext;

   pbtext = [[UIPasteboard generalPasteboard] string];

   return (pbtext != nil);
}

void _al_iphone_add_clipboard_functions(A5O_DISPLAY_INTERFACE *vt)
{
   vt->set_clipboard_text = iphone_set_clipboard_text;
   vt->get_clipboard_text = iphone_get_clipboard_text;
   vt->has_clipboard_text = iphone_has_clipboard_text;
}

/* vim: set sts=3 sw=3 et: */
