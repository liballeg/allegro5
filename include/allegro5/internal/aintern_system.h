#ifndef ALLEGRO_INTERNAL_SYSTEM_NEW_H
#define ALLEGRO_INTERNAL_SYSTEM_NEW_H

#include "allegro5/system_new.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_mouse.h"
#include "allegro5/internal/aintern_vector.h"

typedef struct ALLEGRO_SYSTEM_INTERFACE ALLEGRO_SYSTEM_INTERFACE;

struct ALLEGRO_SYSTEM_INTERFACE
{
   int id;
   ALLEGRO_SYSTEM *(*initialize)(int flags);
   ALLEGRO_DISPLAY_INTERFACE *(*get_display_driver)(void);
   ALLEGRO_KEYBOARD_DRIVER *(*get_keyboard_driver)(void);
   ALLEGRO_MOUSE_DRIVER *(*get_mouse_driver)(void);
   ALLEGRO_JOYSTICK_DRIVER *(*get_joystick_driver)(void);
   int (*get_num_display_modes)(void);
   ALLEGRO_DISPLAY_MODE *(*get_display_mode)(int index, ALLEGRO_DISPLAY_MODE *mode);
   void (*shutdown_system)(void);
   int (*get_num_video_adapters)(void);
   void (*get_monitor_info)(int adapter, ALLEGRO_MONITOR_INFO *info);
   bool (*get_cursor_position)(int *ret_x, int *ret_y);
   AL_CONST char *(*get_path)(uint32_t id, char *path, size_t size);
};

struct ALLEGRO_SYSTEM
{
   ALLEGRO_SYSTEM_INTERFACE *vt;
   _AL_VECTOR displays; /* Keep a list of all displays attached to us. */
   ALLEGRO_CONFIG *config;
};


AL_FUNC(void, _al_register_system_interfaces, (void));
AL_VAR(_AL_VECTOR, _al_system_interfaces);

#endif
