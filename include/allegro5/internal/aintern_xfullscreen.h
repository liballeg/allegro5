#ifndef __al_included_allegro5_aintern_xfullscreen_h
#define __al_included_allegro5_aintern_xfullscreen_h

/* fullscreen and multi monitor stuff */

typedef struct _A5O_XGLX_MMON_INTERFACE _A5O_XGLX_MMON_INTERFACE;

struct _A5O_XGLX_MMON_INTERFACE {
    int (*get_num_display_modes)(A5O_SYSTEM_XGLX *s, int adapter);
    A5O_DISPLAY_MODE *(*get_display_mode)(A5O_SYSTEM_XGLX *s, int, int, A5O_DISPLAY_MODE*);
    bool (*set_mode)(A5O_SYSTEM_XGLX *, A5O_DISPLAY_XGLX *, int, int, int, int);
    void (*store_mode)(A5O_SYSTEM_XGLX *);
    void (*restore_mode)(A5O_SYSTEM_XGLX *, int);
    void (*get_display_offset)(A5O_SYSTEM_XGLX *, int, int *, int *);
    int (*get_num_adapters)(A5O_SYSTEM_XGLX *);
    bool (*get_monitor_info)(A5O_SYSTEM_XGLX *, int, A5O_MONITOR_INFO *);
    int (*get_default_adapter)(A5O_SYSTEM_XGLX *);
    int (*get_adapter)(A5O_SYSTEM_XGLX *, A5O_DISPLAY_XGLX *);
    int (*get_xscreen)(A5O_SYSTEM_XGLX *, int);
    void (*post_setup)(A5O_SYSTEM_XGLX *, A5O_DISPLAY_XGLX *);
    void (*handle_xevent)(A5O_SYSTEM_XGLX *, A5O_DISPLAY_XGLX *, XEvent *e);
};

extern _A5O_XGLX_MMON_INTERFACE _al_xglx_mmon_interface;

int _al_xsys_mheadx_get_default_adapter(A5O_SYSTEM_XGLX *s);
int _al_xsys_mheadx_get_xscreen(A5O_SYSTEM_XGLX *s, int adapter);
void _al_xsys_get_active_window_center(A5O_SYSTEM_XGLX *s, int *x, int *y);

void _al_xsys_mmon_exit(A5O_SYSTEM_XGLX *s);

int _al_xglx_get_num_display_modes(A5O_SYSTEM_XGLX *s, int adapter);
A5O_DISPLAY_MODE *_al_xglx_get_display_mode(
   A5O_SYSTEM_XGLX *s, int adapter, int index, A5O_DISPLAY_MODE *mode);
bool _al_xglx_fullscreen_set_mode(A5O_SYSTEM_XGLX *s, A5O_DISPLAY_XGLX *d, int w, int h,
   int format, int refresh_rate);
void _al_xglx_store_video_mode(A5O_SYSTEM_XGLX *s);
void _al_xglx_restore_video_mode(A5O_SYSTEM_XGLX *s, int adapter);
void _al_xglx_fullscreen_to_display(A5O_SYSTEM_XGLX *s,
   A5O_DISPLAY_XGLX *d);
void _al_xglx_set_fullscreen_window(A5O_DISPLAY *display, int value);
void _al_xglx_get_display_offset(A5O_SYSTEM_XGLX *s, int adapter, int *x, int *y);

int _al_xglx_fullscreen_select_mode(A5O_SYSTEM_XGLX *s, int adapter, int w, int h, int format, int refresh_rate);

bool _al_xglx_get_monitor_info(A5O_SYSTEM_XGLX *s, int adapter, A5O_MONITOR_INFO *info);
int _al_xglx_get_num_video_adapters(A5O_SYSTEM_XGLX *s);

int _al_xglx_get_default_adapter(A5O_SYSTEM_XGLX *s);
int _al_xglx_get_xscreen(A5O_SYSTEM_XGLX *s, int adapter);

void _al_xglx_set_above(A5O_DISPLAY *display, int value);

int _al_xglx_get_adapter(A5O_SYSTEM_XGLX *s, A5O_DISPLAY_XGLX *d, bool recalc);

void _al_xglx_handle_mmon_event(A5O_SYSTEM_XGLX *s, A5O_DISPLAY_XGLX *d, XEvent *e);

#ifdef A5O_XWINDOWS_WITH_XRANDR
void _al_xsys_xrandr_init(A5O_SYSTEM_XGLX *s);
void _al_xsys_xrandr_exit(A5O_SYSTEM_XGLX *s);
#endif /* A5O_XWINDOWS_WITH_XRANDR */

#endif

/* vim: set sts=3 sw=3 et: */
