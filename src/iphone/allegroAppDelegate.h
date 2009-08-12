#import <UIKit/UIKit.h>

@class EAGLView;

@interface allegroAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    EAGLView *view;    
}

@property (nonatomic, retain) UIWindow *window;
@property (nonatomic, retain) EAGLView *view;

+ (void)run:(int)argc:(char **)argv;
- (void)add_view;

@end

