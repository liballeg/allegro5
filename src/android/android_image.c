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
 *      Android image loading.
 *
 *      By Thomas Fjellstrom.
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_android.h"
#include "allegro5/internal/aintern_android.h"
#include "allegro5/internal/aintern_bitmap.h"

#include <jni.h>

A5O_DEBUG_CHANNEL("android")

static void
copy_bitmap_data(A5O_BITMAP *bitmap,
   const uint32_t *src, A5O_PIXEL_FORMAT src_format, int src_pitch,
   int bitmap_w, int bitmap_h)
{
   A5O_LOCKED_REGION *lr;

   lr = al_lock_bitmap(bitmap, A5O_PIXEL_FORMAT_ANY,
      A5O_LOCK_WRITEONLY);
   ASSERT(lr);
   if (!lr)
      return;

   _al_convert_bitmap_data(src, src_format, src_pitch,
      lr->data, lr->format, lr->pitch,
      0, 0, 0, 0, bitmap_w, bitmap_h);

   al_unlock_bitmap(bitmap);
}

static void
copy_bitmap_data_multiply_alpha(A5O_BITMAP *bitmap, const uint32_t *src,
   int bitmap_w, int bitmap_h)
{
   A5O_LOCKED_REGION *lr;
   int x, y;

   lr = al_lock_bitmap(bitmap, A5O_PIXEL_FORMAT_ABGR_8888,
      A5O_LOCK_WRITEONLY);
   ASSERT(lr);
   if (!lr)
      return;

   for (y = 0; y < bitmap_h; y++) {
      uint32_t *dst = (uint32_t *)(((uint8_t *)lr->data) + y * lr->pitch);
      for (x = 0; x < bitmap_w; x++) {
         uint32_t c = *src++;
         uint32_t a = (c >> 24) & 0xff;
         c = (a << 24)
           | (((c >> 16) & 0xff) * a / 255)
           | ((((c >> 8) & 0xff) * a / 255) << 8)
           | (((c & 0xff) * a / 255) << 16);
         *dst++ = c;
      }
   }

   al_unlock_bitmap(bitmap);
}

static void
copy_bitmap_data_demultiply_alpha(A5O_BITMAP *bitmap, const uint32_t *src,
   int src_format, int src_pitch, int bitmap_w, int bitmap_h)
{
   A5O_LOCKED_REGION *lr;
   int x, y;

   lr = al_lock_bitmap(bitmap, A5O_PIXEL_FORMAT_ABGR_8888,
      A5O_LOCK_WRITEONLY);
   ASSERT(lr);
   if (!lr)
      return;

   _al_convert_bitmap_data(src, src_format, src_pitch,
      lr->data, lr->format, lr->pitch,
      0, 0, 0, 0, bitmap_w, bitmap_h);

   for (y = 0; y < bitmap_h; y++) {
      uint32_t *dst = (uint32_t *)(((uint8_t *)lr->data) + y * lr->pitch);
      for (x = 0; x < bitmap_w; x++) {
         uint32_t c = *dst;
         uint8_t a = (c >> 24);
         uint8_t b = (c >> 16);
         uint8_t g = (c >> 8);
         uint8_t r = (c >> 0);
         // NOTE: avoid divide by zero by adding a fraction.
         float alpha_mul = 255.0f / (a+0.001f);
         r *= alpha_mul;
         g *= alpha_mul;
         b *= alpha_mul;
         *dst = ((uint32_t)a << 24)
              | ((uint32_t)b << 16)
              | ((uint32_t)g << 8)
              | ((uint32_t)r);
         dst++;
      }
   }

   al_unlock_bitmap(bitmap);
}

/* Note: This is not used when loading an image from the .apk.
 * 
 * The ImageLoader class uses Java to load a bitmap. To support
 * Allegro's filesystem functions, the bitmap is read from an
 * AllegroInputStream which in turn calls back into C to use Allegro's
 * file functions.
 */
A5O_BITMAP *_al_android_load_image_f(A5O_FILE *fh, int flags)
{
   JNIEnv *jnienv;
   jclass image_loader_class;
   jobject input_stream_class;
   jmethodID input_stream_ctor;
   jobject input_stream;
   int buffer_len;
   uint8_t *buffer;
   jobject byte_buffer;
   jobject jbitmap;
   A5O_BITMAP *bitmap;
   int bitmap_w;
   int bitmap_h;
   int pitch;

   if (flags & A5O_KEEP_INDEX) {
      A5O_ERROR("A5O_KEEP_INDEX not yet supported\n");
      return NULL;
   }

   jnienv = (JNIEnv *)_al_android_get_jnienv();
   // Note: This is always ImageLoader
   image_loader_class = _al_android_image_loader_class();
   // Note: This is always AllegroInputStream
   input_stream_class = _al_android_input_stream_class();
   input_stream_ctor = _jni_call(jnienv, jclass, GetMethodID,
       input_stream_class, "<init>", "(J)V");
   input_stream = _jni_call(jnienv, jobject, NewObject, input_stream_class,
      input_stream_ctor, (jlong)(intptr_t)fh);
   if (!input_stream) {
      A5O_ERROR("failed to create new AllegroInputStream object");
      return NULL;
   }

   jbitmap = _jni_callStaticObjectMethodV(jnienv, image_loader_class,
      "decodeBitmapStream",
      "(Ljava/io/InputStream;)Landroid/graphics/Bitmap;",
      input_stream);

   _jni_callv(jnienv, DeleteLocalRef, input_stream);

   if (!jbitmap)
      return NULL;

   bitmap_w = _jni_callIntMethod(jnienv, jbitmap, "getWidth");
   bitmap_h = _jni_callIntMethod(jnienv, jbitmap, "getHeight");
   pitch = _jni_callIntMethod(jnienv, jbitmap, "getRowBytes");

   buffer_len = pitch * bitmap_h;
   buffer = al_malloc(buffer_len);
   if (!buffer) {
      _jni_callv(jnienv, DeleteLocalRef, jbitmap);
      return NULL;
   }

   int src_format = _jni_callStaticIntMethodV(jnienv, image_loader_class,
      "getBitmapFormat", "(Landroid/graphics/Bitmap;)I", jbitmap);

   // FIXME: at some point add support for the ndk AndroidBitmap api need to
   // check for header at build time, and android version at runtime if thats
   // even possible, might need to try and dynamically load the lib? I dunno.
   // That would get rid of this buffer allocation and copy.
   byte_buffer = _jni_call(jnienv, jobject, NewDirectByteBuffer, buffer,
      buffer_len);

   _jni_callVoidMethodV(jnienv, jbitmap,
      "copyPixelsToBuffer", "(Ljava/nio/Buffer;)V", byte_buffer);

   // Tell java we don't need the byte_buffer object.
   _jni_callv(jnienv, DeleteLocalRef, byte_buffer);

   // Tell java we're done with the bitmap as well.
   _jni_callv(jnienv, DeleteLocalRef, jbitmap);

   bitmap = al_create_bitmap(bitmap_w, bitmap_h);
   if (!bitmap) {
      al_free(buffer);
      return NULL;
   }

   /* buffer already has alpha multiplied in. */
   if (flags & A5O_NO_PREMULTIPLIED_ALPHA) {
      copy_bitmap_data_demultiply_alpha(bitmap, (const uint32_t *)buffer,
         src_format, pitch, bitmap_w, bitmap_h);
   }
   else {
      copy_bitmap_data(bitmap, (const uint32_t *)buffer, src_format, pitch,
         bitmap_w, bitmap_h);
   }

   al_free(buffer);

   return bitmap;
}

static A5O_BITMAP *android_load_image_asset(const char *filename, int flags)
{
   JNIEnv *jnienv;
   jclass image_loader_class;
   jobject activity;
   jobject str;
   jobject jbitmap;
   int bitmap_w;
   int bitmap_h;
   A5O_BITMAP *bitmap;
   jintArray ia;
   jint *arr;

   if (flags & A5O_KEEP_INDEX) {
      A5O_ERROR("A5O_KEEP_INDEX not yet supported\n");
      return NULL;
   }

   jnienv = _al_android_get_jnienv();
   image_loader_class = _al_android_image_loader_class();
   activity = _al_android_activity_object();
   str = (*jnienv)->NewStringUTF(jnienv, filename);
   jbitmap = _jni_callStaticObjectMethodV(jnienv, image_loader_class,
      "decodeBitmapAsset",
      "(Landroid/app/Activity;Ljava/lang/String;)Landroid/graphics/Bitmap;",
      activity, str);

   /* For future Java noobs like me: If the calling thread is a Java
    * thread, it will clean up these references when the native method
    * returns. But here that's not the case (though technically we were
    * spawned ultimately by a Java thread, we never return.) In any case,
    * it never does any harm to release the reference anyway.
    */
   (*jnienv)->DeleteLocalRef(jnienv, str);

   if (!jbitmap)
      return NULL;

   bitmap_w = _jni_callIntMethod(jnienv, jbitmap, "getWidth");
   bitmap_h = _jni_callIntMethod(jnienv, jbitmap, "getHeight");
   A5O_DEBUG("bitmap dimensions: %d, %d", bitmap_w, bitmap_h);

   bitmap = al_create_bitmap(bitmap_w, bitmap_h);
   if (!bitmap) {
      _jni_callv(jnienv, DeleteLocalRef, jbitmap);
      return NULL;
   }

   ia = _jni_callStaticObjectMethodV(jnienv, image_loader_class,
      "getPixels", "(Landroid/graphics/Bitmap;)[I", jbitmap);
   arr = (*jnienv)->GetIntArrayElements(jnienv, ia, 0);

   /* arr is an array of packed colours, NON-premultiplied alpha. */
   if (flags & A5O_NO_PREMULTIPLIED_ALPHA) {
      int src_format = A5O_PIXEL_FORMAT_ARGB_8888;
      int src_pitch = bitmap_w * sizeof(uint32_t);
      copy_bitmap_data(bitmap, (const uint32_t *)arr,
         src_format, src_pitch, bitmap_w, bitmap_h);
   }
   else {
      copy_bitmap_data_multiply_alpha(bitmap, (const uint32_t *)arr,
         bitmap_w, bitmap_h);
   }

   (*jnienv)->ReleaseIntArrayElements(jnienv, ia, arr, JNI_ABORT);
   _jni_callv(jnienv, DeleteLocalRef, ia);
   _jni_callv(jnienv, DeleteLocalRef, jbitmap);

   return bitmap;
}

A5O_BITMAP *_al_android_load_image(const char *filename, int flags)
{
   A5O_FILE *fp;
   A5O_BITMAP *bmp;

   /* Bypass the A5O_FILE interface when we know the underlying stream
    * implementation, to avoid a lot of shunting between C and Java.
    * We could probably do this for normal filesystem as well.
    */
   if (al_get_new_file_interface() == _al_get_apk_file_vtable()) {
      return android_load_image_asset(filename, flags);
   }

   fp = al_fopen(filename, "rb");
   if (fp) {
      bmp = _al_android_load_image_f(fp, flags);
      al_fclose(fp);
      return bmp;
   }

   return NULL;
}

/* vim: set sts=3 sw=3 et: */
