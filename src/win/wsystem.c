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
 *      New Windows system driver
 *
 *      Based on the X11 OpenGL driver by Elias Pschernig.
 *
 *      Heavily modified by Trent Gamblin.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/platform/aintwin.h"

#if defined ALLEGRO_CFG_OPENGL
   #include "allegro5/allegro_opengl.h"
#endif

#include <windows.h>
#include <mmsystem.h>

ALLEGRO_DEBUG_CHANNEL("system")


/* FIXME: should we check for psapi _WIN32_IE and shlobj?
{ */
   #include <psapi.h>

   #if _WIN32_IE < 0x500
   #undef _WIN32_IE
   #define _WIN32_IE 0x500
   #endif
   #include <shlobj.h>
   #include <shlwapi.h>
/* } */

bool _al_win_disable_screensaver = false;

static ALLEGRO_SYSTEM_INTERFACE *vt = 0;
static bool using_higher_res_timer;

static ALLEGRO_SYSTEM_WIN *_al_win_system;

static bool d3d_available = true;

/* _WinMain:
 *  Entry point for Windows GUI programs, hooked by a macro in alwin.h,
 *  which makes it look as if the application can still have a normal
 *  main() function.
 */
int _WinMain(void *_main, void *hInst, void *hPrev, char *Cmd, int nShow)
{
   int (*mainfunc) (int argc, char *argv[]) = (int (*)(int, char *[]))_main;
   char *argbuf;
   char *cmdline;
   char **argv;
   int argc;
   int argc_max;
   int i, q;

   (void)hInst;
   (void)hPrev;
   (void)Cmd;
   (void)nShow;

   /* can't use parameter because it doesn't include the executable name */
   cmdline = GetCommandLine();
   i = strlen(cmdline) + 1;
   argbuf = al_malloc(i);
   memcpy(argbuf, cmdline, i);

   argc = 0;
   argc_max = 64;
   argv = al_malloc(sizeof(char *) * argc_max);
   if (!argv) {
      al_free(argbuf);
      return 1;
   }

   i = 0;

   /* parse commandline into argc/argv format */
   while (argbuf[i]) {
      while ((argbuf[i]) && (isspace(argbuf[i])))
         i++;

      if (argbuf[i]) {
         if ((argbuf[i] == '\'') || (argbuf[i] == '"')) {
            q = argbuf[i++];
            if (!argbuf[i])
               break;
         }
         else
            q = 0;

         argv[argc++] = &argbuf[i];

         if (argc >= argc_max) {
            argc_max += 64;
            argv = al_realloc(argv, sizeof(char *) * argc_max);
            if (!argv) {
               al_free(argbuf);
               return 1;
            }
         }

         while ((argbuf[i]) && ((q) ? (argbuf[i] != q) : (!isspace(argbuf[i]))))
            i++;

            if (argbuf[i]) {
            argbuf[i] = 0;
            i++;
         }
      }
   }

   argv[argc] = NULL;

   /* call the application entry point */
   i = mainfunc(argc, argv);

   al_free(argv);
   al_free(argbuf);

   return i;
}


static bool maybe_d3d_init_display(void)
{
#ifdef ALLEGRO_CFG_D3D
   return _al_d3d_init_display();
#else
   return false;
#endif
}


/* Create a new system object. */
static ALLEGRO_SYSTEM *win_initialize(int flags)
{
   (void)flags;

   _al_win_system = al_calloc(1, sizeof *_al_win_system);

   // Request a 1ms resolution from our timer
   if (timeBeginPeriod(1) != TIMERR_NOCANDO) {
      using_higher_res_timer = true;
   }
   _al_win_init_time();

   _al_win_init_window();

   _al_vector_init(&_al_win_system->system.displays, sizeof (ALLEGRO_SYSTEM_WIN *));

   _al_win_system->system.vt = vt;

   d3d_available = maybe_d3d_init_display();

   return &_al_win_system->system;
}


static void win_shutdown(void)
{
   ALLEGRO_SYSTEM *s;
   ALLEGRO_DISPLAY_INTERFACE *display_driver;
   ASSERT(vt);

   /* Close all open displays. */
   s = al_get_system_driver();
   while (_al_vector_size(&s->displays) > 0) {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&s->displays, 0);
      ALLEGRO_DISPLAY *d = *dptr;
      _al_destroy_display_bitmaps(d);
      al_destroy_display(d);
   }

   _al_vector_free(&s->displays);

   display_driver = vt->get_display_driver();
   if (display_driver && display_driver->shutdown) {
      display_driver->shutdown();
   }

   _al_win_shutdown_time();

   if (using_higher_res_timer) {
      timeEndPeriod(1);
   }

   al_free(vt);
   vt = NULL;

   ASSERT(_al_win_system);
   al_free(_al_win_system);
}


static ALLEGRO_DISPLAY_INTERFACE *win_get_display_driver(void)
{
   const int flags = al_get_new_display_flags();
   ALLEGRO_SYSTEM *sys = al_get_system_driver();
   ALLEGRO_SYSTEM_WIN *syswin = (ALLEGRO_SYSTEM_WIN *)sys;

   /* Look up the toggle_mouse_grab_key binding.  This isn't such a great place
    * to do it, but the config file is not available in win_initialize,
    * and this is neutral between the D3D and OpenGL display drivers.
    */
   if (sys->config && !syswin->toggle_mouse_grab_keycode) {
      const char *binding = al_get_config_value(sys->config,
         "keyboard", "toggle_mouse_grab_key");
      if (binding) {
         syswin->toggle_mouse_grab_keycode = _al_parse_key_binding(binding,
            &syswin->toggle_mouse_grab_modifiers);
         if (syswin->toggle_mouse_grab_keycode) {
            ALLEGRO_DEBUG("Toggle mouse grab key: '%s'\n", binding);
         }
         else {
            ALLEGRO_WARN("Cannot parse key binding '%s'\n", binding);
         }
      }
   }

   /* Programmatic selection. */
#ifdef ALLEGRO_CFG_D3D
   if (flags & ALLEGRO_DIRECT3D_INTERNAL) {
      if (d3d_available) {
         return _al_display_d3d_driver();
      }
      ALLEGRO_WARN("Direct3D graphics driver not available.\n");
      return NULL;
   }
#endif
#ifdef ALLEGRO_CFG_OPENGL
   if (flags & ALLEGRO_OPENGL) {
      return _al_display_wgl_driver();
   }
#endif

   /* Selection by configuration file.  The configuration value is non-binding.
    * The user may unknowingly set a value which was configured out at compile
    * time.  The value should have no effect instead of causing a failure.
    */
   if (sys->config) {
      const char *s = al_get_config_value(sys->config, "graphics", "driver");
      if (s) {
         ALLEGRO_DEBUG("Configuration value graphics.driver = %s\n", s);
         if (0 == _al_stricmp(s, "DIRECT3D") || 0 == _al_stricmp(s, "D3D")) {
#ifdef ALLEGRO_CFG_D3D
            if (d3d_available) {
               al_set_new_display_flags(flags | ALLEGRO_DIRECT3D_INTERNAL);
               return _al_display_d3d_driver();
            }
#endif
         }
         else if (0 == _al_stricmp(s, "OPENGL")) {
#ifdef ALLEGRO_CFG_OPENGL
            al_set_new_display_flags(flags | ALLEGRO_OPENGL);
            return _al_display_wgl_driver();
#endif
         }
         else if (0 != _al_stricmp(s, "DEFAULT")) {
            ALLEGRO_WARN("Graphics driver selection unrecognised: %s\n", s);
         }
      }
   }

   /* Automatic graphics driver selection. */
   /* XXX is implicitly setting new_display_flags the desired behaviour? */
#ifdef ALLEGRO_CFG_D3D
   if (d3d_available) {
      al_set_new_display_flags(flags | ALLEGRO_DIRECT3D_INTERNAL);
      return _al_display_d3d_driver();
   }
#endif
#ifdef ALLEGRO_CFG_OPENGL
   {
      al_set_new_display_flags(flags | ALLEGRO_OPENGL);
      return _al_display_wgl_driver();
   }
#endif
   ALLEGRO_WARN("No graphics driver available.\n");
   return NULL;
}

/* FIXME: use the list */
static ALLEGRO_KEYBOARD_DRIVER *win_get_keyboard_driver(void)
{
   return _al_keyboard_driver_list[0].driver;
}

static ALLEGRO_JOYSTICK_DRIVER *win_get_joystick_driver(void)
{
   return _al_joystick_driver_list[0].driver;
}

static int win_get_num_display_modes(void)
{
   int format = _al_deduce_color_format(_al_get_new_display_settings());
   int refresh_rate = al_get_new_display_refresh_rate();
   int flags = al_get_new_display_flags();

#if defined ALLEGRO_CFG_OPENGL
   if (flags & ALLEGRO_OPENGL) {
      return _al_wgl_get_num_display_modes(format, refresh_rate, flags);
   }
#endif
#if defined ALLEGRO_CFG_D3D
   return _al_d3d_get_num_display_modes(format, refresh_rate, flags);
#endif

   return 0;
}

static ALLEGRO_DISPLAY_MODE *win_get_display_mode(int index,
   ALLEGRO_DISPLAY_MODE *mode)
{
   int format = _al_deduce_color_format(_al_get_new_display_settings());
   int refresh_rate = al_get_new_display_refresh_rate();
   int flags = al_get_new_display_flags();

#if defined ALLEGRO_CFG_OPENGL
   if (flags & ALLEGRO_OPENGL) {
      return _al_wgl_get_display_mode(index, format, refresh_rate, flags, mode);
   }
#endif
#if defined ALLEGRO_CFG_D3D
   return _al_d3d_get_display_mode(index, format, refresh_rate, flags, mode);
#endif


   return NULL;
}

static int win_get_num_video_adapters(void)
{
   DISPLAY_DEVICE dd;
   int count = 0;
   int c = 0;

   memset(&dd, 0, sizeof(dd));
   dd.cb = sizeof(dd);

   while (EnumDisplayDevices(NULL, count, &dd, 0) != false) {
      if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
         c++;
      count++;
   }

   return c;
}

static bool win_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO *info)
{
   DISPLAY_DEVICE dd;
   DEVMODE dm;

   memset(&dd, 0, sizeof(dd));
   dd.cb = sizeof(dd);
   if (!EnumDisplayDevices(NULL, adapter, &dd, 0)) {
      return false;
   }

   memset(&dm, 0, sizeof(dm));
   dm.dmSize = sizeof(dm);
   if (!EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm)) {
      return false;
   }

   ASSERT(dm.dmFields & DM_PELSHEIGHT);
   ASSERT(dm.dmFields & DM_PELSWIDTH);
   /* Disabled this assertion for now as it fails under Wine 1.2. */
   /* ASSERT(dm.dmFields & DM_POSITION); */

   info->x1 = dm.dmPosition.x;
   info->y1 = dm.dmPosition.y;
   info->x2 = info->x1 + dm.dmPelsWidth;
   info->y2 = info->y1 + dm.dmPelsHeight;
   return true;
}

static bool win_get_cursor_position(int *ret_x, int *ret_y)
{
   POINT p;
   GetCursorPos(&p);
   *ret_x = p.x;
   *ret_y = p.y;
   return true;
}

static bool win_grab_mouse(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_SYSTEM_WIN *system = (void *)al_get_system_driver();
   ALLEGRO_DISPLAY_WIN *win_disp = (void *)display;
   RECT rect;

   GetWindowRect(win_disp->window, &rect);
   if (ClipCursor(&rect) != 0) {
      system->mouse_grab_display = display;
      return true;
   }

   return false;
}

static bool win_ungrab_mouse(void)
{
   ALLEGRO_SYSTEM_WIN *system = (void *)al_get_system_driver();

   ClipCursor(NULL);
   system->mouse_grab_display = NULL;
   return true;
}

static ALLEGRO_MOUSE_DRIVER *win_get_mouse_driver(void)
{
   return _al_mouse_driver_list[0].driver;
}

/* _al_win_get_path:
 *  Returns full path to various system and user diretories
 */
ALLEGRO_PATH *_al_win_get_path(int id)
{
   char path[MAX_PATH];
   uint32_t csidl = 0;
   HRESULT ret = 0;
   ALLEGRO_PATH *cisdl_path = NULL;

   switch (id) {
      case ALLEGRO_TEMP_PATH: {
         /* Check: TMP, TMPDIR, TEMP or TEMPDIR */
         DWORD ret = GetTempPath(MAX_PATH, path);
         if (ret > MAX_PATH) {
            /* should this ever happen, windows is more broken than I ever thought */
            return NULL;
         }

         return al_create_path_for_directory(path);

      } break;

      case ALLEGRO_RESOURCES_PATH: { /* where the program is in */
         HANDLE process = GetCurrentProcess();
         char *ptr;
         GetModuleFileNameEx(process, NULL, path, MAX_PATH);
         ptr = strrchr(path, '\\');
         if (!ptr) { /* shouldn't happen */
            return NULL;
         }

         /* chop off everything including and after the last slash */
         /* should this not chop the slash? */
         *ptr = '\0';

         return al_create_path_for_directory(path);
      } break;

      case ALLEGRO_USER_DATA_PATH: /* CSIDL_APPDATA */
      case ALLEGRO_USER_SETTINGS_PATH:
         csidl = CSIDL_APPDATA;
         break;

      case ALLEGRO_USER_HOME_PATH: /* CSIDL_PROFILE */
         csidl = CSIDL_PROFILE;
         break;
         
      case ALLEGRO_USER_DOCUMENTS_PATH: /* CSIDL_PERSONAL */
         csidl = CSIDL_PERSONAL;
         break;

      case ALLEGRO_EXENAME_PATH: { /* full path to the exe including its name */
         HANDLE process = GetCurrentProcess();
         GetModuleFileNameEx(process, NULL, path, MAX_PATH);

         return al_create_path(path);
      } break;
      
      default:
         return NULL;
   }

   ret = SHGetFolderPath(NULL, csidl, NULL, SHGFP_TYPE_CURRENT, path);
   if (ret != S_OK) {
      return NULL;
   }

   cisdl_path = al_create_path_for_directory(path);
   if (!cisdl_path)
      return NULL;

   if (csidl == CSIDL_APPDATA) {
      const char *org_name = al_get_org_name();
      const char *app_name = al_get_app_name();

      if (!app_name || !app_name[0]) {
         /* this shouldn't ever happen. */
         al_destroy_path(cisdl_path);
         return NULL;
      }

      if (org_name && org_name[0]) {
         al_append_path_component(cisdl_path, org_name);
      }

      al_append_path_component(cisdl_path, app_name);
   }

   return cisdl_path;
}

static bool win_inhibit_screensaver(bool inhibit)
{
   _al_win_disable_screensaver = inhibit;
   return true;
}

static HMODULE load_library_at_path(const char *path_str)
{
   HMODULE lib;

   /*
    * XXX LoadLibrary will search the current directory for any dependencies of
    * the library we are loading.  Using LoadLibraryEx with the appropriate
    * flags would fix that, but when I tried it I was unable to load dsound.dll
    * on Vista.
    */

   ALLEGRO_DEBUG("Calling LoadLibrary %s\n", path_str);
   lib = LoadLibraryA(path_str);
   if (lib) {
      ALLEGRO_INFO("Loaded %s\n", path_str);
   }
   else {
      DWORD error = GetLastError();
      HRESULT hr = HRESULT_FROM_WIN32(error);
      /* XXX do something with it */
      (void)hr;
      ALLEGRO_WARN("Failed to load %s (error: %ld)\n", path_str, error);
   }

   return lib;
}

static bool is_build_config_name(const char *s)
{
   return s &&
      (  0 == strcmp(s, "Debug")
      || 0 == strcmp(s, "Release")
      || 0 == strcmp(s, "RelWithDebInfo")
      || 0 == strcmp(s, "Profile"));
}

static ALLEGRO_PATH *maybe_parent_dir(const ALLEGRO_PATH *path)
{
   ALLEGRO_PATH *path2;

   if (!path)
      return NULL;

   if (!is_build_config_name(al_get_path_tail(path)))
      return NULL;

   path2 = al_clone_path(path);
   if (path2) {
      al_drop_path_tail(path2);
      al_set_path_filename(path2, NULL);
      ALLEGRO_DEBUG("Also searching %s\n", al_path_cstr(path2, '\\'));
   }

   return path2;
}

/*
 * Calling LoadLibrary with a relative file name is a security risk:
 * see e.g. Microsoft Security Advisory (2269637)
 * "Insecure Library Loading Could Allow Remote Code Execution"
 */
HMODULE _al_win_safe_load_library(const char *filename)
{
   ALLEGRO_PATH *path1 = NULL;
   ALLEGRO_PATH *path2 = NULL;
   char buf[MAX_PATH];
   const char *other_dirs[3];
   HMODULE lib = NULL;
   bool msvc_only = false;
   
   /* MSVC only: if the executable is in the build configuration directory,
    * which is also just under the current directory, then also try to load the
    * library from the current directory.  This leads to less surprises when
    * running example programs.
    */
#if defined(ALLEGRO_MSVC)
   msvc_only = true;
#endif

   /* Try to load the library from the directory containing the running
    * executable, the Windows system directories, or directories on the PATH.
    * Do NOT try to load the library from the current directory.
    */

   if (al_is_system_installed()) {
      path1 = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
   }
   else if (GetModuleFileName(NULL, buf, sizeof(buf)) < sizeof(buf)) {
      path1 = al_create_path(buf);
   }
   if (msvc_only) {
      path2 = maybe_parent_dir(path1);
   }

   other_dirs[0] = path1 ? al_path_cstr(path1, '\\') : NULL;
   other_dirs[1] = path2 ? al_path_cstr(path2, '\\') : NULL;
   other_dirs[2] = NULL; /* sentinel */

   _al_sane_strncpy(buf, filename, sizeof(buf));
   if (PathFindOnPath(buf, other_dirs)) {
      ALLEGRO_DEBUG("PathFindOnPath found: %s\n", buf);
      lib = load_library_at_path(buf);
   }
   else {
      ALLEGRO_WARN("PathFindOnPath failed to find %s\n", filename);
   }

   al_destroy_path(path1);
   al_destroy_path(path2);

   return lib;
}

static void *win_open_library(const char *filename)
{
   return _al_win_safe_load_library(filename);
}

static void *win_import_symbol(void *library, const char *symbol)
{
   ASSERT(library);
   return GetProcAddress(library, symbol);
}

static void win_close_library(void *library)
{
   ASSERT(library);
   FreeLibrary(library);
}

static ALLEGRO_SYSTEM_INTERFACE *_al_system_win_driver(void)
{
   if (vt) return vt;

   vt = al_calloc(1, sizeof *vt);

   vt->initialize = win_initialize;
   vt->get_display_driver = win_get_display_driver;
   vt->get_keyboard_driver = win_get_keyboard_driver;
   vt->get_mouse_driver = win_get_mouse_driver;
   vt->get_joystick_driver = win_get_joystick_driver;
   vt->get_num_display_modes = win_get_num_display_modes;
   vt->get_display_mode = win_get_display_mode;
   vt->shutdown_system = win_shutdown;
   vt->get_num_video_adapters = win_get_num_video_adapters;
   vt->create_mouse_cursor = _al_win_create_mouse_cursor;
   vt->destroy_mouse_cursor = _al_win_destroy_mouse_cursor;
   vt->get_monitor_info = win_get_monitor_info;
   vt->get_cursor_position = win_get_cursor_position;
   vt->grab_mouse = win_grab_mouse;
   vt->ungrab_mouse = win_ungrab_mouse;
   vt->get_path = _al_win_get_path;
   vt->inhibit_screensaver = win_inhibit_screensaver;
   vt->open_library = win_open_library;
   vt->import_symbol = win_import_symbol;
   vt->close_library = win_close_library;

   return vt;
}

void _al_register_system_interfaces()
{
   ALLEGRO_SYSTEM_INTERFACE **add;

   add = _al_vector_alloc_back(&_al_system_interfaces);
   *add = _al_system_win_driver();
}

/* vim: set sts=3 sw=3 et: */
