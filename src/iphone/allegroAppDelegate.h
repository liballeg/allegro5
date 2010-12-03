#import <UIKit/UIKit.h>
#include <allegro5/allegro.h>

@class EAGLView;

@interface allegroAppDelegate : NSObject <UIApplicationDelegate,
   UIAccelerometerDelegate> {
   UIWindow *window;
   EAGLView *view;
   ALLEGRO_DISPLAY *allegro_display;
}

@property (nonatomic, retain) UIWindow *window;
@property (nonatomic, retain) EAGLView *view;

+ (void)run:(int)argc:(char **)argv;
- (void)add_view;
- (void)set_allegro_display:(ALLEGRO_DISPLAY *)d;
- (void)display_splash_screen;
- (void)orientation_change:(NSNotification *)notification;

@end

