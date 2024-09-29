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


#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_driver.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/internal/aintern_osxclipboard.h"
#include "allegro5/platform/aintosx.h"
#include "./osxgl.h"

#ifndef A5O_MACOSX
#error Something is wrong with the makefile
#endif

/* Ensure that we have the right version number available. */
#ifndef NSAppKitVersionNumber10_6
#define NSAppKitVersionNumber10_6 1038
#endif

static NSString *osx_get_text_format(A5O_DISPLAY *display)
{
   (void) display;

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
   return NSStringPboardType;
#else
   /* This is now the preferred way of version checking,
      Gestalt has been deprecated.
   */
   if ( floor(NSAppKitVersionNumber) < NSAppKitVersionNumber10_6 ) {
      return NSPasteboardTypeString;
   }
   else {
      return NSStringPboardType;
   }
#endif
}

static bool osx_set_clipboard_text(A5O_DISPLAY *display, const char *text)
{
   NSAutoreleasePool *pool;
   NSPasteboard *pasteboard;
   NSString *format = osx_get_text_format(display);
   BOOL ok;

   pool = [[NSAutoreleasePool alloc] init];
   if (!pool) return false;

   pasteboard = [NSPasteboard generalPasteboard];
   if (!pasteboard) return false;
   /* First clear the clipboard, otherwise the setString will fail. */
   [pasteboard clearContents];
   ok = [pasteboard setString:[NSString stringWithUTF8String:text] forType:format];

   [pool release];

   return ok == YES;
}

static char * osx_get_clipboard_text(A5O_DISPLAY *display)
{
   NSAutoreleasePool *pool;
   NSPasteboard *pasteboard;
   NSString *format = osx_get_text_format(display);
   NSString *available;
   char *text;

   pool = [[NSAutoreleasePool alloc] init];

   pasteboard = [NSPasteboard generalPasteboard];
   available = [pasteboard availableTypeFromArray: [NSArray arrayWithObject:format]];
   if ([available isEqualToString:format]) {
      NSString* string;
      const char *utf8;

      string = [pasteboard stringForType:format];
      if (string == nil) {
         text = NULL;
      } else {
         size_t size;
         utf8 = [string UTF8String];
         size = strlen(utf8);
         text = al_malloc(size+1);
         text = _al_sane_strncpy(text, utf8, size+1);
      }
   } else {
      text = NULL;
   }

   [pool release];

   return text;
}

static bool osx_has_clipboard_text(A5O_DISPLAY *display)
{
   NSAutoreleasePool *pool;
   NSPasteboard *pasteboard;
   NSString *format = osx_get_text_format(display);
   NSString *available;
   bool result = false;

   pool = [[NSAutoreleasePool alloc] init];

   pasteboard = [NSPasteboard generalPasteboard];
   available = [pasteboard availableTypeFromArray: [NSArray arrayWithObject:format]];

   if ([available isEqualToString:format]) {
      NSString* string;
      string = [pasteboard stringForType:format];
      if (string != nil) {
         result = true;
      }
   }

   [pool release];

   return result;
}

void _al_osx_add_clipboard_functions(A5O_DISPLAY_INTERFACE *vt)
{
   vt->set_clipboard_text = osx_set_clipboard_text;
   vt->get_clipboard_text = osx_get_clipboard_text;
   vt->has_clipboard_text = osx_has_clipboard_text;
}

/* vim: set sts=3 sw=3 et: */
