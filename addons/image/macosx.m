#import <Foundation/Foundation.h>
#import <AppKit/NSImage.h>
#import <AppKit/NSGraphicsContext.h>

#include "allegro5/allegro.h"
#include "allegro5/fshook.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern_image.h"

#include "iio.h"

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1050
typedef float CGFloat;
#endif

ALLEGRO_DEBUG_CHANNEL("OSXIIO")

// Just to make sure it's never al_malloc.
#define apple_malloc malloc

static ALLEGRO_BITMAP *really_load_image(char *buffer, int size, int flags)
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc]init];
   ALLEGRO_BITMAP *bmp = NULL;
   void *pixels = NULL;
   /* Note: buffer is now owned (and later freed) by the data object. */
   NSData *nsdata = [[NSData alloc] initWithBytesNoCopy:buffer length:size];
   NSImage *image = [[NSImage alloc] initWithData:nsdata];
   [nsdata release];
   bool premul = !(flags & ALLEGRO_NO_PREMULTIPLIED_ALPHA);

   if (!image)
       goto done;

   /* Get the image representations */
   NSArray *reps = [image representations];
   NSImageRep *image_rep = [reps objectAtIndex: 0];

   // Note: Do we want to support this on OSX 10.5? It doesn't have
   // CGImageForProposedRect...
   //CGImageRef cgimage = [image_rep CGImageForProposedRect: nil context: nil hints: nil];
   
   if (!image_rep) {
      [image release];
      goto done;
   }

   /* Get the actual size in pixels from the representation */
   int w = [image_rep pixelsWide];
   int h = [image_rep pixelsHigh];

   ALLEGRO_DEBUG("Read image of size %dx%d\n", w, h);

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
   CGColorSpaceRef colour_space =
      CGColorSpaceCreateCalibratedRGB(
         whitePoint, blackPoint, gamma, matrix
      );
   CGContextRef context = CGBitmapContextCreate(pixels, w, h, 8, w * 4,
      colour_space,
      kCGImageAlphaPremultipliedLast);
      
   [NSGraphicsContext saveGraphicsState];
   [NSGraphicsContext setCurrentContext:[NSGraphicsContext
      graphicsContextWithGraphicsPort:context flipped:NO]];
   [image drawInRect:NSMakeRect(0, 0, w, h)
      fromRect:NSZeroRect
      operation : NSCompositeCopy
      fraction : 1.0];
   [NSGraphicsContext restoreGraphicsState];
   
   //CGContextDrawImage(context, CGRectMake(0.0, 0.0, (CGFloat)w, (CGFloat)h),
   //   cgimage);
   CGContextRelease(context);
   CGColorSpaceRelease(colour_space);
   
   [image release];

   /* Then create a bitmap out of the memory buffer. */
   bmp = al_create_bitmap(w, h);
   if (bmp) {
      ALLEGRO_LOCKED_REGION *lock = al_lock_bitmap(bmp,
            ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);
      int i;
      if (!premul) {
         int x;
         for (i = 0; i < h; i++) {
            unsigned char *src_ptr = (unsigned char *)pixels + w * 4 * i;
            unsigned char *dest_ptr = (unsigned char *)lock->data +
	       lock->pitch * i;
            for (x = 0; x < w; x++) {
               unsigned char r, g, b, a;
               r = *src_ptr++;
               g = *src_ptr++;
               b = *src_ptr++;
               a = *src_ptr++;
               if (a == 0) {
                   r = g = b = 0;     
               }
               else {
                  r = r * 255 / a;
                  g = g * 255 / a;
                  b = b * 255 / a;
               }
               *dest_ptr++ = r;
               *dest_ptr++ = g;
               *dest_ptr++ = b;
               *dest_ptr++ = a;
            }
         }
      }
      else {
         for (i = 0; i < h; i++) {
            memcpy(lock->data + lock->pitch * i, pixels + w * 4 * i, w * 4);
         }
      }
      al_unlock_bitmap(bmp);
   }
   al_free(pixels);
done:
   [pool drain];
   return bmp;
}


static ALLEGRO_BITMAP *_al_osx_load_image_f(ALLEGRO_FILE *f, int flags)
{
   ALLEGRO_BITMAP *bmp;
   ASSERT(f);
    
   int64_t size = al_fsize(f);
   if (size <= 0) {
      // TODO: Read from stream until we have the whole image
      return NULL;
   }
   /* Note: This *MUST* be the Apple malloc and not any wrapper, as the
    * buffer will be owned and freed by the NSData object not us.
    */
   void *buffer = apple_malloc(size);
   al_fread(f, buffer, size);

   /* Really load the image now. */
   bmp = really_load_image(buffer, size, flags);
   return bmp;
}


static ALLEGRO_BITMAP *_al_osx_load_image(const char *filename, int flags)
{
   ALLEGRO_FILE *fp;
   ALLEGRO_BITMAP *bmp;

   ASSERT(filename);

   ALLEGRO_DEBUG("Using native loader to read %s\n", filename);
   fp = al_fopen(filename, "rb");
   if (!fp)
      return NULL;

   bmp = _al_osx_load_image_f(fp, flags);

   al_fclose(fp);

   return bmp;
}


extern NSImage* NSImageFromAllegroBitmap(ALLEGRO_BITMAP* bmp);

bool _al_osx_save_image_f(ALLEGRO_FILE *f, const char *ident, ALLEGRO_BITMAP *bmp)
{
   NSBitmapImageFileType type;
   
   if (!strcmp(ident, ".bmp")) {
      type = NSBMPFileType;
   }
   else if (!strcmp(ident, ".jpg") || !strcmp(ident, ".jpeg")) {
      type = NSJPEGFileType;
   }
   else if (!strcmp(ident, ".gif")) {
      type = NSGIFFileType;
   }
   else if (!strcmp(ident, ".tif") || !strcmp(ident, ".tiff")) {
      type = NSTIFFFileType;
   }
   else if (!strcmp(ident, ".png")) {
      type = NSPNGFileType;
   }
   else {
      return false;
   }
   
   NSImage *image = NSImageFromAllegroBitmap(bmp);
   NSArray *reps = [image representations];
   NSData *nsdata = [NSBitmapImageRep representationOfImageRepsInArray: reps usingType: type properties: nil];
   
   size_t size = (size_t)[nsdata length];
   bool ret = al_fwrite(f, [nsdata bytes], size) == size;
   
   [image release];
   
   return ret;
}


bool _al_osx_save_image(const char *filename, ALLEGRO_BITMAP *bmp)
{
   ALLEGRO_FILE *fp;
   bool ret = false;

   fp = al_fopen(filename, "wb");
   if (fp) {
      ALLEGRO_PATH *path = al_create_path(filename);
      if (path) {
         ret = _al_osx_save_image_f(fp, al_get_path_extension(path), bmp);
         al_destroy_path(path);
      }
      al_fclose(fp);
   }

   return ret;
}


bool _al_osx_save_png_f(ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp)
{
   return _al_osx_save_image_f(f, ".png", bmp);
}

bool _al_osx_save_jpg_f(ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp)
{
   return _al_osx_save_image_f(f, ".jpg", bmp);
}

bool _al_osx_save_tif_f(ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp)
{
   return _al_osx_save_image_f(f, ".tif", bmp);
}

bool _al_osx_save_gif_f(ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp)
{
   return _al_osx_save_image_f(f, ".gif", bmp);
}


bool _al_osx_register_image_loader(void)
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   bool success = false;
   int num_types;
   int i;

   /* Get a list of all supported image types */
   NSArray *file_types = [NSImage imageFileTypes];
   num_types = [file_types count];
   for (i = 0; i < num_types; i++) {
      NSString *str = @".";
      NSString *type_str = [str stringByAppendingString: [file_types objectAtIndex: i]];
      const char *s = [type_str UTF8String];

      /* Unload previous loader, if any */
      al_register_bitmap_loader(s, NULL);
      al_register_bitmap_loader_f(s, NULL);

      ALLEGRO_DEBUG("Registering native loader for bitmap type %s\n", s);
      success |= al_register_bitmap_loader(s, _al_osx_load_image);
      success |= al_register_bitmap_loader_f(s, _al_osx_load_image_f);
   }
   
   char const *extensions[] = { ".tif", ".tiff", ".gif", ".png", ".jpg", ".jpeg", NULL };
   
   for (i = 0; extensions[i]; i++) {
      ALLEGRO_DEBUG("Registering native saver for bitmap type %s\n", extensions[i]);
      success |= al_register_bitmap_saver(extensions[i], _al_osx_save_image);
   }

   success |= al_register_bitmap_saver_f(".tif", _al_osx_save_tif_f);
   success |= al_register_bitmap_saver_f(".tiff", _al_osx_save_tif_f);
   success |= al_register_bitmap_saver_f(".gif", _al_osx_save_gif_f);
   success |= al_register_bitmap_saver_f(".png", _al_osx_save_png_f);
   success |= al_register_bitmap_saver_f(".jpg", _al_osx_save_jpg_f);
   success |= al_register_bitmap_saver_f(".jpeg", _al_osx_save_jpg_f);
   
   [pool drain];

   return success;
}

