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


#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/platform/aintosx.h"
#include "./osxgl.h"
#include "allegro5/allegro_osx.h"
#ifndef ALLEGRO_MACOSX
#error something is wrong with the makefile
#endif

#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>

ALLEGRO_DEBUG_CHANNEL("MacOSX")

/* This constant isn't available on OS X < 10.7, define
 * it here so the library can be built on < 10.7 (10.6
 * tested so far.)
 */
#if (MAC_OS_X_VERSION_MAX_ALLOWED < 1070 && !defined(NSWindowCollectionBehaviorFullScreenPrimary))
enum {
   NSWindowCollectionBehaviorFullScreenPrimary = (1 << 7)
};
#endif

/* Defines */
#define MINIMUM_WIDTH 48
#define MINIMUM_HEIGHT 48

/* Unsigned integer; data type only avaliable for OS X >= 10.5 */
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1050
typedef unsigned int NSUInteger;
#endif

/* Module Variables */
static BOOL _osx_mouse_installed = NO, _osx_keyboard_installed = NO;
static NSPoint last_window_pos;
static unsigned int next_display_group = 1;

/* New window locations are set in threat-local storage by the user with
 * al_set_new_window_position, however, our window creation routine in
 * initialiseDisplay (in ALDisplayHelper) calls al_get_new_window_position
 * from a different threat (the main threat). The "proper" fix for this
 * situation is to call al_get_new_window position() before invoking
 * initialiseDisplay and passing these values as parameters. It's a little
 * tedious to do, however, so we use two global variables instead. This can
 * lead to a race condition if two windows are created simultaneously from
 * different threats. Unlikely, but it's possible.
 * FIXME: do this the proper way!
 */
static int new_window_pos_x;
static int new_window_pos_y;

static int new_display_adapter;

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
   //{ ALLEGRO_RENDER_METHOD,      NSOpenGLPFAAccelerated, 0}, handled separately
   { ALLEGRO_FLOAT_COLOR,        NSOpenGLPFAColorFloat, 0},
   { ALLEGRO_FLOAT_DEPTH,        0, 0},
   { ALLEGRO_SINGLE_BUFFER ,     0, 0},   // Only have inverse of this
   { ALLEGRO_SWAP_METHOD,        0, 0},
   { ALLEGRO_COMPATIBLE_DISPLAY, 0, 0},
   { ALLEGRO_DISPLAY_OPTIONS_COUNT, 0, 0}
};
static const int number_of_settings =
   sizeof(allegro_to_osx_settings)/sizeof(*allegro_to_osx_settings);

static const char *allegro_pixel_format_names[] = {
   "ALLEGRO_RED_SIZE",
   "ALLEGRO_GREEN_SIZE",
   "ALLEGRO_BLUE_SIZE",
   "ALLEGRO_ALPHA_SIZE",
   "ALLEGRO_RED_SHIFT",
   "ALLEGRO_GREEN_SHIFT",
   "ALLEGRO_BLUE_SHIFT",
   "ALLEGRO_ALPHA_SHIFT",
   "ALLEGRO_ACC_RED_SIZE",
   "ALLEGRO_ACC_GREEN_SIZE",
   "ALLEGRO_ACC_BLUE_SIZE",
   "ALLEGRO_ACC_ALPHA_SIZE",
   "ALLEGRO_STEREO",
   "ALLEGRO_AUX_BUFFERS",
   "ALLEGRO_COLOR_SIZE",
   "ALLEGRO_DEPTH_SIZE",
   "ALLEGRO_STENCIL_SIZE",
   "ALLEGRO_SAMPLE_BUFFERS",
   "ALLEGRO_SAMPLES",
   "ALLEGRO_RENDER_METHOD",
   "ALLEGRO_FLOAT_COLOR",
   "ALLEGRO_FLOAT_DEPTH",
   "ALLEGRO_SINGLE_BUFFER",
   "ALLEGRO_SWAP_METHOD",
   "ALLEGRO_COMPATIBLE_DISPLAY",
   "ALLEGRO_DISPLAY_OPTIONS_COUNT"
};

/* Module functions */
static NSView* osx_view_from_display(ALLEGRO_DISPLAY* disp);
ALLEGRO_DISPLAY_INTERFACE* _al_osx_get_display_driver(void);
ALLEGRO_DISPLAY_INTERFACE* _al_osx_get_display_driver_win(void);
ALLEGRO_DISPLAY_INTERFACE* _al_osx_get_display_driver_fs(void);
static NSOpenGLContext* osx_create_shareable_context(NSOpenGLPixelFormat* fmt, unsigned int* group);
static bool set_display_flag(ALLEGRO_DISPLAY *display, int flag, bool onoff);

static NSTrackingArea *create_tracking_area(NSView *view)
{
   NSTrackingAreaOptions options =
      NSTrackingMouseEnteredAndExited |
      NSTrackingMouseMoved |
      NSTrackingCursorUpdate |
      NSTrackingActiveAlways;
   return [[NSTrackingArea alloc] initWithRect:[view bounds] options:options owner:view userInfo:nil];
}

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
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
@interface ALOpenGLView : NSOpenGLView <NSWindowDelegate>
#else
@interface ALOpenGLView : NSOpenGLView
#endif
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
-(void) enterFullScreenWindowMode;
-(void) exitFullScreenWindowMode;
-(void) finishExitingFullScreenWindowMode;
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

/* ALSetWindowFrame:
 * Because we create the window frame on the main thread, we should change
 * it on the main thread as well, otherwise we will get a warning similar
 * to "-[NSLock unlock]: lock  unlocked from thread which did not lock it"
 * everytime we change the window frame, which happens when we resize or
 * move the window.
 */
@interface ALSetWindowFrame : NSObject
+(void) set_frame : (NSValue *)param;

@end
@implementation ALSetWindowFrame
+(void) set_frame : (NSValue *) param {
   NSArray *array = [param pointerValue];
   NSRect *rc = [[array objectAtIndex:0] pointerValue];
   NSWindow *win = [[array objectAtIndex:1] pointerValue];

   // Couldn't resist animating it!
   [win setFrame:*rc display:YES animate:YES];
}
@end

/* _al_osx_mouse_was_installed:
 * Called by the mouse driver when the driver is installed or uninstalled.
 * Set the variable so we can decide to pass events or not, and notify all
 * existing displays that they need to set up their tracking areas.
 */
void _al_osx_mouse_was_installed(BOOL install) {
   unsigned int i;
   if (_osx_mouse_installed == install) {
      // done it already
      return;
   }
   _osx_mouse_installed = install;
   _AL_VECTOR* dpys = &al_get_system_driver()->displays;
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
-(void) setAllegroDisplay: (ALLEGRO_DISPLAY*) ptr
{
   dpy_ptr = ptr;
}

/* display 
* return the display this view is associated with
*/
-(ALLEGRO_DISPLAY*) allegroDisplay
{
   return dpy_ptr;
}

/* reshape
* Called when the view changes size */
- (void) reshape 
{
   if ([NSOpenGLContext currentContext] != nil) {
      ALLEGRO_DISPLAY_OSX_WIN* dpy =  (ALLEGRO_DISPLAY_OSX_WIN*) dpy_ptr;
      NSRect rc = [self bounds];
      glViewport(0, 0, NSWidth(rc), NSHeight(rc));
      
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(0, NSWidth(rc), NSHeight(rc), 0, -1, 1);
      
      if (dpy->tracking) {
         [self removeTrackingArea: dpy->tracking];
         dpy->tracking = create_tracking_area(self);
	 [self addTrackingArea: dpy->tracking];
      }
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
   dpy->tracking = create_tracking_area(self);
   [self addTrackingArea: dpy->tracking];
}

- (void) viewWillMoveToWindow: (NSWindow*) newWindow {
   ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) dpy_ptr;
   (void)newWindow;
   if (([self window] != nil) && (dpy->tracking != 0)) {
      [self removeTrackingArea:dpy->tracking];
      dpy->tracking = 0;
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
      event.display.timestamp = al_get_time();
      event.display.width = NSWidth(content);
      event.display.height = NSHeight(content);
      _al_event_source_emit_event(es, &event);
      ALLEGRO_INFO("Window finished resizing");
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
      event.display.timestamp = al_get_time();
      event.display.width = NSWidth(content);
      event.display.height = NSHeight(content);
      _al_event_source_emit_event(es, &event);
      ALLEGRO_INFO("Window was resized\n");
   }
   _al_event_source_unlock(es);
}

-(void) enterFullScreenWindowMode
{
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
   ALLEGRO_DISPLAY_OSX_WIN *dpy =  (ALLEGRO_DISPLAY_OSX_WIN*) dpy_ptr;
   NSMutableDictionary *dict = [[NSMutableDictionary alloc] init];
   int flags = NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar;
   [dict setObject:[NSNumber numberWithInt: flags] forKey:NSFullScreenModeApplicationPresentationOptions];
   [[dpy->win contentView] enterFullScreenMode: [dpy->win screen] withOptions: dict];
   [dict release];
#endif
}

-(void) exitFullScreenWindowMode
{
   ALLEGRO_DISPLAY_OSX_WIN *dpy =  (ALLEGRO_DISPLAY_OSX_WIN*) dpy_ptr;

   /* Going from a fullscreen window to a smaller window, the mouse may end up outside
    * the window when it was inside before (not possible the other way.) This causes a
    * crash. To avoid it, remove the tracking area and add it back after exiting fullscreen.
    * (my theory)
    */
   [dpy->win orderOut:[dpy->win contentView]];
   [self exitFullScreenModeWithOptions: nil];
}

-(void) finishExitingFullScreenWindowMode
{
   ALLEGRO_DISPLAY_OSX_WIN *dpy =  (ALLEGRO_DISPLAY_OSX_WIN*) dpy_ptr;

   [dpy->win center];
   [dpy->win makeKeyAndOrderFront:[dpy->win contentView]];
   [[self window] makeFirstResponder: self];
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
   if (!dpy->depth) {   /* Use default */
      NSScreen *screen;
      int adapter = al_get_new_display_adapter();
      if ((adapter >= 0) && (adapter < al_get_num_video_adapters())) {
         screen = [[NSScreen screens] objectAtIndex: adapter];
      } else {
         screen = [NSScreen mainScreen];
      }
      dpy->depth = NSBitsPerPixelFromDepth([screen depth]);
      if (dpy->depth == 24)
         dpy->depth = 32;
      if (dpy->depth == 15)
         dpy->depth = 16;
      ALLEGRO_DEBUG("Using default colour depth %d\n", dpy->depth);
   }
   *a = NSOpenGLPFAColorSize; a++;
   *a = dpy->depth; a++; 

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
               ALLEGRO_DEBUG("Passing pixel format attribute %s = %d\n", allegro_pixel_format_names[n], extras->settings[i]);
            }
         } else if (extras->settings[i]) {      /* Boolean, just turn this on */
            *a = allegro_to_osx_settings[n][1]; a++;
            ALLEGRO_DEBUG("Passing pixel format attribute %s = %d\n", allegro_pixel_format_names[n], extras->settings[i]);
         }
      }
   }

   /* Accelerated is always preferred, so we only set this for required not
    * for suggested.
    */
   if (extras->required & ALLEGRO_RENDER_METHOD) {
      *a++ = NSOpenGLPFAAccelerated;
   }
}


/* Set the extra_settings[] array in the display, to report which options
 * we have selected.
 */
static void osx_get_opengl_pixelformat_attributes(ALLEGRO_DISPLAY_OSX_WIN *dpy)
{
   int n;

   if (!dpy)
      return;

   /* Clear list of settings (none selected) */
   memset(&dpy->parent.extra_settings, 0, sizeof(dpy->parent.extra_settings));

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1050
   /* Get the pixel format associated with the OpenGL context.
    * We use the Carbon API rather than the Cocoa API because that way we
    * can use the same code in Windowed mode as in fullscreen mode (we
    * don't have an NSOpenGLView in fullscreen mode).
    */
   CGLContextObj ctx = [dpy->ctx CGLContextObj];
   CGLPixelFormatObj pixel_format = CGLGetPixelFormat(ctx);
   GLint screen_id;
   CGLGetVirtualScreen(ctx, &screen_id);
   ALLEGRO_DEBUG("Screen has ID %d\n", (int)screen_id);
   for (n = 0; n < number_of_settings; n++) {
      /* Go through the list of options and relist the ones that we have
       * set to Allegro's option list.
       */

      CGLPixelFormatAttribute attrib = allegro_to_osx_settings[n][1];
      /* Skip options that don't exist on OS X */
      if (attrib == 0)
         continue;

      /* Get value for this attribute */
      GLint value;
      CGLDescribePixelFormat(pixel_format, screen_id, attrib, &value);

      int al_setting = allegro_to_osx_settings[n][0];
      if (allegro_to_osx_settings[n][2] == 0)      /* Boolean attribute */
         value = 1;

      dpy->parent.extra_settings.settings[al_setting] = value;
      ALLEGRO_DEBUG("Pixel format attribute %s set to %d\n", allegro_pixel_format_names[n], value);
   }
#else
   /* CGLGetPixelFormat does not exist on Tiger, so we need to do something
    * else.
    * FIXME: right now, we just return the settings that were chosen by the
    * user. To be correct, we should query the pixelformat corresponding to
    * the display, using the NSOpenGLView pixelFormat method and the
    * NSOpenGLPixelFormat getValues:forAttribute:forVirtualScreen: method,
    * for which we need to know the logical screen number that the display
    * is on. That doesn't work for fullscreen modes though...
    */
   NSOpenGLPixelFormatAttribute *attributes = dpy->attributes;

   for (n = 0; n < number_of_settings; n++) {
      /* Go through the list of options and relist the ones that we have
       * set to Allegro's option list.
       */
      NSOpenGLPixelFormatAttribute *a = dpy->attributes;
      while (*a) {
         if (*a == allegro_to_osx_settings[n][1]) {
            int al_setting = allegro_to_osx_settings[n][0];
            int value = 1;
            if (allegro_to_osx_settings[n][2])
               value = a[1];
            dpy->parent.extra_settings.settings[al_setting] = value;
            ALLEGRO_DEBUG("Setting pixel format attribute %d to %d\n", al_setting, value);
         }
         /* Advance to next option */
         if (allegro_to_osx_settings[n][2]) /* Has a parameter in the list */
            a++;
         a++;
      }
   }
#endif
   dpy->parent.extra_settings.settings[ALLEGRO_COMPATIBLE_DISPLAY] = 1;

   // Fill in the missing colour format options, as best we can
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds = &dpy->parent.extra_settings;
   if (eds->settings[ALLEGRO_COLOR_SIZE] == 0) {
      eds->settings[ALLEGRO_COLOR_SIZE] = 32;
      eds->settings[ALLEGRO_RED_SIZE] = 8;
      eds->settings[ALLEGRO_GREEN_SIZE] = 8;
      eds->settings[ALLEGRO_BLUE_SIZE] = 8;
      eds->settings[ALLEGRO_ALPHA_SIZE] = 8;
      eds->settings[ALLEGRO_RED_SHIFT] = 0;
      eds->settings[ALLEGRO_GREEN_SHIFT] = 8;
      eds->settings[ALLEGRO_BLUE_SHIFT] = 16;
      eds->settings[ALLEGRO_ALPHA_SHIFT] = 24;
   } else {
      int size = eds->settings[ALLEGRO_ALPHA_SIZE];
      if (!size) {
         switch (eds->settings[ALLEGRO_COLOR_SIZE]) {
            case 32:
               size = 8;
               break;

            case 16:
               size = 5;
               break;

            case 8:
               size = 8;
               break;
         }
      }
      if (!eds->settings[ALLEGRO_RED_SIZE])
         eds->settings[ALLEGRO_RED_SIZE] = size;
      if (!eds->settings[ALLEGRO_BLUE_SIZE])
         eds->settings[ALLEGRO_BLUE_SIZE] = size;
      if (!eds->settings[ALLEGRO_GREEN_SIZE])
         eds->settings[ALLEGRO_GREEN_SIZE] = size;
      if (!eds->settings[ALLEGRO_RED_SHIFT])
         eds->settings[ALLEGRO_RED_SHIFT] = 0;
      if (!eds->settings[ALLEGRO_GREEN_SHIFT])
         eds->settings[ALLEGRO_GREEN_SHIFT] = eds->settings[ALLEGRO_RED_SIZE];
      if (!eds->settings[ALLEGRO_BLUE_SHIFT])
         eds->settings[ALLEGRO_BLUE_SHIFT] = eds->settings[ALLEGRO_GREEN_SIZE]+eds->settings[ALLEGRO_GREEN_SHIFT];
      if (!eds->settings[ALLEGRO_ALPHA_SHIFT])
         eds->settings[ALLEGRO_ALPHA_SHIFT] = eds->settings[ALLEGRO_BLUE_SIZE]+eds->settings[ALLEGRO_BLUE_SHIFT];
   }
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
   NSScreen *screen;
   unsigned int mask = (dpy->parent.flags & ALLEGRO_FRAMELESS) ? NSBorderlessWindowMask :
      (NSTitledWindowMask|NSClosableWindowMask|NSMiniaturizableWindowMask);
   if (dpy->parent.flags & ALLEGRO_RESIZABLE)
      mask |= NSResizableWindowMask;
   if (dpy->parent.flags & ALLEGRO_FULLSCREEN)
      mask |= NSResizableWindowMask;
  
   if ((new_display_adapter >= 0) && (new_display_adapter < al_get_num_video_adapters())) {
      screen = [[NSScreen screens] objectAtIndex: new_display_adapter];
   } else {
      screen = [NSScreen mainScreen];
   }
   [win initWithContentRect: rc
               styleMask: mask
                backing: NSBackingStoreBuffered
                  defer: NO
                 screen: screen];
   if (dpy->parent.flags & ALLEGRO_RESIZABLE) {
      if ([win respondsToSelector:NSSelectorFromString(@"setCollectionBehavior:")]) {
         [win setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
      }
   }
   NSOpenGLPixelFormat* fmt =
      [[NSOpenGLPixelFormat alloc] initWithAttributes: dpy->attributes];
   ALOpenGLView* view = [[ALOpenGLView alloc] initWithFrame: rc];
   dpy->ctx = osx_create_shareable_context(fmt, &dpy->display_group);
   if (dpy->ctx == nil) {
      ALLEGRO_DEBUG("Could not create rendering context\n");
	  [view release];
	  [fmt release];
	  
      return;
   }
   [view setOpenGLContext: dpy->ctx];
   [view setPixelFormat: fmt];
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

   if ((new_window_pos_x != INT_MAX) && (new_window_pos_y != INT_MAX)) {
      /* The user gave us window coordinates */
      NSRect rc = [win frame];
      NSRect sc = [[win screen] frame];
      NSPoint origin;

      /* We need to modify the y coordinate, cf. set_window_position */
      origin.x = sc.origin.x + new_window_pos_x;
      origin.y = sc.origin.y + sc.size.height - rc.size.height - new_window_pos_y;
      [win setFrameOrigin: origin];
   }
   else {
      /* Center the window */
      NSRect rc = [win frame];
      NSRect sc = [[win screen] frame];
      NSPoint origin;

      origin.x = sc.origin.x + sc.size.width/2 - rc.size.width/2;
      origin.y = sc.origin.y + sc.size.height/2 - rc.size.height/2;
      [win setFrameOrigin: origin];
   }
   [win makeKeyAndOrderFront:self];
   if (!(mask & NSBorderlessWindowMask)) [win makeMainWindow];
   [fmt release];
   [view release];
}
+(void) destroyDisplay: (NSValue*) display_object {
   ALLEGRO_DISPLAY_OSX_WIN* dpy = [display_object pointerValue];
   ALLEGRO_OGL_EXTRAS *ogl = ((ALLEGRO_DISPLAY *)dpy)->ogl_extras;
   _al_vector_find_and_delete(&al_get_system_driver()->displays, &dpy);
   if (ogl->backbuffer) {
      _al_ogl_destroy_backbuffer(ogl->backbuffer);
      ogl->backbuffer = NULL;
      ALLEGRO_DEBUG("destroy backbuffer.\n");
   }
   // Disconnect from its view or exit fullscreen mode
   [dpy->ctx clearDrawable];
   // Unlock the screen 
   if (dpy->parent.flags & ALLEGRO_FULLSCREEN) {
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1050
      if (dpy->win) {
         [[dpy->win contentView] exitFullScreenModeWithOptions: nil];
      }
#endif
      CGDisplayRelease(dpy->display_id);
      dpy->in_fullscreen = false;
   }
   if (dpy->win) {
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
   ALLEGRO_DISPLAY_OSX_WIN *dpy = (ALLEGRO_DISPLAY_OSX_WIN *)display;
   while (dpy->in_fullscreen) {
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
   _AL_VECTOR* dpys = &al_get_system_driver()->displays;
   unsigned int i;
   NSOpenGLContext* compat = nil;
   
   for (i = 0; i < _al_vector_size(dpys); ++i) {
      ALLEGRO_DISPLAY_OSX_WIN* other = *(ALLEGRO_DISPLAY_OSX_WIN**) _al_vector_ref(dpys, i);
      compat = [[NSOpenGLContext alloc] initWithFormat:fmt shareContext: other->ctx];
      if (compat != nil) {
      // OK, we can share with this one
         *group = other->display_group;
         ALLEGRO_DEBUG("Sharing display group %d\n", *group);
         break;
      }
   }
   if (compat == nil) {
      // Set to a new group
      *group = next_display_group++;
      ALLEGRO_DEBUG("Creating new display group %d\n", *group);
      compat = [[NSOpenGLContext alloc] initWithFormat:fmt shareContext: nil];      
   }
   return compat;
}

/* create_display_fs:
 * Create a fullscreen display - capture the display
 */
static ALLEGRO_DISPLAY* create_display_fs(int w, int h)
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   ALLEGRO_DEBUG("Switching to fullscreen mode sized %dx%d\n", w, h);
   if (al_get_new_display_adapter() >= al_get_num_video_adapters()) {
      [pool drain];
      return NULL;
   }
   ALLEGRO_DISPLAY_OSX_WIN* dpy = al_malloc(sizeof(ALLEGRO_DISPLAY_OSX_WIN));
   if (dpy == NULL) {
      [pool drain];
      return NULL;
   }
   memset(dpy, 0, sizeof(*dpy));

   /* Set up the ALLEGRO_DISPLAY part */
   dpy->parent.vt = _al_osx_get_display_driver_fs();
   dpy->parent.refresh_rate = al_get_new_display_refresh_rate();
   dpy->parent.flags = al_get_new_display_flags() | ALLEGRO_OPENGL | ALLEGRO_FULLSCREEN;
   dpy->parent.w = w;
   dpy->parent.h = h;
   _al_event_source_init(&dpy->parent.es);
   dpy->cursor = [[NSCursor arrowCursor] retain];
   dpy->display_id = CGMainDisplayID();

   /* Get display ID for the requested display */
   if (al_get_new_display_adapter() > 0) {
      int adapter = al_get_new_display_adapter();
      NSScreen *screen = [[NSScreen screens] objectAtIndex: adapter];
      NSDictionary *dict = [screen deviceDescription];
      NSNumber *display_id = [dict valueForKey: @"NSScreenNumber"];
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1050
      dpy->display_id = [display_id intValue];
#else
      dpy->display_id = [display_id integerValue];
#endif
      //dpy->display_id = (CGDirectDisplayID)[display_id pointerValue];
   }

   // Set up a pixel format to describe the mode we want.
   osx_set_opengl_pixelformat_attributes(dpy);
   NSOpenGLPixelFormat* fmt =
      [[NSOpenGLPixelFormat alloc] initWithAttributes: dpy->attributes];
   if (fmt == nil) {
      ALLEGRO_DEBUG("Could not set pixel format\n");
      [pool drain];
      return NULL;
   }

   // Create a context which shares with any other contexts with the same format
   NSOpenGLContext* context = osx_create_shareable_context(fmt, &dpy->display_group);
   [fmt release];
   if (context == nil) {
      ALLEGRO_DEBUG("Could not create rendering context\n");
      [pool drain];
      return NULL;
   }
   [context makeCurrentContext];
   dpy->ctx = context;

   // Prevent other apps from writing to this display and switch it to our
   // chosen mode.
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
   CGDisplayCapture(dpy->display_id);
   CFDictionaryRef mode = CGDisplayBestModeForParametersAndRefreshRate(dpy->display_id, dpy->depth, w, h, dpy->parent.refresh_rate, NULL);
   CGDisplaySwitchToMode(dpy->display_id, mode);
#else
   CGDisplayModeRef mode = NULL;
   CFArrayRef modes = NULL;
   CFStringRef pixel_format = NULL;
   int i;

   /* Set pixel format string */
   if (dpy->depth == 32)
      pixel_format = CFSTR(IO32BitDirectPixels);
   if (dpy->depth == 16)
      pixel_format = CFSTR(IO16BitDirectPixels);
   if (dpy->depth == 8)
      pixel_format = CFSTR(IO8BitIndexedPixels);
   modes = CGDisplayCopyAllDisplayModes(dpy->display_id, NULL);
   for (i = 0; i < CFArrayGetCount(modes); i++) {
      CGDisplayModeRef try_mode =
         (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);

      CFStringRef pixel_encoding = CGDisplayModeCopyPixelEncoding(try_mode);

      /* Found mode with matching size/colour depth */
      if ( w == (int)CGDisplayModeGetWidth(try_mode) &&
           h == (int)CGDisplayModeGetHeight(try_mode) &&
           CFStringCompare(pixel_encoding, pixel_format, 1) == 0) {
         mode = try_mode;
         CFRelease(pixel_encoding);
         break;
      }

      /* Found mode with matching size; colour depth does not match, but
       * it's the best so far.
       */
      if ( w == (int)CGDisplayModeGetWidth(try_mode) &&
           h == (int)CGDisplayModeGetHeight(try_mode) &&
           mode == NULL) {
         mode = try_mode;
      }

      CFRelease(pixel_encoding);
   }
   CFRelease(modes);

   if (!mode) {
      /* Trouble! - we can't find a nice mode to set. */
      [dpy->ctx clearDrawable];
      CGDisplayRelease(dpy->display_id);
      al_free(dpy);
      [pool drain];
      return NULL;
   }

   /* Switch display mode */
   CFDictionaryRef options = CFDictionaryCreate(NULL, NULL, NULL, 0, NULL, NULL);
   CGDisplaySetDisplayMode(dpy->display_id, mode, NULL);
   CFRelease(options);
#endif
   [context setFullScreen];

   // Set up the Allegro OpenGL implementation
   dpy->parent.ogl_extras = al_malloc(sizeof(ALLEGRO_OGL_EXTRAS));
   memset(dpy->parent.ogl_extras, 0, sizeof(ALLEGRO_OGL_EXTRAS));
   _al_ogl_manage_extensions(&dpy->parent);
   _al_ogl_set_extensions(dpy->parent.ogl_extras->extension_api);
   dpy->parent.ogl_extras->is_shared = true;

   /* Retrieve the options that were set */
   osx_get_opengl_pixelformat_attributes(dpy);
   dpy->parent.ogl_extras->backbuffer = _al_ogl_create_backbuffer(&dpy->parent);
   /* Turn on vsyncing possibly */
   if (_al_get_new_display_settings()->settings[ALLEGRO_VSYNC] == 1) {
      GLint swapInterval = 1;
      [context setValues:&swapInterval forParameter: NSOpenGLCPSwapInterval];
   }
   else {
      GLint swapInterval = 0;
      [context setValues:&swapInterval forParameter: NSOpenGLCPSwapInterval];
   }

   /* Set up GL as we want */
   setup_gl(&dpy->parent);

   /* Clear and flush (for double buffering) */
   glClear(GL_COLOR_BUFFER_BIT);
   [context flushBuffer];
   glClear(GL_COLOR_BUFFER_BIT);

   /* Add to the display list */
   ALLEGRO_DISPLAY **add = _al_vector_alloc_back(&al_get_system_driver()->displays);
   *add = &dpy->parent;
   dpy->in_fullscreen = YES;
   // Begin the 'private' event loop 
   // Necessary because there's no NSResponder (i.e. a view) to collect 
   // events from the window server.
   [ALDisplayHelper performSelectorOnMainThread: @selector(runFullScreenDisplay:) 
                                     withObject: [NSValue valueWithPointer:dpy] 
                                  waitUntilDone: NO];

   [pool drain];
   return &dpy->parent;
}
#if 0
/* Alternative, Cocoa-based mode switching.
 * Works, but I can't get the display to respond to a resize request
 * properly - EG
 */
static ALLEGRO_DISPLAY* create_display_fs(int w, int h)
{
   ALLEGRO_DEBUG("Creating full screen mode sized %dx%d\n", w, h);
   if (al_get_new_display_adapter() >= al_get_num_video_adapters())
      return NULL;
   ALLEGRO_DISPLAY_OSX_WIN* dpy = al_malloc(sizeof(ALLEGRO_DISPLAY_OSX_WIN));
   if (dpy == NULL) {
      return NULL;
   }
   memset(dpy, 0, sizeof(*dpy));

   /* Set up the ALLEGRO_DISPLAY part */
   dpy->parent.vt = _al_osx_get_display_driver_win();
   dpy->parent.refresh_rate = al_get_new_display_refresh_rate();
   dpy->parent.flags = al_get_new_display_flags() | ALLEGRO_OPENGL | ALLEGRO_FULLSCREEN;
   dpy->parent.w = w;
   dpy->parent.h = h;
   _al_event_source_init(&dpy->parent.es);
   osx_change_cursor(dpy, [NSCursor arrowCursor]);
   dpy->show_cursor = YES;
   
   // Set up a pixel format to describe the mode we want.
   osx_set_opengl_pixelformat_attributes(dpy);

   // Clear last window position to 0 if there are no other open windows
   if (_al_vector_is_empty(&al_get_system_driver()->displays)) {
      last_window_pos = NSZeroPoint;
   }

   /* OSX specific part - finish the initialisation on the main thread */
   [ALDisplayHelper performSelectorOnMainThread: @selector(initialiseDisplay:) 
      withObject: [NSValue valueWithPointer:dpy] 
      waitUntilDone: YES];

   [dpy->ctx makeCurrentContext];
   [dpy->ctx setFullScreen];

   NSScreen *screen = [dpy->win screen];
   NSDictionary *dict = [screen deviceDescription];
   NSNumber *display_id = [dict valueForKey: @"NSScreenNumber"];
   dpy->display_id = [display_id integerValue];
   //dpy->display_id = (CGDirectDisplayID)[display_id pointerValue];
   CGDisplayCapture(dpy->display_id);

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
   CFDictionaryRef mode = CGDisplayBestModeForParametersAndRefreshRate(dpy->display_id, dpy->depth, w, h, dpy->parent.refresh_rate, NULL);
   CGDisplaySwitchToMode(dpy->display_id, mode);
#else
   CGDisplayModeRef mode = NULL;
   CFArrayRef modes = NULL;
   CFStringRef pixel_format = NULL;
   int i;

   /* Set pixel format string */
   if (dpy->depth == 32)
      pixel_format = CFSTR(IO32BitDirectPixels);
   if (dpy->depth == 16)
      pixel_format = CFSTR(IO16BitDirectPixels);
   if (dpy->depth == 8)
      pixel_format = CFSTR(IO8BitIndexedPixels);
   modes = CGDisplayCopyAllDisplayModes(dpy->display_id, NULL);
   for (i = 0; i < CFArrayGetCount(modes); i++) {
      CGDisplayModeRef try_mode =
         (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);

      CFStringRef pixel_encoding = CGDisplayModeCopyPixelEncoding(try_mode);

      /* Found mode with matching size/colour depth */
      if ( w == (int)CGDisplayModeGetWidth(try_mode) &&
           h == (int)CGDisplayModeGetHeight(try_mode) &&
           CFStringCompare(pixel_encoding, pixel_format, 1) == 0) {
         mode = try_mode;
         CFRelease(pixel_encoding);
         break;
      }

      /* Found mode with matching size; colour depth does not match, but
       * it's the best so far.
       */
      if ( w == (int)CGDisplayModeGetWidth(try_mode) &&
           h == (int)CGDisplayModeGetHeight(try_mode) &&
           mode == NULL) {
         mode = try_mode;
      }

      CFRelease(pixel_encoding);
   }

   if (!mode) {
      /* Trouble! - we can't find a nice mode to set. */
      [dpy->ctx clearDrawable];
      CGDisplayRelease(dpy->display_id);
      [dpy->win close];
      al_free(dpy);
      return NULL;
   }

   /* Switch display mode */
   CFDictionaryRef options = CFDictionaryCreate(NULL, NULL, NULL, 0, NULL, NULL);
   CGDisplaySetDisplayMode(dpy->display_id, mode, NULL);
   CFRelease(options);
   CFRelease(modes);
#endif

   [[dpy->win contentView] enterFullScreenMode: screen withOptions: nil];

   // Set up the Allegro OpenGL implementation
   dpy->parent.ogl_extras = al_malloc(sizeof(ALLEGRO_OGL_EXTRAS));
   memset(dpy->parent.ogl_extras, 0, sizeof(ALLEGRO_OGL_EXTRAS));
   _al_ogl_manage_extensions(&dpy->parent);
   _al_ogl_set_extensions(dpy->parent.ogl_extras->extension_api);
   dpy->parent.ogl_extras->is_shared = true;

   /* Retrieve the options that were set */
   osx_get_opengl_pixelformat_attributes(dpy);
   dpy->parent.ogl_extras->backbuffer = _al_ogl_create_backbuffer(&dpy->parent);
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

   /* Add to the display list */
   ALLEGRO_DISPLAY **add = _al_vector_alloc_back(&al_get_system_driver()->displays);
   *add = &dpy->parent;
   dpy->in_fullscreen = YES;

   return &dpy->parent;
}
#endif

/* create_display_win:
* Create a windowed display - create the window with an ALOpenGLView
* to be its content view
*/
static ALLEGRO_DISPLAY* create_display_win(int w, int h) {
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   /* Create a temporary view so that we can check whether a fullscreen
    * window can be created.
    */
   if (al_get_new_display_flags() & ALLEGRO_FULLSCREEN_WINDOW) {
      NSRect rc = NSMakeRect(0, 0, w,  h);
      ALOpenGLView* view = [[ALOpenGLView alloc] initWithFrame: rc];
      if (![view respondsToSelector:
                  @selector(enterFullScreenMode:withOptions:)]) {
         ALLEGRO_DEBUG("Cannot create FULLSCREEN_WINDOW");
         [view release];
         [pool drain];
         return NULL;
      }
      [view release];
   }

   ALLEGRO_DEBUG("Creating window sized %dx%d\n", w, h);
   if (al_get_new_display_adapter() >= al_get_num_video_adapters()) {
      [pool drain];
      return NULL;
   }
   ALLEGRO_DISPLAY_OSX_WIN* dpy = al_malloc(sizeof(ALLEGRO_DISPLAY_OSX_WIN));
   if (dpy == NULL) {
      [pool drain];
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
   
   // Set up a pixel format to describe the mode we want.
   osx_set_opengl_pixelformat_attributes(dpy);

   // Clear last window position to 0 if there are no other open windows
   if (_al_vector_is_empty(&al_get_system_driver()->displays)) {
      last_window_pos = NSZeroPoint;
   }

   /* Get the new window position. This is stored in TLS, so we need to do
    * this before calling initialiseDisplay, which runs on a different
    * threat.
    */
   al_get_new_window_position(&new_window_pos_x, &new_window_pos_y);
   
   new_display_adapter = al_get_new_display_adapter();

   /* OSX specific part - finish the initialisation on the main thread */
   [ALDisplayHelper performSelectorOnMainThread: @selector(initialiseDisplay:) 
      withObject: [NSValue valueWithPointer:dpy] 
      waitUntilDone: YES];
   
   if (dpy->parent.flags & ALLEGRO_FULLSCREEN_WINDOW) {
      NSRect sc = [[dpy->win screen] frame];
      dpy->parent.w = sc.size.width;
      dpy->parent.h = sc.size.height;
   }

   [dpy->ctx makeCurrentContext];

   /* Print out OpenGL version info */
   ALLEGRO_INFO("OpenGL Version: %s\n", glGetString(GL_VERSION));
   ALLEGRO_INFO("Vendor: %s\n", glGetString(GL_VENDOR));
   ALLEGRO_INFO("Renderer: %s\n", glGetString(GL_RENDERER));
   
   /* Set up a pixel format to describe the mode we want. */
   osx_set_opengl_pixelformat_attributes(dpy);
   /* Retrieve the options that were set */
   // Note: This initializes dpy->extra_settings
   osx_get_opengl_pixelformat_attributes(dpy);

   // Set up the Allegro OpenGL implementation
   dpy->parent.ogl_extras = al_malloc(sizeof(ALLEGRO_OGL_EXTRAS));
   memset(dpy->parent.ogl_extras, 0, sizeof(ALLEGRO_OGL_EXTRAS));
   _al_ogl_manage_extensions(&dpy->parent);
   _al_ogl_set_extensions(dpy->parent.ogl_extras->extension_api);
   dpy->parent.ogl_extras->is_shared = true;

   dpy->parent.ogl_extras->backbuffer = _al_ogl_create_backbuffer(&dpy->parent);
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

   /* Add to the display list */
   ALLEGRO_DISPLAY **add = _al_vector_alloc_back(&al_get_system_driver()->displays);
   *add = &dpy->parent;
   dpy->in_fullscreen = NO;

   if (dpy->parent.flags & ALLEGRO_FULLSCREEN_WINDOW) {
      dpy->parent.flags ^= ALLEGRO_FULLSCREEN_WINDOW; /* Not set yet */
      set_display_flag(&dpy->parent, ALLEGRO_FULLSCREEN_WINDOW, true);
   }

   [pool drain];

   return &dpy->parent;
}

/* destroy_display:
 * Destroy display, actually close the window or exit fullscreen on the main thread
 */
static void destroy_display(ALLEGRO_DISPLAY* d)
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   ALLEGRO_DISPLAY *old_dpy = al_get_current_display();
   ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
   ALLEGRO_DISPLAY_OSX_WIN* other = NULL;
   unsigned int i;

   // Set the display as the current display; needed because we need to
   // make the context current.
   if (old_dpy != d)
      _al_set_current_display_only(d);

   /* First of all, save video bitmaps attached to this display. */
   // Check for other displays in this display group
   _AL_VECTOR* dpys = &al_get_system_driver()->displays;
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
   al_free(d->ogl_extras);

   // Restore original display from before this function was called.
   // If the display we just destroyed is actually current, set the current
   // display to NULL.
   if (old_dpy != d)
      _al_set_current_display_only(old_dpy);
   else {
      // Is this redundant? --pw
      _al_set_current_display_only(NULL);
   }

   al_free(d->vertex_cache);
   al_free(d);
   [pool drain];
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

   ALLEGRO_BITMAP *old_target = NULL;
   if (!disp->ogl_extras->opengl_target->is_backbuffer) {
   	old_target = al_get_target_bitmap();
	al_set_target_backbuffer(disp);
   }

   if (disp->extra_settings.settings[ALLEGRO_SINGLE_BUFFER]) {
      glFlush();
   }
   else {
      [dpy->ctx flushBuffer];
   }

   if (old_target) {
      al_set_target_bitmap(old_target);
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

/* _al_osx_create_mouse_cursor:
 * creates a custom system cursor from the bitmap bmp.
 * (x_focus, y_focus) indicates the cursor hot-spot.
 */
ALLEGRO_MOUSE_CURSOR *_al_osx_create_mouse_cursor(ALLEGRO_BITMAP *bmp,
   int x_focus, int y_focus)
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   ALLEGRO_MOUSE_CURSOR_OSX *cursor = NULL;
   
   if (!bmp) {
      [pool drain];
      return NULL;
   }

   NSImage* cursor_image = NSImageFromAllegroBitmap(bmp);
   cursor = al_malloc(sizeof *cursor);
   cursor->cursor = [[NSCursor alloc] initWithImage: cursor_image
                            hotSpot: NSMakePoint(x_focus, y_focus)];
   [cursor_image release];
   [pool drain];

   return (ALLEGRO_MOUSE_CURSOR *)cursor;
}
 
/* _al_osx_destroy_mouse_cursor:
 * destroys a mouse cursor previously created with _al_osx_create_mouse_cursor
 */
void _al_osx_destroy_mouse_cursor(ALLEGRO_MOUSE_CURSOR *curs)
{
   ALLEGRO_MOUSE_CURSOR_OSX *cursor = (ALLEGRO_MOUSE_CURSOR_OSX *) curs;
   unsigned i;

   if (!cursor)
      return;

   /* XXX not at all thread safe */

   _AL_VECTOR* dpys = &al_get_system_driver()->displays;
   for (i = 0; i < _al_vector_size(dpys); ++i) {
      ALLEGRO_DISPLAY* dpy = *(ALLEGRO_DISPLAY**) _al_vector_ref(dpys, i);
      ALLEGRO_DISPLAY_OSX_WIN *osx_dpy = (ALLEGRO_DISPLAY_OSX_WIN*) dpy;

      if (osx_dpy->cursor == cursor->cursor) {
         osx_change_cursor(osx_dpy, [NSCursor arrowCursor]);
      }
   }

   [cursor->cursor release];
   al_free(cursor);
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
 */
static bool osx_set_system_mouse_cursor(ALLEGRO_DISPLAY *display,
   ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id)
{
   ALLEGRO_DISPLAY_OSX_WIN *dpy = (ALLEGRO_DISPLAY_OSX_WIN *)display;
   NSCursor *requested_cursor = NULL;

   switch (cursor_id) {
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_ARROW:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_BUSY:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_QUESTION:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_NE:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_SW:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_NW:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_SE:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_PROGRESS:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_ALT_SELECT:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_UNAVAILABLE:
         requested_cursor = [NSCursor arrowCursor];
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_EDIT:
         requested_cursor = [NSCursor IBeamCursor];
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_N:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_S:
         requested_cursor = [NSCursor resizeUpDownCursor];
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_E:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_W:
         requested_cursor = [NSCursor resizeLeftRightCursor];
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_PRECISION:
         requested_cursor = [NSCursor crosshairCursor];
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_LINK:
         requested_cursor = [NSCursor pointingHandCursor];
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
static bool show_cursor(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
   dpy->show_cursor = YES;
   [NSCursor unhide];
   return true;
}

static bool hide_cursor(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
   dpy->show_cursor = NO;
   [NSCursor hide];
   return true;
}

static bool acknowledge_resize_display_win(ALLEGRO_DISPLAY *d)
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   ALLEGRO_DISPLAY_OSX_WIN *dpy = (ALLEGRO_DISPLAY_OSX_WIN *)d;
   NSWindow* window = dpy->win;
   NSRect frame = [window frame];
   NSRect content = [window contentRectForFrameRect: frame];

   d->w = NSWidth(content);
   d->h = NSHeight(content);

   _al_ogl_resize_backbuffer(d->ogl_extras->backbuffer, d->w, d->h);
   setup_gl(d);

   [pool drain]; 
   return true;
}

/* resize_display_(win|fs)
 * Change the size of the display by altering the window size or changing the screen mode
 */
static bool resize_display_win(ALLEGRO_DISPLAY *d, int w, int h)
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
   NSWindow* window = dpy->win;
   NSRect current = [window frame];
   w = _ALLEGRO_MAX(w, MINIMUM_WIDTH);
   h = _ALLEGRO_MAX(h, MINIMUM_HEIGHT);
   NSRect rc = [window frameRectForContentRect: NSMakeRect(0.0f, 0.0f, (float) w, (float) h)];
   rc.origin = current.origin;

   /* Don't resize a fullscreen window */
   if (d->flags & ALLEGRO_FULLSCREEN_WINDOW) {
      [pool drain];
      return false;
   }


   /* Finalise setting the frame on the main thread. Because this is where
    * the window's frame was initially set, this is where it should be
    * modified too.
    */
   NSArray *param = [NSArray arrayWithObjects :
      [NSValue valueWithPointer:&rc],
      [NSValue valueWithPointer:window], nil];
   [ALSetWindowFrame performSelectorOnMainThread: @selector(set_frame:) 
                                     withObject: [NSValue valueWithPointer:param]
                                  waitUntilDone: YES];

   [pool drain];
   return acknowledge_resize_display_win(d);
}

static bool resize_display_fs(ALLEGRO_DISPLAY *d, int w, int h)
{
   ALLEGRO_DEBUG("Resize full screen display to %d x %d\n", w, h);
   bool success = true;
   ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
   CFDictionaryRef current = CGDisplayCurrentMode(dpy->display_id);
   CFNumberRef bps = (CFNumberRef) CFDictionaryGetValue(current, kCGDisplayBitsPerPixel);
   int b;
   CFNumberGetValue(bps, kCFNumberSInt32Type,&b);
   CFDictionaryRef mode = CGDisplayBestModeForParameters(dpy->display_id, b, w, h, NULL);
   [dpy->ctx clearDrawable];
   CGError err = CGDisplaySwitchToMode(dpy->display_id, mode);
   success = (err == kCGErrorSuccess);
#else
   CGDisplayModeRef current = CGDisplayCopyDisplayMode(dpy->display_id);
   CFStringRef bps = CGDisplayModeCopyPixelEncoding(current);
   CFRelease(current);
   CGDisplayModeRef mode = NULL;
   CFArrayRef modes = NULL;
   int i;

   modes = CGDisplayCopyAllDisplayModes(dpy->display_id, NULL);
   for (i = 0; i < CFArrayGetCount(modes); i++) {
      CGDisplayModeRef try_mode =
         (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);

      CFStringRef pixel_encoding = CGDisplayModeCopyPixelEncoding(try_mode);

      /* Found mode with matching size/colour depth */
      if ( w == (int)CGDisplayModeGetWidth(try_mode) &&
           h == (int)CGDisplayModeGetHeight(try_mode) &&
           CFStringCompare(pixel_encoding, bps, 1) == 0) {
         mode = try_mode;
         CFRelease(pixel_encoding);
         break;
      }

      CFRelease(pixel_encoding);
   }
  CFRelease(bps);
  CFRelease(modes);
   if (!mode) {
      ALLEGRO_DEBUG("Can't resize fullscreen display\n");
 
      return false;
   }

   /* Switch display mode */
   [dpy->ctx clearDrawable];
   CFDictionaryRef options = CFDictionaryCreate(NULL, NULL, NULL, 0, NULL, NULL);
   CGDisplaySetDisplayMode(dpy->display_id, mode, NULL);
   CFRelease(options);
#endif
   d->w = w;
   d->h = h;
   [dpy->ctx setFullScreen];
   _al_ogl_resize_backbuffer(d->ogl_extras->backbuffer, d->w, d->h);
   setup_gl(d);
   return success;
}

static bool is_compatible_bitmap(ALLEGRO_DISPLAY* disp, ALLEGRO_BITMAP* bmp)
{
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
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   ALLEGRO_DISPLAY_OSX_WIN* d = (ALLEGRO_DISPLAY_OSX_WIN*) display;
   NSWindow* window = d->win;
   NSRect rc = [window frame];
   NSRect sc = [[window screen] frame];
   rc.origin.x = (float) x;
   rc.origin.y = sc.size.height - rc.size.height - ((float) y);

   /* Finalise setting the frame on the main thread. Because this is where
    * the window's frame was initially set, this is where it should be
    * modified too.
    */
   NSArray *param = [NSArray arrayWithObjects :
      [NSValue valueWithPointer:&rc],
      [NSValue valueWithPointer:window], nil];
   [ALSetWindowFrame performSelectorOnMainThread: @selector(set_frame:) 
                                     withObject: [NSValue valueWithPointer:param]
                                  waitUntilDone: YES];
   
   [pool drain];
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
static void set_window_title(ALLEGRO_DISPLAY *display, const char *title)
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) display;
   [dpy->win setTitle: [NSString stringWithUTF8String:title]];
   [pool drain];
}

/* set_icons:
 * Set the icon - OS X doesn't have per-window icons so 
 * ignore the display parameter
 */
static void set_icons(ALLEGRO_DISPLAY *display, int num_icons, ALLEGRO_BITMAP* bitmaps[])
{
   /* Multiple icons not yet implemented. */
   ALLEGRO_BITMAP *bitmap = bitmaps[num_icons - 1];
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   NSImage *image = NSImageFromAllegroBitmap(bitmap);
   (void)display;
   [NSApp setApplicationIconImage: image];
   [image release];
   [pool drain];
}

/* set_display_flag:
 * Change settings for an already active display
 */
static bool set_display_flag(ALLEGRO_DISPLAY *display, int flag, bool onoff)
{
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
   return false;
#else
   if (!display)
      return false;

   ALLEGRO_DISPLAY_OSX_WIN *dpy = (ALLEGRO_DISPLAY_OSX_WIN *)display;
   NSUInteger mask;
   NSWindow *win = dpy->win;

   if (!win)
      return false;

   switch (flag) {
      case ALLEGRO_FRAMELESS:
         mask = [win styleMask];
         if (onoff)
            mask |= NSBorderlessWindowMask;
         else
            mask &= ~NSBorderlessWindowMask;
         ALLEGRO_DEBUG("Toggle FRAME for display %p to %d\n", dpy, onoff);
         [win setStyleMask : mask];
         return true;

      case ALLEGRO_RESIZABLE:
         mask = [win styleMask];
         if (onoff)
            mask |= NSResizableWindowMask;
         else
            mask &= ~NSResizableWindowMask;
         ALLEGRO_DEBUG("Toggle RESIZABLE for display %p to %d\n", dpy, onoff);
         [win setStyleMask : mask];
         return true;

      case ALLEGRO_FULLSCREEN_WINDOW: {
         ALOpenGLView *view = (ALOpenGLView *)[win contentView];
         if (onoff) {
            [view performSelectorOnMainThread: @selector(enterFullScreenWindowMode) withObject:nil waitUntilDone:YES];
            NSRect sc = [[win screen] frame];
            resize_display_win(display, sc.size.width, sc.size.height);
            display->flags |= ALLEGRO_FULLSCREEN_WINDOW;
         } else {
            [view performSelectorOnMainThread: @selector(exitFullScreenWindowMode) withObject:nil waitUntilDone:YES];
            NSRect sc = [view frame];
            display->flags &= ~ALLEGRO_FULLSCREEN_WINDOW;
            resize_display_win(display, sc.size.width, sc.size.height);
            [view performSelectorOnMainThread: @selector(finishExitingFullScreenWindowMode) withObject:nil waitUntilDone:YES];
         }
         return true;
      }
   }

   return false;
#endif
}

ALLEGRO_DISPLAY_INTERFACE* _al_osx_get_display_driver_win(void)
{
   static ALLEGRO_DISPLAY_INTERFACE* vt = NULL;
   if (vt == NULL) {
      vt = al_malloc(sizeof(*vt));
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
      vt->set_mouse_cursor = osx_set_mouse_cursor;
      vt->set_system_mouse_cursor = osx_set_system_mouse_cursor;
      vt->get_window_position = get_window_position;
      vt->set_window_position = set_window_position;
      vt->set_window_title = set_window_title;
      vt->set_display_flag = set_display_flag;
      vt->set_icons = set_icons;
      _al_ogl_add_drawing_functions(vt);
   }
   return vt;
}   

ALLEGRO_DISPLAY_INTERFACE* _al_osx_get_display_driver_fs(void)
{
   static ALLEGRO_DISPLAY_INTERFACE* vt = NULL;
   if (vt == NULL) {
      vt = al_malloc(sizeof(*vt));
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
      vt = al_malloc(sizeof(*vt));
      memset(vt, 0, sizeof(ALLEGRO_DISPLAY_INTERFACE));
      vt->create_display = create_display;
   }
   return vt;
}

/* Function: al_osx_get_window
 */
NSWindow* al_osx_get_window(ALLEGRO_DISPLAY *display)
{
   if (!display)
      return NULL;
   return ((ALLEGRO_DISPLAY_OSX_WIN *)display)->win;
}
