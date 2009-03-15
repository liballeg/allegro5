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


static ALLEGRO_SYSTEM *active_sysdrv = NULL;

_AL_VECTOR _al_system_interfaces = _AL_VECTOR_INITIALIZER(ALLEGRO_SYSTEM_INTERFACE *);
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
   }
}



/* Function: al_install_system
 */
bool al_install_system(int (*atexit_ptr)(void (*)(void)))
{
#ifdef ALLEGRO_UNIX
   ALLEGRO_CONFIG *temp;
   char buf[256], tmp[256];
#endif
   
   if (active_sysdrv) {
      return true;
   }

#ifdef ALLEGRO_BCC32
   /* This supresses exceptions on floating point divide by zero */
   _control87(MCW_EM, MCW_EM);
#endif

   /* Register builtin system drivers */
   _al_register_system_interfaces();

   /* Check for a user-defined system driver first */
   active_sysdrv = find_system(&_user_system_interfaces);

   /* If a user-defined driver is not found, look for a builtin one */
   if (active_sysdrv == NULL) {
      active_sysdrv = find_system(&_al_system_interfaces);
   }

   if (active_sysdrv == NULL) {
      return false;
   }

   if(ustrcmp(al_get_orgname(), "") == 0) {
      al_set_orgname(NULL);
   }

   if(ustrcmp(al_get_appname(), "") == 0) {
      al_set_appname(NULL);
   }
   
#ifdef ALLEGRO_UNIX
   active_sysdrv->config = al_load_config_file("/etc/allegrorc");
   if (active_sysdrv->config) {
      TRACE("Applying system settings from /etc/allegrorc\n");
   }

   ustrzcpy(buf, sizeof(buf) - ucwidth(OTHER_PATH_SEPARATOR), uconvert_ascii(getenv("HOME"), tmp));
   ustrzcat(buf, sizeof(buf), uconvert_ascii("/.allegrorc", tmp));
   temp = al_load_config_file(buf);
   if (temp) {
      TRACE("Applying system settings from %s\n", buf);
      if (active_sysdrv->config) {
         al_merge_config_into(active_sysdrv->config, temp);
         al_destroy_config(temp);
      }
      else {
         active_sysdrv->config = temp;
      }
   }
   
   temp = al_load_config_file("allegro.cfg");
   if (temp) {
      TRACE("Applying system settings from allegro.cfg\n");
      if (active_sysdrv->config) {
         al_merge_config_into(active_sysdrv->config, temp);
         al_destroy_config(temp);
      }
      else {
         active_sysdrv->config = temp;
      }
   }
#else
   active_sysdrv->config = al_load_config_file("allegro.cfg");
#endif

   _al_add_exit_func(shutdown_system_driver, "shutdown_system_driver");

   _al_dtor_list = _al_init_destructors();

   _al_init_events();

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

/* Function: al_get_path
 */
AL_CONST char *al_get_path(uint32_t id, char *path, size_t size)
{
   ASSERT(active_sysdrv);

   if (active_sysdrv->vt->get_path)
      return active_sysdrv->vt->get_path(id, path, size);

   return NULL;
}


/* Function: al_set_orgname
 */
void al_set_orgname(AL_CONST char *orgname)
{
   if(orgname)
      ustrzcpy(_al_orgname, sizeof(_al_orgname), orgname);
   else
      ustrzcpy(_al_orgname, sizeof(_al_orgname), "allegro");
}

/* Function: al_set_appname
 */
void al_set_appname(AL_CONST char *appname)
{
   if(appname) {
      ustrzcpy(_al_appname, sizeof(_al_appname), appname);
   }
   else {
      char _appname_tmp[PATH_MAX];
      ALLEGRO_PATH *_appname_path = NULL;

      al_get_path(AL_EXENAME_PATH, _appname_tmp, PATH_MAX);
      _appname_path = al_path_create(_appname_tmp);
      ustrzcpy(_al_appname, sizeof(_al_appname), al_path_get_filename(_appname_path) );
      al_path_free(_appname_path);
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
