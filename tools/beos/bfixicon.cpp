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

#define ALLEGRO_USE_CONSOLE

#include <stdio.h>
#include <string.h>
#include <allegro.h>

/* Live happy with the Be general header file */
#define color_map be_color_map
#define drawing_mode be_drawing_mode
#undef MIN
#undef MAX
#undef TRACE
#undef ASSERT

#ifndef SCAN_DEPEND
   #include <Be.h>
#endif


/* convert_to_bbitmap:
 *  Gets a bitmap and converts it into a BBitmap suitable as an icon whose
 *  size depends on the size parameter: 0 for a 16x16 icon, 1 for a 32x32 one.
 */
extern "C" BBitmap *convert_to_bbitmap(BITMAP *bmp, PALETTE pal, PALETTE system_pal, int size)
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



extern "C" void usage(void)
{
   printf("\nBeOS executable icon patcher for Allegro " ALLEGRO_VERSION_STR "\n");
   printf("By Angelo Mottola, " ALLEGRO_DATE_STR "\n\n");
   printf("Usage: bfixicon exename bitmap [bitmap]\n");
   printf(" or\n");
   printf("       bfixicon exename -d datafile object [palette] [object [palette]]\n");
   printf(" where object is either a bitmap or a RLE sprite.\n");
   printf("Options:\n");
   printf("   -d datafile   use datafile as the source for objects and palettes\n");
   exit(EXIT_FAILURE);
}



/* main:
 *  Guess what this function does.
 */
int main(int argc, char *argv[])
{
   char mime_type[MAXPATHLEN], dat_name[128];
   BITMAP *icon_bmp[2], *temp;
   RLE_SPRITE *sprite;
   PALETTE icon_pal[2], system_pal;
   DATAFILE *dat;
   BApplication app("application/x-vnd.Allegro-bfixicon");
   BAppFileInfo app_info;
   BScreen bscreen(B_MAIN_SCREEN_ID);
   rgb_color col;
   BBitmap *icon;
   BFile file;
   int i, j, icon_num = 0, pal_start = 0, arg;

   install_allegro(SYSTEM_NONE, &errno, &atexit);
   
   if (argc < 3)
      usage();

   dat_name[0] = '\0';
   
   for (arg = 2; arg < argc; arg++) {
      
      if (argv[arg][0] == '-') {
         
         switch (argv[arg][1]) {
            
            case 'd':
               if (argc < arg + 2)
                  usage();
                  
               strcpy(dat_name, argv[++arg]);
               pal_start = icon_num;
               break;
               
            default:
               usage();
         }
      }
      else {
         if (dat_name[0]) {
            dat = load_datafile_object(dat_name, argv[arg]);

            if (!dat) {
               printf("Error reading %s from %s.\n", argv[arg], dat_name);
               exit(EXIT_FAILURE);
            }

            switch (dat->type) {
         
               case DAT_BITMAP:
                  temp = (BITMAP *)dat->dat;
                  icon_bmp[icon_num] = create_bitmap_ex(temp->vtable->color_depth, temp->w, temp->h);
                  blit(temp, icon_bmp[icon_num], 0, 0, 0, 0, temp->w, temp->h);
                  icon_num++;
                  break;
               
               case DAT_RLE_SPRITE:
                  sprite = (RLE_SPRITE *)dat->dat;
                  icon_bmp[icon_num] = create_bitmap_ex(sprite->color_depth, sprite->w, sprite->h);
                  clear_to_color(icon_bmp[icon_num], icon_bmp[icon_num]->vtable->mask_color);
                  draw_rle_sprite(icon_bmp[icon_num], sprite, 0, 0);
                  icon_num++;
                  break;
                  
               case DAT_PALETTE:
                  if (pal_start == icon_num)
                     usage();
                  
                  for (j = pal_start; j < icon_num; j++)
                     for (i = 0; i < PAL_SIZE; i++)
                        icon_pal[j][i] = ((RGB *)dat->dat)[i];
                  
                  pal_start = icon_num;
                  break;
               
               default:
                  usage();
            }
            unload_datafile_object(dat);
         }
         else {
            icon_bmp[icon_num] = load_bitmap(argv[arg], icon_pal[icon_num]);
            if (!icon_bmp[icon_num]) {
               printf("Error reading %s.\n", argv[arg]);
               exit(EXIT_FAILURE);
            }
            icon_num++;
         }

         if (icon_num == 2)
            break;
      }
   }

   if (icon_num == 0)
      usage();
   
   if (!exists(argv[1])) {
      fprintf(stderr, "Error: %s not found.\n", argv[1]);
      exit(EXIT_FAILURE);
   }
   file.SetTo(argv[1], B_READ_WRITE);
   if (app_info.SetTo(&file) != B_OK) {
      fprintf(stderr, "Error: %s is not accessible.\n", argv[1]);
      exit(EXIT_FAILURE);
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
   icon = convert_to_bbitmap(icon_bmp[0], icon_pal[0], system_pal, 1);
   app_info.SetIcon(icon, B_LARGE_ICON);
   delete icon;
   
   /* Now set small icon */
   icon = convert_to_bbitmap(icon_bmp[icon_num - 1], icon_pal[icon_num - 1], system_pal, 0);
   app_info.SetIcon(icon, B_MINI_ICON);
   delete icon;

   for (i = 0; i < icon_num; i++)
      destroy_bitmap(icon_bmp[i]);

   exit(EXIT_SUCCESS);
}
END_OF_MAIN();

