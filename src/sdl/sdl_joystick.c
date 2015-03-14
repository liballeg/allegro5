#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/platform/allegro_internal_sdl.h"

ALLEGRO_DEBUG_CHANNEL("SDL")

typedef struct ALLEGRO_JOYSTICK_SDL
{
   int id;
   ALLEGRO_JOYSTICK allegro;
   SDL_Joystick *sdl;
} ALLEGRO_JOYSTICK_SDL;

static ALLEGRO_JOYSTICK_DRIVER *vt;
static int count;
static ALLEGRO_JOYSTICK_SDL *joysticks;

static int get_id(ALLEGRO_JOYSTICK *allegro)
{
   int i;
   for (i = 0; i < count; i++) {
      if (&joysticks[i].allegro == allegro)
         return i;
   }
   return -1;
}

static SDL_Joystick *get_sdl(ALLEGRO_JOYSTICK *allegro)
{
   int id = get_id(allegro);
   if (id < 0)
      return NULL;
   return joysticks[id].sdl;
}

void _al_sdl_joystick_event(SDL_Event *e)
{
   if (count <= 0)
      return;

   ALLEGRO_EVENT event;
   memset(&event, 0, sizeof event);

   event.joystick.timestamp = al_get_time();

   if (e->type == SDL_JOYAXISMOTION) {
      event.joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
      event.joystick.stick = e->jaxis.which;
      event.joystick.axis = e->jaxis.axis;
      event.joystick.pos = e->jaxis.value / 32768.0;
      event.joystick.button = 0;
   }
   else if (e->type == SDL_JOYBUTTONDOWN) {
      event.joystick.type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
      event.joystick.stick = e->jbutton.which;
      event.joystick.axis = 0;
      event.joystick.pos = 0;
      event.joystick.button = e->jbutton.button;
   }
   else if (e->type == SDL_JOYBUTTONUP) {
      event.joystick.type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
      event.joystick.stick = e->jbutton.which;
      event.joystick.axis = 0;
      event.joystick.pos = 0;
      event.joystick.button = e->jbutton.button;
   }
   else {
      return;
   }

   ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();
   _al_event_source_lock(es);
   _al_event_source_emit_event(es, &event);
   _al_event_source_unlock(es);
}

static bool sdl_init_joystick(void)
{
   count = SDL_NumJoysticks();
   joysticks = calloc(count, sizeof * joysticks);
   SDL_JoystickEventState(SDL_ENABLE);
   return true;
}

static void sdl_exit_joystick(void)
{
}

static bool sdl_reconfigure_joysticks(void)
{
   return false;
}

static int sdl_num_joysticks(void)
{
   return SDL_NumJoysticks();
}

static ALLEGRO_JOYSTICK *sdl_get_joystick(int joyn)
{
   return &joysticks[joyn].allegro;
}

static void sdl_release_joystick(ALLEGRO_JOYSTICK *joy)
{
   ASSERT(joy);
}

static void sdl_get_joystick_state(ALLEGRO_JOYSTICK *joy,
   ALLEGRO_JOYSTICK_STATE *ret_state)
{
   SDL_Joystick *sdl = get_sdl(joy);
   int an = SDL_JoystickNumAxes(sdl);
   int i;
   for (i = 0; i < an; i++) {
      ret_state->stick[0].axis[i] = SDL_JoystickGetAxis(sdl, i) / 32768.0;
   }
   int bn = SDL_JoystickNumButtons(sdl);
   for (i = 0; i < bn; i++) {
      ret_state->button[i] = SDL_JoystickGetButton(sdl, i) * 32767;
   }
}

static const char *sdl_get_name(ALLEGRO_JOYSTICK *joy)
{
   SDL_Joystick *sdl = get_sdl(joy);
   return SDL_JoystickName(sdl);
}

static bool sdl_get_active(ALLEGRO_JOYSTICK *joy)
{
   SDL_Joystick *sdl = get_sdl(joy);
   return SDL_JoystickGetAttached(sdl);
}

ALLEGRO_JOYSTICK_DRIVER *_al_sdl_joystick_driver(void)
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
