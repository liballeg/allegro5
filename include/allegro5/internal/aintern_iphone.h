#ifndef A5O_INTERNAL_IPHONE
#define A5O_INTERNAL_IPHONE

#include "allegro5/allegro.h"
#include <allegro5/internal/aintern_system.h>
#include <allegro5/internal/aintern_display.h>

typedef struct A5O_SYSTEM_IPHONE {
    A5O_SYSTEM system;

    A5O_MUTEX *mutex;
    A5O_COND *cond;

   bool has_shutdown, wants_shutdown;
   int visuals_count;
   A5O_EXTRA_DISPLAY_SETTINGS **visuals;
} A5O_SYSTEM_IPHONE;

typedef struct A5O_DISPLAY_IPHONE_EXTRA A5O_DISPLAY_IPHONE_EXTRA;

typedef struct A5O_DISPLAY_IPHONE {
    A5O_DISPLAY display;
    A5O_DISPLAY_IPHONE_EXTRA *extra;
} A5O_DISPLAY_IPHONE;


void _al_iphone_init_path(void);
bool _al_iphone_add_view(A5O_DISPLAY *d);
void _al_iphone_make_view_current(A5O_DISPLAY *display);
void _al_iphone_flip_view(A5O_DISPLAY *display);
void _al_iphone_reset_framebuffer(A5O_DISPLAY *display);
void _al_iphone_recreate_framebuffer(A5O_DISPLAY *);
void _al_iphone_destroy_screen(A5O_DISPLAY *display);
A5O_SYSTEM_INTERFACE *_al_get_iphone_system_interface(void);
A5O_DISPLAY_INTERFACE *_al_get_iphone_display_interface(void);
A5O_KEYBOARD_DRIVER *_al_get_iphone_keyboard_driver(void);
A5O_MOUSE_DRIVER *_al_get_iphone_mouse_driver(void);
A5O_TOUCH_INPUT_DRIVER *_al_get_iphone_touch_input_driver(void);
A5O_JOYSTICK_DRIVER *_al_get_iphone_joystick_driver(void);
void _al_iphone_setup_opengl_view(A5O_DISPLAY *d, bool manage_backbuffer);
//void _al_iphone_generate_mouse_event(unsigned int type,
//   int x, int y, unsigned int button, A5O_DISPLAY *d);
//void _al_iphone_generate_touch_event(unsigned int type, double timestamp, float x, float y, bool primary, A5O_DISPLAY *d);
void _al_iphone_update_visuals(void);
void _al_iphone_accelerometer_control(int frequency);
void _al_iphone_generate_joystick_event(float x, float y, float z);
void _al_iphone_await_termination(void);
void _al_iphone_get_screen_size(int adapter, int *w, int *h);
int _al_iphone_get_num_video_adapters(void);
int _al_iphone_get_orientation();
void _al_iphone_translate_from_screen(A5O_DISPLAY *d, int *x, int *y);
void _al_iphone_translate_to_screen(A5O_DISPLAY *d, int *x, int *y);
void _al_iphone_clip(A5O_BITMAP const *bitmap, int x_1, int y_1, int x_2, int y_2);

void _al_iphone_touch_input_handle_begin(int id, double timestamp, float x, float y, bool primary, A5O_DISPLAY *disp);
void _al_iphone_touch_input_handle_end(int id, double timestamp, float x, float y, bool primary, A5O_DISPLAY *disp);
void _al_iphone_touch_input_handle_move(int id, double timestamp, float x, float y, bool primary, A5O_DISPLAY *disp);
void _al_iphone_touch_input_handle_cancel(int id, double timestamp, float x, float y, bool primary, A5O_DISPLAY *disp);

void _al_iphone_disconnect(A5O_DISPLAY *display);
void _al_iphone_add_clipboard_functions(A5O_DISPLAY_INTERFACE *vt);

#endif // A5O_INTERNAL_IPHONE
