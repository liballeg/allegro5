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
 *      Improved screen display API, draft.
 *
 *      See readme.txt for copyright information.
 */
#ifndef ALLEGRO_DISPLAY_H
#define ALLEGRO_DISPLAY_H

#include "base.h"
#include "gfx.h"

#ifdef __cplusplus
   extern "C" {
#endif

/* Colour depth defines */
#define AL_DEPTH_AUTO   0
#define AL_DEPTH_8      8
#define AL_DEPTH_15     15
#define AL_DEPTH_16     16
#define AL_DEPTH_24     24
#define AL_DEPTH_32     32

/* Update method flags */
#define AL_UPDATE_NONE              0x0000
#define AL_UPDATE_TRIPLE_BUFFER     0x0001
#define AL_UPDATE_PAGE_FLIP         0x0002
#define AL_UPDATE_SYSTEM_BUFFER     0x0004
#define AL_UPDATE_DOUBLE_BUFFER     0x0008

#define AL_UPDATE_ALL               0x000F

/* Misc. flags */
#define AL_DISABLE_VSYNC            0x1000

#if 0
typedef struct AL_DISPLAY {   /* Allegro display struct */
   GFX_DRIVER *gfx_driver;    /* Graphics driver for this display */
   BITMAP *screen;            /* Screen bitmap */
   BITMAP **page;             /* Display pages */
   int num_pages;             /* Number of display pages */
   int active_page;           /* Number of active page (tri/dbl buffer) */
   int flags;                 /* Display flags */
   int depth;                 /* Colour depth */
   int gfx_capabilities;      /* Driver capabilities */
} AL_DISPLAY;
#endif

//AL_VAR(AL_DISPLAY *, al_main_display);

/*
AL_FUNC(BITMAP *, al_create_video_bitmap, (AL_DISPLAY *display, int width, int height));
AL_FUNC(BITMAP *, al_create_system_bitmap, (AL_DISPLAY *display, int width, int height));

AL_FUNC(int, al_scroll_display, (AL_DISPLAY *display, int x, int y));
AL_FUNC(int, al_request_scroll, (AL_DISPLAY *display, int x, int y));
AL_FUNC(int, al_poll_scroll, (AL_DISPLAY *display));
AL_FUNC(int, al_show_video_bitmap, (AL_DISPLAY *display, BITMAP *bitmap));
AL_FUNC(int, al_request_video_bitmap, (AL_DISPLAY *display, BITMAP *bitmap));
AL_FUNC(int, al_enable_triple_buffer, (AL_DISPLAY *display));

//AL_FUNC(AL_DISPLAY *, al_create_display, (int driver, int flags, int depth, int w, int h));
AL_FUNC(int, al_set_update_method, (AL_DISPLAY *display, int method));
AL_FUNC(void, al_destroy_display, (AL_DISPLAY *display));

AL_FUNC(void, al_flip_display, (AL_DISPLAY *display));
AL_FUNC(BITMAP *, al_get_buffer, (const AL_DISPLAY *display));
AL_FUNC(int, al_get_update_method, (const AL_DISPLAY *display));

AL_FUNC(void, al_enable_vsync, (AL_DISPLAY *display));
AL_FUNC(void, al_disable_vsync, (AL_DISPLAY *display));
AL_FUNC(void, al_toggle_vsync, (AL_DISPLAY *display));
AL_FUNC(int, al_vsync_is_enabled, (const AL_DISPLAY *display));
*/

AL_FUNC(BITMAP *, al_create_video_bitmap, ( int width, int height));
AL_FUNC(BITMAP *, al_create_system_bitmap, ( int width, int height));

AL_FUNC(int, al_scroll_display, (int x, int y));
AL_FUNC(int, al_request_scroll, (int x, int y));
AL_FUNC(int, al_poll_scroll, ());
AL_FUNC(int, al_show_video_bitmap, (BITMAP *bitmap));
AL_FUNC(int, al_request_video_bitmap, (BITMAP *bitmap));
AL_FUNC(int, al_enable_triple_buffer, ());
/*
AL_FUNC(AL_DISPLAY *, al_create_display, (int driver, int flags, int depth, int w, int h));
AL_FUNC(int, al_set_update_method, (int method));
AL_FUNC(void, al_destroy_display, ());

AL_FUNC(void, al_flip_display, ());
AL_FUNC(BITMAP *, al_get_buffer, (const AL_DISPLAY *display));
AL_FUNC(int, al_get_update_method, (const AL_DISPLAY *display));

AL_FUNC(void, al_enable_vsync, (AL_DISPLAY *display));
AL_FUNC(void, al_disable_vsync, (AL_DISPLAY *display));
AL_FUNC(void, al_toggle_vsync, (AL_DISPLAY *display));
AL_FUNC(int, al_vsync_is_enabled, (const AL_DISPLAY *display));
*/
#ifdef __cplusplus
   }
#endif


#endif      /* #ifndef ALLEGRO_DISPLAY_H */
