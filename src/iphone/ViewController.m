#import "ViewController.h"
#import "EAGLView.h"
#import <allegro5/allegro_iphone_objc.h>

#include "allegroAppDelegate.h"

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

- (BOOL)shouldAutorotate
{
   ALLEGRO_DISPLAY_IPHONE *d = (ALLEGRO_DISPLAY_IPHONE *)display;
   if (display == NULL)
      return YES;
   if (d->extra->adapter != 0) {
      return NO;
   }
   return YES;
}

/* Taken from Apple docs
 */
/*
typedef enum {
   UIInterfaceOrientationMaskPortrait = (1 << UIInterfaceOrientationPortrait),
   UIInterfaceOrientationMaskLandscapeLeft = (1 << UIInterfaceOrientationLandscapeLeft),
   UIInterfaceOrientationMaskLandscapeRight = (1 << UIInterfaceOrientationLandscapeRight),
   UIInterfaceOrientationMaskPortraitUpsideDown = (1 << UIInterfaceOrientationPortraitUpsideDown),
   UIInterfaceOrientationMaskLandscape =
   (UIInterfaceOrientationMaskLandscapeLeft | UIInterfaceOrientationMaskLandscapeRight),
   UIInterfaceOrientationMaskAll =
   (UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskLandscapeLeft |
   UIInterfaceOrientationMaskLandscapeRight | UIInterfaceOrientationMaskPortraitUpsideDown),
   UIInterfaceOrientationMaskAllButUpsideDown =
   (UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskLandscapeLeft |
   UIInterfaceOrientationMaskLandscapeRight),
} UIInterfaceOrientationMask;
*/

- (NSUInteger)supportedInterfaceOrientations
{
   if (!display) return UIInterfaceOrientationMaskAll;
   ALLEGRO_DISPLAY_IPHONE *d = (ALLEGRO_DISPLAY_IPHONE *)display;
   if (d->extra->adapter != 0) return UIInterfaceOrientationMaskAll;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *options = &display->extra_settings;
   int supported = options->settings[ALLEGRO_SUPPORTED_ORIENTATIONS];
   int mask = 0;

   switch (supported) {
      default:
      case ALLEGRO_DISPLAY_ORIENTATION_ALL:
         mask = UIInterfaceOrientationMaskAll;
         break;
      case ALLEGRO_DISPLAY_ORIENTATION_PORTRAIT:
         mask = UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskPortraitUpsideDown;
         break;
      case ALLEGRO_DISPLAY_ORIENTATION_LANDSCAPE:
         mask = UIInterfaceOrientationMaskLandscapeLeft | UIInterfaceOrientationMaskLandscapeRight;
         break;
      case ALLEGRO_DISPLAY_ORIENTATION_0_DEGREES:
         mask = UIInterfaceOrientationMaskPortrait;
         break;
      case ALLEGRO_DISPLAY_ORIENTATION_90_DEGREES:
         mask = UIInterfaceOrientationMaskLandscapeRight;
         break;
      case ALLEGRO_DISPLAY_ORIENTATION_180_DEGREES:
         mask = UIInterfaceOrientationMaskPortraitUpsideDown;
         break;
      case ALLEGRO_DISPLAY_ORIENTATION_270_DEGREES:
         mask = UIInterfaceOrientationMaskLandscapeLeft;
         break;
   }

   return mask;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
   ALLEGRO_DISPLAY_IPHONE *d = (ALLEGRO_DISPLAY_IPHONE *)display;
   if (display == NULL)
      return YES;
   if (d->extra->adapter != 0) {
      return NO;
   }
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
