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
 *      Color (translucency and lighting) mapping table construction utility.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#define ALLEGRO_USE_CONSOLE

#include <stdio.h>
#include <string.h>

#include "allegro.h"



void usage(void)
{
   printf("\nColor map construction utility for Allegro " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR);
   printf("\nBy Shawn Hargreaves, " ALLEGRO_DATE_STR "\n\n");
   printf("Usage: colormap <mode> palfile.[pcx|bmp] outputfile [r g b] [modifiers]\n\n");
   printf("   Reads a palette from the input file, and writes out a 64k mapping\n");
   printf("   table for implementing lighting and translucency effects (in a\n");
   printf("   suitable format for use with the global color_map pointer).\n");
   printf("\n");
   printf("   The mode can be either 'light', to build a lighting table, or 'trans',\n");
   printf("   to build a translucency table.\n");
   printf("\n");
   printf("   The r, g, and b values control the color blending formula. For lighting\n");
   printf("   tables they specify the color to use at light level zero, in VGA 0-63\n");
   printf("   format (default black). For translucency tables they specify the degree\n");
   printf("   of translucency for each color component, ranging 0-255 (default 128,\n");
   printf("   0=invisible, 255=solid).\n");
   printf("\n");
   printf("Modifiers:\n");
   printf("   'ss=x' - makes source color #x completely solid\n");
   printf("   'st=x' - makes source color #x completely transparent\n");
   printf("   'ds=x' - makes destination color #x completely solid\n");
   printf("   'dt=x' - makes destination color #x completely transparent\n");
}



int check_color_value(char *s, int def, int min, int max, char *t)
{
   int val;

   if (!s)
      return def;

   val = strtol(s, NULL, 0);

   if ((val < min) || (val > max)) {
      printf("Error: %s value %d is invalid (range is %d to %d)\n", t, val, min, max);
      exit(1);
   }

   return val;
}



PALETTE the_pal;
COLOR_MAP the_map;



void modifier(char *s)
{
   int col, i;

   if (((utolower(s[0]) != 's') && (utolower(s[0]) != 'd')) ||
       ((utolower(s[1]) != 's') && (utolower(s[1]) != 't')) ||
       (s[2] != '=')) {
      printf("Error: unknown modifier '%s'\n", s);
      exit(1);
   } 

   col = strtol(s+3, NULL, 0);
   if ((col < 0) || (col > 255)) {
      printf("Error: modifier color %d is invalid (range is 0 to 255)\n", col);
      exit(1);
   }

   if (utolower(s[0] == 's')) {
      if (utolower(s[1] == 's')) {
	 printf("Making source color #%d solid\n", col);
	 for (i=0; i<PAL_SIZE; i++)
	    the_map.data[col][i] = col;
      }
      else {
	 printf("Making source color #%d transparent\n", col);
	 for (i=0; i<PAL_SIZE; i++)
	    the_map.data[col][i] = i;
      }
   }
   else {
      if (utolower(s[1] == 's')) {
	 printf("Making destination color #%d solid\n", col);
	 for (i=0; i<PAL_SIZE; i++)
	    the_map.data[i][col] = col;
      }
      else {
	 printf("Making destination color #%d transparent\n", col);
	 for (i=0; i<PAL_SIZE; i++)
	    the_map.data[i][col] = i;
      }
   }
}



int main(int argc, char *argv[])
{
   BITMAP *bmp;
   PACKFILE *f;
   int trans = TRUE;
   char *infile = NULL;
   char *outfile = NULL;
   char *r = NULL;
   char *g = NULL;
   char *b = NULL;
   int rval, gval, bval;
   int i = 1;

   if (argc > i) {
      if (stricmp(argv[i], "light") == 0) {
	 trans = FALSE;
	 i++;
      }
      else if (stricmp(argv[i], "trans") == 0) {
	 trans = TRUE;
	 i++;
      }
   }

   if (argc > i)
      infile = argv[i++];

   if (argc > i)
      outfile = argv[i++];

   if ((argc > i) && (!strchr(argv[i], '=')))
      r = argv[i++];

   if ((argc > i) && (!strchr(argv[i], '=')))
      g = argv[i++];

   if ((argc > i) && (!strchr(argv[i], '=')))
      b = argv[i++];

   if ((!infile) || (!outfile)) {
      usage();
      return 1;
   }

   if (install_allegro(SYSTEM_NONE, &errno, atexit) != 0)
      return 1;
   set_color_conversion(COLORCONV_NONE);

   bmp = load_bitmap(infile, the_pal);
   if (!bmp) {
      printf("Error reading palette from '%s'\n", infile);
      return 1;
   }

   destroy_bitmap(bmp);

   printf("Palette read from '%s'\n", infile);

   if (trans) {
      rval = check_color_value(r, 128, 0, 255, "red");
      gval = check_color_value(g, 128, 0, 255, "green");
      bval = check_color_value(b, 128, 0, 255, "blue");

      printf("Solidity: red=%d%%, green=%d%%, blue=%d%%\n", rval*100/255, gval*100/255, bval*100/255);
      printf("Creating translucency color map\n");

      create_trans_table(&the_map, the_pal, rval, gval, bval, NULL);
   }
   else {
      rval = check_color_value(r, 0, 0, 63, "red");
      gval = check_color_value(g, 0, 0, 63, "green");
      bval = check_color_value(b, 0, 0, 63, "blue");

      printf("Light color: red=%d, green=%d, blue=%d\n", rval, gval, bval);
      printf("Creating lighting color map\n");

      create_light_table(&the_map, the_pal, rval, gval, bval, NULL);
   }

   while (i < argc)
      modifier(argv[i++]);

   f = pack_fopen(outfile, F_WRITE);
   if (!f) {
      printf("Error writing '%s'\n", outfile);
      return 1;
   }

   pack_fwrite(&the_map, sizeof(the_map), f);
   pack_fclose(f);

   printf("Color mapping table written to '%s'\n", outfile);
   return 0;
}

END_OF_MAIN()
