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
 *      File compression utility for the Allegro library.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#define ALLEGRO_USE_CONSOLE

#include <stdio.h>

#include "allegro.h"



static void usage(void)
{
   printf("\nFile compression utility for Allegro " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR);
   printf("\nBy Shawn Hargreaves, " ALLEGRO_DATE_STR "\n\n");
   printf("Usage: 'pack <in> <out>' to pack a file\n");
   printf("       'pack u <in> <out>' to unpack a file\n");
}



static void err(char *s1, char *s2)
{
   printf("\nError %d", errno);

   if (s1)
      printf(": %s", s1);

   if (s2)
      printf("%s", s2);

   printf("\n");

   if (errno==EDOM)
      printf("Not a packed file\n");
}



int main(int argc, char *argv[])
{
   char *f1, *f2;
   char *m1, *m2;
   long s1, s2;
   char *t;
   PACKFILE *in, *out;
   int c;

   if (install_allegro(SYSTEM_NONE, &errno, atexit) != 0)
      return 1;

   if (argc == 3) {
      f1 = argv[1];
      f2 = argv[2];
      m1 = F_READ;
      m2 = F_WRITE_PACKED;
      t = "Pack";
   }
   else if ((argc == 4) && (argv[1][1] == 0) && ((argv[1][0] == 'u') || (argv[1][0] == 'U'))) {
      f1 = argv[2];
      f2 = argv[3];
      m1 = F_READ_PACKED;
      m2 = F_WRITE;
      t = "Unpack";
   }
   else {
      usage();
      return 1;
   }

   s1 = file_size_ex(f1);

   in = pack_fopen(f1, m1);
   if (!in) {
      err("can't open ", f1);
      return 1;
   }

   out = pack_fopen(f2, m2);
   if (!out) {
      delete_file(f2);
      pack_fclose(in);
      err("can't create ", f2);
      return 1;
   }

   printf("%sing %s into %s...\n", t, f1, f2);

   while ((c = pack_getc(in)) != EOF) {
      if (pack_putc(c, out) != c)
	 break;
   }

   pack_fclose(in);
   pack_fclose(out);

   if (errno) {
      delete_file(f2);
      err(NULL, NULL);
      return 1;
   }

   s2 = file_size_ex(f2);
   if (s1 == 0)
      printf("\nInput size: %ld\nOutput size: %ld\n", s1, s2);
   else
      printf("\nInput size: %ld\nOutput size: %ld\n%ld%%\n", s1, s2, (s2*100+(s1>>1))/s1);

   return 0;
}

END_OF_MAIN()
