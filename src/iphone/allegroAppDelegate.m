#import "allegroAppDelegate.h"
#import "EAGLView.h"

#import <UIKit/UIKit.h>
#include <pthread.h>


ALLEGRO_DEBUG_CHANNEL("iphone")

static allegroAppDelegate *global_delegate;

static void _al_iphone_generate_shake_event(void)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   if (display) {
      if (_al_event_source_needs_to_generate_event(&display->es)) {
         ALLEGRO_EVENT e;
         e.type = ALLEGRO_EVENT_DISPLAY_SHAKE;
         _al_event_source_lock(&display->es);
         _al_event_source_emit_event(&display->es, &e);
         _al_event_source_unlock(&display->es);
      }
   }
}

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

@implementation allegroAppDelegate

@synthesize window;
@synthesize view;
@synthesize lastAcceleration;

+ (void)run:(int)argc:(char **)argv {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    UIApplicationMain(argc, argv, nil, @"allegroAppDelegate");
    [pool release];
}

void _al_iphone_run_user_main(void);
- (void)applicationDidFinishLaunching:(UIApplication *)application {
   ALLEGRO_INFO("App launched.\n");
    
   [UIAccelerometer sharedAccelerometer].delegate = self;

   application.statusBarHidden = true;
   global_delegate = self;

   _al_iphone_run_user_main();   
}

- (void)applicationWillTerminate:(UIApplication *)application {
    ALLEGRO_EVENT event;
    ALLEGRO_DISPLAY *d = allegro_display;
    ALLEGRO_SYSTEM_IPHONE *iphone = (void *)al_system_driver();
    iphone->wants_shutdown = true;

    _al_event_source_lock(&d->es);
    
    if (_al_event_source_needs_to_generate_event(&d->es)) {
        event.display.type = ALLEGRO_EVENT_DISPLAY_CLOSE;
        event.display.timestamp = al_current_time();
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

/* Shake event code by Emanuele Vulcano */

static BOOL L0AccelerationIsShaking(UIAcceleration* last, UIAcceleration* current, double threshold) {
   double
      deltaX = fabs(last.x - current.x),
      deltaY = fabs(last.y - current.y),
      deltaZ = fabs(last.z - current.z);

   return
      (deltaX > threshold && deltaY > threshold) ||
      (deltaX > threshold && deltaZ > threshold) ||
      (deltaY > threshold && deltaZ > threshold);
}

- (void)accelerometer:(UIAccelerometer*)accelerometer didAccelerate:(UIAcceleration*)acceleration
{
    _al_iphone_generate_joystick_event(acceleration.x, acceleration.y, acceleration.z);

   if (self.lastAcceleration) {
      if (!histeresisExcited && L0AccelerationIsShaking(self.lastAcceleration, acceleration, 0.7)) {
         histeresisExcited = YES;

         _al_iphone_generate_shake_event();

      } else if (histeresisExcited && !L0AccelerationIsShaking(self.lastAcceleration, acceleration, 0.2)) {
         histeresisExcited = NO;
      }
   }

   self.lastAcceleration = acceleration;
}

- (void)dealloc {
	[window release];
	[super dealloc];
}

@end
