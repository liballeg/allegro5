#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"

#import <Cocoa/Cocoa.h>

bool _al_init_native_dialog_addon(void)
{
   return true;
}

void _al_shutdown_native_dialog_addon(void)
{
}


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

/* Wrapper to run NSAlert on main thread */
@interface NSAlertWrapper : NSObject {
@public
   int retval;
}
-(void) go : (NSAlert *)param;
@end
@implementation NSAlertWrapper
-(void) go : (NSAlert *) param {
   retval = [param runModal];
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

   _al_osx_clear_mouse_state();

   return true;
}

int _al_show_native_message_box(ALLEGRO_DISPLAY *display,
   ALLEGRO_NATIVE_DIALOG *fd)
{
   unsigned i;

   (void)display;

   /* Note: the message box code cannot assume that Allegro is installed. */

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

   NSAlertWrapper *wrap = [[NSAlertWrapper alloc] init];
   [wrap performSelectorOnMainThread: @selector(go:) withObject: box
      waitUntilDone: YES];
   fd->mb_pressed_button = wrap->retval + 1 - NSAlertFirstButtonReturn;
   [wrap release];

   [pool drain];

   _al_osx_clear_mouse_state();

   return fd->mb_pressed_button;
}


#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
@interface LogView : NSTextView <NSWindowDelegate>
#else
@interface LogView : NSTextView
#endif
{
@public
	ALLEGRO_NATIVE_DIALOG *textlog;
}
- (void)keyDown: (NSEvent*)event;
- (BOOL)windowShouldClose: (id)sender;
- (void)emitCloseEventWithKeypress: (BOOL)keypress;
+ (void)appendText: (NSValue*)param;
@end


@implementation LogView


- (void)keyDown: (NSEvent*)event
{
   if (([event keyCode] == 0x35) ||                                                   // Escape
       (([event keyCode] == 0x0D) && ([event modifierFlags] & NSCommandKeyMask))) {   // Command+W
      [[self window] close];
      [self emitCloseEventWithKeypress: YES];
   }
   else {
      [super keyDown: event];
   }
}

- (BOOL)windowShouldClose: (id)sender
{
   (void)sender;
   if (self->textlog->is_active) {
      if (!(self->textlog->flags & ALLEGRO_TEXTLOG_NO_CLOSE)) {
         [self emitCloseEventWithKeypress: NO];
      }
   }
   return YES;
}

- (void)emitCloseEventWithKeypress: (BOOL)keypress
{
   ALLEGRO_EVENT event;
   event.user.type = ALLEGRO_EVENT_NATIVE_DIALOG_CLOSE;
   event.user.timestamp = al_get_time();
   event.user.data1 = (intptr_t)self->textlog;
   event.user.data2 = (intptr_t)keypress;
   al_emit_user_event(&self->textlog->tl_events, &event, NULL);
}

+ (void)appendText: (NSValue*)param
{
   ALLEGRO_NATIVE_DIALOG *textlog = (ALLEGRO_NATIVE_DIALOG*)[param pointerValue];
   al_lock_mutex(textlog->tl_text_mutex);
   if (textlog->is_active) {
      LogView *view = (LogView *)textlog->tl_textview;
      NSString *text = [[NSString alloc] initWithUTF8String: al_cstr(textlog->tl_pending_text)];
      NSRange range = NSMakeRange([[view string] length], 0);
      
      [view setEditable: YES];
      [view setSelectedRange: range];
      [view insertText: text];
      [view setEditable: NO];
      [text release];
      al_ustr_truncate(textlog->tl_pending_text, 0);
      textlog->tl_have_pending = false;
   }
   al_unlock_mutex(textlog->tl_text_mutex);
}

@end


bool _al_open_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
   NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
   
   al_lock_mutex(textlog->tl_text_mutex);
   
   NSRect rect = NSMakeRect(0, 0, 640, 480);
   NSWindow *win = [NSWindow alloc];
   int adapter = al_get_new_display_adapter();
   NSScreen *screen;
   unsigned int mask = NSTitledWindowMask|NSMiniaturizableWindowMask|NSResizableWindowMask;
   if (!(textlog->flags & ALLEGRO_TEXTLOG_NO_CLOSE))
      mask |= NSClosableWindowMask;
   
   if ((adapter >= 0) && (adapter < al_get_num_video_adapters())) {
      screen = [[NSScreen screens] objectAtIndex: adapter];
   } else {
      screen = [NSScreen mainScreen];
   }
   [win initWithContentRect: rect
                  styleMask: mask
                    backing: NSBackingStoreBuffered
                      defer: NO
                     screen: screen];
   [win setReleasedWhenClosed: NO];
   [win setTitle: @"Allegro Text Log"];
   [win setMinSize: NSMakeSize(128, 128)];
   
   NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame: rect];
   [scrollView setHasHorizontalScroller: YES];
   [scrollView setHasVerticalScroller: YES];
   [scrollView setAutohidesScrollers: YES];
   [scrollView setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable];
   
   [[scrollView contentView] setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable];
   [[scrollView contentView] setAutoresizesSubviews: YES];
   
   rect = [[scrollView contentView] frame];
   LogView *view = [[LogView alloc] initWithFrame: rect];
   view->textlog = textlog;
   [view setHorizontallyResizable: YES];
   [view setVerticallyResizable: YES];
   [view setAutoresizingMask: NSViewHeightSizable | NSViewWidthSizable];
   [[view textContainer] setContainerSize: NSMakeSize(rect.size.width, 1000000)];
   [[view textContainer] setWidthTracksTextView: NO];
   [view setTextColor: [NSColor grayColor]];
   if (textlog->flags & ALLEGRO_TEXTLOG_MONOSPACE)
      [view setFont: [NSFont userFixedPitchFontOfSize: 0]];
   [view setEditable: NO];
   [scrollView setDocumentView: view];
   
   [[win contentView] addSubview: scrollView];
   [scrollView release];
   
   [win setDelegate: view];
   [win orderFront: nil];
   
   /* Save handles for future use. */
   textlog->window = win;
   textlog->tl_textview = view;
   textlog->is_active = true;

   /* Now notify al_show_native_textlog that the text log is ready. */
   textlog->tl_done = true;
   al_signal_cond(textlog->tl_text_cond);
   al_unlock_mutex(textlog->tl_text_mutex);
   
   while ([win isVisible]) {
      al_rest(0.05);
   }
   
   al_lock_mutex(textlog->tl_text_mutex);
   _al_close_native_text_log(textlog);
   al_unlock_mutex(textlog->tl_text_mutex);
   
   [pool drain];
   return true;
}

void _al_close_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   NSWindow *win = (NSWindow *)textlog->window;
   if ([win isVisible]) {
      [win close];
   }
   
   /* Notify everyone that we're gone. */
   textlog->is_active = false;
   textlog->tl_done = true;
   al_signal_cond(textlog->tl_text_cond);
   [pool drain];
}

void _al_append_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   if (textlog->tl_have_pending) {
      [pool drain];
      return;
   }
   textlog->tl_have_pending = true;
   
   [LogView performSelectorOnMainThread: @selector(appendText:) 
                             withObject: [NSValue valueWithPointer: textlog]
                          waitUntilDone: NO];
   [pool drain];
}

