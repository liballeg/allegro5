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

ALLEGRO_DEBUG_CHANNEL("SDL")

typedef struct ALLEGRO_TOUCH_INPUT_SDL
{
   ALLEGRO_TOUCH_INPUT touch_input;
   ALLEGRO_TOUCH_INPUT_STATE state;
   ALLEGRO_DISPLAY *display;
   int touches;
} ALLEGRO_TOUCH_INPUT_SDL;

static ALLEGRO_TOUCH_INPUT_DRIVER *vt;
static ALLEGRO_TOUCH_INPUT_SDL *touch_input;
static ALLEGRO_MOUSE_STATE mouse_state;

static void generate_touch_input_event(unsigned int type, double timestamp,
   int id, float x, float y, float dx, float dy, bool primary,
   ALLEGRO_DISPLAY *disp)
{
   ALLEGRO_EVENT event;

   bool want_touch_event = _al_event_source_needs_to_generate_event(&touch_input->touch_input.es);
   bool want_mouse_emulation_event;

   if (touch_input->touch_input.mouse_emulation_mode == ALLEGRO_MOUSE_EMULATION_5_0_x) {
      want_mouse_emulation_event = _al_event_source_needs_to_generate_event(&touch_input->touch_input.mouse_emulation_es) && al_is_mouse_installed();
   }
   else {
      want_mouse_emulation_event = _al_event_source_needs_to_generate_event(&touch_input->touch_input.mouse_emulation_es) && primary && al_is_mouse_installed();
   }

   if (touch_input->touch_input.mouse_emulation_mode == ALLEGRO_MOUSE_EMULATION_NONE)
      want_mouse_emulation_event = false;
   else if (touch_input->touch_input.mouse_emulation_mode == ALLEGRO_MOUSE_EMULATION_INCLUSIVE)
      want_touch_event = al_is_mouse_installed() ? (want_touch_event && !primary) : want_touch_event;
   else if (touch_input->touch_input.mouse_emulation_mode == ALLEGRO_MOUSE_EMULATION_EXCLUSIVE)
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

      _al_event_source_lock(&touch_input->touch_input.es);
      _al_event_source_emit_event(&touch_input->touch_input.es, &event);
      _al_event_source_unlock(&touch_input->touch_input.es);
   }

   if (touch_input->touch_input.mouse_emulation_mode != ALLEGRO_MOUSE_EMULATION_NONE) {
      mouse_state.x = (int)x;
      mouse_state.y = (int)y;
      if (type == ALLEGRO_EVENT_TOUCH_BEGIN)
         mouse_state.buttons++;
      else if (type == ALLEGRO_EVENT_TOUCH_END)
         mouse_state.buttons--;

      mouse_state.pressure = mouse_state.buttons ? 1.0 : 0.0; /* TODO */

      _al_event_source_lock(&touch_input->touch_input.mouse_emulation_es);
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
         if (touch_input->touch_input.mouse_emulation_mode != ALLEGRO_MOUSE_EMULATION_5_0_x) {
            event.mouse.button = 1;
         }
         else {
            event.mouse.button = id;
         }
         event.mouse.pressure  = mouse_state.pressure;

         if (touch_input->touch_input.mouse_emulation_mode != ALLEGRO_MOUSE_EMULATION_5_0_x) {
            al_set_mouse_xy(event.mouse.display, event.mouse.x, event.mouse.y);
         }

         _al_event_source_emit_event(&touch_input->touch_input.mouse_emulation_es, &event);
      }
      _al_event_source_unlock(&touch_input->touch_input.mouse_emulation_es);
   }
}

void _al_sdl_touch_input_event(SDL_Event *e)
{
   if (!touch_input)
      return;

   ALLEGRO_EVENT_TYPE type;

   ALLEGRO_DISPLAY *d = NULL;
   /* Use the first display as event source if we have any displays. */
   ALLEGRO_SYSTEM *s = al_get_system_driver();
   if (_al_vector_size(&s->displays) > 0) {
      void **v = (void **)_al_vector_ref(&s->displays, 0);
      d = *v;
   }
   if (!d) {
      return;
   }

   int touchId = e->tfinger.fingerId;

   // SDL2 only returns absolute positions of touches on the input device. This means we have to fake them
   // in order to make them related to the display, which is what Allegro returns in its API.
   // This is likely to break in lots of cases, but should work well with non-rotated fullscreen or Wayland windows.
   // NOTE: SDL 2.0.10 is going to have SDL_GetTouchDeviceType API which may be somewhat helpful here.

   touch_input->state.touches[touchId].x = e->tfinger.x * al_get_display_width(d);
   touch_input->state.touches[touchId].y = e->tfinger.y * al_get_display_height(d);
   touch_input->state.touches[touchId].dx = e->tfinger.dx * al_get_display_width(d);
   touch_input->state.touches[touchId].dy = e->tfinger.dy * al_get_display_height(d);

   if (e->type == SDL_FINGERDOWN) {
      type = ALLEGRO_EVENT_TOUCH_BEGIN;
      touch_input->state.touches[touchId].id = touchId;
      touch_input->state.touches[touchId].primary = (touch_input->touches == 0);
      touch_input->touches++;
   }
   else if (e->type == SDL_FINGERMOTION) {
      type = ALLEGRO_EVENT_TOUCH_MOVE;
   }
   else if (e->type == SDL_FINGERUP) {
      type = ALLEGRO_EVENT_TOUCH_END;
      touch_input->touches--;
      touch_input->state.touches[touchId].id = -1;
   } else {
      return;
   }

   generate_touch_input_event(type, e->tfinger.timestamp / 1000.0, touchId,
                              touch_input->state.touches[touchId].x,
                              touch_input->state.touches[touchId].y,
                              touch_input->state.touches[touchId].dx,
                              touch_input->state.touches[touchId].dy,
                              touch_input->state.touches[touchId].primary,
                              d);
}

static bool sdl_init_touch_input(void)
{
   touch_input = al_calloc(1, sizeof *touch_input);
   _al_event_source_init(&touch_input->touch_input.es);

   _al_event_source_init(&touch_input->touch_input.mouse_emulation_es);
   touch_input->touch_input.mouse_emulation_mode = ALLEGRO_MOUSE_EMULATION_TRANSPARENT;

   int i;
   for (i = 0; i < ALLEGRO_TOUCH_INPUT_MAX_TOUCH_COUNT; i++) {
      touch_input->state.touches[i].id = -1;
   }

   return true;
}

static void sdl_exit_touch_input(void)
{
}

static ALLEGRO_TOUCH_INPUT *sdl_get_touch_input(void)
{
   return &touch_input->touch_input;
}


static void sdl_get_touch_input_state(ALLEGRO_TOUCH_INPUT_STATE *ret_state)
{
   _al_event_source_lock(&touch_input->touch_input.es);
   *ret_state = touch_input->state;
   _al_event_source_unlock(&touch_input->touch_input.es);
}

static void touch_input_handle_cancel(int index, double timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY *disp)
{
   ALLEGRO_TOUCH_STATE* state = touch_input->state.touches + index;
   (void)primary;

   if (NULL == state)
      return;

   _al_event_source_lock(&touch_input->touch_input.es);
   state->dx = x - state->x;
   state->dy = y - state->y;
   state->x  = x;
   state->y  = y;
   _al_event_source_unlock(&touch_input->touch_input.es);

   generate_touch_input_event(ALLEGRO_EVENT_TOUCH_CANCEL, timestamp,
      state->id, state->x, state->y, state->dx, state->dy, state->primary, disp);

   _al_event_source_lock(&touch_input->touch_input.es);
   state->id = -1;
   _al_event_source_unlock(&touch_input->touch_input.es);
}

static void sdl_set_mouse_emulation_mode(int mode)
{
   if (touch_input->touch_input.mouse_emulation_mode != mode) {

      int i;

      for (i = 0; i < ALLEGRO_TOUCH_INPUT_MAX_TOUCH_COUNT; ++i) {

         ALLEGRO_TOUCH_STATE* touch = touch_input->state.touches + i;

         if (touch->id > 0) {
            touch_input_handle_cancel(i, al_get_time(),
               touch->x, touch->y, touch->primary, touch->display);
         }
      }

      touch_input->touch_input.mouse_emulation_mode = mode;
   }
}

ALLEGRO_TOUCH_INPUT_DRIVER *_al_sdl_touch_input_driver(void)
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
