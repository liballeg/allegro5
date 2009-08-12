#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>

#import "EAGLView.h"
#include <pthread.h>

ALLEGRO_DEBUG_CHANNEL("iphone")

#define USE_DEPTH_BUFFER 0

@interface EAGLView ()

@property (nonatomic, retain) EAGLContext *context;

- (BOOL) createFramebuffer;
- (void) destroyFramebuffer;

@end


@implementation EAGLView

@synthesize context;


// You must implement this method
+ (Class)layerClass {
    return [CAEAGLLayer class];
}

- (void)set_allegro_display:(ALLEGRO_DISPLAY *)display {
    allegro_display = display;
}

- (id)initWithFrame:(CGRect)frame {
    
    self = [super initWithFrame:frame];
       
    // Get the layer
    CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;

    eaglLayer.opaque = YES;
    eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
        [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
        kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
    
    context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
    
    if (!context || ![EAGLContext setCurrentContext:context]) {
        [self release];
        return nil;
    }
   
   /* FIXME: Make this depend on a display setting. */
   [self setMultipleTouchEnabled:YES];

    ALLEGRO_INFO("Created EAGLView.\n");

    return self;
}

- (void)make_current {
    [EAGLContext setCurrentContext:context];
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
}

- (void)reset_framebuffer {
   glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
}

- (void)flip {
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
    [context presentRenderbuffer:GL_RENDERBUFFER_OES];
}

- (void)layoutSubviews {
    [EAGLContext setCurrentContext:context];
    [self destroyFramebuffer];
    [self createFramebuffer];
    ALLEGRO_INFO("Initialized EAGLView.\n");
}


- (BOOL)createFramebuffer {
    ALLEGRO_INFO("Creating GL framebuffer.\n");
    glGenFramebuffersOES(1, &viewFramebuffer);
    glGenRenderbuffersOES(1, &viewRenderbuffer);
    
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
    [context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:(CAEAGLLayer*)self.layer];
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, viewRenderbuffer);
    
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);
    
    if (USE_DEPTH_BUFFER) {
        glGenRenderbuffersOES(1, &depthRenderbuffer);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);
        glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, backingWidth, backingHeight);
        glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthRenderbuffer);
    }
    
    if(glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
        NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
        return NO;
    }
    
    return YES;
}


- (void)destroyFramebuffer {
    
    glDeleteFramebuffersOES(1, &viewFramebuffer);
    viewFramebuffer = 0;
    glDeleteRenderbuffersOES(1, &viewRenderbuffer);
    viewRenderbuffer = 0;
    
    if(depthRenderbuffer) {
        glDeleteRenderbuffersOES(1, &depthRenderbuffer);
        depthRenderbuffer = 0;
    }
}

- (void)dealloc {
    if ([EAGLContext currentContext] == context) {
        [EAGLContext setCurrentContext:nil];
    }
    
    [context release];  
    [super dealloc];
}

/* Handling of touch events. */

// Handles the start of a touch
-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
   // TODO: handle double-clicks (send two events?)
	// NSUInteger numTaps = [[touches anyObject] tapCount];
	// Enumerate through all the touch objects.
	NSUInteger touchCount = 0;
	for (UITouch *touch in touches) {
      CGPoint p = [touch locationInView:self];
      touchCount++;
		_al_iphone_generate_mouse_event(ALLEGRO_EVENT_MOUSE_BUTTON_DOWN,
                                      p.x, p.y, touchCount, allegro_display);
	}
}

// Handles the continuation of a touch.
-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{  
	
	NSUInteger touchCount = 0;
	// Enumerates through all touch objects
	for (UITouch *touch in touches) {
      CGPoint p = [touch locationInView:self];
      touchCount++;
      _al_iphone_generate_mouse_event(ALLEGRO_EVENT_MOUSE_AXES,
                                      p.x, p.y, touchCount, allegro_display);		
	}
}

// Handles the end of a touch event.
-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
   NSUInteger touchCount = 0;
	// Enumerates through all touch object
	for (UITouch *touch in touches) {
		CGPoint p = [touch locationInView:self];
      touchCount++;
      _al_iphone_generate_mouse_event(ALLEGRO_EVENT_MOUSE_BUTTON_UP,
                                      p.x, p.y, touchCount, allegro_display);		
	}
}

// Qooting Apple docs:
// "The system cancelled tracking for the touch, as when (for example) the user
// puts the device to his or her face."
-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	// Enumerates through all touch object
	for (UITouch *touch in touches) {
		// FIXME: We probably should generate ALLEGRO_EVENT_MOUSE_BUTTON_UP
      // for all currently pressed buttons.
	}
}

@end
