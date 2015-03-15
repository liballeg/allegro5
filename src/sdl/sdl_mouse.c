#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/platform/allegro_internal_sdl.h"

ALLEGRO_DEBUG_CHANNEL("SDL")

typedef struct ALLEGRO_MOUSE_SDL
{
   ALLEGRO_MOUSE mouse;
   ALLEGRO_MOUSE_STATE state;
   ALLEGRO_DISPLAY *display;
} ALLEGRO_MOUSE_SDL;

static ALLEGRO_MOUSE_DRIVER *vt;
static ALLEGRO_MOUSE_SDL *mouse;

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
      if (e->window.event == SDL_WINDOWEVENT_ENTER) {
         event.mouse.type = ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY;
         SDL_GetMouseState(&event.mouse.x, &event.mouse.y );
      }
      else {
         event.mouse.type = ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY;
         event.mouse.x = mouse->state.x;
         event.mouse.y = mouse->state.y;
         event.mouse.z = mouse->state.z;
         event.mouse.w = mouse->state.w;
      }
      d = _al_sdl_find_display(e->window.windowID);
      mouse->display = e->window.event == SDL_WINDOWEVENT_ENTER ? d : NULL;
   }
   else if (e->type == SDL_MOUSEMOTION) {
      event.mouse.type = ALLEGRO_EVENT_MOUSE_AXES;
      event.mouse.x = e->motion.x;
      event.mouse.y = e->motion.y;
      event.mouse.z = mouse->state.z;
      event.mouse.w = mouse->state.w;
      event.mouse.dx = e->motion.xrel;
      event.mouse.dy = e->motion.yrel;
      event.mouse.dz = 0;
      event.mouse.dw = 0;
      mouse->state.x = e->motion.x;
      mouse->state.y = e->motion.y;
      d = _al_sdl_find_display(e->motion.windowID);
   }
   else if (e->type == SDL_MOUSEWHEEL) {
      event.mouse.type = ALLEGRO_EVENT_MOUSE_AXES;
      mouse->state.z += al_get_mouse_wheel_precision() * e->wheel.y;
      mouse->state.w += al_get_mouse_wheel_precision() * e->wheel.x;
      event.mouse.x = mouse->state.x;
      event.mouse.y = mouse->state.y;
      event.mouse.z = mouse->state.z;
      event.mouse.w = mouse->state.w;
      event.mouse.dx = 0;
      event.mouse.dy = 0;
      event.mouse.dz = al_get_mouse_wheel_precision() * e->wheel.y;
      event.mouse.dw = al_get_mouse_wheel_precision() * e->wheel.x;
      d = _al_sdl_find_display(e->wheel.windowID);
   }
   else {
      switch (e->button.button) {
         case SDL_BUTTON_LEFT: event.mouse.button = 1; break;
         case SDL_BUTTON_RIGHT: event.mouse.button = 2; break;
         case SDL_BUTTON_MIDDLE: event.mouse.button = 3; break;
         case SDL_BUTTON_X1: event.mouse.button = 4; break;
         case SDL_BUTTON_X2: event.mouse.button = 5; break;
      }
      event.mouse.x = e->button.x;
      event.mouse.y = e->button.y;
      event.mouse.z = mouse->state.z;
      event.mouse.w = mouse->state.w;
      if (e->type == SDL_MOUSEBUTTONDOWN) {
         event.mouse.type = ALLEGRO_EVENT_MOUSE_BUTTON_DOWN;
         mouse->state.buttons |= 1 << (event.mouse.button - 1);
      }
      if (e->type == SDL_MOUSEBUTTONUP) {
         event.mouse.type = ALLEGRO_EVENT_MOUSE_BUTTON_UP;
         mouse->state.buttons &= ~(1 << (event.mouse.button - 1));
      }
      d = _al_sdl_find_display(e->button.windowID);
   }

   event.mouse.pressure = mouse->state.buttons ? 1.0 : 0.0; /* TODO */
   event.mouse.display = d;

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
   SDL_WarpMouseInWindow(sdl->window, x, y);
   return true;
}

static bool sdl_set_mouse_axis(int which, int value)
{
   if (which == 1) mouse->state.z = value;
   else if (which == 2) mouse->state.w = value;
   else return false;
   return true;
}

static void sdl_get_mouse_state(ALLEGRO_MOUSE_STATE *ret_state)
{
   int x, y, i;
   ALLEGRO_SYSTEM_INTERFACE *sdl = _al_sdl_system_driver();
   sdl->heartbeat();
   SDL_GetMouseState(&x, &y);
   ret_state->x = x;
   ret_state->y = y;
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
