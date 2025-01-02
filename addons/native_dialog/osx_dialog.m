#import <Cocoa/Cocoa.h>
#import <Availability.h>
#import <IOKit/hid/IOHIDLib.h>
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 110000
#include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#endif

#define ALLEGRO_INTERNAL_UNSTABLE
#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"
#include "allegro5/platform/aintosx.h"
#include "allegro5/allegro_osx.h"

bool _al_init_native_dialog_addon(void)
{
    return true;
}

void _al_shutdown_native_dialog_addon(void)
{
}
#pragma mark File Dialog

static NSArray * get_filter_array(const _AL_VECTOR *patterns)
{
   NSMutableArray *filter_array = [[NSMutableArray alloc] init];

   bool any_catchalls = false;
   for (size_t i = 0; i < _al_vector_size(patterns); i++) {
      _AL_PATTERNS_AND_DESC *patterns_and_desc = _al_vector_ref(patterns, (int)i);
      for (size_t j = 0; j < _al_vector_size(&patterns_and_desc->patterns_vec); j++) {
         _AL_PATTERN *pattern = _al_vector_ref(&patterns_and_desc->patterns_vec, (int)j);
         if (pattern->is_catchall) {
            any_catchalls = true;
            break;
         }
         char *cstr = al_cstr_dup(al_ref_info(&pattern->info));
         NSString *filter_text = [NSString stringWithUTF8String: cstr];
         al_free(cstr);
         if (!pattern->is_mime) {
            /* MacOS expects extensions, so make an attempt to extract them. */
            NSArray *parts = [filter_text componentsSeparatedByString: @"."];
            size_t num_parts = parts.count;
            if (num_parts <= 1)
               continue;
            /* Extensions with dots did not work for me, so just grab the last component. */
            parts = [parts subarrayWithRange:NSMakeRange(num_parts - 1, 1)];
            filter_text = [parts componentsJoinedByString: @"."];
         }
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 110000
         UTType *type;
         if (pattern->is_mime)
            type = [UTType typeWithMIMEType:filter_text];
         else
            type = [UTType typeWithFilenameExtension:filter_text];
         if (type != nil)
            [filter_array addObject: type];
#else
         if (pattern->is_mime) {
            continue;
         }
         [filter_array addObject: filter_text];
#endif
      }
   }

   if (any_catchalls) {
      filter_array = [[NSMutableArray alloc] init];
   }

   return filter_array;
}

bool _al_show_native_file_dialog(ALLEGRO_DISPLAY *display,
                                 ALLEGRO_NATIVE_DIALOG *fd)
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   (void)display;
   int mode = fd->flags;
   NSString *filename;
   NSURL *directory;

   /* Set initial directory to pass to the file selector */
   if (fd->fc_initial_path) {
      ALLEGRO_PATH *initial_directory = al_clone_path(fd->fc_initial_path);
      /* Strip filename from path  */
      al_set_path_filename(initial_directory, NULL);

      /* Convert path and filename to NSString objects */
      directory = [NSURL fileURLWithPath: [NSString stringWithUTF8String: al_path_cstr(initial_directory, '/')]
                             isDirectory: YES];
      filename = [NSString stringWithUTF8String: al_get_path_filename(fd->fc_initial_path)];
      al_destroy_path(initial_directory);
   } else {
      directory = nil;
      filename = nil;
   }
   dispatch_sync(dispatch_get_main_queue(), ^{
      /* We need slightly different code for SAVE and LOAD dialog boxes, which
       * are handled by slightly different classes.
       */
      if (mode & ALLEGRO_FILECHOOSER_SAVE) {    // Save dialog
         NSSavePanel *panel = [NSSavePanel savePanel];
         NSArray *filter_array;

         /* Set file save dialog box options */
         [panel setCanCreateDirectories: YES];
         [panel setCanSelectHiddenExtension: YES];
         [panel setAllowsOtherFileTypes: YES];
         [panel setExtensionHidden: NO];
         if (filename) {
            [panel setNameFieldStringValue:filename];
         }
         if (_al_vector_size(&fd->fc_patterns) > 0) {
            filter_array = get_filter_array(&fd->fc_patterns);
            if (filter_array && [filter_array count] > 0) {
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 110000
               [panel setAllowedContentTypes:filter_array];
#else
               [panel setAllowedFileTypes:filter_array];
#endif
            }
         }
         [panel setDirectoryURL: directory];
         /* Open dialog box */
         if ([panel runModal] == NSOKButton) {
            /* NOTE: at first glance, it looks as if this code might leak
             * memory, but in fact it doesn't: the string returned by
             * UTF8String is freed automatically when it goes out of scope
             * (according to the UTF8String docs anyway).
             */
            const char *s = [[[panel URL] path] UTF8String];
            fd->fc_path_count = 1;
            fd->fc_paths = al_malloc(fd->fc_path_count * sizeof *fd->fc_paths);
            fd->fc_paths[0] = al_create_path(s);
         }
      } else {                                  // Open dialog
         NSOpenPanel *panel = [NSOpenPanel openPanel];
         NSArray *filter_array;

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
         [panel setDirectoryURL:directory];
         if (filename) {
            [panel setNameFieldStringValue:filename];
         }
         if (_al_vector_size(&fd->fc_patterns) > 0) {
            filter_array = get_filter_array(&fd->fc_patterns);
            if (filter_array && [filter_array count] > 0) {
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 110000
               [panel setAllowedContentTypes:filter_array];
#else
               [panel setAllowedFileTypes:filter_array];
#endif
            }
         }
         /* Open dialog box */
         if ([panel runModal] == NSOKButton) {
            size_t i;
            fd->fc_path_count = [[panel URLs] count];
            fd->fc_paths = al_malloc(fd->fc_path_count * sizeof *fd->fc_paths);
            for (i = 0; i < fd->fc_path_count; i++) {
               /* NOTE: at first glance, it looks as if this code might leak
                * memory, but in fact it doesn't: the string returned by
                * UTF8String is freed automatically when it goes out of scope
                * (according to the UTF8String docs anyway).
                */
               NSURL* url = [[panel URLs] objectAtIndex: i];
               const char* s = [[url path] UTF8String];
               fd->fc_paths[i] = al_create_path(s);
            }
         }
      }

   });
   _al_osx_clear_mouse_state();

   [pool release];

   return true;
}

#pragma mark Alert Box
int _al_show_native_message_box(ALLEGRO_DISPLAY *display,
                                ALLEGRO_NATIVE_DIALOG *fd)
{
   (void)display;
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   dispatch_sync(dispatch_get_main_queue(), ^{
      NSString* button_text;
      unsigned int i;
      NSAlert* box = [[NSAlert alloc] init];

      if (fd->mb_buttons == NULL) {
         button_text = @"OK";
         if (fd->flags & ALLEGRO_MESSAGEBOX_YES_NO) button_text = @"Yes|No";
         if (fd->flags & ALLEGRO_MESSAGEBOX_OK_CANCEL) button_text = @"OK|Cancel";
      }
      else {
         button_text = [NSString stringWithUTF8String: al_cstr(fd->mb_buttons)];
      }

      NSArray* buttons = [button_text componentsSeparatedByString: @"|"];
      [box setMessageText:[NSString stringWithUTF8String: al_cstr(fd->title)]];
      [box setInformativeText:[NSString stringWithUTF8String: al_cstr(fd->mb_text)]];
      [box setAlertStyle: NSWarningAlertStyle];
      [[box window] setLevel: NSFloatingWindowLevel];
      for (i = 0; i < [buttons count]; ++i)
         [box addButtonWithTitle: [buttons objectAtIndex: i]];

      int retval = [box runModal];
      fd->mb_pressed_button = retval + 1 - NSAlertFirstButtonReturn;
      [box release];
   });
   [pool release];
   _al_osx_clear_mouse_state();
   return fd->mb_pressed_button;
}

#pragma mark Text Log View

@interface ALLEGLogView : NSTextView <NSWindowDelegate>
{
@public
   ALLEGRO_NATIVE_DIALOG *textlog;
}
- (void)keyDown: (NSEvent*)event;
- (BOOL)windowShouldClose: (id)sender;
- (void)emitCloseEventWithKeypress: (BOOL)keypress;
- (void)appendText: (NSString*)text;
@end


@implementation ALLEGLogView


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

- (void)appendText: (NSString*)text
{
   id keys[] = {NSForegroundColorAttributeName, NSFontAttributeName};
   id objects[] = {[NSColor controlTextColor], [NSFont userFixedPitchFontOfSize: 0]};
   int count = textlog->flags & ALLEGRO_TEXTLOG_MONOSPACE ? 2 : 1;
   NSDictionary *attributes = [NSDictionary dictionaryWithObjects:objects forKeys:keys count:count];
   NSAttributedString *attributedString = [[[NSAttributedString alloc] initWithString:text attributes:attributes] autorelease];
   NSTextStorage* store = [self textStorage];
   [store beginEditing];
   [store appendAttributedString:attributedString];
   [store endEditing];
}
@end

@interface ALLEGScrollView : NSScrollView <NSWindowDelegate>
{
}
- (void)appendText: (NSString*)text;
@end

@implementation ALLEGScrollView

- (void)appendText: (NSString*)text
{
   ALLEGLogView *view = (ALLEGLogView *)[self documentView];
   [view appendText:text];
   float bottom = view.frame.size.height;
   [self.contentView scrollToPoint: NSMakePoint(0, bottom)];
}
@end

bool _al_open_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
   NSRect rect = NSMakeRect(0, 0, 640, 480);
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

   dispatch_sync(dispatch_get_main_queue(), ^{
      NSWindow *win = [[NSWindow alloc] initWithContentRect: rect
                                                  styleMask: mask
                                                    backing: NSBackingStoreBuffered
                                                      defer: NO
                                                     screen: screen];
      [win setReleasedWhenClosed: NO];
      [win setTitle: @"Allegro Text Log"];
      [win setMinSize: NSMakeSize(128, 128)];
      ALLEGScrollView *scrollView = [[ALLEGScrollView alloc] initWithFrame: rect];
      [scrollView setHasHorizontalScroller: YES];
      [scrollView setHasVerticalScroller: YES];
      [scrollView setAutohidesScrollers: YES];
      [scrollView setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable];

      [[scrollView contentView] setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable];
      [[scrollView contentView] setAutoresizesSubviews: YES];

      NSRect framerect = [[scrollView contentView] frame];
      ALLEGLogView *view = [[ALLEGLogView alloc] initWithFrame: framerect];
      view->textlog = textlog;
      [view setHorizontallyResizable: YES];
      [view setVerticallyResizable: YES];
      [view setAutoresizingMask: NSViewHeightSizable | NSViewWidthSizable];
      [[view textContainer] setContainerSize: NSMakeSize(rect.size.width, 1000000)];
      [[view textContainer] setWidthTracksTextView: NO];
      [view setEditable: NO];
      [scrollView setDocumentView: view];

      [[win contentView] addSubview: scrollView];
      [scrollView release];

      [win setDelegate: view];
      [win orderFront: nil];

      /* Save handles for future use. */
      textlog->window = win; // Non-owning reference
      textlog->tl_textview = scrollView; // Non-owning reference
   });
   /* Now notify al_show_native_textlog that the text log is ready. */
   textlog->is_active = true;
   textlog->tl_done = true;
   return true;
}

void _al_close_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
    NSWindow *win = (NSWindow *)textlog->window;
    __block bool is_visible = false;
    dispatch_sync(dispatch_get_main_queue(), ^{
       is_visible = [win isVisible];
    });
    if (is_visible) {
       [win performSelectorOnMainThread:@selector(close) withObject:nil waitUntilDone:YES];
    }
    [win performSelectorOnMainThread:@selector(release) withObject:nil waitUntilDone:NO];
    textlog->window = NULL;
    textlog->tl_textview = NULL;
    /* Notify everyone that we're gone. */
    textlog->is_active = false;
    textlog->tl_done = true;
}

void _al_append_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
    if (textlog->is_active) {
        ALLEGScrollView *scrollView = (NSScrollView *)textlog->tl_textview;
        NSString *text = [NSString stringWithUTF8String: al_cstr(textlog->tl_pending_text)];
        [scrollView performSelectorOnMainThread:@selector(appendText:)
                               withObject:text
                            waitUntilDone:NO];

        al_ustr_truncate(textlog->tl_pending_text, 0);
    }
}

#pragma mark  Menus
/* This class manages the association between windows and menus.
 * OS X has one menu per application, but Allegro has one menu
 * per display (i.e. window) as Windows and Linux tend to do.
 * As a result this class is used to keep track of which menu
 * was assigned to which window. It listens for the notification when
 * a window becomes main and swaps in the corresponding menu.
 */
@interface ALLEGTargetManager : NSObject {
    NSMutableArray * _items;
}
+(ALLEGTargetManager*) sharedManager;
-(id) init;
-(void) dealloc;
-(void) setMenu: (NSMenu*) menu forWindow:(NSWindow*) window;
@end

/* This class represents a menu item target. There is one of these
 * for each NSMenu and it handles all the items in that menu. It
 * maintains the correspondence between ALLEGRO_MENU_ITEMs and
 * NSMenuItems, taking into account the 'App menu' (the one with
 * the app's name in bold) which appears by convention on OS X.
 */
@interface ALLEGMenuTarget : NSObject
{
    NSLock* lock;
    ALLEGRO_MENU* amenu;
    BOOL _hasAppMenu;
    NSMenu* _menu;
}
-(NSMenu*) menu;
-(id) initWithMenu:(ALLEGRO_MENU*) amenu; // Designated initializer
-(NSMenu*) menu;
-(void) show;
-(void) showPopup;
-(void) insertItem:(ALLEGRO_MENU_ITEM*) item atIndex:(int) index;
-(void) updateItem:(ALLEGRO_MENU_ITEM*) item atIndex: (int) index;
-(void) destroyItem:(ALLEGRO_MENU_ITEM*) item atIndex: (int) index;
-(void) activated: (id) sender;
-(BOOL) validateMenuItem:(NSMenuItem *)menuItem;
-(void) dealloc;
+(ALLEGMenuTarget*) targetForMenu:(ALLEGRO_MENU*) amenu;
@end

/* Take a menu caption. If it has an accelerator char (preceeded by & or _)
 * remove the & or _ from the string and return the char.
 */
static NSString* extract_accelerator(NSMutableString* caption) {
    NSRange range = [caption rangeOfCharacterFromSet:[NSCharacterSet characterSetWithCharactersInString:@"&_"]];
    if (range.location != NSNotFound && range.location < [caption length]) {
        [caption deleteCharactersInRange:range];
        return [caption substringWithRange: range];
    } else {
        // Not found or you ended the string with & or _
        return @"";
    }
}

bool _al_init_menu(ALLEGRO_MENU *amenu)
{
   amenu->extra1 = [[ALLEGMenuTarget alloc] initWithMenu: amenu];
   return true;
}

bool _al_init_popup_menu(ALLEGRO_MENU *amenu)
{
    amenu->extra1 = [[ALLEGMenuTarget alloc] initWithMenu: amenu];
    return true;
}

bool _al_insert_menu_item_at(ALLEGRO_MENU_ITEM *aitem, int i)
{
   ALLEGMenuTarget* mt = [ALLEGMenuTarget targetForMenu: aitem->parent];
   [mt insertItem: aitem atIndex: i];
   return true;
}

bool _al_destroy_menu_item_at(ALLEGRO_MENU_ITEM *aitem, int i)
{
    ALLEGMenuTarget* mt = [ALLEGMenuTarget targetForMenu: aitem->parent];
    [mt destroyItem: aitem atIndex: i];
    return true;}

bool _al_update_menu_item_at(ALLEGRO_MENU_ITEM *item, int i)
{
   ALLEGMenuTarget* mt = [ALLEGMenuTarget targetForMenu: item->parent];
   [mt updateItem: item atIndex: i];
   return true;
}

bool _al_show_display_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *amenu)
{
    ALLEGMenuTarget* target = [ALLEGMenuTarget targetForMenu:amenu];
    NSWindow* window = al_osx_get_window(display);
    [[ALLEGTargetManager sharedManager] setMenu:target.menu
                                      forWindow:window];
    [target performSelectorOnMainThread:@selector(show) withObject:nil waitUntilDone:YES];
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
    (void) display;
    ALLEGMenuTarget* target = [ALLEGMenuTarget targetForMenu: amenu];
    [target performSelectorOnMainThread:@selector(showPopup) withObject:nil waitUntilDone:NO];

    return true;
}

int _al_get_menu_display_height(void)
{
   return 0;
}

@implementation ALLEGMenuTarget
/* Initial conversion of ALLEGRO_MENU_ITEM to NSMenuItem.
 * The target (self) is set for the item.
 * The checkbox state is not set here, it's done dynamically in -validateMenuItem.
 */
-(NSMenuItem*) buildMenuItemFor:(ALLEGRO_MENU_ITEM*) aitem
{
    NSMenuItem* item;
    if (aitem->caption && al_ustr_length(aitem->caption) > 0) {
        NSMutableString* title = [NSMutableString stringWithUTF8String:al_cstr(aitem->caption)];
        NSString* key = extract_accelerator(title);
        item = [[NSMenuItem alloc] initWithTitle:title
                                          action:@selector(activated:)
                                   keyEquivalent:key];
        [item setTarget:self];
        aitem->extra1 = item;
        return item;
    } else {
        item = [NSMenuItem separatorItem];
    }
    return item;
}
// Insert the app menu if it's not there already
-(void) insertAppMenu
{
    if (!self->_hasAppMenu) {
        NSMenuItem* apple = [[NSMenuItem alloc] init];
        [apple setTitle:@"Apple"];

        NSMenu* appleItems = [[NSMenu alloc] init];
        [appleItems setTitle:@"Apple"];
        [apple setSubmenu:appleItems];
        [appleItems addItemWithTitle:@"Hide" action:@selector(hide:) keyEquivalent:@"h"];
        [appleItems addItemWithTitle:@"Hide others" action:@selector(hideOtherApplications:) keyEquivalent:@""];
        [appleItems addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];
        [appleItems addItem:[NSMenuItem separatorItem]];
        [appleItems addItemWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
        [self.menu insertItem:apple atIndex:0];
        [apple release];
        self->_hasAppMenu = YES;
    }
}
// Get the NSMenuItem corresponding to this ALLEGRO_MENU_ITEM
- (NSMenuItem*) itemForAllegroItem:(ALLEGRO_MENU_ITEM*) aitem
{
    int index = _al_vector_find(&self->amenu->items, &aitem);
    if (index >= 0) {
        /* If the app menu is showing it will be at index 0 so account for this */
        if (self->_hasAppMenu) {
            ++index;
        }
    return [self.menu itemAtIndex:index];
    } else {
        return nil;
    }
}
// Get the ALLEGRO_MENU_ITEM corresponding to this NSMenuItem
- (ALLEGRO_MENU_ITEM*) allegroItemforItem: (NSMenuItem*) mi
{
    int i;
    ALLEGRO_MENU_ITEM * ami;

    for (i = 0; i < (int)_al_vector_size(&self->amenu->items); i++) {
        ami = *(ALLEGRO_MENU_ITEM**) _al_vector_ref(&self->amenu->items, i);
        if (ami->extra1 == mi) {
            return ami;
        }
    }
    return NULL;
}
// Create target with ALLEGRO_MENU bound to it.
- (id)initWithMenu:(ALLEGRO_MENU*) source_menu
{
    self = [super init];
    if (self) {
        self->_hasAppMenu = NO;
        self->lock = [[NSLock alloc] init];
        self->amenu = source_menu;
        self->_menu = [[NSMenu alloc] init];
    }
    return self;
}
-(id) init
{
    /* This isn't a valid initializer */
    return nil;
}
// Manage the enabled and checked state (done dynamically by the framework)
-(BOOL) validateMenuItem:(NSMenuItem *)menuItem {
    ALLEGRO_MENU_ITEM* aitem = [self allegroItemforItem:menuItem];
    if (aitem) {
        int checked = (aitem->flags & (ALLEGRO_MENU_ITEM_CHECKBOX|ALLEGRO_MENU_ITEM_CHECKED)) == (ALLEGRO_MENU_ITEM_CHECKBOX|ALLEGRO_MENU_ITEM_CHECKED);
        [menuItem setState: checked ? NSOnState : NSOffState ];
        return aitem->flags & ALLEGRO_MENU_ITEM_DISABLED ? NO : YES;
    }
    return YES;
}
- (void)dealloc
{
    [self->lock release];
    [self->_menu release];
    [super dealloc];
}
// Get the menu
-(NSMenu*) menu
{
    return self->_menu;
}
// Action event when the menu is selected
-(void) activated:(id)sender {
    NSMenuItem* mitem = (NSMenuItem*) sender;
    ALLEGRO_MENU_ITEM* aitim = [self allegroItemforItem:mitem];
    if (aitim) {
        if (aitim->flags & ALLEGRO_MENU_ITEM_CHECKBOX) {
            aitim->flags ^= ALLEGRO_MENU_ITEM_CHECKED;
        }
        _al_emit_menu_event(aitim->parent->display, aitim->unique_id);
    }
}
// Insert an item, keep the NSMenu in sync
-(void) insertItem:(ALLEGRO_MENU_ITEM*) aitem atIndex: (int) index
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSMenuItem* item = [self buildMenuItemFor:aitem];
    [self.menu insertItem:item atIndex:index];
    if (aitem->popup) {
        ALLEGMenuTarget* sub = [ALLEGMenuTarget targetForMenu:aitem->popup];
        [[sub menu] setTitle:[item title]];
        [item setAction:nil];
        [item setSubmenu:[sub menu]];
    }
    [pool release];
}
// Update an item (caption only, see -validateMenuItem: )
-(void) updateItem:(ALLEGRO_MENU_ITEM *)aitem atIndex:(int)index
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    (void) index;
    NSMenuItem* item = [self itemForAllegroItem:aitem];
    NSMutableString* caption = [NSMutableString stringWithUTF8String:al_cstr(aitem->caption)];
    NSString* key = extract_accelerator(caption);
    [item setTitle:caption];
    [item setKeyEquivalent:key];
    if (aitem->popup) {
        [item setEnabled:(aitem->flags & ALLEGRO_MENU_ITEM_DISABLED) ? NO : YES];
    }
    [pool release];
}
// Remove an item, keep the NSMenu in sync
-(void) destroyItem:(ALLEGRO_MENU_ITEM *)aitem atIndex:(int)index
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    (void) index;
    NSMenuItem* item = [self itemForAllegroItem:aitem];
    [item.menu removeItem:item];
    [pool release];
}
// Show the menu on the main application menu bar
-(void) show
{
    [self insertAppMenu];
    [NSApp setMainMenu:self.menu];
}
// Show the menu as a pop-up on the current key window.
-(void) showPopup
{
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

    [NSMenu popUpContextMenu:self.menu withEvent:fakeMouseEvent forView:[window contentView]];
}
// Find the target associated with this ALLEGRO_MENU
+(ALLEGMenuTarget*) targetForMenu:(ALLEGRO_MENU *)amenu {
    if (!amenu->extra1) {
        amenu->extra1 = [[ALLEGMenuTarget alloc] initWithMenu:amenu];
    }
    return amenu->extra1;
}
@end // ALLEGMenuTarget

static ALLEGTargetManager* _sharedmanager = nil;
@implementation ALLEGTargetManager
// Get the singleton manager object
+(ALLEGTargetManager*) sharedManager
{
    if (!_sharedmanager) {
        _sharedmanager = [[ALLEGTargetManager alloc] init];
    }
    return _sharedmanager;
}
// Set up and register for notifications
-(id) init
{
    self = [super init];
    if (self) {
        self->_items = [[NSMutableArray alloc] init];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onWindowChange:) name:NSWindowDidBecomeMainNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onWindowClose:) name:NSWindowWillCloseNotification object:nil];

    }
    return self;
}
// Destroy and un-register
-(void) dealloc
{
    [self->_items release];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
}
// Find the index for a window which this object is managing or NSNotFound
-(NSUInteger) indexForWindow:(NSWindow*) window
{
    NSUInteger i;
    for (i=0; i<[self->_items count]; ++i) {
        NSDictionary* entry = [self->_items objectAtIndex:i];
        if ([window isEqual:[entry valueForKey:@"window"]]) {
            return i;
        }
    }
    return NSNotFound;
}
// Event when main window changes - locate the matching menu and show it
-(void) onWindowChange: (NSNotification*) notification
{
    NSWindow* window = [notification object];
    NSUInteger index = [self indexForWindow: window];
    if (index != NSNotFound) {
        NSDictionary* entry = [self->_items objectAtIndex:index];
        NSMenu* menu = [entry valueForKey:@"menu"];
        [NSApp setMainMenu:menu];
    }
    /* If not found we do nothing */
}
// Event when window closes, remove it from the managed list
-(void) onWindowClose: (NSNotification*) notification
{
    NSWindow* window = [notification object];
    NSUInteger index = [self indexForWindow: window];
    if (index != NSNotFound) {
        [self->_items removeObjectAtIndex:index];
    }
    /* If not found we do nothing */
}
// Link a menu with a window, replace any existing link
-(void) setMenu:(NSMenu *)menu forWindow:(NSWindow *)window
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSUInteger index = [self indexForWindow:window];
    if (menu) {
        NSDictionary* newentry = [NSDictionary dictionaryWithObjectsAndKeys:menu, @"menu", window, @"window", nil];
        if (index == NSNotFound) {
            [self->_items addObject: newentry];
        }
        else {
            [self->_items replaceObjectAtIndex:index withObject: newentry];
        }
    } else {
        /* Menu was null so delete */
        if (index != NSNotFound) {
            [self->_items removeObjectAtIndex:index];
        }
    }
    [pool release];
}
@end // ALLEGTargetManager
