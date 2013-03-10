#import <UIKit/UIKit.h>
#include <allegro5/allegro.h>
#import "ViewController.h"

struct ALLEGRO_DISPLAY_IPHONE_EXTRA {
   bool failed;
   ViewController *vc;
   UIWindow *window;
   int adapter;
   bool disconnected;
};

@class EAGLView;

@interface allegroAppDelegate : NSObject <UIApplicationDelegate,
   UIAccelerometerDelegate> {
@public
   ALLEGRO_DISPLAY *main_display;
}

+ (void)run:(int)argc:(char **)argv;
- (void)add_view:(NSValue *)value;
- (void)orientation_change:(NSNotification *)notification;
- (void)setupScreenConnectionNotificationHandlers;
- (void)add_screen:(UIScreen *)screen;

@end

