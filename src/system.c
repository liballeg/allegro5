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
 *      New system driver.
 *
 *      By Elias Pschernig.
 *
 *      Modified by Trent Gamblin.
 */

/* Title: System routines
 */
#include <stdio.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#ifdef ALLEGRO_CFG_SHADER_GLSL
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_opengl.h"
#endif
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_debug.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_thread.h"
#include "allegro5/internal/aintern_timer.h"
#include "allegro5/internal/aintern_tls.h"
#include "allegro5/internal/aintern_vector.h"

ALLEGRO_DEBUG_CHANNEL("system")

static ALLEGRO_SYSTEM *active_sysdrv = NULL;
static ALLEGRO_CONFIG *sys_config = NULL;

_AL_VECTOR _al_system_interfaces;
static _AL_VECTOR _user_system_interfaces = _AL_VECTOR_INITIALIZER(ALLEGRO_SYSTEM_INTERFACE *);

_AL_DTOR_LIST *_al_dtor_list = NULL;

static bool atexit_virgin = true;

static char _al_app_name[256] = "";
static char _al_org_name[256] = "";



static ALLEGRO_SYSTEM *find_system(_AL_VECTOR *vector)
{
   ALLEGRO_SYSTEM_INTERFACE **sptr;
   ALLEGRO_SYSTEM_INTERFACE *sys_interface;
   ALLEGRO_SYSTEM *system;
   unsigned int i;

   for (i = 0; i < vector->_size; i++) {
      sptr = _al_vector_ref(vector, i);
      sys_interface = *sptr;
      if ((system = sys_interface->initialize(0)) != NULL)
         return system;
   }

   return NULL;
}



static void shutdown_system_driver(void)
{
   if (active_sysdrv) {
      if (active_sysdrv->user_exe_path)
         al_destroy_path(active_sysdrv->user_exe_path);
      if (active_sysdrv->vt && active_sysdrv->vt->shutdown_system)
         active_sysdrv->vt->shutdown_system();
      active_sysdrv = NULL;

      while (!_al_vector_is_empty(&_al_system_interfaces))
         _al_vector_delete_at(&_al_system_interfaces, _al_vector_size(&_al_system_interfaces)-1);
      _al_vector_free(&_al_system_interfaces);
      _al_vector_init(&_al_system_interfaces, sizeof(ALLEGRO_SYSTEM_INTERFACE *));
   }
   al_destroy_config(sys_config);
   sys_config = NULL;
}



/* al_get_standard_path() does not work before the system driver is
 * initialised.  Before that, we need to call the underlying functions
 * directly.
 */
static ALLEGRO_PATH *early_get_exename_path(void)
{
#if defined(ALLEGRO_WINDOWS)
   return _al_win_get_path(ALLEGRO_EXENAME_PATH);
#elif defined(ALLEGRO_MACOSX)
   return _al_osx_get_path(ALLEGRO_EXENAME_PATH);
#elif defined(ALLEGRO_IPHONE)
   return _al_iphone_get_path(ALLEGRO_EXENAME_PATH);
#elif defined(ALLEGRO_UNIX)
   return _al_unix_get_path(ALLEGRO_EXENAME_PATH);
#elif defined(ALLEGRO_ANDROID)
   return _al_android_get_path(ALLEGRO_EXENAME_PATH);
#elif defined(ALLEGRO_SDL)
   char* p = SDL_GetBasePath();
   ALLEGRO_PATH *path = al_create_path_for_directory(p);
   SDL_free(p);
   return path;
#else
   #error early_get_exename_path not implemented
#endif
}



static void read_allegro_cfg(void)
{
   /* We assume that the stdio file interface is in effect. */

   ALLEGRO_PATH *path;
   ALLEGRO_CONFIG *temp;

   if (!sys_config)
      sys_config = al_create_config();

#if defined(ALLEGRO_UNIX) && !defined(ALLEGRO_IPHONE)
   temp = al_load_config_file("/etc/allegro5rc");
   if (temp) {
      al_merge_config_into(sys_config, temp);
      al_destroy_config(temp);
   }

   path = _al_unix_get_path(ALLEGRO_USER_HOME_PATH);
   if (path) {
      al_set_path_filename(path, "allegro5rc");
      temp = al_load_config_file(al_path_cstr(path, '/'));
      if (temp) {
         al_merge_config_into(sys_config, temp);
         al_destroy_config(temp);
      }
      al_set_path_filename(path, ".allegro5rc");
      temp = al_load_config_file(al_path_cstr(path, '/'));
      if (temp) {
         al_merge_config_into(sys_config, temp);
         al_destroy_config(temp);
      }
      al_destroy_path(path);
   }
#endif

   path = early_get_exename_path();
   if (path) {
      al_set_path_filename(path, "allegro5.cfg");
      temp = al_load_config_file(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP));
      if (temp) {
         al_merge_config_into(sys_config, temp);
         al_destroy_config(temp);
      }
      al_destroy_path(path);
   }
   /* Reconfigure logging in case something changed. */
   _al_configure_logging();
}



/*
 * Can a binary with version a use a library with version b?
 *
 * Let a = xa.ya.za.*
 * Let b = xb.yb.zb.*
 *
 * When a has the unstable bit, a is compatible with b if xa.ya.za = xb.yb.zb.
 * Otherwise, a is compatible with b if xa.ya = xb.yb and zb >= za.
 *
 * Otherwise a and b are incompatible.
 */
static bool compatible_versions(int a, int b)
{
   int a_unstable = a & _ALLEGRO_UNSTABLE_BIT_SET;

   int a_major = (a & 0x7f000000) >> 24;
   int a_sub   = (a & 0x00ff0000) >> 16;
   int a_wip   = (a & 0x0000ff00) >> 8;

   int b_major = (b & 0x7f000000) >> 24;
   int b_sub   = (b & 0x00ff0000) >> 16;
   int b_wip   = (b & 0x0000ff00) >> 8;

   if (a_major != b_major) {
      return false;
   }
   if (a_unstable && a_wip != b_wip) {
      return false;
   }
   if (a_sub != b_sub) {
      return false;
   }
   if (a_wip > b_wip) {
      return false;
   }
   return true;
}



/* Function: al_install_system
 */
bool al_install_system(int version, int (*atexit_ptr)(void (*)(void)))
{
   ALLEGRO_SYSTEM bootstrap;
   ALLEGRO_SYSTEM *real_system;
   int library_version = al_get_allegro_version();

   if (active_sysdrv) {
      return true;
   }

   /* Note: We cannot call logging functions yet.
    * TODO: Maybe we want to do the check after the "bootstrap" system
    * is available at least?
    */
   if (!compatible_versions(version, library_version))
      return false;

   _al_tls_init_once();
   _al_reinitialize_tls_values();

   _al_vector_init(&_al_system_interfaces, sizeof(ALLEGRO_SYSTEM_INTERFACE *));

   /* Set up a bootstrap system so the calls expecting it don't freak out */
   memset(&bootstrap, 0, sizeof(bootstrap));
   active_sysdrv = &bootstrap;
   read_allegro_cfg();

#ifdef ALLEGRO_BCC32
   /* This supresses exceptions on floating point divide by zero */
   _control87(MCW_EM, MCW_EM);
#endif

   /* Register builtin system drivers */
   _al_register_system_interfaces();

   /* Check for a user-defined system driver first */
   real_system = find_system(&_user_system_interfaces);

   /* If a user-defined driver is not found, look for a builtin one */
   if (real_system == NULL) {
      real_system = find_system(&_al_system_interfaces);
   }

   if (real_system == NULL) {
      active_sysdrv = NULL;
      return false;
   }

   active_sysdrv = real_system;
   active_sysdrv->mouse_wheel_precision = 1;

   const char *min_bitmap_size = al_get_config_value(
      al_get_system_config(), "graphics", "min_bitmap_size");
   active_sysdrv->min_bitmap_size = min_bitmap_size ? atoi(min_bitmap_size) : 16;

   ALLEGRO_INFO("Allegro version: %s\n", ALLEGRO_VERSION_STR);

   if (strcmp(al_get_app_name(), "") == 0) {
      al_set_app_name(NULL);
   }

   _al_add_exit_func(shutdown_system_driver, "shutdown_system_driver");

   _al_dtor_list = _al_init_destructors();

   _al_init_events();

   _al_init_iio_table();

   _al_init_convert_bitmap_list();

   _al_init_timers();

#ifdef ALLEGRO_CFG_SHADER_GLSL
   _al_glsl_init_shaders();
#endif

   if (active_sysdrv->vt->heartbeat_init)
      active_sysdrv->vt->heartbeat_init();

   if (atexit_ptr && atexit_virgin) {
#ifndef ALLEGRO_ANDROID
      atexit_ptr(al_uninstall_system);
#endif
      atexit_virgin = false;
   }

   /* Clear errnos set while searching for config files. */
   al_set_errno(0);

   active_sysdrv->installed = true;

   _al_srand(time(NULL));

   return true;
}

/* Function: al_uninstall_system
 */
void al_uninstall_system(void)
{
   /* Note: al_uninstall_system may get called multiple times without an
    * al_install_system in between. For example if the user manually
    * calls it at the end of the program it is called right again
    * because it's installed as an atexit function by al_init.
    */

   _al_run_destructors(_al_dtor_list);
   _al_run_exit_funcs();
   _al_shutdown_destructors(_al_dtor_list);
   _al_dtor_list = NULL;

#ifdef ALLEGRO_CFG_SHADER_GLSL
   _al_glsl_shutdown_shaders();
#endif

   _al_shutdown_logging();

   /* shutdown_system_driver is registered as an exit func so we don't need
    * to do any more here.
    */

   ASSERT(active_sysdrv == NULL);
}



/* Function: al_is_system_installed
 */
bool al_is_system_installed(void)
{
   return (active_sysdrv && active_sysdrv->installed) ? true : false;
}


/* Hidden function: al_get_system_driver
 *  This was exported and documented in 5.0rc1 but probably shouldn't have been
 *  as ALLEGRO_SYSTEM is not documented.
 */
ALLEGRO_SYSTEM *al_get_system_driver(void)
{
   return active_sysdrv;
}

/* Function: al_get_system_id
 */
ALLEGRO_SYSTEM_ID al_get_system_id(void)
{
   ASSERT(active_sysdrv);
   return active_sysdrv->vt->id;
}

/* Function: al_get_system_config
 */
ALLEGRO_CONFIG *al_get_system_config(void)
{
   if (!sys_config)
      sys_config = al_create_config();
   return sys_config;
}


/* Function: al_get_standard_path
 */
ALLEGRO_PATH *al_get_standard_path(int id)
{
   ASSERT(active_sysdrv);
   ASSERT(active_sysdrv->vt);
   ASSERT(active_sysdrv->vt->get_path);

   if (id == ALLEGRO_EXENAME_PATH && active_sysdrv->user_exe_path)
      return al_clone_path(active_sysdrv->user_exe_path);

   if (id == ALLEGRO_RESOURCES_PATH && active_sysdrv->user_exe_path) {
      ALLEGRO_PATH *exe_dir = al_clone_path(active_sysdrv->user_exe_path);
      al_set_path_filename(exe_dir, NULL);
      return exe_dir;
   }

   if (active_sysdrv->vt->get_path)
      return active_sysdrv->vt->get_path(id);

   return NULL;
}


/* Function: al_set_exe_name
 */
void al_set_exe_name(char const *path)
{
   ASSERT(active_sysdrv);
   if (active_sysdrv->user_exe_path) {
      al_destroy_path(active_sysdrv->user_exe_path);
   }
   active_sysdrv->user_exe_path = al_create_path(path);
}


/* Function: al_set_org_name
 */
void al_set_org_name(const char *org_name)
{
   if (!org_name)
      org_name = "";

   _al_sane_strncpy(_al_org_name, org_name, sizeof(_al_org_name));
}


/* Function: al_set_app_name
 */
void al_set_app_name(const char *app_name)
{
   if (app_name) {
      _al_sane_strncpy(_al_app_name, app_name, sizeof(_al_app_name));
   }
   else {
      ALLEGRO_PATH *path;
      path = al_get_standard_path(ALLEGRO_EXENAME_PATH);
      _al_sane_strncpy(_al_app_name, al_get_path_filename(path), sizeof(_al_app_name));
      al_destroy_path(path);
   }
}


/* Function: al_get_org_name
 */
const char *al_get_org_name(void)
{
   return _al_org_name;
}


/* Function: al_get_app_name
 */
const char *al_get_app_name(void)
{
   return _al_app_name;
}


/* Function: al_inhibit_screensaver
 */
bool al_inhibit_screensaver(bool inhibit)
{
   ASSERT(active_sysdrv);

   if (active_sysdrv->vt->inhibit_screensaver)
      return active_sysdrv->vt->inhibit_screensaver(inhibit);
   else
      return false;
}


void *_al_open_library(const char *filename)
{
   ASSERT(active_sysdrv);

   if (active_sysdrv->vt->open_library)
      return active_sysdrv->vt->open_library(filename);
   else
      return NULL;
}


void *_al_import_symbol(void *library, const char *symbol)
{
   ASSERT(active_sysdrv);

   if (active_sysdrv->vt->import_symbol)
      return active_sysdrv->vt->import_symbol(library, symbol);
   else
      return NULL;
}


void _al_close_library(void *library)
{
   ASSERT(active_sysdrv);

   if (active_sysdrv->vt->close_library)
      active_sysdrv->vt->close_library(library);
}


/* Function: al_get_time
 */
double al_get_time(void)
{
   ASSERT(active_sysdrv);

   if (active_sysdrv->vt->get_time)
      return active_sysdrv->vt->get_time();
   return 0.0;
}


/* Function: al_rest
 */
void al_rest(double seconds)
{
   ASSERT(active_sysdrv);

   if (active_sysdrv->vt->rest)
      active_sysdrv->vt->rest(seconds);
}


/* Function: al_init_timeout
 */
void al_init_timeout(ALLEGRO_TIMEOUT *timeout, double seconds)
{
   ASSERT(active_sysdrv);

   if (active_sysdrv->vt->init_timeout)
      active_sysdrv->vt->init_timeout(timeout, seconds);
}


static uint32_t get_joystick_compat_version(const char* key)
{
   ALLEGRO_CONFIG *system_config = al_get_system_config();
   const char* compat_version = al_get_config_value(system_config, "compatibility", key);
   if (!compat_version || strlen(compat_version) == 0)
      return al_get_allegro_version();
   int version = 0;
   int sub_version = 0;
   int wip_version = 0;
   /* Ignore the release number, we don't expect that to make a difference */
   sscanf(compat_version, "%2d.%2d.%2d", &version, &sub_version, &wip_version);
   return AL_ID(version, sub_version, wip_version, 0);
}


uint32_t _al_get_joystick_compat_version(void)
{
   return get_joystick_compat_version("joystick_version");
}

uint32_t _al_get_keyboard_compat_version(void)
{
   return get_joystick_compat_version("keyboard_version");
}

/* vim: set sts=3 sw=3 et: */
