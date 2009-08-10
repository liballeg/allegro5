/*         ______   ___    ___ 
*        /\  _  \ /\_ \  /\_ \ 
*        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
*         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
*          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
*           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
*            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
*                                           /\____/
*                                           \_/__/
*
*      MacOS X OpenGL gfx driver
*
*      By Peter Hull.
*
*      See readme.txt for copyright information.
*/


#include "allegro5/allegro5.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/platform/aintosx.h"
#include "./osxgl.h"
#ifndef ALLEGRO_MACOSX
#error something is wrong with the makefile
#endif

#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>

/* Defines */
#define MINIMUM_WIDTH 48
#define MINIMUM_HEIGHT 48

/* Module Variables */
static BOOL _osx_mouse_installed = NO, _osx_keyboard_installed = NO;
static NSPoint last_window_pos;
static unsigned int next_display_group = 1;
static BOOL in_fullscreen = NO;

/* Dictionary to map Allegro's DISPLAY_OPTIONS to OS X
 * PixelFormatAttributes. 
 * The first column is Allegro's name, the second column is the OS X
 * PixelFormatAttribute (or 0), the third column indicates whether we
 * need an extra parameter or not (eg, colour depth).
 */
static const unsigned int allegro_to_osx_settings[][3] = {
   { ALLEGRO_RED_SIZE,           0, 0},   // Not supported per component
   { ALLEGRO_GREEN_SIZE,         0, 0},   // Not supported per component
   { ALLEGRO_BLUE_SIZE,          0, 0},   // Not supported per component
   { ALLEGRO_ALPHA_SIZE,         NSOpenGLPFAAlphaSize, 1},
   { ALLEGRO_RED_SHIFT,          0, 0},   // Not available
   { ALLEGRO_GREEN_SHIFT,        0, 0},   // Not available
   { ALLEGRO_BLUE_SHIFT,         0, 0},   // Not available
   { ALLEGRO_ALPHA_SHIFT,        0, 0},   // Not available
   { ALLEGRO_ACC_RED_SIZE,       NSOpenGLPFAAccumSize, 1}, // Correct?
   { ALLEGRO_ACC_GREEN_SIZE,     NSOpenGLPFAAccumSize, 1}, // Correct?
   { ALLEGRO_ACC_BLUE_SIZE,      NSOpenGLPFAAccumSize, 1}, // Correct?
   { ALLEGRO_ACC_ALPHA_SIZE,     NSOpenGLPFAAccumSize, 1}, // Correct?
   { ALLEGRO_STEREO,             NSOpenGLPFAStereo, 0},
   { ALLEGRO_AUX_BUFFERS,        NSOpenGLPFAAuxBuffers, 1},
   { ALLEGRO_COLOR_SIZE,         NSOpenGLPFAColorSize, 1},
   { ALLEGRO_DEPTH_SIZE,         NSOpenGLPFADepthSize, 1},
   { ALLEGRO_STENCIL_SIZE,       NSOpenGLPFAStencilSize, 1},
   { ALLEGRO_SAMPLE_BUFFERS,     NSOpenGLPFASampleBuffers, 1},
   { ALLEGRO_SAMPLES,            NSOpenGLPFASamples, 1},
   { ALLEGRO_RENDER_METHOD,      NSOpenGLPFAAccelerated, 0},
   { ALLEGRO_FLOAT_COLOR,        NSOpenGLPFAColorFloat, 0},
   { ALLEGRO_FLOAT_DEPTH,        0, 0},
   { ALLEGRO_SINGLE_BUFFER ,     0, 0},   // Only have inverse of this
   { ALLEGRO_SWAP_METHOD,        0, 0},
   { ALLEGRO_COMPATIBLE_DISPLAY, 0, 0},
   { ALLEGRO_DISPLAY_OPTIONS_COUNT, 0, 0}
};
static const int number_of_settings =
   sizeof(allegro_to_osx_settings)/sizeof(*allegro_to_osx_settings);

/* Module functions */
static NSView* osx_view_from_display(ALLEGRO_DISPLAY* disp);
ALLEGRO_DISPLAY_INTERFACE* _al_osx_get_display_driver(void);
ALLEGRO_DISPLAY_INTERFACE* _al_osx_get_display_driver_win(void);
ALLEGRO_DISPLAY_INTERFACE* _al_osx_get_display_driver_fs(void);
static NSOpenGLContext* osx_create_shareable_context(NSOpenGLPixelFormat* fmt, unsigned int* group);

/* osx_change_cursor:
 * Actually change the current cursor. This can be called fom any thread 
 * but ensures that the change is only called from the main thread.
 */
static void osx_change_cursor(ALLEGRO_DISPLAY_OSX_WIN *dpy, NSCursor* cursor)
{
	NSCursor* old = dpy->cursor;
	dpy->cursor = [cursor retain];
	[old release];
	if (dpy->show_cursor)
		[cursor performSelectorOnMainThread: @selector(set) withObject: nil waitUntilDone: NO];
}

/* _al_osx_keyboard_was_installed:
 * Called by the keyboard driver when the driver is installed or uninstalled.
 * Set the variable so we can decide to pass events or not.
 */
void _al_osx_keyboard_was_installed(BOOL install) {
   _osx_keyboard_installed = install;
}

/* The main additions to this view are event-handling functions */
@interface ALOpenGLView : NSOpenGLView
{
	/* This is passed onto the event functions so we know where the event came from */
	ALLEGRO_DISPLAY* dpy_ptr;
}
-(void)setAllegroDisplay: (ALLEGRO_DISPLAY*) ptr;
-(ALLEGRO_DISPLAY*) allegroDisplay;
-(void) reshape;
-(BOOL) acceptsFirstResponder;
-(void) keyDown:(NSEvent*) event;
-(void) keyUp:(NSEvent*) event; 
-(void) flagsChanged:(NSEvent*) event;
-(void) mouseDown: (NSEvent*) evt;
-(void) mouseUp: (NSEvent*) evt;
-(void) mouseDragged: (NSEvent*) evt;
-(void) rightMouseDown: (NSEvent*) evt;
-(void) rightMouseUp: (NSEvent*) evt;
-(void) rightMouseDragged: (NSEvent*) evt;
-(void) otherMouseDown: (NSEvent*) evt;
-(void) otherMouseUp: (NSEvent*) evt;
-(void) otherMouseDragged: (NSEvent*) evt;
-(void) mouseMoved: (NSEvent*) evt;
-(void) scrollWheel: (NSEvent*) evt;
-(void) viewDidMoveToWindow;
-(void) viewWillMoveToWindow: (NSWindow*) newWindow;
-(void) mouseEntered: (NSEvent*) evt;
-(void) mouseExited: (NSEvent*) evt;
-(void) viewDidEndLiveResize;
/* Window delegate methods */
-(void) windowDidBecomeMain:(NSNotification*) notification;
-(void) windowDidResignMain:(NSNotification*) notification;
-(void) windowDidResize:(NSNotification*) notification;
@end

/* ALWindow:
 * This class is only here to return YES from canBecomeKeyWindow
 * to accept events when the window is frameless.
 */
@interface ALWindow : NSWindow
@end

@implementation ALWindow
-(BOOL) canBecomeKeyWindow 
{
	return YES;
}
@end

/* _al_osx_mouse_was_installed:
 * Called by the mouse driver when the driver is installed or uninstalled.
 * Set the variable so we can decide to pass events or not, and notify all
 * existing displays that they need to set up their tracking rectangles.
 */
void _al_osx_mouse_was_installed(BOOL install) {
   unsigned int i;
   if (_osx_mouse_installed == install) {
      // done it already
      return;
   }
   _osx_mouse_installed = install;
   _AL_VECTOR* dpys = &al_system_driver()->displays;
   for (i = 0; i < _al_vector_size(dpys); ++i) {
         ALLEGRO_DISPLAY* dpy = *(ALLEGRO_DISPLAY**) _al_vector_ref(dpys, i);
         NSView* view = osx_view_from_display(dpy);
         [[view window] setAcceptsMouseMovedEvents: _osx_mouse_installed];
   }
}

@implementation ALOpenGLView
/* setDisplay:
* Set the display this view is associated with
*/
-(void) setAllegroDisplay: (ALLEGRO_DISPLAY*) ptr {
	dpy_ptr = ptr;
}

/* display 
* return the display this view is associated with
*/
-(ALLEGRO_DISPLAY*) allegroDisplay {
	return dpy_ptr;
}

/* reshape
* Called when the view changes size */
- (void) reshape 
{
	if ([NSOpenGLContext currentContext] != nil) {
		NSRect rc = [self bounds];
		glViewport(0, 0, NSWidth(rc), NSHeight(rc));
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, NSWidth(rc), NSHeight(rc), 0, -1, 1);
		
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
      // To do: update the tracking rect
	}
}
/* acceptsFirstResponder
* Overridden to return YES, so that
* this view will receive events
*/
-(BOOL) acceptsFirstResponder
{
	return YES;
}

/* Keyboard event handler */
-(void) keyDown:(NSEvent*) event
{
	if (_osx_keyboard_installed)
		_al_osx_keyboard_handler(true, event, dpy_ptr);
}
-(void) keyUp:(NSEvent*) event 
{
	if (_osx_keyboard_installed)
		_al_osx_keyboard_handler(false, event, dpy_ptr);
}
-(void) flagsChanged:(NSEvent*) event
{
	if (_osx_keyboard_installed) {
		_al_osx_keyboard_modifiers([event modifierFlags], dpy_ptr);
	}
}

/* Mouse handling 
 */
-(void) mouseDown: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
      _al_osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) mouseUp: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	_al_osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) mouseDragged: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	_al_osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) rightMouseDown: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	_al_osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) rightMouseUp: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	_al_osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) rightMouseDragged: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	_al_osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) otherMouseDown: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	_al_osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) otherMouseUp: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	_al_osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) otherMouseDragged: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	_al_osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) mouseMoved: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	_al_osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) scrollWheel: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	_al_osx_mouse_generate_event(evt, dpy_ptr);
}
/* Cursor handling */
- (void) viewDidMoveToWindow {
   ALLEGRO_DISPLAY_OSX_WIN* dpy =  (ALLEGRO_DISPLAY_OSX_WIN*) dpy_ptr;
   dpy->tracking = [self addTrackingRect:[self bounds] owner:self userData:nil assumeInside:NO];
}

- (void) viewWillMoveToWindow: (NSWindow*) newWindow {
   ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) dpy_ptr;
   (void)newWindow;
   if (([self window] != nil) && (dpy->tracking != 0)) {
      [self removeTrackingRect:dpy->tracking];
   }
}

-(void) mouseEntered: (NSEvent*) evt
{
   ALLEGRO_DISPLAY_OSX_WIN* dpy =  (ALLEGRO_DISPLAY_OSX_WIN*) dpy_ptr;
	ALLEGRO_EVENT_SOURCE* src = &([self allegroDisplay]->es);
   if (dpy->show_cursor) {
      [dpy->cursor set];
   }
   else {
      [NSCursor hide];
   }
	_al_event_source_lock(src);
   _al_osx_switch_mouse_focus(dpy_ptr, true);
	_al_event_source_unlock(src);
   _al_osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) mouseExited: (NSEvent*) evt
{
   ALLEGRO_DISPLAY_OSX_WIN* dpy =  (ALLEGRO_DISPLAY_OSX_WIN*) dpy_ptr;
	ALLEGRO_EVENT_SOURCE* src = &([self allegroDisplay]->es);
   if (!dpy->show_cursor) {
      [NSCursor unhide];
   }
	_al_event_source_lock(src);
   _al_osx_switch_mouse_focus(dpy_ptr, false);
	_al_event_source_unlock(src);
	_al_osx_mouse_generate_event(evt, dpy_ptr);
}

/* windowShouldClose:
* Veto the close and post a message
*/
- (BOOL)windowShouldClose:(id)sender
{
   (void)sender;
	ALLEGRO_EVENT_SOURCE* src = &([self allegroDisplay]->es);
	_al_event_source_lock(src);
	ALLEGRO_EVENT evt;
	evt.type = ALLEGRO_EVENT_DISPLAY_CLOSE;
	_al_event_source_emit_event(src, &evt);
	_al_event_source_unlock(src);
	return NO;
}
-(void) viewDidEndLiveResize
{
   [super viewDidEndLiveResize];
	ALLEGRO_DISPLAY_OSX_WIN* dpy =  (ALLEGRO_DISPLAY_OSX_WIN*) dpy_ptr;
   NSWindow *window = dpy->win;
   NSRect rc = [window frame];
   NSRect content = [window contentRectForFrameRect: rc];
   ALLEGRO_EVENT_SOURCE *es = &dpy->parent.es;

   _al_event_source_lock(es);
   if (_al_event_source_needs_to_generate_event(es)) {
      ALLEGRO_EVENT event;
      event.display.type = ALLEGRO_EVENT_DISPLAY_RESIZE;
      event.display.timestamp = al_current_time();
      event.display.width = NSWidth(content);
      event.display.height = NSHeight(content);
      _al_event_source_emit_event(es, &event);
   }
   _al_event_source_unlock(es);
}
/* Window switch in/out */
-(void) windowDidBecomeMain:(NSNotification*) notification
{
   (void)notification;
	ALLEGRO_DISPLAY_OSX_WIN* dpy =  (ALLEGRO_DISPLAY_OSX_WIN*) dpy_ptr;
	ALLEGRO_EVENT_SOURCE* src = &([self allegroDisplay]->es);
	_al_event_source_lock(src);
	ALLEGRO_EVENT evt;
	evt.type = ALLEGRO_EVENT_DISPLAY_SWITCH_IN;
	_al_event_source_emit_event(src, &evt);
   _al_osx_switch_keyboard_focus(dpy_ptr, true);
	_al_event_source_unlock(src);
	osx_change_cursor(dpy, dpy->cursor);
}
-(void) windowDidResignMain:(NSNotification*) notification
{
   (void)notification;
	ALLEGRO_EVENT_SOURCE* src = &([self allegroDisplay]->es);
	_al_event_source_lock(src);
	ALLEGRO_EVENT evt;
	evt.type = ALLEGRO_EVENT_DISPLAY_SWITCH_OUT;
	_al_event_source_emit_event(src, &evt);
   _al_osx_switch_keyboard_focus(dpy_ptr, false);
	_al_event_source_unlock(src);
}
-(void) windowDidResize:(NSNotification*) notification
{
   (void)notification;
	ALLEGRO_DISPLAY_OSX_WIN* dpy =  (ALLEGRO_DISPLAY_OSX_WIN*) dpy_ptr;
   NSWindow *window = dpy->win;
   NSRect rc = [window frame];
   NSRect content = [window contentRectForFrameRect: rc];
   ALLEGRO_EVENT_SOURCE *es = &dpy->parent.es;

   _al_event_source_lock(es);
   if (_al_event_source_needs_to_generate_event(es)) {
      ALLEGRO_EVENT event;
      event.display.type = ALLEGRO_EVENT_DISPLAY_RESIZE;
      event.display.timestamp = al_current_time();
      event.display.width = NSWidth(content);
      event.display.height = NSHeight(content);
      _al_event_source_emit_event(es, &event);
   }
   _al_event_source_unlock(es);
}
/* End of ALOpenGLView implementation */
@end

/* osx_view_from_display:
 * given an ALLEGRO_DISPLAY, return the associated Cocoa View or nil
 * if fullscreen 
 */
static NSView* osx_view_from_display(ALLEGRO_DISPLAY* disp)
{
	NSWindow* window = ((ALLEGRO_DISPLAY_OSX_WIN*) disp)->win;
   return window == nil ? nil : [window contentView];
}

/* set_current_display:
* Set the current windowed display to be current.
*/
bool set_current_display(ALLEGRO_DISPLAY* d) {
	ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
	if (dpy->ctx != nil) {
		[dpy->ctx makeCurrentContext];
	}
   _al_ogl_set_extensions(d->ogl_extras->extension_api);
   return true;
}

/* Helper to set up GL state as we want it. */
static void setup_gl(ALLEGRO_DISPLAY *d)
{
	glViewport(0, 0, d->w, d->h);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, d->w, d->h, 0, -1, 1);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
	[dpy->ctx update];
}


/* Fills the array of NSOpenGLPixelFormatAttributes, which has a specfied
 * maximum size, with the options appropriate for the display.
 */
static void osx_set_opengl_pixelformat_attributes(ALLEGRO_DISPLAY_OSX_WIN *dpy)
{
   int i, n;
   bool want_double_buffer;
   NSOpenGLPixelFormatAttribute *a;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *extras;
   /* The following combination of flags indicates that multi-sampling is
    * requested.
    */
   const int sample_flags = (1<<ALLEGRO_SAMPLES)|(1<<ALLEGRO_SAMPLE_BUFFERS);

   /* Get extra display settings */
   extras = _al_get_new_display_settings();

   /* Begin by setting all elements in the array to 0 */
   memset(dpy->attributes, 0, AL_OSX_NUM_PFA*sizeof(*dpy->attributes));

   /* We normally want a double buffer */
   want_double_buffer = true;

   /* Begin with the first attribute */
   a = dpy->attributes;

   /* First, check the display flags, so we know if we want a fullscreen
    * mode or a windowed mode.
    */
   if (dpy->parent.flags & ALLEGRO_FULLSCREEN) {
      *a = NSOpenGLPFAFullScreen; a++;
      // Take over the screen.
       *a = NSOpenGLPFAScreenMask; a++;
       *a = CGDisplayIDToOpenGLDisplayMask(dpy->display_id); a++;
   } else {
      *a = NSOpenGLPFAWindow; a++;
   }

   /* Find the requested colour depth */
   if (extras)
      dpy->depth = extras->settings[ALLEGRO_COLOR_SIZE];
   if (!dpy->depth)
      dpy->depth = 32;  // FIXME
   *a = NSOpenGLPFAColorSize; a++;
   *a = dpy->depth; a++; 
   /* This should be set automatically because al_set_new_display_format
    * sets this.
    */

   /* Say we don't need an exact match for the depth of the colour buffer.
    * FIXME: right now, this is set whenever nothing is required or
    * something is suggested. Probably need finer control over this...
    */
   if (!extras || !(extras->required) || extras->suggested) {
      *a = NSOpenGLPFAClosestPolicy; a++;
   }

   /* Should we set double buffering? If it's not required we don't care
    * and go with the default.
    */
   if (extras && (extras->required & (1 << ALLEGRO_SINGLE_BUFFER)) && 
         extras->settings[ALLEGRO_SINGLE_BUFFER]) {
      want_double_buffer = false;
   }
   if (want_double_buffer) {
      *a = NSOpenGLPFADoubleBuffer; a++;
   }

   /* Detect if multi-sampling is requested */
   /* Or "NSOpenGLPFASupersample" ? */
   if (extras && (extras->required & sample_flags) &&
       (extras->settings[ALLEGRO_SAMPLES]||extras->settings[ALLEGRO_SAMPLE_BUFFERS])) {
      *a = NSOpenGLPFAMultisample; a++;
   }

   /* Now go through all other options, if set */
   for (n = 0; n < number_of_settings; n++) {
      i = allegro_to_osx_settings[n][0];
      if (allegro_to_osx_settings[n][1] && extras &&
          ((extras->required & (1 << i)) || (extras->suggested & (1 << i)))) {
         /* Need to distinguish between boolean attributes and settings that
          * require a value.
          */
         if (allegro_to_osx_settings[n][2]) {   /* Value */
            /* We must make sure the value is non-zero because the list are
             * building is 0-terminated.
             */
            if (extras->settings[i]) {
               *a = allegro_to_osx_settings[n][1]; a++;
               *a = extras->settings[i]; a++;
            }
         } else if (extras->settings[i]) {      /* Boolean, just turn this on */
            *a = allegro_to_osx_settings[n][1]; a++;
         }
      }
   }
}


/* Set the extra_settings[] array in the display, to report which options
 * we have selected.
 */
static void osx_get_opengl_pixelformat_attributes(ALLEGRO_DISPLAY_OSX_WIN *dpy)
{
   int n;
   NSOpenGLPixelFormatAttribute *a;
   NSOpenGLPixelFormatAttribute *attributes;

   if (!dpy) return;

   /* Get pixelformat attributes selected for the display
    * FIXME: right now, we just return the settings that were chosen by the
    * user. To be correct, we should query the pixelformat corresponding to
    * the display, using the NSOpenGLView pixelFormat method and the
    * NSOpenGLPixelFormat getValues:forAttribute:forVirtualScreen: method,
    * for which we need to know the logical screen number that the display
    * is on.
    */
   attributes = dpy->attributes;

   /* Clear list of settings (none selected) */
   memset(&dpy->parent.extra_settings, 0, sizeof(dpy->parent.extra_settings));

   for (n = 0; n < number_of_settings; n++) {
      /* Go through the list of options and relist the ones that we have
       * set to Allegro's option list.
       */
      a = dpy->attributes;
      while (*a) {
         if (*a == allegro_to_osx_settings[n][1]) {
            int al_setting = allegro_to_osx_settings[n][0];
            int value = 1;
            if (allegro_to_osx_settings[n][2])
               value = a[1];
            dpy->parent.extra_settings.settings[al_setting] = value;
         }
         /* Advance to next option */
         if (allegro_to_osx_settings[n][2]) /* Has a parameter in the list */
            a++;
         a++;
      }
   }
   dpy->parent.extra_settings.settings[ALLEGRO_COMPATIBLE_DISPLAY] = 1;
}


/* The purpose of this object is to provide a receiver for "perform on main 
 * thread" messages - we can't call C function directly so we do it
 * like this.
 * The perform on main thread mechanism is a simple way to avoid threading
 * issues.
 */
@interface ALDisplayHelper : NSObject 
+(void) initialiseDisplay: (NSValue*) display_object;
+(void) destroyDisplay: (NSValue*) display_object;
+(void) runFullScreenDisplay: (NSValue*) display_object;
@end

@implementation ALDisplayHelper
+(void) initialiseDisplay: (NSValue*) display_object {
   ALLEGRO_DISPLAY_OSX_WIN* dpy = [display_object pointerValue];
	NSRect rc = NSMakeRect(0, 0, dpy->parent.w,  dpy->parent.h);
	NSWindow* win = dpy->win = [ALWindow alloc]; 
   int adapter = al_get_current_video_adapter();
   NSScreen *screen;
   unsigned int mask = (dpy->parent.flags & ALLEGRO_NOFRAME) ? NSBorderlessWindowMask : 
      (NSTitledWindowMask|NSClosableWindowMask|NSMiniaturizableWindowMask);
   if (dpy->parent.flags & ALLEGRO_RESIZABLE) mask |= NSResizableWindowMask;
  
   if ((adapter >= 0) && (adapter < al_get_num_video_adapters())) {
      screen = [[NSScreen screens] objectAtIndex: adapter];
   } else {
      screen = [NSScreen mainScreen];
   }
	[win initWithContentRect: rc
				   styleMask: mask
					 backing: NSBackingStoreBuffered
					   defer: NO
                 screen: screen];
	NSOpenGLPixelFormat* fmt =
      [[NSOpenGLPixelFormat alloc] initWithAttributes: dpy->attributes];
	ALOpenGLView* view = [[ALOpenGLView alloc] initWithFrame: rc];
   dpy->ctx = osx_create_shareable_context(fmt, &dpy->display_group);
   [fmt release];
   [view setOpenGLContext: dpy->ctx];
	/* Hook up the view to its display */
	[view setAllegroDisplay: &dpy->parent];
	/* Realize the window on the main thread */
	[win setContentView: view];
	[win setDelegate: view];
	[win setReleasedWhenClosed: YES];
	[win setAcceptsMouseMovedEvents: _osx_mouse_installed];
	[win setTitle: @"Allegro"];
	/* Set minimum size, otherwise the window can be resized so small we can't
	 * grab the handle any more to make it bigger
	 */
	[win setMinSize: NSMakeSize(MINIMUM_WIDTH, MINIMUM_HEIGHT)];

   /* Place the window, respecting the location set by the user with
    * al_set_new_window_position().
    * If the user never called al_set_new_window_position, we simply let
    * the window manager pick a suitable location.
    *
    * CAUTION: the window manager under OS X requires that x and y be in
    * the range -16000 ... 16000 (approximately, probably the range of a
    * signed 16 bit integer). Should we check for this?
    */
   int new_x, new_y;
   al_get_new_window_position(&new_x, &new_y);
   if ((new_x != INT_MAX) && (new_y != INT_MAX)) {
      /* The user gave us window coordinates */
      NSRect rc = [win frame];
      NSRect sc = [[win screen] frame];
      NSPoint origin;

      /* We need to modify the y coordinate, cf. set_window_position */
      origin.x = sc.origin.x + new_x;
      origin.y = sc.origin.y + sc.size.height - rc.size.height - new_y;
      [win setFrameOrigin: origin];
   } 
   else {
      /* The window manager decides where to place the window */
      NSRect screen_rect = [screen frame];
      if (NSEqualPoints(last_window_pos, NSZeroPoint)) {
         /* We haven't positioned a window before, centre it */
         [win center];
      }

      /* We measure window positions relative to the origin of whatever the
       * screen we're placing the window on is, but OS X measures these
       * relative to a coordinate system spanning all of the screens, so we
       * need to translate to the new origin.
       * DON'T store the translated coordinates though, or the next window
       * may not end up where the user expects (so translate them back at
       * the end).
       */
      last_window_pos.x += screen_rect.origin.x;
      last_window_pos.y += screen_rect.origin.y;
      last_window_pos = [win cascadeTopLeftFromPoint:last_window_pos];
      last_window_pos.x -= screen_rect.origin.x;
      last_window_pos.y -= screen_rect.origin.y;
   }
   [win makeKeyAndOrderFront:self];
	if (!(mask & NSBorderlessWindowMask)) [win makeMainWindow];
	[view release];
}
+(void) destroyDisplay: (NSValue*) display_object {
   ALLEGRO_DISPLAY_OSX_WIN* dpy = [display_object pointerValue];
   _al_vector_find_and_delete(&al_system_driver()->displays, &dpy);
   // Disconnect from its view or exit fullscreen mode
   [dpy->ctx clearDrawable];
   // Unlock the screen 
   if (dpy->parent.flags & ALLEGRO_FULLSCREEN) {
      CGDisplayRelease(dpy->display_id);
      in_fullscreen = false;
   }
   else {
      // Destroy the containing window if there is one
      [dpy->win close];
      dpy->win = nil;      
   }
}
/* runFullScreenDisplay:
 * Capture the display and enter fullscreen mode. Do not leave this function
 * until full screen is cancelled
 */
+(void) runFullScreenDisplay: (NSValue*) display_object
{
   ALLEGRO_DISPLAY* display = (ALLEGRO_DISPLAY*) [display_object pointerValue];
   while (in_fullscreen) {
      NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	  // Collect an event
      NSEvent* event = [NSApp nextEventMatchingMask:NSAnyEventMask 
                                          untilDate:[NSDate distantFuture] 
                                             inMode:NSDefaultRunLoopMode 
                                            dequeue:YES];
      // Process it as required.
	  switch ([event type]) {
         case NSKeyDown:
            _al_osx_keyboard_handler(true,event,display);
            break;
         case NSKeyUp:
            _al_osx_keyboard_handler(false,event,display);
            break;
         case NSFlagsChanged:
            _al_osx_keyboard_modifiers([event modifierFlags],display);
            break;
         case NSLeftMouseDown:
         case NSLeftMouseUp:
         case NSRightMouseDown:
         case NSRightMouseUp:
         case NSOtherMouseDown:
         case NSOtherMouseUp:
         case NSMouseMoved:
         case NSLeftMouseDragged:
         case NSRightMouseDragged:
         case NSOtherMouseDragged:
            if (_osx_mouse_installed) 
               _al_osx_mouse_generate_event(event, display);
            break;
         default:
            [NSApp sendEvent: event];
            break;
      }
	  [pool release];
   }
}
/* End of ALDisplayHelper implementation */
@end

/* osx_create_shareable_context:
 *
 * Create an NSOpenGLContext with a given pixel format. If possible, make
 * the context compatible with one that has already been created and 
 * assigned to a display. If this can't be done, create an unshared one
 * (which may itself be shared in the future)
 * Each context is given a group number so that all shared contexts have the
 * same group number.
 *
 * Parameters:
 *  fmt - The pixel format to use
 *  group - Pointer to the assigned group for the new context
 *
 * Returns:
 *  The new context or nil if it cannot be created.
 */ 
static NSOpenGLContext* osx_create_shareable_context(NSOpenGLPixelFormat* fmt, unsigned int* group)
{
   // Iterate through all existing displays and try and find one that's compatible
   _AL_VECTOR* dpys = &al_system_driver()->displays;
   unsigned int i;
   NSOpenGLContext* compat = nil;
   
   for (i = 0; i < _al_vector_size(dpys); ++i) {
      ALLEGRO_DISPLAY_OSX_WIN* other = *(ALLEGRO_DISPLAY_OSX_WIN**) _al_vector_ref(dpys, i);
      compat = [[NSOpenGLContext alloc] initWithFormat:fmt shareContext: other->ctx];
      if (compat != nil) {
      // OK, we can share with this one
         *group = other->display_group;
         break;
      }
   }
   if (compat == nil) {
      // Set to a new group
      *group = next_display_group++;
      compat = [[NSOpenGLContext alloc] initWithFormat:fmt shareContext: nil];      
   }
   return compat;
}
/* create_display_fs:
* Create a fullscreen display - capture the display
*/
static ALLEGRO_DISPLAY* create_display_fs(int w, int h) {
   if (al_get_current_video_adapter() >= al_get_num_video_adapters())
      return NULL;
	ALLEGRO_DISPLAY_OSX_WIN* dpy = _AL_MALLOC(sizeof(ALLEGRO_DISPLAY_OSX_WIN));
	if (dpy == NULL) {
		return NULL;
	}
   memset(dpy, 0, sizeof(*dpy));
	/* Set up the ALLEGRO_DISPLAY part */
	dpy->parent.vt = _al_osx_get_display_driver_fs();
	dpy->parent.refresh_rate = al_get_new_display_refresh_rate();
	dpy->parent.flags = al_get_new_display_flags() | ALLEGRO_OPENGL | ALLEGRO_FULLSCREEN;
	_al_event_source_init(&dpy->parent.es);
   dpy->cursor = [[NSCursor arrowCursor] retain];
   dpy->display_id = CGMainDisplayID();
   /* Get display ID for the requested display */
   if (al_get_current_video_adapter() > 0) {
      int adapter = al_get_current_video_adapter();
      NSScreen *screen = [[NSScreen screens] objectAtIndex: adapter];
      NSDictionary *dict = [screen deviceDescription];
      NSNumber *display_id = [dict valueForKey: @"NSScreenNumber"];
      dpy->display_id = (CGDirectDisplayID) [display_id pointerValue];
   }
   // Set up a pixel format to describe the mode we want.
   osx_set_opengl_pixelformat_attributes(dpy);
   NSOpenGLPixelFormat* fmt =
      [[NSOpenGLPixelFormat alloc] initWithAttributes: dpy->attributes];
   if (fmt == nil) return NULL;
   // Create a context which shares with any other contexts with the same format
   NSOpenGLContext* context = osx_create_shareable_context(fmt, &dpy->display_group);
   [fmt release];
   if (context == nil) return NULL;
   [context makeCurrentContext];
   /* Turn on vsyncing possibly */
   if (_al_get_new_display_settings()->settings[ALLEGRO_VSYNC] == 1) {
      GLint swapInterval = 1;
      [context setValues:&swapInterval forParameter: NSOpenGLCPSwapInterval];
   }
   else {
      GLint swapInterval = 0;
      [context setValues:&swapInterval forParameter: NSOpenGLCPSwapInterval];
   }
   dpy->ctx = context;
   dpy->parent.w = w;
   dpy->parent.h = h;
   // Prevent other apps from writing to this display and switch it to our
   // chosen mode.
   CGDisplayCapture(dpy->display_id);
   CFDictionaryRef mode = CGDisplayBestModeForParametersAndRefreshRate(dpy->display_id, dpy->depth, w, h, dpy->parent.refresh_rate, NULL);
   CGDisplaySwitchToMode(dpy->display_id, mode);
   [context setFullScreen];
   // Set up the Allegro OpenGL implementation
   dpy->parent.ogl_extras = _AL_MALLOC(sizeof(ALLEGRO_OGL_EXTRAS));
   memset(dpy->parent.ogl_extras, 0, sizeof(ALLEGRO_OGL_EXTRAS));
   _al_ogl_manage_extensions(&dpy->parent);
   _al_ogl_set_extensions(dpy->parent.ogl_extras->extension_api);
	dpy->parent.ogl_extras->backbuffer = _al_ogl_create_backbuffer(&dpy->parent);
   dpy->parent.ogl_extras->is_shared = true;
	/* Set up GL as we want */
	setup_gl(&dpy->parent);
   /* Clear and flush (for double buffering) */
   glClear(GL_COLOR_BUFFER_BIT);
   [context flushBuffer];
   glClear(GL_COLOR_BUFFER_BIT);

   /* Add to the display list */
	ALLEGRO_DISPLAY **add = _al_vector_alloc_back(&al_system_driver()->displays);
	*add = &dpy->parent;
   in_fullscreen = YES;
   // Begin the 'private' event loop 
   // Necessary because there's no NSResponder (i.e. a view) to collect 
   // events from the window server.
   [ALDisplayHelper performSelectorOnMainThread: @selector(runFullScreenDisplay:) 
                                     withObject: [NSValue valueWithPointer:dpy] 
                                  waitUntilDone: NO];
   /* Retrieve the options that were set */
   osx_get_opengl_pixelformat_attributes(dpy);
   return &dpy->parent;
}

/* create_display_win:
* Create a windowed display - create the window with an ALOpenGLView
* to be its content view
*/
static ALLEGRO_DISPLAY* create_display_win(int w, int h) {
   if (al_get_current_video_adapter() >= al_get_num_video_adapters())
      return NULL;
	ALLEGRO_DISPLAY_OSX_WIN* dpy = _AL_MALLOC(sizeof(ALLEGRO_DISPLAY_OSX_WIN));
	if (dpy == NULL) {
		return NULL;
	}
   memset(dpy, 0, sizeof(*dpy));
	/* Set up the ALLEGRO_DISPLAY part */
	dpy->parent.vt = _al_osx_get_display_driver_win();
	dpy->parent.refresh_rate = al_get_new_display_refresh_rate();
	dpy->parent.flags = al_get_new_display_flags() | ALLEGRO_OPENGL | ALLEGRO_WINDOWED;
	dpy->parent.w = w;
	dpy->parent.h = h;
	_al_event_source_init(&dpy->parent.es);
	osx_change_cursor(dpy, [NSCursor arrowCursor]);
   dpy->show_cursor = YES;
   
   if (_al_vector_is_empty(&al_system_driver()->displays)) {
      last_window_pos = NSZeroPoint;
   }
   osx_set_opengl_pixelformat_attributes(dpy);
   /* OSX specific part - finish the initialisation on the main thread */
   [ALDisplayHelper performSelectorOnMainThread: @selector(initialiseDisplay:) 
      withObject: [NSValue valueWithPointer:dpy] 
      waitUntilDone: YES];
	[dpy->ctx makeCurrentContext];
   dpy->parent.ogl_extras = _AL_MALLOC(sizeof(ALLEGRO_OGL_EXTRAS));
   memset(dpy->parent.ogl_extras, 0, sizeof(ALLEGRO_OGL_EXTRAS));
   _al_ogl_manage_extensions(&dpy->parent);
   _al_ogl_set_extensions(dpy->parent.ogl_extras->extension_api);
	dpy->parent.ogl_extras->backbuffer = _al_ogl_create_backbuffer(&dpy->parent);
   dpy->parent.ogl_extras->is_shared = true;

   /* Retrieve the options that were set */
   osx_get_opengl_pixelformat_attributes(dpy);
   
   if (_al_get_new_display_settings()->settings[ALLEGRO_VSYNC] == 1) {
      GLint swapInterval = 1;
      [dpy->ctx setValues:&swapInterval forParameter: NSOpenGLCPSwapInterval];
   }
   else {
      GLint swapInterval = 0;
      [dpy->ctx setValues:&swapInterval forParameter: NSOpenGLCPSwapInterval];
   }

	/* Set up GL as we want */
	setup_gl(&dpy->parent);
	ALLEGRO_DISPLAY **add = _al_vector_alloc_back(&al_system_driver()->displays);
	return *add = &dpy->parent;
}
/* destroy_display:
 * Destroy display, actually close the window or exit fullscreen on the main thread
 */
static void destroy_display(ALLEGRO_DISPLAY* d) {
   ALLEGRO_DISPLAY *old_dpy = al_get_current_display();
	ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
   ALLEGRO_DISPLAY_OSX_WIN* other = NULL;
   unsigned int i;

   // Set the display as the current display; needed because we need to
   // make the context current.
   if (old_dpy != d)
      al_set_current_display(d);

   /* First of all, save video bitmaps attached to this display. */
   // Check for other displays in this display group
   _AL_VECTOR* dpys = &al_system_driver()->displays;
   for (i = 0; i < _al_vector_size(dpys); ++i) {
      ALLEGRO_DISPLAY_OSX_WIN* d = *(ALLEGRO_DISPLAY_OSX_WIN**) _al_vector_ref(dpys, i);
      if (d->display_group == dpy->display_group && (d!=dpy)) {
         other = d;
         break;
      }
   }
   if (other != NULL) {
      // Found another compatible display. Transfer our bitmaps to it.
      _AL_VECTOR* bmps = &dpy->parent.bitmaps;
      for (i = 0; i<_al_vector_size(bmps); ++i) {
         ALLEGRO_BITMAP **add = _al_vector_alloc_back(&other->parent.bitmaps);
         ALLEGRO_BITMAP **ref = _al_vector_ref(bmps, i);
         *add = *ref;
         (*add)->display = &(other->parent);
      }
   }
   else {
      // This is the last in its group. Convert all its bitmaps to memory bmps
      while (dpy->parent.bitmaps._size > 0) {
         ALLEGRO_BITMAP **bptr = _al_vector_ref_back(&dpy->parent.bitmaps);
         ALLEGRO_BITMAP *bmp = *bptr;
         _al_convert_to_memory_bitmap(bmp);
      }
   }
   _al_vector_free(&dpy->parent.bitmaps);

   [ALDisplayHelper performSelectorOnMainThread: @selector(destroyDisplay:) 
      withObject: [NSValue valueWithPointer:dpy] 
      waitUntilDone: YES];
   _al_ogl_unmanage_extensions(&dpy->parent);
	[dpy->ctx release];
   [dpy->cursor release];
	_al_event_source_free(&d->es);
   _AL_FREE(d->ogl_extras);

   // Restore original display from before this function was called.
   // If the display we just destroyed is actually current, set the current
   // display to NULL.
   if (old_dpy != d)
      al_set_current_display(old_dpy);
   else
      al_set_current_display(NULL);

   _AL_FREE(d);
}
/* create_display:
 * Create a display either fullscreen or windowed depending on flags
 */
static ALLEGRO_DISPLAY* create_display(int w, int h)
{
   int flags = al_get_new_display_flags();
   if (flags & ALLEGRO_FULLSCREEN) {
      return create_display_fs(w,h);
   }
   else {
      return create_display_win(w,h);
   }
}
/* Note: in windowed mode, contexts always behave like single-buffered
 * though in fact they are composited offscreen */
static void flip_display(ALLEGRO_DISPLAY *disp) 
{
   ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) disp;
   if (disp->ogl_extras->opengl_target->is_backbuffer) {
      if (disp->extra_settings.settings[ALLEGRO_SINGLE_BUFFER]) {
         glFlush();
      }
      else {
         [dpy->ctx flushBuffer];
      }
   }
}

static void update_display_region(ALLEGRO_DISPLAY *disp,
   int x, int y, int width, int height)
{
   (void)x;
   (void)y;
   (void)width;
   (void)height;
   flip_display(disp);
}

/* osx_create_mouse_cursor:
 * creates a custom system cursor from the bitmap bmp.
 * (x_focus, y_focus) indicates the cursor hot-spot.
 */
static ALLEGRO_MOUSE_CURSOR *osx_create_mouse_cursor(ALLEGRO_DISPLAY *display,
   ALLEGRO_BITMAP *bmp, int x_focus, int y_focus) {
	ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) display;
   ALLEGRO_MOUSE_CURSOR_OSX *cursor = NULL;
	
   if (!bmp || !dpy)
      return NULL;

	NSImage* cursor_image = NSImageFromAllegroBitmap(bmp);
   cursor = _AL_MALLOC(sizeof *cursor);
	cursor->cursor = [[NSCursor alloc] initWithImage: cursor_image
									 hotSpot: NSMakePoint(x_focus, y_focus)];
	[cursor_image release];

   return (ALLEGRO_MOUSE_CURSOR *)cursor;
}
 
/* osx_destroy_mouse_cursor:
 * destroys a mouse cursor previously created with osx_create_mouse_cursor
 */
static void osx_destroy_mouse_cursor(ALLEGRO_DISPLAY *display,
   ALLEGRO_MOUSE_CURSOR *curs) {
	ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) display;
   ALLEGRO_MOUSE_CURSOR_OSX *cursor = (ALLEGRO_MOUSE_CURSOR_OSX *) curs;

   if (!dpy || !cursor)
      return;

   osx_change_cursor(dpy, [NSCursor arrowCursor]);

   [cursor->cursor release];
   _AL_FREE(cursor);
}

/* osx_set_mouse_cursor:
 * change the mouse cursor for the active window to the cursor previously
 * allocated by osx_create_mouse_cursor
 */
static bool osx_set_mouse_cursor(ALLEGRO_DISPLAY *display,
   ALLEGRO_MOUSE_CURSOR *cursor)
{
   ALLEGRO_DISPLAY_OSX_WIN *dpy = (ALLEGRO_DISPLAY_OSX_WIN *)display;
   ALLEGRO_MOUSE_CURSOR_OSX *osxcursor = (ALLEGRO_MOUSE_CURSOR_OSX *)cursor;

   osx_change_cursor(dpy, osxcursor->cursor);

   return true;
}

/* osx_set_system_mouse_cursor:
 * change the mouse cursor to one of the system default cursors.
 * NOTE: Allegro defines four of these, but OS X has no dedicated "busy" or
 * "question" cursors, so we just set an arrow in those cases.
 *
 * TODO: maybe Allegro could expose more of the system default cursors?
 * Check with other platforms!
 */
static bool osx_set_system_mouse_cursor(ALLEGRO_DISPLAY *display,
   ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id) {
   ALLEGRO_DISPLAY_OSX_WIN *dpy = (ALLEGRO_DISPLAY_OSX_WIN *)display;
   NSCursor *requested_cursor = NULL;

   switch (cursor_id) {
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_ARROW:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_BUSY:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_QUESTION:
         requested_cursor = [NSCursor arrowCursor];
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_EDIT:
         requested_cursor = [NSCursor IBeamCursor];
         break;
      default:
         return false;
   }

   osx_change_cursor(dpy, requested_cursor);
   return true;
}

/* (show|hide)_cursor:
 Cursor show or hide. The same function works for both windowed and fullscreen.
 */
static bool show_cursor(ALLEGRO_DISPLAY *d) {
	ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
   dpy->show_cursor = YES;
   [NSCursor unhide];
   return true;
}

static bool hide_cursor(ALLEGRO_DISPLAY *d) {
	ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
   dpy->show_cursor = NO;
   [NSCursor hide];
   return true;
}

static bool acknowledge_resize_display_win(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_OSX_WIN *dpy = (ALLEGRO_DISPLAY_OSX_WIN *)d;
   NSWindow* window = dpy->win;
   NSRect frame = [window frame];
   NSRect content = [window contentRectForFrameRect: frame];

   d->w = NSWidth(content);
   d->h = NSHeight(content);

   _al_ogl_resize_backbuffer(d->ogl_extras->backbuffer, d->w, d->h);
   setup_gl(d);

   return true;
}

/* resize_display_(win|fs)
 Change the size of the display by altering the window size or changing the screen mode
 */
static bool resize_display_win(ALLEGRO_DISPLAY *d, int w, int h) {
	ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
   NSWindow* window = dpy->win;
   NSRect current = [window frame];
   w = _ALLEGRO_MAX(w, MINIMUM_WIDTH);
   h = _ALLEGRO_MAX(h, MINIMUM_HEIGHT);
   NSRect rc = [window frameRectForContentRect: NSMakeRect(0.0f, 0.0f, (float) w, (float) h)];
   rc.origin = current.origin;
   [window setFrame:rc display:YES animate:YES];
   return acknowledge_resize_display_win(d);
}
static bool resize_display_fs(ALLEGRO_DISPLAY *d, int w, int h) {
	ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
	CFDictionaryRef current = CGDisplayCurrentMode(dpy->display_id);
	CFNumberRef bps = (CFNumberRef) CFDictionaryGetValue(current, kCGDisplayBitsPerPixel);
	int b;
	CFNumberGetValue(bps, kCFNumberSInt32Type,&b);
	CFDictionaryRef mode = CGDisplayBestModeForParameters(dpy->display_id, b, w, h, NULL);
	[dpy->ctx clearDrawable];
	CGError err = CGDisplaySwitchToMode(dpy->display_id, mode);
	d->w = w;
	d->h = h;
	[dpy->ctx setFullScreen];
   _al_ogl_resize_backbuffer(d->ogl_extras->backbuffer, d->w, d->h);
	setup_gl(d);
	return  err == kCGErrorSuccess;
}

static bool is_compatible_bitmap(ALLEGRO_DISPLAY* disp, ALLEGRO_BITMAP* bmp) {
   return (bmp->display == disp)
      || (((ALLEGRO_DISPLAY_OSX_WIN*) bmp->display)->display_group == ((ALLEGRO_DISPLAY_OSX_WIN*) disp)->display_group);
}

/* set_window_position:
 * Set the position of the window that owns this display
 * Slightly complicated because Allegro measures from the top down to
 * the top left corner, OS X from the bottom up to the bottom left corner.
 */
static void set_window_position(ALLEGRO_DISPLAY* display, int x, int y) 
{
   ALLEGRO_DISPLAY_OSX_WIN* d = (ALLEGRO_DISPLAY_OSX_WIN*) display;
   NSWindow* window = d->win;
   NSRect rc = [window frame];
   NSRect sc = [[window screen] frame];
   rc.origin.x = (float) x;
   rc.origin.y = sc.size.height - rc.size.height - ((float) y);
   // Couldn't resist animating it!
   [window setFrame:rc display:YES animate:YES];
}

/* get_window_position:
 * Get the position of the window that owns this display. See comment for
 * set_window_position.
 */
static void get_window_position(ALLEGRO_DISPLAY* display, int* px, int* py) 
{
   ALLEGRO_DISPLAY_OSX_WIN* d = (ALLEGRO_DISPLAY_OSX_WIN*) display;
   NSWindow* window = d->win;
   NSRect rc = [window frame];
   NSRect sc = [[window screen] frame];
   *px = (int) rc.origin.x;
   *py = (int) (sc.size.height - rc.origin.y - rc.size.height);
}

/* set_window_title:
 * Set the title of the window with this display
 */
static void set_window_title(ALLEGRO_DISPLAY *display, AL_CONST char *title)
{
   ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) display;
   [dpy->win setTitle: [NSString stringWithCString:title]];
}

/* set_icon:
 * Set the icon - OS X doesn't have per-window icons so 
 * ignore the display parameter
 */
static void set_icon(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP* bitmap)
{
   (void)display;
   [NSApp setApplicationIconImage: NSImageFromAllegroBitmap(bitmap)];
}

ALLEGRO_DISPLAY_INTERFACE* _al_osx_get_display_driver_win(void)
{
	static ALLEGRO_DISPLAY_INTERFACE* vt = NULL;
   if (vt == NULL) {
      vt = _AL_MALLOC(sizeof(*vt));
      memset(vt, 0, sizeof(ALLEGRO_DISPLAY_INTERFACE));
      vt->create_display = create_display_win;
      vt->destroy_display = destroy_display;
      vt->set_current_display = set_current_display;
      vt->flip_display = flip_display;
      vt->update_display_region = update_display_region;
      vt->resize_display = resize_display_win;
      vt->acknowledge_resize = acknowledge_resize_display_win;
      vt->create_bitmap = _al_ogl_create_bitmap;
      vt->set_target_bitmap = _al_ogl_set_target_bitmap;
      vt->get_backbuffer = _al_ogl_get_backbuffer;
      vt->is_compatible_bitmap = is_compatible_bitmap;
      vt->create_sub_bitmap = _al_ogl_create_sub_bitmap;
      vt->show_mouse_cursor = show_cursor;
      vt->hide_mouse_cursor = hide_cursor;
      vt->create_mouse_cursor = osx_create_mouse_cursor;
      vt->destroy_mouse_cursor = osx_destroy_mouse_cursor;
      vt->set_mouse_cursor = osx_set_mouse_cursor;
      vt->set_system_mouse_cursor = osx_set_system_mouse_cursor;
      vt->get_window_position = get_window_position;
      vt->set_window_position = set_window_position;
      vt->set_window_title = set_window_title;
      vt->set_icon = set_icon;
      _al_ogl_add_drawing_functions(vt);
   }
	return vt;
}   

ALLEGRO_DISPLAY_INTERFACE* _al_osx_get_display_driver_fs(void)
{
	static ALLEGRO_DISPLAY_INTERFACE* vt = NULL;
   if (vt == NULL) {
      vt = _AL_MALLOC(sizeof(*vt));
      memset(vt, 0, sizeof(ALLEGRO_DISPLAY_INTERFACE));
      vt->create_display = create_display_fs;
      vt->destroy_display = destroy_display;
      vt->set_current_display = set_current_display;
      vt->flip_display = flip_display;
      vt->resize_display = resize_display_fs;
      vt->create_bitmap = _al_ogl_create_bitmap;
      vt->set_target_bitmap = _al_ogl_set_target_bitmap;
      vt->show_mouse_cursor = show_cursor;
      vt->hide_mouse_cursor = hide_cursor;
      vt->create_mouse_cursor = osx_create_mouse_cursor;
      vt->destroy_mouse_cursor = osx_destroy_mouse_cursor;
      vt->set_mouse_cursor = osx_set_mouse_cursor;
      vt->set_system_mouse_cursor = osx_set_system_mouse_cursor;
      vt->get_backbuffer = _al_ogl_get_backbuffer;
      vt->is_compatible_bitmap = is_compatible_bitmap;
      vt->create_sub_bitmap = _al_ogl_create_sub_bitmap;
      _al_ogl_add_drawing_functions(vt);
   }
	return vt;
}  
 
/* Mini VT just for creating displays */
ALLEGRO_DISPLAY_INTERFACE* _al_osx_get_display_driver(void)
{
	static ALLEGRO_DISPLAY_INTERFACE* vt = NULL;
   if (vt == NULL) {
      vt = _AL_MALLOC(sizeof(*vt));
      memset(vt, 0, sizeof(ALLEGRO_DISPLAY_INTERFACE));
      vt->create_display = create_display;
   }
	return vt;
}   

