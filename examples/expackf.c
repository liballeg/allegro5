/*    Modified by -------------->Allegro<--------------
 *    (the first line is for the seek test), the "Modified by" is to
 *    silence genexample.py.
 *
 *    Example program for the Allegro library, by Peter Wang and
 *    Elias Pschernig.
 *
 *    This program demonstrates the use of the packfile functions, with some
 *    simple tests.
 *
 *    The first test uses the standard packfile functions to transfer a
 *    bitmap file into a block of memory, then reads the bitmap out of the
 *    block of memory, using a custom packfile vtable.
 *
 *    The second test reads in a bitmap with another custom packfile
 *    vtable, which uses libc's file stream functions.
 *
 *    The third test demonstrates seeking with a custom vtable.
 *
 *    The fourth test reads two bitmaps, and dumps them back into a
 *    single file, using a custom vtable again.
 */

#include <stdio.h>
#include <string.h>
#include "allegro.h"

/*----------------------------------------------------------------------*/
/*                memory vtable                                         */
/*----------------------------------------------------------------------*/

/* The packfile data for our memory reader. */
typedef struct MEMREAD_INFO
{
   AL_CONST unsigned char *block;
   long length;
   long offset;
} MEMREAD_INFO;

static int memread_getc(void *userdata)
{
   MEMREAD_INFO *info = userdata;
   ASSERT(info);
   ASSERT(info->offset <= info->length);

   if (info->offset == info->length)
      return EOF;
   else
      return info->block[info->offset++];
}

static int memread_ungetc(int c, void *userdata)
{
   MEMREAD_INFO *info = userdata;
   unsigned char ch = c;

   if ((info->offset > 0) && (info->block[info->offset-1] == ch))
      return ch;
   else
      return EOF;
}

static int memread_putc(int c, void *userdata)
{
   return EOF;
}

static long memread_fread(void *p, long n, void *userdata)
{
   MEMREAD_INFO *info = userdata;
   size_t actual;
   ASSERT(info);
   ASSERT(info->offset <= info->length);

   actual = MIN(n, info->length - info->offset);

   memcpy(p, info->block + info->offset, actual);
   info->offset += actual;

   ASSERT(info->offset <= info->length);

   return actual;
}

static long memread_fwrite(AL_CONST void *p, long n, void *userdata)
{
   return 0;
}

static int memread_seek(void *userdata, int offset)
{
   MEMREAD_INFO *info = userdata;
   long actual;
   ASSERT(info);
   ASSERT(info->offset <= info->length);

   actual = MIN(offset, info->length - info->offset);

   info->offset += actual;

   ASSERT(info->offset <= info->length);

   if (offset == actual)
      return 0;
   else
      return -1;
}

static int memread_fclose(void *userdata)
{
   return 0;
}

static int memread_feof(void *userdata)
{
   MEMREAD_INFO *info = userdata;

   return info->offset >= info->length;
}

static int memread_ferror(void *userdata)
{
   (void)userdata;

   return FALSE;
}

/* The actual vtable. Note that writing is not supported, the functions for
 * writing above are only placeholders.
 */
static PACKFILE_VTABLE memread_vtable =
{
   memread_fclose,
   memread_getc,
   memread_ungetc,
   memread_fread,
   memread_putc,
   memread_fwrite,
   memread_seek,
   memread_feof,
   memread_ferror
};

/*----------------------------------------------------------------------*/
/*                stdio vtable                                          */
/*----------------------------------------------------------------------*/

static int stdio_fclose(void *userdata)
{
   FILE *fp = userdata;
   return fclose(fp);
}

static int stdio_getc(void *userdata)
{
   FILE *fp = userdata;
   return fgetc(fp);
}

static int stdio_ungetc(int c, void *userdata)
{
   FILE *fp = userdata;
   return ungetc(c, fp);
}

static long stdio_fread(void *p, long n, void *userdata)
{
   FILE *fp = userdata;
   return fread(p, 1, n, fp);
}

static int stdio_putc(int c, void *userdata)
{
   FILE *fp = userdata;
   return fputc(c, fp);
}

static long stdio_fwrite(AL_CONST void *p, long n, void *userdata)
{
   FILE *fp = userdata;
   return fwrite(p, 1, n, fp);
}

static int stdio_seek(void *userdata, int n)
{
   FILE *fp = userdata;
   return fseek(fp, n, SEEK_CUR);
}

static int stdio_feof(void *userdata)
{
   FILE *fp = userdata;
   return feof(fp);
}

static int stdio_ferror(void *userdata)
{
   FILE *fp = userdata;
   return ferror(fp);
}

/* The actual vtable. */
static PACKFILE_VTABLE stdio_vtable =
{
   stdio_fclose,
   stdio_getc,
   stdio_ungetc,
   stdio_fread,
   stdio_putc,
   stdio_fwrite,
   stdio_seek,
   stdio_feof,
   stdio_ferror
};

/*----------------------------------------------------------------------*/
/*                tests                                                 */
/*----------------------------------------------------------------------*/

static void next(void)
{
   textprintf_centre_ex(screen, font, SCREEN_W / 2,
      SCREEN_H - text_height(font), -1, -1, "Press a key to continue");
   readkey();
   clear_bitmap(screen);
}

#define CHECK(x, err)                                   \
   do {                                                 \
      if (!x) {                                         \
         alert("Error", err, NULL, "Ok", NULL, 0, 0);   \
         return;                                        \
      }                                                 \
   } while (0)

/* This reads the files mysha.pcx and allegro.pcx into a memory block as
 * binary data, and then uses the memory vtable to read the bitmaps out of
 * the memory block.
 */
static void memread_test(void)
{
   PACKFILE *f;
   MEMREAD_INFO memread_info;
   BITMAP *bmp, *bmp2;
   unsigned char *block;
   int64_t l1, l2;
   PACKFILE *f1, *f2;

   l1 = file_size_ex("allegro.pcx");
   l2 = file_size_ex("mysha.pcx");

   block = malloc(l1 + l2);

   /* Read mysha.pcx into the memory block. */
   f1 = pack_fopen("allegro.pcx", "rb");
   CHECK(f1, "opening allegro.pcx");
   pack_fread(block, l1, f1);
   pack_fclose(f1);

   /* Read allegro.pcx into the memory block. */
   f2 = pack_fopen("mysha.pcx", "rb");
   CHECK(f2, "opening mysha.pcx");
   pack_fread(block + l1, l2, f2);
   pack_fclose(f2);

   /* Open the memory block as PACKFILE, using our memory vtable. */
   memread_info.block = block;
   memread_info.length = l1 + l2;
   memread_info.offset = 0;
   f = pack_fopen_vtable(&memread_vtable, &memread_info);
   CHECK(f, "reading from memory block");

   /* Read the bitmaps out of the memory block. */
   bmp = load_pcx_pf(f, NULL);
   CHECK(bmp, "load_pcx_pf");
   bmp2 = load_pcx_pf(f, NULL);
   CHECK(bmp2, "load_pcx_pf");

   blit(bmp, screen, 0, 0, 0, 0, bmp->w, bmp->h);
   textprintf_ex(screen, font, bmp->w + 8, 8, -1, -1,
      "\"allegro.pcx\"");
   textprintf_ex(screen, font, bmp->w + 8, 8 + 20, -1, -1,
      "read out of a memory file");

   blit(bmp2, screen, 0, 0, 0, bmp->h + 8, bmp2->w, bmp2->h);
   textprintf_ex(screen, font, bmp2->w + 8, bmp->h + 8, -1, -1,
      "\"mysha.pcx\"");
   textprintf_ex(screen, font, bmp2->w + 8, bmp->h + 8 + 20, -1, -1,
      "read out of a memory file");

   destroy_bitmap(bmp);
   destroy_bitmap(bmp2);
   pack_fclose(f);

   next();
}

/* This reads in allegro.pcx, but it does so by using the stdio vtable. */
static void stdio_read_test(void)
{
   FILE *fp, *fp2;
   PACKFILE *f;
   BITMAP *bmp,*bmp2;

   /* Simply open the file with the libc fopen. */
   fp = fopen("allegro.pcx", "rb");
   CHECK(fp, "opening allegro.pcx");

   /* Create a PACKFILE, with our custom stdio vtable. */
   f = pack_fopen_vtable(&stdio_vtable, fp);
   CHECK(f, "reading with stdio");

   /* Now read in the bitmap. */
   bmp = load_pcx_pf(f, NULL);
   CHECK(bmp, "load_pcx_pf");

   /* A little bit hackish, we re-assign the file pointer in our PACKFILE
    * to another file.
    */
   fp2 = freopen("mysha.pcx", "rb", fp);
   CHECK(fp2, "opening mysha.pcx");

   /* Read in the other bitmap. */
   bmp2 = load_pcx_pf(f, NULL);
   CHECK(bmp, "load_pcx_pf");

   blit(bmp, screen, 0, 0, 0, 0, bmp->w, bmp->h);
   textprintf_ex(screen, font, bmp2->w + 8, 8, -1, -1,
      "\"allegro.pcx\"");
   textprintf_ex(screen, font, bmp2->w + 8, 8 + 20, -1, -1,
      "read with stdio functions");

   blit(bmp2, screen, 0, 0, 0, bmp->h + 8, bmp2->w, bmp2->h);
   textprintf_ex(screen, font, bmp2->w + 8, bmp->h + 8, -1, -1,
      "\"mysha.pcx\"");
   textprintf_ex(screen, font, bmp2->w + 8, bmp->h + 8 + 20, -1, -1,
      "read with stdio functions");

   destroy_bitmap(bmp);
   destroy_bitmap(bmp2);
   pack_fclose(f);

   next();
}

/* This demonstrates seeking. It opens expackf.c, and reads some characters
 * from it.
 */
static void stdio_seek_test(void)
{
   FILE *fp;
   PACKFILE *f;
   char str[8];

   fp = fopen("expackf.c", "rb");
   if (!fp) {
      /* Handle the case where the user is running from a build directory
       * directly under the Allegro root directory.
       */
      fp = fopen("../../examples/expackf.c", "rb");
   }
   CHECK(fp, "opening expackf.c");
   f = pack_fopen_vtable(&stdio_vtable, fp);
   CHECK(f, "reading with stdio");

   pack_fseek(f, 33);
   pack_fread(str, 7, f);
   str[7] = '\0';

   textprintf_ex(screen, font, 0, 0, -1, -1, "Reading from \"expackf.c\" with stdio.");
   textprintf_ex(screen, font, 0, 20, -1, -1, "Seeking to byte 33, reading 7 bytes:");
   textprintf_ex(screen, font, 0, 40, -1, -1, "\"%s\"", str);
   textprintf_ex(screen, font, 0, 60, -1, -1, "(Should be \"Allegro\")");

   pack_fclose(f);

   next();
}

/* This demonstrates writing. It simply saves the two bitmaps into a binary
 * file.
 */
static void stdio_write_test(void)
{
   FILE *fp;
   PACKFILE *f;
   BITMAP *bmp, *bmp2;

   /* Read the bitmaps. */
   bmp = load_pcx("allegro.pcx", NULL);
   CHECK(bmp, "load_pcx");
   bmp2 = load_pcx("mysha.pcx", NULL);
   CHECK(bmp2, "load_pcx");

   /* Write them with out custom vtable. */
   fp = fopen("expackf.out", "wb");
   CHECK(fp, "writing expackf.out");
   f = pack_fopen_vtable(&stdio_vtable, fp);
   CHECK(f, "writing with stdio");

   save_tga_pf(f, bmp, NULL);
   save_bmp_pf(f, bmp2, NULL);

   destroy_bitmap(bmp);
   destroy_bitmap(bmp2);
   pack_fclose(f);

   /* Now read them in again with our custom vtable. */
   fp = fopen("expackf.out", "rb");
   CHECK(fp, "fopen");
   f = pack_fopen_vtable(&stdio_vtable, fp);
   CHECK(f, "reading from stdio");

   /* Note: in general you would need to implement a "chunking" system
    * that knows where the boundary of each file is. Many file format
    * loaders will happily read everything to the end of the file,
    * whereas others stop reading as soon as they have all the essential
    * data (e.g. there may be some metadata at the end of the file).
    * Concatenating bare files together only works in examples programs.
    */
   bmp = load_tga_pf(f, NULL);
   CHECK(bmp, "load_tga_pf");
   bmp2 = load_bmp_pf(f, NULL);
   CHECK(bmp2, "load_bmp_pf");

   blit(bmp, screen, 0, 0, 0, 0, bmp->w, bmp->h);
   textprintf_ex(screen, font, bmp2->w + 8, 8, -1, -1,
      "\"allegro.pcx\" (as tga)");
   textprintf_ex(screen, font, bmp2->w + 8, 8 + 20, -1, -1,
      "wrote with stdio functions");

   blit(bmp2, screen, 0, 0, 0, bmp->h + 8, bmp2->w, bmp2->h);
   textprintf_ex(screen, font, bmp2->w + 8, bmp->h + 8, -1, -1,
      "\"mysha.pcx\" (as bmp)");
   textprintf_ex(screen, font, bmp2->w + 8, bmp->h + 8 + 20, -1, -1,
      "wrote with stdio functions");

   destroy_bitmap(bmp);
   destroy_bitmap(bmp2);
   pack_fclose(f);

   next();
}

/*----------------------------------------------------------------------*/

int main(void)
{
   if (allegro_init() != 0)
      return 1;
   install_keyboard();
   set_color_depth(32);
   if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Unable to set a 640x480x32 windowed mode\n%s\n", allegro_error);
      return 1;
   }

   memread_test();

   stdio_read_test();

   stdio_seek_test();

   stdio_write_test();

   return 0;
}

END_OF_MAIN()
