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

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/platform/aintwin.h"

#if defined ALLEGRO_CFG_OPENGL
   #include "allegro5/a5_opengl.h"
#endif
#if defined ALLEGRO_CFG_D3D
   #include "allegro5/a5_direct3d.h"
#endif

#include <windows.h>


#ifndef SCAN_DEPEND
   #include <mmsystem.h>
#endif

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
   argbuf = _AL_MALLOC(i);
   memcpy(argbuf, cmdline, i);

   argc = 0;
   argc_max = 64;
   argv = _AL_MALLOC(sizeof(char *) * argc_max);
   if (!argv) {
      _AL_FREE(argbuf);
      return 1;
   }

   i = 0;

   /* parse commandline into argc/argv format */
   while (argbuf[i]) {
      while ((argbuf[i]) && (uisspace(argbuf[i])))
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
            argv = _AL_REALLOC(argv, sizeof(char *) * argc_max);
            if (!argv) {
               _AL_FREE(argbuf);
               return 1;
            }
         }

         while ((argbuf[i]) && ((q) ? (argbuf[i] != q) : (!uisspace(argbuf[i]))))
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

   _AL_FREE(argv);
   _AL_FREE(argbuf);

   return i;
}




/* Create a new system object. */
static ALLEGRO_SYSTEM *win_initialize(int flags)
{
   (void)flags;

   _al_win_system = _AL_MALLOC(sizeof *_al_win_system);
   memset(_al_win_system, 0, sizeof *_al_win_system);

   // Request a 1ms resolution from our timer
   if (timeBeginPeriod(1) != TIMERR_NOCANDO) {
      using_higher_res_timer = true;
   }
   _al_win_init_time();

   _al_win_input_init();

   _al_win_init_window();

   _al_vector_init(&_al_win_system->system.displays, sizeof (ALLEGRO_SYSTEM_WIN *));

   _al_win_system->system.vt = vt;

#if defined ALLEGRO_CFG_D3D
   if (_al_d3d_init_display() != true)
      return NULL;
#endif
   
   return &_al_win_system->system;
}


static void win_shutdown(void)
{
   /* Close all open displays. */
   ALLEGRO_SYSTEM *s = al_system_driver();
   while (_al_vector_size(&s->displays) > 0) {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&s->displays, 0);
      ALLEGRO_DISPLAY *d = *dptr;
      _al_destroy_display_bitmaps(d);
      al_destroy_display(d);
   }

   if (vt->get_display_driver && vt->get_display_driver()->shutdown) {
      vt->get_display_driver()->shutdown();
   }

   _al_win_shutdown_time();

    _al_win_input_exit();

   if (using_higher_res_timer) {
      timeEndPeriod(1);
   }
}


static ALLEGRO_DISPLAY_INTERFACE *win_get_display_driver(void)
{
   int flags = al_get_new_display_flags();
   ALLEGRO_SYSTEM *sys = al_system_driver();
   AL_CONST char *s;

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
      s = al_config_get_value(sys->config, "graphics", "driver");
      if (s) {
         if (!stricmp(s, "OPENGL")) {
#if defined ALLEGRO_CFG_OPENGL
            flags |= ALLEGRO_OPENGL;
            al_set_new_display_flags(flags);
            return _al_display_wgl_driver();
#else
            return NULL;
#endif
         }
         else if (!stricmp(s, "DIRECT3D") || !stricmp(s, "D3D")) {
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
      flags |= ALLEGRO_DIRECT3D;
      al_set_new_display_flags(flags);
      return _al_display_d3d_driver();
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
   int format = al_get_new_display_format();
   int refresh_rate = al_get_new_display_refresh_rate();
   int flags = al_get_new_display_flags();

   if (!(flags & ALLEGRO_FULLSCREEN))
      return -1;

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
   int format = al_get_new_display_format();
   int refresh_rate = al_get_new_display_refresh_rate();
   int flags = al_get_new_display_flags();

   if (!(flags & ALLEGRO_FULLSCREEN))
      return NULL;

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
   int flags = al_get_new_display_flags();

#if defined ALLEGRO_CFG_OPENGL
   if (flags & ALLEGRO_OPENGL) {
      return _al_wgl_get_num_video_adapters();
   }
#endif

#if defined ALLEGRO_CFG_D3D
   return _al_d3d_get_num_video_adapters();
#endif

   return 0;
}

static void win_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO *info)
{
   int flags = al_get_new_display_flags();

#if defined ALLEGRO_CFG_OPENGL
   if (flags & ALLEGRO_OPENGL) {
      _al_wgl_get_monitor_info(adapter, info);
   }
#endif

#if defined ALLEGRO_CFG_D3D
   _al_d3d_get_monitor_info(adapter, info);
#endif
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

static AL_CONST char *win_get_path(uint32_t id, char *dir, size_t size)
{
   char path[MAX_PATH];
   uint32_t csidl = 0;
   HRESULT ret = 0;

   memset(dir, 0, size);

   switch (id) {
      case AL_TEMP_PATH: {
         /* Check: TMP, TMPDIR, TEMP or TEMPDIR */
         DWORD ret = GetTempPath(MAX_PATH, path);
         if (ret > MAX_PATH) {
            /* should this ever happen, windows is more broken than I ever thought */
            return dir;
         }

         do_uconvert(path, U_ASCII, dir, U_CURRENT, strlen(path)+1);
         return dir;

      } break;

      case AL_PROGRAM_PATH: { /* where the program is in */
         HANDLE process = GetCurrentProcess();
         char *ptr;
         GetModuleFileNameEx(process, NULL, path, MAX_PATH);
         ptr = strrchr(path, '\\');
         if (!ptr) { /* shouldn't happen */
            return dir;
         }

         /* chop off everything including and after the last slash */
         /* should this not chop the slash? */
         *ptr = '\0';

         do_uconvert(path, U_ASCII, dir, U_CURRENT, strlen(path)+1);
         ustrcat(dir, "\\");
         return dir;
      } break;

      case AL_SYSTEM_DATA_PATH: /* CSIDL_COMMON_APPDATA */
         csidl = CSIDL_COMMON_APPDATA;
         break;

      case AL_USER_DATA_PATH: /* CSIDL_APPDATA */
         csidl = CSIDL_APPDATA;
         break;

      case AL_USER_HOME_PATH: /* CSIDL_PERSONAL */
         csidl = CSIDL_PERSONAL;
         break;

      case AL_USER_SETTINGS_PATH:
         /* CHECKME: is this correct? Windows doesn't seem to have a "path"
          * for settings; I guess these should go in the registry instead?
          */
         csidl = CSIDL_APPDATA;
         break;

      case AL_SYSTEM_SETTINGS_PATH:
         /* CHECKME: is this correct? Windows doesn't seem to have a "path"
          * for settings; I guess these should go in the registry instead?
          */
         csidl = CSIDL_COMMON_APPDATA;
         break;

      default:
         return dir;
   }

   ret = SHGetFolderPath(NULL, csidl, NULL, SHGFP_TYPE_CURRENT, path);
   if (ret != S_OK) {
      return dir;
   }

   do_uconvert(path, U_ASCII, dir, U_CURRENT, strlen(path)+1);
   ustrcat(dir, "\\");
   
   return dir;
}

static bool win_inhibit_screensaver(bool inhibit)
{
   _al_win_disable_screensaver = inhibit;
   return true;
}


static ALLEGRO_SYSTEM_INTERFACE *_al_system_win_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
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
   vt->get_monitor_info = win_get_monitor_info;
   vt->get_cursor_position = win_get_cursor_position;
   vt->get_path = win_get_path;
   vt->inhibit_screensaver = win_inhibit_screensaver;

   TRACE("ALLEGRO_SYSTEM_INTERFACE created.\n");

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
