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
 *      SDL system interface.
 *
 *      See LICENSE.txt for copyright information.
 */
#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/platform/allegro_internal_sdl.h"
#include "allegro5/internal/aintern_timer.h"

ALLEGRO_DEBUG_CHANNEL("SDL")

static ALLEGRO_SYSTEM_INTERFACE *vt;

#define ALLEGRO_SDL_EVENT_QUEUE_SIZE 8

#ifdef DEBUGMODE
#define _E(x) if (type == x) return #x;
static char const *event_name(int type)
{
   _E(SDL_FIRSTEVENT)
   _E(SDL_QUIT)
   _E(SDL_APP_TERMINATING)
   _E(SDL_APP_LOWMEMORY)
   _E(SDL_APP_WILLENTERBACKGROUND)
   _E(SDL_APP_DIDENTERBACKGROUND)
   _E(SDL_APP_WILLENTERFOREGROUND)
   _E(SDL_APP_DIDENTERFOREGROUND)
   _E(SDL_WINDOWEVENT)
   _E(SDL_SYSWMEVENT)
   _E(SDL_KEYDOWN)
   _E(SDL_KEYUP)
   _E(SDL_TEXTEDITING)
   _E(SDL_TEXTINPUT)
   _E(SDL_MOUSEMOTION)
   _E(SDL_MOUSEBUTTONDOWN)
   _E(SDL_MOUSEBUTTONUP)
   _E(SDL_MOUSEWHEEL)
   _E(SDL_JOYAXISMOTION)
   _E(SDL_JOYBALLMOTION)
   _E(SDL_JOYHATMOTION)
   _E(SDL_JOYBUTTONDOWN)
   _E(SDL_JOYBUTTONUP)
   _E(SDL_JOYDEVICEADDED)
   _E(SDL_JOYDEVICEREMOVED)
   _E(SDL_CONTROLLERAXISMOTION)
   _E(SDL_CONTROLLERBUTTONDOWN)
   _E(SDL_CONTROLLERBUTTONUP)
   _E(SDL_CONTROLLERDEVICEADDED)
   _E(SDL_CONTROLLERDEVICEREMOVED)
   _E(SDL_CONTROLLERDEVICEREMAPPED)
   _E(SDL_FINGERDOWN)
   _E(SDL_FINGERUP)
   _E(SDL_FINGERMOTION)
   _E(SDL_DOLLARGESTURE)
   _E(SDL_DOLLARRECORD)
   _E(SDL_MULTIGESTURE)
   _E(SDL_CLIPBOARDUPDATE)
   _E(SDL_DROPFILE)
   _E(SDL_RENDER_TARGETS_RESET)
   _E(SDL_USEREVENT)
   return "(unknown)";
}
#undef _E
#endif

static void sdl_heartbeat(void)
{
   SDL_Event events[ALLEGRO_SDL_EVENT_QUEUE_SIZE];
   ALLEGRO_SYSTEM_SDL *s = (void *)al_get_system_driver();
   al_lock_mutex(s->mutex);
   SDL_PumpEvents();
   int n, i;
   while ((n = SDL_PeepEvents(events, ALLEGRO_SDL_EVENT_QUEUE_SIZE, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)) > 0) {
      for (i = 0; i < n; i++) {
         //printf("event %s\n", event_name(events[i].type));
         switch (events[i].type) {
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            case SDL_TEXTINPUT:
               _al_sdl_keyboard_event(&events[i]);
               break;
            case SDL_MOUSEMOTION:
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEWHEEL:
               _al_sdl_mouse_event(&events[i]);
               break;
            case SDL_FINGERDOWN:
            case SDL_FINGERMOTION:
            case SDL_FINGERUP:
                _al_sdl_touch_input_event(&events[i]);
                break;
            case SDL_JOYAXISMOTION:
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
            case SDL_JOYDEVICEADDED:
            case SDL_JOYDEVICEREMOVED:
                _al_sdl_joystick_event(&events[i]);
                break;
            case SDL_QUIT:
               _al_sdl_display_event(&events[i]);
               break;
            case SDL_WINDOWEVENT:
               switch (events[i].window.event) {
                  case SDL_WINDOWEVENT_ENTER:
                  case SDL_WINDOWEVENT_LEAVE:
                     _al_sdl_mouse_event(&events[i]);
                     break;
                  case SDL_WINDOWEVENT_FOCUS_GAINED:
                  case SDL_WINDOWEVENT_FOCUS_LOST:
                     _al_sdl_display_event(&events[i]);
                     _al_sdl_keyboard_event(&events[i]);
                     break;
                  case SDL_WINDOWEVENT_CLOSE:
                  case SDL_WINDOWEVENT_RESIZED:
                     _al_sdl_display_event(&events[i]);
                     break;
               }
         }
      }
   }
#ifdef __EMSCRIPTEN__
   double t = al_get_time();
   double interval = t - s->timer_time;
   _al_timer_thread_handle_tick(interval);
   s->timer_time = t;
#endif
   al_unlock_mutex(s->mutex);
}

static ALLEGRO_SYSTEM *sdl_initialize(int flags)
{
   (void)flags;
   ALLEGRO_SYSTEM_SDL *s = al_calloc(1, sizeof *s);
   s->system.vt = vt;

   // TODO: map allegro flags to sdl flags.
   unsigned int sdl_flags = SDL_INIT_EVERYTHING;
#ifdef __EMSCRIPTEN__
   // SDL currently does not support haptic feedback for emscripten.
   sdl_flags &= ~SDL_INIT_HAPTIC;
#endif
   if (SDL_Init(sdl_flags) < 0) {
      ALLEGRO_ERROR("SDL_Init failed: %s", SDL_GetError());
      return NULL;
   }

   _al_vector_init(&s->system.displays, sizeof (ALLEGRO_DISPLAY_SDL *));

   return &s->system;
}

static void sdl_heartbeat_init(void)
{
   ALLEGRO_SYSTEM_SDL *s = (void *)al_get_system_driver();

   /* This cannot be done in sdl_initialize because the threading system
    * requires a completed ALLEGRO_SYSTEM which only exists after the
    * function returns. This function on the other hand will get called
    * once the system was created.
    */
   s->mutex = al_create_mutex();

#ifdef __EMSCRIPTEN__
   s->timer_time = al_get_time();
#endif
}

static void sdl_shutdown_system(void)
{
   ALLEGRO_SYSTEM_SDL *s = (void *)al_get_system_driver();

   /* Close all open displays. */
   while (_al_vector_size(&s->system.displays) > 0) {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&s->system.displays, 0);
      ALLEGRO_DISPLAY *d = *dptr;
      al_destroy_display(d);
   }
   _al_vector_free(&s->system.displays);
   
   al_destroy_mutex(s->mutex);
   al_free(s);
   SDL_Quit();
}

static ALLEGRO_PATH *sdl_get_path(int id)
{
   ALLEGRO_PATH *p = NULL;
   char* dir;
   switch (id) {
      case ALLEGRO_TEMP_PATH:
      case ALLEGRO_USER_DOCUMENTS_PATH:
      case ALLEGRO_USER_DATA_PATH:
      case ALLEGRO_USER_SETTINGS_PATH:
         dir = SDL_GetPrefPath(al_get_org_name(), al_get_app_name());
         p = al_create_path_for_directory(dir);
         if (id == ALLEGRO_TEMP_PATH) {
            al_append_path_component(p, "tmp");
         }
         SDL_free(dir);
         break;
      case ALLEGRO_RESOURCES_PATH:
      case ALLEGRO_EXENAME_PATH:
      case ALLEGRO_USER_HOME_PATH:
         dir = SDL_GetBasePath();
         p = al_create_path_for_directory(dir);
         if (id == ALLEGRO_EXENAME_PATH) {
            al_set_path_filename(p, al_get_app_name());
         }
         SDL_free(dir);
         break;
   }
   return p;
}

static ALLEGRO_DISPLAY_INTERFACE *sdl_get_display_driver(void)
{
   return _al_sdl_display_driver();
}

static ALLEGRO_KEYBOARD_DRIVER *sdl_get_keyboard_driver(void)
{
   return _al_sdl_keyboard_driver();
}

static ALLEGRO_MOUSE_DRIVER *sdl_get_mouse_driver(void)
{
   return _al_sdl_mouse_driver();
}

static ALLEGRO_TOUCH_INPUT_DRIVER *sdl_get_touch_input_driver(void)
{
   return _al_sdl_touch_input_driver();
}

static ALLEGRO_JOYSTICK_DRIVER *sdl_get_joystick_driver(void)
{
   return _al_sdl_joystick_driver();
}

#define ADD(allegro, sdl) if (sdl_format == \
   SDL_PIXELFORMAT_##sdl) return ALLEGRO_PIXEL_FORMAT_##allegro;
int _al_sdl_get_allegro_pixel_format(int sdl_format) {
   ADD(ARGB_8888, ARGB8888)
   ADD(RGBA_8888, RGBA8888)
   ADD(ABGR_8888, ABGR8888)
   return 0;
}
#undef ADD

#define ADD(allegro, sdl) if (allegro_format == \
   ALLEGRO_PIXEL_FORMAT_##allegro) return SDL_PIXELFORMAT_##sdl;
int _al_sdl_get_sdl_pixel_format(int allegro_format) {
   ADD(ANY, ABGR8888)
   ADD(ANY_NO_ALPHA, ABGR8888)
   ADD(ANY_WITH_ALPHA, ABGR8888)
   ADD(ANY_32_NO_ALPHA, ABGR8888)
   ADD(ANY_32_WITH_ALPHA, ABGR8888)
   ADD(ARGB_8888, ARGB8888)
   ADD(RGBA_8888, RGBA8888)
   ADD(ABGR_8888, ABGR8888)
   ADD(ABGR_8888_LE, ABGR8888)
   return 0;
}
#undef ADD


static int sdl_get_num_video_adapters(void)
{
   return SDL_GetNumVideoDisplays();
}

static bool sdl_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO *info)
{
   SDL_Rect rect;
   if (SDL_GetDisplayBounds(adapter, &rect) < 0)
      return false;
   info->x1 = rect.x;
   info->y1 = rect.y;
   info->x2 = rect.x + rect.w;
   info->y2 = rect.y + rect.h;
   return true;
}

static ALLEGRO_MOUSE_CURSOR* sdl_create_mouse_cursor(ALLEGRO_BITMAP *sprite, int xfocus, int yfocus)
{
   SDL_Cursor *cursor;
   ALLEGRO_MOUSE_CURSOR_SDL *sdl_cursor;

   int w = al_get_bitmap_width(sprite);
   int h = al_get_bitmap_height(sprite);
   int data_size = w * h * 4;

   unsigned char* data = al_malloc(data_size * sizeof(data[0]));

   Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
   rmask = 0xff000000;
   gmask = 0x00ff0000;
   bmask = 0x0000ff00;
   amask = 0x000000ff;
#else // little endian, like x86
   rmask = 0x000000ff;
   gmask = 0x0000ff00;
   bmask = 0x00ff0000;
   amask = 0xff000000;
#endif

   ALLEGRO_LOCKED_REGION *lock = al_lock_bitmap(sprite, ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_READONLY);
   if (lock) {
      int i = 0, y = 0;
      for (y = 0; y < h; y++) {
         int x = 0;
         for (x = 0; x < w; x++) {
            ALLEGRO_COLOR c = al_get_pixel(sprite, x, y);
            al_unmap_rgba(c, data+i, data+i+1, data+i+2, data+i+3);
            i += 4;
         }
      }
      al_unlock_bitmap(sprite);

      SDL_Surface *icon = SDL_CreateRGBSurfaceFrom(data, w, h, 4 * 8, w * 4, rmask, gmask, bmask, amask);
      cursor = SDL_CreateColorCursor(icon, xfocus, yfocus);
      SDL_FreeSurface(icon);
   }

   al_free(data);
   if (!cursor) {
      return NULL;
   }

   sdl_cursor = al_malloc(sizeof *sdl_cursor);
   if (!sdl_cursor) {
      SDL_FreeCursor(cursor);
      return NULL;
   }

   sdl_cursor->cursor = cursor;
   return (ALLEGRO_MOUSE_CURSOR *)sdl_cursor;
}

static void sdl_destroy_mouse_cursor(ALLEGRO_MOUSE_CURSOR *cursor)
{
   ALLEGRO_MOUSE_CURSOR_SDL *sdl_cursor = (ALLEGRO_MOUSE_CURSOR_SDL *) cursor;
   ASSERT(sdl_cursor);
   SDL_FreeCursor(sdl_cursor->cursor);
   al_free(sdl_cursor);
}

static int sdl_get_num_display_modes(void)
{
   int i = al_get_new_display_adapter();
   if (i < 0)
      i = 0;
   return SDL_GetNumDisplayModes(i);
}

static ALLEGRO_DISPLAY_MODE *sdl_get_display_mode(int index, ALLEGRO_DISPLAY_MODE *mode)
{
   SDL_DisplayMode sdl_mode;
   int i = al_get_new_display_adapter();
   if (i < 0)
      i = 0;
   if (SDL_GetDisplayMode(i, index, &sdl_mode) < 0)
      return NULL;
   mode->width = sdl_mode.w;
   mode->height = sdl_mode.h;
   mode->format = _al_sdl_get_allegro_pixel_format(sdl_mode.format);
   mode->refresh_rate = sdl_mode.refresh_rate;
   return mode;
}

static bool sdl_inhibit_screensaver(bool inhibit)
{
  if (inhibit) {
    SDL_DisableScreenSaver();
  } else {
    SDL_EnableScreenSaver();
  }
  return SDL_IsScreenSaverEnabled() != inhibit;
}

/* Internal function to get a reference to this driver. */
ALLEGRO_SYSTEM_INTERFACE *_al_sdl_system_driver(void)
{
   if (vt)
      return vt;

   vt = al_calloc(1, sizeof *vt);
   vt->id = ALLEGRO_SYSTEM_ID_SDL;
   vt->initialize = sdl_initialize;
   vt->get_display_driver = sdl_get_display_driver;
   vt->get_keyboard_driver = sdl_get_keyboard_driver;
   vt->get_mouse_driver = sdl_get_mouse_driver;
   vt->get_touch_input_driver = sdl_get_touch_input_driver;
   vt->get_joystick_driver = sdl_get_joystick_driver;
   //vt->get_haptic_driver = sdl_get_haptic_driver;
   vt->get_num_display_modes = sdl_get_num_display_modes;
   vt->get_display_mode = sdl_get_display_mode;
   vt->shutdown_system = sdl_shutdown_system;
   vt->get_num_video_adapters = sdl_get_num_video_adapters;
   vt->get_monitor_info = sdl_get_monitor_info;
   vt->create_mouse_cursor = sdl_create_mouse_cursor;
   vt->destroy_mouse_cursor = sdl_destroy_mouse_cursor;
   /*vt->get_cursor_position = sdl_get_cursor_position;
   vt->grab_mouse = sdl_grab_mouse;
   vt->ungrab_mouse = sdl_ungrab_mouse;*/
   vt->get_path = sdl_get_path;
   vt->inhibit_screensaver = sdl_inhibit_screensaver;
   /*vt->thread_init = sdl_thread_init;
   vt->thread_exit = sdl_thread_exit;
   vt->open_library = sdl_open_library;
   vt->import_symbol = sdl_import_symbol;
   vt->close_library = sdl_close_library;*/
   vt->heartbeat = sdl_heartbeat;
   vt->heartbeat_init = sdl_heartbeat_init;
   vt->get_time = _al_sdl_get_time;
   vt->rest = _al_sdl_rest;
   vt->init_timeout = _al_sdl_init_timeout;

   return vt;
}

void _al_register_system_interfaces(void)
{
   ALLEGRO_SYSTEM_INTERFACE **add;
   add = _al_vector_alloc_back(&_al_system_interfaces);
   *add = _al_sdl_system_driver();
}
