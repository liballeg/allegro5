#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"

#include <UIKit/UIAlertView.h>

#include "allegro5/internal/aintern_iphone.h"


/* In 5.1 this is exposed but not in 5.0 */
UIView *_al_iphone_get_view(void);


bool _al_init_native_dialog_addon(void)
{
    return true;
}

void _al_shutdown_native_dialog_addon(void)
{
}

bool _al_show_native_file_dialog(ALLEGRO_DISPLAY *display,
                                 ALLEGRO_NATIVE_DIALOG *fd)
{
    (void)display;
    (void)fd;
    return false;
}

@interface AlertDelegate : NSObject<UIAlertViewDelegate> {
    ALLEGRO_MUTEX *mutex;
    ALLEGRO_COND *button_pressed;
}
@property ALLEGRO_MUTEX *mutex;
@property ALLEGRO_COND *button_pressed;
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
-(void) createAlert:(id)alert {
    [_al_iphone_get_view() addSubview:alert];
    [alert show];
    [alert release];
}

@end

int _al_show_native_message_box(ALLEGRO_DISPLAY *display,
                                ALLEGRO_NATIVE_DIALOG *nd)
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

    UIAlertView *alert = [[UIAlertView alloc]
                          initWithTitle:title
                          message:message
                          delegate:delegate
                          cancelButtonTitle:ok
                          otherButtonTitles:nil];

    [delegate performSelectorOnMainThread:@selector(createAlert:)
                               withObject:alert
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

bool _al_open_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
    (void)textlog;
    return false;
}

void _al_close_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
    (void) textlog;
}

void _al_append_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
    (void) textlog;
}
