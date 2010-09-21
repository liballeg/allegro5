#import "allegroAppDelegate.h"
#import "EAGLView.h"

#import <UIKit/UIKit.h>
#include <pthread.h>


ALLEGRO_DEBUG_CHANNEL("iphone")

void _al_iphone_run_user_main(void);

static allegroAppDelegate *global_delegate;
static UIImageView *splashview;
static UIWindow *splashwin;

void _al_iphone_add_view(ALLEGRO_DISPLAY *display)
{
   [global_delegate set_allegro_display:display];
   /* This is the same as
    * [global_delegate.view add_view];
    * except it will run in the main thread.
    */
   [global_delegate performSelectorOnMainThread: @selector(add_view) 
      withObject:nil
      waitUntilDone: YES];
}

void _al_iphone_make_view_current(void)
{
    [global_delegate.view make_current];
}

void _al_iphone_flip_view(void)
{
    [global_delegate.view flip];
   if (splashview) {
      [splashview release];
      [splashwin release];
      splashview = nil;
      splashwin = nil;
   }
}

void _al_iphone_reset_framebuffer(void)
{
   [global_delegate.view reset_framebuffer];
}

/* Use a frequency to start receiving events at the freuqency, 0 to shut off
 * the accelerometer (according to Apple, it drains a bit of battery while on).
 */
void _al_iphone_accelerometer_control(int frequency)
{
    if (frequency) {
        [[UIAccelerometer sharedAccelerometer] setUpdateInterval:(1.0 / frequency)];
        [[UIAccelerometer sharedAccelerometer] setDelegate:global_delegate];
    }
    else {
        [[UIAccelerometer sharedAccelerometer] setDelegate:nil];
    }
}

void _al_iphone_get_screen_size(int *w, int *h)
{
    *w = [[UIScreen mainScreen] bounds].size.width;
    *h = [[UIScreen mainScreen] bounds].size.height;
}

@implementation allegroAppDelegate

@synthesize window;
@synthesize view;

+ (void)run:(int)argc:(char **)argv {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    UIApplicationMain(argc, argv, nil, @"allegroAppDelegate");
    [pool release];
}

/* When applicationDidFinishLaunching() returns, the current view gets visible
 * for at least one frame. Since we have no OpenGL context in the main thread
 * we cannot draw anything into our OpenGL view so it means there's a short
 * black flicker before the first al_flip_display() no matter what.
 *
 * To prevent the black flicker we create a dummy view here which loads the
 * Default.png just as apple does internally. This way the moment the user
 * view is first displayed in the user thread we switch from displaying the
 * splash screen to the first user frame, without any flicker.
 */
static void display_splash_screen(void)
{
   UIScreen *screen = [UIScreen mainScreen];
   splashwin = [[UIWindow alloc] initWithFrame:[screen bounds]];
   UIImage *img = [UIImage imageNamed:@"Default.png"];
   splashview = [[UIImageView alloc] initWithImage:img];
   [splashwin addSubview:splashview];
   [splashwin makeKeyAndVisible];
}

- (void)applicationDidFinishLaunching:(UIApplication *)application {
   ALLEGRO_INFO("App launched.\n");
    
   application.statusBarHidden = true;
   global_delegate = self;

   _al_iphone_run_user_main();
    
    display_splash_screen();
}

- (void)applicationWillTerminate:(UIApplication *)application {
    (void)application;
    ALLEGRO_EVENT event;
    ALLEGRO_DISPLAY *d = allegro_display;
    ALLEGRO_SYSTEM_IPHONE *iphone = (void *)al_get_system_driver();
    iphone->wants_shutdown = true;

    _al_event_source_lock(&d->es);
    
    if (_al_event_source_needs_to_generate_event(&d->es)) {
        event.display.type = ALLEGRO_EVENT_DISPLAY_CLOSE;
        event.display.timestamp = al_get_time();
        _al_event_source_emit_event(&d->es, &event);
    }
    _al_event_source_unlock(&d->es);
    
    /* When this method returns, the application terminates - so lets wait a bit
     * so the user app can shutdown properly, e.g. to save state as is usually
     * required by iphone apps.
     */
    _al_iphone_await_termination();
}

- (void)set_allegro_display:(ALLEGRO_DISPLAY *)d {
   allegro_display = d;
}

/* Note: This must be called from the main thread. Ask Apple why - but I tried
 * it and otherwise things simply don't work (the screen just stays black).
 */
- (void)add_view {
   window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
   view = [[EAGLView alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
   [view set_allegro_display:allegro_display];
   [window addSubview:view];
   [window makeKeyAndVisible];
}

- (void)accelerometer:(UIAccelerometer*)accelerometer didAccelerate:(UIAcceleration*)acceleration
{
    (void)accelerometer;
    _al_iphone_generate_joystick_event(acceleration.x, acceleration.y, acceleration.z);
}

- (void)dealloc {
	[window release];
	[super dealloc];
}

@end
