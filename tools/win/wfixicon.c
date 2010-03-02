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
 *      Allegro bitmap -> Windows icon converter.
 *
 *      By Elias Pschernig.
 * 
 *      See readme.txt for copyright information.      
 */


#define ALLEGRO_USE_CONSOLE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <allegro5.h>


#define ICON_MAX  16



/* save_ico:
 *  Saves <num> bitmaps as an .ico, using <pal> as palette for 8-bit bitmaps.
 *  Other color depths are saved as 24-bit.
 */
int save_ico(const char *filename, BITMAP *bmp[], int num, PALETTE pal[])
{
   PACKFILE *f;
   int depth, bpp, bw, bitsw;
   int size, offset, n, i;
   int c, x, y, b, m, v;

   errno = 0;

   f = pack_fopen(filename, F_WRITE);
   if (!f)
      return errno;

   offset = 6 + num * 16;  /* ICONDIR + ICONDIRENTRYs */
   
   /* ICONDIR */
   pack_iputw(0, f);    /* reserved            */
   pack_iputw(1, f);    /* resource type: ICON */
   pack_iputw(num, f);  /* number of icons     */

   for(n = 0; n < num; n++) {
      depth = bitmap_color_depth(bmp[n]);
      bpp = (depth == 8) ? 8 : 24;  
      bw = (((bmp[n]->w * bpp / 8) + 3) / 4) * 4;
      bitsw = ((((bmp[n]->w + 7) / 8) + 3) / 4) * 4;
      size = bmp[n]->h * (bw + bitsw) + 40;

      if (bpp == 8)
         size += 256 * 4;

      /* ICONDIRENTRY */
      pack_putc(bmp[n]->w, f);  /* width                       */
      pack_putc(bmp[n]->h, f);  /* height                      */
      pack_putc(0, f);          /* color count                 */
      pack_putc(0, f);          /* reserved                    */
      pack_iputw(1, f);         /* color planes                */
      pack_iputw(bpp, f);       /* bits per pixel              */
      pack_iputl(size, f);      /* size in bytes of image data */
      pack_iputl(offset, f);    /* file offset to image data   */

      offset += size;
   }

   for(n = 0; n < num; n++) {
      depth = bitmap_color_depth(bmp[n]);
      bpp = (depth == 8) ? 8 : 24;
      bw = (((bmp[n]->w * bpp / 8) + 3) / 4) * 4;
      bitsw = ((((bmp[n]->w + 7) / 8) + 3) / 4) * 4;
      size = bmp[n]->h * (bw + bitsw) + 40;

      if (bpp == 8)
         size += 256 * 4;

      /* BITMAPINFOHEADER */
      pack_iputl(40, f);             /* size           */
      pack_iputl(bmp[n]->w, f);      /* width          */
      pack_iputl(bmp[n]->h * 2, f);  /* height x 2     */
      pack_iputw(1, f);              /* planes         */
      pack_iputw(bpp, f);            /* bitcount       */
      pack_iputl(0, f);              /* unused for ico */
      pack_iputl(size, f);           /* size           */
      pack_iputl(0, f);              /* unused for ico */
      pack_iputl(0, f);              /* unused for ico */
      pack_iputl(0, f);              /* unused for ico */
      pack_iputl(0, f);              /* unused for ico */

      /* PALETTE */
      if (bpp == 8) {
         pack_iputl(0, f);  /* color 0 is black, so the XOR mask works */

         for (i = 1; i<256; i++) {
            if (pal[n]) {
               pack_putc(_rgb_scale_6[pal[n][i].b], f);
               pack_putc(_rgb_scale_6[pal[n][i].g], f);
               pack_putc(_rgb_scale_6[pal[n][i].r], f);
               pack_putc(0, f);
            }
            else {
               pack_iputl(0, f);
            }
         }
      }

      /* XOR MASK */
      for (y = bmp[n]->h - 1; y >= 0; y--) {
         for (x = 0; x < bmp[n]->w; x++) {
            if (bpp == 8) {
               pack_putc(getpixel(bmp[n], x, y), f);
            }
            else {
               c = getpixel(bmp[n], x, y);
               pack_putc(getb_depth(depth, c), f);
               pack_putc(getg_depth(depth, c), f);
               pack_putc(getr_depth(depth, c), f);
            }
         }

         /* every scanline must be 32-bit aligned */
         while (x&3) {
            pack_putc(0, f);
            x++;
         } 
      }

      /* AND MASK */
      for (y = bmp[n]->h - 1; y >= 0; y--) {
         for (x = 0; x < (bmp[n]->w + 7)/8; x++) {
            m = 0;
            v = 128;

            for (b = 0; b < 8; b++) {
               c = getpixel(bmp[n], x * 8 + b, y);
               if (c == bitmap_mask_color(bmp[n]))
                  m += v;
               v /= 2;
            }

            pack_putc(m, f);  
         }

         /* every scanline must be 32-bit aligned */
         while (x&3) {
            pack_putc(0, f);
            x++;
         }
      }
   }

   pack_fclose(f);

   return errno;
}



void usage(void)
{
   printf("\nWindows icon converter for Allegro " ALLEGRO_VERSION_STR "\n");
   printf("By Elias Pschernig, " ALLEGRO_DATE_STR "\n\n");
   printf("Usage: wfixicon icon [-r[o]] bitmap [bitmap...]\n");
   printf("        or\n");
   printf("       wfixicon icon [-r[o]] -d datafile object [palette] [object...]\n");
   printf("         where each 'object' is a bitmap or a RLE sprite.\n");
   printf("Options:\n");
   printf("   -r      output .rc file for the icon\n");
   printf("   -ro     call the resource compiler on the .rc file\n");

   exit(1);
}



int main(int argc, char *argv[])
{
   char dat_name[128], rc_name[128], res_name[128], str[256];
   int icon_num = 0, pal_start = 0;
   int create_rc = FALSE, call_windres = FALSE;
   int i, j, arg;
   BITMAP *bmp[ICON_MAX];
   PALETTE pal[ICON_MAX];
   RLE_SPRITE *sprite;
   DATAFILE *dat;
   PACKFILE *f;

   if (install_allegro(SYSTEM_NONE, &errno, atexit) != 0)
       exit(EXIT_FAILURE);
   set_color_conversion(COLORCONV_NONE);

   if (argc < 3)
      usage();

   dat_name[0] = '\0';

   for (arg = 2; arg < argc; arg++) {

      if (argv[arg][0] == '-') {

         switch(argv[arg][1]) {

            case 'd':  /* datafile argument */
               if (argc < arg+2)
                  usage();

               strcpy(dat_name, argv[++arg]);
               pal_start = icon_num;
               break;

            case 'r':
               create_rc = TRUE;

               if (argv[arg][2] == 'o') {
                  call_windres = TRUE;
               }
               break;

            default:
               usage();
         }
      }  /* end of '-' handling */
      else {
         if (dat_name[0]) {
            dat = load_datafile_object(dat_name, argv[arg]);

            if (!dat) {
               printf("Error reading %s from %s.\n", argv[arg], dat_name);
               exit(EXIT_FAILURE);
            }

            switch (dat->type) {

               case DAT_BITMAP:
                  bmp[icon_num] = (BITMAP *)dat->dat;
                  icon_num++;
                  break;

               case DAT_RLE_SPRITE:
                  sprite = (RLE_SPRITE *)dat->dat;
                  bmp[icon_num] = create_bitmap_ex(sprite->color_depth, sprite->w, sprite->h);
                  clear_to_color(bmp[icon_num], bitmap_mask_color(bmp[icon_num]));
                  draw_rle_sprite(bmp[icon_num], sprite, 0, 0);
                  icon_num++;
                  break;

               case DAT_PALETTE:
                  if (pal_start == icon_num)
                     usage();

                  for (j = pal_start; j < icon_num; j++)
                     for (i = 0; i < PAL_SIZE; i++)
                        pal[j][i] = ((RGB *)dat->dat)[i];

                  pal_start = icon_num;
                  break;

               default:
                  usage();
            }
         }
         else {
            bmp[icon_num] = load_bitmap(argv[arg], pal[icon_num]);
            if (!bmp[icon_num]) {
               printf("Error reading %s.\n", argv[arg]);
               exit(EXIT_FAILURE);
            }

            icon_num++;
         }

         if (icon_num == ICON_MAX)
            break;
      } /* end of normal argument handling */
   }

   if (icon_num == 0)
      usage();

   /* do the hard work */
   if (save_ico(argv[1], bmp, icon_num, pal) != 0) {
      printf("Error writing %s.\n", argv[1]);
      exit(EXIT_FAILURE);
   }

   /* output a .rc file along with the ico, to be processed by the resource compiler */
   if (create_rc) {
      replace_extension(rc_name, argv[1], "rc", sizeof(rc_name));

      f = pack_fopen(rc_name, F_WRITE);

      sprintf(str, "allegro_icon ICON %s\n", argv[1]);
      pack_fwrite(str, strlen(str), f);

      pack_fclose(f);

      if (call_windres) {
         replace_extension(res_name, argv[1], "res", sizeof(res_name));

#if defined ALLEGRO_MINGW32
         sprintf(str, "windres -O coff -o %s -i %s", res_name, rc_name);
#elif defined ALLEGRO_MSVC
         sprintf(str, "rc -fo %s %s", res_name, rc_name);
#elif defined ALLEGRO_BCC32
         sprintf(str, "brc32 -r -fo %s %s", res_name, rc_name);
#endif

         system(str);
         delete_file(argv[1]);
         delete_file(rc_name);
      }
   }

   return 0;
}
