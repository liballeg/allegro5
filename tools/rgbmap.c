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
 *      RGB -> palette mapping table construction utility.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#define ALLEGRO_USE_CONSOLE

#include <stdio.h>

#include "allegro.h"



void usage(void)
{
   printf("\nRGB map construction utility for Allegro " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR);
   printf("\nBy Shawn Hargreaves, " ALLEGRO_DATE_STR "\n\n");
   printf("Usage: rgbmap palfile.[pcx|bmp] outputfile\n\n");
   printf("   Reads a palette from the input file, and writes out a 32k mapping\n");
   printf("   table for converting RGB values to this palette (in a suitable\n");
   printf("   format for use with the global rgb_map pointer).\n");
}



PALETTE the_pal;
RGB_MAP the_map;



int main(int argc, char *argv[])
{
   BITMAP *bmp;
   PACKFILE *f;

   if (argc != 3) {
      usage();
      return 1;
   }

   if (install_allegro(SYSTEM_NONE, &errno, atexit) != 0)
      return 1;
   set_color_conversion(COLORCONV_NONE);

   bmp = load_bitmap(argv[1], the_pal);
   if (!bmp) {
      printf("Error reading palette from '%s'\n", argv[1]);
      return 1;
   }

   destroy_bitmap(bmp);

   printf("Palette read from '%s'\n", argv[1]);
   printf("Creating RGB map\n");

   create_rgb_table(&the_map, the_pal, NULL);

   f = pack_fopen(argv[2], F_WRITE);
   if (!f) {
      printf("Error writing '%s'\n", argv[2]);
      return 1;
   }

   pack_fwrite(&the_map, sizeof(the_map), f);
   pack_fclose(f);

   printf("RGB mapping table written to '%s'\n", argv[2]);
   return 0;
}

END_OF_MAIN()
