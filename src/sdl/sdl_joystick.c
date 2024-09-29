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
 *      SDL joystick driver.
 *
 *      See LICENSE.txt for copyright information.
 */
#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/platform/allegro_internal_sdl.h"

A5O_DEBUG_CHANNEL("SDL")

typedef struct A5O_JOYSTICK_SDL
{
   int id;
   A5O_JOYSTICK allegro;
   SDL_Joystick *sdl;
} A5O_JOYSTICK_SDL;

static A5O_JOYSTICK_DRIVER *vt;
static int count;
static A5O_JOYSTICK_SDL *joysticks;

static int get_id(A5O_JOYSTICK *allegro)
{
   int i;
   for (i = 0; i < count; i++) {
      if (&joysticks[i].allegro == allegro)
         return i;
   }
   return -1;
}

static SDL_Joystick *get_sdl(A5O_JOYSTICK *allegro)
{
   int id = get_id(allegro);
   if (id < 0)
      return NULL;
   return joysticks[id].sdl;
}

void _al_sdl_joystick_event(SDL_Event *e)
{
   A5O_EVENT event;
   memset(&event, 0, sizeof event);

   event.joystick.timestamp = al_get_time();

   if (e->type == SDL_JOYAXISMOTION) {
      event.joystick.type = A5O_EVENT_JOYSTICK_AXIS;
      event.joystick.id = &joysticks[e->jaxis.which].allegro;
      event.joystick.stick = e->jaxis.axis / 2;
      event.joystick.axis = e->jaxis.axis % 2;
      event.joystick.pos = e->jaxis.value / 32768.0;
      event.joystick.button = 0;
   }
   else if (e->type == SDL_JOYBUTTONDOWN) {
      event.joystick.type = A5O_EVENT_JOYSTICK_BUTTON_DOWN;
      event.joystick.id = &joysticks[e->jbutton.which].allegro;
      event.joystick.stick = 0;
      event.joystick.axis = 0;
      event.joystick.pos = 0;
      event.joystick.button = e->jbutton.button;
   }
   else if (e->type == SDL_JOYBUTTONUP) {
      event.joystick.type = A5O_EVENT_JOYSTICK_BUTTON_UP;
      event.joystick.id = &joysticks[e->jbutton.which].allegro;
      event.joystick.stick = 0;
      event.joystick.axis = 0;
      event.joystick.pos = 0;
      event.joystick.button = e->jbutton.button;
   }
   else if (e->type == SDL_JOYDEVICEADDED || e->type == SDL_JOYDEVICEREMOVED) {
      event.joystick.type = A5O_EVENT_JOYSTICK_CONFIGURATION;
   }
   else {
      return;
   }

   A5O_EVENT_SOURCE *es = al_get_joystick_event_source();
   _al_event_source_lock(es);
   _al_event_source_emit_event(es, &event);
   _al_event_source_unlock(es);
}

static bool sdl_init_joystick(void)
{
   count = SDL_NumJoysticks();
   joysticks = count > 0 ? calloc(count, sizeof * joysticks) : NULL;
   int i;
   for (i = 0; i < count; i++) {
      joysticks[i].sdl = SDL_JoystickOpen(i);
      _AL_JOYSTICK_INFO *info = &joysticks[i].allegro.info;
      int an = SDL_JoystickNumAxes(joysticks[i].sdl);
      int a;
      info->num_sticks = an / 2;
      for (a = 0; a < an; a++) {
         info->stick[a / 2].num_axes = 2;
         info->stick[a / 2].name = "stick";
         info->stick[a / 2].axis[0].name = "X";
         info->stick[a / 2].axis[1].name = "Y";
      }

      int bn = SDL_JoystickNumButtons(joysticks[i].sdl);
      info->num_buttons = bn;
      int b;
      for (b = 0; b < bn; b++) {
         info->button[b].name = SDL_IsGameController(i) ?
            SDL_GameControllerGetStringForButton(b) : "button";
      }
   }
   SDL_JoystickEventState(SDL_ENABLE);
   return true;
}

static void sdl_exit_joystick(void)
{
   int i;
   for (i = 0; i < count; i++) {
      SDL_JoystickClose(joysticks[i].sdl);
   }
   count = 0;
   free(joysticks);
   joysticks = NULL;
}

static bool sdl_reconfigure_joysticks(void)
{
   sdl_exit_joystick();
   return sdl_init_joystick();
}

static int sdl_num_joysticks(void)
{
   return count;
}

static A5O_JOYSTICK *sdl_get_joystick(int joyn)
{
   return &joysticks[joyn].allegro;
}

static void sdl_release_joystick(A5O_JOYSTICK *joy)
{
   ASSERT(joy);
}

static void sdl_get_joystick_state(A5O_JOYSTICK *joy,
   A5O_JOYSTICK_STATE *ret_state)
{
   A5O_SYSTEM_INTERFACE *s = _al_sdl_system_driver();
   s->heartbeat();

   SDL_Joystick *sdl = get_sdl(joy);
   int an = SDL_JoystickNumAxes(sdl);
   int i;
   for (i = 0; i < an; i++) {
      ret_state->stick[i / 2].axis[i % 2] = SDL_JoystickGetAxis(sdl, i) / 32768.0;
   }
   int bn = SDL_JoystickNumButtons(sdl);
   for (i = 0; i < bn; i++) {
      ret_state->button[i] = SDL_JoystickGetButton(sdl, i) * 32767;
   }
}

static const char *sdl_get_name(A5O_JOYSTICK *joy)
{
   SDL_Joystick *sdl = get_sdl(joy);
   return SDL_JoystickName(sdl);
}

static bool sdl_get_active(A5O_JOYSTICK *joy)
{
   SDL_Joystick *sdl = get_sdl(joy);
   return SDL_JoystickGetAttached(sdl);
}

A5O_JOYSTICK_DRIVER *_al_sdl_joystick_driver(void)
{
   if (vt)
      return vt;

   vt = al_calloc(1, sizeof *vt);
   vt->joydrv_id = AL_ID('S','D','L','2');
   vt->joydrv_name = "SDL2 Joystick";
   vt->joydrv_desc = "SDL2 Joystick";
   vt->joydrv_ascii_name = "SDL2 Joystick";
   vt->init_joystick = sdl_init_joystick;
   vt->exit_joystick = sdl_exit_joystick;
   vt->reconfigure_joysticks = sdl_reconfigure_joysticks;
   vt->num_joysticks = sdl_num_joysticks;
   vt->get_joystick = sdl_get_joystick;
   vt->release_joystick = sdl_release_joystick;
   vt->get_joystick_state = sdl_get_joystick_state;;
   vt->get_name = sdl_get_name;
   vt->get_active = sdl_get_active;

   return vt;
}
