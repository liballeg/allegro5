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
 *      A simple DDS reader.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern_image.h"

#include "iio.h"

A5O_DEBUG_CHANNEL("image")

typedef int DWORD;

typedef struct {
   DWORD dwSize;
   DWORD dwFlags;
   DWORD dwFourCC;
   DWORD dwRGBBitCount;
   DWORD dwRBitMask;
   DWORD dwGBitMask;
   DWORD dwBBitMask;
   DWORD dwABitMask;
} DDS_PIXELFORMAT;

typedef struct {
   DWORD           dwSize;
   DWORD           dwFlags;
   DWORD           dwHeight;
   DWORD           dwWidth;
   DWORD           dwPitchOrLinearSize;
   DWORD           dwDepth;
   DWORD           dwMipMapCount;
   DWORD           dwReserved1[11];
   DDS_PIXELFORMAT ddspf;
   DWORD           dwCaps;
   DWORD           dwCaps2;
   DWORD           dwCaps3;
   DWORD           dwCaps4;
   DWORD           dwReserved2;
} DDS_HEADER;

#define DDS_HEADER_SIZE 124
#define DDS_PIXELFORMAT_SIZE 32

#define FOURCC(c0, c1, c2, c3) ((int)(c0) | ((int)(c1) << 8) | ((int)(c2) << 16) | ((int)(c3) << 24))

#define DDPF_FOURCC 0x4

A5O_BITMAP *_al_load_dds_f(A5O_FILE *f, int flags)
{
   A5O_BITMAP *bmp;
   DDS_HEADER header;
   DWORD magic;
   size_t num_read;
   int w, h, fourcc, format, block_width, block_height, block_size;
   A5O_STATE state;
   A5O_LOCKED_REGION *lr = NULL;
   int ii;
   char* bitmap_data;
   (void)flags;

   magic = al_fread32le(f);
   if (magic != 0x20534444) {
      A5O_ERROR("Invalid DDS magic number.\n");
      return NULL;
   }

   num_read = al_fread(f, &header, sizeof(DDS_HEADER));
   if (num_read != DDS_HEADER_SIZE) {
      A5O_ERROR("Wrong DDS header size. Got %d, expected %d.\n",
         (int)num_read, DDS_HEADER_SIZE);
      return NULL;
   }

   if (!(header.ddspf.dwFlags & DDPF_FOURCC)) {
      A5O_ERROR("Only compressed DDS formats supported.\n");
      return NULL;
   }

   w = header.dwWidth;
   h = header.dwHeight;
   fourcc = header.ddspf.dwFourCC;

   switch (fourcc) {
      case FOURCC('D', 'X', 'T', '1'):
         format = A5O_PIXEL_FORMAT_COMPRESSED_RGBA_DXT1;
         break;
      case FOURCC('D', 'X', 'T', '3'):
         format = A5O_PIXEL_FORMAT_COMPRESSED_RGBA_DXT3;
         break;
      case FOURCC('D', 'X', 'T', '5'):
         format = A5O_PIXEL_FORMAT_COMPRESSED_RGBA_DXT5;
         break;
      default:
         A5O_ERROR("Invalid pixel format.\n");
         return NULL;
   }

   block_width = al_get_pixel_block_width(format);
   block_height = al_get_pixel_block_height(format);
   block_size = al_get_pixel_block_size(format);

   al_store_state(&state, A5O_STATE_NEW_BITMAP_PARAMETERS);
   al_set_new_bitmap_flags(A5O_VIDEO_BITMAP);
   al_set_new_bitmap_format(format);
   bmp = al_create_bitmap(w, h);
   if (!bmp) {
      A5O_ERROR("Failed to create bitmap.\n");
      goto FAIL;
   }

   if (al_get_bitmap_format(bmp) != format) {
      A5O_ERROR("Created a bad bitmap.\n");
      goto FAIL;
   }

   lr = al_lock_bitmap_blocked(bmp, A5O_LOCK_WRITEONLY);

   if (!lr) {
      switch (format) {
         case A5O_PIXEL_FORMAT_COMPRESSED_RGBA_DXT1:
         case A5O_PIXEL_FORMAT_COMPRESSED_RGBA_DXT3:
         case A5O_PIXEL_FORMAT_COMPRESSED_RGBA_DXT5:
            A5O_ERROR("Could not lock the bitmap (probably the support for locking this format has not been enabled).\n");
            break;
         default:
            A5O_ERROR("Could not lock the bitmap.\n");
      }
      return NULL;
   }

   bitmap_data = lr->data;

   for (ii = 0; ii < (h + block_height - 1) / block_height; ii++) {
      size_t pitch = (size_t)((w + block_width - 1) / block_width * block_size);
      num_read = al_fread(f, bitmap_data, pitch);
      if (num_read != pitch) {
         A5O_ERROR("DDS file too short.\n");
         goto FAIL;
      }
      bitmap_data += lr->pitch;
   }
   al_unlock_bitmap(bmp);

   goto RESET;
FAIL:
   if (lr)
      al_unlock_bitmap(bmp);
   al_destroy_bitmap(bmp);
   bmp = NULL;
RESET:
   al_restore_state(&state);
   return bmp;
}

A5O_BITMAP *_al_load_dds(const char *filename, int flags)
{
   A5O_FILE *f;
   A5O_BITMAP *bmp;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f) {
      A5O_ERROR("Unable open %s for reading.\n", filename);
      return NULL;
   }

   bmp = _al_load_dds_f(f, flags);

   al_fclose(f);

   return bmp;
}

bool _al_identify_dds(A5O_FILE *f)
{
   uint8_t x[4];
   uint32_t y;
   al_fread(f, x, 4);
   if (memcmp(x, "DDS ", 4) != 0)
      return false;
   y = al_fread32le(f);
   if (y != 124)
      return false;
   return true;
}
