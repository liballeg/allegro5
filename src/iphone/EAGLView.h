#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#include <allegro5/allegro5.h>
#include <allegro5/internal/aintern_iphone.h>

/*
This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
The view content is basically an EAGL surface you render your OpenGL scene into.
Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
*/
@interface EAGLView : UIView {

@private
	EAGLContext *context;
   GLint backingWidth;
   GLint backingHeight;
   ALLEGRO_DISPLAY *allegro_display;

   /* OpenGL names for the renderbuffer and framebuffers used to render to this view */
   GLuint viewRenderbuffer, viewFramebuffer;
    
   /* OpenGL name for the depth buffer that is attached to viewFramebuffer, if it exists (0 if it does not exist) */
   GLuint depthRenderbuffer;
}

@property (nonatomic, retain) EAGLContext *context;


- (void)make_current;
- (void)flip;
- (void)reset_framebuffer;
- (void)set_allegro_display:(ALLEGRO_DISPLAY *)display;
- (BOOL) createFramebuffer;
- (void) destroyFramebuffer;
@end
