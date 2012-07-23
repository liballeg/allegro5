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
 *      Android Java/JNI display driver
 *
 *      By Thomas Fjellstrom.
 *
 */

#include "allegro5/allegro.h"
#include "allegro5/platform/aintandroid.h"
#include "allegro5/platform/alandroid.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_android.h"

#include <allegro5/allegro_opengl.h>
#include "allegro5/internal/aintern_opengl.h"

#include "EGL/egl.h"

/* Locking the screen causes a pause (good), then we resize (bad)
 * 
 * something isn't handling some onConfigChange events
 * 
 */

ALLEGRO_DEBUG_CHANNEL("display")

static ALLEGRO_DISPLAY_INTERFACE *vt;
static ALLEGRO_EXTRA_DISPLAY_SETTINGS main_thread_display_settings;

static void _al_android_update_visuals(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *d);
static void android_change_display_option(ALLEGRO_DISPLAY *d, int o, int v);
static void _al_android_resize_display(ALLEGRO_DISPLAY_ANDROID *d, int width, int height);
static bool _al_android_init_display(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *display);
void _al_android_clear_current(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *d);
void  _al_android_make_current(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *d);
void _al_android_clear_current(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *d);

JNI_FUNC(void, AllegroSurface, nativeOnCreate, (JNIEnv *env, jobject obj))
{
   ALLEGRO_DEBUG("nativeOnCreate");
   (void)env;
   (void)obj;
   
   ALLEGRO_SYSTEM *system = (void *)al_get_system_driver();
   ASSERT(system != NULL);
   
   ALLEGRO_DEBUG("AllegroSurface_nativeOnCreate");
   
   ALLEGRO_DISPLAY_ANDROID *d = *(ALLEGRO_DISPLAY_ANDROID**)_al_vector_ref(&system->displays, 0);
   ASSERT(d != NULL);

   d->recreate = true;
}

JNI_FUNC(bool, AllegroSurface, nativeOnDestroy, (JNIEnv *env, jobject obj))
{
   ALLEGRO_SYSTEM *sys = (void *)al_get_system_driver();
   ASSERT(system != NULL);

   ALLEGRO_DISPLAY_ANDROID *display = *(ALLEGRO_DISPLAY_ANDROID**)_al_vector_ref(&sys->displays, 0);
   ASSERT(display != NULL);

   ALLEGRO_DISPLAY *d = (ALLEGRO_DISPLAY *)display;
   ALLEGRO_EVENT event;

   ALLEGRO_DEBUG("AllegroSurface_nativeOnDestroy");
   (void)obj;
   (void)env;

   if (!display->created) {
      ALLEGRO_DEBUG("Display creation failed, not sending HALT");
      return false;
   }
   
   display->created = false;
   
   ALLEGRO_DEBUG("locking display event source: %p %p", d, &d->es);
 
   _al_event_source_lock(&d->es);
   
   if(_al_event_source_needs_to_generate_event(&d->es)) {
      event.display.type = ALLEGRO_EVENT_DISPLAY_HALT_DRAWING;
      event.display.timestamp = al_current_time();
      _al_event_source_emit_event(&d->es, &event);
   }
   
   ALLEGRO_DEBUG("unlocking display event source");
   _al_event_source_unlock(&d->es);

   // wait for acknowledge_drawing_halt
   al_lock_mutex(display->mutex);
   al_wait_cond(display->cond, display->mutex);
   al_unlock_mutex(display->mutex);

   ALLEGRO_DEBUG("AllegroSurface_nativeOnDestroy end");
   
   return true;
}

// FIXME: need to loop over the display list looking for the right surface object in the following jni callbacks
JNI_FUNC(void, AllegroSurface, nativeOnChange, (JNIEnv *env, jobject obj, jint format, jint width, jint height))
{
   ALLEGRO_EVENT event;

   (void)env;
   (void)obj;
   (void)format;

   ALLEGRO_DEBUG("on change!");
   ALLEGRO_DEBUG("sys: %p", system);
   
   ALLEGRO_SYSTEM *system = (void *)al_get_system_driver();
   ASSERT(system != NULL);
   ALLEGRO_DISPLAY_ANDROID *d = *(ALLEGRO_DISPLAY_ANDROID**)_al_vector_ref(&system->displays, 0);
   ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY*)d;
   ASSERT(display != NULL);

   if (!d->first_run && !d->recreate) {
      _al_android_resize_display(d, width, height);
      return;
   }
   
   al_lock_mutex(d->mutex);
  
   if (d->first_run || d->recreate) {
      d->surface_object = (*env)->NewGlobalRef(env, obj);
   }

   display->w = width;
   display->h = height;
   
   if (!_al_android_init_display(env, d) && d->first_run) {
      al_broadcast_cond(d->cond);
      al_unlock_mutex(d->mutex);
      return;
   }

   _al_android_clear_current(env, d);

   d->created = true;
   d->recreate = false;
   d->resumed = false;
   
   if (!d->first_run) {
      ALLEGRO_DEBUG("locking display event source: %p", d);
      _al_event_source_lock(&display->es);
   
      ALLEGRO_DEBUG("check generate event");
      if (_al_event_source_needs_to_generate_event(&display->es)) {
         event.display.type = ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING;
         event.display.timestamp = al_current_time();
         _al_event_source_emit_event(&display->es, &event);
      }
      
      ALLEGRO_DEBUG("unlocking display event source");
      _al_event_source_unlock(&display->es);
   }

   if (d->first_run) {
      al_broadcast_cond(d->cond);
      d->first_run = false;
   }
   else {
      ALLEGRO_DEBUG("Waiting for al_acknowledge_drawing_resume");
      while (!d->resumed) {
         al_wait_cond(d->cond, d->mutex);
      }
      ALLEGRO_DEBUG("Got al_acknowledge_drawing_resume");
   }

   al_unlock_mutex(d->mutex);
}

JNI_FUNC(void, AllegroSurface, nativeOnKeyDown, (JNIEnv *env, jobject obj, jint scancode))
{
   (void)env; (void)obj;
   
   ALLEGRO_SYSTEM *system = (void *)al_get_system_driver();
   ASSERT(system != NULL);
   
   ALLEGRO_DISPLAY *display = *(ALLEGRO_DISPLAY**)_al_vector_ref(&system->displays, 0);
   ASSERT(display != NULL);
   
   _al_android_keyboard_handle_event(display, scancode, true);
}

JNI_FUNC(void, AllegroSurface, nativeOnKeyUp, (JNIEnv *env, jobject obj, jint scancode))
{
   (void)env; (void)obj;
   
   ALLEGRO_SYSTEM *system = (void *)al_get_system_driver();
   ASSERT(system != NULL);
   
   ALLEGRO_DISPLAY *display = *(ALLEGRO_DISPLAY**)_al_vector_ref(&system->displays, 0);
   ASSERT(display != NULL);

   _al_android_keyboard_handle_event(display, scancode, false);
}

JNI_FUNC(void, AllegroSurface, nativeOnTouch, (JNIEnv *env, jobject obj, jint id, jint action, jfloat x, jfloat y, jboolean primary))
{
   (void)env; (void)obj;
   
   ALLEGRO_SYSTEM *system = (void *)al_get_system_driver();
   ASSERT(system != NULL);
   
   ALLEGRO_DISPLAY *display = *(ALLEGRO_DISPLAY**)_al_vector_ref(&system->displays, 0);
   ASSERT(display != NULL);

   switch(action) {
      case ALLEGRO_EVENT_TOUCH_BEGIN:
         _al_android_touch_input_handle_begin(id, al_get_time(), x, y, primary, display);
         break;
         
      case ALLEGRO_EVENT_TOUCH_END:
         _al_android_touch_input_handle_end(id, al_get_time(), x, y, primary, display);
         break;
         
      case ALLEGRO_EVENT_TOUCH_MOVE:
         _al_android_touch_input_handle_move(id, al_get_time(), x, y, primary, display);
         break;
         
      case ALLEGRO_EVENT_TOUCH_CANCEL:
         _al_android_touch_input_handle_cancel(id, al_get_time(), x, y, primary, display);
         break;
         
      default:
         ALLEGRO_ERROR("unknown touch action: %i", action);
   }
   
}

void _al_android_create_surface(JNIEnv *env, bool post)
{
   _jni_callVoidMethod(env, _al_android_activity_object(), post ? "postCreateSurface" : "createSurface");
}

void _al_android_destroy_surface(JNIEnv *env, jobject surface, bool post)
{
   (void)surface;
   if (post) {
      _jni_callVoidMethodV(env, _al_android_activity_object(), "postDestroySurface", "(Landroid/view/View;)V");
   }
   else {
      _jni_callVoidMethodV(env, _al_android_activity_object(), "destroySurface", "()V");
   }
}

void  _al_android_make_current(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *d)
{
   _jni_callVoidMethodV(env, d->surface_object, "egl_makeCurrent", "()V");
}

void _al_android_clear_current(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *d)
{
   _jni_callVoidMethodV(env, d->surface_object, "egl_clearCurrent", "()V");
}

void _al_android_setup_opengl_view(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DEBUG("setup opengl view d->w=%d d->h=%d", d->w, d->h);

   al_set_target_backbuffer(d);
   
   glViewport(0, 0, d->w, d->h);

   al_identity_transform(&d->proj_transform);
   al_ortho_transform(&d->proj_transform, 0, d->w, d->h, 0, -1, 1);
   al_set_projection_transform(d, &d->proj_transform);

   al_identity_transform(&d->view_transform);
   al_use_transform(&d->view_transform);
}

static bool _al_android_init_display(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *display)
{
   ALLEGRO_SYSTEM_ANDROID *system = (void *)al_get_system_driver();
   ALLEGRO_DISPLAY *d = (ALLEGRO_DISPLAY *)display;
   int version, ret;
      
   ASSERT(system != NULL);
   ASSERT(display != NULL);

   ALLEGRO_DEBUG("calling egl_Init");
      
   if(!_jni_callBooleanMethodV(env, display->surface_object, "egl_Init", "()Z")) {
      // XXX should probably destroy the AllegroSurface here
      ALLEGRO_ERROR("failed to initialize EGL");
      display->failed = true;
      return false;
   }

   ALLEGRO_DEBUG("updating visuals");
   _al_android_update_visuals(env, display);
   ALLEGRO_DEBUG("done updating visuals");
   
   memcpy(&d->extra_settings, &system->visual, sizeof(ALLEGRO_EXTRA_DISPLAY_SETTINGS));

   if (d->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) {
   	version = 2;
   }
   else {
   	version = 1;
   }

   ALLEGRO_DEBUG("calling egl_createContext");
   if (!(ret = _jni_callIntMethodV(env, display->surface_object, "egl_createContext", "(I)I", version))) {
      // XXX should probably destroy the AllegroSurface here
      ALLEGRO_ERROR("failed to create egl context!");
      display->failed = true;
      return false;
   }

   if (ret == 2 && (d->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE)) {
   	d->flags &= ~ALLEGRO_USE_PROGRAMMABLE_PIPELINE;
   }
   
   ALLEGRO_DEBUG("calling egl_createSurface");
   if (!_jni_callBooleanMethodV(env, display->surface_object, "egl_createSurface", "()Z")) {
      // XXX should probably destroy the AllegroSurface here
      ALLEGRO_ERROR("failed to create egl surface!");
      display->failed = true;
      return false;
   }

   ALLEGRO_DEBUG("initialize ogl extensions");
   _al_ogl_manage_extensions(d);
   _al_ogl_set_extensions(d->ogl_extras->extension_api);

   d->ogl_extras->backbuffer = _al_ogl_create_backbuffer(d);
   
   return true;
}


/* implementation helpers */

static void _al_android_resize_display(ALLEGRO_DISPLAY_ANDROID *d, int width, int height)
{
   ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY *)d;
   bool emitted_event = true;

   ALLEGRO_DEBUG("display resize");
   
   d->resize_acknowledge = false;
   d->resize_acknowledge2 = false;
  
   ALLEGRO_DEBUG("locking mutex");
   al_lock_mutex(d->mutex);
   ALLEGRO_DEBUG("done locking mutex");
   
   ALLEGRO_DEBUG("locking display event source: %p", d);
   _al_event_source_lock(&display->es);

   ALLEGRO_DEBUG("check generate event");
   if(_al_event_source_needs_to_generate_event(&display->es)) {
      ALLEGRO_EVENT event;
      event.display.type = ALLEGRO_EVENT_DISPLAY_RESIZE;
      event.display.timestamp = al_current_time();
      event.display.x = 0; // FIXME: probably not correct
      event.display.y = 0;
      event.display.width = width;
      event.display.height = height;
      event.display.orientation = _al_android_get_display_orientation();
      _al_event_source_emit_event(&display->es, &event);
   }
   else {
   	emitted_event = false;
   }
   
   ALLEGRO_DEBUG("unlocking display event source");
   _al_event_source_unlock(&display->es);

   if (emitted_event) {
      ALLEGRO_DEBUG("waiting for display resize acknowledge");
      while (!d->resize_acknowledge) {
         ALLEGRO_DEBUG("calling al_wait_cond");
         al_wait_cond(d->cond, d->mutex);
      }
      al_unlock_mutex(d->mutex);
      ALLEGRO_DEBUG("done waiting for display resize acknowledge");
   }

   display->w = width;
   display->h = height;
   
   ALLEGRO_DEBUG("resize backbuffer");
   _al_ogl_resize_backbuffer(display->ogl_extras->backbuffer, width, height);

   if (emitted_event) {
      d->resize_acknowledge2 = true;
      al_broadcast_cond(d->cond);
   }
   
   al_unlock_mutex(d->mutex);

   ALLEGRO_DEBUG("done");
}

static void _al_android_update_visuals(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *d)
{
   const int RENDERABLE_TYPE = 0x3040;
   const int OPENGLES2_BIT = 4;

   ALLEGRO_SYSTEM_ANDROID *system = (void *)al_get_system_driver();
   ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY *)d;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref = &main_thread_display_settings;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds = &system->visual;
   
   eds->settings[ALLEGRO_RENDER_METHOD] = 1;
   eds->settings[ALLEGRO_COMPATIBLE_DISPLAY] = 1;
   eds->settings[ALLEGRO_SWAP_METHOD] = 2;
   eds->settings[ALLEGRO_VSYNC] = 1;

   /* FIXME: after setting our values and calling eglChooseConfig, read the values back into system->visual */
   if ((ref->required & ((int64_t)1 << ALLEGRO_COLOR_SIZE)) || (ref->suggested & ((int64_t)1 << ALLEGRO_COLOR_SIZE))) {
      if (ref->settings[ALLEGRO_COLOR_SIZE] == 16) {
         _jni_callVoidMethodV(env, d->surface_object, "egl_setConfigAttrib", "(II)V", ALLEGRO_RED_SIZE, 5);
         _jni_callVoidMethodV(env, d->surface_object, "egl_setConfigAttrib", "(II)V", ALLEGRO_GREEN_SIZE, 6);
         _jni_callVoidMethodV(env, d->surface_object, "egl_setConfigAttrib", "(II)V", ALLEGRO_BLUE_SIZE, 5);
      }
      else { // only other thing supported is 32 bit
         _jni_callVoidMethodV(env, d->surface_object, "egl_setConfigAttrib", "(II)V", ALLEGRO_RED_SIZE, 8);
         _jni_callVoidMethodV(env, d->surface_object, "egl_setConfigAttrib", "(II)V", ALLEGRO_GREEN_SIZE, 8);
         _jni_callVoidMethodV(env, d->surface_object, "egl_setConfigAttrib", "(II)V", ALLEGRO_BLUE_SIZE, 8);
      }
   }

   /* There is no equivalent ot COLOR_SIZE in EGL except setting the individual components */

   #define MAYBE_SET(v)                                                       \
      do {                                                                    \
         if ((ref->required & ((int64_t)1 << v)) ||                           \
             (ref->suggested & ((int64_t)1 << v)))                            \
         {                                                                    \
            ALLEGRO_DEBUG("calling egl_setConfigAttrib(%d, %d)",              \
               v, ref->settings[v]);                                          \
            _jni_callVoidMethodV(env, d->surface_object,                      \
               "egl_setConfigAttrib", "(II)V", v, ref->settings[v]);          \
         }                                                                    \
      } while (0)

   MAYBE_SET(ALLEGRO_RED_SIZE);
   MAYBE_SET(ALLEGRO_GREEN_SIZE);
   MAYBE_SET(ALLEGRO_BLUE_SIZE);
   MAYBE_SET(ALLEGRO_ALPHA_SIZE);
   MAYBE_SET(ALLEGRO_DEPTH_SIZE);
   MAYBE_SET(ALLEGRO_STENCIL_SIZE);
   MAYBE_SET(ALLEGRO_SAMPLE_BUFFERS);
   MAYBE_SET(ALLEGRO_SAMPLES);

   #undef MAYBE_SET

   if (display->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) {
      _jni_callVoidMethodV(env, d->surface_object, "egl_setConfigAttrib", "(II)V", RENDERABLE_TYPE, OPENGLES2_BIT);
   }
}

/* driver implementation hooks */
ALLEGRO_DISPLAY_INTERFACE *_al_get_android_display_driver(void);

static ALLEGRO_DISPLAY *android_create_display(int w, int h)
{
   ALLEGRO_DEBUG("begin");

   int flags = al_get_new_display_flags();
#ifdef ALLEGRO_NO_GLES2
   if (flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) {
      return NULL;
   }
#endif
   
   ALLEGRO_DISPLAY_ANDROID *d = al_malloc(sizeof *d);
   ALLEGRO_DISPLAY *display = (void *)d;
   
   ALLEGRO_OGL_EXTRAS *ogl = al_malloc(sizeof *ogl);
   memset(d, 0, sizeof *d);
   memset(ogl, 0, sizeof *ogl);
   display->ogl_extras = ogl;
   display->vt = _al_get_android_display_driver();
   display->w = w;
   display->h = h;
   display->flags = flags;
   
   _al_event_source_init(&display->es);
   
   /* Java thread needs this but it's thread local. For now we assume display is created and set up in main thread */
   memcpy(&main_thread_display_settings, _al_get_new_display_settings(), sizeof(main_thread_display_settings));

   d->mutex = al_create_mutex();
   d->cond = al_create_cond();
   d->recreate = true;
   d->first_run = true;
   d->failed = false;
   
   ALLEGRO_SYSTEM *system = (void *)al_get_system_driver();
   ASSERT(system != NULL);
   
   ALLEGRO_DISPLAY_ANDROID **add;
   add = _al_vector_alloc_back(&system->displays);
   *add = d;
   
   al_lock_mutex(d->mutex);

   // post create surface request and wait
   _al_android_create_surface(_al_android_get_jnienv(), true);
   
   // wait for sizing to happen
   ALLEGRO_DEBUG("waiting for surface onChange");
   while (!d->created && !d->failed) {
      al_wait_cond(d->cond, d->mutex);
   }
   al_unlock_mutex(d->mutex);
   ALLEGRO_DEBUG("done waiting for surface onChange");

   if (d->failed) {
      ALLEGRO_DEBUG("Display creation failed");
      _al_vector_find_and_delete(&system->displays, &d);
      al_free(ogl);
      al_free(d);
      return NULL;
   }
   
   display->flags |= ALLEGRO_OPENGL;

   ALLEGRO_DEBUG("display: %p %ix%i", display, display->w, display->h);

   _al_android_clear_current(_al_android_get_jnienv(), d);
   _al_android_make_current(_al_android_get_jnienv(), d);
   
   _al_android_setup_opengl_view(display);

   /* Don't need to repeat what this does */
   android_change_display_option(display, ALLEGRO_SUPPORTED_ORIENTATIONS, al_get_new_display_option(ALLEGRO_SUPPORTED_ORIENTATIONS, NULL));

   ALLEGRO_DEBUG("end");
   return display;
}

static void android_destroy_display(ALLEGRO_DISPLAY *dpy)
{
   ALLEGRO_DISPLAY_ANDROID *d = (ALLEGRO_DISPLAY_ANDROID*)dpy;
   
   _al_android_clear_current(_al_android_get_jnienv(), d);
   
   al_lock_mutex(d->mutex);
  
   _al_android_destroy_surface(_al_android_get_jnienv(), d, true);

   /* I don't think we can use a condition for this, because there are two possibilities
    * of how a nativeOnDestroy/surfaceDestroyed callback can be called. One is from here,
    * manually, and one happens automatically and is out of our hands.
    */
   while (d->created) {
   	al_rest(0.001);
   }
   
   _al_event_source_free(&dpy->es);
   
   // XXX: this causes a crash, no idea why as of yet
   //ALLEGRO_DEBUG("destroy backbuffer");
   //_al_ogl_destroy_backbuffer(al_get_backbuffer(dpy));
   
   ALLEGRO_DEBUG("destroy mutex");
   al_destroy_mutex(d->mutex);
   
   ALLEGRO_DEBUG("destroy cond");
   al_destroy_cond(d->cond);
   
   ALLEGRO_DEBUG("free ogl_extras");
   free(dpy->ogl_extras);
   
   ALLEGRO_DEBUG("free display");
   free(d);
   
   ALLEGRO_DEBUG("remove display from system list");
   ALLEGRO_SYSTEM *s = al_get_system_driver();
   _al_vector_find_and_delete(&s->displays, &d);
   
   ALLEGRO_DEBUG("done");
}

static bool android_set_current_display(ALLEGRO_DISPLAY *dpy)
{
   if (al_get_current_display() == dpy)
      return true;
   
   _al_android_clear_current(_al_android_get_jnienv(), (ALLEGRO_DISPLAY_ANDROID*)al_get_current_display());
   
   ALLEGRO_DEBUG("make current %p", dpy);
   if (dpy) _al_android_make_current(_al_android_get_jnienv(), (ALLEGRO_DISPLAY_ANDROID*)dpy);

   _al_ogl_update_render_state(dpy);

   return true;
}

static void android_unset_current_display(ALLEGRO_DISPLAY *dpy)
{
   ALLEGRO_DEBUG("unset current %p", dpy);
   _al_android_clear_current(_al_android_get_jnienv(), (ALLEGRO_DISPLAY_ANDROID*)dpy);
}

static void android_flip_display(ALLEGRO_DISPLAY *dpy)
{
   _jni_callVoidMethod(_al_android_get_jnienv(), ((ALLEGRO_DISPLAY_ANDROID*)dpy)->surface_object, "egl_SwapBuffers");

   /* Backup bitmaps created without ALLEGRO_NO_PRESERVE_TEXTURE that are dirty, to system memory */
   _al_opengl_backup_dirty_bitmaps(dpy, true);
}

static void android_update_display_region(ALLEGRO_DISPLAY *dpy, int x, int y, int width, int height)
{
   (void)dpy; (void)x; (void)y; (void)width; (void)height;
}


static bool android_acknowledge_resize(ALLEGRO_DISPLAY *dpy)
{
   ALLEGRO_DISPLAY_ANDROID *d = (ALLEGRO_DISPLAY_ANDROID*)dpy;
   
   ALLEGRO_DEBUG("android_acknowledge_resize");
   
   ALLEGRO_DEBUG("clear current context");
   _al_android_clear_current(_al_android_get_jnienv(), d);

   ALLEGRO_DEBUG("locking mutex");
   al_lock_mutex(d->mutex);
   d->resize_acknowledge = true;
   al_broadcast_cond(d->cond);
   ALLEGRO_DEBUG("broadcasted condvar");

   ALLEGRO_DEBUG("waiting for display resize acknowledge 2");
   while (!d->resize_acknowledge2)  {
      ALLEGRO_DEBUG("calling al_wait_cond");
      al_wait_cond(d->cond, d->mutex);
   }
   al_unlock_mutex(d->mutex);
   ALLEGRO_DEBUG("done waiting for display resize acknowledge 2");

   ALLEGRO_DEBUG("acquire context");
   _al_android_make_current(_al_android_get_jnienv(), d);
   
   ALLEGRO_DEBUG("done");
   return true;
}

static int android_get_orientation(ALLEGRO_DISPLAY *dpy)
{
   (void)dpy;
   return _al_android_get_display_orientation();
}

static bool android_is_compatible_bitmap(ALLEGRO_DISPLAY *dpy, ALLEGRO_BITMAP *bmp)
{
   (void)dpy;
   (void)bmp;
   return true;
}

static bool android_resize_display(ALLEGRO_DISPLAY *dpy, int w, int h)
{
   (void)dpy; (void)w; (void)h;
   return false;
}

static void android_set_icon(ALLEGRO_DISPLAY *dpy, ALLEGRO_BITMAP *bmp)
{
   (void)dpy; (void)bmp;
}

static void android_set_window_title(ALLEGRO_DISPLAY *dpy, const char *title)
{
   (void)dpy; (void)title;
}

static void android_set_window_position(ALLEGRO_DISPLAY *dpy, int x, int y)
{
   (void)dpy; (void)x; (void)y;
}

static void android_get_window_position(ALLEGRO_DISPLAY *dpy, int *x, int *y)
{
   (void)dpy;
   *x = *y = 0;
}

static bool android_set_display_flag(ALLEGRO_DISPLAY *dpy, int flag, bool onoff)
{
   (void)dpy; (void)flag; (void)onoff;
   return false;
}

static bool android_wait_for_vsync(ALLEGRO_DISPLAY *dpy)
{
   (void)dpy;
   return false;
}

static bool android_set_mouse_cursor(ALLEGRO_DISPLAY *dpy, ALLEGRO_MOUSE_CURSOR *cursor)
{
   (void)dpy; (void)cursor;
   return false;
}

static bool android_set_system_mouse_cursor(ALLEGRO_DISPLAY *dpy, ALLEGRO_SYSTEM_MOUSE_CURSOR id)
{
   (void)dpy; (void)id;
   return false;
}

static bool android_show_mouse_cursor(ALLEGRO_DISPLAY *dpy)
{
   (void)dpy;
   return false;
}

static bool android_hide_mouse_cursor(ALLEGRO_DISPLAY *dpy)
{
   (void)dpy;
   return false;
}

static void android_acknowledge_drawing_halt(ALLEGRO_DISPLAY *dpy)
{
   int i;
   ALLEGRO_DEBUG("android_acknowledge_drawing_halt");

   for (i = 0; i < (int)dpy->bitmaps._size; i++) {
      ALLEGRO_BITMAP **bptr = (ALLEGRO_BITMAP **)_al_vector_ref(&dpy->bitmaps, i);
      ALLEGRO_BITMAP *bmp = *bptr;
      if (!(bmp->flags & ALLEGRO_MEMORY_BITMAP) && !bmp->parent) {
	    if (!(bmp->flags & ALLEGRO_NO_PRESERVE_TEXTURE)) {
               ALLEGRO_BITMAP_EXTRA_OPENGL *extra = bmp->extra;
               al_remove_opengl_fbo(bmp);
               glDeleteTextures(1, &extra->texture);
	       extra->texture = 0;
	    }
      }
   }

   ALLEGRO_DISPLAY_ANDROID *d = (ALLEGRO_DISPLAY_ANDROID *)dpy;
   
   _al_android_clear_current(_al_android_get_jnienv(), d);

   al_broadcast_cond(d->cond);

   ALLEGRO_DEBUG("acknowledged drawing halt");
}

void android_broadcast_resume(ALLEGRO_DISPLAY_ANDROID *d)
{
   ALLEGRO_DEBUG("Broadcasting resume");
   al_lock_mutex(d->mutex);
   d->resumed = true;
   al_broadcast_cond(d->cond);
   al_unlock_mutex(d->mutex);
   ALLEGRO_DEBUG("done broadcasting resume");
}

static void android_acknowledge_drawing_resume(ALLEGRO_DISPLAY *dpy, void (*user_reload)(void))
{
   int i, size;

   ALLEGRO_DEBUG("begin");

   ALLEGRO_DEBUG("acknowledge_drawing_resume");
   
   ALLEGRO_DISPLAY_ANDROID *d = (ALLEGRO_DISPLAY_ANDROID *)dpy;
   
   _al_android_clear_current(_al_android_get_jnienv(), d);
   _al_android_make_current(_al_android_get_jnienv(), d);
   
   ALLEGRO_DEBUG("made current");
   
   _al_android_setup_opengl_view(dpy);

   if (user_reload) {
   	(*user_reload)();
   }

   // Restore bitmaps
   // have to get this because new bitmaps could be created below
   size = (int)dpy->bitmaps._size;
   for (i = 0; i < size; i++) {
      ALLEGRO_BITMAP **bptr = (ALLEGRO_BITMAP **)_al_vector_ref(&dpy->bitmaps, i);
      ALLEGRO_BITMAP *bmp = *bptr;
      if (!bmp->parent && !(bmp->flags & ALLEGRO_MEMORY_BITMAP) &&
            !(bmp->flags & ALLEGRO_NO_PRESERVE_TEXTURE)) {
         _al_ogl_upload_bitmap_memory(bmp, bmp->format, bmp->memory);
         bmp->dirty = false;
      }
   }

   android_broadcast_resume(d);

   ALLEGRO_DEBUG("acknowledge_drawing_resume end");
}

static void android_change_display_option(ALLEGRO_DISPLAY *d, int o, int v)
{
   (void)d;
   if (o == ALLEGRO_SUPPORTED_ORIENTATIONS) {
      int orientation = _jni_callIntMethodV(_al_android_get_jnienv(),
         _al_android_activity_object(), "getAndroidOrientation", "(I)I", v);
      _jni_callVoidMethodV(_al_android_get_jnienv(), _al_android_activity_object(),
         "setRequestedOrientation", "(I)V", orientation);
   }
}

/* obtain a reference to the android driver */
ALLEGRO_DISPLAY_INTERFACE *_al_get_android_display_driver(void)
{
   if (vt)
      return vt;
   
   vt = al_malloc(sizeof *vt);
   memset(vt, 0, sizeof *vt);
   
   vt->create_display = android_create_display;
   vt->destroy_display = android_destroy_display;
   vt->set_current_display = android_set_current_display;
   vt->unset_current_display = android_unset_current_display;
   vt->flip_display = android_flip_display;
   vt->update_display_region = android_update_display_region;
   vt->acknowledge_resize = android_acknowledge_resize;
   vt->create_bitmap = _al_ogl_create_bitmap;
   vt->get_backbuffer = _al_ogl_get_backbuffer;
   vt->set_target_bitmap = _al_ogl_set_target_bitmap;
   
   vt->get_orientation = android_get_orientation;

   vt->is_compatible_bitmap = android_is_compatible_bitmap;
   vt->resize_display = android_resize_display;
   vt->set_icon = android_set_icon;
   vt->set_window_title = android_set_window_title;
   vt->set_window_position = android_set_window_position;
   vt->get_window_position = android_get_window_position;
   vt->set_display_flag = android_set_display_flag;
   vt->wait_for_vsync = android_wait_for_vsync;

   vt->set_mouse_cursor = android_set_mouse_cursor;
   vt->set_system_mouse_cursor = android_set_system_mouse_cursor;
   vt->show_mouse_cursor = android_show_mouse_cursor;
   vt->hide_mouse_cursor = android_hide_mouse_cursor;

   vt->acknowledge_drawing_halt = android_acknowledge_drawing_halt;
   vt->acknowledge_drawing_resume = android_acknowledge_drawing_resume;

   vt->change_display_option = android_change_display_option;

   vt->update_render_state = _al_ogl_update_render_state;
   
   _al_ogl_add_drawing_functions(vt);
    
   return vt;
}
