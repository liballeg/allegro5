#ifndef ALLEGRO_AINTERN_ANDROID_H
#define ALLEGRO_AINTERN_ANDROID_H

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_display.h"

#include <jni.h>

typedef struct ALLEGRO_SYSTEM_ANDROID {
   ALLEGRO_SYSTEM system;
   
   int visuals_count;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS **visuals;    
   
} ALLEGRO_SYSTEM_ANDROID;

typedef struct ALLEGRO_DISPLAY_ANDROID {
   ALLEGRO_DISPLAY display;
   jobject surface_object;
   
   ALLEGRO_COND *cond;
   ALLEGRO_MUTEX *mutex;
   
   bool recreate;
   bool created;
   
   bool resize_acknowledge;

} ALLEGRO_DISPLAY_ANDROID;

ALLEGRO_SYSTEM_INTERFACE *_al_system_android_interface();
/* ALLEGRO_SYSTEM_INTERFACE *_al_system_android_jni_interface(); */

ALLEGRO_DISPLAY_INTERFACE *_al_get_android_display_driver(void);
ALLEGRO_KEYBOARD_DRIVER *_al_get_android_keyboard_driver(void);
ALLEGRO_MOUSE_DRIVER *_al_get_android_mouse_driver(void);
ALLEGRO_TOUCH_INPUT_DRIVER *_al_get_android_touch_input_driver(void);
ALLEGRO_JOYSTICK_DRIVER *_al_get_android_joystick_driver(void);
bool _al_get_android_montior_info(int adapter, ALLEGRO_MONITOR_INFO *info);

JNIEnv *_jni_getEnv();

void _jni_checkException(JNIEnv *env);
jobject _jni_callObjectMethod(JNIEnv *env, jobject object, char *name, char *sig);
ALLEGRO_USTR *_jni_getString(JNIEnv *env, jobject object);
ALLEGRO_USTR *_jni_callStringMethod(JNIEnv *env, jobject obj, char *name, char *sig);
void _jni_callVoidMethod(JNIEnv *env, jobject obj, char *name);
void _jni_callVoidMethodV(JNIEnv *env, jobject obj, char *name, char *sig, ...);
int _jni_callIntMethod(JNIEnv *env, jobject, char *name);
int _jni_callIntMethodV(JNIEnv *env, jobject obj, char *name, char *sig, ...);
bool _jni_callBooleanMethodV(JNIEnv *env, jobject obj, char *name, char *sig, ...);

void _al_android_touch_input_handle_begin(int id, double timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY *disp);
void _al_android_touch_input_handle_end(int id, double timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY *disp);
void _al_android_touch_input_handle_move(int id, double timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY *disp);
void _al_android_touch_input_handle_cancel(int id, double timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY *disp);
void _al_android_keyboard_handle_event(ALLEGRO_DISPLAY *display, int scancode, bool down);

void _al_android_create_surface(JNIEnv *env, bool post);
void _al_android_destroy_surface(JNIEnv *env, jobject obj, bool post);
bool _al_android_init_display(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *display);

ALLEGRO_BITMAP *_al_android_load_image_f(ALLEGRO_FILE *fh, int flags);

jobject _al_android_activity_object();
int _al_android_get_orientation();

#endif /* ALLEGRO_AINTERN_ANDROID_H */

/* vim: set sts=3 sw=3 et: */
