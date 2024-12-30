#import "allegroAppDelegate.h"
#import "EAGLView.h"

#import <UIKit/UIKit.h>
#include <pthread.h>

#include "allegro5/allegro_opengl.h"
#include "allegro5/allegro_iphone.h"
#include "allegro5/internal/aintern_opengl.h"

ALLEGRO_DEBUG_CHANNEL("iphone")

void _al_iphone_run_user_main(void);

static allegroAppDelegate *global_delegate;
static UIApplication *app = NULL;
static volatile bool waiting_for_program_halt = false;
static bool disconnect_wait = false;
static UIScreen *airplay_screen = NULL;
static bool airplay_connected = false;

ALLEGRO_MUTEX *_al_iphone_display_hotplug_mutex = NULL;

/* Screen handling */
@interface iphone_screen : NSObject
{
@public
   int adapter;
   UIScreen *screen;
   UIWindow *window;
   ViewController *vc;
   EAGLView *view;
   ALLEGRO_DISPLAY *display;
}
@end

@implementation iphone_screen
@end

void _al_iphone_disconnect(ALLEGRO_DISPLAY *display)
{
   (void)display;

   disconnect_wait = false;
}

static NSMutableArray *iphone_screens;

static int iphone_get_adapter(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_IPHONE *d = (ALLEGRO_DISPLAY_IPHONE *)display;
   return d->extra->adapter;
}

static iphone_screen *iphone_get_screen_by_adapter(int adapter)
{
   al_lock_mutex(_al_iphone_display_hotplug_mutex);

   int num = [iphone_screens count];
   int i;
   iphone_screen *ret = NULL;

   for (i = 0; i < num; i++) {
      iphone_screen *scr = [iphone_screens objectAtIndex:i];
      if (scr->adapter == adapter) {
         ret = scr;
         break;
      }
   }

   al_unlock_mutex(_al_iphone_display_hotplug_mutex);
   return ret;
}

static iphone_screen *iphone_get_screen(ALLEGRO_DISPLAY *display)
{
   int adapter = iphone_get_adapter(display);
   return iphone_get_screen_by_adapter(adapter);
}

bool _al_iphone_is_display_connected(ALLEGRO_DISPLAY *display)
{
   iphone_screen *scr = iphone_get_screen(display);
   return scr && (scr->display == display);
}

static void iphone_remove_screen(UIScreen *screen)
{
   al_lock_mutex(_al_iphone_display_hotplug_mutex);

   int num_screens = [iphone_screens count];
   int i;

   for (i = 1; i < num_screens; i++) {
      iphone_screen *scr = iphone_get_screen_by_adapter(i);
      if (scr->screen == screen) {
         [iphone_screens removeObjectAtIndex:i];
         [scr release];
         break;
      }
   }

   al_unlock_mutex(_al_iphone_display_hotplug_mutex);
}

void _al_iphone_destroy_screen(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_IPHONE *d = (ALLEGRO_DISPLAY_IPHONE *)display;

   if (d->extra->adapter == 0) {
      global_delegate->main_display = NULL;
   }

   al_free(d->extra);
}

// create iphone_screen for all currently attached screens
static void iphone_create_screens(void)
{
   iphone_screens = [[NSMutableArray alloc] init];

   _al_iphone_display_hotplug_mutex = al_create_mutex_recursive();

   if ([UIScreen respondsToSelector:NSSelectorFromString(@"screens")]) {
      int num_screens;
      int i;

      num_screens = [[UIScreen screens] count];;
      for (i = 0; i < num_screens && i < 2; i++) {
         if (i == 1) {
            airplay_screen = [[UIScreen screens] objectAtIndex:i];
            continue;
         }
         [global_delegate add_screen:[[UIScreen screens] objectAtIndex:i]];
      }
   }
   else {
         [global_delegate add_screen:[UIScreen mainScreen]];
   }
}

/* Function: al_iphone_get_window
 */
UIWindow *al_iphone_get_window(ALLEGRO_DISPLAY *display)
{
   al_lock_mutex(_al_iphone_display_hotplug_mutex);
   iphone_screen *scr = iphone_get_screen(display);
   if (scr == NULL) {
      al_unlock_mutex(_al_iphone_display_hotplug_mutex);
      return NULL;
   }
   al_unlock_mutex(_al_iphone_display_hotplug_mutex);
   return scr->window;
}

/* Function: al_iphone_get_view
 */
UIView *al_iphone_get_view(ALLEGRO_DISPLAY *display)
{
   iphone_screen *scr = iphone_get_screen(display);
   if (scr == NULL) {
      return NULL;
   }
   return scr->vc.view;
}

static int iphone_orientation_to_allegro(UIDeviceOrientation orientation)
{
   switch (orientation) {
      case UIDeviceOrientationPortrait:
         return ALLEGRO_DISPLAY_ORIENTATION_0_DEGREES;

      case UIDeviceOrientationPortraitUpsideDown:
         return ALLEGRO_DISPLAY_ORIENTATION_180_DEGREES;

      case UIDeviceOrientationLandscapeRight:
         return ALLEGRO_DISPLAY_ORIENTATION_90_DEGREES;

      case UIDeviceOrientationLandscapeLeft:
         return ALLEGRO_DISPLAY_ORIENTATION_270_DEGREES;

      case UIDeviceOrientationFaceUp:
         return ALLEGRO_DISPLAY_ORIENTATION_FACE_UP;

      case UIDeviceOrientationFaceDown:
         return ALLEGRO_DISPLAY_ORIENTATION_FACE_DOWN;

      default:
         return ALLEGRO_DISPLAY_ORIENTATION_UNKNOWN;
   }
}

static void iphone_send_orientation_event(ALLEGRO_DISPLAY* display, int orientation)
{
   _al_event_source_lock(&display->es);
   if (_al_event_source_needs_to_generate_event(&display->es)) {
      ALLEGRO_EVENT event;
      event.display.type = ALLEGRO_EVENT_DISPLAY_ORIENTATION;
      event.display.timestamp = al_get_time();
      event.display.source = display;
      event.display.orientation = orientation;
      _al_event_source_emit_event(&display->es, &event);
   }
   _al_event_source_unlock(&display->es);
}


void _al_iphone_acknowledge_drawing_halt(ALLEGRO_DISPLAY *display)
{
   (void)display;
   waiting_for_program_halt = false;
}

/* Function: al_iphone_set_statusbar_orientation
 */
void al_iphone_set_statusbar_orientation(int o)
{
   UIInterfaceOrientation orientation = UIInterfaceOrientationPortrait;

   if (o == ALLEGRO_IPHONE_STATUSBAR_ORIENTATION_PORTRAIT)
      orientation = UIInterfaceOrientationPortrait;
   else if (o == ALLEGRO_IPHONE_STATUSBAR_ORIENTATION_PORTRAIT_UPSIDE_DOWN)
      orientation = UIInterfaceOrientationPortraitUpsideDown;
   else if (o == ALLEGRO_IPHONE_STATUSBAR_ORIENTATION_LANDSCAPE_RIGHT)
      orientation = UIInterfaceOrientationLandscapeRight;
   else if (o == ALLEGRO_IPHONE_STATUSBAR_ORIENTATION_LANDSCAPE_LEFT)
      orientation = UIInterfaceOrientationLandscapeLeft;

   [app setStatusBarOrientation:orientation animated:NO];
}

bool _al_iphone_add_view(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_IPHONE *d = (ALLEGRO_DISPLAY_IPHONE *)display;

   d->extra = al_calloc(1, sizeof(ALLEGRO_DISPLAY_IPHONE_EXTRA));
   int adapter = al_get_new_display_adapter();
   if (adapter < 0)
      adapter = 0;
   d->extra->adapter = adapter;

   /* This is the same as
    * [global_delegate.view add_view];
    * except it will run in the main thread.
    */
   [global_delegate performSelectorOnMainThread: @selector(add_view:)
                                     withObject: [NSValue valueWithPointer:display]
                                  waitUntilDone: YES];

   if (d->extra->failed) {
      return false;
   }

   /* There are two ways to get orientation information under ios - but they seem
    * to be mutually exclusive (just my experience, the documentation never says
    * they are).
    * One method has 6 orientations including face up and face down and is
    * independent of the screen orientations, just directly using the accelerometer
    * to determine how the device is positioned relative to gravity. The other
    * method has 4 orientations and simply tells how the view controller thinks
    * the user interface should be rotated.
    *
    * Supporting both at the same time is a) slightly confusing since we'd need
    * two sets of query functions and events and b) does not seem to work due to
    * the mutual exclusivity.
    *
    * Now normally using just the 4-orientation way would appear to be the best
    * choice as you can always use the accelerometer anyway to get the actual
    * 3d orientation and otherwise are likely more concerned with the orientation
    * things are being displayed at right now (instead of just seeing FACE_UP which
    * would not tell you).
    *
    * But to stay compatible with how things worked at first we still support the
    * 6-orientations way as well - but only if the display has a sole supported
    * orientation of ALLEGRO_DISPLAY_ORIENTATION_0_DEGREES.
    */
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *options = &display->extra_settings;
   int supported = options->settings[ALLEGRO_SUPPORTED_ORIENTATIONS];
   if (supported == ALLEGRO_DISPLAY_ORIENTATION_0_DEGREES) {
      [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
   }

   return true;
}

void _al_iphone_make_view_current(ALLEGRO_DISPLAY *display)
{
   iphone_screen *scr = iphone_get_screen(display);
   if (scr)
      [scr->view make_current];
}

void _al_iphone_recreate_framebuffer(ALLEGRO_DISPLAY *display)
{
   iphone_screen *scr = iphone_get_screen(display);
   if (scr) {
      EAGLView *view = scr->view;
      [view destroyFramebuffer];
      [view createFramebuffer];
      display->w = view.backingWidth;
      display->h = view.backingHeight;
      _al_ogl_resize_backbuffer(display->ogl_extras->backbuffer, display->w, display->h);
   }
}

void _al_iphone_flip_view(ALLEGRO_DISPLAY *display)
{
   iphone_screen *scr = iphone_get_screen(display);
   if (scr)
      [scr->view flip];
}

void _al_iphone_reset_framebuffer(ALLEGRO_DISPLAY *display)
{
   iphone_screen *scr = iphone_get_screen(display);
   if (scr)
      [scr->view reset_framebuffer];
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

int _al_iphone_get_num_video_adapters(void)
{
   if (![UIScreen respondsToSelector:NSSelectorFromString(@"screens")]) {
      return 1;
   }
   return [[UIScreen screens] count];
}

void _al_iphone_get_screen_size(int adapter, int *w, int *h)
{
   iphone_screen *scr = iphone_get_screen_by_adapter(adapter);
   UIScreen *screen = scr->screen;

   if (NULL != screen) {

      CGRect bounds = [screen bounds];
      CGFloat scale = 1.0f;

      if (adapter == 0 && [screen respondsToSelector:NSSelectorFromString(@"scale")]) {
         scale = [screen scale];
      }

      *w = (int)(bounds.size.width  * scale);
      *h = (int)(bounds.size.height * scale);
   }
   else {

      ASSERT("You should never see this message, unless Apple changed their policy and allows for removing screens from iDevices." && false);
   }
}

int _al_iphone_get_orientation(ALLEGRO_DISPLAY *display)
{
   if (display) {
      ALLEGRO_EXTRA_DISPLAY_SETTINGS *options = &display->extra_settings;
      int supported = options->settings[ALLEGRO_SUPPORTED_ORIENTATIONS];
      if (supported != ALLEGRO_DISPLAY_ORIENTATION_0_DEGREES) {
         iphone_screen *scr = iphone_get_screen(display);
         UIInterfaceOrientation o = scr->vc.interfaceOrientation;
         UIDeviceOrientation od = (int)o; /* They are compatible. */
         return iphone_orientation_to_allegro(od);
      }
   }

   UIDevice* device = [UIDevice currentDevice];

   if (NULL != device)
      return iphone_orientation_to_allegro([device orientation]);
   else
      return ALLEGRO_DISPLAY_ORIENTATION_UNKNOWN;
}

@implementation allegroAppDelegate

+ (void)run:(int)argc:(char **)argv {
   UIApplicationMain(argc, argv, nil, @"allegroAppDelegate");
}

- (void)orientation_change:(NSNotification *)notification
{
   (void)notification;

   int orientation = _al_iphone_get_orientation(NULL);

   if (main_display == NULL)
      return;

   iphone_send_orientation_event(main_display, orientation);
}

- (void)applicationDidFinishLaunching:(UIApplication *)application {
   ALLEGRO_INFO("App launched.\n");

   global_delegate = self;
   app = application;

   iphone_create_screens();

   // Register for device orientation notifications
   // Note: The notifications won't be generated unless they are enabled, which
   // we do elsewhere.
   [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(orientation_change:)
        name:UIDeviceOrientationDidChangeNotification object:nil];

   // Register for screen connect/disconnect notifications
   [self setupScreenConnectionNotificationHandlers];

   _al_iphone_run_user_main();
}

/* This may never get called on iOS 4 */
- (void)applicationWillTerminate:(UIApplication *)application {
    (void)application;
    ALLEGRO_EVENT event;
    ALLEGRO_DISPLAY *d = main_display;
    ALLEGRO_SYSTEM_IPHONE *iphone = (void *)al_get_system_driver();
    iphone->wants_shutdown = true;

    ALLEGRO_INFO("Terminating.\n");

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
   ALLEGRO_DISPLAY *d = main_display;
   ALLEGRO_EVENT event;

   (void)application;

   ALLEGRO_INFO("Application becoming inactive.\n");

   _al_event_source_lock(&d->es);
   if (_al_event_source_needs_to_generate_event(&d->es)) {
       event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_OUT;
       event.display.timestamp = al_current_time();
       _al_event_source_emit_event(&d->es, &event);
   }
   _al_event_source_unlock(&d->es);
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    ALLEGRO_DISPLAY *d = main_display;
   ALLEGRO_EVENT event;

   (void)application;

    ALLEGRO_INFO("Application becoming active.\n");

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

- (void)applicationDidEnterBackground:(UIApplication *)application {
   ALLEGRO_DISPLAY *d = main_display;
   ALLEGRO_EVENT event;

   (void)application;

   ALLEGRO_INFO("Application entering background.\n");

   waiting_for_program_halt = true;

   _al_event_source_lock(&d->es);
   if (_al_event_source_needs_to_generate_event(&d->es)) {
       event.display.type = ALLEGRO_EVENT_DISPLAY_HALT_DRAWING;
       event.display.timestamp = al_current_time();
       _al_event_source_emit_event(&d->es, &event);
   }
   _al_event_source_unlock(&d->es);

   while (waiting_for_program_halt) {
      // do nothing, this should be quick
   al_rest(0.001);
   }
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
   ALLEGRO_DISPLAY *d = main_display;
   ALLEGRO_EVENT event;

   (void)application;

   ALLEGRO_INFO("Application coming back to foreground.\n");

   _al_event_source_lock(&d->es);
   if (_al_event_source_needs_to_generate_event(&d->es)) {
       event.display.type = ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING;
       event.display.timestamp = al_current_time();
       _al_event_source_emit_event(&d->es, &event);
   }
   _al_event_source_unlock(&d->es);
}

/* Note: This must be called from the main thread. Ask Apple why - but I tried
 * it and otherwise things simply don't work (the screen just stays black).
 */
- (void)add_view:(NSValue *)value {
   ALLEGRO_DISPLAY *d = [value pointerValue];
   ALLEGRO_DISPLAY_IPHONE *disp = (ALLEGRO_DISPLAY_IPHONE *)d;

   int adapter = iphone_get_adapter(d);
   if (adapter == 0) {
      main_display = d;
   }

   iphone_screen *scr = iphone_get_screen_by_adapter(adapter);

   scr->display = d;

   ViewController *vc = scr->vc;
   vc->display = d;
   EAGLView *view = (EAGLView *)vc.view;
   [view set_allegro_display:d];

   [scr->window addSubview:view];

   if (adapter == 0) {
      [view becomeFirstResponder];
      [scr->window makeKeyAndVisible];
   }
   else {
      scr->window.hidden = NO;
   }

   disp->extra->failed = false;
   disp->extra->vc = vc;
   disp->extra->window = scr->window;
}

- (void)accelerometer:(UIAccelerometer*)accelerometer didAccelerate:(UIAcceleration*)acceleration
{
    (void)accelerometer;
    _al_iphone_generate_joystick_event(acceleration.x, acceleration.y, acceleration.z);
}

- (void)setupScreenConnectionNotificationHandlers
{
   /* Screen connect/disconnect notifications were added in iOS 3.2 */
   NSString *reqSysVer = @"3.2";
   NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
   BOOL osVersionSupported = ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending);

   if (osVersionSupported) {
      NSNotificationCenter* center = [NSNotificationCenter defaultCenter];

      [center addObserver:self selector:@selector(handleScreenConnectNotification:)
         name:UIScreenDidConnectNotification object:nil];
      [center addObserver:self selector:@selector(handleScreenDisconnectNotification:)
         name:UIScreenDidDisconnectNotification object:nil];
   }
}

- (void)handleScreenConnectNotification:(NSNotification*)aNotification
{
   airplay_screen = [aNotification object];

   ALLEGRO_DISPLAY *display = main_display;

   if (!display) return;

   _al_event_source_lock(&display->es);
   if (_al_event_source_needs_to_generate_event(&display->es)) {
      ALLEGRO_EVENT event;
      event.display.type = ALLEGRO_EVENT_DISPLAY_CONNECTED;
      event.display.timestamp = al_get_time();
      event.display.source = display;
      _al_event_source_emit_event(&display->es, &event);
   }
   _al_event_source_unlock(&display->es);
}

- (void)handleScreenDisconnectNotification:(NSNotification*)aNotification
{
   ALLEGRO_DISPLAY *display = main_display;
   ALLEGRO_DISPLAY_IPHONE *idisplay = (ALLEGRO_DISPLAY_IPHONE *)display;
   ALLEGRO_DISPLAY_IPHONE_EXTRA *extra;

   if (!display) return;

   extra = idisplay->extra;
   extra->disconnected = true;

   disconnect_wait = true;

   _al_event_source_lock(&display->es);
   if (_al_event_source_needs_to_generate_event(&display->es)) {
      ALLEGRO_EVENT event;
      event.display.type = ALLEGRO_EVENT_DISPLAY_DISCONNECTED;
      event.display.timestamp = al_get_time();
      event.display.source = display;
      _al_event_source_emit_event(&display->es, &event);
   }
   _al_event_source_unlock(&display->es);

   if (airplay_connected) {
      // wait for user to destroy display
      while (disconnect_wait)
         al_rest(0.001);

      iphone_remove_screen([aNotification object]);
      airplay_connected = false;
   }
}

- (void)dealloc {
   [super dealloc];
}

- (void)add_screen:(UIScreen *)screen
{
   int i;

   if (screen == [UIScreen mainScreen]) {
      i = 0;
   }
   else {
      for (i = 0; i < (int)[[UIScreen screens] count]; i++) {
         if ([[UIScreen screens] objectAtIndex:i] == screen)
            break;
      }
   }

   if (i != 0) {
      if ([screen respondsToSelector:NSSelectorFromString(@"availableModes")]) {
         NSArray *a = [screen availableModes];
         int j;
         UIScreenMode *largest = NULL;
         for (j = 0; j < (int)[a count]; j++) {
            UIScreenMode *m = [a objectAtIndex:j];
            float w = m.size.width;
            float h = m.size.height;
            if ((largest == NULL) || (w+h > largest.size.width+largest.size.height)) {
          largest = m;
            }
         }
         if (largest) {
            screen.currentMode = largest;
         }
      }
   }

   iphone_screen *scr = iphone_get_screen_by_adapter(i);
   bool add = scr == NULL;

   UIWindow *window = [[UIWindow alloc] initWithFrame:[screen bounds]];
   if ([window respondsToSelector:NSSelectorFromString(@"screen")]) {
       window.screen = screen;
   }

   ViewController *vc = [[ViewController alloc] init];
   vc->adapter = i;
   [vc create_view];
   // Doesn't work on iOS < 4.
   NSString *reqSysVer = @"4.0";
   NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
   BOOL osVersionSupported = ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending);
   if (osVersionSupported) {
      window.rootViewController = vc;
   }

   if (add)
      scr = [[iphone_screen alloc] init];

   scr->adapter = i;
   scr->screen = screen;
   scr->window = window;
   scr->vc = vc;
   scr->view = (EAGLView *)vc.view;
   scr->display = NULL;

   if (add) {
      al_lock_mutex(_al_iphone_display_hotplug_mutex);
      [iphone_screens addObject:scr];
      al_unlock_mutex(_al_iphone_display_hotplug_mutex);
   }
}

@end

void _al_iphone_connect_airplay(void)
{
   [global_delegate performSelectorOnMainThread: @selector(add_screen:) withObject:airplay_screen waitUntilDone:TRUE];
   airplay_connected = true;
}
