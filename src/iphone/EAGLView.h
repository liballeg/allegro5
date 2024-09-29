#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#include <allegro5/allegro.h>
#include <allegro5/internal/aintern_iphone.h>
#include <allegro5/internal/aintern_list.h>

/*
This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
The view content is basically an EAGL surface you render your OpenGL scene into.
Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
*/
@interface EAGLView : UIView {

@private
   EAGLContext *context;
   A5O_DISPLAY *allegro_display;

   /* OpenGL names for the renderbuffer and framebuffers used to render to this view */
   GLuint viewRenderbuffer, viewFramebuffer;
    
   /* OpenGL name for the depth buffer that is attached to viewFramebuffer, if it exists (0 if it does not exist) */
   GLuint depthRenderbuffer;
   
	/* Stuff for managing ID's for touches. List held struct which connect UITouch with ID on Allegro site.
    * NSMutableIndexSet hold list of free id's which were used. 'next_free_touch_id' hold next unused ID.
    *
    * 'touch_list' serve actuall as dictionary which map UITouch do ID.
    */
   _AL_LIST*            touch_list;
   NSMutableIndexSet*   touch_id_set;
   UITouch*             primary_touch;
   int                  next_free_touch_id;

   float                scale;
}

@property (nonatomic, retain) EAGLContext *context;
@property GLint backingWidth;
@property GLint backingHeight;


- (void)make_current;
- (void)flip;
- (void)reset_framebuffer;
- (void)set_allegro_display:(A5O_DISPLAY *)display;
- (BOOL) createFramebuffer;
- (void) destroyFramebuffer;
- (BOOL)orientation_supported:(UIInterfaceOrientation)o;
@end
