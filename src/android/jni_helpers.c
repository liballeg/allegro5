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
 *      Android jni helpers
 *
 *      By Thomas Fjellstrom.
 *
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_android.h"
#include <jni.h>
#include <stdarg.h>

A5O_DEBUG_CHANNEL("jni")

#define VERBOSE_DEBUG(a, ...) (void)0

/* jni helpers */

void __jni_checkException(JNIEnv *env, const char *file, const char *func, int line)
{
   jthrowable exc;

   exc = (*env)->ExceptionOccurred(env);
   if (exc) {
      A5O_DEBUG("GOT AN EXCEPTION @ %s:%i %s", file, line, func);
      /* We don't do much with the exception, except that
         we print a debug message for it, clear it, and
         throw a new exception. */
      (*env)->ExceptionDescribe(env);
      (*env)->ExceptionClear(env);
      (*env)->FatalError(env, "EXCEPTION");
      //(*env)->ThrowNew(env, system_data.illegal_argument_exception_class, "thrown from C code");
   }
}

jobject _jni_callObjectMethod(JNIEnv *env, jobject object,
   const char *name, const char *sig)
{
   VERBOSE_DEBUG("%s (%s)", name, sig);

   jclass class_id = _jni_call(env, jclass, GetObjectClass, object);
   jmethodID method_id = _jni_call(env, jmethodID, GetMethodID, class_id, name, sig);
   jobject ret = _jni_call(env, jobject, CallObjectMethod, object, method_id);

   _jni_callv(env, DeleteLocalRef, class_id);

   return ret;
}

jobject _jni_callObjectMethodV(JNIEnv *env, jobject object,
   const char *name, const char *sig, ...)
{
   va_list ap;

   VERBOSE_DEBUG("%s (%s)", name, sig);

   jclass class_id = _jni_call(env, jclass, GetObjectClass, object);
   jmethodID method_id = _jni_call(env, jmethodID, GetMethodID, class_id, name, sig);

   va_start(ap, sig);
   jobject ret = _jni_call(env, jobject, CallObjectMethodV, object, method_id, ap);
   va_end(ap);

   _jni_callv(env, DeleteLocalRef, class_id);

   VERBOSE_DEBUG("callObjectMethodV end");
   return ret;
}

A5O_USTR *_jni_getString(JNIEnv *env, jstring str_obj)
{
   VERBOSE_DEBUG("GetStringUTFLength");
   jsize len = _jni_call(env, jsize, GetStringUTFLength, str_obj);

   const char *str = _jni_call(env, const char *, GetStringUTFChars, str_obj, NULL);

   VERBOSE_DEBUG("al_ustr_new_from_buffer");
   A5O_USTR *ustr = al_ustr_new_from_buffer(str, len);

   _jni_callv(env, ReleaseStringUTFChars, str_obj, str);

   return ustr;
}

A5O_USTR *_jni_callStringMethod(JNIEnv *env, jobject obj,
   const char *name, const char *sig)
{
   jstring str_obj = (jstring)_jni_callObjectMethod(env, obj, name, sig);
   A5O_USTR *ustr = _jni_getString(env, str_obj);

   _jni_callv(env, DeleteLocalRef, str_obj);

   return ustr;
}

jobject _jni_callStaticObjectMethodV(JNIEnv *env, jclass cls,
   const char *name, const char *sig, ...)
{
   jmethodID mid;
   jobject ret;
   va_list ap;

   mid = _jni_call(env, jmethodID, GetStaticMethodID, cls, name, sig);

   va_start(ap, sig);
   ret = _jni_call(env, jobject, CallStaticObjectMethodV, cls, mid, ap);
   va_end(ap);

   return ret;
}

jint _jni_callStaticIntMethodV(JNIEnv *env, jclass cls,
   const char *name, const char *sig, ...)
{
   jmethodID mid;
   jint ret;
   va_list ap;

   mid = _jni_call(env, jmethodID, GetStaticMethodID, cls, name, sig);

   va_start(ap, sig);
   ret = _jni_call(env, jint, CallStaticIntMethodV, cls, mid, ap);
   va_end(ap);

   return ret;
}

/* vim: set sts=3 sw=3 et: */
