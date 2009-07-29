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

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_vector.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/internal/aintern_thread.h"

ALLEGRO_DEBUG_CHANNEL("system")

static ALLEGRO_SYSTEM *active_sysdrv = NULL;

_AL_VECTOR _al_system_interfaces;
static _AL_VECTOR _user_system_interfaces = _AL_VECTOR_INITIALIZER(ALLEGRO_SYSTEM_INTERFACE *);

_AL_DTOR_LIST *_al_dtor_list = NULL;

static bool atexit_virgin = true;

/* too large? */
static char _al_appname[1024] = "";
static char _al_orgname[1024] = "";

#if 0
bool al_register_system_driver(ALLEGRO_SYSTEM_INTERFACE *sys_interface)
{
   ALLEGRO_SYSTEM_INTERFACE **add = _al_vector_alloc_back(&_user_system_interfaces);
   if (!add)
      return false;
   *add = sys_interface;
   return true;
}
#endif



#if 0
void al_unregister_system_driver(ALLEGRO_SYSTEM_INTERFACE *sys_interface)
{
   _al_vector_find_and_delete(&_user_system_interfaces, &sys_interface);
}
#endif



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



static void read_allegro_cfg(void)
{
   /* We cannot use any logging in this function as it would cause the
    * logging system to be initialised before all the relevant config files
    * have been read in.
    */

#if defined ALLEGRO_UNIX && !defined ALLEGRO_GP2XWIZ
   ALLEGRO_CONFIG *temp;
   ALLEGRO_PATH *path;

   active_sysdrv->config = al_load_config_file("/etc/allegro5rc");

   path = _al_unix_get_path(ALLEGRO_USER_HOME_PATH);
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
   al_free_path(path);

   temp = al_load_config_file("allegro5.cfg");
   if (temp) {
      if (active_sysdrv->config) {
         al_merge_config_into(active_sysdrv->config, temp);
         al_destroy_config(temp);
      }
      else {
         active_sysdrv->config = temp;
      }
   }
#else
   active_sysdrv->config = al_load_config_file("allegro5.cfg");
#endif
}



/* Function: al_install_system
 */
bool al_install_system(int (*atexit_ptr)(void (*)(void)))
{
   ALLEGRO_SYSTEM bootstrap;
   ALLEGRO_SYSTEM *real_system;
   
   if (active_sysdrv) {
      return true;
   }

#ifdef ALLEGRO_CFG_PTHREADS_TLS
   _al_pthreads_tls_init();
#endif
   
   _al_vector_init(&_al_system_interfaces, sizeof(ALLEGRO_SYSTEM_INTERFACE *));

   /* We want active_sysdrv->config to be available as soon as
    * possible - for example what if a system driver need to read
    * settings out of there. Hence we use a dummy ALLEGRO_SYSTEM
    * here to load the initial config.
    */
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

   if(strcmp(al_get_orgname(), "") == 0) {
      al_set_orgname(NULL);
   }

   if(strcmp(al_get_appname(), "") == 0) {
      al_set_appname(NULL);
   }

   _al_add_exit_func(shutdown_system_driver, "shutdown_system_driver");

   _al_dtor_list = _al_init_destructors();

   _al_init_events();

   _al_init_pixels();

   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, al_map_rgb(255, 255, 255));

   if (atexit_ptr && atexit_virgin) {
      atexit_ptr(al_uninstall_system);
      atexit_virgin = false;
   }

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


   /* shutdown_system_driver is registered as an exit func so we don't need
    * to do any more here.
    */

   ASSERT(active_sysdrv == NULL);
}



/* Function: al_system_driver
 */
ALLEGRO_SYSTEM *al_system_driver(void)
{
   return active_sysdrv;
}


/* Function: al_get_standard_path
 */
ALLEGRO_PATH *al_get_standard_path(int id)
{
   ASSERT(active_sysdrv);

   if (active_sysdrv->vt->get_path)
      return active_sysdrv->vt->get_path(id);

   return NULL;
}


/* Function: al_set_orgname
 */
void al_set_orgname(AL_CONST char *orgname)
{
   if(orgname)
      strncpy(_al_orgname, orgname, sizeof(_al_orgname));
   else
      strncpy(_al_orgname, "allegro", sizeof(_al_orgname));
}


/* Function: al_set_appname
 */
void al_set_appname(AL_CONST char *appname)
{
   if(appname) {
      strncpy(_al_appname, appname, sizeof(_al_appname));
   }
   else {
      ALLEGRO_PATH *_appname_path;
      _appname_path = al_get_standard_path(ALLEGRO_EXENAME_PATH);
      strncpy(_al_appname, al_get_path_filename(_appname_path), sizeof(_al_appname));
      al_free_path(_appname_path);
   }
}


/* Function: al_get_orgname
 */
AL_CONST char *al_get_orgname(void)
{
   return _al_orgname;
}


/* Function: al_get_appname
 */
AL_CONST char *al_get_appname(void)
{
   return _al_appname;
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


/* vim: set sts=3 sw=3 et: */
