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
 *      Android Java/JNI system driver
 *
 *      By Thomas Fjellstrom.
 *
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_android.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/internal/aintern_android.h"
#include "allegro5/internal/aintern_tls.h"
#include "allegro5/platform/aintandroid.h"
#include "allegro5/platform/alandroid.h"
#include "allegro5/threads.h"

#include <dlfcn.h>
#include <jni.h>

#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_opengl.h"

ALLEGRO_DEBUG_CHANNEL("android")

struct system_data_t {
   JNIEnv *env;
   jobject activity_object;
   jclass input_stream_class;
   jclass illegal_argument_exception_class;
   jclass apk_stream_class;
   jclass image_loader_class;
   jclass clipboard_class;
   jclass apk_fs_class;

   ALLEGRO_SYSTEM_ANDROID *system;
   ALLEGRO_MUTEX *mutex;
   ALLEGRO_COND *cond;
   ALLEGRO_THREAD *trampoline;
   bool trampoline_running;

   ALLEGRO_USTR *user_lib_name;
   ALLEGRO_USTR *resources_dir;
   ALLEGRO_USTR *data_dir;
   ALLEGRO_USTR *apk_path;
   ALLEGRO_USTR *model;
   ALLEGRO_USTR *manufacturer;

   void *user_lib;
   int (*user_main)(int argc, char **argv);

   int orientation;

   bool paused;
};

static struct system_data_t system_data;
static JavaVM* javavm;
static JNIEnv *main_env;

static const char *_real_al_android_get_os_version(JNIEnv *env);

bool _al_android_is_paused(void)
{
   return system_data.paused;
}

int _al_android_get_display_orientation(void)
{
   return system_data.orientation;
}

jclass _al_android_input_stream_class(void)
{
   return system_data.input_stream_class;
}

jclass _al_android_apk_stream_class(void)
{
   return system_data.apk_stream_class;
}

jclass _al_android_image_loader_class(void)
{
   return system_data.image_loader_class;
}

jclass _al_android_clipboard_class(void)
{
   return system_data.clipboard_class;
}

jobject _al_android_activity_object()
{
   return system_data.activity_object;
}

jclass _al_android_apk_fs_class(void)
{
   return system_data.apk_fs_class;
}

static void finish_activity(JNIEnv *env);

static bool already_cleaned_up = false;

/* NOTE: don't put any ALLEGRO_DEBUG in here! */
static void android_cleanup(bool uninstall_system)
{
   if (already_cleaned_up) {
      return;
   }

   already_cleaned_up = true;

   if (uninstall_system) {
      /* I don't think android calls our atexit() stuff since we're in a shared lib
         so make sure al_uninstall_system is called */
      al_uninstall_system();
   }

   finish_activity(_al_android_get_jnienv());

   (*javavm)->DetachCurrentThread(javavm);
}

static void *android_app_trampoline(ALLEGRO_THREAD *thr, void *arg)
{
   const int argc = 1;
   const char *argv[2] = {system_data.user_lib, NULL};
   int ret;

   (void)thr;
   (void)arg;

   ALLEGRO_DEBUG("signaling running");

   al_lock_mutex(system_data.mutex);
   system_data.trampoline_running = true;
   al_broadcast_cond(system_data.cond);
   al_unlock_mutex(system_data.mutex);

   ALLEGRO_DEBUG("entering main function %p", system_data.user_main);

   ret = (system_data.user_main)(argc, (char **)argv);

   /* Can we do anything with this exit code? */
   ALLEGRO_DEBUG("returned from main function, exit code = %d", ret);

   /* NOTE: don't put any ALLEGRO_DEBUG in here after running main! */

   android_cleanup(true);

   return NULL;
}

/* called by JNI/Java */
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
   (void)reserved;
   javavm = vm;
   return JNI_VERSION_1_4;
}

JNI_FUNC(bool, AllegroActivity, nativeOnCreate, (JNIEnv *env, jobject obj))
{
   ALLEGRO_SYSTEM_ANDROID *na_sys = NULL;
   jclass iae;
   jclass aisc;
   jclass asc;

   ALLEGRO_DEBUG("entered nativeOnCreate");

   // we're already initialized, we REALLY don't want to run all the stuff below again.
   if(system_data.system) {
      return true;
   }

   pthread_t self = pthread_self();
   ALLEGRO_DEBUG("pthread_self:%p", (void*)self);
   ALLEGRO_DEBUG("nativeOnCreate begin");

   memset(&system_data, 0, sizeof(system_data));

   ALLEGRO_DEBUG("grab activity global refs");
   system_data.env             = env;
   system_data.activity_object = (*env)->NewGlobalRef(env, obj);

   iae = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
   system_data.illegal_argument_exception_class = (*env)->NewGlobalRef(env, iae);

   aisc = (*env)->FindClass(env, ALLEGRO_ANDROID_PACKAGE_NAME_SLASH "/AllegroInputStream");
   system_data.input_stream_class = (*env)->NewGlobalRef(env, aisc);

   asc = (*env)->FindClass(env, ALLEGRO_ANDROID_PACKAGE_NAME_SLASH "/AllegroAPKStream");
   system_data.apk_stream_class = (*env)->NewGlobalRef(env, asc);

   asc = (*env)->FindClass(env, ALLEGRO_ANDROID_PACKAGE_NAME_SLASH "/ImageLoader");
   system_data.image_loader_class = (*env)->NewGlobalRef(env, asc);

   asc = (*env)->FindClass(env, ALLEGRO_ANDROID_PACKAGE_NAME_SLASH "/Clipboard");
   system_data.clipboard_class = (*env)->NewGlobalRef(env, asc);

   asc = (*env)->FindClass(env, ALLEGRO_ANDROID_PACKAGE_NAME_SLASH "/AllegroAPKList");
   system_data.apk_fs_class = (*env)->NewGlobalRef(env, asc);

   ALLEGRO_DEBUG("create mutex and cond objects");
   system_data.mutex = al_create_mutex();
   system_data.cond  = al_create_cond();

   ALLEGRO_DEBUG("get directories");
   system_data.user_lib_name = _jni_callStringMethod(env, system_data.activity_object, "getUserLibName", "()Ljava/lang/String;");
   system_data.resources_dir = _jni_callStringMethod(env, system_data.activity_object, "getResourcesDir", "()Ljava/lang/String;");
   system_data.data_dir = _jni_callStringMethod(env, system_data.activity_object, "getPubDataDir", "()Ljava/lang/String;");
   system_data.apk_path = _jni_callStringMethod(env, system_data.activity_object, "getApkPath", "()Ljava/lang/String;");
   system_data.model = _jni_callStringMethod(env, system_data.activity_object, "getModel", "()Ljava/lang/String;");
   system_data.manufacturer = _jni_callStringMethod(env, system_data.activity_object, "getManufacturer", "()Ljava/lang/String;");
   ALLEGRO_DEBUG("resources_dir: %s", al_cstr(system_data.resources_dir));
   ALLEGRO_DEBUG("data_dir: %s", al_cstr(system_data.data_dir));
   ALLEGRO_DEBUG("apk_path: %s", al_cstr(system_data.apk_path));
   ALLEGRO_DEBUG("model: %s", al_cstr(system_data.model));
   ALLEGRO_DEBUG("manufacturer: %s", al_cstr(system_data.manufacturer));

   ALLEGRO_DEBUG("creating ALLEGRO_SYSTEM_ANDROID struct");
   na_sys = system_data.system = (ALLEGRO_SYSTEM_ANDROID*)al_malloc(sizeof *na_sys);
   memset(na_sys, 0, sizeof *na_sys);

   ALLEGRO_DEBUG("get system pointer");
   ALLEGRO_SYSTEM *sys = &na_sys->system;
   ALLEGRO_DEBUG("get system interface");
   sys->vt = _al_system_android_interface();

   ALLEGRO_DEBUG("init display vector");
   _al_vector_init(&sys->displays, sizeof(ALLEGRO_DISPLAY_ANDROID *));

   ALLEGRO_DEBUG("init time");
   _al_unix_init_time();

   const char *user_lib_name = al_cstr(system_data.user_lib_name);
   ALLEGRO_DEBUG("load user lib: %s", user_lib_name);
   system_data.user_lib = dlopen(user_lib_name, RTLD_LAZY|RTLD_GLOBAL);
   if (!system_data.user_lib) {
      ALLEGRO_ERROR("failed to load user lib: %s", user_lib_name);
      ALLEGRO_ERROR("%s", dlerror());
      return false;
   }

   system_data.user_main = dlsym(system_data.user_lib, "main");
   if (!system_data.user_main) {
      ALLEGRO_ERROR("failed to locate symbol main: %s", dlerror());
      dlclose(system_data.user_lib);
      return false;
   }
   ALLEGRO_DEBUG("main function address: %p\n", system_data.user_main);

   ALLEGRO_DEBUG("creating trampoline for app thread");
   system_data.trampoline = al_create_thread(android_app_trampoline, NULL);
   al_start_thread(system_data.trampoline);

   ALLEGRO_DEBUG("waiting for app trampoline to signal running");
   al_lock_mutex(system_data.mutex);
   while(!system_data.trampoline_running) {
      al_wait_cond(system_data.cond, system_data.mutex);
   }
   al_unlock_mutex(system_data.mutex);

   ALLEGRO_DEBUG("setup done. returning to dalvik.");

   return true;
}


JNI_FUNC(void, AllegroActivity, nativeOnPause, (JNIEnv *env, jobject obj))
{
   (void)env;
   (void)obj;

   ALLEGRO_DEBUG("pause activity\n");

   system_data.paused = true;

   ALLEGRO_SYSTEM *sys = (void *)al_get_system_driver();

   if (!system_data.system || !sys) {
      ALLEGRO_DEBUG("no system driver");
      return;
   }

   if (!_al_vector_size(&sys->displays)) {
      ALLEGRO_DEBUG("no display, not sending SWITCH_OUT event");
      return;
   }

   ALLEGRO_DISPLAY *display = *(ALLEGRO_DISPLAY**)_al_vector_ref(&sys->displays, 0);

   if (display) {
      ALLEGRO_EVENT event;
      _al_event_source_lock(&display->es);

      if(_al_event_source_needs_to_generate_event(&display->es)) {
         event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_OUT;
         event.display.timestamp = al_current_time();
         _al_event_source_emit_event(&display->es, &event);
      }
      _al_event_source_unlock(&display->es);
   }
}

JNI_FUNC(void, AllegroActivity, nativeOnResume, (JNIEnv *env, jobject obj))
{
   ALLEGRO_SYSTEM *sys = &system_data.system->system;
   ALLEGRO_DISPLAY *d = NULL;

   (void)obj;

   system_data.paused = false;

   ALLEGRO_DEBUG("resume activity");

   if(!system_data.system || !sys) {
      ALLEGRO_DEBUG("no system driver");
      return;
   }

   if(!_al_vector_size(&sys->displays)) {
      ALLEGRO_DEBUG("no display, not sending SWITCH_IN event");
      return;
   }

   d = *(ALLEGRO_DISPLAY**)_al_vector_ref(&sys->displays, 0);
   ALLEGRO_DEBUG("got display: %p", d);

   if(!((ALLEGRO_DISPLAY_ANDROID*)d)->created) {
      _al_android_create_surface(env, true); // request android create our surface
   }

   ALLEGRO_DISPLAY *display = *(ALLEGRO_DISPLAY**)_al_vector_ref(&sys->displays, 0);

   if (display) {
      ALLEGRO_EVENT event;
      _al_event_source_lock(&display->es);

      if(_al_event_source_needs_to_generate_event(&display->es)) {
         event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_IN;
         event.display.timestamp = al_current_time();
         _al_event_source_emit_event(&display->es, &event);
      }
      _al_event_source_unlock(&display->es);
   }
}

/* NOTE: don't put any ALLEGRO_DEBUG in here! */
JNI_FUNC(void, AllegroActivity, nativeOnDestroy, (JNIEnv *env, jobject obj))
{
   (void)obj;

   /* onDestroy can be called before main returns, for example if you start
    * a new activity, your Allegro game will get onDestroy when it returns.
    * At that point there's nothing you can do and any code you execute will
    * crash, so this attempts to handle that more gracefully. Calling
    * android_cleanup() eventually leads back here anyway. We ask android_cleanup
    * not to call al_uninstall_system because GPU access causes a crash at
    * this point (cleaning up bitmaps/displays etc.) The trampoline will exit
    * eventually too so we guard against android_cleanup() being called twice.
    */
   bool main_returned = _jni_callBooleanMethodV(
      env,
      system_data.activity_object,
      "getMainReturned",
      "()Z"
   );

   if (!main_returned) {
      exit(0);
   }

   if(!system_data.user_lib) {
      return;
   }

   system_data.user_main = NULL;
   if(dlclose(system_data.user_lib) != 0) {
      return;
   }

   (*env)->DeleteGlobalRef(env, system_data.activity_object);
   (*env)->DeleteGlobalRef(env, system_data.illegal_argument_exception_class);
   (*env)->DeleteGlobalRef(env, system_data.input_stream_class);

   free(system_data.system);

   memset(&system_data, 0, sizeof(system_data));
}

JNI_FUNC(void, AllegroActivity, nativeOnOrientationChange, (JNIEnv *env, jobject obj, int orientation, bool init))
{
   ALLEGRO_SYSTEM *sys = &system_data.system->system;
   ALLEGRO_DISPLAY *d = NULL;
   ALLEGRO_EVENT event;

   (void)env; (void)obj;

   ALLEGRO_DEBUG("got orientation change!");

   system_data.orientation = orientation;

   if (!init) {

      /* no display, just skip */
      if (!_al_vector_size(&sys->displays)) {
         ALLEGRO_DEBUG("no display, not sending orientation change event");
         return;
      }

      d = *(ALLEGRO_DISPLAY**)_al_vector_ref(&sys->displays, 0);
      ASSERT(d != NULL);

      ALLEGRO_DEBUG("locking display event source: %p %p", d, &d->es);

      _al_event_source_lock(&d->es);

      if(_al_event_source_needs_to_generate_event(&d->es)) {
         ALLEGRO_DEBUG("emit event");
         event.display.type = ALLEGRO_EVENT_DISPLAY_ORIENTATION;
         event.display.timestamp = al_current_time();
         event.display.orientation = orientation;
         _al_event_source_emit_event(&d->es, &event);
      }

      ALLEGRO_DEBUG("unlocking display event source");
      _al_event_source_unlock(&d->es);

   }
}

JNI_FUNC(void, AllegroActivity, nativeSendJoystickConfigurationEvent, (JNIEnv *env, jobject obj))
{
   (void)env;
   (void)obj;

   if (!al_is_joystick_installed()) {
      return;
   }

   ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();
   _al_event_source_lock(es);
   ALLEGRO_EVENT event;
   event.type = ALLEGRO_EVENT_JOYSTICK_CONFIGURATION;
   _al_event_source_emit_event(es, &event);
   _al_event_source_unlock(es);
}

/* NOTE: don't put any ALLEGRO_DEBUG in here! */
static void finish_activity(JNIEnv *env)
{
   _jni_callVoidMethod(env, system_data.activity_object, "postFinish");
}

static ALLEGRO_SYSTEM *android_initialize(int flags)
{
   (void)flags;

   ALLEGRO_DEBUG("android_initialize");

   /* This was stored before user main ran, to make it easy and accessible
    * the same way for all threads, we set it in tls
    */
   _al_android_set_jnienv(main_env);

   return &system_data.system->system;
}

static ALLEGRO_JOYSTICK_DRIVER *android_get_joystick_driver(void)
{
   return &_al_android_joystick_driver;
}

static int android_get_num_video_adapters(void)
{
   return 1;
}

static bool android_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO *info)
{
   if (adapter >= android_get_num_video_adapters())
      return false;

   JNIEnv * env = (JNIEnv *)_al_android_get_jnienv();
   jobject rect = _jni_callObjectMethod(env, _al_android_activity_object(), "getDisplaySize", "()Landroid/graphics/Rect;");

   info->x1 = 0;
   info->y1 = 0;
   info->x2 = _jni_callIntMethod(env, rect, "width");
   info->y2 = _jni_callIntMethod(env, rect, "height");

   ALLEGRO_DEBUG("Monitor Info: %d:%d", info->x2, info->y2);

   _jni_callv(env, DeleteLocalRef, rect);

   return true;
}

static void android_shutdown_system(void)
{
   ALLEGRO_SYSTEM *s = al_get_system_driver();
  /* Close all open displays. */
   while (_al_vector_size(&s->displays) > 0) {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&s->displays, 0);
      ALLEGRO_DISPLAY *d = *dptr;
      al_destroy_display(d);
   }
   _al_vector_free(&s->displays);
}

static bool android_inhibit_screensaver(bool inhibit)
{
   return _jni_callBooleanMethodV(_al_android_get_jnienv(), system_data.activity_object, "inhibitScreenLock", "(Z)Z", inhibit);
}

static ALLEGRO_SYSTEM_INTERFACE *android_vt;

ALLEGRO_SYSTEM_INTERFACE *_al_system_android_interface()
{
   if(android_vt)
      return android_vt;

   android_vt = al_malloc(sizeof *android_vt);
   memset(android_vt, 0, sizeof *android_vt);

   android_vt->id = ALLEGRO_SYSTEM_ID_ANDROID;
   android_vt->initialize = android_initialize;
   android_vt->get_display_driver = _al_get_android_display_driver;
   android_vt->get_keyboard_driver = _al_get_android_keyboard_driver;
   android_vt->get_mouse_driver = _al_get_android_mouse_driver;
   android_vt->get_touch_input_driver = _al_get_android_touch_input_driver;
   android_vt->get_joystick_driver = android_get_joystick_driver;
   android_vt->get_num_video_adapters = android_get_num_video_adapters;
   android_vt->get_monitor_info = android_get_monitor_info;
   android_vt->get_path = _al_android_get_path;
   android_vt->shutdown_system = android_shutdown_system;
   android_vt->inhibit_screensaver = android_inhibit_screensaver;
   android_vt->get_time = _al_unix_get_time;
   android_vt->rest = _al_unix_rest;
   android_vt->init_timeout = _al_unix_init_timeout;

   return android_vt;
}

ALLEGRO_PATH *_al_android_get_path(int id)
{
   ALLEGRO_PATH *path = NULL;

   switch(id) {
      case ALLEGRO_RESOURCES_PATH:
         /* path to bundle's files */
         path = al_create_path_for_directory(al_cstr(system_data.resources_dir));
         break;

      case ALLEGRO_TEMP_PATH:
      case ALLEGRO_USER_DATA_PATH:
      case ALLEGRO_USER_HOME_PATH:
      case ALLEGRO_USER_SETTINGS_PATH:
      case ALLEGRO_USER_DOCUMENTS_PATH:
         /* path to sdcard */
         path = al_create_path_for_directory(al_cstr(system_data.data_dir));
         break;

      case ALLEGRO_EXENAME_PATH:
         /* bundle path + bundle name */
         // FIXME!
         path = al_create_path(al_cstr(system_data.apk_path));
         break;

		default:
			path = al_create_path_for_directory("/DANGER/WILL/ROBINSON");
			break;
   }

   return path;
}

static const char *_real_al_android_get_os_version(JNIEnv *env)
{
   static char buffer[25];
   ALLEGRO_USTR *s = _jni_callStringMethod(env, system_data.activity_object, "getOsVersion", "()Ljava/lang/String;");
   strncpy(buffer, al_cstr(s), 25);
   al_ustr_free(s);
   return buffer;
}

/* Function: al_android_get_os_version
 */
const char *al_android_get_os_version(void)
{
   return _real_al_android_get_os_version(_al_android_get_jnienv());
}

void _al_android_thread_created(void)
{
   JNIEnv *env;
   JavaVMAttachArgs attach_args = { JNI_VERSION_1_4, "trampoline", NULL };
   (*javavm)->AttachCurrentThread(javavm, &env, &attach_args);
   /* This function runs once before al_init, so before TLS is initialized
    * so we save the environment and set it later in that case.
    */
   ALLEGRO_SYSTEM *s = al_get_system_driver();
   if (s && s->installed) {
      _al_android_set_jnienv(env);
   }
   else {
      main_env = env;
   }
}

void _al_android_thread_ended(void)
{
   (*javavm)->DetachCurrentThread(javavm);
}

void _al_android_set_capture_volume_keys(ALLEGRO_DISPLAY *display, bool onoff)
{
   ALLEGRO_DISPLAY_ANDROID *d = (ALLEGRO_DISPLAY_ANDROID *)display;
   _jni_callVoidMethodV(_al_android_get_jnienv(), d->surface_object, "setCaptureVolumeKeys", "(Z)V", onoff);
}

/* register system interfaces */

void _al_register_system_interfaces(void)
{
   ALLEGRO_SYSTEM_INTERFACE **add;

   /* add the native activity driver */
   add = _al_vector_alloc_back(&_al_system_interfaces);
   *add = _al_system_android_interface();

   /* TODO: add the non native activity driver */
}

/* Function: al_android_get_jni_env
 */
JNIEnv *al_android_get_jni_env(void)
{
   return _al_android_get_jnienv();
}

/* Function: al_android_get_activity
 */
jobject al_android_get_activity(void)
{
   return _al_android_activity_object();
}

/* vim: set sts=3 sw=3 et: */
