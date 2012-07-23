#ifndef ALLEGRO_AINTERN_ANDROID_H
#define ALLEGRO_AINTERN_ANDROID_H

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/allegro_opengl.h"

#include <jni.h>

typedef struct ALLEGRO_SYSTEM_ANDROID {
   ALLEGRO_SYSTEM system;
   
   ALLEGRO_EXTRA_DISPLAY_SETTINGS visual;
   
} ALLEGRO_SYSTEM_ANDROID;

typedef struct ALLEGRO_DISPLAY_ANDROID {
   ALLEGRO_DISPLAY display;
   jobject surface_object;
   
   ALLEGRO_COND *cond;
   ALLEGRO_MUTEX *mutex;
   
   bool created;
   bool recreate;
   bool first_run;
   bool resize_acknowledge;
   bool resize_acknowledge2;
   bool resumed;
   bool failed;
} ALLEGRO_DISPLAY_ANDROID;

ALLEGRO_SYSTEM_INTERFACE *_al_system_android_interface();
/* ALLEGRO_SYSTEM_INTERFACE *_al_system_android_jni_interface(); */

ALLEGRO_DISPLAY_INTERFACE *_al_get_android_display_driver(void);
ALLEGRO_KEYBOARD_DRIVER *_al_get_android_keyboard_driver(void);
ALLEGRO_MOUSE_DRIVER *_al_get_android_mouse_driver(void);
ALLEGRO_TOUCH_INPUT_DRIVER *_al_get_android_touch_input_driver(void);
ALLEGRO_JOYSTICK_DRIVER *_al_get_android_joystick_driver(void);
bool _al_get_android_montior_info(int adapter, ALLEGRO_MONITOR_INFO *info);

int _al_android_get_display_orientation(void);

#define _jni_checkException(env) __jni_checkException(env, __FILE__, __FUNCTION__, __LINE__)
void __jni_checkException(JNIEnv *env, const char *file, const char *fname, int line);

#define _jni_call(env, rett, method, args...) ({ \
   /*ALLEGRO_DEBUG("_jni_call: %s(%s)", #method, #args);*/ \
   rett ret = (*env)->method(env, ##args); \
   _jni_checkException(env); \
   ret; \
})

#define _jni_callv(env, method, args...) ({ \
   /*ALLEGRO_DEBUG("_jni_call: %s(%s)", #method, #args);*/ \
   (*env)->method(env, ##args); \
   _jni_checkException(env); \
})

#define _jni_callBooleanMethodV(env, obj, name, sig, args...) ({ \
   jclass class_id = _jni_call(env, jclass, GetObjectClass, obj); \
   \
   jmethodID method_id = _jni_call(env, jmethodID, GetMethodID, class_id, name, sig); \
   \
   bool ret = false; \
   if(method_id == NULL) { \
      ALLEGRO_DEBUG("couldn't find method :("); \
   } \
   else { \
      ret = _jni_call(env, bool, CallBooleanMethod, obj, method_id, ##args); \
   } \
   \
   _jni_callv(env, DeleteLocalRef, class_id); \
   \
   ret; \
})

jobject _jni_callObjectMethod(JNIEnv *env, jobject object, char *name, char *sig);
jobject _jni_callObjectMethodV(JNIEnv *env, jobject object, char *name, char *sig, ...);
ALLEGRO_USTR *_jni_getString(JNIEnv *env, jobject object);
ALLEGRO_USTR *_jni_callStringMethod(JNIEnv *env, jobject obj, char *name, char *sig);
//void _jni_callVoidMethod(JNIEnv *env, jobject obj, char *name);
//void _jni_callVoidMethodV(JNIEnv *env, jobject obj, char *name, char *sig, ...);

//int _jni_callIntMethod(JNIEnv *env, jobject, char *name);
//int _jni_callIntMethodV(JNIEnv *env, jobject obj, char *name, char *sig, ...);

#define _jni_callIntMethodV(env, obj, name, sig, args...) ({ \
   /*ALLEGRO_DEBUG("_jni_callIntMethodV: %s (%s)", name, sig);*/ \
   jclass class_id = _jni_call(env, jclass, GetObjectClass, obj); \
   \
   jmethodID method_id = _jni_call(env, jmethodID, GetMethodID, class_id, name, sig); \
   \
   int ret = -1; \
   if(method_id == NULL) { \
      ALLEGRO_DEBUG("couldn't find method :("); \
   } \
   else { \
      ret = _jni_call(env, int, CallIntMethod, obj, method_id, ##args); \
   } \
   \
   _jni_callv(env, DeleteLocalRef, class_id); \
   \
   ret; \
})

#define _jni_callIntMethod(env, obj, name) _jni_callIntMethodV(env, obj, name, "()I");

#define _jni_callLongMethodV(env, obj, name, sig, args...) ({ \
   /*ALLEGRO_DEBUG("_jni_callLongMethodV: %s (%s)", name, sig);*/ \
   jclass class_id = _jni_call(env, jclass, GetObjectClass, obj); \
   \
   jmethodID method_id = _jni_call(env, jmethodID, GetMethodID, class_id, name, sig); \
   \
   long ret = -1; \
   if(method_id == NULL) { \
      ALLEGRO_DEBUG("couldn't find method :("); \
   } \
   else { \
      ret = _jni_call(env, long, CallLongMethod, obj, method_id, ##args); \
   } \
   \
   _jni_callv(env, DeleteLocalRef, class_id); \
   \
   ret; \
})

#define _jni_callLongMethod(env, obj, name) _jni_callLongMethodV(env, obj, name, "()J");

#define _jni_callVoidMethodV(env, obj, name, sig, args...) ({ \
   /*ALLEGRO_DEBUG("_jni_callVoidMethodV: %s (%s)", name, sig);*/ \
   \
   jclass class_id = _jni_call(env, jclass, GetObjectClass, obj); \
   \
   jmethodID method_id = _jni_call(env, jmethodID, GetMethodID, class_id, name, sig); \
   if(method_id == NULL) { \
      ALLEGRO_ERROR("couldn't find method :("); \
   } else { \
      _jni_callv(env, CallVoidMethod, obj, method_id, ##args); \
   } \
   \
   _jni_callv(env, DeleteLocalRef, class_id); \
})

#define _jni_callVoidMethod(env, obj, name) _jni_callVoidMethodV(env, obj, name, "()V");

void _al_android_touch_input_handle_begin(int id, double timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY *disp);
void _al_android_touch_input_handle_end(int id, double timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY *disp);
void _al_android_touch_input_handle_move(int id, double timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY *disp);
void _al_android_touch_input_handle_cancel(int id, double timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY *disp);
void _al_android_keyboard_handle_event(ALLEGRO_DISPLAY *display, int scancode, bool down);

void _al_android_create_surface(JNIEnv *env, bool post);
void _al_android_destroy_surface(JNIEnv *env, jobject obj, bool post);

ALLEGRO_BITMAP *_al_android_load_image_f(ALLEGRO_FILE *fh, int flags);
ALLEGRO_BITMAP *_al_android_load_image(const char *filename, int flags);

jobject _al_android_activity_object();
jclass _al_android_apk_stream_class(void);

void _al_android_generate_joystick_event(float x, float y, float z);

GLint _al_android_get_curr_fbo(void);
void _al_android_set_curr_fbo(GLint fbo);
bool _al_android_is_os_2_1(void);
void _al_android_thread_created(void);
void _al_android_thread_ended(void);

void _al_android_set_jnienv(JNIEnv *jnienv);
JNIEnv *_al_android_get_jnienv(void);
bool _al_android_is_paused(void);

#define JNI_FUNC_PASTER(ret, cls, name, params, x) \
	JNIEXPORT ret JNICALL Java_ ## x ## _ ## cls ## _ ## name params
#define JNI_FUNC_EVALUATOR(ret, cls, name, params, x) \
	JNI_FUNC_PASTER(ret, cls, name, params, x)
#define JNI_FUNC(ret, cls, name, params) \
	JNI_FUNC_EVALUATOR(ret, cls, name, params, ALLEGRO_CFG_ANDROID_APP_NAME)

#endif /* ALLEGRO_AINTERN_ANDROID_H */

/* vim: set sts=3 sw=3 et: */
