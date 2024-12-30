/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      iPhone family device touch input driver.
 *
 *      By Michał Cichoń.
 *
 *      See readme.txt for copyright information.
 */
#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_touch_input.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_iphone.h"


static ALLEGRO_TOUCH_INPUT_STATE touch_input_state;
static ALLEGRO_MOUSE_STATE mouse_state;
static ALLEGRO_TOUCH_INPUT touch_input;
static bool installed = false;

static void generate_touch_input_event(unsigned int type, double timestamp, int id, float x, float y, float dx, float dy, bool primary, ALLEGRO_DISPLAY *disp)
{
   ALLEGRO_EVENT event;

   bool want_touch_event           = _al_event_source_needs_to_generate_event(&touch_input.es);
   bool want_mouse_emulation_event;

   if (touch_input.mouse_emulation_mode == ALLEGRO_MOUSE_EMULATION_5_0_x) {
      want_mouse_emulation_event = _al_event_source_needs_to_generate_event(&touch_input.mouse_emulation_es) && al_is_mouse_installed();
   }
   else {
      want_mouse_emulation_event = _al_event_source_needs_to_generate_event(&touch_input.mouse_emulation_es) && primary && al_is_mouse_installed();
   }

   if (touch_input.mouse_emulation_mode == ALLEGRO_MOUSE_EMULATION_NONE)
      want_mouse_emulation_event = false;
   else if (touch_input.mouse_emulation_mode == ALLEGRO_MOUSE_EMULATION_INCLUSIVE)
      want_touch_event = al_is_mouse_installed() ? (want_touch_event && !primary) : want_touch_event;
   else if (touch_input.mouse_emulation_mode == ALLEGRO_MOUSE_EMULATION_EXCLUSIVE)
      want_touch_event = al_is_mouse_installed() ? false : want_touch_event;


   if (!want_touch_event && !want_mouse_emulation_event)
      return;

   if (want_touch_event) {

      event.touch.type      = type;
      event.touch.display   = (ALLEGRO_DISPLAY*)disp;
      event.touch.timestamp = timestamp;
      event.touch.id        = id;
      event.touch.x         = x;
      event.touch.y         = y;
      event.touch.dx        = dx;
      event.touch.dy        = dy;
      event.touch.primary   = primary;

      _al_event_source_lock(&touch_input.es);
      _al_event_source_emit_event(&touch_input.es, &event);
      _al_event_source_unlock(&touch_input.es);
   }

   if (touch_input.mouse_emulation_mode != ALLEGRO_MOUSE_EMULATION_NONE) {
      _al_event_source_lock(&touch_input.mouse_emulation_es);
      if (want_mouse_emulation_event) {

         switch (type) {
            case ALLEGRO_EVENT_TOUCH_BEGIN: type = ALLEGRO_EVENT_MOUSE_BUTTON_DOWN; break;
            case ALLEGRO_EVENT_TOUCH_CANCEL:
            case ALLEGRO_EVENT_TOUCH_END:   type = ALLEGRO_EVENT_MOUSE_BUTTON_UP;   break;
            case ALLEGRO_EVENT_TOUCH_MOVE:  type = ALLEGRO_EVENT_MOUSE_AXES;        break;
         }

         event.mouse.type      = type;
         event.mouse.timestamp = timestamp;
         event.mouse.display   = (ALLEGRO_DISPLAY*)disp;
         event.mouse.x         = (int)x;
         event.mouse.y         = (int)y;
         event.mouse.dx        = (int)dx;
         event.mouse.dy        = (int)dy;
         event.mouse.dz        = 0;
         event.mouse.dw        = 0;
         if (touch_input.mouse_emulation_mode != ALLEGRO_MOUSE_EMULATION_5_0_x) {
            event.mouse.button = 1;
         }
         else {
            event.mouse.button = id;
         }
         event.mouse.pressure  = 1.0;

         if (touch_input.mouse_emulation_mode != ALLEGRO_MOUSE_EMULATION_5_0_x) {
            al_set_mouse_xy(event.mouse.display, event.mouse.x, event.mouse.y);
         }

         _al_event_source_emit_event(&touch_input.mouse_emulation_es, &event);
      }

      mouse_state.x = (int)x;
      mouse_state.y = (int)y;
      if (type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN)
         mouse_state.buttons |= id;
      else if (type == ALLEGRO_EVENT_MOUSE_BUTTON_UP)
         mouse_state.buttons &= ~id;

      _al_event_source_unlock(&touch_input.mouse_emulation_es);
   }
}


static bool init_touch_input(void)
{
   if (installed)
      return false;

   memset(&touch_input_state, 0, sizeof(touch_input_state));
   memset(&mouse_state, 0, sizeof(mouse_state));

   _al_event_source_init(&touch_input.es);
   _al_event_source_init(&touch_input.mouse_emulation_es);
   touch_input.mouse_emulation_mode = ALLEGRO_MOUSE_EMULATION_TRANSPARENT;

   installed = true;

   return true;
}


static void exit_touch_input(void)
{
   if (!installed)
      return;

   memset(&touch_input_state, 0, sizeof(touch_input_state));
   memset(&mouse_state, 0, sizeof(mouse_state));

   _al_event_source_free(&touch_input.es);
   _al_event_source_free(&touch_input.mouse_emulation_es);

   installed = false;
}


static ALLEGRO_TOUCH_INPUT* get_touch_input(void)
{
   return &touch_input;
}


static void get_touch_input_state(ALLEGRO_TOUCH_INPUT_STATE *ret_state)
{
   _al_event_source_lock(&touch_input.es);
   *ret_state = touch_input_state;
   _al_event_source_unlock(&touch_input.es);
}


static void set_mouse_emulation_mode(int mode)
{
   if (touch_input.mouse_emulation_mode != mode) {

      int i;

      for (i = 0; i < ALLEGRO_TOUCH_INPUT_MAX_TOUCH_COUNT; ++i) {

         ALLEGRO_TOUCH_STATE* touch = touch_input_state.touches + i;

         if (touch->id > 0) {
            _al_iphone_touch_input_handle_cancel(touch->id, al_get_time(),
               touch->x, touch->y, touch->primary, touch->display);
         }
      }

      touch_input.mouse_emulation_mode = mode;
   }
}


static ALLEGRO_TOUCH_STATE* find_free_touch_state()
{
   int i;

   for (i = 0; i < ALLEGRO_TOUCH_INPUT_MAX_TOUCH_COUNT; ++i)
      if (touch_input_state.touches[i].id <= 0)
         return touch_input_state.touches + i;

   return NULL;
}


static ALLEGRO_TOUCH_STATE* find_touch_state_with_id(int id)
{
   int i;

   for (i = 0; i < ALLEGRO_TOUCH_INPUT_MAX_TOUCH_COUNT; ++i)
      if (touch_input_state.touches[i].id == id)
         return touch_input_state.touches + i;

   return NULL;
}



void _al_iphone_touch_input_handle_begin(int id, double timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY *disp)
{
   ALLEGRO_TOUCH_STATE* state = find_free_touch_state();
   (void)primary;

   if (NULL == state)
      return;

   _al_event_source_lock(&touch_input.es);
   state->id      = id;
   state->x       = x;
   state->y       = y;
   state->dx      = 0.0f;
   state->dy      = 0.0f;
   state->primary = primary;
   state->display = disp;
   _al_event_source_unlock(&touch_input.es);

   generate_touch_input_event(ALLEGRO_EVENT_TOUCH_BEGIN, timestamp,
      state->id, state->x, state->y, state->dx, state->dy, state->primary, disp);
}


void _al_iphone_touch_input_handle_end(int id, double timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY *disp)
{
   ALLEGRO_TOUCH_STATE* state = find_touch_state_with_id(id);
   (void)primary;

   if (NULL == state)
      return;

   _al_event_source_lock(&touch_input.es);
   state->dx      = x - state->x;
   state->dy      = y - state->y;
   state->x       = x;
   state->y       = y;
   _al_event_source_unlock(&touch_input.es);

   generate_touch_input_event(ALLEGRO_EVENT_TOUCH_END, timestamp,
      state->id, state->x, state->y, state->dx, state->dy, state->primary, disp);

   _al_event_source_lock(&touch_input.es);
   memset(state, 0, sizeof(ALLEGRO_TOUCH_STATE));
   _al_event_source_unlock(&touch_input.es);
}


void _al_iphone_touch_input_handle_move(int id, double timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY *disp)
{
   ALLEGRO_TOUCH_STATE* state = find_touch_state_with_id(id);
   (void)primary;

   if (NULL == state)
      return;

   _al_event_source_lock(&touch_input.es);
   state->dx      = x - state->x;
   state->dy      = y - state->y;
   state->x       = x;
   state->y       = y;
   _al_event_source_unlock(&touch_input.es);

   generate_touch_input_event(ALLEGRO_EVENT_TOUCH_MOVE, timestamp,
      state->id, state->x, state->y, state->dx, state->dy, state->primary, disp);
}


void _al_iphone_touch_input_handle_cancel(int id, double timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY *disp)
{
   ALLEGRO_TOUCH_STATE* state = find_touch_state_with_id(id);
   (void)primary;

   if (NULL == state)
      return;

   _al_event_source_lock(&touch_input.es);
   state->dx      = x - state->x;
   state->dy      = y - state->y;
   state->x       = x;
   state->y       = y;
   _al_event_source_unlock(&touch_input.es);

   generate_touch_input_event(ALLEGRO_EVENT_TOUCH_CANCEL, timestamp,
      state->id, state->x, state->y, state->dx, state->dy, state->primary, disp);

   _al_event_source_lock(&touch_input.es);
   memset(state, 0, sizeof(ALLEGRO_TOUCH_STATE));
   _al_event_source_unlock(&touch_input.es);
}


/* the driver vtable */
#define TOUCH_INPUT_IPHONE AL_ID('I','T','I','D')

static ALLEGRO_TOUCH_INPUT_DRIVER touch_input_driver =
{
   TOUCH_INPUT_IPHONE,
   init_touch_input,
   exit_touch_input,
   get_touch_input,
   get_touch_input_state,
   set_mouse_emulation_mode,
   NULL
};

ALLEGRO_TOUCH_INPUT_DRIVER *_al_get_iphone_touch_input_driver(void)
{
    return &touch_input_driver;
}

void imouse_get_state(ALLEGRO_MOUSE_STATE *ret_state)
{
   _al_event_source_lock(&touch_input.es);
   *ret_state = mouse_state;
   _al_event_source_unlock(&touch_input.es);
}

