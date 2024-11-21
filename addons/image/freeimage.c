/* Allegro wrapper routines for FreeImage
 * by Karthik Kumar Viswanathan <karthikkumar@gmail.com>.
 */

#include <FreeImage.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_image.h"

#include "iio.h"

ALLEGRO_DEBUG_CHANNEL("image")

static bool freeimage_initialized = false;

static void _fiio_al_error_handler(FREE_IMAGE_FORMAT fif, const char *message) {
   ALLEGRO_ERROR("FreeImage %s : %s\n", (fif == FIF_UNKNOWN)? "UNKNOWN" : FreeImage_GetFormatFromFIF(fif), message);
}

bool _al_init_fi(void)
{
   if (freeimage_initialized)
      return true;
   FreeImage_Initialise(FALSE);
   _al_add_exit_func(_al_shutdown_fi, "_al_shutdown_fi");
   FreeImage_SetOutputMessage(_fiio_al_error_handler);
   freeimage_initialized = true;
   return true;
}

void _al_shutdown_fi(void)
{
   if (!freeimage_initialized)
      return;
   FreeImage_DeInitialise();
   freeimage_initialized = false;
}

static ALLEGRO_BITMAP *_al_fi_to_al_bitmap(FIBITMAP *fib) {
   int width = 0, height = 0;
   ALLEGRO_BITMAP *bitmap = NULL;
   width = FreeImage_GetWidth(fib);
   height = FreeImage_GetHeight(fib);
   bitmap = al_create_bitmap(width, height);
   if (bitmap) {
      ALLEGRO_LOCKED_REGION *a_lock = al_lock_bitmap(bitmap,
         ALLEGRO_PIXEL_FORMAT_ARGB_8888, ALLEGRO_LOCK_WRITEONLY);
      if (a_lock) {
         unsigned char *out = (unsigned char *)a_lock->data;
         int out_inc = a_lock->pitch - (width*4);
         for (int j=height - 1; j > -1; --j) {
            for (int i=0; i < width; ++i) {
               RGBQUAD color = { 0.0, 0.0, 0.0, 0.0 } ;
               if (FreeImage_GetPixelColor(fib, i, j, &color) == FALSE) {
                  ALLEGRO_ERROR("Unable to get pixel data at %d,%d\n", i , j);
               }
               *out++ = (unsigned char) color.rgbBlue;
               *out++ = (unsigned char) color.rgbGreen;
               *out++ = (unsigned char) color.rgbRed;
               *out++ = (unsigned char) color.rgbReserved;
            }
            out += out_inc;
         }
         al_unlock_bitmap(bitmap);
      }
   }
   return bitmap;
}

ALLEGRO_BITMAP *_al_load_fi_bitmap(const char *filename, int flags)
{
   FIBITMAP *fib = NULL;
   ALLEGRO_BITMAP *bitmap = NULL;
   FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

   ASSERT(filename);
   ASSERT(freeimage_initialized == true);

   fif = FreeImage_GetFIFFromFilename(filename);
   if (fif == FIF_UNKNOWN)
      fif = FreeImage_GetFileType(filename, 0);
   if (fif == FIF_UNKNOWN) {
      ALLEGRO_WARN("Could not determine the file type for '%s'\n", filename);
      return NULL;
   }

   {
     FIBITMAP *fibRaw = FreeImage_Load(fif, filename, flags);
     if (!fibRaw)
       return NULL;
     fib = FreeImage_ConvertTo32Bits(fibRaw);
     FreeImage_Unload(fibRaw);
     if (!fib)
       return NULL;
   }

   bitmap = _al_fi_to_al_bitmap(fib);
   FreeImage_Unload(fib);
   return bitmap;
}

/* FreeImage requires stdcall on win32 for these. DLL_CALLCONV is provided by FreeImage.h */
static unsigned int DLL_CALLCONV _fiio_al_read(void *buffer, unsigned size, unsigned count, fi_handle handle) {
   return (unsigned int) al_fread((ALLEGRO_FILE *)handle, buffer, (count * size));
}

static unsigned int DLL_CALLCONV _fiio_al_write(void *buffer, unsigned size, unsigned count, fi_handle handle) {
   return (unsigned int) al_fwrite((ALLEGRO_FILE *)handle, buffer, (count * size));
}

static int DLL_CALLCONV _fiio_al_fseek(fi_handle handle, long offset, int origin) {
   return al_fseek((ALLEGRO_FILE *)handle, offset, origin);
}

static long DLL_CALLCONV _fiio_al_ftell(fi_handle handle) {
   return (long) al_ftell((ALLEGRO_FILE *)handle);
}

ALLEGRO_BITMAP *_al_load_fi_bitmap_f(ALLEGRO_FILE *f, int flags)
{
   FreeImageIO fio;
   ALLEGRO_BITMAP *bitmap = NULL;
   FIBITMAP *fib = NULL;
   FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

   if (flags != 0) {
      ALLEGRO_WARN("Ignoring bitmap loading flags.\n");
   }

   ASSERT(f);
   ASSERT(freeimage_initialized == true);

   fio.read_proc  = _fiio_al_read;
   fio.write_proc = _fiio_al_write;
   fio.seek_proc  = _fiio_al_fseek;
   fio.tell_proc  = _fiio_al_ftell;

   fif = FreeImage_GetFileTypeFromHandle(&fio, (fi_handle)f, 0);
   if (fif == FIF_UNKNOWN) {
      ALLEGRO_WARN("Could not determine the file type for Allegro file.\n");
      return NULL;
   }

   {
     FIBITMAP *fibRaw = FreeImage_LoadFromHandle(fif, &fio, (fi_handle)f, 0);
     if (!fibRaw)
       return NULL;
     fib = FreeImage_ConvertTo32Bits(fibRaw);
     FreeImage_Unload(fibRaw);
     if (!fib)
       return NULL;
   }

   bitmap = _al_fi_to_al_bitmap(fib);
   FreeImage_Unload(fib);
   return bitmap;
}

bool _al_identify_fi(ALLEGRO_FILE *f)
{
   FreeImageIO fio;
   FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

   ASSERT(f);
   ASSERT(freeimage_initialized == true);

   fio.read_proc  = _fiio_al_read;
   fio.write_proc = _fiio_al_write;
   fio.seek_proc  = _fiio_al_fseek;
   fio.tell_proc  = _fiio_al_ftell;

   fif = FreeImage_GetFileTypeFromHandle(&fio, (fi_handle)f, 0);
   if (fif == FIF_UNKNOWN) {
      ALLEGRO_WARN("Could not determine the file type for Allegro file.\n");
      return false;
   }

   return true;
}


/* vim: set sts=3 sw=3 et: */
