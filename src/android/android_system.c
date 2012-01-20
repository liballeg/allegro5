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
#include "allegro5/platform/aintunix.h"
#include "allegro5/internal/aintern_android.h"
#include "allegro5/internal/aintern_tls.h"
#include "allegro5/platform/aintandroid.h"
#include "allegro5/platform/alandroid.h"
#include "allegro5/threads.h"

#include <dlfcn.h>
#include <jni.h>

ALLEGRO_DEBUG_CHANNEL("android")

struct system_data_t {
   JNIEnv *env;
   jobject activity_object;
   
   ALLEGRO_SYSTEM_ANDROID *system;
   ALLEGRO_MUTEX *mutex;
   ALLEGRO_COND *cond;
   ALLEGRO_THREAD *trampoline;
   bool trampoline_running;
   JNIEnv *main_env;
   
   ALLEGRO_USTR *lib_dir;
   ALLEGRO_USTR *app_name;
   ALLEGRO_USTR *resources_dir;
   ALLEGRO_USTR *data_dir;
	ALLEGRO_USTR *apk_path;
	
   void *user_lib;
   int (*user_main)();
   
   int orientation;
};

static struct system_data_t system_data = { NULL, 0, NULL, NULL, NULL, NULL, false, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0 };
static JavaVM* javavm;

/* define here, so we have access to system_data */
JNIEnv *_jni_getEnv()
{
   return system_data.main_env;
}

jobject _al_android_activity_object()
{
   return system_data.activity_object;
}

int _al_android_get_orientation()
{
   return system_data.orientation;
}

static void finish_activity(JNIEnv *env);

void *android_app_trampoline(ALLEGRO_THREAD *thr, void *arg)
{
   const char *argv[2] = { al_cstr(system_data.app_name), NULL };
   (void)thr; (void)arg;
   
   // setup main_env
   JavaVMAttachArgs attach_args = { JNI_VERSION_1_4, "main_trampoline", NULL };
   jint attach_ret = (*javavm)->AttachCurrentThread(javavm, &system_data.main_env, &attach_args);
   if(attach_ret != 0) {
      ALLEGRO_ERROR("failed to attach trampoline to jvm >:(");
      return NULL;
   }
   
   // chdir(al_cstr(system_data.data_dir));
   
   ALLEGRO_DEBUG("signaling running");
   
   al_lock_mutex(system_data.mutex);
   system_data.trampoline_running = true;
   al_broadcast_cond(system_data.cond);
   al_unlock_mutex(system_data.mutex);
   
   ALLEGRO_DEBUG("entering app's main function");
   
   int ret = (system_data.user_main)(1, (char **)argv);
   if(ret != 0) {
      ALLEGRO_DEBUG("app's main returned failure?");
   }
   
   /* I don't think android calls our atexit() stuff since we're in a shared lib
      so make sure al_uninstall_system is called */
   ALLEGRO_DEBUG("call exit funcs");
   al_uninstall_system();
   
   ALLEGRO_DEBUG("calling finish_activity");
   finish_activity(system_data.main_env);
   ALLEGRO_DEBUG("returning/exit-thread");
   
   jint detach_ret = (*javavm)->DetachCurrentThread(javavm);
   if(detach_ret != 0 ) {
      ALLEGRO_ERROR("failed to detach current thread");
   }
   
   return NULL;
}

/* called by JNI/Java */
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
   (void)reserved;
   javavm = vm;
   return JNI_VERSION_1_4;
}

JNIEXPORT bool Java_org_liballeg_app_AllegroActivity_nativeOnCreate(JNIEnv *env, jobject obj)
{
   ALLEGRO_PATH *lib_path = NULL;
   ALLEGRO_USTR *lib_fname = NULL;
   const char *full_path = NULL;
   ALLEGRO_SYSTEM_ANDROID *na_sys = NULL;
   
   // we're already initialized, we REALLY don't want to run all the stuff below again.
   if(system_data.system) {
      return true;
   }
   
   //_al_pthreads_tls_init();
   
   pthread_t self = pthread_self();
   ALLEGRO_DEBUG("pthread_self:%p", (void*)self);
   ALLEGRO_DEBUG("nativeOnCreate begin");
   
   memset(&system_data, 0, sizeof(system_data));
   
   ALLEGRO_DEBUG("grab activity global refs");
   system_data.env             = env;
   system_data.activity_object = (*env)->NewGlobalRef(env, obj);
   
   ALLEGRO_DEBUG("create mutex and cond objects");
   system_data.mutex = al_create_mutex();
   system_data.cond  = al_create_cond();

   ALLEGRO_DEBUG("get lib_dir, app_name, and data_dir");
   system_data.lib_dir  = _jni_callStringMethod(env, system_data.activity_object, "getLibraryDir", "()Ljava/lang/String;");
   system_data.app_name = _jni_callStringMethod(env, system_data.activity_object, "getAppName", "()Ljava/lang/String;");
   system_data.resources_dir = _jni_callStringMethod(env, system_data.activity_object, "getResourcesDir", "()Ljava/lang/String;");
	system_data.data_dir = _jni_callStringMethod(env, system_data.activity_object, "getPubDataDir", "()Ljava/lang/String;");
   system_data.apk_path = _jni_callStringMethod(env, system_data.activity_object, "getApkPath", "()Ljava/lang/String;");
	ALLEGRO_DEBUG("got lib_dir: %s, app_name: %s, resources_dir: %s, data_dir: %s, apk_path: %s", al_cstr(system_data.lib_dir), al_cstr(system_data.app_name), al_cstr(system_data.resources_dir), al_cstr(system_data.data_dir), al_cstr(system_data.apk_path));
   
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
   
   ALLEGRO_DEBUG("strdup app_name");
   lib_fname = al_ustr_dup(system_data.app_name);
   al_ustr_insert_cstr(lib_fname, 0, "lib");
   al_ustr_append_cstr(lib_fname, ".so");
   
   lib_path = al_create_path_for_directory(al_cstr(system_data.lib_dir));
   al_set_path_filename(lib_path, al_cstr(lib_fname));
   
   full_path = al_path_cstr(lib_path, ALLEGRO_NATIVE_PATH_SEP);

   ALLEGRO_DEBUG("load user lib: %s", full_path);
   system_data.user_lib = dlopen(full_path, RTLD_LAZY|RTLD_GLOBAL);
   if(!system_data.user_lib) {
      ALLEGRO_ERROR("failed to load user app: '%s': %s", full_path, dlerror());
      return false;
   }
   
   ALLEGRO_DEBUG("grab user main");
   system_data.user_main = dlsym(system_data.user_lib, "main");
   if(!system_data.user_main) {
      ALLEGRO_ERROR("failed to locate main entry point in user app '%s': %s", full_path, dlerror());
      dlclose(system_data.user_lib);
      return false;
   }
   
   ALLEGRO_DEBUG("creating trampoline for app thread");
   system_data.trampoline = al_create_thread(android_app_trampoline, NULL);
   al_start_thread(system_data.trampoline);
   
   ALLEGRO_DEBUG("waiting for app trampoline to signal running");
   al_lock_mutex(system_data.mutex);
   while(!system_data.trampoline_running) {
      al_wait_cond(system_data.cond, system_data.mutex);
   }
   al_unlock_mutex(system_data.mutex);
   
   ALLEGRO_DEBUG("setup done. returning to dalkvik.");
   
   return true;
}


JNIEXPORT void JNICALL Java_org_liballeg_app_AllegroActivity_nativeOnPause(JNIEnv *env, jobject obj)
{
   ALLEGRO_SYSTEM *sys = &system_data.system->system;
   ALLEGRO_DISPLAY *d = NULL;
   ALLEGRO_EVENT event;

   (void)env; (void)obj;
   
   ALLEGRO_DEBUG("pause activity\n");
   
   /* no display, just skip */
   if(!_al_vector_size(&sys->displays)) {
      ALLEGRO_DEBUG("no display, not sending SWITCH_OUT event");
      return;
   }
   
   d = *(ALLEGRO_DISPLAY**)_al_vector_ref(&sys->displays, 0);
   ASSERT(d != NULL);
   
   ALLEGRO_DEBUG("locking display event source: %p %p", d, &d->es);
   
   _al_event_source_lock(&d->es);
   
   if(_al_event_source_needs_to_generate_event(&d->es)) {
      ALLEGRO_DEBUG("emit event");
      event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_OUT;
      event.display.timestamp = al_current_time();
      _al_event_source_emit_event(&d->es, &event);
   }
   
   ALLEGRO_DEBUG("unlocking display event source");
   _al_event_source_unlock(&d->es);
}

JNIEXPORT void JNICALL Java_org_liballeg_app_AllegroActivity_nativeOnResume(JNIEnv *env, jobject obj)
{
   ALLEGRO_SYSTEM *sys = &system_data.system->system;
   ALLEGRO_DISPLAY *d = NULL;
   
   (void)obj;
   
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
   
   //if(!((ALLEGRO_DISPLAY_ANDROID*)d)->surface_object) {
   //   ALLEGRO_DEBUG("display is not fully initialized yet, skip re-init and display switch in");
   //   return;
   //}
   
   if(!((ALLEGRO_DISPLAY_ANDROID*)d)->created) {
      _al_android_create_surface(env, true); // request android create our surface
   }
   
   // ??
}

JNIEXPORT void JNICALL Java_org_liballeg_app_AllegroActivity_nativeOnDestroy(JNIEnv *env, jobject obj)
{
   (void)obj;
   
   ALLEGRO_DEBUG("destroy activity");
   if(!system_data.user_lib) {
      ALLEGRO_DEBUG("user lib not loaded.");
      return;
   }
   
   system_data.user_main = NULL;
   if(dlclose(system_data.user_lib) != 0) {
      ALLEGRO_ERROR("failed to unload user lib: %s", dlerror());
      return;
   }
   
   (*env)->DeleteGlobalRef(env, system_data.activity_object);
   
   free(system_data.system);
   
   memset(&system_data, 0, sizeof(system_data));
}

JNIEXPORT void JNICALL Java_org_liballeg_app_AllegroActivity_nativeOnAccel(JNIEnv *env, jobject obj, jint id, jfloat x, jfloat y, jfloat z)
{
   (void)env; (void)obj; (void)id; (void)x; (void)y; (void)z;
   //ALLEGRO_DEBUG("got some accelerometer data!");
}

JNIEXPORT void JNICALL Java_org_liballeg_app_AllegroActivity_nativeOnOrientationChange(JNIEnv *env, jobject obj, int orientation, bool init)
{
   ALLEGRO_SYSTEM *sys = &system_data.system->system;
   ALLEGRO_DISPLAY *d = NULL;
   ALLEGRO_EVENT event;

   (void)env; (void)obj;
   
   ALLEGRO_DEBUG("got orientation change!");
   
   system_data.orientation = orientation;
      
   if(!init) {
         
      /* no display, just skip */
      if(!_al_vector_size(&sys->displays)) {
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

JNIEXPORT void JNICALL Java_org_liballeg_app_AllegroActivity_nativeCreateDisplay(JNIEnv *env, jobject obj)
{
   (void)obj;
   ALLEGRO_DEBUG("nativeCreateDisplay begin");
   
   _al_android_create_surface(env, false);
   
   ALLEGRO_DEBUG("nativeCreateDisplay end");
}


static void finish_activity(JNIEnv *env)
{
   ALLEGRO_DEBUG("pre post");
   _jni_callVoidMethod(env, system_data.activity_object, "postFinish");
   ALLEGRO_DEBUG("post post");
}

static ALLEGRO_SYSTEM *android_initialize(int flags)
{
   (void)flags;
   
   ALLEGRO_DEBUG("init dummy");
   return &system_data.system->system;
}

static int android_get_num_video_adapters(void)
{
   return 1;
}

static void android_shutdown_system()
{
   ALLEGRO_SYSTEM *s = al_get_system_driver();
  /* Close all open displays. */
   while (_al_vector_size(&s->displays) > 0) {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&s->displays, 0);
      ALLEGRO_DISPLAY *d = *dptr;
      _al_destroy_display_bitmaps(d);
      al_destroy_display(d);
   }
   _al_vector_free(&s->displays);  
}

static ALLEGRO_SYSTEM_INTERFACE *android_vt;

ALLEGRO_SYSTEM_INTERFACE *_al_system_android_interface()
{
   if(android_vt)
      return android_vt;
   
   android_vt = al_malloc(sizeof *android_vt);
   memset(android_vt, 0, sizeof *android_vt);
   
   android_vt->initialize = android_initialize;
   android_vt->get_display_driver = _al_get_android_display_driver;
   android_vt->get_keyboard_driver = _al_get_android_keyboard_driver;
   //android_vt->get_mouse_driver = _al_get_android_na_mouse_driver;
   android_vt->get_touch_input_driver = _al_get_android_touch_input_driver;
   //android_vt->get_joystick_driver = _al_get_android_na_joystick_driver;
   android_vt->get_num_video_adapters = android_get_num_video_adapters;
   //android_vt->get_monitor_info = _al_get_android_na_montior_info;
   android_vt->get_path = _al_android_get_path;
   android_vt->shutdown_system = android_shutdown_system;
   
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

ALLEGRO_BITMAP *_al_android_load_image_f(ALLEGRO_FILE *fh, int flags)
{
	jobject byte_buffer;
	jobject jbitmap;
	void *buffer = 0;
	int buffer_len = al_fsize(fh);
	ALLEGRO_BITMAP *bitmap = NULL;
   int bitmap_w = 0, bitmap_h = 0;
   ALLEGRO_STATE state;
   
   ALLEGRO_DEBUG("load image begin");
	/* we could implement a Java IO Stream class that call's allegro fshook functions
	 * which would get rid of this first data copy on our end, but likely it doesn't save
	 * the memory allocation at all */
   /* looking at the skia image library that android uses for image loading,
    * it at least loads png incrementally, so it will save this initiall buffer allocation */
   
   
	buffer = al_malloc(buffer_len+10);
	if(!buffer)
		return NULL;
	
   ALLEGRO_DEBUG("malloced %i bytes ok", buffer_len);
   
	if(al_fread(fh, buffer, buffer_len) != buffer_len) {
		al_free(buffer);
		return NULL;
	}
	
	ALLEGRO_DEBUG("fread ok");
   
	byte_buffer = (*system_data.env)->NewDirectByteBuffer(system_data.env, buffer, buffer_len);
	_jni_checkException(system_data.env);
	
   ALLEGRO_DEBUG("NewDirectByteBuffer ok");
   
	jbitmap = _jni_callObjectMethod(system_data.env, system_data.activity_object, "decodeBitmap", "(Ljava/nio/ByteBuffer;)Landroid/graphics/Bitmap;");
	
   ALLEGRO_DEBUG("decodeBitmap ok");
   
   (*system_data.env)->DeleteLocalRef(system_data.env, byte_buffer);
   _jni_checkException(system_data.env);
   
   ALLEGRO_DEBUG("DeleteLocalRef ok");
   
	free(buffer);
	
	
	buffer_len = _jni_callIntMethod(system_data.env, jbitmap, "getByteCount");
	
   ALLEGRO_DEBUG("getByteCount ok");
   
	buffer = al_malloc(buffer_len);
	if(!buffer)
		return NULL;
	
   ALLEGRO_DEBUG("malloc %i bytes ok", buffer_len);
   
	byte_buffer = (*system_data.env)->NewDirectByteBuffer(system_data.env, buffer, buffer_len);
	_jni_checkException(system_data.env);
	
   ALLEGRO_DEBUG("NewDirectByteBuffer 2 ok");
   
	_jni_callVoidMethodV(system_data.env, jbitmap, "copyPixelsToBuffer", "(Ljava/nio/ByteBuffer;)V", byte_buffer);

   ALLEGRO_DEBUG("copyPixelsToBuffer ok");
   
   // tell java we don't need the byte_buffer object
   (*system_data.env)->DeleteLocalRef(system_data.env, byte_buffer);
   _jni_checkException(system_data.env);

   ALLEGRO_DEBUG("DeleteLocalRef 2 ok");
   
   // tell java we're done with the bitmap as well
   (*system_data.env)->DeleteLocalRef(system_data.env, jbitmap);
   _jni_checkException(system_data.env);
   
   ALLEGRO_DEBUG("DeleteLocalRef 3 ok");
   
   bitmap_w = _jni_callIntMethod(system_data.env, jbitmap, "getWidth");
   bitmap_h = _jni_callIntMethod(system_data.env, jbitmap, "getHeight");

   al_store_state(&state, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ARGB_8888);
   bitmap = al_create_bitmap(bitmap_w, bitmap_h);
   al_restore_state(&state);
   if(!bitmap) {
      al_free(buffer);
      return NULL;
   }

   ALLEGRO_DEBUG("create bitmap ok");
   
   _al_ogl_upload_bitmap_memory(bitmap, ALLEGRO_PIXEL_FORMAT_ARGB_8888, buffer);

   ALLEGRO_DEBUG("upload bitmap memory ok");
   
   al_free(buffer);
   
   ALLEGRO_DEBUG("load image end");
   return bitmap;
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

/* vim: set sts=3 sw=3 et: */
