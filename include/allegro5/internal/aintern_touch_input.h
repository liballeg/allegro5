#ifndef __al_included_allegro5_aintern_touch_input_h
#define __al_included_allegro5_aintern_touch_input_h

#include "allegro5/internal/aintern_driver.h"
#include "allegro5/internal/aintern_events.h"

#ifdef __cplusplus
   extern "C" {
#endif


typedef struct A5O_TOUCH_INPUT_DRIVER
{
   int id;
   AL_METHOD(bool, init_touch_input, (void));
   AL_METHOD(void, exit_touch_input, (void));
   AL_METHOD(A5O_TOUCH_INPUT*, get_touch_input, (void));
   AL_METHOD(void, get_touch_input_state, (A5O_TOUCH_INPUT_STATE *ret_state));
   AL_METHOD(void, set_mouse_emulation_mode, (int mode));
   AL_METHOD(int, get_mouse_emulation_mode, (void));
} A5O_TOUCH_INPUT_DRIVER;


struct A5O_TOUCH_INPUT
{
   A5O_EVENT_SOURCE es;
   A5O_EVENT_SOURCE mouse_emulation_es;
   int mouse_emulation_mode;
};

extern _AL_DRIVER_INFO _al_touch_input_driver_list[];

#ifdef __cplusplus
   }
#endif

#endif
