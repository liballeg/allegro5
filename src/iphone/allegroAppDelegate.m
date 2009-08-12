#import "allegroAppDelegate.h"
#import "EAGLView.h"

#import <UIKit/UIKit.h>
#include <pthread.h>


ALLEGRO_DEBUG_CHANNEL("iphone")

static allegroAppDelegate *global_delegate;

void _al_iphone_add_view(ALLEGRO_DISPLAY *display)
{
    // FIXME?
    [global_delegate.view set_allegro_display:display];
    [global_delegate.view make_current];
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
    
    // FIXME: The view should be added when a display is created, since only
    // then we can know things like color depth or depth buffer format. But it
    // has to be done from this thread, not the user thread. Will have to figure
    // something out.
    [global_delegate add_view];
    
    _al_iphone_run_user_main();    
}

- (void)add_view {
    window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    view = [[EAGLView alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    [window addSubview:view];
    [window makeKeyAndVisible];
}

- (void)dealloc {
	[window release];
	[super dealloc];
}

@end
