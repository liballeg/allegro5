#include "global.h"

bool mouse_button_down(int b);
bool mouse_button_pressed(int b);
int mouse_x(void);
int mouse_y(void);
/* 'mouse_event' conflicts with winuser.h */
void mouse_handle_event(A5O_EVENT *event);
void mouse_tick(void);
