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
    self.view = [[EAGLView alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    [self.view release];
}


- (void)viewDidUnload
{
    [super viewDidUnload];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
   EAGLView *view = (EAGLView *)al_iphone_get_view();
   if (view) {
      return [view orientation_supported:interfaceOrientation];
   }
   return NO;
}

@end
