#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"

#import <Cocoa/Cocoa.h>

/* Normally we should include aintosx.h to get this prototype but it doesn't 
 * compile. So repeat the prototype here as a stopgap measure.
 */
void _al_osx_clear_mouse_state(void);

/* Declare this here to avoid getting a warning. 
 * Unfortunately, there seems to be no better way to do this...
 */
@interface NSApplication(AllegroOSX)
- (void)setAppleMenu:(NSMenu *) menu;
@end

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

/* Menus */

static int get_accelerator(ALLEGRO_USTR *caption, char buf[200])
{
   int amp_pos = al_ustr_find_chr(caption, 0, '&');
   if (amp_pos >= 0) {
      strncpy(buf, al_cstr(caption), amp_pos);
      strncpy(buf+amp_pos, al_cstr(caption)+amp_pos+1, 200-amp_pos);
      return amp_pos;
   }
   else {
      int underscore_pos = al_ustr_find_chr(caption, 0, '_');
      if (underscore_pos >= 0) {
         strncpy(buf, al_cstr(caption), underscore_pos);
         strncpy(buf+underscore_pos, al_cstr(caption)+underscore_pos+1, 200-underscore_pos);
    return underscore_pos;
      }
      else {
         strncpy(buf, al_cstr(caption), 200);
      }
   }

   return -1;
}

static void update_checked_state(NSMenuItem *item, bool checked)
{
      if (checked) {
         [item setState:NSOnState];
      }
      else {
         [item setState:NSOffState];
      }
}

@interface Runner : NSObject
- (void) create_popup:(id)obj;
@end

@implementation Runner
- (void) create_popup:(id)obj
{
   NSMenu *main_menu = (NSMenu *)obj;

   NSWindow *window = [NSApp keyWindow];
   NSPoint mouseLocation = [NSEvent mouseLocation];
   NSPoint locationInWindow = [window convertScreenToBase: mouseLocation];
   int eventType = NSLeftMouseDown;
   int winnum = [window windowNumber];
   NSEvent *fakeMouseEvent = [NSEvent mouseEventWithType:eventType
      location:locationInWindow
      modifierFlags:0
      timestamp:0
      windowNumber:winnum
      context:nil
      eventNumber:0
      clickCount:0
      pressure:0];

   [NSMenu popUpContextMenu:main_menu withEvent:fakeMouseEvent forView:[window contentView]];
}
@end

@class MenuDelegate;

typedef struct MENU_ITEM_THINGS {
   MenuDelegate *menu_delegate;
   NSMenuItem *menu_item;
   NSMenu *menu;
} MENU_ITEM_THINGS;

typedef struct MENU_THINGS {
   NSMenu *menu;
   _AL_VECTOR items;
   ALLEGRO_DISPLAY *display;
   ALLEGRO_MENU *toplevel_menu;
} MENU_THINGS;

typedef struct DISPLAY_INFO {
   ALLEGRO_DISPLAY *display;
   ALLEGRO_MENU *toplevel_menu;
} DISPLAY_INFO;

@interface MenuDelegate : NSObject {
@public
   ALLEGRO_MENU_ITEM *item;
}
- (void) activated;
- (NSMenuItem *) build_menu_item:(ALLEGRO_MENU_ITEM *)aitem;
@end

@implementation MenuDelegate : NSObject
- (void) activated
{
   if (item->flags & ALLEGRO_MENU_ITEM_CHECKBOX) {
      if (item->flags & ALLEGRO_MENU_ITEM_CHECKED) {
         item->flags &= ~ALLEGRO_MENU_ITEM_CHECKED;
      }
      else {
         item->flags |= ALLEGRO_MENU_ITEM_CHECKED;
      }
      MENU_ITEM_THINGS  *mit = item->extra1;
      update_checked_state(mit->menu_item, item->flags & ALLEGRO_MENU_ITEM_CHECKED);
   }
   if (item->parent)
      _al_emit_menu_event(item->parent->display, item->id);
}

- (NSMenuItem *) build_menu_item:(ALLEGRO_MENU_ITEM *)aitem
{
   char buf[200];
   char key[5] = { 0, };
   int amp_pos = get_accelerator(aitem->caption, buf);
   if (amp_pos == -1) {
      key[0] = 0;
   }
   else {
      *key = al_ustr_get(aitem->caption, amp_pos+1);
   }
   NSMenuItem *menu_item = [[[NSMenuItem alloc]
      initWithTitle:[[NSString alloc] initWithUTF8String:buf]
      action:@selector(activated)
      keyEquivalent:[[NSString alloc] initWithUTF8String:key]]
      autorelease
   ];
   [menu_item setTarget:self];

   return menu_item;
}
@end

static _AL_VECTOR menus = _AL_VECTOR_INITIALIZER(ALLEGRO_MENU *);
static _AL_VECTOR displays = _AL_VECTOR_INITIALIZER(DISPLAY_INFO *);
static volatile ALLEGRO_EVENT_QUEUE *queue = NULL;
static ALLEGRO_MUTEX *mutex;



#define add_menu(name, sel, eq)                                          \
        [menu addItem: [[[NSMenuItem alloc]   \
                                    initWithTitle: name                  \
                                           action: @selector(sel)        \
                                    keyEquivalent: eq] autorelease]]

static NSMenu *init_apple_menu(void)
{
      NSMenu *menu;
      NSMenuItem *temp_item;

      NSString* title = nil;
      NSDictionary* app_dictionary = [[NSBundle mainBundle] infoDictionary];
      if (app_dictionary) {
          title = [app_dictionary objectForKey: @"CFBundleName"];
      }
      if (title == nil) {
          title = [[NSProcessInfo processInfo] processName];
      }

      NSMenu *main_menu = [[NSMenu alloc] initWithTitle: @""];

      /* Add application ("Apple") menu */
      menu = [[NSMenu alloc] initWithTitle: @"Apple menu"];
      temp_item = [[NSMenuItem alloc]
              initWithTitle: @""
              action: NULL
              keyEquivalent: @""];
      [main_menu addItem:temp_item];
      [main_menu setSubmenu:menu forItem:temp_item];
      [temp_item release];
      add_menu([@"Hide " stringByAppendingString: title], hide:, @"h");
      add_menu(@"Hide Others", hideOtherApplications:, @"");
      add_menu(@"Show All", unhideAllApplications:, @"");
      [menu addItem:[NSMenuItem separatorItem]];
      add_menu([@"Quit " stringByAppendingString: title], terminate:, @"q");
      [NSApp setAppleMenu:menu];
      [menu release];

      return [main_menu autorelease];
}

static void *event_thread(void *unused)
{
   (void)unused;

   mutex = al_create_mutex_recursive();
   queue = al_create_event_queue();

   while (true) {
      ALLEGRO_EVENT event;
      al_wait_for_event((ALLEGRO_EVENT_QUEUE *)queue, &event);
      if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_IN) {
         int i;
         al_lock_mutex(mutex);
         for (i = 0; i < (int)displays._size; i++) {
            DISPLAY_INFO *d = *(DISPLAY_INFO **)_al_vector_ref(&displays, i);
            if (d->display == event.display.source) {
               /* The rest gives time for the window to be fully focussed */
               al_rest(0.1);
               _al_show_display_menu(d->display, d->toplevel_menu);
               break;
            }
         }
         al_unlock_mutex(mutex);
      }
   }

   al_destroy_mutex(mutex);
   mutex = NULL;
   al_destroy_event_queue((ALLEGRO_EVENT_QUEUE *)queue);
   queue = NULL;
   return NULL;
}

static  void ensure_event_thread(void)
{
   if (queue)
      return;

   al_run_detached_thread(event_thread, NULL);

   while (!queue)
      al_rest(0.001);
}

bool _al_init_menu(ALLEGRO_MENU *amenu)
{
   ensure_event_thread();

   ALLEGRO_MENU **ptr = _al_vector_alloc_back(&menus);
   *ptr = amenu;

   return true;
}

bool _al_init_popup_menu(ALLEGRO_MENU *menu)
{
   return _al_init_menu(menu);
}

bool _al_insert_menu_item_at(ALLEGRO_MENU_ITEM *aitem, int i)
{
   ALLEGRO_MENU *amenu = aitem->parent;
   MENU_THINGS *mt = amenu->extra1;

   if (!mt) {
      amenu->extra1 = al_calloc(1, sizeof(MENU_THINGS));
      mt = amenu->extra1;
      _al_vector_init(&mt->items, sizeof(ALLEGRO_MENU_ITEM *));
   }

   ALLEGRO_MENU_ITEM **ptr;

   if (i >= (int)mt->items._size) {
      ptr = _al_vector_alloc_back(&mt->items);
   }
   else {
      ptr = _al_vector_alloc_mid(&mt->items, i);
   }

   *ptr = aitem;

   if (mt->display && mt->toplevel_menu) {
      _al_show_display_menu(mt->display, mt->toplevel_menu);
   }

   return true;
}

bool _al_destroy_menu_item_at(ALLEGRO_MENU_ITEM *item, int i)
{
   (void)i;
   MENU_ITEM_THINGS *mit = item->extra1;
   NSMenu *menu = mit->menu;
   NSMenuItem *menu_item = mit->menu_item;
   [menu removeItem:menu_item];
   MENU_THINGS *mt = item->parent->extra1;
   _al_vector_find_and_delete(&mt->items, &item);
   al_free(mit);
   item->extra1 = NULL;
   return true;
}

bool _al_update_menu_item_at(ALLEGRO_MENU_ITEM *item, int i)
{
   (void)i;

   MENU_ITEM_THINGS *mit = item->extra1;

   if (mit == NULL) {
      return true;
   }

   NSMenuItem *menu_item = mit->menu_item;

   char buf[200];
   get_accelerator(item->caption, buf);
   [menu_item setTitle:[NSString stringWithUTF8String:buf]];

   if (item->flags & ALLEGRO_MENU_ITEM_CHECKBOX) {
      update_checked_state(mit->menu_item, item->flags & ALLEGRO_MENU_ITEM_CHECKED);
   }

   [menu_item setEnabled:((item->flags & ALLEGRO_MENU_ITEM_DISABLED) ? FALSE : TRUE)];

   return true;
}

static void add_items(NSMenu *menu, ALLEGRO_DISPLAY *display, ALLEGRO_MENU *amenu, ALLEGRO_MENU *the_menu, bool popup)
{
   int i;
   MENU_THINGS *mt = the_menu->extra1;
   char buf[200];
   char key[5];
   int amp_pos;
   NSMenu *temp_menu;

   [NSApp activateIgnoringOtherApps:YES];

   /* recursively add menu items */
   for (i = 0; i < (int)mt->items._size; i++) {
      ALLEGRO_MENU_ITEM *aitem = *(ALLEGRO_MENU_ITEM **)_al_vector_ref(&mt->items, i);
      if (aitem->caption == NULL) {
         [menu addItem:[NSMenuItem separatorItem]];
      }
      else {
         amp_pos = get_accelerator(aitem->caption, buf);
         if (popup) {
            if (amp_pos == -1) {
               key[0] = 0;
            }
            else {
               *key = al_ustr_get(aitem->caption, amp_pos+1);
            }
            MenuDelegate *menu_delegate = [[MenuDelegate alloc] init];
            NSMenuItem *menu_item = [menu_delegate build_menu_item:aitem];
            menu_delegate->item = aitem;
            [menu addItem:menu_item];
         }
         else {
            if (aitem->popup) {
               temp_menu = [[NSMenu alloc] initWithTitle: [NSString stringWithUTF8String:buf]];
               [temp_menu setAutoenablesItems:NO];
               MenuDelegate *menu_delegate = [[MenuDelegate alloc] init];
               NSMenuItem *nsmenuitem = [menu_delegate build_menu_item:aitem];
               [menu addItem:nsmenuitem];
               [menu setSubmenu:temp_menu forItem:nsmenuitem];
               the_menu = aitem->popup;
               add_items(temp_menu, display, amenu, the_menu, popup);
               [menu_delegate release];
               [temp_menu release];
            }
            else {
               temp_menu = [[NSMenu alloc] initWithTitle: [NSString stringWithUTF8String:buf]];
               [temp_menu setAutoenablesItems:NO];
               MenuDelegate *menu_delegate = [[MenuDelegate alloc] init];
               NSMenuItem *nsmenuitem = [menu_delegate build_menu_item:aitem];
               menu_delegate->item = aitem;
               MENU_ITEM_THINGS *mit = al_malloc(sizeof(MENU_ITEM_THINGS));
               mit->menu_delegate = menu_delegate;
               mit->menu_item = nsmenuitem;
               mit->menu = menu;
               aitem->extra1 = mit;
               MENU_THINGS *mt = aitem->parent->extra1;
               mt->display = display;
               mt->toplevel_menu = amenu;
               [menu addItem:nsmenuitem];
               if (aitem->flags & ALLEGRO_MENU_ITEM_CHECKBOX) {
                  update_checked_state(nsmenuitem, aitem->flags & ALLEGRO_MENU_ITEM_CHECKED);
               }
               [nsmenuitem setEnabled:((aitem->flags & ALLEGRO_MENU_ITEM_DISABLED) ? FALSE : TRUE)];
               _al_update_menu_item_at(aitem, -1);
               [temp_menu release];
            }
         }
      }
   }
}

static void destroy_menu_hierarchy(ALLEGRO_MENU *amenu)
{
   int mainidx;
   int i;

   for (mainidx = 0; mainidx < (int)menus._size; mainidx++) {
      ALLEGRO_MENU *m = *(ALLEGRO_MENU **)_al_vector_ref(&menus, mainidx);
      if (m == amenu) {
         break;
      }
   }

   for (i = 0; i < (int)amenu->items._size; i++) {
      ALLEGRO_MENU_ITEM *it = *(ALLEGRO_MENU_ITEM **)_al_vector_ref(&amenu->items, i);
      al_free(it->extra1);
      it->extra1 = NULL;
      ALLEGRO_MENU *m = *(ALLEGRO_MENU **)_al_vector_ref(&menus, mainidx+1+i);
      MENU_THINGS *mt = m->extra1;
      if (mt) {
         int j;
         for (j = 0; j < (int)mt->items._size; j++) {
            ALLEGRO_MENU_ITEM *aitem = *(ALLEGRO_MENU_ITEM **)_al_vector_ref(&mt->items, i);
            al_free(aitem->extra1);
            aitem->extra1 = NULL;
         }
      }
      al_free(mt);
      m->extra1 = NULL;
      _al_vector_find_and_delete(&menus, &m);
   }
}

static NSMenu *show_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *amenu, bool popup)
{
   int i;
   NSMenu *main_menu;

   NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

   if (!popup) {
      if (amenu == NULL) {
         al_lock_mutex(mutex);
         for (i = 0; i < (int)displays._size; i++) {
            DISPLAY_INFO *d = *(DISPLAY_INFO **)_al_vector_ref(&displays, i);
            if (d->display == display) {
               al_unregister_event_source((ALLEGRO_EVENT_QUEUE *)queue, (ALLEGRO_EVENT_SOURCE *)display);
                  _al_vector_find_and_delete(&displays, &d);
                  /* Free whole hierarchy */
               destroy_menu_hierarchy(d->toplevel_menu);
                  al_free(d);
                  break;
            }
         }
         al_unlock_mutex(mutex);
         return NULL;
      }

      DISPLAY_INFO *d = al_calloc(1, sizeof(DISPLAY_INFO));
      d->display = display;
      d->toplevel_menu = amenu;
      al_lock_mutex(mutex);
      DISPLAY_INFO **ptr = _al_vector_alloc_back(&displays);
      *ptr = d;
      al_register_event_source((ALLEGRO_EVENT_QUEUE *)queue, (ALLEGRO_EVENT_SOURCE *)display);
      al_unlock_mutex(mutex);
   }
   else {
   }

   if(popup) {
      main_menu = [[NSMenu alloc] initWithTitle: @""];
      [main_menu setAutoenablesItems:NO];
      [NSApp activateIgnoringOtherApps:YES];
   }
   else {
      main_menu = init_apple_menu();
   }
   add_items(main_menu, display, amenu, amenu, popup);

   [pool drain];

   return main_menu;
}

bool _al_show_display_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *amenu)
{
   NSMenu *main_menu = show_menu(display, amenu, false);

   if (main_menu == NULL) {
      return true;
   }

   [NSApp setMainMenu: main_menu];

   return true;
}

bool _al_hide_display_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *menu)
{
   (void)menu;
   (void)display;
   /* Nowhere to hide on OS X */
   return false;
}

bool _al_show_popup_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *amenu)
{
   NSMenu *main_menu = show_menu(display, amenu, true);

   if (main_menu == NULL) {
      return true;
   }

   Runner *r = [[Runner alloc] init];
   [r performSelectorOnMainThread:@selector(create_popup:) withObject:main_menu waitUntilDone:TRUE];
   [r release];

   return true;
}
