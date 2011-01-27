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
#if defined ALLEGRO_CFG_D3D
   #include "allegro5/allegro_direct3d.h"
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




/* Create a new system object. */
static ALLEGRO_SYSTEM *win_initialize(int flags)
{
   bool result;
   (void)flags;
   (void)result;

   _al_win_system = al_malloc(sizeof *_al_win_system);
   memset(_al_win_system, 0, sizeof *_al_win_system);

   // Request a 1ms resolution from our timer
   if (timeBeginPeriod(1) != TIMERR_NOCANDO) {
      using_higher_res_timer = true;
   }
   _al_win_init_time();

   _al_win_init_window();

   _al_vector_init(&_al_win_system->system.displays, sizeof (ALLEGRO_SYSTEM_WIN *));

   _al_win_system->system.vt = vt;

#if defined ALLEGRO_CFG_D3D
   result = _al_d3d_init_display();
#ifndef ALLEGRO_CFG_OPENGL
   if (result != true)
      return NULL;
#else
   if (result != true)
      d3d_available = false;
#endif
#endif
   
   return &_al_win_system->system;
}


static void win_shutdown(void)
{
   /* Close all open displays. */
   ALLEGRO_SYSTEM *s = al_get_system_driver();
   while (_al_vector_size(&s->displays) > 0) {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&s->displays, 0);
      ALLEGRO_DISPLAY *d = *dptr;
      _al_destroy_display_bitmaps(d);
      al_destroy_display(d);
   }

   _al_vector_free(&s->displays);

   if (vt->get_display_driver && vt->get_display_driver()->shutdown) {
      vt->get_display_driver()->shutdown();
   }

   _al_win_shutdown_time();

   if (using_higher_res_timer) {
      timeEndPeriod(1);
   }

   ASSERT(vt);
   al_free(vt);
   vt = NULL;

   ASSERT(_al_win_system);
   al_free(_al_win_system);
}


static ALLEGRO_DISPLAY_INTERFACE *win_get_display_driver(void)
{
   int flags = al_get_new_display_flags();
   ALLEGRO_SYSTEM *sys = al_get_system_driver();
   const char *s;
   ALLEGRO_DISPLAY_INTERFACE *ret = NULL;

#if defined ALLEGRO_CFG_D3D
   if (flags & ALLEGRO_DIRECT3D) {
      return _al_display_d3d_driver();
   }
#endif
#if defined ALLEGRO_CFG_OPENGL
   if (flags & ALLEGRO_OPENGL) {
      return _al_display_wgl_driver();
   }
#endif

   if (sys->config) {
      s = al_get_config_value(sys->config, "graphics", "driver");
      if (s) {
         if (!_al_stricmp(s, "OPENGL")) {
#if defined ALLEGRO_CFG_OPENGL
            flags |= ALLEGRO_OPENGL;
            al_set_new_display_flags(flags);
            return _al_display_wgl_driver();
#else
            return NULL;
#endif
         }
         else if (!_al_stricmp(s, "DIRECT3D") || !_al_stricmp(s, "D3D")) {
#if defined ALLEGRO_CFG_D3D
            flags |= ALLEGRO_DIRECT3D;
            al_set_new_display_flags(flags);
            return _al_display_d3d_driver();
#else
            return NULL;
#endif
         }
      }
   }

#if defined ALLEGRO_CFG_D3D
#ifdef ALLEGRO_CFG_OPENGL
   if (d3d_available) {
#endif
      flags |= ALLEGRO_DIRECT3D;
      al_set_new_display_flags(flags);
      ret = _al_display_d3d_driver();
      if (ret != NULL)
         return ret;
      flags &= ~ALLEGRO_DIRECT3D;
#ifdef ALLEGRO_CFG_OPENGL
   }
#endif
#endif
#if defined ALLEGRO_CFG_OPENGL
   flags |= ALLEGRO_OPENGL;
   al_set_new_display_flags(flags);
   return _al_display_wgl_driver();
#endif

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

static void win_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO *info)
{
   DISPLAY_DEVICE dd;
   DEVMODE dm;

   memset(&dd, 0, sizeof(dd));
   dd.cb = sizeof(dd);
   EnumDisplayDevices(NULL, adapter, &dd, 0);

   memset(&dm, 0, sizeof(dm));
   dm.dmSize = sizeof(dm);
   EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm);

   ASSERT(dm.dmFields & DM_PELSHEIGHT);
   ASSERT(dm.dmFields & DM_PELSWIDTH);
   /* Disabled this assertion for now as it fails under Wine 1.2. */
   /* ASSERT(dm.dmFields & DM_POSITION); */

   info->x1 = dm.dmPosition.x;
   info->y1 = dm.dmPosition.y;
   info->x2 = info->x1 + dm.dmPelsWidth;
   info->y2 = info->y1 + dm.dmPelsHeight;
}

static bool win_get_cursor_position(int *ret_x, int *ret_y)
{
   POINT p;
   GetCursorPos(&p);
   *ret_x = p.x;
   *ret_y = p.y;
   return true;
}


static ALLEGRO_MOUSE_DRIVER *win_get_mouse_driver(void)
{
   return _al_mouse_driver_list[0].driver;
}

/* _win_get_path:
 *  Returns full path to various system and user diretories
 */

static ALLEGRO_PATH *win_get_path(int id)
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

   if (id != ALLEGRO_USER_HOME_PATH) {
      al_append_path_component(cisdl_path, al_get_org_name());
      al_append_path_component(cisdl_path, al_get_app_name());
   }

   return cisdl_path;
}

static bool win_inhibit_screensaver(bool inhibit)
{
   _al_win_disable_screensaver = inhibit;
   return true;
}

static HMODULE load_library_at_path(ALLEGRO_PATH *path, const char *filename)
{
   const char *path_str;
   HMODULE lib;

   al_set_path_filename(path, filename);
   path_str = al_path_cstr(path, '\\');

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

static bool same_dir(ALLEGRO_PATH *dir1, ALLEGRO_PATH *dir2)
{
   const char *s1;
   const char *s2;
   int i, n1, n2;

   n1 = al_get_path_num_components(dir1);
   n2 = al_get_path_num_components(dir2);
   if (n1 != n2)
      return false;

   for (i = 0; i < n1; i++) {
      s1 = al_get_path_component(dir1, i);
      s2 = al_get_path_component(dir2, i);
      if (strcmp(s1, s2) != 0)
         return false;
   }

   s1 = al_get_path_drive(dir1);
   s2 = al_get_path_drive(dir2);
   if (!s1 || !s2 || strcmp(s1, s2) != 0)
      return false;

   return true;
}

static HMODULE maybe_load_library_at_cwd(ALLEGRO_PATH *path)
{
   char cwd_buf[MAX_PATH];
   ALLEGRO_PATH *cwd;
   const char *path_str;
   HMODULE lib = NULL;

   if (!is_build_config_name(al_get_path_tail(path)))
      return NULL;

   if (GetCurrentDirectoryA(sizeof(cwd_buf), cwd_buf) >= sizeof(cwd_buf)) {
      ALLEGRO_WARN("GetCurrentDirectoryA failed\n");
      return NULL;
   }

   al_drop_path_tail(path);
   path_str = al_path_cstr(path, '\\');

   cwd = al_create_path_for_directory(cwd_buf);
   if (same_dir(path, cwd)) {
      ALLEGRO_DEBUG("Assuming MSVC build directory, trying %s\n", path_str);
      lib = LoadLibraryA(path_str);
   }
   al_destroy_path(cwd);

   return lib;
}

/*
 * Calling LoadLibrary with a relative file name is a security risk:
 * see e.g. Microsoft Security Advisory (2269637)
 * "Insecure Library Loading Could Allow Remote Code Execution"
 */
HMODULE _al_win_safe_load_library(const char *filename)
{
   char buf[MAX_PATH];
   HMODULE lib;
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
    * executable.
    */
   if (GetModuleFileName(NULL, buf, sizeof(buf)) < sizeof(buf)) {
      ALLEGRO_PATH *path = al_create_path(buf);
      lib = load_library_at_path(path, filename);
      if (!lib && msvc_only) {
         lib = maybe_load_library_at_cwd(path);
      }
      al_destroy_path(path);
      if (lib)
         return lib;
   }

   /* Try to load the library from the Windows system directory. */
   if (GetSystemDirectoryA(buf, sizeof(buf)) < sizeof(buf)) {
      ALLEGRO_PATH *path = al_create_path_for_directory(buf);
      lib = load_library_at_path(path, filename);
      al_destroy_path(path);
      if (lib)
         return lib;
   }

   /* Do NOT try to load the library from the current directory, or
    * directories on the PATH.
    */

   return NULL;
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

   vt = al_malloc(sizeof *vt);
   memset(vt, 0, sizeof *vt);

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
   vt->get_path = win_get_path;
   vt->inhibit_screensaver = win_inhibit_screensaver;
   vt->open_library = win_open_library;
   vt->import_symbol = win_import_symbol;
   vt->close_library = win_close_library;

   return vt;
}

void _al_register_system_interfaces()
{
   ALLEGRO_SYSTEM_INTERFACE **add;

#if defined ALLEGRO_CFG_D3D || defined ALLEGRO_CFG_OPENGL
   add = _al_vector_alloc_back(&_al_system_interfaces);
   *add = _al_system_win_driver();
#endif
}

/* vim: set sts=3 sw=3 et: */
