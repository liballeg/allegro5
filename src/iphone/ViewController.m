#import "ViewController.h"
#import "EAGLView.h"
#import <allegro5/allegro_iphone_objc.h>

ALLEGRO_DEBUG_CHANNEL("iphone");

@implementation ViewController

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
}

- (void)loadView
{
}

- (void)viewDidLoad
{
    ALLEGRO_DEBUG("Loading view controller.\n");
    display = NULL;
}

- (void)viewDidUnload
{
    [super viewDidUnload];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
   if (display == NULL)
      return NO;
   EAGLView *view = (EAGLView *)al_iphone_get_view(display);
   if (view) {
      return [view orientation_supported:interfaceOrientation];
   }
   return NO;
}

- (void) create_view
{
   UIScreen *screen;
   
   if (adapter == 0)
      screen = [UIScreen mainScreen];
   else
      screen = [[UIScreen screens] objectAtIndex:adapter];
   self.view = [[EAGLView alloc] initWithFrame:[screen bounds]];
   [self.view release];
}

@end
