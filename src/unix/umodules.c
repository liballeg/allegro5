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
 *      Dynamically loaded modules.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"


#ifdef ALLEGRO_WITH_MODULES

#include <dlfcn.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>



typedef struct MODULE
{
   void *handle;
   struct MODULE *next;
} MODULE;


/* list of loaded modules */
static MODULE *module_list = NULL;


/* where to look for modules.lst */
static char *module_path[] =
{
   "/usr/local/lib/allegro/", "/usr/lib/allegro/", NULL
};



/* strip:
 *  Strips leading and trailing whitespace from an ASCII string.
 */
static void strip(char *s)
{
    char *x;
    if (!s[0]) return;
    for (x = s + strlen(s) - 1; isspace(*x); x--);
    x[1] = '\0';
    for (x = s; isspace(*x); x++);
    memmove(s, x, strlen(x) + 1);
}



/* _unix_load_modules:
 *  Find a modules.lst file and load the modules listed in it.
 */
void _unix_load_modules(int system_driver)
{
   PACKFILE *f;
   char fullpath[1024];
   char *fullpath_slash;
   char buf[1024];
   char buf2[1024];
   char **pathptr;
   char *filename;
   void *handle;
   void (*init)(int);
   MODULE *m;
 
   /* Read the ALLEGRO_MODULES environment variable.
    * But don't do it if we are root (for obvious reasons).
    */
   if (geteuid() != 0) {
      char *env = getenv("ALLEGRO_MODULES");
      if (env) {
	 snprintf(fullpath, sizeof fullpath, "%s/%s", env, "modules.lst");
	 fullpath[(sizeof fullpath) - 1] = 0;
	 f = pack_fopen(uconvert_ascii(fullpath, buf), F_READ);
	 if (f) goto found;
      }
   }

   for (pathptr = module_path; *pathptr; pathptr++) {
      snprintf(fullpath, sizeof fullpath, "%s/%d.%d/modules.lst",
	       *pathptr, ALLEGRO_VERSION, ALLEGRO_SUB_VERSION);
      fullpath[(sizeof fullpath) - 1] = 0;
      f = pack_fopen(uconvert_ascii(fullpath, buf), F_READ);
      if (f) goto found;
   }
   
   return;

   found:

   fullpath_slash = strrchr(fullpath, '/');
   
   while (!pack_feof(f)) {
      if (!pack_fgets(buf, sizeof buf, f))
         break;
      filename = uconvert_toascii(buf, buf2);
      strip(filename);
      if ((filename[0] == '#') || (strlen(filename) == 0))
	 continue;

      if (!fullpath_slash) {
         snprintf(fullpath, sizeof fullpath, filename);
	 fullpath[(sizeof fullpath) - 1] = 0;
      }
      else {
	 snprintf(fullpath_slash+1, (sizeof fullpath) - (fullpath_slash - fullpath) - 1, filename);
	 fullpath[(sizeof fullpath) - 1] = 0;
      }
      
      if (!exists(uconvert_ascii(fullpath, buf)))
	 continue;

      handle = dlopen(fullpath, RTLD_NOW);
      if (!handle) {
	 /* useful during development */
	 /* printf("Error loading module: %s\n", dlerror()); */
	 continue;
      }
      
      init = dlsym(handle, "_module_init");
      if (init)
         init(system_driver);

      m = malloc(sizeof(MODULE));
      if (m) {
	 m->handle = handle;
	 m->next = module_list;
	 module_list = m;
      }
   }

   pack_fclose(f);
}



/* _unix_unload_modules:
 *  Unload loaded modules.
 */             
void _unix_unload_modules(void)
{
   MODULE *m, *next;
   void (*shutdown)(void);
   
   for (m = module_list; m; m = next) {
      next = m->next;
      shutdown = dlsym(m->handle, "_module_shutdown");
      if (shutdown)
         shutdown();
      dlclose(m->handle);
      free(m);
   }
   
   module_list = NULL;
}



#else	/* ifdef ALLEGRO_WITH_MODULES */



void _unix_load_modules(int system_driver)
{
   /*	Fiddle dee dum,
	Fiddle dee dee,
	Eric, the half a bee	*/
}



void _unix_unload_modules(void)
{
   /*	Ho ho ho,
	Tee hee hee,
	Eric, the half a bee	*/
}



#endif
