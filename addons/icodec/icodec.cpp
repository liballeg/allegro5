/*
 * Allegro Magick++ image loading/saving routines
 * author: Ryan Dickie (c) 2008
 * image magick supports these formats:
 *     http://www.imagemagick.org/script/formats.php
 */
#include <Magick++.h>
#include <exception>
#include "allegro5/allegro5.h"

extern "C" {
   #include "allegro5/icodec.h"

   /* image magick++ reads/writes based on string format */
   /* FIXME: for some inexplicable reason these are opposite endianess
    * image magick problem or allegro5 weirdness? */
   const char* _pixel_format_string(int fmt) {
      switch (fmt) {
         case ALLEGRO_PIXEL_FORMAT_ARGB_8888:
            return "BGRA";
         case ALLEGRO_PIXEL_FORMAT_RGBA_8888:
            return "ABGR";
         case ALLEGRO_PIXEL_FORMAT_RGB_888:
            return "BGR";
         case ALLEGRO_PIXEL_FORMAT_ABGR_8888:
            return "RGBA";
         /* TODO other formats.. and what does the X mean? in BRGX_8888 */
         default:
            return NULL;
      }
   }

   ALLEGRO_BITMAP* al_load_image(const char* path) {
      try {
         Magick::Image in(path);
         unsigned int width = in.columns();
         unsigned int height = in.rows();
         ALLEGRO_BITMAP* bmp = al_create_bitmap(width,height);
         ALLEGRO_LOCKED_REGION lr;
         al_lock_bitmap(bmp, &lr, ALLEGRO_LOCK_WRITEONLY);
         const char* sfmt = _pixel_format_string(lr.format);
         if (!sfmt) {
            fprintf(stderr, "Unsupported pixel format. Currently 24bpp and 32bpp only\n");
            return NULL;
         }
         fprintf(stderr, "Pixel format %s\n",sfmt);
         in.write(0,0,width,height, sfmt, Magick::CharPixel, lr.data);
         al_unlock_bitmap(bmp);
         return bmp;
      } catch(const std::exception& err ) {
         fprintf(stderr, "icodec load error: %s\n", err.what());
      } 
      return NULL;
   }
   
   int al_save_image(ALLEGRO_BITMAP* bmp, const char* path) {
      try {
         unsigned int width = al_get_bitmap_width(bmp);
         unsigned int height = al_get_bitmap_height(bmp);
         const char* sfmt = NULL;
         ALLEGRO_LOCKED_REGION lr;
         al_lock_bitmap(bmp, &lr, ALLEGRO_LOCK_READONLY);
         sfmt = _pixel_format_string(lr.format);
         if (!sfmt) {
            fprintf(stderr, "Unsupported pixel format. Currently 24bpp and 32bpp only\n");
            return 0;
         }
         fprintf(stderr, "Pixel format %s\n",sfmt);
         Magick::Image out(width,height, sfmt, Magick::CharPixel, lr.data);
         al_unlock_bitmap(bmp);
         out.write(path);
         return 0;
      } catch( const std::exception& err ) {
         fprintf(stderr, "icodec write error: %s\n", err.what());
         return 1;
      } 
   }
} /* extern C */
