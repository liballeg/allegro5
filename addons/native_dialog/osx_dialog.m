#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"

#import <Cocoa/Cocoa.h>

/* We need to run the dialog box on the main thread because AppKit is not
 * re-entrant and running it from another thread can cause unpredictable
 * crashes.
 * We use a dedicated class for this and simply forward the call.
 * The textbox is apparently fine the way it is.
 */
@interface FileDialog : NSObject
+(void) show : (NSValue *)param;
@end
@implementation FileDialog
+(void) show : (NSValue *) param {
   ALLEGRO_NATIVE_DIALOG *fd = [param pointerValue];
   int mode = fd->flags;
   NSString *directory, *filename;

   /* Set initial directory to pass to the file selector */
   if (fd->fc_initial_path) {
      ALLEGRO_PATH *initial_directory = al_clone_path(fd->fc_initial_path);
      /* Strip filename from path  */
      al_set_path_filename(initial_directory, NULL);

      /* Convert path and filename to NSString objects */
      directory = [NSString stringWithUTF8String: al_path_cstr(initial_directory, '/')];
      filename = [NSString stringWithUTF8String: al_get_path_filename(fd->fc_initial_path)];
      al_destroy_path(initial_directory);
   } else {
      directory = nil;
      filename = nil;
   }

   /* We need slightly different code for SAVE and LOAD dialog boxes, which
    * are handled by slightly different classes.
    * Actually, NSOpenPanel inherits from NSSavePanel, so we can possibly
    * avoid some code cuplication here...
    */
   if (mode & ALLEGRO_FILECHOOSER_SAVE) {    // Save dialog
      NSSavePanel *panel = [NSSavePanel savePanel];

      /* Set file save dialog box options */
      [panel setCanCreateDirectories: YES];
      [panel setCanSelectHiddenExtension: YES];
      [panel setAllowsOtherFileTypes: YES];

      /* Open dialog box */
      if ([panel runModalForDirectory:directory file:filename] == NSOKButton) {
         /* NOTE: at first glance, it looks as if this code might leak
          * memory, but in fact it doesn't: the string returned by
          * UTF8String is freed automatically when it goes out of scope
          * (according to the UTF8String docs anyway).
          */
         const char *s = [[panel filename] UTF8String];
         fd->fc_path_count = 1;
         fd->fc_paths = al_malloc(fd->fc_path_count * sizeof *fd->fc_paths);
         fd->fc_paths[0] = al_create_path(s);
      }
   } else {                                  // Open dialog
      NSOpenPanel *panel = [NSOpenPanel openPanel];

      /* Set file selection box options */
      if (mode & ALLEGRO_FILECHOOSER_FOLDER) {
         [panel setCanChooseFiles: NO];
         [panel setCanChooseDirectories: YES];
      } else {
         [panel setCanChooseFiles: YES];
         [panel setCanChooseDirectories: NO];
      }

      [panel setResolvesAliases:YES];
      if (mode & ALLEGRO_FILECHOOSER_MULTIPLE)
         [panel setAllowsMultipleSelection: YES];
      else
         [panel setAllowsMultipleSelection: NO];

      /* Open dialog box */
      if ([panel runModalForDirectory:directory file:filename] == NSOKButton) {
         size_t i;
         fd->fc_path_count = [[panel filenames] count];
         fd->fc_paths = al_malloc(fd->fc_path_count * sizeof *fd->fc_paths);
         for (i = 0; i < fd->fc_path_count; i++) {
            /* NOTE: at first glance, it looks as if this code might leak
             * memory, but in fact it doesn't: the string returned by
             * UTF8String is freed automatically when it goes out of scope
             * (according to the UTF8String docs anyway).
             */
            const char *s = [[[panel filenames] objectAtIndex: i] UTF8String];
            fd->fc_paths[i] = al_create_path(s);
         }
      }
   }
}
@end


bool _al_show_native_file_dialog(ALLEGRO_DISPLAY *display,
   ALLEGRO_NATIVE_DIALOG *fd)
{
   (void)display;

   /* Since this function may be called from a separate thread (our own
    * example program does this), we need to setup a release pool, or we
    * get memory leaks.
    */
   NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
   [FileDialog performSelectorOnMainThread: @selector(show:) 
                                withObject: [NSValue valueWithPointer:fd]
                             waitUntilDone: YES];
   [pool drain];
   return true;
}

int _al_show_native_message_box(ALLEGRO_DISPLAY *display,
   ALLEGRO_NATIVE_DIALOG *fd)
{
   unsigned i;

   (void)display;

   /* Since this might be run from a separate thread, we setup
    * release pool, or we get memory leaks
    */
   NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

   NSString* button_text;
   NSArray* buttons;
   NSAlert* box = [[NSAlert alloc] init];
   NSAlertStyle style;
   [box autorelease];
   if (fd->mb_buttons == NULL) {
      button_text = @"OK";
      if (fd->flags & ALLEGRO_MESSAGEBOX_YES_NO) button_text = @"Yes|No";
      if (fd->flags & ALLEGRO_MESSAGEBOX_OK_CANCEL) button_text = @"OK|Cancel";
   }
   else {
      button_text = [NSString stringWithUTF8String: al_cstr(fd->mb_buttons)];
   }

   style = NSWarningAlertStyle;
   buttons = [button_text componentsSeparatedByString: @"|"];
   [box setMessageText:[NSString stringWithUTF8String: al_cstr(fd->title)]];
   [box setInformativeText:[NSString stringWithUTF8String: al_cstr(fd->mb_text)]];
   [box setAlertStyle: style];
   for (i = 0; i < [buttons count]; ++i)
      [box addButtonWithTitle: [buttons objectAtIndex: i]];

   fd->mb_pressed_button = [box runModal] + 1 - NSAlertFirstButtonReturn;

   [pool drain];
   return fd->mb_pressed_button;
}
