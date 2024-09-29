#include "SDL.h"

#include "allegro5/altime.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/internal/aintern_mouse.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_system.h"

typedef struct
{
   A5O_SYSTEM system;
   A5O_MUTEX *mutex;
   #ifdef __EMSCRIPTEN__
      double timer_time;
   #endif
} A5O_SYSTEM_SDL;

typedef struct A5O_DISPLAY_SDL
{
   A5O_DISPLAY display; /* This must be the first member. */

   int x, y;
   SDL_Window *window;
   SDL_GLContext context;
} A5O_DISPLAY_SDL;

A5O_SYSTEM_INTERFACE *_al_sdl_system_driver(void);
A5O_DISPLAY_INTERFACE *_al_sdl_display_driver(void);
A5O_KEYBOARD_DRIVER *_al_sdl_keyboard_driver(void);
A5O_MOUSE_DRIVER *_al_sdl_mouse_driver(void);
A5O_TOUCH_INPUT_DRIVER *_al_sdl_touch_input_driver(void);
A5O_JOYSTICK_DRIVER *_al_sdl_joystick_driver(void);
A5O_BITMAP_INTERFACE *_al_sdl_bitmap_driver(void);

void _al_sdl_keyboard_event(SDL_Event *e);
void _al_sdl_mouse_event(SDL_Event *e);
void _al_sdl_touch_input_event(SDL_Event *e);
void _al_sdl_display_event(SDL_Event *e);
void _al_sdl_joystick_event(SDL_Event *e);

int _al_sdl_get_allegro_pixel_format(int sdl_format);
int _al_sdl_get_sdl_pixel_format(int allegro_format);

A5O_DISPLAY *_al_sdl_find_display(uint32_t window_id);
float _al_sdl_get_display_pixel_ratio(A5O_DISPLAY *display);

void _al_sdl_event_hack(void);

double _al_sdl_get_time(void);
void _al_sdl_rest(double seconds);
void _al_sdl_init_timeout(A5O_TIMEOUT *timeout, double seconds);

typedef struct A5O_MOUSE_CURSOR_SDL A5O_MOUSE_CURSOR_SDL;
struct A5O_MOUSE_CURSOR_SDL
{
   SDL_Cursor *cursor;
};
