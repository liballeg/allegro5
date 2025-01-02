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
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/platform/allegro_internal_sdl.h"

ALLEGRO_DEBUG_CHANNEL("SDL")

typedef struct ALLEGRO_JOYSTICK_SDL
{
   int id;
   ALLEGRO_JOYSTICK allegro;
   SDL_Joystick *sdl;
   SDL_GameController *sdl_gc;
   bool dpad_state[4];
} ALLEGRO_JOYSTICK_SDL;

static ALLEGRO_JOYSTICK_DRIVER *vt;
static int count;
static ALLEGRO_JOYSTICK_SDL *joysticks;

static bool compat_5_2_10(void) {
   /* Mappings. */
   return _al_get_joystick_compat_version() < AL_ID(5, 2, 11, 0);
}

static int get_id(ALLEGRO_JOYSTICK *allegro)
{
   int i;
   for (i = 0; i < count; i++) {
      if (&joysticks[i].allegro == allegro)
         return i;
   }
   ASSERT(false);
   return 0;
}

void _al_sdl_joystick_event(SDL_Event *e)
{
   bool emit = false;
   ALLEGRO_EVENT event;
   memset(&event, 0, sizeof event);

   event.joystick.timestamp = al_get_time();

   if (joysticks[e->jaxis.which].allegro.info.type == ALLEGRO_JOYSTICK_TYPE_GAMEPAD) {
      if (e->type == SDL_CONTROLLERAXISMOTION) {
         int stick = -1;
         int axis = -1;
         switch (e->caxis.axis) {
            case SDL_CONTROLLER_AXIS_LEFTX:
               stick = ALLEGRO_GAMEPAD_STICK_LEFT_THUMB;
               axis = 0;
               break;
            case SDL_CONTROLLER_AXIS_LEFTY:
               stick = ALLEGRO_GAMEPAD_STICK_LEFT_THUMB;
               axis = 1;
               break;
            case SDL_CONTROLLER_AXIS_RIGHTX:
               stick = ALLEGRO_GAMEPAD_STICK_RIGHT_THUMB;
               axis = 0;
               break;
            case SDL_CONTROLLER_AXIS_RIGHTY:
               stick = ALLEGRO_GAMEPAD_STICK_RIGHT_THUMB;
               axis = 1;
               break;
            case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
               stick = ALLEGRO_GAMEPAD_STICK_LEFT_TRIGGER;
               axis = 0;
               break;
            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
               stick = ALLEGRO_GAMEPAD_STICK_RIGHT_TRIGGER;
               axis = 0;
               break;
         }
         if (stick >= 0 && axis >= 0) {
            event.joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
            event.joystick.id = &joysticks[e->jaxis.which].allegro;
            event.joystick.stick = stick;
            event.joystick.axis = axis;
            event.joystick.pos = e->jaxis.value / 32768.0;
            event.joystick.button = 0;
            emit = true;
         }
      }
      else if (e->type == SDL_CONTROLLERBUTTONDOWN || e->type == SDL_CONTROLLERBUTTONUP) {
         int stick = 0;
         int axis = 0;
         int button = 0;
         float pos = 0.;
         bool down = e->type == SDL_CONTROLLERBUTTONDOWN;
         int type = down ? ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN : ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
         ALLEGRO_JOYSTICK_SDL *joy = &joysticks[e->cbutton.which];

         switch (e->cbutton.button) {
            case SDL_CONTROLLER_BUTTON_A:
               button = ALLEGRO_GAMEPAD_BUTTON_A;
               break;
            case SDL_CONTROLLER_BUTTON_B:
               button = ALLEGRO_GAMEPAD_BUTTON_B;
               break;
            case SDL_CONTROLLER_BUTTON_X:
               button = ALLEGRO_GAMEPAD_BUTTON_X;
               break;
            case SDL_CONTROLLER_BUTTON_Y:
               button = ALLEGRO_GAMEPAD_BUTTON_Y;
               break;
            case SDL_CONTROLLER_BUTTON_BACK:
               button = ALLEGRO_GAMEPAD_BUTTON_BACK;
               break;
            case SDL_CONTROLLER_BUTTON_GUIDE:
               button = ALLEGRO_GAMEPAD_BUTTON_GUIDE;
               break;
            case SDL_CONTROLLER_BUTTON_START:
               button = ALLEGRO_GAMEPAD_BUTTON_START;
               break;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
               button = ALLEGRO_GAMEPAD_BUTTON_LEFT_SHOULDER;
               break;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
               button = ALLEGRO_GAMEPAD_BUTTON_RIGHT_SHOULDER;
               break;
            case SDL_CONTROLLER_BUTTON_LEFTSTICK:
               button = ALLEGRO_GAMEPAD_BUTTON_LEFT_THUMB;
               break;
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
               button = ALLEGRO_GAMEPAD_BUTTON_RIGHT_THUMB;
               break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
               type = ALLEGRO_EVENT_JOYSTICK_AXIS;
               axis = 0;
               joy->dpad_state[2] = down;
               break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
               type = ALLEGRO_EVENT_JOYSTICK_AXIS;
               axis = 0;
               joy->dpad_state[0] = down;
               break;
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
               type = ALLEGRO_EVENT_JOYSTICK_AXIS;
               axis = 1;
               joy->dpad_state[3] = down;
               break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
               type = ALLEGRO_EVENT_JOYSTICK_AXIS;
               axis = 1;
               joy->dpad_state[1] = down;
               break;
            default:
               type = 0;
         }
         if (type == ALLEGRO_EVENT_JOYSTICK_AXIS) {
            stick = ALLEGRO_GAMEPAD_STICK_DPAD;
            if (axis == 0)
               pos = (int)joy->dpad_state[0] - (int)joy->dpad_state[2];
            else
               pos = (int)joy->dpad_state[1] - (int)joy->dpad_state[3];
         }
         if (type != 0) {
            event.joystick.type = type;
            event.joystick.id = &joy->allegro;
            event.joystick.stick = stick;
            event.joystick.axis = axis;
            event.joystick.pos = pos;
            event.joystick.button = button;
            emit = true;
         }
      }
      else if (e->type == SDL_CONTROLLERDEVICEADDED || e->type == SDL_CONTROLLERDEVICEREMOVED) {
         event.joystick.type = ALLEGRO_EVENT_JOYSTICK_CONFIGURATION;
      }
      else {
         return;
      }
   }
   else {
      if (e->type == SDL_JOYAXISMOTION) {
         event.joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
         event.joystick.id = &joysticks[e->jaxis.which].allegro;
         event.joystick.stick = e->jaxis.axis / 2;
         event.joystick.axis = e->jaxis.axis % 2;
         event.joystick.pos = e->jaxis.value / 32768.0;
         event.joystick.button = 0;
      }
      else if (e->type == SDL_JOYBUTTONDOWN) {
         event.joystick.type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
         event.joystick.id = &joysticks[e->jbutton.which].allegro;
         event.joystick.stick = 0;
         event.joystick.axis = 0;
         event.joystick.pos = 0;
         event.joystick.button = e->jbutton.button;
      }
      else if (e->type == SDL_JOYBUTTONUP) {
         event.joystick.type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
         event.joystick.id = &joysticks[e->jbutton.which].allegro;
         event.joystick.stick = 0;
         event.joystick.axis = 0;
         event.joystick.pos = 0;
         event.joystick.button = e->jbutton.button;
      }
      else if (e->type == SDL_JOYDEVICEADDED || e->type == SDL_JOYDEVICEREMOVED) {
         event.joystick.type = ALLEGRO_EVENT_JOYSTICK_CONFIGURATION;
      }
      else {
         return;
      }
      emit = true;
   }

   if (emit) {
      ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();
      _al_event_source_lock(es);
      _al_event_source_emit_event(es, &event);
      _al_event_source_unlock(es);
   }
}

static bool sdl_init_joystick(void)
{
   count = SDL_NumJoysticks();
   joysticks = count > 0 ? al_calloc(count, sizeof * joysticks) : NULL;
   int i;
   for (i = 0; i < count; i++) {
      ALLEGRO_JOYSTICK_SDL *joy = &joysticks[i];
      SDL_JoystickGUID sdl_guid = SDL_JoystickGetDeviceGUID(i);
      memcpy(&joy->allegro.info.guid, &sdl_guid, sizeof(ALLEGRO_JOYSTICK_GUID));
      _al_fill_gamepad_info(&joy->allegro.info);

      if (SDL_IsGameController(i) && !compat_5_2_10()) {
         joy->sdl_gc = SDL_GameControllerOpen(i);
         SDL_GameControllerEventState(SDL_ENABLE);
         continue;
      }
      joy->sdl = SDL_JoystickOpen(i);
      _AL_JOYSTICK_INFO *info = &joy->allegro.info;
      int an = SDL_JoystickNumAxes(joy->sdl);
      int a;
      info->num_sticks = an / 2;
      for (a = 0; a < an; a++) {
         info->stick[a / 2].num_axes = 2;
         info->stick[a / 2].name = _al_strdup("stick");
         info->stick[a / 2].axis[0].name = _al_strdup("X");
         info->stick[a / 2].axis[1].name = _al_strdup("Y");
      }

      int bn = SDL_JoystickNumButtons(joy->sdl);
      info->num_buttons = bn;
      int b;
      for (b = 0; b < bn; b++) {
         info->button[b].name = _al_strdup(SDL_IsGameController(i) ?
            SDL_GameControllerGetStringForButton(b) : "button");
      }
      SDL_JoystickEventState(SDL_ENABLE);
   }
   return true;
}

static void sdl_exit_joystick(void)
{
   int i;
   for (i = 0; i < count; i++) {
      if (joysticks[i].sdl)
         SDL_JoystickClose(joysticks[i].sdl);
      if (joysticks[i].sdl_gc)
         SDL_GameControllerClose(joysticks[i].sdl_gc);
      _al_destroy_joystick_info(&joysticks[i].allegro.info);
   }
   count = 0;
   al_free(joysticks);
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
   ALLEGRO_SYSTEM_INTERFACE *s = _al_sdl_system_driver();
   s->heartbeat();

#define BUTTON(x) ((x) * 32767)
#define AXIS(x) ((x) / 32768.0)

   int id = get_id(joy);
   if (joysticks[id].allegro.info.type == ALLEGRO_JOYSTICK_TYPE_GAMEPAD) {
      SDL_GameController *sdl_gc = joysticks[id].sdl_gc;
      ret_state->stick[ALLEGRO_GAMEPAD_STICK_DPAD].axis[0] = SDL_GameControllerGetButton(sdl_gc, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
            - SDL_GameControllerGetButton(sdl_gc, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
      ret_state->stick[ALLEGRO_GAMEPAD_STICK_DPAD].axis[1] = SDL_GameControllerGetButton(sdl_gc, SDL_CONTROLLER_BUTTON_DPAD_DOWN)
            - SDL_GameControllerGetButton(sdl_gc, SDL_CONTROLLER_BUTTON_DPAD_UP);
      ret_state->stick[ALLEGRO_GAMEPAD_STICK_LEFT_THUMB].axis[0] = AXIS(SDL_GameControllerGetAxis(sdl_gc, SDL_CONTROLLER_AXIS_LEFTX));
      ret_state->stick[ALLEGRO_GAMEPAD_STICK_LEFT_THUMB].axis[1] = AXIS(SDL_GameControllerGetAxis(sdl_gc, SDL_CONTROLLER_AXIS_LEFTY));
      ret_state->stick[ALLEGRO_GAMEPAD_STICK_RIGHT_THUMB].axis[0] = AXIS(SDL_GameControllerGetAxis(sdl_gc, SDL_CONTROLLER_AXIS_RIGHTX));
      ret_state->stick[ALLEGRO_GAMEPAD_STICK_RIGHT_THUMB].axis[1] = AXIS(SDL_GameControllerGetAxis(sdl_gc, SDL_CONTROLLER_AXIS_RIGHTY));
      ret_state->stick[ALLEGRO_GAMEPAD_STICK_LEFT_TRIGGER].axis[0] = AXIS(SDL_GameControllerGetAxis(sdl_gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT));
      ret_state->stick[ALLEGRO_GAMEPAD_STICK_RIGHT_TRIGGER].axis[0] = AXIS(SDL_GameControllerGetAxis(sdl_gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT));

      ret_state->button[ALLEGRO_GAMEPAD_BUTTON_A] = BUTTON(SDL_GameControllerGetButton(sdl_gc, SDL_CONTROLLER_BUTTON_A));
      ret_state->button[ALLEGRO_GAMEPAD_BUTTON_B] = BUTTON(SDL_GameControllerGetButton(sdl_gc, SDL_CONTROLLER_BUTTON_B));
      ret_state->button[ALLEGRO_GAMEPAD_BUTTON_X] = BUTTON(SDL_GameControllerGetButton(sdl_gc, SDL_CONTROLLER_BUTTON_X));
      ret_state->button[ALLEGRO_GAMEPAD_BUTTON_Y] = BUTTON(SDL_GameControllerGetButton(sdl_gc, SDL_CONTROLLER_BUTTON_Y));
      ret_state->button[ALLEGRO_GAMEPAD_BUTTON_BACK] = BUTTON(SDL_GameControllerGetButton(sdl_gc, SDL_CONTROLLER_BUTTON_BACK));
      ret_state->button[ALLEGRO_GAMEPAD_BUTTON_GUIDE] = BUTTON(SDL_GameControllerGetButton(sdl_gc, SDL_CONTROLLER_BUTTON_GUIDE));
      ret_state->button[ALLEGRO_GAMEPAD_BUTTON_START] = BUTTON(SDL_GameControllerGetButton(sdl_gc, SDL_CONTROLLER_BUTTON_START));
      ret_state->button[ALLEGRO_GAMEPAD_BUTTON_LEFT_SHOULDER] = BUTTON(SDL_GameControllerGetButton(sdl_gc, SDL_CONTROLLER_BUTTON_LEFTSHOULDER));
      ret_state->button[ALLEGRO_GAMEPAD_BUTTON_RIGHT_SHOULDER] = BUTTON(SDL_GameControllerGetButton(sdl_gc, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER));
      ret_state->button[ALLEGRO_GAMEPAD_BUTTON_LEFT_THUMB] = BUTTON(SDL_GameControllerGetButton(sdl_gc, SDL_CONTROLLER_BUTTON_LEFTSTICK));
      ret_state->button[ALLEGRO_GAMEPAD_BUTTON_RIGHT_THUMB] = BUTTON(SDL_GameControllerGetButton(sdl_gc, SDL_CONTROLLER_BUTTON_RIGHTSTICK));
   }
   else {
      SDL_Joystick *sdl = joysticks[id].sdl;
      int an = SDL_JoystickNumAxes(sdl);
      int i;
      for (i = 0; i < an; i++) {
         ret_state->stick[i / 2].axis[i % 2] = AXIS(SDL_JoystickGetAxis(sdl, i));
      }
      int bn = SDL_JoystickNumButtons(sdl);
      for (i = 0; i < bn; i++) {
         ret_state->button[i] = BUTTON(SDL_JoystickGetButton(sdl, i));
      }
   }

#undef AXIS
#undef BUTTON

}

static const char *sdl_get_name(ALLEGRO_JOYSTICK *joy)
{
   int id = get_id(joy);
   if (joysticks[id].sdl)
      return SDL_JoystickName(joysticks[id].sdl);
   else
      return SDL_GameControllerName(joysticks[id].sdl_gc);
}

static bool sdl_get_active(ALLEGRO_JOYSTICK *joy)
{
   int id = get_id(joy);
   if (joysticks[id].sdl)
      return SDL_JoystickGetAttached(joysticks[id].sdl);
   else
      return SDL_GameControllerGetAttached(joysticks[id].sdl_gc);
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
