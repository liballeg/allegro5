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

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
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
      ALLEGRO_CONFIG *temp = active_sysdrv->config;
      if (active_sysdrv->user_exe_path)
         al_destroy_path(active_sysdrv->user_exe_path);
      if (active_sysdrv->vt && active_sysdrv->vt->shutdown_system)
         active_sysdrv->vt->shutdown_system();
      active_sysdrv = NULL;
      /* active_sysdrv is not accessible here so we copied it */
      al_destroy_config(temp);
 
      while (!_al_vector_is_empty(&_al_system_interfaces))
         _al_vector_delete_at(&_al_system_interfaces, _al_vector_size(&_al_system_interfaces)-1);
      _al_vector_free(&_al_system_interfaces);
      _al_vector_init(&_al_system_interfaces, sizeof(ALLEGRO_SYSTEM_INTERFACE *));
   }
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
#else
   #error early_get_exename_path not implemented
#endif
}



static void read_allegro_cfg(void)
{
   /* We cannot use any logging in this function as it would cause the
    * logging system to be initialised before all the relevant config files
    * have been read in.
    */

   /* We assume that the stdio file interface is in effect. */

   ALLEGRO_PATH *path;
   ALLEGRO_CONFIG *temp;

   active_sysdrv->config = NULL;

#if defined(ALLEGRO_UNIX) && !defined(ALLEGRO_GP2XWIZ) && !defined(ALLEGRO_IPHONE)
   active_sysdrv->config = al_load_config_file("/etc/allegro5rc");

   path = _al_unix_get_path(ALLEGRO_USER_HOME_PATH);
   if (path) {
      al_set_path_filename(path, "allegro5rc");
      temp = al_load_config_file(al_path_cstr(path, '/'));
      if (temp) {
         if (active_sysdrv->config) {
            al_merge_config_into(active_sysdrv->config, temp);
            al_destroy_config(temp);
         }
         else {
            active_sysdrv->config = temp;
         }
      }
      al_destroy_path(path);
   }
#endif

   path = early_get_exename_path();
   if (path) {
      al_set_path_filename(path, "allegro5.cfg");
      temp = al_load_config_file(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP));
      if (temp) {
         if (active_sysdrv->config) {
            al_merge_config_into(active_sysdrv->config, temp);
            al_destroy_config(temp);
         }
         else {
            active_sysdrv->config = temp;
         }
      }
      al_destroy_path(path);
   }

   /* Always have a configuration available whether or not a config file
    * exists.
    */
   if (!active_sysdrv->config)
      active_sysdrv->config = al_create_config();
}



/*
 * Let a = xa.ya.za.*
 * Let b = xb.yb.zb.*
 *
 * When ya is odd, a is compatible with b if xa.ya.za = xb.yb.zb.
 * When ya is even, a is compatible with b if xa.ya = xb.yb.
 *
 * Otherwise a and b are incompatible.
 */
static bool compatible_versions(int a, int b)
{
   if ((a & 0xffff0000) != (b & 0xffff0000)) {
      return false;
   }
   if (((a & 0x00ff0000) >> 16) & 1) {
      
      if ((a & 0x0000ff00) != (b & 0x0000ff00)) {
         return false;
      }
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

   _al_vector_init(&_al_system_interfaces, sizeof(ALLEGRO_SYSTEM_INTERFACE *));

   /* We want active_sysdrv->config to be available as soon as
    * possible - for example what if a system driver need to read
    * settings out of there. Hence we use a dummy ALLEGRO_SYSTEM
    * here to load the initial config.
    */
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
   active_sysdrv->config = bootstrap.config;

   ALLEGRO_INFO("Allegro version: %s\n", ALLEGRO_VERSION_STR);

   if (strcmp(al_get_app_name(), "") == 0) {
      al_set_app_name(NULL);
   }

   _al_add_exit_func(shutdown_system_driver, "shutdown_system_driver");

   _al_dtor_list = _al_init_destructors();

   _al_init_events();

   _al_init_pixels();

   _al_init_iio_table();

   _al_init_timers();

   if (atexit_ptr && atexit_virgin) {
      atexit_ptr(al_uninstall_system);
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
   _al_run_destructors(_al_dtor_list);
   _al_run_exit_funcs();
   _al_shutdown_destructors(_al_dtor_list);
   _al_dtor_list = NULL;
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


/* Function: al_get_system_config
 */
ALLEGRO_CONFIG *al_get_system_config(void)
{
   return (active_sysdrv) ? active_sysdrv->config : NULL;
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

/* vim: set sts=3 sw=3 et: */
