#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"

#include <UIKit/UIAlertView.h>

#include "allegro5/allegro_iphone_objc.h"

bool _al_init_native_dialog_addon(void)
{
    return true;
}

void _al_shutdown_native_dialog_addon(void)
{
}

bool _al_show_native_file_dialog(A5O_DISPLAY *display,
                                 A5O_NATIVE_DIALOG *fd)
{
    (void)display;
    (void)fd;
    return false;
}

@interface AlertDelegate : NSObject<UIAlertViewDelegate> {
    A5O_MUTEX *mutex;
    A5O_COND *button_pressed;
}
@property A5O_MUTEX *mutex;
@property A5O_COND *button_pressed;
@end

@implementation AlertDelegate
@synthesize mutex;
@synthesize button_pressed;
- (void) alertView:(UIAlertView *)alert clickedButtonAtIndex:(NSInteger)bindex {
    (void)alert;
    (void)bindex;
    al_lock_mutex(mutex);
    al_signal_cond(button_pressed);
    al_unlock_mutex(mutex);
}
- (void) createAlert:(NSArray*)array {
    UIView *view = [array objectAtIndex:0];
    UIAlertView *alert = [array objectAtIndex:1];
    [view addSubview:alert];
    [alert show];
    [alert release];
}

@end

int _al_show_native_message_box(A5O_DISPLAY *display,
                                A5O_NATIVE_DIALOG *nd)
{
    (void)display;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc]init];
    NSString *title = [NSString stringWithUTF8String:al_cstr(nd->title)];
    NSString *heading = [NSString stringWithUTF8String:al_cstr(nd->mb_heading)];
    NSString *text = [NSString stringWithUTF8String:al_cstr(nd->mb_text)];
    NSString *ok = [NSString stringWithUTF8String:"Ok"];
    NSString *message = [NSString stringWithFormat:@"%@\n\n%@", heading, text];

    AlertDelegate *delegate = [[AlertDelegate alloc]init];
    delegate.mutex = al_create_mutex();
    delegate.button_pressed = al_create_cond();
    
    // This needs to be done on the thread with the display due to TLS.
    UIView *view = al_iphone_get_view(al_get_current_display());

    UIAlertView *alert = [[UIAlertView alloc]
                          initWithTitle:title
                          message:message
                          delegate:delegate
                          cancelButtonTitle:ok
                          otherButtonTitles:nil];

    [delegate performSelectorOnMainThread:@selector(createAlert:)
                               withObject:@[view,alert]
                            waitUntilDone:YES];    

    al_lock_mutex(delegate.mutex);
    al_wait_cond(delegate.button_pressed, delegate.mutex);
    al_unlock_mutex(delegate.mutex);
    al_destroy_cond(delegate.button_pressed);
    al_destroy_mutex(delegate.mutex);
    
    [delegate release];

    [pool drain];
    return 0;
}

bool _al_open_native_text_log(A5O_NATIVE_DIALOG *textlog)
{
    (void)textlog;
    return false;
}

void _al_close_native_text_log(A5O_NATIVE_DIALOG *textlog)
{
    (void) textlog;
}

void _al_append_native_text_log(A5O_NATIVE_DIALOG *textlog)
{
    (void) textlog;
}

bool _al_init_menu(A5O_MENU *menu)
{
    (void) menu;
    return false;
}

bool _al_init_popup_menu(A5O_MENU *menu)
{
    (void) menu;
    return false;
}

bool _al_insert_menu_item_at(A5O_MENU_ITEM *item, int i)
{
    (void) item;
    (void) i;
    return false;
}

bool _al_destroy_menu_item_at(A5O_MENU_ITEM *item, int i)
{
    (void) item;
    (void) i;
    return false;
}

bool _al_update_menu_item_at(A5O_MENU_ITEM *item, int i)
{
    (void) item;
    (void) i;
    return false;
}

bool _al_show_display_menu(A5O_DISPLAY *display, A5O_MENU *menu)
{
    (void) display;
    (void) menu;
    return false;
}

bool _al_hide_display_menu(A5O_DISPLAY *display, A5O_MENU *menu)
{
    (void) display;
    (void) menu;
    return false;
}

bool _al_show_popup_menu(A5O_DISPLAY *display, A5O_MENU *menu)
{
    (void) display;
    (void) menu;
    return false;
}

int _al_get_menu_display_height(void)
{
   return 0;
}
