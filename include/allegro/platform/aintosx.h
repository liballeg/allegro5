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
 *      Internal header file for the MacOS X Allegro library port.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#ifndef AINTOSX_H
#define AINTOSX_H

#include "allegro/platform/aintunix.h"

#ifdef __OBJC__

#define display_id       kCGDirectMainDisplay

#define OSX_GFX_NONE     0
#define OSX_GFX_WINDOW   1
#define OSX_GFX_FULL     2


@interface AllegroAppDelegate : NSObject
- (NSApplicationTerminateReply)applicationShouldTerminate: (NSApplication *)sender;
- (void)applicationDidFinishLaunching: (NSNotification *)aNotification;
+ (void)app_main: (id)arg;
@end


@interface AllegroWindow : NSWindow
- (void)display;
- (void)miniaturize: (id)sender;
@end


@interface AllegroWindowDelegate : NSObject
- (BOOL)windowShouldClose: (id)sender;
- (void)windowDidDeminiaturize: (NSNotification *)aNotification;
@end


@interface AllegroView: NSQuickDrawView
- (void)resetCursorRects;
@end


typedef void RETSIGTYPE;

typedef struct BMP_EXTRA_INFO
{
   GrafPtr port;
} BMP_EXTRA_INFO;

#define BMP_EXTRA(bmp)     ((BMP_EXTRA_INFO *)((bmp)->extra))


void osx_event_handler(void);

void setup_direct_shifts(void);
void osx_qz_blit_to_self(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void osx_qz_created_sub_bitmap(BITMAP *bmp, BITMAP *parent);
BITMAP *osx_qz_create_video_bitmap(int width, int height);
BITMAP *osx_qz_create_system_bitmap(int width, int height);
void osx_qz_destroy_video_bitmap(BITMAP *bmp);
void osx_update_dirty_lines(void);

unsigned long osx_qz_write_line(BITMAP *bmp, int line);
void osx_qz_unwrite_line(BITMAP *bmp);
void osx_qz_acquire(BITMAP *bmp);
void osx_qz_release(BITMAP *bmp);

void osx_keyboard_handler(int pressed, NSEvent *event);
void osx_keyboard_modifiers(unsigned int new_mods);
void osx_keyboard_focused(int focused, int state);

void osx_mouse_handler(int dx, int dy, int dz, int buttons);


AL_VAR(NSBundle *, osx_bundle);
AL_VAR(pthread_mutex_t, osx_event_mutex);
AL_VAR(pthread_mutex_t, osx_window_mutex);
AL_VAR(int, osx_gfx_mode);
AL_VAR(NSCursor *, osx_cursor);
AL_VAR(AllegroWindow *, osx_window);
AL_ARRAY(char, osx_window_title);
AL_METHOD(void, osx_window_close_hook, (void));
AL_VAR(int, osx_mouse_warped);
AL_VAR(int, osx_skip_mouse_move);
AL_VAR(int, osx_emulate_mouse_buttons);
AL_VAR(NSTrackingRectTag, osx_mouse_tracking_rect);
AL_VAR(NoteAllocator, osx_note_allocator);


#endif

#endif
