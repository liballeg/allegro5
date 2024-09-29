#import <UIKit/UIKit.h>

struct A5O_DISPLAY;

@interface ViewController : UIViewController
{
@public
	int adapter;
	struct A5O_DISPLAY *display;
}

- (void) create_view;

@end
