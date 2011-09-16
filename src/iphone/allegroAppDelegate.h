#import <UIKit/UIKit.h>
#include <allegro5/allegro.h>
#import "ViewController.h"

@class EAGLView;

@interface allegroAppDelegate : NSObject <UIApplicationDelegate,
   UIAccelerometerDelegate> {
   UIWindow *window;
   EAGLView *view;
   ALLEGRO_DISPLAY *allegro_display;
}

@property (nonatomic, retain) UIWindow *window;
@property (nonatomic, retain) EAGLView *view;
@property (nonatomic, retain) ViewController *view_controller;

+ (void)run:(int)argc:(char **)argv;
- (void)add_view:(NSValue *)value;
- (void)display_splash_screen;
- (void)orientation_change:(NSNotification *)notification;

@end

