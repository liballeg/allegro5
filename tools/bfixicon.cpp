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
 *      BeOS executable icon resource fixer.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */

#define USE_CONSOLE

#include <stdio.h>
#include <string.h>
#include <allegro.h>

#ifdef ALLEGRO_BEOS

/* Live happy with the Be general header file */
#define color_map be_color_map
#define drawing_mode be_drawing_mode
#undef MIN
#undef MAX
#undef TRACE
#undef ASSERT

#include <Be.h>


PALETTE system_pal;



/* convert_to_bbitmap:
 *  Gets a bitmap and converts it into a BBitmap suitable as an icon whose
 *  size depends on the size parameter: 0 for a 16x16 icon, 1 for a 32x32 one.
 */
BBitmap *convert_to_bbitmap(BITMAP *bmp, PALETTE pal, int size)
{
   BBitmap *icon;
   BITMAP *truecolor_bmp, *stretched_bmp, *ready_bmp;
   PALETTE opt_pal;
   signed char rsvd[256];
   unsigned long addr;
   int i, x, y;
   
   size = (size ? 32 : 16);
   
   truecolor_bmp = create_bitmap_ex(32, bmp->w, bmp->h);
   stretched_bmp = create_bitmap_ex(32, size, size);
   ready_bmp = create_bitmap_ex(8, size, size);
   
   if ((!truecolor_bmp) || (!stretched_bmp) || (!ready_bmp))
      return NULL;
   
   select_palette(pal);
   blit(bmp, truecolor_bmp, 0, 0, 0, 0, bmp->w, bmp->h);
   stretch_blit(truecolor_bmp, stretched_bmp, 0, 0, bmp->w, bmp->h, 0, 0, size, size);
   for (i=0; i<255; i++) rsvd[i] = 1;
   rsvd[255] = -1;
   generate_optimized_palette(stretched_bmp, system_pal, rsvd);
   select_palette(system_pal);
   blit(stretched_bmp, ready_bmp, 0, 0, 0, 0, size, size);
   for (y=0; y<size; y++) for (x=0; x<size; x++) {
      if (_getpixel32(stretched_bmp, x, y) == stretched_bmp->vtable->mask_color)
         _putpixel(ready_bmp, x, y, 255);
   }

   icon = new BBitmap(BRect(0, 0, size - 1, size - 1), B_CMAP8, false, true);
   if (icon) {
      addr = (unsigned long)icon->Bits();
      for (i=0; i<size; i++) {
         memcpy((void *)addr, ready_bmp->line[i], size);
         addr += icon->BytesPerRow();
      }
   }
   
   destroy_bitmap(truecolor_bmp);
   destroy_bitmap(stretched_bmp);
   destroy_bitmap(ready_bmp);
   
   return icon;
}



/* main:
 *  Guess what this function does.
 */
int main(int argc, char *argv[])
{
   char exe[MAXPATHLEN], mime_type[MAXPATHLEN];
   BITMAP *icon_bmp;
   PALETTE pal;
   BApplication app("application/x-vnd.Allegro-bfixicon");
   BAppFileInfo app_info;
   BScreen bscreen(B_MAIN_SCREEN_ID);
   rgb_color col;
   BBitmap *icon;
   BFile file;
   int i;

   install_allegro(SYSTEM_NONE, &errno, &atexit);
   
   if ((argc != 3) && (argc != 4)) {
      printf("\nBeOS executable icon patcher for Allegro " ALLEGRO_VERSION_STR "\n");
      printf("By Shawn Hargreaves, " ALLEGRO_DATE_STR "\n\n");
      printf("Usage: bfixicon exename largeicon [smallicon]\n\n");
      return 1;
   }
   
   strcpy(exe, argv[1]);
   if (!exists(exe)) {
      fprintf(stderr, "Error: %s not found\n", exe);
      return 1;
   }
   file.SetTo(exe, B_READ_WRITE);
   if (app_info.SetTo(&file) != B_OK) {
      fprintf(stderr, "Error: %s is not accessible\n", exe);
      return 1;
   };
   app_info.GetType(mime_type);
   if (strncmp(mime_type, "application/", 12)) {
      fprintf(stderr, "Error: %s does not appear to be a valid executable\n", exe);
   }
   app_info.SetInfoLocation(B_USE_BOTH_LOCATIONS);

   /* Prepare for color conversion and read in system wide palette */
   set_color_conversion(COLORCONV_TOTAL | COLORCONV_KEEP_TRANS);
   for (i=0; i<255; i++) {
      col = bscreen.ColorForIndex(i);
      system_pal[i].r = col.red >> 2;
      system_pal[i].g = col.green >> 2;
      system_pal[i].b = col.blue >> 2;
   }

   /* Set large icon first */
   if ((!(icon_bmp = load_bitmap(argv[2], pal))) ||
       (!(icon = convert_to_bbitmap(icon_bmp, pal, 1)))) {
      fprintf(stderr, "Error loading %s: %s\n", argv[2], allegro_error);
      if (icon_bmp)
         destroy_bitmap(icon_bmp);
      return 1;
   }
   app_info.SetIcon(icon, B_LARGE_ICON);
   delete icon;
   
   if (argc == 3) {
      /* Generate small icon from the large one */
      icon = convert_to_bbitmap(icon_bmp, pal, 0);
   }
   else {
      /* Set small icon */
      destroy_bitmap(icon_bmp);
      if ((!(icon_bmp = load_bitmap(argv[3], pal))) ||
          (!(icon = convert_to_bbitmap(icon_bmp, pal, 0)))) {
         fprintf(stderr, "Error loading %s: %s\n", argv[2], allegro_error);
         if (icon_bmp)
            destroy_bitmap(icon_bmp);
         return 1;
      }
   }
   app_info.SetIcon(icon, B_MINI_ICON);
   delete icon;
   destroy_bitmap(icon_bmp);

   return 0;
}
END_OF_MAIN();



#else



int main(int argc, char *argv[])
{
   printf("\nSorry, the bfixicon program only works on BeOS\n\n");
   return -1;
}
END_OF_MAIN();



#endif
