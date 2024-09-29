#include "allegro5/allegro.h"
#include "allegro5/allegro_android.h"
#include "allegro5/internal/aintern_android.h"

#define streq(a, b)  (0 == strcmp(a, b))

A5O_DEBUG_CHANNEL("android")


typedef struct A5O_FILE_APK A5O_FILE_APK;

struct A5O_FILE_APK
{
   jobject apk;
   bool error_indicator;
};


static A5O_FILE_APK *cast_stream(A5O_FILE *f)
{
   return (A5O_FILE_APK *)al_get_file_userdata(f);
}


static void apk_set_errno(A5O_FILE_APK *fp)
{
   al_set_errno(-1);

   if (fp) {
      fp->error_indicator = true;
   }
}


static jobject APK_openRead(const char *filename)
{
   JNIEnv *jnienv;
   jmethodID ctor;
   jstring str;
   jobject is, ref;
   jboolean res;

   jnienv = _al_android_get_jnienv();
   ctor = _jni_call(jnienv, jclass, GetMethodID,
      _al_android_apk_stream_class(), "<init>",
      "(L" A5O_ANDROID_PACKAGE_NAME_SLASH "/AllegroActivity;Ljava/lang/String;)V");
   str = (*jnienv)->NewStringUTF(jnienv, filename);

   ref = _jni_call(jnienv, jobject, NewObject, _al_android_apk_stream_class(),
      ctor, _al_android_activity_object(), str);

   is = _jni_call(jnienv, jobject, NewGlobalRef, ref);
   _jni_callv(jnienv, DeleteLocalRef, ref);
   _jni_callv(jnienv, DeleteLocalRef, str);

   res = _jni_callBooleanMethodV(_al_android_get_jnienv(), is, "open", "()Z");

   return (res) ? is : NULL;
}


static bool APK_close(jobject apk_stream)
{
   jboolean ret = _jni_callBooleanMethodV(_al_android_get_jnienv(), apk_stream,
      "close", "()Z");
   _jni_callv(_al_android_get_jnienv(), DeleteGlobalRef, apk_stream);

   if (ret) {
      apk_set_errno(NULL);
   }
   return ret;
}


static bool APK_seek(jobject apk_stream, long bytes)
{
   jboolean res = _jni_callBooleanMethodV(_al_android_get_jnienv(),
      apk_stream, "seek", "(J)Z", (jlong)bytes);
   return res;
}


static long APK_tell(jobject apk_stream)
{
   long res = _jni_callLongMethodV(_al_android_get_jnienv(),
      apk_stream, "tell", "()J");
   return res;
}


static int APK_read(jobject apk_stream, char *buf, int len)
{
   JNIEnv *jnienv = _al_android_get_jnienv();

   jbyteArray b = (*jnienv)->NewByteArray(jnienv, len);

   jint res = _jni_callIntMethodV(jnienv, apk_stream, "read", "([B)I", b);

   if (res > 0) {
      (*jnienv)->GetByteArrayRegion(jnienv, b, 0, res, (jbyte *)buf);
   }

   _jni_callv(jnienv, DeleteLocalRef, b);

   return res;
}


static long APK_size(jobject apk_stream)
{
   long res = _jni_callLongMethod(_al_android_get_jnienv(), apk_stream, "size");
   return res;
}


static void *file_apk_fopen(const char *filename, const char *mode)
{
   A5O_FILE_APK *fp;
   jobject apk;

   if (streq(mode, "r") || streq(mode, "rb"))
      apk = APK_openRead(filename);
   else
      return NULL;

   if (!apk) {
      apk_set_errno(NULL);
      return NULL;
   }

   fp = al_malloc(sizeof(*fp));
   if (!fp) {
      al_set_errno(ENOMEM);
      APK_close(apk);
      return NULL;
   }

   fp->apk = apk;
   fp->error_indicator = false;

   return fp;
}


static bool file_apk_fclose(A5O_FILE *f)
{
   A5O_FILE_APK *fp = cast_stream(f);
   bool ret;

   ret = APK_close(fp->apk);

   al_free(fp);

   return ret;
}


static size_t file_apk_fread(A5O_FILE *f, void *buf, size_t buf_size)
{
   A5O_FILE_APK *fp = cast_stream(f);
   int n;

   if (buf_size == 0)
      return 0;

   n = APK_read(fp->apk, buf, buf_size);
   if (n < 0) {
      apk_set_errno(fp);
      return 0;
   }
   return n;
}


static size_t file_apk_fwrite(A5O_FILE *f, const void *buf,
   size_t buf_size)
{
   // Not supported
   (void)f;
   (void)buf;
   (void)buf_size;
   return 0;
}


static bool file_apk_fflush(A5O_FILE *f)
{
   // Not supported
   (void)f;
   return true;
}


static int64_t file_apk_ftell(A5O_FILE *f)
{
   A5O_FILE_APK *fp = cast_stream(f);
   return APK_tell(fp->apk);
}


static bool file_apk_seek(A5O_FILE *f, int64_t offset, int whence)
{
   A5O_FILE_APK *fp = cast_stream(f);
   long base;

   switch (whence) {
      case A5O_SEEK_SET:
         base = 0;
         break;

      case A5O_SEEK_CUR:
         base = APK_tell(fp->apk);
         if (base < 0) {
            apk_set_errno(fp);
            return false;
         }
         break;

      case A5O_SEEK_END:
         base = APK_size(fp->apk);
         if (base < 0) {
            apk_set_errno(fp);
            return false;
         }
         break;

      default:
         al_set_errno(EINVAL);
         return false;
   }

   if (!APK_seek(fp->apk, base + offset)) {
      apk_set_errno(fp);
      return false;
   }

   return true;
}


static bool file_apk_feof(A5O_FILE *f)
{
   A5O_FILE_APK *fp = cast_stream(f);
   jboolean res = _jni_callBooleanMethodV(_al_android_get_jnienv(), fp->apk,
      "eof", "()Z");
   return res;
}


static int file_apk_ferror(A5O_FILE *f)
{
   A5O_FILE_APK *fp = cast_stream(f);

   return (fp->error_indicator) ? 1 : 0;
}


static const char *file_apk_ferrmsg(A5O_FILE *f)
{
   (void)f;
   return "";
}


static void file_apk_fclearerr(A5O_FILE *f)
{
   A5O_FILE_APK *fp = cast_stream(f);

   fp->error_indicator = false;
}


static off_t file_apk_fsize(A5O_FILE *f)
{
   A5O_FILE_APK *fp = cast_stream(f);
   return APK_size(fp->apk);
}


static const A5O_FILE_INTERFACE file_apk_vtable =
{
   file_apk_fopen,
   file_apk_fclose,
   file_apk_fread,
   file_apk_fwrite,
   file_apk_fflush,
   file_apk_ftell,
   file_apk_seek,
   file_apk_feof,
   file_apk_ferror,
   file_apk_ferrmsg,
   file_apk_fclearerr,
   NULL, /* default ungetc implementation */
   file_apk_fsize
};


const A5O_FILE_INTERFACE *_al_get_apk_file_vtable(void)
{
   return &file_apk_vtable;
}


/* Function: al_android_set_apk_file_interface
 */
void al_android_set_apk_file_interface(void)
{
   al_set_new_file_interface(&file_apk_vtable);
}


/* vim: set sts=3 sw=3 et: */
