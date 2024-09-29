#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern_image.h"

#include "iio.h"

A5O_DEBUG_CHANNEL("image")

static A5O_BITMAP *really_load_image(char *buffer, int size, int flags)
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

   A5O_BITMAP *bmp = NULL;
   void *pixels = NULL;
   /* Note: buffer is now owned (and later freed) by the data object. */
   NSData *nsdata = [NSData dataWithBytesNoCopy:buffer length:size];
   UIImage *uiimage = [UIImage imageWithData:nsdata];
   int w = uiimage.size.width;
   int h = uiimage.size.height;
   bool premul = !(flags & A5O_NO_PREMULTIPLIED_ALPHA);

   /* Now we need to draw the image into a memory buffer. */
   pixels = al_malloc(w * h * 4);
   CGFloat whitePoint[3] = {
      1, 1, 1
   };
   CGFloat blackPoint[3] = {
      0, 0, 0
   };
   CGFloat gamma[3] = {
      2.2, 2.2, 2.2
   };
   CGFloat matrix[9] = {
      1, 1, 1,
      1, 1, 1,
      1, 1, 1
   };
   /* This sets up a color space that results in identical values
    * as the image data itself, which is the same as the standalone
    * libpng loader
    */
   CGColorSpaceRef cs =
      CGColorSpaceCreateCalibratedRGB(
         whitePoint, blackPoint, gamma, matrix
      );
   CGContextRef context = CGBitmapContextCreate(pixels, w, h, 8, w * 4,
      cs, kCGImageAlphaPremultipliedLast);
   CGContextSetBlendMode(context, kCGBlendModeCopy);
   CGContextDrawImage(context, CGRectMake(0.0, 0.0, (CGFloat)w, (CGFloat)h),
      uiimage.CGImage);
   CGContextRelease(context);
   CGColorSpaceRelease(cs);

   /* Then create a bitmap out of the memory buffer. */
   bmp = al_create_bitmap(w, h);
   if (!bmp) goto done;
   A5O_LOCKED_REGION *lock = al_lock_bitmap(bmp,
      A5O_PIXEL_FORMAT_ABGR_8888, A5O_LOCK_WRITEONLY);

   if (!premul) {
      for (int i = 0; i < h; i++) {
         unsigned char *src_ptr = (unsigned char *)pixels + w * 4 * i;
         unsigned char *dest_ptr = (unsigned char *)lock->data +
            lock->pitch * i;
         for (int x = 0; x < w; x++) {
            unsigned char r, g, b, a;
            r = *src_ptr++;
            g = *src_ptr++;
            b = *src_ptr++;
            a = *src_ptr++;
            // NOTE: avoid divide by zero by adding a fraction
            float alpha_mul = 255.0f / (a+0.001f);
            r *= alpha_mul;
            g *= alpha_mul;
            b *= alpha_mul;
            *dest_ptr++ = r;
            *dest_ptr++ = g;
            *dest_ptr++ = b;
            *dest_ptr++ = a;
         }
      }
   }
   else {
      for (int i = 0; i < h; i++) {
         memcpy(lock->data + lock->pitch * i, pixels + w * 4 * i, w * 4);
      }
   }
   al_unlock_bitmap(bmp);
done:
   al_free(pixels);

   [pool release];

   return bmp;
}


A5O_BITMAP *_al_iphone_load_image_f(A5O_FILE *f, int flags)
{
   A5O_BITMAP *bmp;
   A5O_ASSERT(f);
    
   int64_t size = al_fsize(f);
   if (size <= 0) {
      // TODO: Read from stream until we have the whole image
      A5O_ERROR("Unable to determine file size.\n");
      return NULL;
   }
   /* Note: This *MUST* be the Apple malloc and not any wrapper, as the
    * buffer will be owned and freed by the NSData object not us.
    */
   void *buffer = al_malloc(size);
   al_fread(f, buffer, size);

   /* Really load the image now. */
   bmp = really_load_image(buffer, size, flags);
   return bmp;
}


A5O_BITMAP *_al_iphone_load_image(const char *filename, int flags)
{
   A5O_FILE *fp;
   A5O_BITMAP *bmp;

   A5O_ASSERT(filename);

   fp = al_fopen(filename, "rb");
   if (!fp) {
      A5O_ERROR("Unable to open %s for reading.\n", filename);
      return NULL;
   }

   bmp = _al_iphone_load_image_f(fp, flags);

   al_fclose(fp);

   return bmp;
}
