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
 *      Windows touch input driver.
 *
 *      By Michał Cichoń.
 *
 *      See readme.txt for copyright information.
 */

#include <windows.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_driver.h"
#include "allegro5/internal/aintern_touch_input.h"
#include "allegro5/platform/aintwin.h"
#include "allegro5/internal/aintern_display.h"

ALLEGRO_DEBUG_CHANNEL("touch")

static ALLEGRO_TOUCH_INPUT_STATE touch_input_state;
static ALLEGRO_TOUCH_INPUT touch_input;
static bool installed = false;
static size_t initiali_time_stamp = ~0;


/* WinAPI import related stuff. */
static int touch_input_api_reference_counter = 0;
static void* user32_module = NULL;

CLOSETOUCHINPUTHANDLEPROC     _al_win_close_touch_input_handle = NULL;
GETTOUCHINPUTINFOPROC         _al_win_get_touch_input_info     = NULL;
ISTOUCHWINDOWPROC             _al_win_is_touch_window          = NULL;
REGISTERTOUCHWINDOWPROC       _al_win_register_touch_window    = NULL;
UNREGISTERTOUCHWINDOWPROC     _al_win_unregister_touch_window  = NULL;

bool _al_win_init_touch_input_api(void)
{
   /* Reference counter will be negative if we already tried to initialize the driver.
    * Lack of touch input API is persistent state, so do not test for it again. */
   if (touch_input_api_reference_counter < 0)
      return false;

   /* Increase API reference counter if it is already initialized. */
   if (touch_input_api_reference_counter > 0)
   {
      touch_input_api_reference_counter++;
      return true;
   }

   /* Open handle to user32.dll. This one should be always present. */
   user32_module = _al_open_library("user32.dll");
   if (NULL == user32_module)
      return false;

   _al_win_close_touch_input_handle = (CLOSETOUCHINPUTHANDLEPROC)_al_import_symbol(user32_module, "CloseTouchInputHandle");
   _al_win_get_touch_input_info     = (GETTOUCHINPUTINFOPROC)    _al_import_symbol(user32_module, "GetTouchInputInfo");
   _al_win_is_touch_window          = (ISTOUCHWINDOWPROC)        _al_import_symbol(user32_module, "IsTouchWindow");
   _al_win_register_touch_window    = (REGISTERTOUCHWINDOWPROC)  _al_import_symbol(user32_module, "RegisterTouchWindow");
   _al_win_unregister_touch_window  = (UNREGISTERTOUCHWINDOWPROC)_al_import_symbol(user32_module, "UnregisterTouchWindow");

   if (NULL == _al_win_close_touch_input_handle || NULL == _al_win_get_touch_input_info  ||
       NULL == _al_win_is_touch_window          || NULL == _al_win_register_touch_window ||
       NULL == _al_win_unregister_touch_window) {

      _al_close_library(user32_module);

      _al_win_close_touch_input_handle = NULL;
      _al_win_get_touch_input_info     = NULL;
      _al_win_is_touch_window          = NULL;
      _al_win_register_touch_window    = NULL;
      _al_win_unregister_touch_window  = NULL;

      /* Mark as 'do not try this again'. */
      touch_input_api_reference_counter = -1;
	  ALLEGRO_WARN("failed loading the touch input API\n");
      return false;
   }

   /* Set initial reference count. */
   touch_input_api_reference_counter = 1;
   ALLEGRO_INFO("touch input API installed successfully\n");
   return true;
}

void _al_win_exit_touch_input_api(void)
{
   if (touch_input_api_reference_counter > 1)
   {
      --touch_input_api_reference_counter;
      return;
   }

   if (touch_input_api_reference_counter != 1)
      return;

   _al_close_library(user32_module);

   _al_win_close_touch_input_handle = NULL;
   _al_win_get_touch_input_info     = NULL;
   _al_win_is_touch_window          = NULL;
   _al_win_register_touch_window    = NULL;
   _al_win_unregister_touch_window  = NULL;

   touch_input_api_reference_counter = 0;
}


/* Actual driver implementation. */

static void generate_touch_input_event(int type, double timestamp, int id, float x, float y, float dx, float dy, bool primary, ALLEGRO_DISPLAY_WIN *win_disp)
{
   ALLEGRO_EVENT event;

   bool want_touch_event           = _al_event_source_needs_to_generate_event(&touch_input.es);
   bool want_mouse_emulation_event = _al_event_source_needs_to_generate_event(&touch_input.mouse_emulation_es) && primary && al_is_mouse_installed();

   if (touch_input.mouse_emulation_mode == ALLEGRO_MOUSE_EMULATION_NONE)
      want_mouse_emulation_event = false;
   else if (touch_input.mouse_emulation_mode == ALLEGRO_MOUSE_EMULATION_INCLUSIVE)
      want_touch_event = want_mouse_emulation_event ? (want_touch_event && !primary) : want_touch_event;
   else if (touch_input.mouse_emulation_mode == ALLEGRO_MOUSE_EMULATION_EXCLUSIVE)
      want_touch_event = want_mouse_emulation_event ? false : want_touch_event;

   if (!want_touch_event && !want_mouse_emulation_event)
      return;

   if (want_touch_event) {

      event.touch.type      = type;
      event.touch.display   = (ALLEGRO_DISPLAY*)win_disp;
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

   if (want_mouse_emulation_event) {

      ALLEGRO_MOUSE_STATE state;

      switch (type) {
         case ALLEGRO_EVENT_TOUCH_BEGIN: type = ALLEGRO_EVENT_MOUSE_BUTTON_DOWN; break;
         case ALLEGRO_EVENT_TOUCH_CANCEL:
         case ALLEGRO_EVENT_TOUCH_END:   type = ALLEGRO_EVENT_MOUSE_BUTTON_UP;   break;
         case ALLEGRO_EVENT_TOUCH_MOVE:  type = ALLEGRO_EVENT_MOUSE_AXES;        break;
      }

      al_get_mouse_state(&state);

      event.mouse.type      = type;
      event.mouse.timestamp = timestamp;
      event.mouse.display   = (ALLEGRO_DISPLAY*)win_disp;
      event.mouse.x         = (int)x;
      event.mouse.y         = (int)y;
      event.mouse.z         = state.z;
      event.mouse.w         = state.w;
      event.mouse.dx        = (int)dx;
      event.mouse.dy        = (int)dy;
      event.mouse.dz        = 0;
      event.mouse.dw        = 0;
      event.mouse.button    = 1;
      event.mouse.pressure  = state.pressure;

      al_set_mouse_xy(event.mouse.display, event.mouse.x, event.mouse.y);

      _al_event_source_lock(&touch_input.mouse_emulation_es);
      _al_event_source_emit_event(&touch_input.mouse_emulation_es, &event);
      _al_event_source_unlock(&touch_input.mouse_emulation_es);
   }
}

static bool init_touch_input(void)
{
   unsigned i;
   ALLEGRO_SYSTEM* system;

   if (installed)
      return false;

   if (!_al_win_init_touch_input_api())
      return false;

   memset(&touch_input_state, 0, sizeof(touch_input_state));

   _al_event_source_init(&touch_input.es);
   _al_event_source_init(&touch_input.mouse_emulation_es);

   touch_input.mouse_emulation_mode = ALLEGRO_MOUSE_EMULATION_TRANSPARENT;

   installed = true;

   system = al_get_system_driver();
   for (i = 0; i < _al_vector_size(&system->displays); ++i) {
      bool r;
      ALLEGRO_DISPLAY_WIN *win_display = *((ALLEGRO_DISPLAY_WIN**)_al_vector_ref(&system->displays, i));
      r = _al_win_register_touch_window(win_display->window, 0);
	  ALLEGRO_INFO("registering touch window %p: %d\n", win_display, r);
	  if (!r) {
		 ALLEGRO_ERROR("RegisterTouchWindow failed: %s\n", _al_win_last_error());
	     return false;
	  }
   }

   return true;
}


static void exit_touch_input(void)
{
   if (!installed)
      return;

   memset(&touch_input_state, 0, sizeof(touch_input_state));

   _al_event_source_free(&touch_input.es);
   _al_event_source_free(&touch_input.mouse_emulation_es);

   _al_win_exit_touch_input_api();

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
            _al_win_touch_input_handle_cancel(touch->id, initiali_time_stamp,
               touch->x, touch->y, touch->primary, (ALLEGRO_DISPLAY_WIN*)touch->display);
         }
      }

      touch_input.mouse_emulation_mode = mode;
   }
}


static ALLEGRO_TOUCH_STATE* find_free_touch_state(void)
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


static double get_time_stamp(size_t timestamp)
{
   return al_get_time() - ((timestamp - initiali_time_stamp) / 1000.0);
}


void _al_win_touch_input_set_time_stamp(size_t timestamp)
{
   if (initiali_time_stamp != (size_t)~0)
      initiali_time_stamp = timestamp;
}


void _al_win_touch_input_handle_begin(int id, size_t timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY_WIN *win_disp)
{
   ALLEGRO_TOUCH_STATE* state = find_free_touch_state();
   if (NULL == state)
      return;

   _al_event_source_lock(&touch_input.es);
   state->id      = id;
   state->x       = x;
   state->y       = y;
   state->dx      = 0.0f;
   state->dy      = 0.0f;
   state->primary = primary;
   _al_event_source_unlock(&touch_input.es);

   generate_touch_input_event(ALLEGRO_EVENT_TOUCH_BEGIN, get_time_stamp(timestamp),
      state->id, state->x, state->y, state->dx, state->dy, state->primary, win_disp);
}


void _al_win_touch_input_handle_end(int id, size_t timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY_WIN *win_disp)
{
   ALLEGRO_TOUCH_STATE* state;
   (void)primary;

   state = find_touch_state_with_id(id);
   if (NULL == state)
      return;

   _al_event_source_lock(&touch_input.es);
   state->dx      = x - state->x;
   state->dy      = y - state->y;
   state->x       = x;
   state->y       = y;
   _al_event_source_unlock(&touch_input.es);

   generate_touch_input_event(ALLEGRO_EVENT_TOUCH_END, get_time_stamp(timestamp),
      state->id, state->x, state->y, state->dx, state->dy, state->primary, win_disp);

   _al_event_source_lock(&touch_input.es);
   memset(state, 0, sizeof(ALLEGRO_TOUCH_STATE));
   _al_event_source_unlock(&touch_input.es);
}


void _al_win_touch_input_handle_move(int id, size_t timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY_WIN *win_disp)
{
   ALLEGRO_TOUCH_STATE* state;
   (void)primary;

   state = find_touch_state_with_id(id);
   if (NULL == state)
      return;

   _al_event_source_lock(&touch_input.es);
   state->dx      = x - state->x;
   state->dy      = y - state->y;
   state->x       = x;
   state->y       = y;
   _al_event_source_unlock(&touch_input.es);

   generate_touch_input_event(ALLEGRO_EVENT_TOUCH_MOVE, get_time_stamp(timestamp),
      state->id, state->x, state->y, state->dx, state->dy, state->primary, win_disp);
}


void _al_win_touch_input_handle_cancel(int id, size_t timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY_WIN *win_disp)
{
   ALLEGRO_TOUCH_STATE* state;
   (void)primary;

   state = find_touch_state_with_id(id);
   if (NULL == state)
      return;

   _al_event_source_lock(&touch_input.es);
   state->dx      = x - state->x;
   state->dy      = y - state->y;
   state->x       = x;
   state->y       = y;
   _al_event_source_unlock(&touch_input.es);

   generate_touch_input_event(ALLEGRO_EVENT_TOUCH_CANCEL, get_time_stamp(timestamp),
      state->id, state->x, state->y, state->dx, state->dy, state->primary, win_disp);

   _al_event_source_lock(&touch_input.es);
   memset(state, 0, sizeof(ALLEGRO_TOUCH_STATE));
   _al_event_source_unlock(&touch_input.es);
}


/* the driver vtable */
#define TOUCH_INPUT_WINAPI AL_ID('W','T','I','D')

static ALLEGRO_TOUCH_INPUT_DRIVER touch_input_driver =
{
   TOUCH_INPUT_WINAPI,
   init_touch_input,
   exit_touch_input,
   get_touch_input,
   get_touch_input_state,
   set_mouse_emulation_mode,
   NULL
};

_AL_DRIVER_INFO _al_touch_input_driver_list[] =
{
   {TOUCH_INPUT_WINAPI, &touch_input_driver, true},
   {0, NULL, 0}
};
