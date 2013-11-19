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

ALLEGRO_DEBUG_CHANNEL("android")

ALLEGRO_BITMAP *_al_android_load_image_f(ALLEGRO_FILE *fh, int flags)
{
   int x, y;
   jobject byte_buffer;
   jobject jbitmap;

   JNIEnv *jnienv;
   jobject activity;
   jobject input_stream_class;
   jmethodID input_stream_ctor;
   jobject input_stream;
   uint8_t *buffer;
   int buffer_len = al_fsize(fh);
   ALLEGRO_BITMAP *bitmap = NULL;
   int bitmap_w;
   int bitmap_h;
   ALLEGRO_STATE state;

   /* XXX flags not respected */
   (void)flags;

   jnienv = (JNIEnv *)_al_android_get_jnienv();
   activity = _al_android_activity_object();
   input_stream_class = _al_android_input_stream_class();

   input_stream_ctor = _jni_call(jnienv, jclass, GetMethodID,
       input_stream_class, "<init>", "(I)V");

   jint fhi = (jint)fh;

   input_stream = _jni_call(jnienv, jobject, NewObject, input_stream_class,
      input_stream_ctor, fhi);

   if (!input_stream) {
      ALLEGRO_ERROR("failed to create new AllegroInputStream object");
      return NULL;
   }

   jbitmap = _jni_callObjectMethodV(jnienv, activity, "decodeBitmap_f",
      "(L" ALLEGRO_ANDROID_PACKAGE_NAME_SLASH "/AllegroInputStream;)Landroid/graphics/Bitmap;",
      input_stream);
   /* XXX failure should be allowed */
   ASSERT(jbitmap != NULL);

   _jni_callv(jnienv, DeleteLocalRef, input_stream);

   bitmap_w = _jni_callIntMethod(jnienv, jbitmap, "getWidth");
   bitmap_h = _jni_callIntMethod(jnienv, jbitmap, "getHeight");

   int pitch = _jni_callIntMethod(jnienv, jbitmap, "getRowBytes");
   buffer_len = pitch * bitmap_h;

   ALLEGRO_DEBUG("bitmap size: %i", buffer_len);

   buffer = al_malloc(buffer_len);
   if (!buffer)
      return NULL;

   ALLEGRO_DEBUG("malloc %i bytes ok", buffer_len);

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

   al_store_state(&state, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE);
   bitmap = al_create_bitmap(bitmap_w, bitmap_h);
   al_restore_state(&state);
   if (!bitmap) {
      al_free(buffer);
      return NULL;
   }

   //_al_ogl_upload_bitmap_memory(bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, buffer);
   ALLEGRO_LOCKED_REGION *lr = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ANY,
      ALLEGRO_LOCK_WRITEONLY);
   for (y = 0; y < bitmap_h; y++) {
      for (x = 0; x < bitmap->w; x++) {
         memcpy(
            ((uint8_t *)lr->data)+lr->pitch*y,
            buffer+pitch*y,
            bitmap_w*4
         );
      }
   }
   al_unlock_bitmap(bitmap);

   al_free(buffer);

   return bitmap;
}

ALLEGRO_BITMAP *_al_android_load_image(const char *filename, int flags)
{
   int x, y;
   jobject jbitmap;
   ALLEGRO_BITMAP *bitmap;
   int bitmap_w;
   int bitmap_h;
   ALLEGRO_STATE state;
   JNIEnv *jnienv;
   jobject activity;

   /* XXX flags not respected */
   (void)flags;

   jnienv = _al_android_get_jnienv();
   activity = _al_android_activity_object();

   jstring str = (*jnienv)->NewStringUTF(jnienv, filename);

   jbitmap = _jni_callObjectMethodV(jnienv, activity,
      "decodeBitmap", "(Ljava/lang/String;)Landroid/graphics/Bitmap;", str);
   if (!jbitmap)
      return NULL;

   /* For future Java noobs like me: If the calling thread is a Java
    * thread, it will clean up these references when the native method
    * returns. But here that's not the case (though technically we were
    * spawned ultimately by a Java thread, we never return.) In any case,
    * it never does any harm to release the reference anyway.
    */
   (*jnienv)->DeleteLocalRef(jnienv, str);

   bitmap_w = _jni_callIntMethod(jnienv, jbitmap, "getWidth");
   bitmap_h = _jni_callIntMethod(jnienv, jbitmap, "getHeight");

   ALLEGRO_DEBUG("bitmap dimensions: %d, %d", bitmap_w, bitmap_h);

   al_store_state(&state, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE);
   bitmap = al_create_bitmap(bitmap_w, bitmap_h);
   al_restore_state(&state);
   if (!bitmap) {
      _jni_callv(jnienv, DeleteLocalRef, jbitmap);
      return NULL;
   }

   ALLEGRO_LOCKED_REGION *lr = al_lock_bitmap(bitmap,
      ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);

   jintArray ia = _jni_callObjectMethodV(jnienv, activity,
      "getPixels", "(Landroid/graphics/Bitmap;)[I", jbitmap);
   jint *arr = (*jnienv)->GetIntArrayElements(jnienv, ia, 0);
   uint32_t *src = (uint32_t *)arr;
   uint32_t c;
   uint8_t a;
   for (y = 0; y < bitmap_h; y++) {
      /* Can use this for NO_PREMULTIPLIED_ALPHA I think
      _al_convert_bitmap_data(
         ((uint8_t *)arr) + (bitmap_w * 4) * y, ALLEGRO_PIXEL_FORMAT_ARGB_8888, bitmap_w*4,
         ((uint8_t *)lr->data) + lr->pitch * y, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, lr->pitch,
         0, 0, 0, 0, bitmap_w, 1
      );
      else use this:
      */
      uint32_t *dst = (uint32_t *)(((uint8_t *)lr->data) + lr->pitch * y);
      for (x = 0; x < bitmap_w; x++) {
         c = *src++;
         a = (c >> 24) & 0xff;
         c = (a << 24)
           | (((c >> 16) & 0xff) * a / 255)
           | ((((c >> 8) & 0xff) * a / 255) << 8)
           | (((c & 0xff) * a / 255) << 16);
         *dst++ = c;
      }
   }
   (*jnienv)->ReleaseIntArrayElements(jnienv, ia, arr, JNI_ABORT);
   _jni_callv(jnienv, DeleteLocalRef, ia);

   // Tell java we're done with the bitmap as well.
   _jni_callv(jnienv, DeleteLocalRef, jbitmap);

   al_unlock_bitmap(bitmap);

   return bitmap;
}

/* vim: set sts=3 sw=3 et: */
