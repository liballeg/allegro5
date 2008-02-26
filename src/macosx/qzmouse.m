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
*      MacOS X mouse driver.
*
*      By Angelo Mottola.
*
*      Modified for 4.9 mouse API by Peter Hull.
*
*      See readme.txt for copyright information.
*/


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_mouse.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/platform/aintosx.h"
// #import "allegro/internal/aintern2.h"

#ifndef ALLEGRO_MACOSX
#error Something is wrong with the makefile
#endif

typedef struct ALLEGRO_MOUSE AL_MOUSE;
typedef struct ALLEGRO_MSESTATE AL_MSESTATE;

static bool osx_mouse_init(void);
static void osx_mouse_exit(void);
static bool osx_mouse_position(int, int);
static bool osx_mouse_set_range(int, int, int, int);
static void osx_mouse_get_mickeys(int *, int *);
static void osx_enable_hardware_cursor(AL_CONST int mode);
static int osx_select_system_cursor(AL_CONST int cursor);
static unsigned int osx_get_mouse_num_buttons(void);
static unsigned int osx_get_mouse_num_axes(void);
static bool osx_set_mouse_axis(int axis, int value);
static ALLEGRO_MOUSE* osx_get_mouse(void);
static void osx_get_state(ALLEGRO_MSESTATE *ret_state);


ALLEGRO_MOUSE_DRIVER mouse_macosx = {
	MOUSE_MACOSX,
	empty_string,
	empty_string,
	"MacOS X mouse",
	osx_mouse_init,
	osx_mouse_exit,
	osx_get_mouse, // ALLEGRO_METHOD(ALLEGRO_MOUSE*, get_mouse, (void));
	osx_get_mouse_num_buttons, // ALLEGRO_METHOD(unsigned int, get_mouse_num_buttons, (void));
	osx_get_mouse_num_axes, // ALLEGRO_METHOD(unsigned int, get_mouse_num_axes, (void));
	osx_mouse_position, // ALLEGRO_METHOD(bool, set_mouse_xy, (int x, int y));
	osx_set_mouse_axis, // ALLEGRO_METHOD(bool, set_mouse_axis, (int axis, int value));
	osx_mouse_set_range, // ALLEGRO_METHOD(bool, set_mouse_range, (int x1, int y1, int x2, int y2));
	osx_get_state, // ALLEGRO_METHOD(void, get_state, (ALLEGRO_MSESTATE *ret_state));
};


/* global variable */
int osx_mouse_warped = FALSE;
int osx_skip_mouse_move = FALSE;
NSTrackingRectTag osx_mouse_tracking_rect = -1;

/* Mouse info - includes extra info for OS X */
static struct {
	ALLEGRO_MOUSE parent;
	unsigned int button_count;
	unsigned int axis_count;
	int minx, miny, maxx, maxy;
	ALLEGRO_MSESTATE state;
	float z_axis, w_axis;
	BOOL captured;
	NSCursor* cursor;
} osx_mouse;

/* osx_get_mouse:
* Return the Allegro mouse structure
*/
static ALLEGRO_MOUSE* osx_get_mouse(void)
{
	return (ALLEGRO_MOUSE*) &osx_mouse.parent;
}

static char driver_desc[256];


/* osx_change_cursor:
* Actually change the current cursor. This can be called fom any thread 
* but ensures that the change is only called from the main thread.
*/
static void osx_change_cursor(NSCursor* cursor)
{
	return (ALLEGRO_MOUSE*) &osx_mouse.parent;
}

/* osx_mouse_generate_event:
* Convert an OS X mouse event to an Allegro event
* and push it into a queue.
* First check that the event is wanted.
*/
void osx_mouse_generate_event(NSEvent* evt, ALLEGRO_DISPLAY* dpy)
{
	NSPoint pos;
	int type, b_change = 0, dx = 0, dy = 0, dz = 0, dw = 0, b = 0;
	switch ([evt type])
	{
		case NSMouseMoved:
			type = ALLEGRO_EVENT_MOUSE_AXES;
			dx = [evt deltaX];
			dy = [evt deltaY];
			break;
		case NSLeftMouseDragged:
		case NSRightMouseDragged:
		case NSOtherMouseDragged:
			type = ALLEGRO_EVENT_MOUSE_AXES;
			b = [evt buttonNumber]+1;
			dx = [evt deltaX];
			dy = [evt deltaY];
			break;
		case NSLeftMouseDown:
		case NSRightMouseDown:
		case NSOtherMouseDown:
			type = ALLEGRO_EVENT_MOUSE_BUTTON_DOWN;
			b = [evt buttonNumber]+1;
			b_change = 1;
			break;
		case NSLeftMouseUp:
		case NSRightMouseUp:
		case NSOtherMouseUp:
			type = ALLEGRO_EVENT_MOUSE_BUTTON_UP;
			b = [evt buttonNumber]+1;
			break;
		case NSScrollWheel:
			type = ALLEGRO_EVENT_MOUSE_AXES;
			dx = 0;
			dy = 0;
			osx_mouse.w_axis += [evt deltaX];
			osx_mouse.z_axis += [evt deltaY];
			dw = osx_mouse.w_axis - osx_mouse.state.w;
			dz = osx_mouse.z_axis - osx_mouse.state.z;
			break;
		case NSMouseEntered:
			type = ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY;
			b = [evt buttonNumber]+1;
			dx = [evt deltaX];
			dy = [evt deltaY];
			break;
		case NSMouseExited:
			type = ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY;
			b = [evt buttonNumber]+1;
			dx = [evt deltaX];
			dy = [evt deltaY];
			break;
		default:
			return;
	}
	pos = [evt locationInWindow];
	BOOL within = TRUE;			
	if ([evt window])
	{
		NSRect frm = [[[evt window] contentView] frame];
		within = NSMouseInRect(pos, frm, NO);
		// Y-coordinates in OS X start from the bottom.
		pos.y = NSHeight(frm) - pos.y;
	}
	else 
	{
		// To do: full screen handling 
	}
	_al_event_source_lock(&osx_mouse.parent.es);
	if ((within || osx_mouse.captured) && _al_event_source_needs_to_generate_event(&osx_mouse.parent.es))
	{
		ALLEGRO_EVENT* new_event = _al_event_source_get_unused_event(&osx_mouse.parent.es);
		ALLEGRO_MOUSE_EVENT* mouse_event = &new_event->mouse;
		mouse_event->type = type;
		// Note: we use 'allegro time' rather than the time stamp 
		// from the event 
		mouse_event->timestamp = al_current_time();
		mouse_event->button = b;
		mouse_event->x = pos.x;
		mouse_event->y = pos.y; 
		mouse_event->z = osx_mouse.z_axis;
		mouse_event->dx = dx;
		mouse_event->dy = dy;
		mouse_event->dz = dz;
		_al_event_source_emit_event(&osx_mouse.parent.es, new_event);
	}
	// Record current state
	osx_mouse.state.x = pos.x;
	osx_mouse.state.y = pos.y;
	osx_mouse.state.w = osx_mouse.w_axis;
	osx_mouse.state.z = osx_mouse.z_axis;
	if (b_change)
		osx_mouse.state.buttons |= (1<<b);
	else
		osx_mouse.state.buttons &= ~(1<<b);
	_al_event_source_unlock(&osx_mouse.parent.es);
}

/* osx_mouse_init:
*  Initializes the driver.
*/
static bool osx_mouse_init(void)
{
	HID_DEVICE_COLLECTION devices={0,0,NULL};
	int i = 0, j = 0;
	int axes = 0, buttons = 0;
	HID_DEVICE* device = nil;
	NSString* desc = nil;
	
	if (floor(NSAppKitVersionNumber) <= NSAppKitVersionNumber10_1) {
		/* On 10.1.x mice and keyboards aren't available from the HID Manager,
		* so we can't autodetect. We assume an 1-button mouse to always be
		* present.
		*/
		buttons = 1;
		axes = 2;
	}
	else {
		osx_hid_scan(HID_MOUSE, &devices);
		if (devices.count > 0) 
		{
            device=&devices.devices[i];
            buttons = 0;
            axes = 0;
            for (j = 0; j < device->num_elements; j++) 
			{
				switch(device->element[j].type)
				{
					case HID_ELEMENT_BUTTON:
                        buttons ++;
                        break;
					case HID_ELEMENT_AXIS:
					case HID_ELEMENT_AXIS_PRIMARY_X:
					case HID_ELEMENT_AXIS_PRIMARY_Y:
					case HID_ELEMENT_STANDALONE_AXIS:
                        axes ++;
                        break;
				}
			}
            desc = [NSString stringWithFormat: @"%s %s",
 device->manufacturer ? device->manufacturer : "",
		   device->product ? device->product : ""];
		}
		osx_hid_free(&devices);
	}
	if (buttons <= 0) return FALSE;
	_al_mutex_lock(&osx_event_mutex);
	const char* str = [desc UTF8String];
	/* FIXME */
	// mouse_macosx.desc = strcpy(malloc(strlen(str) + 1), str);
	osx_emulate_mouse_buttons = (buttons == 1) ? TRUE : FALSE;
	_al_event_source_init(&osx_mouse.parent.es);
	osx_mouse.button_count = buttons;
	osx_mouse.axis_count = axes;
	memset(&osx_mouse.state, 0, sizeof(ALLEGRO_MSESTATE));
	_al_mutex_unlock(&osx_event_mutex);
	return TRUE;
}

/* osx_get_mouse_num_buttons:
* Return the number of buttons on the mouse
*/
static unsigned int osx_get_mouse_num_buttons(void)
{
	return osx_mouse.button_count;
}

/* osx_get_mouse_num_buttons:
* Return the number of buttons on the mouse
*/
static unsigned int osx_get_mouse_num_axes(void)
{
	return osx_mouse.axis_count;
}

/* osx_mouse_exit:
*  Shuts down the driver.
*/
static void osx_mouse_exit(void)
{
	osx_cursor = osx_blank_cursor;
	// Go back to the system cursor
	[[NSCursor arrowCursor] performSelectorOnMainThread: @selector(set)
											 withObject: nil
										  waitUntilDone: NO];
	[osx_mouse.cursor release];
	osx_mouse.cursor = nil;
	/* FIXME */
	// free((char*) mouse_macosx.desc);
	_al_event_source_free(&osx_mouse.parent.es);
	osx_mouse.button_count = 0;
}



/* osx_mouse_position:
*  Sets the position of the mouse.
*/
static bool osx_mouse_position(int x, int y)
{
	CGPoint point;
	NSRect frame;
	int screen_height;
	
	/* at the moment, disable this feature, as it doesn't work well on windowed mode */
	return FALSE;
	
	_al_mutex_lock(&osx_event_mutex);
	
	point.x = x;
	point.y = y;
	
	
	CGDisplayMoveCursorToPoint(kCGDirectMainDisplay, point);
	
	osx_mouse_warped = TRUE;
	
	_al_mutex_unlock(&osx_event_mutex);
	return TRUE;
}



/* osx_mouse_set_range:
*  Sets the range of the mouse.
*/
static bool osx_mouse_set_range(int x1, int y1, int x2, int y2)
{
	// May be deleted soon
	osx_mouse.minx = x1;
	osx_mouse.miny = y1;
	osx_mouse.maxx = x2;
	osx_mouse.maxy = y2;
	
	return TRUE;
}

/* osx_mouse_set_sprite:
*  Sets the hardware cursor sprite.
*/
int osx_mouse_set_sprite(BITMAP *sprite, int x, int y)
{
	int ix, iy;
	int sw, sh;
	
	if (!sprite)
		return -1;
	sw = sprite->w;
	sh = sprite->h;
	if (floor(NSAppKitVersionNumber) <= NSAppKitVersionNumber10_2) {
		// Before MacOS X 10.3, NSCursor can handle only 16x16 cursor sprites
		// Pad to 16x16 or fail if the sprite is already larger.
		if (sw>16 || sh>16)
			return -1;
		sh = sw = 16;
	}
	
	// release any previous cursor
	[osx_mouse.cursor release];
	
	NSBitmapImageRep* cursor_rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes: NULL
																		   pixelsWide: sw
																		   pixelsHigh: sh
																		bitsPerSample: 8
																	  samplesPerPixel: 4
																			 hasAlpha: YES
																			 isPlanar: NO
																	   colorSpaceName: NSDeviceRGBColorSpace
																		  bytesPerRow: 0
																		 bitsPerPixel: 0];
	int bpp = bitmap_color_depth(sprite);
	int mask = bitmap_mask_color(sprite);
	for (iy = 0; iy< sh; ++iy) {
		unsigned char* ptr = [cursor_rep bitmapData] + (iy * [cursor_rep bytesPerRow]);
		for (ix = 0; ix< sw; ++ix) {
			int color = is_inside_bitmap(sprite, ix, iy, 1) 
            ? getpixel(sprite, ix, iy) : mask; 
			// Disable the possibility of mouse sprites with alpha for now, because
			// this causes the built-in cursors to be invisible in 32 bit mode.
			// int alpha = (color == mask) ? 0 : ((bpp == 32) ? geta_depth(bpp, color) : 255);
			int alpha = (color == mask) ? 0 : 255;
			// BitmapImageReps use premultiplied alpha
			ptr[0] = getb_depth(bpp, color) * alpha / 255;
			ptr[1] = getg_depth(bpp, color) * alpha / 255;
			ptr[2] = getr_depth(bpp, color) * alpha / 255;
			ptr[3] = alpha;
			ptr += 4;
		}
	}
	NSImage* cursor_image = [[NSImage alloc] initWithSize: NSMakeSize(sw, sh)];
	[cursor_image addRepresentation: cursor_rep];
	[cursor_rep release];
	osx_mouse.cursor = [[NSCursor alloc] initWithImage: cursor_image
											   hotSpot: NSMakePoint(x, y)];
	
	[cursor_image release];
	NSView* v = osx_view_from_display(NULL);
	if (v)
	{
		[v performSelectorOnMainThread: @selector(setCursor)
							withObject: osx_mouse.cursor
						 waitUntilDone: NO];
	}
	else
	{
		[osx_mouse.cursor set];
	}
	
	return 0;
}


/* osx_mouse_show:
*  Show the hardware cursor.
*/
int osx_mouse_show(BITMAP *bmp, int x, int y)
{
	/* Only draw on screen */
	if (!is_same_bitmap(bmp, screen))
		return -1;
	
	NSView* v = osx_view_from_display(NULL);
	if (v)
	{
		[v performSelectorOnMainThread: @selector(setCursorVisible)
							withObject: nil
						 waitUntilDone: NO];
	}
	else
	{
		[NSCursor unhide];
	}
	
	return 0;
}



/* osx_mouse_hide:
*  Hide the hardware cursor.
*/
void osx_mouse_hide(void)
{
	NSView* v = osx_view_from_display(NULL);
	if (v)
	{
		[v performSelectorOnMainThread: @selector(setCursorHidden)
							withObject: nil
						 waitUntilDone: NO];
	}
	else
	{
		[NSCursor hide];
	}
}

/* osx_enable_hardware_cursor:
*  Enable hardware cursor - on OSX it's always enabled.
*/
void osx_enable_hardware_cursor(AL_CONST int mode) 
{
	(void)mode;
}



/* osx_select_system_cursor:
*  Select a system cursor - on this platform, only the I-beam and the Arrow
*  are available as system cursors.
*/
static int osx_select_system_cursor(AL_CONST int cursor)
{
	NSCursor* requested_cursor;
	switch (cursor) {
		case MOUSE_CURSOR_ARROW:
			requested_cursor = [NSCursor arrowCursor];
			break;
		case MOUSE_CURSOR_EDIT:
			requested_cursor = [NSCursor IBeamCursor];
			break;
		default:
			return 0;
	}
	NSView* v = osx_view_from_display(NULL);
	if (v)
	{
		[v performSelectorOnMainThread: @selector(setCursor)
							withObject: requested_cursor
						 waitUntilDone: NO];
	}
	else
	{
		[requested_cursor set];
	}
	return cursor;
}

static void osx_get_state(ALLEGRO_MSESTATE *ret_state)
{
	_al_event_source_lock(&osx_mouse.parent.es);
	memcpy(ret_state, &osx_mouse.state, sizeof(ALLEGRO_MSESTATE));
	_al_event_source_unlock(&osx_mouse.parent.es);
}

/* osx_set_mouse_axis:
* Set the axis value of the mouse
*/
static bool osx_set_mouse_axis(int axis, int value) 
{
	_al_event_source_lock(&osx_mouse.parent.es);	
	switch(axis)
	{
		case 0:
		case 1:
			// By design, this doesn't apply to (x, y)
			break;
		case 2:
			osx_mouse.z_axis = value;
			return TRUE;
		case 3:
			osx_mouse.w_axis = value;
			return TRUE;
	}
	_al_event_source_unlock(&osx_mouse.parent.es);
	return FALSE;
}

/* list the available drivers */
_DRIVER_INFO _al_mouse_driver_list[] =
{
	{  MOUSE_MACOSX, &mouse_macosx, TRUE  },
	{  0,  NULL,  0  }
};

/*
 * osx_mouse_move:
 *  Required for compatibility with 4.2 api, may disappear soon.
 */
void osx_mouse_move(int x, int y)
{
}

/* Local variables:       */
/* c-basic-offset: 3      */
/* indent-tabs-mode: nil  */
/* End:                   */
