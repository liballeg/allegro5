#include "allegro5/allegro5.h"
#include "allegro5/a5_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"
#include "allegro5/internal/aintern_memory.h"

#import <Cocoa/Cocoa.h>

struct ALLEGRO_NATIVE_FILE_DIALOG
{
   ALLEGRO_PATH *initial_path;
   ALLEGRO_USTR *title;
   ALLEGRO_USTR *patterns;
   int mode;
   int count;
   NSArray *files;
};

/* Function: al_show_native_file_dialog
 * 
 * This functions completely blocks the calling thread until it returns,
 * so usually you may want to spawn a thread with [al_create_thread] and
 * call it from inside that thread.
 */
void al_show_native_file_dialog(ALLEGRO_NATIVE_FILE_DIALOG *fd)
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
      char *path_str;
      size_t size = al_get_path_string_length(fd->initial_path);

      /* Strip filename from path  */
      al_path_set_filename(initial_directory, NULL);

      /* Convert path and filename to NSString objects */
      path_str = malloc(size);
      al_path_to_string(initial_directory, path_str, size, '/');
      directory = [NSString stringWithUTF8String: path_str];
      filename = [NSString stringWithUTF8String: al_path_get_filename(fd->initial_path)];
      free(path_str);
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
   fd->files = nil;
   if (mode & ALLEGRO_FILECHOOSER_SAVE) {    // Save dialog
      NSSavePanel *panel = [NSSavePanel savePanel];

      /* Set file save dialog box options */
      [panel setCanCreateDirectories: YES];
      [panel setCanSelectHiddenExtension: YES];
      [panel setAllowsOtherFileTypes: YES];

      /* Open dialog box */
      if ([panel runModalForDirectory:directory file:filename] == NSOKButton)
         fd->files = [NSArray arrayWithObject: [panel filename]];
      [panel release];
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
      if ([panel runModalForDirectory:directory file:filename] == NSOKButton)
         fd->files = [panel filenames];
      [panel release];
   }

   if (fd->files)
      [fd->files retain];

   [pool release];
}

/* Function: al_create_native_file_dialog
 * 
 * Creates a new native file dialog.
 * 
 * Parameters:
 * - initial_path: The initial search path and filename. Can be NULL.
 * - title: Title of the dialog.
 * - patterns: A list of semi-colon separated patterns to match. You
 *   should always include the pattern "*.*" as usually the MIME type
 *   and not the file pattern is relevant. If no file patterns are
 *   supported by the native dialog, this parameter is ignored.
 * - mode: 0, or a combination of the flags below.
 * 
 * Possible flags for the 'mode' parameter are:
 * 
 * - ALLEGRO_FILECHOOSER_FILE_MUST_EXIST: If supported by the native dialog,
 *   it will not allow entering new names, but just allow existing files
 *   to be selected. Else it is ignored.
 * - ALLEGRO_FILECHOOSER_SAVE: If the native dialog system has a
 *   different dialog for saving (for example one which allows creating
 *   new directories), it is used. Else ignored.
 * - ALLEGRO_FILECHOOSER_FOLDER: If there is support for a separate
 *   dialog to select a folder instead of a file, it will be used.
 * - ALLEGRO_FILECHOOSER_PICTURES: If a different dialog is available
 *   for selecting pictures, it is used. Else ignored.
 * - ALLEGRO_FILECHOOSER_SHOW_HIDDEN: If the platform supports it, also
 *   hidden files will be shown.
 * - ALLEGRO_FILECHOOSER_MULTIPLE: If supported, allow selecting
 *   multiple files.
 *
 * Returns:
 * A handle to the dialog from which you can query the results. When you
 * are done, call [al_destroy_native_file_dialog] on it.
 */
ALLEGRO_NATIVE_FILE_DIALOG *al_create_native_file_dialog(
    ALLEGRO_PATH const *initial_path,
    char const *title,
    char const *patterns,
    int mode)
{
   ALLEGRO_NATIVE_FILE_DIALOG *fd;
   fd = _AL_MALLOC(sizeof *fd);
   memset(fd, 0, sizeof *fd);

   if (initial_path)
      fd->initial_path = al_path_clone(initial_path);
   fd->title = al_ustr_new(title);
   fd->patterns = al_ustr_new(patterns);
   fd->mode = mode;

   return fd;
}

/* al_get_native_file_dialog_count
 */
int al_get_native_file_dialog_count(ALLEGRO_NATIVE_FILE_DIALOG *fd)
{
   return [fd->files count];
}

/* al_get_native_file_dialog_path
 */
ALLEGRO_PATH *al_get_native_file_dialog_path(
   ALLEGRO_NATIVE_FILE_DIALOG *fd, size_t i)
{
   if (i < [fd->files count]) {
      /* NOTE: at first glance, it looks as if this code might leak
       * memory, but in fact it doesn't: the string returned by
       * UTF8String is freed automatically when it goed out of scope
       * (according to the UTF8String docs anyway).
       */
      const char *s = [[fd->files objectAtIndex:i] UTF8String];
      return al_path_create(strdup(s));
   }
   return NULL;
}

/* al_destroy_native_file_dialog
 */
void al_destroy_native_file_dialog(ALLEGRO_NATIVE_FILE_DIALOG *fd)
{
   [fd->files release];
   if (fd->initial_path)
      al_path_free(fd->initial_path);
   al_ustr_free(fd->title);
   al_ustr_free(fd->patterns);
   _AL_FREE(fd);
}

