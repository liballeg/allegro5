#ifndef		__DEMO_GAMEPAD_H__
#define		__DEMO_GAMEPAD_H__

#include <allegro5/allegro.h>
#include "vcontroller.h"

VCONTROLLER *create_gamepad_controller(const char *config_path);
void gamepad_event(A5O_EVENT *event);
bool gamepad_button(void);

#endif				/* __DEMO_GAMEPAD_H__ */
