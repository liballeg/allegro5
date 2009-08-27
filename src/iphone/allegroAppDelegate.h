#import <UIKit/UIKit.h>
#include <allegro5/allegro5.h>

@class EAGLView;

@interface allegroAppDelegate : NSObject <UIApplicationDelegate,
   UIAccelerometerDelegate> {
   UIWindow *window;
   EAGLView *view;
   ALLEGRO_DISPLAY *allegro_display;
   BOOL histeresisExcited;
   UIAcceleration *lastAcceleration;
}

@property (nonatomic, retain) UIWindow *window;
@property (nonatomic, retain) EAGLView *view;
@property (retain) UIAcceleration *lastAcceleration;

+ (void)run:(int)argc:(char **)argv;
- (void)add_view;
- (void)set_allegro_display:(ALLEGRO_DISPLAY *)d;

@end

