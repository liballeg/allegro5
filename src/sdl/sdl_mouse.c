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
 *      SDL mouse driver.
 *
 *      See LICENSE.txt for copyright information.
 */
#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/platform/allegro_internal_sdl.h"

ALLEGRO_DEBUG_CHANNEL("SDL")

typedef struct ALLEGRO_MOUSE_SDL
{
   ALLEGRO_MOUSE mouse;
   ALLEGRO_MOUSE_FLOAT_STATE state;
   ALLEGRO_DISPLAY *display;
} ALLEGRO_MOUSE_SDL;

static ALLEGRO_MOUSE_DRIVER *vt;
static ALLEGRO_MOUSE_SDL *mouse;

static ALLEGRO_DISPLAY *find_display(uint32_t window_id)
{
   ALLEGRO_DISPLAY *d = _al_sdl_find_display(window_id);
   if (d) {
      return d;
   }
   else {
      // if there's only one display, we can assume that all
      // events refer to its coordinate system
      ALLEGRO_SYSTEM *s = al_get_system_driver();
      if (_al_vector_size(&s->displays) == 1) {
         void **v = (void **)_al_vector_ref(&s->displays, 0);
         ALLEGRO_DISPLAY_SDL *d = *v;
         return &d->display;
      }
   }
   return NULL;
}

void _al_sdl_mouse_event(SDL_Event *e)
{
   if (!mouse)
      return;

   ALLEGRO_EVENT_SOURCE *es = &mouse->mouse.es;
   _al_event_source_lock(es);
   ALLEGRO_EVENT event;
   memset(&event, 0, sizeof event);

   event.mouse.timestamp = al_get_time();

   ALLEGRO_DISPLAY *d = NULL;

   if (e->type == SDL_WINDOWEVENT) {
      d = find_display(e->window.windowID);
      float ratio = _al_sdl_get_display_pixel_ratio(d);
      if (e->window.event == SDL_WINDOWEVENT_ENTER) {
         event.mouse_float.type = ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY_FLOAT;
         SDL_GetMouseState(&event.mouse_float.x, &event.mouse_float.y);
         event.mouse_float.x *= ratio;
         event.mouse_float.y *= ratio;
      }
      else {
         event.mouse_float.type = ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY_FLOAT;
         event.mouse_float.x = mouse->state.x;
         event.mouse_float.y = mouse->state.y;
         event.mouse_float.z = mouse->state.z;
         event.mouse_float.w = mouse->state.w;
      }
      mouse->display = e->window.event == SDL_WINDOWEVENT_ENTER ? d : NULL;
   }
   else if (e->type == SDL_MOUSEMOTION) {
      if (e->motion.which == SDL_TOUCH_MOUSEID) {
         _al_event_source_unlock(es);
         return;
      }
      d = find_display(e->motion.windowID);
      float ratio = d ? _al_sdl_get_display_pixel_ratio(d) : 1.0;
      event.mouse_float.type = ALLEGRO_EVENT_MOUSE_AXES_FLOAT;
      event.mouse_float.x = e->motion.x * ratio;
      event.mouse_float.y = e->motion.y * ratio;
      event.mouse_float.z = mouse->state.z;
      event.mouse_float.w = mouse->state.w;
      event.mouse_float.dx = e->motion.xrel * ratio;
      event.mouse_float.dy = e->motion.yrel * ratio;
      event.mouse_float.dz = 0;
      event.mouse_float.dw = 0;
      mouse->state.x = e->motion.x * ratio;
      mouse->state.y = e->motion.y * ratio;
   }
   else if (e->type == SDL_MOUSEWHEEL) {
      if (e->wheel.which == SDL_TOUCH_MOUSEID) {
         _al_event_source_unlock(es);
         return;
      }
      d = find_display(e->wheel.windowID);
      event.mouse_float.type = ALLEGRO_EVENT_MOUSE_AXES_FLOAT;
      mouse->state.z += al_get_mouse_wheel_precision() * e->wheel.y;
      mouse->state.w += al_get_mouse_wheel_precision() * e->wheel.x;
      event.mouse_float.x = mouse->state.x;
      event.mouse_float.y = mouse->state.y;
      event.mouse_float.z = mouse->state.z;
      event.mouse_float.w = mouse->state.w;
      event.mouse_float.dx = 0;
      event.mouse_float.dy = 0;
      event.mouse_float.dz = al_get_mouse_wheel_precision() * e->wheel.y;
      event.mouse_float.dw = al_get_mouse_wheel_precision() * e->wheel.x;
   }
   else {
      if (e->button.which == SDL_TOUCH_MOUSEID) {
         _al_event_source_unlock(es);
         return;
      }
      d = find_display(e->button.windowID);
      float ratio = d ? _al_sdl_get_display_pixel_ratio(d) : 1.0;
      switch (e->button.button) {
         case SDL_BUTTON_LEFT: event.mouse_float.button = 1; break;
         case SDL_BUTTON_RIGHT: event.mouse_float.button = 2; break;
         case SDL_BUTTON_MIDDLE: event.mouse_float.button = 3; break;
         case SDL_BUTTON_X1: event.mouse_float.button = 4; break;
         case SDL_BUTTON_X2: event.mouse_float.button = 5; break;
      }
      event.mouse_float.x = e->button.x * ratio;
      event.mouse_float.y = e->button.y * ratio;
      event.mouse_float.z = mouse->state.z;
      event.mouse_float.w = mouse->state.w;
      if (e->type == SDL_MOUSEBUTTONDOWN) {
         event.mouse_float.type = ALLEGRO_EVENT_MOUSE_BUTTON_DOWN_FLOAT;
         mouse->state.buttons |= 1 << (event.mouse_float.button - 1);
      }
      if (e->type == SDL_MOUSEBUTTONUP) {
         event.mouse_float.type = ALLEGRO_EVENT_MOUSE_BUTTON_UP_FLOAT;
         mouse->state.buttons &= ~(1 << (event.mouse_float.button - 1));
      }
   }

   event.mouse_float.pressure = mouse->state.buttons ? 1.0 : 0.0; /* TODO */
   event.mouse_float.display = d;

   _al_event_source_emit_event(es, &event);
   _al_make_int_mouse_event(&event);
   _al_event_source_emit_event(es, &event);
   _al_event_source_unlock(es);
}

static bool sdl_init_mouse(void)
{
   mouse = al_calloc(1, sizeof *mouse);
   _al_event_source_init(&mouse->mouse.es);
   return true;
}

static void sdl_exit_mouse(void)
{
}

static ALLEGRO_MOUSE *sdl_get_mouse(void)
{
   return &mouse->mouse;
}

static unsigned int sdl_get_mouse_num_buttons(void)
{
   return 5;
}

static unsigned int sdl_get_mouse_num_axes(void)
{
   return 4;
}

static bool sdl_set_mouse_xy(ALLEGRO_DISPLAY *display, int x, int y)
{
   ALLEGRO_DISPLAY_SDL *sdl = (void *)display;
   float ratio = _al_sdl_get_display_pixel_ratio(display);
   SDL_WarpMouseInWindow(sdl->window, x / ratio, y / ratio);
   return true;
}

static bool sdl_set_mouse_axis(int which, int value)
{
   if (which == 1) mouse->state.z = value;
   else if (which == 2) mouse->state.w = value;
   else return false;
   return true;
}

static void sdl_get_mouse_state(ALLEGRO_MOUSE_FLOAT_STATE *ret_state)
{
   float x, y, i;
   float ratio = _al_sdl_get_display_pixel_ratio(mouse->display);
   ALLEGRO_SYSTEM_INTERFACE *sdl = _al_sdl_system_driver();
   sdl->heartbeat();
   SDL_GetMouseState(&x, &y);
   ret_state->x = x * ratio;
   ret_state->y = y * ratio;
   ret_state->z = 0;
   ret_state->w = 0;
   for (i = 0; i < ALLEGRO_MOUSE_MAX_EXTRA_AXES; i++)
      ret_state->more_axes[i] = 0;
   ret_state->buttons = mouse->state.buttons;
   ret_state->pressure = mouse->state.buttons ? 1.0 : 0.0; /* TODO */
   ret_state->display = mouse->display;
}

ALLEGRO_MOUSE_DRIVER *_al_sdl_mouse_driver(void)
{
   if (vt)
      return vt;

   vt = al_calloc(1, sizeof *vt);
   vt->msedrv_id = AL_ID('S','D','L','2');
   vt->msedrv_name = "SDL2 Mouse";
   vt->msedrv_desc = "SDL2 Mouse";
   vt->msedrv_ascii_name = "SDL2 Mouse";
   vt->init_mouse = sdl_init_mouse;
   vt->exit_mouse = sdl_exit_mouse;
   vt->get_mouse = sdl_get_mouse;
   vt->get_mouse_num_buttons = sdl_get_mouse_num_buttons;
   vt->get_mouse_num_axes = sdl_get_mouse_num_axes;
   vt->set_mouse_xy = sdl_set_mouse_xy;
   vt->set_mouse_axis = sdl_set_mouse_axis;
   vt->get_mouse_state = sdl_get_mouse_state;

   return vt;
}
