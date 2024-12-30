#import <UIKit/UIKit.h>

struct ALLEGRO_DISPLAY;

@interface ViewController : UIViewController
{
@public
        int adapter;
        struct ALLEGRO_DISPLAY *display;
}

- (void) create_view;

@end
