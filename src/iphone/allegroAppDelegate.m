#import "allegroAppDelegate.h"
#import "EAGLView.h"

#import <UIKit/UIKit.h>
#include <pthread.h>


ALLEGRO_DEBUG_CHANNEL("iphone")

static allegroAppDelegate *global_delegate;

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

@implementation allegroAppDelegate

@synthesize window;
@synthesize view;

+ (void)run:(int)argc:(char **)argv {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    UIApplicationMain(argc, argv, nil, @"allegroAppDelegate");
    [pool release];
}

void _al_iphone_run_user_main(void);
- (void)applicationDidFinishLaunching:(UIApplication *)application {
   ALLEGRO_INFO("App launched.\n");
    
   application.statusBarHidden = true;
   global_delegate = self;

   _al_iphone_run_user_main();   
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

- (void)dealloc {
	[window release];
	[super dealloc];
}

@end
