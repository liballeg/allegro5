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
   char **path;
   char buf[1024];
   char buf2[1024];
   char buf3[1024];
   char *filename;
   void *handle;
   void (*init)(int);
   MODULE *m;
 
   for (path = module_path; *path; path++) {
      snprintf(buf, sizeof buf, "%s%d.%d/modules.lst", *path,
	       ALLEGRO_VERSION, ALLEGRO_SUB_VERSION);
      f = pack_fopen(uconvert_ascii(buf, buf2), F_READ);
      if (f) goto found;
   }

   return;
   
   found:

   while (!pack_feof(f)) {
      if (!pack_fgets(buf, sizeof buf, f))
         break;
      filename = uconvert_toascii(buf, buf2);
      strip(filename);
      if ((filename[0] == '#') || (strlen(filename) == 0))
	 continue;

      if (filename[0] != '/') {
	 snprintf(buf3, sizeof buf3, "%s%d.%d/%s", *path,
		  ALLEGRO_VERSION, ALLEGRO_SUB_VERSION, filename);
	 filename = buf3;
      }

      if (!exists(uconvert_ascii(filename, buf)))
	 continue;

      handle = dlopen(filename, RTLD_NOW);
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
   void (*shutdown)();
   
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
