#ifndef		__DEMO_GAMEPAD_H__
#define		__DEMO_GAMEPAD_H__

#include <allegro5/allegro.h>
#include "vcontroller.h"

VCONTROLLER *create_gamepad_controller(const char *config_path);

#endif				/* __DEMO_GAMEPAD_H__ */
