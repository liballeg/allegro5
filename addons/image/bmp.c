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
 *      BMP reader.
 *
 *      By Seymour Shlien.
 *
 *      OS/2 BMP support and BMP save function by Jonas Petersen.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern_convert.h"
#include "allegro5/internal/aintern_image.h"

#include "iio.h"

A5O_DEBUG_CHANNEL("image")

/* Do NOT simplify this to just (x), it doesn't work in MSVC. */
#define INT_TO_BOOL(x)   ((x) != 0)


#define BIT_RGB          0
#define BIT_RLE8         1
#define BIT_RLE4         2
#define BIT_BITFIELDS    3

#define OS2INFOHEADERSIZE     12
#define WININFOHEADERSIZE     40
#define WININFOHEADERSIZEV2   52
#define WININFOHEADERSIZEV3   56
#define WININFOHEADERSIZEV4   108
#define WININFOHEADERSIZEV5   124

typedef struct BMPFILEHEADER
{
   unsigned short bfType;
   unsigned long bfSize;
   unsigned short bfReserved1;
   unsigned short bfReserved2;
   unsigned long bfOffBits;
} BMPFILEHEADER;


/* Used for both OS/2 and Windows BMP. 
 * Contains only the parameters needed to load the image 
 */
typedef struct BMPINFOHEADER
{
   unsigned long biWidth;
   signed long biHeight;
   unsigned short biBitCount;
   unsigned long biCompression;
   unsigned long biClrUsed;
   uint32_t biRedMask;
   uint32_t biGreenMask;
   uint32_t biBlueMask;
   bool biHaveAlphaMask;
   uint32_t biAlphaMask;
} BMPINFOHEADER;


typedef struct WINBMPINFOHEADER
{                               /* size: 40 */
   unsigned long biWidth;
   signed long biHeight;
   unsigned short biPlanes;
   unsigned short biBitCount;
   unsigned long biCompression;
   unsigned long biSizeImage;
   unsigned long biXPelsPerMeter;
   unsigned long biYPelsPerMeter;
   unsigned long biClrUsed;
   unsigned long biClrImportant;
} WINBMPINFOHEADER;


typedef struct OS2BMPINFOHEADER
{                               /* size: 12 */
   unsigned short biWidth;
   unsigned short biHeight;
   unsigned short biPlanes;
   unsigned short biBitCount;
} OS2BMPINFOHEADER;


typedef void(*bmp_line_fn)(A5O_FILE *f, char *buf, char *data,
   int length, bool premul);



/* read_bmfileheader:
 *  Reads a BMP file header and check that it has the BMP magic number.
 */
static int read_bmfileheader(A5O_FILE *f, BMPFILEHEADER *fileheader)
{
   fileheader->bfType = (uint16_t)al_fread16le(f);
   fileheader->bfSize = (uint32_t)al_fread32le(f);
   fileheader->bfReserved1 = (uint16_t)al_fread16le(f);
   fileheader->bfReserved2 = (uint16_t)al_fread16le(f);
   fileheader->bfOffBits = (uint32_t)al_fread32le(f);

   if (fileheader->bfType != 0x4D42) {
      A5O_WARN("Not BMP format\n");
      return -1;
   }

   if (al_feof(f) || al_ferror(f)) {
      A5O_ERROR("Failed to read file header\n");
      return -1;
   }

   return 0;
}



/* read_win_bminfoheader:
 *  Reads information from a BMP file header.
 */
static int read_win_bminfoheader(A5O_FILE *f, BMPINFOHEADER *infoheader)
{
   WINBMPINFOHEADER win_infoheader;

   win_infoheader.biWidth = (uint32_t)al_fread32le(f);
   win_infoheader.biHeight = al_fread32le(f);
   win_infoheader.biPlanes = (uint16_t)al_fread16le(f);
   win_infoheader.biBitCount = (uint16_t)al_fread16le(f);
   win_infoheader.biCompression = (uint32_t)al_fread32le(f);
   win_infoheader.biSizeImage = (uint32_t)al_fread32le(f);
   win_infoheader.biXPelsPerMeter = (uint32_t)al_fread32le(f);
   win_infoheader.biYPelsPerMeter = (uint32_t)al_fread32le(f);
   win_infoheader.biClrUsed = (uint32_t)al_fread32le(f);
   win_infoheader.biClrImportant = (uint32_t)al_fread32le(f);

   infoheader->biWidth = win_infoheader.biWidth;
   infoheader->biHeight = win_infoheader.biHeight;
   infoheader->biBitCount = win_infoheader.biBitCount;
   infoheader->biCompression = win_infoheader.biCompression;
   infoheader->biClrUsed = win_infoheader.biClrUsed;

   if (al_feof(f) || al_ferror(f)) {
      A5O_ERROR("Failed to read file header\n");
      return -1;
   }

   return 0;
}



/* read_os2_bminfoheader:
 *  Reads information from an OS/2 format BMP file header.
 */
static int read_os2_bminfoheader(A5O_FILE *f, BMPINFOHEADER *infoheader)
{
   OS2BMPINFOHEADER os2_infoheader;

   os2_infoheader.biWidth = (uint16_t)al_fread16le(f);
   os2_infoheader.biHeight = (uint16_t)al_fread16le(f);
   os2_infoheader.biPlanes = (uint16_t)al_fread16le(f);
   os2_infoheader.biBitCount = (uint16_t)al_fread16le(f);

   infoheader->biWidth = os2_infoheader.biWidth;
   infoheader->biHeight = os2_infoheader.biHeight;
   infoheader->biBitCount = os2_infoheader.biBitCount;
   infoheader->biCompression = BIT_RGB;
   infoheader->biClrUsed = 0; /* default */

   if (al_feof(f) || al_ferror(f)) {
      A5O_ERROR("Failed to read file header\n");
      return -1;
   }

   return 0;
}



/* decode_bitfield:
 *  Converts a bitfield in to a shift+mask pair
 */
static void decode_bitfield(uint32_t m, int *shift_out, uint32_t *mask_out)
{
   int shift = 0;

   if (m == 0) {
      *shift_out = 0;
      *mask_out = 0;
      return;
   }

#ifdef __GNUC__
   shift = __builtin_ctz(m);
   m >>= shift;
#else
   while ((m & 1) == 0) {
      m >>= 1;
      ++shift;
   }
#endif

   *shift_out = shift;
   *mask_out = m;
}



/* read_palette:
 *  Loads the color palette for 1,4,8 bit formats.
 *  OS/2 bitmaps take 3 bytes per color.
 *  Windows bitmaps take 4 bytes per color.
 */
static void read_palette(int ncolors, PalEntry *pal, A5O_FILE *f,
   int flags, const BMPINFOHEADER *infoheader, int win_flag)
{
   int i;
   unsigned char c[3];
   uint32_t r, g, b, a;
   bool premul = !(flags & A5O_NO_PREMULTIPLIED_ALPHA);

   int as;
   uint32_t am;

   decode_bitfield(infoheader->biAlphaMask, &as, &am);

   for (i = 0; i < ncolors; i++) {
      uint32_t pixel;
      al_fread(f, c, 3);
      r = c[2];
      g = c[1];
      b = c[0];

      pixel = (r << 16) | (g << 8) | b;

      switch (am) {
         case 0x00:
            a = 255;
            break;

         case 0x01:
            a = ((pixel >> as) & am) * 255;
            break;

         case 0xFF:
            a = ((pixel >> as) & am);
            break;

         default:
            a = ((pixel >> as) & am) * 255 / am;
      }

      if (am && premul) {
         r = r * a / 255;
         g = g * a / 255;
         b = b * a / 255;
      }

      pal[i].r = r;
      pal[i].g = g;
      pal[i].b = b;
      pal[i].a = a;

      if (win_flag) {
         al_fgetc(f);
      }
   }
}



/* read_16le:
 *  Support function for reading 16-bit little endian values
 *  from a memory buffer.
 */
static uint16_t read_16le(void *buf)
{
   unsigned char *ucbuf = (unsigned char *)buf;

   return ucbuf[0] | (ucbuf[1] << 8);
}



/* read_32le:
 *  Support function for reading 32-bit little endian values
 *  from a memory buffer.
 */
static uint32_t read_32le(void *buf)
{
   unsigned char *ucbuf = (unsigned char *)buf;

   return ucbuf[0] | (ucbuf[1] << 8) | (ucbuf[2] << 16) | (ucbuf[3] << 24);
}



/* read_1bit_line:
 *  Support function for reading the 1 bit bitmap file format.
 */
static void read_1bit_line(A5O_FILE *f, char *buf, char *data,
   int length, bool premul)
{
   int i, j;
   unsigned char *ucbuf = (unsigned char *)buf;
   size_t bytes_wanted = ((length + 7) / 8 + 3) & ~3;

   size_t bytes_read = al_fread(f, ucbuf, bytes_wanted);
   memset(ucbuf + bytes_read, 0, bytes_wanted - bytes_read);

   (void)premul;
   (void)data;

   for (i = (length - 1) / 8; i >= 0; --i) {
      unsigned char x = ucbuf[i];

      for (j = 0; j < 8; ++j)
         ucbuf[i*8 + 7 - j] = (x & (1 << j)) >> j;
   }
}



/* read_2bit_line:
 *  Support function for reading the 2 bit bitmap file format.
 */
static void read_2bit_line(A5O_FILE *f, char *buf, char *data,
   int length, bool premul)
{
   int i;
   unsigned char *ucbuf = (unsigned char *)buf;
   size_t bytes_wanted = ((length + 3) / 4 + 3) & ~3;

   size_t bytes_read = al_fread(f, ucbuf, bytes_wanted);
   memset(ucbuf + bytes_read, 0, bytes_wanted - bytes_read);

   (void)premul;
   (void)data;

   for (i = (length - 1) / 4; i >= 0; --i) {
      unsigned char x = ucbuf[i];
      ucbuf[i*4]   = (x & 0xC0) >> 6;
      ucbuf[i*4+1] = (x & 0x30) >> 4;
      ucbuf[i*4+2] = (x & 0x0C) >> 2;
      ucbuf[i*4+3] = (x & 0x03);
   }
}



/* read_4bit_line:
 *  Support function for reading the 4 bit bitmap file format.
 */
static void read_4bit_line(A5O_FILE *f, char *buf, char *data,
   int length, bool premul)
{
   int i;
   unsigned char *ucbuf = (unsigned char *)buf;
   size_t bytes_wanted = ((length + 1) / 2 + 3) & ~3;

   size_t bytes_read = al_fread(f, ucbuf, bytes_wanted);
   memset(ucbuf + bytes_read, 0, bytes_wanted - bytes_read);

   (void)premul;
   (void)data;

   for (i = (length - 1) / 2; i >= 0; --i) {
      unsigned char x = ucbuf[i];
      ucbuf[i*2]   = (x & 0xF0) >> 4;
      ucbuf[i*2+1] = (x & 0x0F);
   }
}



/* read_8bit_line:
 *  Support function for reading the 8 bit bitmap file format.
 */
static void read_8bit_line(A5O_FILE *f, char *buf, char *data,
   int length, bool premul)
{
   size_t bytes_wanted = (length + 3) & ~3;

   size_t bytes_read = al_fread(f, buf, bytes_wanted);
   memset(buf + bytes_read, 0, bytes_wanted - bytes_read);

   (void)premul;
   (void)data;
}



/* read_16_rgb_555_line:
 *  Support function for reading the 16 bit / RGB555 bitmap file format.
 */
static void read_16_rgb_555_line(A5O_FILE *f, char *buf, char *data,
   int length, bool premul)
{
   int i;
   uint32_t *data32 = (uint32_t *)data;
   size_t bytes_wanted = (length + (length & 1)) * 2;

   size_t bytes_read = al_fread(f, buf, bytes_wanted);
   memset(buf + bytes_read, 0, bytes_wanted - bytes_read);

   (void)premul;

   for (i = 0; i < length; ++i) {
      uint16_t pixel = read_16le(buf + i*2);
      data32[i] = A5O_CONVERT_RGB_555_TO_ABGR_8888_LE(pixel);
   }
}



/* read_16_argb_1555_line:
 *  Support function for reading the 16 bit / ARGB1555 bitmap file format.
 */
static void read_16_argb_1555_line(A5O_FILE *f, char *buf, char *data,
   int length, bool premul)
{
   int i;
   uint32_t *data32 = (uint32_t *)data;
   size_t bytes_wanted = (length + (length & 1)) * 2;

   size_t bytes_read = al_fread(f, buf, bytes_wanted);
   memset(buf + bytes_read, 0, bytes_wanted - bytes_read);

   for (i = 0; i < length; ++i) {
      uint16_t pixel = read_16le(buf + i*2);
      data32[i] = A5O_CONVERT_ARGB_1555_TO_ABGR_8888_LE(pixel);

      if (premul && (pixel & 0x8000))
         data32[i] = 0;
   }
}



/* read_16_rgb_565_line:
 *  Support function for reading the 16 bit / RGB565 bitmap file format.
 */
static void read_16_rgb_565_line(A5O_FILE *f, char *buf, char *data,
   int length, bool premul)
{
   int i;
   uint32_t *data32 = (uint32_t *)data;
   size_t bytes_wanted = (length + (length & 1)) * 2;

   size_t bytes_read = al_fread(f, buf, bytes_wanted);
   memset(buf + bytes_read, 0, bytes_wanted - bytes_read);

   (void)premul;

   for (i = 0; i < length; i++) {
      uint16_t pixel = read_16le(buf + i*2);
      data32[i] = A5O_CONVERT_RGB_565_TO_ABGR_8888_LE(pixel);
   }
}



/* read_24_rgb_888_line:
 *  Support function for reading the 24 bit / RGB888 bitmap file format.
 */
static void read_24_rgb_888_line(A5O_FILE *f, char *buf, char *data,
   int length, bool premul)
{
   int bi, i;
   unsigned char *ucbuf = (unsigned char *)buf;
   uint32_t *data32 = (uint32_t *)data;
   size_t bytes_wanted = length * 3 + (length & 3);

   size_t bytes_read = al_fread(f, buf, bytes_wanted);
   memset(buf + bytes_read, 0, bytes_wanted - bytes_read);

   (void)premul;

   for (i = 0, bi = 0; i < (length & ~3); i += 4, bi += 3) {
      uint32_t a = read_32le(buf + bi*4);     // BGRB [LE:BRGB]
      uint32_t b = read_32le(buf + bi*4 + 4); // GRBG [LE:GBRG]
      uint32_t c = read_32le(buf + bi*4 + 8); // RBGR [LE:RGBR]

      uint32_t w = a;
      uint32_t x = (a >> 24) | (b << 8);
      uint32_t y = (b >> 16) | (c << 16);
      uint32_t z = (c >> 8);

      data32[i]   = A5O_CONVERT_RGB_888_TO_ABGR_8888_LE(w);
      data32[i+1] = A5O_CONVERT_RGB_888_TO_ABGR_8888_LE(x);
      data32[i+2] = A5O_CONVERT_RGB_888_TO_ABGR_8888_LE(y);
      data32[i+3] = A5O_CONVERT_RGB_888_TO_ABGR_8888_LE(z);
   }

   bi *= 4;

   for (; i < length; i++, bi += 3) {
      uint32_t pixel = ucbuf[bi] | (ucbuf[bi+1] << 8) | (ucbuf[bi+2] << 16);
      data32[i] = A5O_CONVERT_RGB_888_TO_ABGR_8888_LE(pixel);
   }
}



/* read_32_xrgb_8888_line:
 *  Support function for reading the 32 bit / XRGB8888 bitmap file format.
 */
static void read_32_xrgb_8888_line(A5O_FILE *f, char *buf, char *data,
   int length, bool premul)
{
   int i;
   uint32_t *data32 = (uint32_t *)data;
   size_t bytes_wanted = length * 4;

   size_t bytes_read = al_fread(f, buf, bytes_wanted);
   memset(buf + bytes_read, 0, bytes_wanted - bytes_read);

   (void)premul;

   for (i = 0; i < length; i++) {
      uint32_t pixel = read_32le(buf + i*4);
      data32[i] = A5O_CONVERT_XRGB_8888_TO_ABGR_8888_LE(pixel);
   }
}



/* read_32_rgbx_8888_line:
 *  Support function for reading the 32 bit / RGBX8888 bitmap file format.
 */
static void read_32_rgbx_8888_line(A5O_FILE *f, char *buf, char *data,
   int length, bool premul)
{
   int i;
   uint32_t *data32 = (uint32_t *)data;
   size_t bytes_wanted = length * 4;

   size_t bytes_read = al_fread(f, buf, bytes_wanted);
   memset(buf + bytes_read, 0, bytes_wanted - bytes_read);

   (void)premul;

   for (i = 0; i < length; i++) {
      uint32_t pixel = read_32le(buf + i*4);
      data32[i] = A5O_CONVERT_RGBX_8888_TO_ABGR_8888_LE(pixel);
   }
}



/* read_32_argb_8888_line:
 *  Support function for reading the 32 bit / ARGB8888 bitmap file format.
 */
static void read_32_argb_8888_line(A5O_FILE *f, char *buf, char *data,
   int length, bool premul)
{
   int i;
   uint32_t *data32 = (uint32_t *)data;
   size_t bytes_wanted = length * 4;

   size_t bytes_read = al_fread(f, buf, bytes_wanted);
   memset(buf + bytes_read, 0, bytes_wanted - bytes_read);

   for (i = 0; i < length; i++) {
      uint32_t pixel = read_32le(buf + i*4);
      uint32_t a = (pixel & 0xFF000000U) >> 24;
      data32[i] = A5O_CONVERT_ARGB_8888_TO_ABGR_8888_LE(pixel);

      if (premul && a != 255) {
         data[i*4+1] = data[i*4+1] * a / 255;
         data[i*4+2] = data[i*4+2] * a / 255;
         data[i*4+3] = data[i*4+3] * a / 255;
      }
   }
}



/* read_32_rgba_8888_line:
 *  Support function for reading the 32 bit / RGBA8888 bitmap file format.
 */
static void read_32_rgba_8888_line(A5O_FILE *f, char *buf, char *data,
   int length, bool premul)
{
   int i;
   uint32_t *data32 = (uint32_t *)data;
   size_t bytes_wanted = length * 4;

   size_t bytes_read = al_fread(f, buf, bytes_wanted);
   memset(buf + bytes_read, 0, bytes_wanted - bytes_read);

   for (i = 0; i < length; i++) {
      uint32_t pixel = read_32le(buf + i*4);
      uint32_t a = (pixel & 0x000000FFU);
      data32[i] = A5O_CONVERT_RGBA_8888_TO_ABGR_8888_LE(pixel);

      if (premul && a != 255) {
         data[i*4]   = data[i*4] * a / 255;
         data[i*4+1] = data[i*4+1] * a / 255;
         data[i*4+2] = data[i*4+2] * a / 255;
      }
   }
}



/* read_RGB_image:
 *  For reading the standard BMP image format
 */
static bool read_RGB_image(A5O_FILE *f, int flags,
   const BMPINFOHEADER *infoheader, A5O_LOCKED_REGION *lr,
   bmp_line_fn fn)
{
   int i, line, height, width, dir;
   size_t linesize;
   char *linebuf;
   bool premul = !(flags & A5O_NO_PREMULTIPLIED_ALPHA);

   height = infoheader->biHeight;
   width = infoheader->biWidth;

   // Includes enough space to read the padding for a line
   linesize = (infoheader->biWidth + 3) & ~3;

   if (infoheader->biBitCount < 8)
      linesize *= (8 / infoheader->biBitCount);
   else
      linesize *= (infoheader->biBitCount / 8);

   linebuf = al_malloc(linesize);

   if (!linebuf) {
      A5O_WARN("Failed to allocate pixel row buffer\n");
      return false;
   }

   line = height < 0 ? 0 : height - 1;
   dir = height < 0 ? 1 : -1;
   height = abs(height);

   for (i = 0; i < height; i++, line += dir) {
      char *data = (char *)lr->data + lr->pitch * line;
      fn(f, linebuf, data, width, premul);
   }

   al_free(linebuf);

   return true;
}

/* read_RGB_image_indices:
 *  For reading the palette indices from BMP image format
 */
static bool read_RGB_image_indices(A5O_FILE *f, int flags,
   const BMPINFOHEADER *infoheader, A5O_LOCKED_REGION *lr,
   bmp_line_fn fn)
{
   int i, line, height, width, dir;
   size_t linesize;
   char *linebuf;
   
   (void)flags;
   
   height = infoheader->biHeight;
   width = infoheader->biWidth;

   // Includes enough space to read the padding for a line
   linesize = (width + 3) & ~3;

   // Indices are always 8-bit, so no need to adjust linesize
   
   linebuf = al_malloc(linesize);

   if (!linebuf) {
      A5O_WARN("Failed to allocate pixel row buffer\n");
      return false;
   }

   if (height < 0) {
      dir = 1;
      line = 0;
      height = -height;
   } else {
      dir = -1;
      line = height - 1;
   }

   for (i = 0; i < height; i++, line += dir) {
      char *data = (char *)lr->data + lr->pitch * line;
      fn(f, linebuf, data, width, false);
      memcpy(data, linebuf, width);
   }

   al_free(linebuf);

   return true;
}

/* read_RGB_paletted_image:
 *  For reading the standard palette mapped BMP image format
 */
static bool read_RGB_paletted_image(A5O_FILE *f, int flags,
   const BMPINFOHEADER *infoheader, PalEntry* pal,
   A5O_LOCKED_REGION *lr, bmp_line_fn fn)
{
   int i, j, line, height, width, dir;
   size_t linesize;
   char *linebuf;

   (void)flags;

   height = infoheader->biHeight;
   width = infoheader->biWidth;

   // Includes enough space to read the padding for a line
   linesize = (width + 3) & ~3;

   if (infoheader->biBitCount < 8)
      linesize *= (8 / infoheader->biBitCount);
   else
      linesize *= (infoheader->biBitCount / 8);

   linebuf = al_malloc(linesize);

   if (!linebuf) {
      A5O_WARN("Failed to allocate pixel row buffer\n");
      return false;
   }

   line = height < 0 ? 0 : height - 1;
   dir = height < 0 ? 1 : -1;
   height = abs(height);

   for (i = 0; i < height; i++, line += dir) {
      char *data = (char *)lr->data + lr->pitch * line;
      fn(f, linebuf, data, width, false);

      for (j = 0; j < width; ++j) {
         unsigned char idx = linebuf[j];
         data[j*4]   = pal[idx].r;
         data[j*4+1] = pal[idx].g;
         data[j*4+2] = pal[idx].b;
         data[j*4+3] = pal[idx].a;
      }
   }

   al_free(linebuf);

   return true;
}



/* generate_scale_table:
 *  Helper function to generate color tables for bitfield format bitmaps.
 */
static void generate_scale_table(int* table, int entries)
{
   int i;

   for (i = 0; i < entries; ++i)
      table[i] = i * 255 / (entries - 1);
}



/* read_bitfields_image:
 *  For reading the generic bitfield compressed BMP image format
 */
static bool read_bitfields_image(A5O_FILE *f, int flags,
   const BMPINFOHEADER *infoheader, A5O_LOCKED_REGION *lr)
{
   int i, k, line, height, width, dir;
   size_t linesize, bytes_read;
   unsigned char *linebuf;
   int bytes_per_pixel = infoheader->biBitCount / 8;
   bool premul = !(flags & A5O_NO_PREMULTIPLIED_ALPHA);

   int rs, gs, bs, as;
   uint32_t rm, gm, bm, am;

   // Temporary colour conversion tables for 1..10 bit channels
   // Worst case: ~7KB is temporarily allocated
   int *tempconvert[10];
   int *rtable = NULL;
   int *gtable = NULL;
   int *btable = NULL;
   int *atable = NULL;

   height = infoheader->biHeight;
   width = infoheader->biWidth;
   // Includes enough space to read the padding for a line
   linesize = width * bytes_per_pixel + ((width * bytes_per_pixel) & 3);

   // Overallocate by bytes_per_pixel so a 32-bit read can overlap the end
   linebuf = al_malloc((width + 1) * bytes_per_pixel);

   if (!linebuf) {
      A5O_WARN("Failed to allocate pixel row buffer\n");
      return false;
   }

   decode_bitfield(infoheader->biRedMask, &rs, &rm);
   decode_bitfield(infoheader->biGreenMask, &gs, &gm);
   decode_bitfield(infoheader->biBlueMask, &bs, &bm);
   decode_bitfield(infoheader->biAlphaMask, &as, &am);

   for (i = 0; i < (int)(sizeof(tempconvert) / sizeof(int *)); ++i) {
      uint32_t mask = ~(0xFFFFFFFFU << (i+1)) & 0xFFFFFFFFU;

      switch (i) {
         case 0: tempconvert[i] = _al_rgb_scale_1; break;
         case 3: tempconvert[i] = _al_rgb_scale_4; break;
         case 4: tempconvert[i] = _al_rgb_scale_5; break;
         case 5: tempconvert[i] = _al_rgb_scale_6; break;
         default:
            if (rm == mask || gm == mask || bm == mask || am == mask) {
               int entries = (1 << (i+1));

               // Skip generating tables for tiny images
               if (width * height > entries * 2) {
                  tempconvert[i] = al_malloc(sizeof(int) * entries);
                  generate_scale_table(tempconvert[i], entries);
               }
               else {
                  tempconvert[i] = NULL;
               }
            }
            else {
               tempconvert[i] = NULL;
            }
      }

      if (rm == mask) rtable = tempconvert[i];
      if (gm == mask) gtable = tempconvert[i];
      if (bm == mask) btable = tempconvert[i];
      if (am == mask) atable = tempconvert[i];
   }

   line = height < 0 ? 0 : height - 1;
   dir = height < 0 ? 1 : -1;
   height = abs(height);

   for (i = 0; i < height; i++, line += dir) {
      unsigned char *data = (unsigned char *)lr->data + lr->pitch * line;

      bytes_read = al_fread(f, linebuf, linesize);
      memset(linebuf + bytes_read, 0, linesize - bytes_read);

      for (k = 0; k < width; k++) {
         uint32_t pixel = read_32le(linebuf + k*bytes_per_pixel);
         uint32_t r, g, b, a = 255;

         r = ((pixel >> rs) & rm);
         g = ((pixel >> gs) & gm);
         b = ((pixel >> bs) & bm);

         if (rtable)      r = rtable[r];
         else if (rm > 0) r = r * 255 / rm;

         if (gtable)      g = gtable[g];
         else if (gm > 0) g = g * 255 / gm;

         if (btable)      b = btable[b];
         else if (bm > 0) b = b * 255 / bm;

         if (am) {
            a = ((pixel >> as) & am);

            if (atable) a = atable[a];
            else        a = a * 255 / am;

            if (premul) {
               r = r * a / 255;
               g = g * a / 255;
               b = b * a / 255;
            }
         }

         data[0] = r;
         data[1] = g;
         data[2] = b;
         data[3] = a;

         data += 4;
      }
   }

   al_free(linebuf);

   for (i = 0; i < (int)(sizeof(tempconvert) / sizeof(int *)); ++i) {
      if (i != 0 && i != 3 && i != 4 && i != 5)
            al_free(tempconvert[i]);
   }

   return true;
}



/* read_RGB_image_32bit_alpha_hack:
 *  For reading the non-compressed BMP image format (32-bit).
 *  These are treatly specially because some programs put alpha information in
 *  the fourth byte of each pixel, which is normally just padding (and zero).
 *  We use a heuristic: if every pixel has zero in that fourth byte then assume
 *  the whole image is opaque (a=255).  Otherwise treat the fourth byte as an
 *  alpha channel.
 *
 *  Note that V3 headers include an alpha bit mask, which can properly indicate
 *  the presence or absence of an alpha channel.
 *  This hack is not required then.
 */
static bool read_RGB_image_32bit_alpha_hack(A5O_FILE *f, int flags,
   const BMPINFOHEADER *infoheader, A5O_LOCKED_REGION *lr)
{
   int i, j, line, startline, height, width, dir;
   int have_alpha = 0;
   size_t linesize;
   char *linebuf;
   const bool premul = !(flags & A5O_NO_PREMULTIPLIED_ALPHA);

   height = infoheader->biHeight;
   width = infoheader->biWidth;

   // Includes enough space to read the padding for a line
   linesize = (infoheader->biWidth + 3) & ~3;

   if (infoheader->biBitCount < 8)
      linesize *= (8 / infoheader->biBitCount);
   else
      linesize *= (infoheader->biBitCount / 8);

   linebuf = al_malloc(linesize);

   if (!linebuf) {
      A5O_WARN("Failed to allocate pixel row buffer\n");
      return false;
   }

   line = startline = height < 0 ? 0 : height - 1;
   dir = height < 0 ? 1 : -1;
   height = abs(height);

   for (i = 0; i < height; i++, line += dir) {
      unsigned char *data = (unsigned char *)lr->data + lr->pitch * line;

      /* Don't premultiply alpha here or the image will come out all black */
      read_32_argb_8888_line(f, linebuf, (char *)data, width, false);

      /* Check the alpha values of every pixel in the row */
      for (j = 0; j < width; j++) {
         have_alpha |= ((data[j*4+3] & 0xFF) != 0);
      }
   }

   /* Fixup pass - make imague opaque or premultiply alpha */
   if (!have_alpha) {
      line = startline;

      for (i = 0; i < height; i++, line += dir) {
         unsigned char *data = (unsigned char *)lr->data + lr->pitch * line;

         for (j = 0; j < width; j++) {
            data[j*4+3] = 255;
         }
      }
   }
   else if (premul) {
      line = startline;

      for (i = 0; i < height; i++, line += dir) {
         unsigned char *data = (unsigned char *)lr->data + lr->pitch * line;

         for (j = 0; j < width; j++) {
            data[j*4]   = data[j*4] * data[j*4+3] / 255;
            data[j*4+1] = data[j*4+1] * data[j*4+3] / 255;
            data[j*4+2] = data[j*4+2] * data[j*4+3] / 255;
         }
      }
   }

   al_free(linebuf);

   return true;
}

/* read_RLE8_compressed_image:
 *  For reading the 8 bit RLE compressed BMP image format.
 */
static bool read_RLE8_compressed_image(A5O_FILE *f, unsigned char *buf,
                                       const BMPINFOHEADER *infoheader)
{
   int count;
   unsigned char val;
   int j, pos, line, width, height, dir;
   int eolflag, eopicflag;

   eopicflag = 0;
   width = (int)infoheader->biWidth;
   height = abs((int)infoheader->biHeight);
   line = (infoheader->biHeight < 0) ? 0 : height - 1;
   dir = (infoheader->biHeight < 0) ? 1 : -1;

   while (eopicflag == 0) {
      pos = 0;                  /* x position in bitmap */
      eolflag = 0;              /* end of line flag */

      while ((eolflag == 0) && (eopicflag == 0)) {
         count = al_fgetc(f);
         if (count == EOF) {
            return false;
         }
         val = al_fgetc(f);

         if (count > 0) {       /* repeat pixel count times */
            if (count > width - pos) {
               count = width - pos;
            }

            for (j = 0; j < count; j++) {
               buf[line * width + pos] = val;
               pos++;
            }
         }
         else {
            switch (val) {

               case 0:         /* end of line flag */
                  eolflag = 1;
                  break;

               case 1:         /* end of picture flag */
                  eopicflag = 1;
                  break;

               case 2:         /* displace picture */
                  count = al_fgetc(f);
                  if (count == EOF) {
                     return false;
                  }
                  pos += count;
                  count = al_fgetc(f);
                  if (count == EOF) {
                     return false;
                  }
                  line += dir * count;
                  if (line < 0)
                     line = 0;
                  if (line >= height)
                     line = height - 1;
                  break;

               default:                      /* read in absolute mode */
                  count = val;
                  if (count > width - pos) {
                     count = width - pos;
                  }
                  for (j = 0; j < count; j++) {
                     val = al_fgetc(f);
                     buf[line * width + pos] = val;
                     pos++;
                  }

                  if (j % 2 == 1)
                     val = al_fgetc(f);    /* align on word boundary */

                  break;
            }
         }

         if (pos > width + 1)
            eolflag = 1;
      }

      line += dir;
      if (line < 0 || line >= height)
         eopicflag = 1;
   }
   return true;
}



/* read_RLE4_compressed_image:
 *  For reading the 4 bit RLE compressed BMP image format.
 */
static bool read_RLE4_compressed_image(A5O_FILE *f, unsigned char *buf,
                                       const BMPINFOHEADER *infoheader)
{
   unsigned char b[8];
   int count;
   unsigned short val;
   int j, k, pos, line, width, height, dir;
   int eolflag, eopicflag;

   eopicflag = 0;               /* end of picture flag */
   width = (int)infoheader->biWidth;
   height = abs((int)infoheader->biHeight);
   line = (infoheader->biHeight < 0) ? 0 : height - 1;
   dir = (infoheader->biHeight < 0) ? 1 : -1;

   while (eopicflag == 0) {
      pos = 0;
      eolflag = 0;              /* end of line flag */

      while ((eolflag == 0) && (eopicflag == 0)) {
         count = al_fgetc(f);
         if (count == EOF)
            return false;

         val = al_fgetc(f);

         if (count > 0) {       /* repeat pixels count times */
            if (count > width - pos) {
               count = width - pos;
            }
            b[1] = val & 15;
            b[0] = (val >> 4) & 15;
            for (j = 0; j < count; j++) {
               buf[line * width + pos] = b[j % 2];
               pos++;
            }
         }
         else {
            switch (val) {

               case 0:         /* end of line */
                  eolflag = 1;
                  break;

               case 1:         /* end of picture */
                  eopicflag = 1;
                  break;

               case 2:         /* displace image */
                  count = al_fgetc(f);
                  if (count == EOF)
                     return false;
                  pos += count;
                  count = al_fgetc(f);
                  if (count == EOF)
                     return false;
                  line += dir * count;
                  if (line < 0)
                     line = 0;
                  if (line >= height)
                     line = height - 1;
                  break;

               default:        /* read in absolute mode */
                  count = val;
                  if (count > width - pos) {
                     count = width - pos;
                  }
                  for (j = 0; j < count; j++) {
                     if ((j % 4) == 0) {
                        val = (uint16_t)al_fread16le(f);
                        for (k = 0; k < 2; k++) {
                           b[2 * k + 1] = val & 15;
                           val = val >> 4;
                           b[2 * k] = val & 15;
                           val = val >> 4;
                        }
                     }
                     buf[line * width + pos] = b[j % 4];
                     pos++;
                  }
                  break;
            }
         }

         if (pos > width + 1)
            eolflag = 1;
      }

      line += dir;
      if (line < 0 || line >= height)
         eopicflag = 1;
   }
   return true;
}



/*  Like load_bmp, but starts loading from the current place in the A5O_FILE
 *  specified. If successful the offset into the file will be left just after
 *  the image data. If unsuccessful the offset into the file is unspecified,
 *  i.e. you must either reset the offset to some known place or close the
 *  packfile. The packfile is not closed by this function.
 */
A5O_BITMAP *_al_load_bmp_f(A5O_FILE *f, int flags)
{
   BMPFILEHEADER fileheader;
   BMPINFOHEADER infoheader;
   A5O_BITMAP *bmp;
   PalEntry pal[256];
   int64_t file_start;
   int64_t header_start;
   unsigned long biSize;
   unsigned char *buf = NULL;
   A5O_LOCKED_REGION *lr;
   bool keep_index = INT_TO_BOOL(flags & A5O_KEEP_INDEX);
   bool loaded_ok;

   ASSERT(f);

   file_start = al_ftell(f);

   if (read_bmfileheader(f, &fileheader) != 0) {
      return NULL;
   }

   header_start = al_ftell(f);

   biSize = (uint32_t)al_fread32le(f);
   if (al_feof(f) || al_ferror(f)) {
      A5O_ERROR("EOF or file error while reading bitmap header.\n");
      return NULL;
   }

   switch (biSize) {
      case WININFOHEADERSIZE:
      case WININFOHEADERSIZEV2:
      case WININFOHEADERSIZEV3:
      case WININFOHEADERSIZEV4:
      case WININFOHEADERSIZEV5:
         if (read_win_bminfoheader(f, &infoheader) != 0) {
            return NULL;
         }
         break;

      case OS2INFOHEADERSIZE:
         if (read_os2_bminfoheader(f, &infoheader) != 0) {
            return NULL;
         }
         ASSERT(infoheader.biCompression == BIT_RGB);
         break;

      default:
         A5O_WARN("Unsupported header size: %ld\n", biSize);
         return NULL;
   }

   /* End of header for OS/2 and BITMAPV2INFOHEADER (V1). */
   if (biSize == OS2INFOHEADERSIZE || biSize == WININFOHEADERSIZE) {
      ASSERT(al_ftell(f) == header_start + (int64_t) biSize);
   }

   if ((int)infoheader.biWidth < 0) {
      A5O_WARN("negative width: %ld\n", infoheader.biWidth);
      return NULL;
   }
   if (infoheader.biCompression != BIT_RGB
       && infoheader.biCompression != BIT_RLE8
       && infoheader.biCompression != BIT_RLE4
       && infoheader.biCompression != BIT_BITFIELDS) {
      A5O_ERROR("Unsupported compression: 0x%x\n", (int) infoheader.biCompression);
      return NULL;
   }
   if (infoheader.biBitCount != 1 && infoheader.biBitCount != 2 && infoheader.biBitCount != 4 &&
       infoheader.biBitCount != 8 && infoheader.biBitCount != 16 && infoheader.biBitCount != 24 &&
       infoheader.biBitCount != 32) {
      A5O_WARN("unsupported bit depth: %d\n", infoheader.biBitCount);
      return NULL;
   }

   if (infoheader.biCompression == BIT_RLE4 && infoheader.biBitCount != 4) {
      A5O_WARN("unsupported bit depth for RLE4 compression: %d\n", infoheader.biBitCount);
      return NULL;
   }

   if (infoheader.biCompression == BIT_RLE8 && infoheader.biBitCount != 8) {
      A5O_WARN("unsupported bit depth for RLE8 compression: %d\n", infoheader.biBitCount);
      return NULL;
   }

   if (infoheader.biCompression == BIT_BITFIELDS && 
      infoheader.biBitCount != 16 && infoheader.biBitCount != 24 && infoheader.biBitCount != 32) {
      A5O_WARN("unsupported bit depth for bitfields compression: %d\n", infoheader.biBitCount);
      return NULL;
   }

   /* In BITMAPINFOHEADER (V1) the RGB bit masks are not part of the header.
    * In BITMAPV2INFOHEADER they form part of the header, but only valid when
    * for BITFIELDS images.
    */
   if (infoheader.biCompression == BIT_BITFIELDS
      || biSize >= WININFOHEADERSIZEV2) {
      infoheader.biRedMask = (uint32_t)al_fread32le(f);
      infoheader.biGreenMask = (uint32_t)al_fread32le(f);
      infoheader.biBlueMask = (uint32_t)al_fread32le(f);
   }

   /* BITMAPV3INFOHEADER and above include an Alpha bit mask. */
   if (biSize < WININFOHEADERSIZEV3) {
      infoheader.biHaveAlphaMask = false;
      infoheader.biAlphaMask = 0x0;
   }
   else {
      uint32_t pixel_mask = 0xFFFFFFFFU;
      infoheader.biHaveAlphaMask = true;
      infoheader.biAlphaMask = (uint32_t)al_fread32le(f);

      if (infoheader.biBitCount < 32)
         pixel_mask = ~(pixel_mask << infoheader.biBitCount) & 0xFFFFFFFFU;

      if ((infoheader.biAlphaMask & pixel_mask) == 0) {
         infoheader.biAlphaMask = 0;
         A5O_WARN("Ignoring invalid alpha mask\n");
      }
   }

   /* Seek past the end of the header to reach the palette / image data */
   if (biSize > WININFOHEADERSIZEV3) {
      if (!al_fseek(f, file_start + 14 + biSize, A5O_SEEK_SET)) {
         A5O_ERROR("Seek error\n");
         return NULL;
      }
   }

   if (infoheader.biBitCount <= 8) {
      int i;
      for (i = 0; i < 256; ++i) {
         pal[i].r = 0;
         pal[i].g = 0;
         pal[i].b = 0;
         pal[i].a = 255;
      }
   }

   /* Read the palette, if any.  Higher bit depth images _may_ have an optional
    * palette but we don't use it.
    */
   if (infoheader.biCompression != BIT_BITFIELDS
      && infoheader.biBitCount <= 8) {
      int win_flag = (biSize != OS2INFOHEADERSIZE);
      int ncolors = infoheader.biClrUsed;
      int extracolors = 0;
      int bytes_per_color = win_flag ? 4 : 3;

      if (infoheader.biClrUsed >= INT_MAX) {
         A5O_ERROR("Illegal palette size: %lu\n", infoheader.biClrUsed);
         return NULL;
      }

      if (win_flag) {
         if (ncolors == 0) {
            ncolors = (1 << infoheader.biBitCount);
         }
      }
      else {
         /* detect palette size for OS2v1 format BMP files */
         if (ncolors == 0) {
            ncolors = (fileheader.bfOffBits - 14 - OS2INFOHEADERSIZE) / 3;
         }

         if (ncolors == 0) {
            A5O_WARN("No palette in OS2v1 BMP file!\n");
         }
      }

      if (ncolors > 256) {
         A5O_WARN("Too many colors: %d\n", ncolors);
         ncolors = 256;
         extracolors = ncolors - 256;
      }

      read_palette(ncolors, pal, f, flags, &infoheader, win_flag);
      if (al_feof(f) || al_ferror(f)) {
         A5O_ERROR("EOF or I/O error\n");
         return NULL;
      }

      if (!al_fseek(f, extracolors * bytes_per_color, A5O_SEEK_SET)) {
         A5O_ERROR("Seek error\n");
         return NULL;
      }
   }
   else if (infoheader.biClrUsed && infoheader.biBitCount > 8) {
      int win_flag = (biSize != OS2INFOHEADERSIZE);
      int bytes_per_color = win_flag ? 4 : 3;

      if (!al_fseek(f, infoheader.biClrUsed * bytes_per_color, A5O_SEEK_CUR)) {
         A5O_ERROR("Seek error\n");
         return NULL;
      }
   }

   /* Skip to the pixel data only if it's outside of the image metadata */
   if (file_start + (int64_t)fileheader.bfOffBits > al_ftell(f)) {
      if (!al_fseek(f, file_start + fileheader.bfOffBits, A5O_SEEK_SET)) {
         A5O_ERROR("Seek error\n");
         return NULL;
      }
   }

   bmp = al_create_bitmap(infoheader.biWidth, abs((int)infoheader.biHeight));
   if (!bmp) {
      A5O_ERROR("Failed to create bitmap\n");
      return NULL;
   }
   
   if (infoheader.biWidth == 0 || infoheader.biHeight == 0) {
      A5O_WARN("Creating zero-sized bitmap\n");
      return bmp;
   }
   
   if (infoheader.biBitCount <= 8 && keep_index) {
      lr = al_lock_bitmap(bmp, A5O_PIXEL_FORMAT_SINGLE_CHANNEL_8,
         A5O_LOCK_WRITEONLY);
   }
   else {
      lr = al_lock_bitmap(bmp, A5O_PIXEL_FORMAT_ABGR_8888_LE,
         A5O_LOCK_WRITEONLY);
   }

   if (!lr) {
      A5O_ERROR("Failed to lock region\n");
      al_destroy_bitmap(bmp);
      return NULL;
   }

   if (infoheader.biCompression == BIT_RLE8
       || infoheader.biCompression == BIT_RLE4) {
      /* Questionable but most loaders handle this, so we should. */
      if (infoheader.biHeight < 0) {
         A5O_WARN("compressed bitmap with negative height\n");
      }

      /* RLE decoding may skip pixels so clear the buffer first. */
      buf = al_calloc(infoheader.biWidth, abs((int)infoheader.biHeight));
   }
   loaded_ok = true;
   switch (infoheader.biCompression) {
      case BIT_RGB:
         if (infoheader.biBitCount == 32 && !infoheader.biHaveAlphaMask) {
            if (!read_RGB_image_32bit_alpha_hack(f, flags, &infoheader, lr))
               return NULL;
         }
         else {
            bmp_line_fn fn = NULL;

            switch (infoheader.biBitCount) {
               case 1: fn = read_1bit_line; break;
               case 2: fn = read_2bit_line; break;
               case 4: fn = read_4bit_line; break;
               case 8: fn = read_8bit_line; break;
               case 16: fn = read_16_rgb_555_line; break;
               case 24: fn = read_24_rgb_888_line; break;
               case 32: fn = read_32_xrgb_8888_line; break;
               default:
                  A5O_ERROR("No decoding function for bit depth %d\n", infoheader.biBitCount);
                  return NULL;
            }

            if (infoheader.biBitCount == 16 && infoheader.biAlphaMask == 0x00008000U)
               fn = read_16_argb_1555_line;
            else if (infoheader.biBitCount == 32 && infoheader.biAlphaMask == 0xFF000000U)
               fn = read_32_argb_8888_line;
            if (keep_index) {
               if (!read_RGB_image_indices(f, flags, &infoheader, lr, fn))
                  return NULL;
            }
            else if (infoheader.biBitCount <= 8) {
               if (!read_RGB_paletted_image(f, flags, &infoheader, pal, lr, fn))
                  return NULL;
            }
            else {
               if (!read_RGB_image(f, flags, &infoheader, lr, fn))
                  return NULL;
            }
         }
         break;

      case BIT_RLE8:
         loaded_ok = read_RLE8_compressed_image(f, buf, &infoheader);
         if (!loaded_ok)
            A5O_ERROR("Error reading RLE8 data\n");
         break;

      case BIT_RLE4:
         loaded_ok = read_RLE4_compressed_image(f, buf, &infoheader);
         if (!loaded_ok)
            A5O_ERROR("Error reading RLE4 data\n");
         break;

      case BIT_BITFIELDS:
         if (infoheader.biBitCount == 16) {
            if (infoheader.biRedMask == 0x00007C00U && infoheader.biGreenMask == 0x000003E0U &&
                infoheader.biBlueMask == 0x0000001FU && infoheader.biAlphaMask == 0x00000000U) {
               loaded_ok = read_RGB_image(f, flags, &infoheader, lr, read_16_rgb_555_line);
            }
            else if (infoheader.biRedMask == 0x00007C00U && infoheader.biGreenMask == 0x000003E0U &&
                     infoheader.biBlueMask == 0x0000001FU && infoheader.biAlphaMask == 0x00008000U) {
               loaded_ok = read_RGB_image(f, flags, &infoheader, lr, read_16_argb_1555_line);
            }
            else if (infoheader.biRedMask == 0x0000F800U && infoheader.biGreenMask == 0x000007E0U &&
                     infoheader.biBlueMask == 0x0000001FU && infoheader.biAlphaMask == 0x00000000U) {
               loaded_ok = read_RGB_image(f, flags, &infoheader, lr, read_16_rgb_565_line);
            }
            else {
               loaded_ok = read_bitfields_image(f, flags, &infoheader, lr);
            }
         }
         else if (infoheader.biBitCount == 24) {
            if (infoheader.biRedMask == 0x00FF0000U && infoheader.biGreenMask == 0x0000FF00U &&
                infoheader.biBlueMask == 0x000000FFU && infoheader.biAlphaMask == 0x00000000U) {
               loaded_ok = read_RGB_image(f, flags, &infoheader, lr, read_24_rgb_888_line);
            }
            else {
               loaded_ok = read_bitfields_image(f, flags, &infoheader, lr);
            }
         }
         else if (infoheader.biBitCount == 32) {
            if (infoheader.biRedMask == 0x00FF0000U && infoheader.biGreenMask == 0x0000FF00U &&
                infoheader.biBlueMask == 0x000000FFU && infoheader.biAlphaMask == 0x00000000U) {
               loaded_ok = read_RGB_image(f, flags, &infoheader, lr, read_32_xrgb_8888_line);
            }
            else if (infoheader.biRedMask == 0x00FF0000U && infoheader.biGreenMask == 0x0000FF00U &&
                infoheader.biBlueMask == 0x000000FFU && infoheader.biAlphaMask == 0xFF000000U) {
               loaded_ok = read_RGB_image(f, flags, &infoheader, lr, read_32_argb_8888_line);
            }
            else if (infoheader.biRedMask == 0xFF000000U && infoheader.biGreenMask == 0x00FF0000U &&
                infoheader.biBlueMask == 0x0000FF00U && infoheader.biAlphaMask == 0x00000000U) {
               loaded_ok = read_RGB_image(f, flags, &infoheader, lr, read_32_rgbx_8888_line);
            }
            else if (infoheader.biRedMask == 0xFF000000U && infoheader.biGreenMask == 0x00FF0000U &&
                infoheader.biBlueMask == 0x0000FF00U && infoheader.biAlphaMask == 0x000000FFU) {
               loaded_ok = read_RGB_image(f, flags, &infoheader, lr, read_32_rgba_8888_line);
            }
            else {
               loaded_ok = read_bitfields_image(f, flags, &infoheader, lr);
            }
         }
         break;

      default:
         A5O_WARN("Unknown compression: %ld\n", infoheader.biCompression);
         loaded_ok = false;
         break;
   }

   if (infoheader.biCompression == BIT_RLE8
       || infoheader.biCompression == BIT_RLE4) {
      int x, y;
      unsigned char *data;

      for (y = 0; y < abs((int)infoheader.biHeight); y++) {
         data = (unsigned char *)lr->data + lr->pitch * y;
         for (x = 0; x < (int)infoheader.biWidth; x++) {
            if (keep_index) {
               data[0] = buf[y * infoheader.biWidth + x];
               data++;
            }
            else {
               data[0] = pal[buf[y * infoheader.biWidth + x]].r;
               data[1] = pal[buf[y * infoheader.biWidth + x]].g;
               data[2] = pal[buf[y * infoheader.biWidth + x]].b;
               data[3] = 255;
               data += 4;
            }
         }
      }
      al_free(buf);
   }

   if (bmp) {
      al_unlock_bitmap(bmp);
   }
   /* If something went wrong internally */
   if (!loaded_ok) {
      al_destroy_bitmap(bmp);
      bmp = NULL;
   }

   return bmp;
}



/*  Like save_bmp but writes into the A5O_FILE given instead of a new file.
 *  The packfile is not closed after writing is completed. On success the
 *  offset into the file is left after the TGA file just written. On failure
 *  the offset is left at the end of whatever incomplete data was written.
 */
bool _al_save_bmp_f(A5O_FILE *f, A5O_BITMAP *bmp)
{
   int bfSize;
   int biSizeImage;
   int bpp;
   int filler;
   int i, j;
   int w, h;
   A5O_LOCKED_REGION *lr;
   ASSERT(f);
   ASSERT(bmp);

   w = al_get_bitmap_width(bmp);
   h = al_get_bitmap_height(bmp);

   bpp = 24;
   filler = 3 - ((w * (bpp / 8) - 1) & 3);
   biSizeImage = (w * 3 + filler) * h;
   bfSize = 14 + WININFOHEADERSIZE + biSizeImage;

   al_set_errno(0);

   /* file_header */
   al_fwrite16le(f, 0x4D42);              /* bfType ("BM") */
   al_fwrite32le(f, bfSize);              /* bfSize */
   al_fwrite16le(f, 0);                   /* bfReserved1 */
   al_fwrite16le(f, 0);                   /* bfReserved2 */
   al_fwrite32le(f, 14 + WININFOHEADERSIZE); /* bfOffBits */

   /* info_header */
   al_fwrite32le(f, WININFOHEADERSIZE);   /* biSize */
   al_fwrite32le(f, w);                   /* biWidth */
   al_fwrite32le(f, h);                   /* biHeight */
   al_fwrite16le(f, 1);                   /* biPlanes */
   al_fwrite16le(f, bpp);                 /* biBitCount */
   al_fwrite32le(f, BIT_RGB);             /* biCompression */
   al_fwrite32le(f, biSizeImage);         /* biSizeImage */
   al_fwrite32le(f, 0xB12);               /* biXPelsPerMeter (0xB12 = 72 dpi) */
   al_fwrite32le(f, 0xB12);               /* biYPelsPerMeter */

   al_fwrite32le(f, 0);                   /* biClrUsed */
   al_fwrite32le(f, 0);                   /* biClrImportant */

   /* Don't really need the alpha channel, just the _LE.
    * Note that there exist 32-bit BMPs now so we could try to save those.
    */
   lr = al_lock_bitmap(bmp, A5O_PIXEL_FORMAT_ABGR_8888_LE,
      A5O_LOCK_READONLY);

   /* image data */
   for (i = h - 1; i >= 0; i--) {
      unsigned char *data = (unsigned char *)lr->data + i * lr->pitch;

      for (j = 0; j < w; j++) {
         unsigned char r = data[0];
         unsigned char g = data[1];
         unsigned char b = data[2];
         data += 4;

         al_fputc(f, b);
         al_fputc(f, g);
         al_fputc(f, r);
      }

      for (j = 0; j < filler; j++)
         al_fputc(f, 0);
   }

   al_unlock_bitmap(bmp);

   return al_get_errno() ? false : true;
}



A5O_BITMAP *_al_load_bmp(const char *filename, int flags)
{
   A5O_FILE *f;
   A5O_BITMAP *bmp;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f) {
      A5O_ERROR("Unable to open %s for reading.\n", filename);
      return NULL;
   }

   bmp = _al_load_bmp_f(f, flags);

   al_fclose(f);

   return bmp;
}


bool _al_save_bmp(const char *filename, A5O_BITMAP *bmp)
{
   A5O_FILE *f;
   bool retsave;
   bool retclose;
   ASSERT(filename);

   f = al_fopen(filename, "wb");
   if (!f) {
      A5O_ERROR("Unable to open %s for writing.\n", filename);
      return false;
   }

   retsave = _al_save_bmp_f(f, bmp);
   retclose = al_fclose(f);

   return retsave && retclose;
}


bool _al_identify_bmp(A5O_FILE *f)
{
   uint16_t x;
   uint16_t y;

   y = al_fread16le(f);

   if (y != 0x4D42)
      return false;

   if (!al_fseek(f, 14 - 2, A5O_SEEK_CUR))
      return false;

   x = al_fread16le(f);
   switch (x) {
      case WININFOHEADERSIZE:
      case WININFOHEADERSIZEV2:
      case WININFOHEADERSIZEV3:
      case WININFOHEADERSIZEV4:
      case WININFOHEADERSIZEV5:
      case OS2INFOHEADERSIZE:
         return true;
   }
   return false;
}

/* vim: set sts=3 sw=3 et: */
