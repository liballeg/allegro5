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
 *      List which resolutions and color depths are available.
 *
 *      By Lorenzo Petrone.
 *
 *      See readme.txt for copyright information.
 */


#define ALLEGRO_USE_CONSOLE
#include <string.h>
#include <stdio.h>
#include "allegro.h"
#include "allegro/internal/aintern.h"



char *al_un_id(char *out, int driver_id)
{
   out[0] = (driver_id >> 24) & 0xff;
   out[1] = (driver_id >> 16) & 0xff;
   out[2] = (driver_id >> 8) & 0xff;
   out[3] = (driver_id) & 0xff;
   out[4] = '\0';
   return out;
}



int al_id_safe(char *s)
{
   int l = strlen(s);
   return AL_ID((l > 0) ? s[0] : ' ',
                (l > 1) ? s[1] : ' ',
                (l > 2) ? s[2] : ' ',
                (l > 3) ? s[3] : ' ');
}



int main(int argc, char *argv[])
{
   _DRIVER_INFO *driver_info;
   GFX_DRIVER **gfx_driver;
   GFX_MODE_LIST *gfx_mode_list;
   int driver_count, i, id;
   char buf[5];

   if (allegro_init() != 0)
      return 1;

   printf("Allegro graphics info utility " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR "\n");
   printf("By Lorenzo Petrone, " ALLEGRO_DATE_STR "\n\n");

   if (system_driver->gfx_drivers)
      driver_info = system_driver->gfx_drivers();
   else
      driver_info = _gfx_driver_list;

   driver_count = 0;
   while (driver_info[driver_count].driver)
      driver_count++;

   gfx_driver = malloc(sizeof(GFX_DRIVER *) * driver_count);
   if (!gfx_driver)
      return 1;
   for (i = 0; i < driver_count; i++)
      gfx_driver[i] = driver_info[i].driver;

   if (argc == 1) {
      printf("No graphics driver specified.  These are available:\n\n");
      for (i = 0; i < driver_count; i++)
         printf("%s : %s\n", al_un_id(buf, gfx_driver[i]->id), gfx_driver[i]->ascii_name);
      printf("\n");
      goto end;
   }

   id = al_id_safe(argv[1]);
   for (i = 0; i < driver_count; i++) {
      if ((strcmp(gfx_driver[i]->ascii_name, argv[1]) == 0) || (gfx_driver[i]->id == id)) {
         printf("Name: %s; driver ID: %s\n", gfx_driver[i]->ascii_name, al_un_id(buf, gfx_driver[i]->id));
         break;
      }
   }

   if (i == driver_count) {
      printf("Unknown graphics driver %s.\n"
             "Run gfxinfo without parameters to see a list.\n\n", argv[1]);
      goto end;
   }

   if (gfx_driver[i]->windowed) {
      printf("This is a windowed mode driver. You can use any resolution you want.\n\n");
      goto end;
   }

   gfx_mode_list = get_gfx_mode_list(gfx_driver[i]->id);
   if (!gfx_mode_list) {
      printf("Failed to retrieve mode list. This might be because the driver\n"
             "doesn't support mode fetching.\n\n");
      goto end;
   }

   for (i = 0; i < gfx_mode_list->num_modes; i++) {
      printf("%4dx%-4d %2d bpp\n",
             gfx_mode_list->mode[i].width,
             gfx_mode_list->mode[i].height,
             gfx_mode_list->mode[i].bpp);
   }
   printf("\n");

   destroy_gfx_mode_list(gfx_mode_list);

   end:
   
   free(gfx_driver);
   
   return 0;
}

END_OF_MAIN()
