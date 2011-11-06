#import "allegroAppDelegate.h"
#import "EAGLView.h"

#import <UIKit/UIKit.h>
#include <pthread.h>


ALLEGRO_DEBUG_CHANNEL("iphone")

void _al_iphone_run_user_main(void);

static allegroAppDelegate *global_delegate;
static UIImageView *splashview;
static UIWindow *splashwin;
static volatile bool waiting_for_program_halt = false;
static float scale_override = -1.0;

/* Function: al_iphone_program_has_halted
 */
void al_iphone_program_has_halted(void)
{
   waiting_for_program_halt = false;
}

float _al_iphone_get_screen_scale(void)
{
   if (scale_override > 0.0) {
   	return scale_override;
   }
   if ([[UIScreen mainScreen] respondsToSelector:NSSelectorFromString(@"scale")]) {
      return [[UIScreen mainScreen] scale];
   }
   return 1.0f;
}

void _al_iphone_get_view(void)
{
   return [global_delegate view];
}

/* Function: al_iphone_override_screen_scale
 */
void al_iphone_override_screen_scale(float scale)
{
   scale_override = scale;
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
   if (splashview) {
      [splashview removeFromSuperview];
      [splashview release];
      [splashwin removeFromSuperview];
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
   UIScreen* screen = [UIScreen mainScreen];
   
   if (NULL != screen) {

      CGRect bounds = [screen bounds];
      CGFloat scale = 1.0f;
      
      if ([screen respondsToSelector:NSSelectorFromString(@"scale")]) {
         scale = [screen scale];
      }
      
      *w = (int)(bounds.size.width  * scale);
      *h = (int)(bounds.size.height * scale);
   }
   else {
   
      ASSERT("You should never see this message, unless Apple changed their policy and allows for removing screens from iDevices." && false);
   }
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
- (void)display_splash_screen
{
   UIScreen *screen = [UIScreen mainScreen];
   splashwin = [[UIWindow alloc] initWithFrame:[screen bounds]];
   UIImage *img = [UIImage imageNamed:@"Default.png"];
   splashview = [[UIImageView alloc] initWithImage:img];
   [splashwin addSubview:splashview];
   [splashwin makeKeyAndVisible];
}

- (void)orientation_change:(NSNotification *)notification
{
   ALLEGRO_DISPLAY *d = allegro_display;
   UIDeviceOrientation o = [[UIDevice currentDevice] orientation];
   int ao;
   
   (void)notification;

   if (d == NULL)
      return;

   switch (o) {
      case (UIDeviceOrientationPortrait):
         ao = ALLEGRO_DISPLAY_ORIENTATION_0_DEGREES;
         break;
      case (UIDeviceOrientationPortraitUpsideDown):
         ao = ALLEGRO_DISPLAY_ORIENTATION_180_DEGREES;
         break;
      case (UIDeviceOrientationLandscapeRight):
         ao = ALLEGRO_DISPLAY_ORIENTATION_90_DEGREES;
         break;
      case (UIDeviceOrientationLandscapeLeft):
         ao = ALLEGRO_DISPLAY_ORIENTATION_270_DEGREES;
         break;
      case (UIDeviceOrientationFaceUp):
         ao = ALLEGRO_DISPLAY_ORIENTATION_FACE_UP;
         break;
      case (UIDeviceOrientationFaceDown):
         ao = ALLEGRO_DISPLAY_ORIENTATION_FACE_DOWN;
         break;
      default:
         return;
   }
            
   _al_event_source_lock(&d->es);
   if (_al_event_source_needs_to_generate_event(&d->es)) {
      ALLEGRO_EVENT event;
      event.display.type = ALLEGRO_EVENT_DISPLAY_ORIENTATION;
      event.display.timestamp = al_get_time();
      event.display.source = allegro_display;
      event.display.orientation = ao;
      _al_event_source_emit_event(&d->es, &event);
   }
   _al_event_source_unlock(&d->es);
}

- (void)applicationDidFinishLaunching:(UIApplication *)application {
   ALLEGRO_INFO("App launched.\n");
    
   application.statusBarHidden = true;
   global_delegate = self;

   // Register for device orientation notifications
   [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
   [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(orientation_change:) name:UIDeviceOrientationDidChangeNotification object:nil];

    [self display_splash_screen];

   _al_iphone_run_user_main();
}

- (void)applicationWillTerminate:(UIApplication *)application {
    (void)application;
    ALLEGRO_EVENT event;
    ALLEGRO_DISPLAY *d = allegro_display;
    ALLEGRO_SYSTEM_IPHONE *iphone = (void *)al_get_system_driver();
    iphone->wants_shutdown = true;
   
   [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];

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

- (void)applicationWillResignActive:(UIApplication *)application {
   ALLEGRO_DISPLAY *d = allegro_display;
   ALLEGRO_EVENT event;
   
   (void)application;
    
   ALLEGRO_INFO("Application becoming inactive.\n");

   waiting_for_program_halt = true;

   _al_event_source_lock(&d->es);
   if (_al_event_source_needs_to_generate_event(&d->es)) {
       event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_OUT;
       event.display.timestamp = al_current_time();
       _al_event_source_emit_event(&d->es, &event);
   }
   _al_event_source_unlock(&d->es);

   while (waiting_for_program_halt) {
   	// do nothing, this should be quick
	al_rest(0.001);
   }
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    ALLEGRO_DISPLAY *d = allegro_display;
	ALLEGRO_EVENT event;
	
	(void)application;
    
    ALLEGRO_INFO("Application becoming active...\n");

   if (!d)
      return;

    _al_event_source_lock(&d->es);
    if (_al_event_source_needs_to_generate_event(&d->es)) {
        event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_IN;
        event.display.timestamp = al_current_time();
        _al_event_source_emit_event(&d->es, &event);
    }
    _al_event_source_unlock(&d->es);
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
   [view becomeFirstResponder];
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
