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
 *      It tells you what resolutions/color depths are available.
 *
 *      By Lorenzo Petrone.
 *
 *      See readme.txt for copyright information.
 */

#define USE_CONSOLE
#include <string.h>
#include <stdio.h>
#include "allegro.h"
#include "allegro/internal/aintern.h"



/* al_un_id:
 *  transforms an AL_ID into a string
 *  will probably segfault if the array passed is shorter that 5 chars
 *  always return 0 (if you have better ideas...)
 *
 *  is there already another way to do that?
 *  if not, should this func be included somewhere?
 *  no i guess if it was needed it would already been done
 */
int al_un_id (char *out, int id)
{
   out[0]=(id>>24)&0xff;
   out[1]=(id>>16)&0xff;
   out[2]=(id>>8)&0xff;
   out[3]=(id)&0xff;
   out[4]='\0';
   return 0;
}



int main (int argc, char *argv[])
{
   int k,id_in,j,h;
   int match=0,driver_count=0;
   int verbose=1;
   char cid[5];
   _DRIVER_INFO *driver_info;
   GFX_DRIVER **gfx_driver;
   GFX_MODE_LIST *gfx_mode_list;

   allegro_init();

   printf("\nAllegro GFX info utility " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR);
   printf("\nBy Lorenzo Petrone, " ALLEGRO_DATE_STR "\n\n");

   if (system_driver->gfx_drivers)
      driver_info = system_driver->gfx_drivers();
   else
      driver_info = _gfx_driver_list;

   for (k=0; driver_info[k].driver!=NULL; k++) {
      driver_count++;
   }

   gfx_driver=(GFX_DRIVER**)malloc(sizeof(GFX_DRIVER*)*driver_count);

   for (k=0; k<driver_count; k++) {
      gfx_driver[k]=driver_info[k].driver;
   }

   if (argc==1) {
      printf("no gfx mode specified, these are available:\n\n");
      for (k=0; k<driver_count; k++) {
         al_un_id(cid,gfx_driver[k]->id);
         printf("%s : %s\n",cid,gfx_driver[k]->ascii_name);
      }
      printf("\n");
      return 0;
      /* return driver_count; */
   }
   else {
      h=strlen(argv[1]);
      id_in=AL_ID(argv[1][0], h>1 ? argv[1][1] : ' ', h>2 ? argv[1][2] : ' ', h>3 ? argv[1][3] : ' ');
      k=0;
      while (1) {
         if (k>=driver_count) break;
         if ( strncmp(gfx_driver[k]->ascii_name,argv[1],strlen(gfx_driver[k]->ascii_name))==0 ) {
            match=2;
            if (verbose) {
               printf("name is matching, id: ");
               al_un_id(cid,gfx_driver[k]->id);
               printf("%s\n",cid);
            }
            break;
         }
         if ( gfx_driver[k]->id==id_in ) {
            match=3;
            if (verbose) {
               printf("id is matching, name: ");
               printf("%s\n",gfx_driver[k]->ascii_name);
            }
            break;
         }
         else {
            k++;
         }
      }
   }

   if (match==0) {
      printf("Invalid gfx mode %s\nrun gfxinfo without parameters to have a list\n\n",argv[1]);
      return -1;
   }

   if (gfx_driver[k]->windowed==TRUE) {
      printf("In windowed modes you can use any resolution you want\n\n");
      return -1;
      /* return gfx_driver[k]->id; */
   }

   gfx_mode_list=get_gfx_mode_list(gfx_driver[k]->id);
   if (gfx_mode_list==NULL) {
      printf("sorry, failed to retrieve mode list\n\n");
      return -1;
      /* return gfx_driver[k]->id; */
   }

   j=0;
   while (1) {
      if ( j>=gfx_mode_list->num_modes )
         break;
      printf("%4dx%-4d %2d bpp\n",gfx_mode_list->mode[j].width,gfx_mode_list->mode[j].height,gfx_mode_list->mode[j].bpp);
      j++;
   }

   putchar('\n');

   destroy_gfx_mode_list(gfx_mode_list);
   return 0;
   /* return gfx_driver[k]->id; */
}
END_OF_MAIN()
