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
 *      Definitions for internal use by the BeOS configuration.
 *
 *      By Jason Wilkins.
 *
 *      See readme.txt for copyright information.
 */


#ifndef __BEOS__
   #error bad include
#endif

#include "bealleg.h"

#ifdef __cplusplus
extern status_t       ignore_result;

extern volatile int32 focus_count;
#endif

#ifdef __cplusplus
extern "C" {
#endif

int  be_key_init(void);
void be_key_exit(void);
void be_key_set_leds(int leds);
void be_key_wait_for_input(void);
void be_key_stop_waiting_for_input(void); 

int  be_sys_init(void);
void be_sys_exit(void);
void _be_sys_get_executable_name(char *output, int size);
void be_sys_get_executable_name(char *output, int size);
void be_sys_set_window_title(char *name);
void be_sys_message(char *msg);

struct BITMAP *be_gfx_fullscreen_init(int w, int h, int v_w, int v_h, int color_depth);
struct BITMAP *be_gfx_fullscreen_safe_init(int w, int h, int v_w, int v_h, int color_depth);
void be_gfx_fullscreen_exit(struct BITMAP *b);
void be_gfx_fullscreen_acquire(struct BITMAP *b);
void be_gfx_fullscreen_release(struct BITMAP *b);
void be_gfx_fullscreen_set_palette(struct RGB *p, int from, int to, int vsync);
int  be_gfx_fullscreen_scroll(int x, int y);
void be_gfx_fullscreen_vsync(void);
void _be_gfx_fullscreen_read_write_bank(void);
void _be_gfx_fullscreen_unwrite_bank(void);

int  be_time_init(void);
void be_time_exit(void);
//int  be_time_can_simulate_retrace(void);
//void be_time_simulate_retrace(int enable);
void be_time_rest(long time, AL_METHOD(void, callback, (void)));

int be_mouse_init(void);
void be_mouse_exit(void);
void be_mouse_position(int x, int y);
void be_mouse_set_range(int x1, int y1, int x2, int y2);
void be_mouse_set_speed(int xspeed, int yspeed);
void be_mouse_get_mickeys(int *mickeyx, int *mickeyy);

#ifdef __cplusplus
}
#endif
