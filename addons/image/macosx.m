#import <Foundation/Foundation.h>
#import <AppKit/NSImage.h>

#include "allegro5/allegro5.h"
#include "allegro5/fshook.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_image.h"

#include "iio.h"

ALLEGRO_DEBUG_CHANNEL("OSXIIO")

/* really_load_png:
 *  Worker routine, used by load_png and load_memory_png.
 */
static ALLEGRO_BITMAP *really_load_image(char *buffer, int size)
{
   ALLEGRO_BITMAP *bmp = NULL;
   void *pixels = NULL;
   /* Note: buffer is now owned (and later freed) by the data object. */
   NSData *nsdata = [NSData dataWithBytesNoCopy:buffer length:size];
   NSImage *image = [[NSImage alloc] initWithData:nsdata];
   if (!image)
      return NULL;

   NSSize s = [image size];
   int w = s.width;
   int h = s.height;

   NSArray *reps = [image representations];
   NSImageRep *image_rep = [reps objectAtIndex: 0];
   [image release];
   if (!image_rep) 
      return NULL;

   ALLEGRO_DEBUG("Read image of size %dx%d, %d representation(s)\n", w, h, (int)[reps count]);

   CGImageRef cgimage = [image_rep CGImageForProposedRect: nil context: nil hints: nil];

   /* Now we need to draw the image into a memory buffer. */
   pixels = _AL_MALLOC(w * h * 4);
   CGColorSpaceRef colour_space =
      CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
   CGContextRef context = CGBitmapContextCreate(pixels, w, h, 8, w * 4,
      colour_space,
      kCGImageAlphaPremultipliedLast);
   CGContextDrawImage(context, CGRectMake(0.0, 0.0, (CGFloat)w, (CGFloat)h),
      cgimage);
   CGContextRelease(context);
   CGColorSpaceRelease(colour_space);

   /* Then create a bitmap out of the memory buffer. */
   bmp = al_create_bitmap(w, h);
   if (bmp) {
      ALLEGRO_LOCKED_REGION *lock = al_lock_bitmap(bmp,
            ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_WRITEONLY);
      int i;
      for (i = 0; i < h; i++) {
         memcpy(lock->data + lock->pitch * i, pixels + w * 4 * i, w * 4);
      }
      al_unlock_bitmap(bmp);
   }
   _AL_FREE(pixels);
   return bmp;
}


/* Function: al_load_png_f
 *  Load a PNG file from disk, doing colour coversion if required.
 */
static ALLEGRO_BITMAP *_al_osx_load_image_f(ALLEGRO_FILE *f)
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
   void *buffer = malloc(size);
   al_fread(f, buffer, size);

   /* Really load the image now. */
   bmp = really_load_image(buffer, size);
   return bmp;
}


/* Function: al_load_png
 */
static ALLEGRO_BITMAP *_al_osx_load_image(const char *filename)
{
   ALLEGRO_FILE *fp;
   ALLEGRO_BITMAP *bmp;

   ASSERT(filename);

   ALLEGRO_DEBUG("Using native loader to read %s\n", filename);
   fp = al_fopen(filename, "rb");
   if (!fp)
      return NULL;

   bmp = _al_osx_load_image_f(fp);

   al_fclose(fp);

   return bmp;
}

bool _al_osx_register_image_loader(void)
{
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

   return success;
}

