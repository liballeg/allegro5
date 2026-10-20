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
   SDL_JoystickID id;
   ALLEGRO_JOYSTICK allegro;
   SDL_Joystick *sdl;
   SDL_GameController *sdl_gc;
   bool dpad_state[4];
} ALLEGRO_JOYSTICK_SDL;

static ALLEGRO_JOYSTICK_DRIVER *vt;
static int num_joysticks; /* number of joysticks known to the user */
static _AL_VECTOR joysticks = _AL_VECTOR_INITIALIZER(ALLEGRO_JOYSTICK_SDL *); /* of ALLEGRO_JOYSTICK_SDL pointers */

static bool compat_5_2_10(void) {
   /* Mappings. */
   return _al_get_joystick_compat_version() < AL_ID(5, 2, 11, 0);
}

static ALLEGRO_JOYSTICK_SDL *get_joystick_from_allegro(ALLEGRO_JOYSTICK *allegro)
{
   for (int i = 0; i < (int)_al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_SDL *joy = *(ALLEGRO_JOYSTICK_SDL **)_al_vector_ref(&joysticks, i);
      if (&joy->allegro == allegro)
         return joy;
   }
   ASSERT(false);
   return NULL;
}

static ALLEGRO_JOYSTICK_SDL *get_joystick(SDL_JoystickID id)
{
   for (int i = 0; i < (int)_al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_SDL *joy = *(ALLEGRO_JOYSTICK_SDL **)_al_vector_ref(&joysticks, i);
      if (joy->id == id)
         return joy;
   }
   ASSERT(false);
   return NULL;
}

static ALLEGRO_JOYSTICK_SDL *get_or_insert_joystick(SDL_JoystickID id)
{
   ALLEGRO_JOYSTICK_SDL **slot;
   ALLEGRO_JOYSTICK_SDL *joy;

   for (int i = 0; i < (int)_al_vector_size(&joysticks); i++) {
      slot = _al_vector_ref(&joysticks, i);
      joy = *slot;
      if (joy->id == id)
         return joy;
   }

   for (int i = 0; i < (int)_al_vector_size(&joysticks); i++) {
      slot = _al_vector_ref(&joysticks, i);
      joy = *slot;
      if (!SDL_JoystickFromInstanceID(joy->id)) {
         joy->id = id;
         return joy;
      }
   }

   joy = al_calloc(1, sizeof *joy);
   joy->id = id;
   slot = _al_vector_alloc_back(&joysticks);
   *slot = joy;
   return joy;
}

void _al_sdl_joystick_event(SDL_Event *e)
{
   bool emit = false;
   ALLEGRO_EVENT event;
   memset(&event, 0, sizeof event);

   event.joystick.timestamp = al_get_time();

   if (e->type == SDL_CONTROLLERAXISMOTION) {
      ALLEGRO_JOYSTICK_SDL *joy = get_joystick(e->caxis.which);
      if (joy->allegro.info.type != ALLEGRO_JOYSTICK_TYPE_GAMEPAD)
         return;
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
         event.joystick.id = &joy->allegro;
         event.joystick.stick = stick;
         event.joystick.axis = axis;
         event.joystick.pos = e->caxis.value / 32768.0;
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
      ALLEGRO_JOYSTICK_SDL *joy = get_joystick(e->cbutton.which);
      if (joy->allegro.info.type != ALLEGRO_JOYSTICK_TYPE_GAMEPAD)
         return;

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
   else if (e->type == SDL_CONTROLLERDEVICEADDED || e->type == SDL_CONTROLLERDEVICEREMOVED
         || e->type == SDL_JOYDEVICEADDED || e->type == SDL_JOYDEVICEREMOVED) {
      event.joystick.type = ALLEGRO_EVENT_JOYSTICK_CONFIGURATION;
      emit = true;
   }
   else if (e->type == SDL_JOYAXISMOTION) {
      ALLEGRO_JOYSTICK_SDL *joy = get_joystick(e->jaxis.which);
      if (joy->allegro.info.type != ALLEGRO_JOYSTICK_TYPE_UNKNOWN)
         return;
      if (e->jaxis.axis >= _AL_MAX_JOYSTICK_STICKS * 2)
         return;
      event.joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
      event.joystick.id = &joy->allegro;
      event.joystick.stick = e->jaxis.axis / 2;
      event.joystick.axis = e->jaxis.axis % 2;
      event.joystick.pos = e->jaxis.value / 32768.0;
      event.joystick.button = 0;
      emit = true;
   }
   else if (e->type == SDL_JOYBUTTONDOWN) {
      ALLEGRO_JOYSTICK_SDL *joy = get_joystick(e->jbutton.which);
      if (joy->allegro.info.type != ALLEGRO_JOYSTICK_TYPE_UNKNOWN)
         return;
      if (e->jbutton.button >= _AL_MAX_JOYSTICK_BUTTONS)
         return;
      event.joystick.type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
      event.joystick.id = &joy->allegro;
      event.joystick.stick = 0;
      event.joystick.axis = 0;
      event.joystick.pos = 0;
      event.joystick.button = e->jbutton.button;
      emit = true;
   }
   else if (e->type == SDL_JOYBUTTONUP) {
      ALLEGRO_JOYSTICK_SDL *joy = get_joystick(e->jbutton.which);
      if (joy->allegro.info.type != ALLEGRO_JOYSTICK_TYPE_UNKNOWN)
         return;
      if (e->jbutton.button >= _AL_MAX_JOYSTICK_BUTTONS)
         return;
      event.joystick.type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
      event.joystick.id = &joy->allegro;
      event.joystick.stick = 0;
      event.joystick.axis = 0;
      event.joystick.pos = 0;
      event.joystick.button = e->jbutton.button;
      emit = true;
   }
   else {
      return;
   }

   if (emit) {
      ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();
      _al_event_source_lock(es);
      _al_event_source_emit_event(es, &event);
      _al_event_source_unlock(es);
   }
}

static void clear_joysticks(void)
{
   for (int i = 0; i < (int)_al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_SDL *joy = *(ALLEGRO_JOYSTICK_SDL **)_al_vector_ref(&joysticks, i);
      if (joy->sdl) {
         SDL_JoystickClose(joy->sdl);
         joy->sdl = NULL;
      }
      if (joy->sdl_gc) {
         SDL_GameControllerClose(joy->sdl_gc);
         joy->sdl_gc = NULL;
      }
      _al_destroy_joystick_info(&joy->allegro.info);
      /* That leaves the type and the stick flags, which would carry over if
       * this slot is reused for a different kind of joystick.
       */
      memset(&joy->allegro.info, 0, sizeof(joy->allegro.info));
      /* Note that we deliberately keep the joystick id as is, so we can reuse it. */
   }
}

/* The number of sticks and buttons of a raw (non game controller) SDL
 * joystick that Allegro exposes. SDL reports whatever the device declares:
 * DirectInput devices can report up to 256 buttons (e.g. 8BitDo receivers'
 * vendor interface does), but _AL_JOYSTICK_INFO and ALLEGRO_JOYSTICK_STATE
 * have room for only _AL_MAX_JOYSTICK_BUTTONS buttons and
 * _AL_MAX_JOYSTICK_STICKS sticks; the rest are dropped.
 */
static int raw_num_axes(SDL_Joystick *sdl)
{
   int an = SDL_JoystickNumAxes(sdl);
   if (an < 0)
      return 0;
   return an > _AL_MAX_JOYSTICK_STICKS * 2 ? _AL_MAX_JOYSTICK_STICKS * 2 : an;
}

static int raw_num_buttons(SDL_Joystick *sdl)
{
   int bn = SDL_JoystickNumButtons(sdl);
   if (bn < 0)
      return 0;
   return bn > _AL_MAX_JOYSTICK_BUTTONS ? _AL_MAX_JOYSTICK_BUTTONS : bn;
}

static bool sdl_init_joystick(void)
{
   /* Hold SDL's joystick lock for the whole scan. Device indices are only
    * stable while it is held: SDL's RawInput and WGI backends (Windows) add
    * and remove devices from their own threads, which shifts the index of
    * every device after the changed one. Without the lock, consecutive calls
    * below with the same index could each see a different device.
    */
   SDL_LockJoysticks();
   num_joysticks = SDL_NumJoysticks();
   for (int i = 0; i < num_joysticks; i++) {
      ALLEGRO_JOYSTICK_SDL *joy = get_or_insert_joystick(SDL_JoystickGetDeviceInstanceID(i));
      _AL_JOYSTICK_INFO *info = &joy->allegro.info;
      SDL_JoystickGUID sdl_guid = SDL_JoystickGetDeviceGUID(i);
      memcpy(&info->guid, &sdl_guid, sizeof(ALLEGRO_JOYSTICK_GUID));

      bool is_gc = SDL_IsGameController(i);
      if (is_gc && !compat_5_2_10()) {
         joy->sdl_gc = SDL_GameControllerOpen(i);
         _al_fill_gamepad_info(info);
         SDL_GameControllerEventState(SDL_ENABLE);
         continue;
      }

      joy->sdl = SDL_JoystickOpen(i);
      int an = raw_num_axes(joy->sdl);
      info->num_sticks = an / 2;
      for (int s = 0; s < info->num_sticks; s++) {
         info->stick[s].num_axes = 2;
         info->stick[s].name = _al_strdup("stick");
         info->stick[s].axis[0].name = _al_strdup("X");
         info->stick[s].axis[1].name = _al_strdup("Y");
      }

      int bn = raw_num_buttons(joy->sdl);
      info->num_buttons = bn;
      for (int b = 0; b < bn; b++) {
         /* NULL for b >= SDL_CONTROLLER_BUTTON_MAX. */
         const char *name = is_gc ? SDL_GameControllerGetStringForButton(b) : NULL;
         info->button[b].name = _al_strdup(name ? name : "button");
      }
      SDL_JoystickEventState(SDL_ENABLE);
   }
   SDL_UnlockJoysticks();
   return true;
}

static void sdl_exit_joystick(void)
{
   clear_joysticks();
   num_joysticks = 0;
   _al_vector_free(&joysticks);
}

static bool sdl_reconfigure_joysticks(void)
{
   clear_joysticks();
   return sdl_init_joystick();
}

static int sdl_num_joysticks(void)
{
   return num_joysticks;
}

static ALLEGRO_JOYSTICK *sdl_get_joystick(int joyn)
{
   for (int i = 0; i < (int)_al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_SDL *joy = *(ALLEGRO_JOYSTICK_SDL **)_al_vector_ref(&joysticks, i);
      if (SDL_JoystickFromInstanceID(joy->id)) {
         if (joyn == 0)
            return &joy->allegro;
         joyn--;
      }
   }
   return NULL;
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

   ALLEGRO_JOYSTICK_SDL *joy_sdl = get_joystick_from_allegro(joy);
   if (joy->info.type == ALLEGRO_JOYSTICK_TYPE_GAMEPAD) {
      SDL_GameController *sdl_gc = joy_sdl->sdl_gc;
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
      SDL_Joystick *sdl = joy_sdl->sdl;
      int an = raw_num_axes(sdl);
      int i;
      for (i = 0; i < an; i++) {
         ret_state->stick[i / 2].axis[i % 2] = AXIS(SDL_JoystickGetAxis(sdl, i));
      }
      int bn = raw_num_buttons(sdl);
      for (i = 0; i < bn; i++) {
         ret_state->button[i] = BUTTON(SDL_JoystickGetButton(sdl, i));
      }
   }

#undef AXIS
#undef BUTTON

}

static const char *sdl_get_name(ALLEGRO_JOYSTICK *joy)
{
   ALLEGRO_JOYSTICK_SDL *joy_sdl = get_joystick_from_allegro(joy);
   if (joy_sdl->sdl)
      return SDL_JoystickName(joy_sdl->sdl);
   if (joy_sdl->sdl_gc)
      return SDL_GameControllerName(joy_sdl->sdl_gc);
   return NULL;
}

static bool sdl_get_active(ALLEGRO_JOYSTICK *joy)
{
   ALLEGRO_JOYSTICK_SDL *joy_sdl = get_joystick_from_allegro(joy);
   if (joy_sdl->sdl)
      return SDL_JoystickGetAttached(joy_sdl->sdl);
   if (joy_sdl->sdl_gc)
      return SDL_GameControllerGetAttached(joy_sdl->sdl_gc);
   return false;
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
