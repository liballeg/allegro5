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

ALLEGRO_DEBUG_CHANNEL("jni")

/* jni helpers */

#undef ALLEGRO_DEBUG
#define ALLEGRO_DEBUG(a, ...) (void)0

void _jni_checkException(JNIEnv *env)
{
   jthrowable exc;
   
   exc = (*env)->ExceptionOccurred(env);
   if (exc) {
      /* We don't do much with the exception, except that
         we print a debug message for it, clear it, and 
         throw a new exception. */
      jclass newExcCls;
      (*env)->ExceptionDescribe(env);
      (*env)->ExceptionClear(env);
      newExcCls = (*env)->FindClass(env, 
                     "java/lang/IllegalArgumentException");
      if (newExcCls == NULL) {
         /* Unable to find the exception class, give up. */
         return;
      }
      (*env)->ThrowNew(env, newExcCls, "thrown from C code");
   }
}

jobject _jni_callObjectMethod(JNIEnv *env, jobject object, char *name, char *sig)
{
   ALLEGRO_DEBUG("%s (%s)", name, sig);
   
   ALLEGRO_DEBUG("GetObjectClass");
   jclass class_id = (*env)->GetObjectClass(env, object);
   ALLEGRO_DEBUG("GetMethodID");
   jmethodID method_id = (*env)->GetMethodID(env, class_id, name, sig);
   ALLEGRO_DEBUG("CallObjectMethod");
   jobject ret = (*env)->CallObjectMethod(env, object, method_id);
   
   _jni_checkException(env);
   
   return ret;
}

ALLEGRO_USTR *_jni_getString(JNIEnv *env, jobject object)
{
   ALLEGRO_DEBUG("GetStringUTFLength");
   jsize len = (*env)->GetStringUTFLength(env, object);
   _jni_checkException(env);
   
   ALLEGRO_DEBUG("GetStringUTFChars");
   const char *str = (*env)->GetStringUTFChars(env, object, NULL);
   _jni_checkException(env);
   
   ALLEGRO_DEBUG("al_ustr_new_from_buffer");
   ALLEGRO_USTR *ustr = al_ustr_new_from_buffer(str, len);
   ALLEGRO_DEBUG("ReleaseStringUTFChars");
   (*env)->ReleaseStringUTFChars(env, object, str);
   _jni_checkException(env);
   
   return ustr;
}

ALLEGRO_USTR *_jni_callStringMethod(JNIEnv *env, jobject obj, char *name, char *sig)
{
   jobject str_obj = _jni_callObjectMethod(env, obj, name, sig);
   return _jni_getString(env, str_obj);
}

void _jni_callVoidMethod(JNIEnv *env, jobject obj, char *name)
{
   
   _jni_callVoidMethodV(env, obj, name, "()V");
}

void _jni_callVoidMethodV(JNIEnv *env, jobject obj, char *name, char *sig, ...)
{
   va_list ap;
   
   ALLEGRO_DEBUG("%s (%s)", name, sig);
   
   ALLEGRO_DEBUG("GetObjectClass");
   
   jclass class_id = (*env)->GetObjectClass(env, obj);
   _jni_checkException(env);
   
   ALLEGRO_DEBUG("GetMethodID");
   
   jmethodID method_id = (*env)->GetMethodID(env, class_id, name, sig);
   if(method_id == NULL) {
      ALLEGRO_DEBUG("couldn't find method :(");
      return;
   }
   
   _jni_checkException(env);
   
   ALLEGRO_DEBUG("CallVoidMethodV");
   
   va_start(ap, sig);
   (*env)->CallVoidMethodV(env, obj, method_id, ap);
   va_end(ap);
   
   _jni_checkException(env);
   
   (*env)->DeleteLocalRef(env, class_id);
   
}

int _jni_callIntMethod(JNIEnv *env, jobject obj, char *name)
{
   ALLEGRO_DEBUG("%s (%s)", name, "()I");
   
   ALLEGRO_DEBUG("GetObjectClass");
   
   jclass class_id = (*env)->GetObjectClass(env, obj);
   _jni_checkException(env);
   
   ALLEGRO_DEBUG("GetMethodID");
   
   jmethodID method_id = (*env)->GetMethodID(env, class_id, name, "()I");
   if(method_id == NULL) {
      ALLEGRO_DEBUG("couldn't find method :(");
      (*env)->DeleteLocalRef(env, class_id);
      return -1;
   }
   
   _jni_checkException(env);
   
   ALLEGRO_DEBUG("CallIntMethod");
   
   int res = (*env)->CallIntMethod(env, obj, method_id);
   _jni_checkException(env);
   
   (*env)->DeleteLocalRef(env, class_id);
      
   return res;
}

int _jni_callIntMethodV(JNIEnv *env, jobject obj, char *name, char *sig, ...)
{
   va_list ap;
   
   ALLEGRO_DEBUG("%s (%s)", name, sig);
   
   ALLEGRO_DEBUG("GetObjectClass");
   
   jclass class_id = (*env)->GetObjectClass(env, obj);
   _jni_checkException(env);
   
   ALLEGRO_DEBUG("GetMethodID");
   
   jmethodID method_id = (*env)->GetMethodID(env, class_id, name, sig);
   if(method_id == NULL) {
      ALLEGRO_DEBUG("couldn't find method :(");
      (*env)->DeleteLocalRef(env, class_id);
      return -1;
   }
   
   _jni_checkException(env);
   
   ALLEGRO_DEBUG("CallIntMethodV");
   
   va_start(ap, sig);
   int res = (*env)->CallIntMethodV(env, obj, method_id, ap);
   va_end(ap);
   
   _jni_checkException(env);
   
   (*env)->DeleteLocalRef(env, class_id);
      
   return res;
}

bool _jni_callBooleanMethodV(JNIEnv *env, jobject obj, char *name, char *sig, ...)
{
   va_list ap;
   
   ALLEGRO_DEBUG("%s (%s)", name, sig);
   
   ALLEGRO_DEBUG("GetObjectClass env:%p obj:%i", env, (int)obj);
   
   jclass class_id = (*env)->GetObjectClass(env, obj);
   _jni_checkException(env);
   
   ALLEGRO_DEBUG("GetMethodID");
   
   jmethodID method_id = (*env)->GetMethodID(env, class_id, name, sig);
   if(method_id == NULL) {
      ALLEGRO_DEBUG("couldn't find method :(");
      (*env)->DeleteLocalRef(env, class_id);
      return -1;
   }
   
   _jni_checkException(env);
   
   ALLEGRO_DEBUG("CallIntMethod");
   
   va_start(ap, sig);
   jboolean res = (*env)->CallBooleanMethodV(env, obj, method_id, ap);
   va_end(ap);
   
   _jni_checkException(env);
   
   (*env)->DeleteLocalRef(env, class_id);
   
   return res;
}