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
 *      Android input stream.
 *
 *      By Thomas Fjellstrom.
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_android.h"

#include <jni.h>

A5O_DEBUG_CHANNEL("android")

/* XXX A5O_FILE pointers currently passed as ints */
A5O_STATIC_ASSERT(android, sizeof(jlong) >= sizeof(A5O_FILE *));

JNI_FUNC(int, AllegroInputStream, nativeRead, (JNIEnv *env, jobject obj,
   jlong handle, jbyteArray array, int offset, int length))
{
   A5O_FILE *fp = (A5O_FILE *)(intptr_t)handle;
   int ret = -1;
   jbyte *array_ptr = NULL;
   ASSERT(fp != NULL);

   (void)obj;
   A5O_DEBUG("nativeRead begin: handle:%lli fp:%p offset:%i length:%i",
      handle, fp, offset, length);

   int array_len = _jni_call(env, int, GetArrayLength, array);
   A5O_DEBUG("array length: %i", array_len);

   array_ptr = _jni_call(env, jbyte *, GetByteArrayElements, array, NULL);
   ASSERT(array_ptr != NULL);

   A5O_DEBUG("al_fread: p:%p, o:%i, l:%i", array_ptr, offset, length);
   ret = al_fread(fp, array_ptr + offset, length);

   if (ret == 0 && al_feof(fp)) {
      /* InputStream.read() semantics. */
      ret = -1;
   }

   _jni_callv(env, ReleaseByteArrayElements, array, array_ptr, 0);

   A5O_DEBUG("nativeRead end");
   return ret;
}

JNI_FUNC(void, AllegroInputStream, nativeClose, (JNIEnv *env, jobject obj,
   jlong handle))
{
   A5O_FILE *fp = (A5O_FILE *)(intptr_t)handle;
   (void)env;
   (void)obj;
   al_fclose(fp);
}

/* vim: set sts=3 sw=3 et: */
