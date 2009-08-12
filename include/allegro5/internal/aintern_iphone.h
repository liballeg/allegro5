#include "allegro5/allegro5.h"
#include <allegro5/internal/aintern_system.h>
#include <allegro5/internal/aintern_display.h>

typedef struct ALLEGRO_SYSTEM_IPHONE {
    ALLEGRO_SYSTEM system;
    
    ALLEGRO_MUTEX *mutex;
    ALLEGRO_COND *cond;
    
} ALLEGRO_SYSTEM_IPHONE;

typedef struct ALLEGRO_DISPLAY_IPHONE {
    ALLEGRO_DISPLAY display;
} ALLEGRO_DISPLAY_IPHONE;

void _al_iphone_init_path(void);
void _al_iphone_add_view(ALLEGRO_DISPLAY *d);
void _al_iphone_make_view_current(void);
void _al_iphone_flip_view(void);
void _al_iphone_reset_framebuffer(void);
ALLEGRO_SYSTEM_INTERFACE *_al_get_iphone_system_interface(void);
ALLEGRO_DISPLAY_INTERFACE *_al_get_iphone_display_interface(void);
ALLEGRO_PATH *_al_iphone_get_path(int id);
ALLEGRO_KEYBOARD_DRIVER *_al_get_iphone_keyboard_driver(void);
ALLEGRO_MOUSE_DRIVER *_al_get_iphone_mouse_driver(void);
void _al_iphone_setup_opengl_view(ALLEGRO_DISPLAY *d);
void _al_iphone_generate_mouse_event(unsigned int type,
   int x, int y, unsigned int button, ALLEGRO_DISPLAY *d);
