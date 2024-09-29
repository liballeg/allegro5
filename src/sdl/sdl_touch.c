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
 *      SDL touch driver.
 *
 *      See LICENSE.txt for copyright information.
 */
#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/platform/allegro_internal_sdl.h"

A5O_DEBUG_CHANNEL("SDL")

typedef struct A5O_TOUCH_INPUT_SDL
{
   A5O_TOUCH_INPUT touch_input;
   A5O_TOUCH_INPUT_STATE state;
   A5O_DISPLAY *display;
   int touches;
} A5O_TOUCH_INPUT_SDL;

static A5O_TOUCH_INPUT_DRIVER *vt;
static A5O_TOUCH_INPUT_SDL *touch_input;
static A5O_MOUSE_STATE mouse_state;

static void generate_touch_input_event(unsigned int type, double timestamp,
   int id, float x, float y, float dx, float dy, bool primary,
   A5O_DISPLAY *disp)
{
   A5O_EVENT event;

   bool want_touch_event = _al_event_source_needs_to_generate_event(&touch_input->touch_input.es);
   bool want_mouse_emulation_event;

   if (touch_input->touch_input.mouse_emulation_mode == A5O_MOUSE_EMULATION_5_0_x) {
      want_mouse_emulation_event = _al_event_source_needs_to_generate_event(&touch_input->touch_input.mouse_emulation_es) && al_is_mouse_installed();
   }
   else {
      want_mouse_emulation_event = _al_event_source_needs_to_generate_event(&touch_input->touch_input.mouse_emulation_es) && primary && al_is_mouse_installed();
   }

   if (touch_input->touch_input.mouse_emulation_mode == A5O_MOUSE_EMULATION_NONE)
      want_mouse_emulation_event = false;
   else if (touch_input->touch_input.mouse_emulation_mode == A5O_MOUSE_EMULATION_INCLUSIVE)
      want_touch_event = al_is_mouse_installed() ? (want_touch_event && !primary) : want_touch_event;
   else if (touch_input->touch_input.mouse_emulation_mode == A5O_MOUSE_EMULATION_EXCLUSIVE)
      want_touch_event = al_is_mouse_installed() ? false : want_touch_event;


   if (!want_touch_event && !want_mouse_emulation_event)
      return;

   if (want_touch_event) {
      event.touch.type      = type;
      event.touch.display   = (A5O_DISPLAY*)disp;
      event.touch.timestamp = timestamp;
      event.touch.id        = id;
      event.touch.x         = x;
      event.touch.y         = y;
      event.touch.dx        = dx;
      event.touch.dy        = dy;
      event.touch.primary   = primary;

      _al_event_source_lock(&touch_input->touch_input.es);
      _al_event_source_emit_event(&touch_input->touch_input.es, &event);
      _al_event_source_unlock(&touch_input->touch_input.es);
   }

   if (touch_input->touch_input.mouse_emulation_mode != A5O_MOUSE_EMULATION_NONE) {
      mouse_state.x = (int)x;
      mouse_state.y = (int)y;
      if (type == A5O_EVENT_TOUCH_BEGIN)
         mouse_state.buttons++;
      else if (type == A5O_EVENT_TOUCH_END)
         mouse_state.buttons--;

      mouse_state.pressure = mouse_state.buttons ? 1.0 : 0.0; /* TODO */

      _al_event_source_lock(&touch_input->touch_input.mouse_emulation_es);
      if (want_mouse_emulation_event) {

         switch (type) {
            case A5O_EVENT_TOUCH_BEGIN: type = A5O_EVENT_MOUSE_BUTTON_DOWN; break;
            case A5O_EVENT_TOUCH_CANCEL:
            case A5O_EVENT_TOUCH_END:   type = A5O_EVENT_MOUSE_BUTTON_UP;   break;
            case A5O_EVENT_TOUCH_MOVE:  type = A5O_EVENT_MOUSE_AXES;        break;
         }

         event.mouse.type      = type;
         event.mouse.timestamp = timestamp;
         event.mouse.display   = (A5O_DISPLAY*)disp;
         event.mouse.x         = (int)x;
         event.mouse.y         = (int)y;
         event.mouse.dx        = (int)dx;
         event.mouse.dy        = (int)dy;
         event.mouse.dz        = 0;
         event.mouse.dw        = 0;
         if (touch_input->touch_input.mouse_emulation_mode != A5O_MOUSE_EMULATION_5_0_x) {
            event.mouse.button = 1;
         }
         else {
            event.mouse.button = id;
         }
         event.mouse.pressure  = mouse_state.pressure;

         if (touch_input->touch_input.mouse_emulation_mode != A5O_MOUSE_EMULATION_5_0_x) {
            al_set_mouse_xy(event.mouse.display, event.mouse.x, event.mouse.y);
         }

         _al_event_source_emit_event(&touch_input->touch_input.mouse_emulation_es, &event);
      }
      _al_event_source_unlock(&touch_input->touch_input.mouse_emulation_es);
   }
}

static int find_free_touch_state_index(void)
{
   int i;

   for (i = 0; i < A5O_TOUCH_INPUT_MAX_TOUCH_COUNT; i++)
      if (touch_input->state.touches[i].id < 0)
         return i;

   return -1;
}

static int find_touch_state_index_with_id(int id)
{
   int i;

   for (i = 0; i < A5O_TOUCH_INPUT_MAX_TOUCH_COUNT; i++)
      if (touch_input->state.touches[i].id == id)
         return i;

   return -1;
}

void _al_sdl_touch_input_event(SDL_Event *e)
{
   if (!touch_input)
      return;

   A5O_EVENT_TYPE type;

   A5O_DISPLAY *d = NULL;
   /* Use the first display as event source if we have any displays. */
   A5O_SYSTEM *s = al_get_system_driver();
   if (_al_vector_size(&s->displays) > 0) {
      void **v = (void **)_al_vector_ref(&s->displays, 0);
      d = *v;
   }
   if (!d) {
      return;
   }

   int touch_idx = find_touch_state_index_with_id(e->tfinger.fingerId);

   if (touch_idx < 0)
      touch_idx = find_free_touch_state_index();
   if (touch_idx < 0)
      return;

   // SDL2 touch events also handle indirect touch devices that aren't directly related to the display.
   // Allegro doesn't really have an equivalent in its API, so filter them out.
#if SDL_VERSION_ATLEAST(2,0,10)
   if (SDL_GetTouchDeviceType(e->tfinger.touchId) != SDL_TOUCH_DEVICE_DIRECT)
      return;
#endif

   touch_input->state.touches[touch_idx].x = e->tfinger.x * al_get_display_width(d);
   touch_input->state.touches[touch_idx].y = e->tfinger.y * al_get_display_height(d);
   touch_input->state.touches[touch_idx].dx = e->tfinger.dx * al_get_display_width(d);
   touch_input->state.touches[touch_idx].dy = e->tfinger.dy * al_get_display_height(d);

   if (e->type == SDL_FINGERDOWN) {
      type = A5O_EVENT_TOUCH_BEGIN;
      touch_input->state.touches[touch_idx].id = e->tfinger.fingerId;
      touch_input->state.touches[touch_idx].primary = (touch_input->touches == 0);
      touch_input->touches++;
   }
   else if (e->type == SDL_FINGERMOTION) {
      type = A5O_EVENT_TOUCH_MOVE;
   }
   else if (e->type == SDL_FINGERUP) {
      type = A5O_EVENT_TOUCH_END;
      touch_input->touches--;
      touch_input->state.touches[touch_idx].id = -1;
   } else {
      return;
   }

   generate_touch_input_event(type, e->tfinger.timestamp / 1000.0, touch_idx,
                              touch_input->state.touches[touch_idx].x,
                              touch_input->state.touches[touch_idx].y,
                              touch_input->state.touches[touch_idx].dx,
                              touch_input->state.touches[touch_idx].dy,
                              touch_input->state.touches[touch_idx].primary,
                              d);
}

static bool sdl_init_touch_input(void)
{
   touch_input = al_calloc(1, sizeof *touch_input);
   _al_event_source_init(&touch_input->touch_input.es);

   _al_event_source_init(&touch_input->touch_input.mouse_emulation_es);
   touch_input->touch_input.mouse_emulation_mode = A5O_MOUSE_EMULATION_TRANSPARENT;

   int i;
   for (i = 0; i < A5O_TOUCH_INPUT_MAX_TOUCH_COUNT; i++) {
      touch_input->state.touches[i].id = -1;
   }

   return true;
}

static void sdl_exit_touch_input(void)
{
}

static A5O_TOUCH_INPUT *sdl_get_touch_input(void)
{
   return &touch_input->touch_input;
}


static void sdl_get_touch_input_state(A5O_TOUCH_INPUT_STATE *ret_state)
{
   _al_event_source_lock(&touch_input->touch_input.es);
   *ret_state = touch_input->state;
   _al_event_source_unlock(&touch_input->touch_input.es);
}

static void touch_input_handle_cancel(int index, double timestamp, float x, float y, bool primary, A5O_DISPLAY *disp)
{
   A5O_TOUCH_STATE* state = touch_input->state.touches + index;
   (void)primary;

   if (NULL == state)
      return;

   _al_event_source_lock(&touch_input->touch_input.es);
   state->dx = x - state->x;
   state->dy = y - state->y;
   state->x  = x;
   state->y  = y;
   _al_event_source_unlock(&touch_input->touch_input.es);

   generate_touch_input_event(A5O_EVENT_TOUCH_CANCEL, timestamp,
      state->id, state->x, state->y, state->dx, state->dy, state->primary, disp);

   _al_event_source_lock(&touch_input->touch_input.es);
   state->id = -1;
   _al_event_source_unlock(&touch_input->touch_input.es);
}

static void sdl_set_mouse_emulation_mode(int mode)
{
   if (touch_input->touch_input.mouse_emulation_mode != mode) {

      int i;

      for (i = 0; i < A5O_TOUCH_INPUT_MAX_TOUCH_COUNT; ++i) {

         A5O_TOUCH_STATE* touch = touch_input->state.touches + i;

         if (touch->id > 0) {
            touch_input_handle_cancel(i, al_get_time(),
               touch->x, touch->y, touch->primary, touch->display);
         }
      }

      touch_input->touch_input.mouse_emulation_mode = mode;
   }
}

A5O_TOUCH_INPUT_DRIVER *_al_sdl_touch_input_driver(void)
{
   if (vt)
      return vt;

   vt = al_calloc(1, sizeof *vt);
   vt->init_touch_input = sdl_init_touch_input;
   vt->exit_touch_input = sdl_exit_touch_input;
   vt->get_touch_input = sdl_get_touch_input;
   vt->get_touch_input_state = sdl_get_touch_input_state;
   vt->set_mouse_emulation_mode = sdl_set_mouse_emulation_mode;

   return vt;
}
