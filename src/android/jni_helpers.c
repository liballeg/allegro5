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

jobject _jni_callObjectMethod(JNIEnv *env, jobject object, char *name, char *sig)
{
   ALLEGRO_DEBUG("%s (%s)", name, sig);
   
   jclass class_id = _jni_call(env, jclass, GetObjectClass, object);
   jmethodID method_id = _jni_call(env, jmethodID, GetMethodID, class_id, name, sig);
   jobject ret = _jni_call(env, jobject, CallObjectMethod, object, method_id);
   
   return ret;
}

jobject _jni_callObjectMethodV(JNIEnv *env, jobject object, char *name, char *sig, ...)
{
   va_list ap;
   
   ALLEGRO_DEBUG("%s (%s)", name, sig);
   
   jclass class_id = _jni_call(env, jclass, GetObjectClass, object);
   jmethodID method_id = _jni_call(env, jmethodID, GetMethodID, class_id, name, sig);
   
   va_start(ap, sig);
   jobject ret = _jni_call(env, jobject, CallObjectMethodV, object, method_id, ap);
   va_end(ap);
   
   ALLEGRO_DEBUG("callObjectMethodV end");
   return ret;
}

ALLEGRO_USTR *_jni_getString(JNIEnv *env, jobject object)
{
   ALLEGRO_DEBUG("GetStringUTFLength");
   jsize len = _jni_call(env, jsize, GetStringUTFLength, object);
   
   const char *str = _jni_call(env, const char *, GetStringUTFChars, object, NULL);
   
   ALLEGRO_DEBUG("al_ustr_new_from_buffer");
   ALLEGRO_USTR *ustr = al_ustr_new_from_buffer(str, len);
   
   _jni_callv(env, ReleaseStringUTFChars, object, str);
   
   return ustr;
}

ALLEGRO_USTR *_jni_callStringMethod(JNIEnv *env, jobject obj, char *name, char *sig)
{
   jobject str_obj = _jni_callObjectMethod(env, obj, name, sig);
   return _jni_getString(env, str_obj);
}

/*
void _jni_callVoidMethod(JNIEnv *env, jobject obj, char *name)
{
   _jni_callVoidMethodV(env, obj, name, "()V");
}

void _jni_callVoidMethodV(JNIEnv *env, jobject obj, char *name, char *sig, ...)
{
   va_list ap;
   
   ALLEGRO_DEBUG("%s (%s)", name, sig);
   
   jclass class_id = _jni_call(env, jclass, GetObjectClass, obj);
   
   jmethodID method_id = _jni_call(env, jmethodID, GetMethodID, class_id, name, sig);
   if(method_id == NULL) {
      ALLEGRO_DEBUG("couldn't find method :(");
      return;
   }
   
   va_start(ap, sig);
   _jni_callv(env, CallVoidMethodV, obj, method_id, ap);
   va_end(ap);
   
   _jni_callv(env, DeleteLocalRef, class_id);
}

int _jni_callIntMethod(JNIEnv *env, jobject obj, char *name)
{
   return _jni_callIntMethodV(env, obj, name, "()I");
}

int _jni_callIntMethodV(JNIEnv *env, jobject obj, char *name, char *sig, ...)
{
   va_list ap;
   
   ALLEGRO_DEBUG("%s (%s)", name, sig);
   
   jclass class_id = _jni_call(env, jclass, GetObjectClass, obj);
   
   jmethodID method_id = _jni_call(env, jmethodID, GetMethodID, class_id, name, sig);
   
   if(method_id == NULL) {
      ALLEGRO_DEBUG("couldn't find method :(");
      _jni_callv(env, DeleteLocalRef, class_id);
      return -1;
   }
   
   va_start(ap, sig);
   int res = _jni_call(env, int, CallIntMethodV, obj, method_id, ap);
   va_end(ap);
   
   _jni_callv(env, DeleteLocalRef, class_id);
      
   return res;
}*/

/*
bool _jni_callBooleanMethodV(JNIEnv *env, jobject obj, char *name, char *sig, ...)
{
   va_list ap;
   
   ALLEGRO_DEBUG("%s (%s)", name, sig);

   jclass class_id = _jni_call(env, jclass, GetObjectClass, obj);
   
   jmethodID method_id = _jni_call(env, jmethodID, GetMethodID, class_id, name, sig);
   
   if(method_id == NULL) {
      ALLEGRO_DEBUG("couldn't find method :(");
      _jni_callv(env, DeleteLocalRef, class_id);
      return -1;
   }
   
   va_start(ap, sig);
   jboolean res = _jni_call(env, jboolean, CallBooleanMethodV, obj, method_id, ap);
   va_end(ap);
   
   _jni_callv(env, DeleteLocalRef, class_id);
   
   return res;
}
*/
