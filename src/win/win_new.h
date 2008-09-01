#ifndef _WIN_NEW_H
#define _WIN_NEW_H

#include "allegro5/internal/aintern_system.h"

#ifdef __cplusplus
extern "C" {
#endif


/* This is used to stop MinGW from complaining about type-punning */
#define MAKE_UNION(ptr, t) \
   union { \
      LPVOID *v; \
      t p; \
   } u; \
   u.p = (ptr);


typedef struct ALLEGRO_SYSTEM_WIN ALLEGRO_SYSTEM_WIN;

/* This is our version of ALLEGRO_SYSTEM with driver specific extra data. */
struct ALLEGRO_SYSTEM_WIN
{
	ALLEGRO_SYSTEM system; /* This must be the first member, we "derive" from it. */
};

AL_VAR(ALLEGRO_SYSTEM_WIN *, _al_win_system);

HWND _al_win_create_window(ALLEGRO_DISPLAY *display, int width, int height, int flags);
HWND _al_win_create_faux_fullscreen_window(LPCTSTR devname, ALLEGRO_DISPLAY *display,
	int x1, int y1, int width, int height, int refresh_rate, int flags);
int _al_win_init_window();
void _al_win_ungrab_input(void);
HWND _al_win_create_hidden_window();

extern HWND _al_win_wnd;
extern HWND _al_win_compat_wnd;
extern UINT _al_win_msg_call_proc;
extern UINT _al_win_msg_suicide;

extern HWND _al_win_active_window;

void _al_win_set_display_icon(ALLEGRO_DISPLAY *display ,ALLEGRO_BITMAP *bitmap);
HICON _al_win_create_icon(HWND wnd, ALLEGRO_BITMAP *sprite, int xfocus, int yfocus, bool is_cursor);
void _al_win_get_window_pos(HWND window, RECT *pos);


void _al_win_set_window_position(HWND window, int x, int y);
void _al_win_get_window_position(HWND window, int *x, int *y);
void _al_win_toggle_window_frame(ALLEGRO_DISPLAY *display, HWND window, int w, int h, bool onoff);
void _al_win_set_window_title(ALLEGRO_DISPLAY *display, AL_CONST char *title);


ALLEGRO_DISPLAY *_al_win_get_event_display(void);


HICON _al_win_create_mouse_hcursor(HWND wnd,
   ALLEGRO_BITMAP *sprite, int xfocus, int yfocus, bool is_cursor);


/* wmcursor.c */

typedef struct ALLEGRO_MOUSE_CURSOR_WIN ALLEGRO_MOUSE_CURSOR_WIN;
struct ALLEGRO_MOUSE_CURSOR_WIN
{
   HCURSOR hcursor;
};

ALLEGRO_MOUSE_CURSOR_WIN* _al_win_create_mouse_cursor(HWND wnd, ALLEGRO_BITMAP *sprite, int xfocus, int yfocus);
void _al_win_destroy_mouse_cursor(ALLEGRO_MOUSE_CURSOR_WIN *cursor);
void _al_win_set_mouse_hcursor(HCURSOR hcursor);
bool _al_win_set_system_mouse_cursor(ALLEGRO_DISPLAY *display, ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id);
bool _al_win_show_mouse_cursor(void);
bool _al_win_hide_mouse_cursor(void);
HCURSOR _al_win_system_cursor_to_hcursor(ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id);



#if defined ALLEGRO_CFG_D3D

int _al_d3d_get_num_display_modes(int format, int refresh_rate, int flags);
ALLEGRO_DISPLAY_MODE* _al_d3d_get_display_mode(int index, int format,
                     int refresh_rate, int flags, ALLEGRO_DISPLAY_MODE *mode);

ALLEGRO_DISPLAY_INTERFACE* _al_display_d3d_driver(void);
bool _al_d3d_init_display();
int _al_d3d_get_num_video_adapters(void);
void _al_d3d_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO *info);

#endif /*  defined ALLEGRO_CFG_D3D */

#if defined ALLEGRO_CFG_OPENGL

int _al_wgl_get_num_display_modes(int format, int refresh_rate, int flags);
ALLEGRO_DISPLAY_MODE* _al_wgl_get_display_mode(int index, int format,
                      int refresh_rate, int flags, ALLEGRO_DISPLAY_MODE *mode);

ALLEGRO_DISPLAY_INTERFACE *_al_display_wgl_driver(void);
bool _al_wgl_init_display();
int _al_wgl_get_num_video_adapters(void);
void _al_wgl_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO *info);

#endif /*  defined ALLEGRO_CFG_OPENGL */

#ifdef __cplusplus
}
#endif


#endif
