#include "allegro5/allegro5.h"
#include "allegro5/a5_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"
#include "allegro5/internal/aintern_memory.h"

#import <Cocoa/Cocoa.h>

void al_show_native_file_dialog(ALLEGRO_NATIVE_DIALOG *fd)
{
   int mode = fd->mode;
   NSString *directory, *filename;

   /* Since this is designed to be run from a separate thread, we setup
    * release pool, or we get memory leaks
    */
   NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

   /* Set initial directory to pass to the file selector */
   if (fd->initial_path) {
      ALLEGRO_PATH *initial_directory = al_path_clone(fd->initial_path);
      /* Strip filename from path  */
      al_path_set_filename(initial_directory, NULL);

      /* Convert path and filename to NSString objects */
      directory = [NSString stringWithUTF8String: al_path_to_string(initial_directory, '/')];
      filename = [NSString stringWithUTF8String: al_path_get_filename(fd->initial_path)];
      al_path_free(initial_directory);
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
         fd->count = 1;
         fd->paths = _AL_MALLOC(fd->count * sizeof *fd->paths);
         fd->paths[0] = al_path_create(s);
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
         [panel setAllowsMultipleSelection:NO];

      /* Open dialog box */
      if ([panel runModalForDirectory:directory file:filename] == NSOKButton) {
         size_t i;
         fd->count = [[panel filenames] count];
         fd->paths = _AL_MALLOC(fd->count * sizeof *fd->paths);
         for (i = 0; i < fd->count; i++) {
            /* NOTE: at first glance, it looks as if this code might leak
             * memory, but in fact it doesn't: the string returned by
             * UTF8String is freed automatically when it goes out of scope
             * (according to the UTF8String docs anyway).
             */
            const char *s = [[[panel filenames] objectAtIndex: i] UTF8String];
            fd->paths[i] = al_path_create(s);
         }
      }
   }

   [pool drain];
}

int _al_show_native_message_box(ALLEGRO_NATIVE_DIALOG *fd)
{
   unsigned i;

   /* Since this might be run from a separate thread, we setup
    * release pool, or we get memory leaks
    */
   NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

   NSString* button_text;
   NSArray* buttons;
   NSAlert* box = [[NSAlert alloc] init];
   NSAlertStyle style;

   if (fd->buttons == NULL) {
      button_text = @"OK";
      if (fd->mode & ALLEGRO_MESSAGEBOX_YES_NO) button_text = @"Yes|No";
      if (fd->mode & ALLEGRO_MESSAGEBOX_OK_CANCEL) button_text = @"OK|Cancel";
   }
   else {
      button_text = [NSString stringWithUTF8String: al_cstr(fd->buttons)];
   }

   style = NSWarningAlertStyle;
   buttons = [button_text componentsSeparatedByString: @"|"];
   [box setMessageText:[NSString stringWithUTF8String: al_cstr(fd->title)]];
   [box setInformativeText:[NSString stringWithUTF8String: al_cstr(fd->text)]];
   [box setAlertStyle: style];
   for (i = 0; i < [buttons count]; ++i)
      [box addButtonWithTitle: [buttons objectAtIndex: i]];

   fd->pressed_button = [box runModal] + 1 - NSAlertFirstButtonReturn;

   [pool drain];
   return fd->pressed_button;
}
