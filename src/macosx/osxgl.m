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
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/platform/aintosx.h"
#ifndef ALLEGRO_MACOSX
#error something is wrong with the makefile
#endif

#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>

typedef struct _AL_DISPLAY_OSX_WIN {
	ALLEGRO_DISPLAY parent;
	int gl_fmt, gl_datasize;
	NSOpenGLContext* ctx;
	NSWindow* win;
	ALLEGRO_BITMAP* backbuffer;
} _AL_DISPLAY_OSX_WIN;

ALLEGRO_BITMAP* create_backbuffer_bitmap(void);
ALLEGRO_BITMAP_INTERFACE *osx_bitmap_driver(void);

/* The main additions to this view are event-handling functions */
@interface ALOpenGLView : NSOpenGLView
{
	/* This is passed onto the event functions so we know where the event came from */
	ALLEGRO_DISPLAY* dpy_ptr;
	/* We maintain one tracking rect over the whole view */
	NSTrackingRectTag trackingRect;
}
-(void)setDisplay: (ALLEGRO_DISPLAY*) ptr;
-(ALLEGRO_DISPLAY*) display;
-(void) reshape;
-(BOOL) acceptsFirstResponder;
-(void) drawRect: (NSRect) rc;
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
-(void) mouseEntered: (NSEvent*) evt;
-(void) mouseExited: (NSEvent*) evt;
@end

@implementation ALOpenGLView
/* setDisplay:
* Set the display this view is associated with
*/
-(void) setDisplay: (ALLEGRO_DISPLAY*) ptr {
	dpy_ptr = ptr;
}

/* display 
* return the display this view is associated with
*/
-(ALLEGRO_DISPLAY*) display {
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

/* drawRect:
* Called when OS X wants the view to be redrawn
*/
-(void) drawRect: (NSRect) rc
{
	// To do: pass on notifications to the display queue
}

/* Keyboard event handler */
-(void) keyDown:(NSEvent*) event
{
	if (_keyboard_installed)
		osx_keyboard_handler(TRUE, event, dpy_ptr);
}
-(void) keyUp:(NSEvent*) event 
{
	if (_keyboard_installed)
		osx_keyboard_handler(FALSE, event, dpy_ptr);
}
-(void) flagsChanged:(NSEvent*) event
{
	if (_keyboard_installed) {
		osx_keyboard_modifiers([event modifierFlags], dpy_ptr);
	}
}

/* Mouse handling */
-(void) mouseDown: (NSEvent*) evt
{
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) mouseUp: (NSEvent*) evt
{
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) mouseDragged: (NSEvent*) evt
{
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) rightMouseDown: (NSEvent*) evt
{
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) rightMouseUp: (NSEvent*) evt
{
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) rightMouseDragged: (NSEvent*) evt
{
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) otherMouseDown: (NSEvent*) evt
{
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) otherMouseUp: (NSEvent*) evt
{
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) otherMouseDragged: (NSEvent*) evt
{
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) mouseMoved: (NSEvent*) evt
{
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) scrollWheel: (NSEvent*) evt
{
	osx_mouse_generate_event(evt, dpy_ptr);
}
/* Cursor handling */
- (void)viewDidMoveToWindow {
    trackingRect = [self addTrackingRect:[self bounds] owner:self userData:NULL assumeInside:NO];
}
-(void) mouseEntered: (NSEvent*) evt
{
	[NSCursor hide];
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) mouseExited: (NSEvent*) evt
{
	osx_mouse_generate_event(evt, dpy_ptr);
	[NSCursor unhide];
}

/* windowShouldClose:
* Veto the close and post a message
*/
- (BOOL)windowShouldClose:(id)sender
{
	ALLEGRO_EVENT_SOURCE* src = &([self display]->es);
	_al_event_source_lock(src);
	ALLEGRO_EVENT* evt = _al_event_source_get_unused_event(src);
	evt->type = ALLEGRO_EVENT_DISPLAY_CLOSE;
	_al_event_source_emit_event(src, evt);
	_al_event_source_unlock(src);
	return NO;
}
@end

/* set_current_display_win:
* Set the current windowed display to be current.
*/
void set_current_display_win(ALLEGRO_DISPLAY* d) {
	_AL_DISPLAY_OSX_WIN* dpy = (_AL_DISPLAY_OSX_WIN*) d;
	if (dpy->ctx != nil) {
		[dpy->ctx makeCurrentContext];
	}
}
/* Helper to set up GL state as we want it. */
static void setup_gl(ALLEGRO_DISPLAY *d)
{
	static bool get_extensions = true;
	if (get_extensions) {
		//FIXME: Direct call to AllegroGL function for now
		//FIXME: Is this the right place to do this?
		__allegro_gl_manage_extensions();
		get_extensions = false;
	}
	
	glViewport(0, 0, d->w, d->h);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, d->w, d->h, 0, -1, 1);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glPixelZoom(1.0f, -1.0f);
}

static int decode_allegro_format(int format, int* glfmt, int* glsize, int* depth) {
	static int glformats[][4] = {
		/* Skip pseudo formats */
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
		/* Actual formats */
	{GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_BGRA, 32}, /* ARGB_8888 */
	{GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8, GL_RGBA, 32}, /* RGBA_8888 */
	{GL_RGBA4, GL_UNSIGNED_SHORT_4_4_4_4_REV, GL_BGRA, 16}, /* ARGB_4444 */
	{GL_RGB8, GL_UNSIGNED_BYTE, GL_BGR, 32}, /* RGB_888 */
	{GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_RGB, 16}, /* RGB_565 */
	{0, 0, 0, 16}, /* RGB_555 */ //FIXME: how?
	{GL_INTENSITY, GL_UNSIGNED_BYTE, GL_COLOR_INDEX, 8}, /* PALETTE_8 */
	{GL_RGB5_A1, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGBA, 16}, /* RGBA_5551 */
	{0, 0, 0, 16}, /* ARGB_1555 */ //FIXME: how?
	{GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_RGBA, 32}, /* ABGR_8888 */
	{GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_RGBA, 32}, /* XBGR_8888 */
	{GL_RGB8, GL_UNSIGNED_BYTE, GL_RGB, 32}, /* BGR_888 */
	{GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_BGR, 32}, /* BGR_565 */
	{0, 0, 0, 16}, /* BGR_555 */ //FIXME: how?
	{GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8, GL_RGBA, 32}, /* RGBX_8888 */
	{GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_BGRA, 32}, /* XRGB_8888 */
	};
	// To do: map the pseudo formats
	if (format >= ALLEGRO_PIXEL_FORMAT_ARGB_8888 && format <= ALLEGRO_PIXEL_FORMAT_XRGB_8888) {
		*glfmt = glformats[format][2];
		*glsize = glformats[format][1];
		*depth = glformats[format][3];
		return 0;
	}
	else {
		return -1;
	}
}

/* create_display_win:
* Create a windowed display - create the window with an ALOpenGLView
* to be its content view
*/
static ALLEGRO_DISPLAY* create_display_win(int w, int h) {
	_AL_DISPLAY_OSX_WIN* dpy = _AL_MALLOC(sizeof(_AL_DISPLAY_OSX_WIN));
	if (dpy == NULL) {
		ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
		return NULL;
	}
	/* Set up the ALLEGRO_DISPLAY part */
	dpy->parent.vt = osx_get_display_driver();
	dpy->parent.format = ALLEGRO_PIXEL_FORMAT_RGBA_8888;
	dpy->parent.refresh_rate = al_get_new_display_refresh_rate();
	dpy->parent.flags = al_get_new_display_flags();
	dpy->parent.w = w;
	dpy->parent.h = h;
	_al_event_source_init(&dpy->parent.es);
	int depth;
	decode_allegro_format(dpy->parent.format,&dpy->gl_fmt,&dpy->gl_datasize, &depth);
	/* OSX specific part */
	NSRect rc = NSMakeRect(0, 0, w, h);
	NSWindow* win = dpy->win = [NSWindow alloc]; 
	[win initWithContentRect: rc
				   styleMask: NSTitledWindowMask|NSClosableWindowMask|NSMiniaturizableWindowMask
					 backing: NSBackingStoreBuffered
					   defer: NO];
	NSOpenGLPixelFormatAttribute attrs[] = 
	{
		NSOpenGLPFAColorSize,
		depth, 
		NSOpenGLPFAWindow,
		0
	};
	NSOpenGLPixelFormat* fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes: attrs];
	ALOpenGLView* view = [[ALOpenGLView alloc] initWithFrame: rc pixelFormat: fmt];
	[fmt release];
	/* Hook up the view to its display */
	[view setDisplay: &dpy->parent];
	dpy->ctx = [[view openGLContext] retain];
	[dpy->ctx makeCurrentContext];
	[win setContentView: view];
	[win setDelegate: view];
	[win setReleasedWhenClosed: YES];
	[win setAcceptsMouseMovedEvents: YES];
	[win center];
	[win setTitle: @"Allegro"];
	/* Realize the window on the main thread */
	[win performSelectorOnMainThread: @selector(makeKeyAndOrderFront:) withObject: nil waitUntilDone: YES]; 
	[win makeMainWindow];
	[view release];
	dpy->backbuffer = create_backbuffer_bitmap();
	/* Set up GL as we want */
	setup_gl(&dpy->parent);
	
	ALLEGRO_DISPLAY **add = _al_vector_alloc_back(&al_system_driver()->displays);
	return *add = &dpy->parent;
}
static void destroy_display(ALLEGRO_DISPLAY* d) {
	_AL_DISPLAY_OSX_WIN* dpy = (_AL_DISPLAY_OSX_WIN*) d;
	al_destroy_bitmap(dpy->backbuffer);
	[dpy->ctx release];
	[dpy->win release];
	_al_event_source_free(&dpy->parent.es);
}

static ALLEGRO_BITMAP* get_backbuffer_win(ALLEGRO_DISPLAY* disp) {
	return ((_AL_DISPLAY_OSX_WIN*) disp)->backbuffer;
}
static void set_target_bitmap(ALLEGRO_DISPLAY* disp, ALLEGRO_BITMAP* bmp) {
	//TODO
}
static void osx_clear(ALLEGRO_DISPLAY *d, ALLEGRO_COLOR *color) {
	glClearColor((GLfloat) color->r, (GLfloat) color->g, (GLfloat) color->b, (GLfloat) color->a);
	glClear(GL_COLOR_BUFFER_BIT);
}
static void flip_display_win(ALLEGRO_DISPLAY *d) {
	glFlush();
}
static void set_opengl_blending(ALLEGRO_DISPLAY *d, ALLEGRO_COLOR *color)
{
	int blend_modes[4] = {
		GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
	};
	ALLEGRO_COLOR *bc;
	int src_mode, dst_mode;
	float r, g, b, a;
	al_unmap_rgba_f(*color, &r, &g, &b, &a);
	
	al_get_blender(&src_mode, &dst_mode, NULL);
	glEnable(GL_BLEND);
	glBlendFunc(blend_modes[src_mode], blend_modes[dst_mode]);
	bc = _al_get_blend_color();
	glColor4f(r * bc->r, g * bc->g, b * bc->b, a * bc->a);
}

static void show_cursor(ALLEGRO_DISPLAY *d) {
	_AL_DISPLAY_OSX_WIN* dpy = (_AL_DISPLAY_OSX_WIN*) d;
	ALOpenGLView* view = (ALOpenGLView*) [dpy->win contentView];
	
}
static void hide_cursor(ALLEGRO_DISPLAY *d) {
	_AL_DISPLAY_OSX_WIN* dpy = (_AL_DISPLAY_OSX_WIN*) d;
	ALOpenGLView* view = (ALOpenGLView*) [dpy->win contentView];
}
static bool resize_display(ALLEGRO_DISPLAY *d, int w, int h) {
	_AL_DISPLAY_OSX_WIN* dpy = (_AL_DISPLAY_OSX_WIN*) d;
	ALOpenGLView* view = (ALOpenGLView*) [dpy->win contentView];
	NSSize newsize = NSMakeSize((float) w,(float) h);
	
	[[view window] setContentSize: newsize];
	return true;
}
static ALLEGRO_BITMAP *osx_create_bitmap(ALLEGRO_DISPLAY *d, int w, int h)
{
	int format = al_get_new_bitmap_format();
	int flags = al_get_new_bitmap_flags();
	
	/* FIXME: do this right */
	if (! _al_pixel_format_is_real(format)) {
		format = d->format;
	}
	
	ALLEGRO_BITMAP *bitmap = _AL_MALLOC(sizeof *bitmap);
	memset(bitmap, 0, sizeof *bitmap);
	bitmap->vt = osx_bitmap_driver();
	bitmap->w = w;
	bitmap->h = h;
	bitmap->format = format;
	bitmap->flags = flags;
	bitmap->cl = 0;
	bitmap->ct = 0;
	bitmap->cr = w - 1;
	bitmap->cb = h - 1;
	
	return bitmap;
}
static bool is_compatible_bitmap(ALLEGRO_DISPLAY* dpy, ALLEGRO_BITMAP* bmp) {
	return true;
}
void osx_window_exit()
{
	return;
}

/* Draw a line. */
static void draw_line(ALLEGRO_DISPLAY *d, float fx, float fy, float tx, float ty,
					  ALLEGRO_COLOR *color)
{
	set_opengl_blending(d, color);
	glBegin(GL_LINES);
	glVertex2d(fx, fy);
	glVertex2d(tx, ty);
	glEnd();
}

ALLEGRO_DISPLAY_INTERFACE* osx_get_display_driver(void)
{
	static ALLEGRO_DISPLAY_INTERFACE display_interface = {
		0, //   int id;
		create_display_win, //   ALLEGRO_DISPLAY *(*create_display)(int w, int h);
		destroy_display, //   void (*destroy_display)(ALLEGRO_DISPLAY *display);
		set_current_display_win, //   void (*set_current_display)(ALLEGRO_DISPLAY *d);
		osx_clear, //   void (*clear)(ALLEGRO_DISPLAY *d, ALLEGRO_COLOR *color);
		draw_line, //   void (*draw_line)(ALLEGRO_DISPLAY *d, float fx, float fy, float tx, float ty,
				   //      ALLEGRO_COLOR *color);
		NULL, //   void (*draw_rectangle)(ALLEGRO_DISPLAY *d, float fx, float fy, float tx,
			  //    float ty, ALLEGRO_COLOR *color, int flags);
		flip_display_win, //   void (*flip_display)(ALLEGRO_DISPLAY *d);
		NULL, //   bool (*update_display_region)(ALLEGRO_DISPLAY *d, int x, int y,
			  //   	int width, int height);
		NULL, //   bool (*acknowledge_resize)(ALLEGRO_DISPLAY *d);
		resize_display, //   bool (*resize_display)(ALLEGRO_DISPLAY *d, int width, int height);
						//
		osx_create_bitmap, //   ALLEGRO_BITMAP *(*create_bitmap)(ALLEGRO_DISPLAY *d,
						   //   	int w, int h);
						   //   
		NULL, //   void (*upload_compat_screen)(struct BITMAP *bitmap, int x, int y, int width, int height);
		set_target_bitmap, //   void (*set_target_bitmap)(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap);
		get_backbuffer_win, //   ALLEGRO_BITMAP *(*get_backbuffer)(ALLEGRO_DISPLAY *display);
		NULL, //   ALLEGRO_BITMAP *(*get_frontbuffer)(ALLEGRO_DISPLAY *display);
			  //
		is_compatible_bitmap, //   bool (*is_compatible_bitmap)(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap);
		NULL, //   void (*switch_out)(ALLEGRO_DISPLAY *display);
		NULL, //   void (*switch_in)(ALLEGRO_DISPLAY *display);
			  //
		NULL, //   void (*draw_memory_bitmap_region)(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap,
			  //      float sx, float sy, float sw, float sh, float dx, float dy, int flags);
			  //
		NULL, //   ALLEGRO_BITMAP *(*create_sub_bitmap)(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *parent,
			  //      int x, int y, int width, int height);
			  //
		NULL,//   bool (*wait_for_vsync)(ALLEGRO_DISPLAY *display);
		show_cursor,//	bool (*show_cursor)(ALLEGRO_DISPLAY *display);
		hide_cursor,//	bool (*hide_cursor)(ALLEGRO_DISPLAY *display);
		NULL,//	void (*set_icon)(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap);
	};
	return &display_interface;
}

ALLEGRO_BITMAP* create_backbuffer_bitmap(void)
{
	ALLEGRO_BITMAP* bmp = _AL_MALLOC(sizeof(ALLEGRO_BITMAP));
	memset(bmp,0,sizeof(*bmp));
	return bmp;
}

static bool upload_bitmap(ALLEGRO_BITMAP* bmp, int x, int y, int w, int h) {
	return true;
}
static void draw_bitmap(ALLEGRO_BITMAP* bmp, float x, float y, int flags) {
	glRasterPos2f(x, y);
	glDrawPixels(bmp->w, bmp->h, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, bmp->memory);
}

ALLEGRO_BITMAP_INTERFACE *osx_bitmap_driver(void) {
	static ALLEGRO_BITMAP_INTERFACE vt = {
		0,//int id;
		draw_bitmap,//   void (*draw_bitmap)(struct ALLEGRO_BITMAP *bitmap, float x, float y, int flags);
		NULL,//   void (*draw_bitmap_region)(ALLEGRO_BITMAP *bitmap, float sx, float sy,
			 //      float sw, float sh, float dx, float dy, int flags);
		NULL,//   void (*draw_scaled_bitmap)(ALLEGRO_BITMAP *bitmap, float sx, float sy,
			 //      float sw, float sh, float dx, float dy, float dw, float dh, int flags);
		NULL,//   void (*draw_rotated_bitmap)(ALLEGRO_BITMAP *bitmap, float cx, float cy,
			 //      float angle, float dx, float dy, int flags);
		NULL,//   void (*draw_rotated_scaled_bitmap)(ALLEGRO_BITMAP *bitmap, float cx, float cy,
			 //      float angle, float dx, float dy, float xscale, float yscale,
			 //      float flags);
		upload_bitmap,//   bool (*upload_bitmap)(ALLEGRO_BITMAP *bitmap, int x, int y, int width, int height);
		NULL,//   void (*destroy_bitmap)(ALLEGRO_BITMAP *bitmap);
		NULL,//   ALLEGRO_LOCKED_REGION * (*lock_region)(ALLEGRO_BITMAP *bitmap,
			 //   	int x, int y, int w, int h,
			 //   	ALLEGRO_LOCKED_REGION *locked_region,
			 //	int flags);
		NULL,//   void (*unlock_region)(ALLEGRO_BITMAP *bitmap);
	};
	return &vt;
}