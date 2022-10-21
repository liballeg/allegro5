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
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_android.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_shader.h"
#include "allegro5/internal/aintern_pixels.h"

#include "EGL/egl.h"

ALLEGRO_DEBUG_CHANNEL("display")

static ALLEGRO_DISPLAY_INTERFACE *vt;
static ALLEGRO_EXTRA_DISPLAY_SETTINGS main_thread_display_settings;

static int select_best_visual_and_update(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *d);
static void android_set_display_option(ALLEGRO_DISPLAY *d, int o, int v);
static void _al_android_resize_display(ALLEGRO_DISPLAY_ANDROID *d, int width, int height);
static bool _al_android_init_display(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *display);
void _al_android_clear_current(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *d);
void  _al_android_make_current(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *d);

JNI_FUNC(void, AllegroSurface, nativeOnCreate, (JNIEnv *env, jobject obj))
{
   ALLEGRO_DEBUG("nativeOnCreate");
   (void)env;
   (void)obj;

   ALLEGRO_SYSTEM *system = (void *)al_get_system_driver();
   ASSERT(system != NULL);

   ALLEGRO_DEBUG("AllegroSurface_nativeOnCreate");

   ALLEGRO_DISPLAY_ANDROID **dptr = _al_vector_ref(&system->displays, 0);
   ALLEGRO_DISPLAY_ANDROID *d = *dptr;
   ASSERT(d != NULL);

   d->recreate = true;
}

JNI_FUNC(bool, AllegroSurface, nativeOnDestroy, (JNIEnv *env, jobject obj))
{
   ALLEGRO_SYSTEM *sys = (void *)al_get_system_driver();
   ASSERT(sys != NULL);

   ALLEGRO_DISPLAY_ANDROID **dptr = _al_vector_ref(&sys->displays, 0);
   ALLEGRO_DISPLAY_ANDROID *display = *dptr;
   ASSERT(display != NULL);

   ALLEGRO_DISPLAY *d = (ALLEGRO_DISPLAY *)display;

   ALLEGRO_DEBUG("AllegroSurface_nativeOnDestroy");
   (void)obj;
   (void)env;

   if (!display->created) {
      ALLEGRO_DEBUG("Display creation failed, not sending HALT");
      return false;
   }

   display->created = false;

   if (display->is_destroy_display) {
      return true;
   }

   ALLEGRO_DEBUG("locking display event source: %p %p", d, &d->es);

   _al_event_source_lock(&d->es);

   if (_al_event_source_needs_to_generate_event(&d->es)) {
      ALLEGRO_EVENT event;
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

// FIXME: need to loop over the display list looking for the right surface
// object in the following jni callbacks
JNI_FUNC(void, AllegroSurface, nativeOnChange, (JNIEnv *env, jobject obj,
   jint format, jint width, jint height))
{
   ALLEGRO_SYSTEM *system = (void *)al_get_system_driver();
   ASSERT(system != NULL);

   ALLEGRO_DISPLAY_ANDROID **dptr = _al_vector_ref(&system->displays, 0);
   ALLEGRO_DISPLAY_ANDROID *d = *dptr;
   ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY*)d;
   ASSERT(display != NULL);

   ALLEGRO_DEBUG("on change %dx%d!", width, height);
   (void)format;

   if (!d->first_run && !d->recreate) {
      _al_android_resize_display(d, width, height);
      return;
   }

   al_lock_mutex(d->mutex);

   if (d->first_run || d->recreate) {
      d->surface_object = (*env)->NewGlobalRef(env, obj);
      if (display->flags & ALLEGRO_FULLSCREEN_WINDOW) {
         display->w = width;
         display->h = height;
      }
   }

   bool ret = _al_android_init_display(env, d);
   if (!ret && d->first_run) {
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
         ALLEGRO_EVENT event;
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

   /* Send a resize event to signify that the display may have changed sizes. */
   if (!d->first_run) {
      _al_android_resize_display(d, width, height);
   }
}

JNI_FUNC(void, AllegroSurface, nativeOnJoystickAxis, (JNIEnv *env, jobject obj,
   jint index, jint stick, jint axis, jfloat value))
{
   (void)env;
   (void)obj;
   _al_android_generate_joystick_axis_event(index+1, stick, axis, value);
}

JNI_FUNC(void, AllegroSurface, nativeOnJoystickButton, (JNIEnv *env, jobject obj,
   jint index, jint button, jboolean down))
{
   (void)env;
   (void)obj;
   _al_android_generate_joystick_button_event(index+1, button, down);
}

void _al_android_create_surface(JNIEnv *env, bool post)
{
   if (post) {
      _jni_callVoidMethod(env, _al_android_activity_object(),
         "postCreateSurface");
   }
   else {
      _jni_callVoidMethod(env, _al_android_activity_object(),
         "createSurface");
   }
}

void _al_android_destroy_surface(JNIEnv *env, jobject surface, bool post)
{
   (void)surface;
   if (post) {
      _jni_callVoidMethodV(env, _al_android_activity_object(),
         "postDestroySurface", "()V");
   }
   else {
      _jni_callVoidMethodV(env, _al_android_activity_object(),
         "destroySurface", "()V");
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

static bool _al_android_init_display(JNIEnv *env,
   ALLEGRO_DISPLAY_ANDROID *display)
{
   ALLEGRO_SYSTEM_ANDROID *system = (void *)al_get_system_driver();
   ALLEGRO_DISPLAY *d = (ALLEGRO_DISPLAY *)display;
   int config_index;
   bool programmable_pipeline;
   int ret;

   ASSERT(system != NULL);
   ASSERT(display != NULL);

   ALLEGRO_DEBUG("calling egl_Init");

   if (!_jni_callBooleanMethodV(env, display->surface_object,
         "egl_Init", "()Z"))
   {
      // XXX should probably destroy the AllegroSurface here
      ALLEGRO_ERROR("failed to initialize EGL");
      display->failed = true;
      return false;
   }

   config_index = select_best_visual_and_update(env, display);
   if (config_index < 0) {
      // XXX should probably destroy the AllegroSurface here
      ALLEGRO_ERROR("Failed to choose config.\n");
      display->failed = true;
      return false;
   }

   programmable_pipeline = (d->flags & ALLEGRO_PROGRAMMABLE_PIPELINE);

   ALLEGRO_DEBUG("calling egl_createContext");
   ret = _jni_callIntMethodV(env, display->surface_object,
      "egl_createContext", "(IZ)I", config_index, programmable_pipeline);
   if (!ret) {
      // XXX should probably destroy the AllegroSurface here
      ALLEGRO_ERROR("failed to create egl context!");
      display->failed = true;
      return false;
   }

   ALLEGRO_DEBUG("calling egl_createSurface");
   if (!_jni_callBooleanMethodV(env, display->surface_object,
         "egl_createSurface", "()Z"))
   {
      // XXX should probably destroy the AllegroSurface here
      ALLEGRO_ERROR("failed to create egl surface!");
      display->failed = true;
      return false;
   }

   ALLEGRO_DEBUG("initialize ogl extensions");
   _al_ogl_manage_extensions(d);
   _al_ogl_set_extensions(d->ogl_extras->extension_api);

   _al_ogl_setup_gl(d);

   return true;
}


/* implementation helpers */

static void _al_android_resize_display(ALLEGRO_DISPLAY_ANDROID *d,
   int width, int height)
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
   _al_ogl_setup_gl(display);

   if (emitted_event) {
      d->resize_acknowledge2 = true;
      al_broadcast_cond(d->cond);
   }

   al_unlock_mutex(d->mutex);

   ALLEGRO_DEBUG("done");
}

static void call_initRequiredAttribs(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *d)
{
   _jni_callVoidMethodV(env, d->surface_object,
      "egl_initRequiredAttribs", "()V");
}

static void call_setRequiredAttrib(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *d,
   int attr, int value)
{
   _jni_callVoidMethodV(env, d->surface_object,
      "egl_setRequiredAttrib", "(II)V", attr, value);
}

static int call_chooseConfig(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *d)
{
   bool pp = (d->display.flags & ALLEGRO_PROGRAMMABLE_PIPELINE);
   return _jni_callIntMethodV(env, d->surface_object,
      "egl_chooseConfig", "(Z)I", pp);
}

static void call_getConfigAttribs(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *d,
   int configIndex, ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds)
{
   jintArray jArray;
   jint *ptr;
   int i;

   jArray = (*env)->NewIntArray(env, ALLEGRO_DISPLAY_OPTIONS_COUNT);
   if (jArray == NULL) {
      ALLEGRO_ERROR("NewIntArray failed\n");
      return;
   }

   _jni_callVoidMethodV(env, d->surface_object,
      "egl_getConfigAttribs", "(I[I)V", configIndex, jArray);

   ptr = (*env)->GetIntArrayElements(env, jArray, 0);
   for (i = 0; i < ALLEGRO_DISPLAY_OPTIONS_COUNT; i++) {
      eds->settings[i] = ptr[i];
   }

   (*env)->ReleaseIntArrayElements(env, jArray, ptr, 0);
}

static void set_required_attribs(JNIEnv *env, ALLEGRO_DISPLAY_ANDROID *d,
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref)
{
   #define MAYBE_SET(v)                                                       \
      do {                                                                    \
         bool required = (ref->required & ((int64_t)1 << (v)));               \
         if (required) {                                                      \
            call_setRequiredAttrib(env, d, (v), ref->settings[(v)]);          \
         }                                                                    \
      } while (0)

   MAYBE_SET(ALLEGRO_RED_SIZE);
   MAYBE_SET(ALLEGRO_GREEN_SIZE);
   MAYBE_SET(ALLEGRO_BLUE_SIZE);
   MAYBE_SET(ALLEGRO_ALPHA_SIZE);
   MAYBE_SET(ALLEGRO_COLOR_SIZE);
   MAYBE_SET(ALLEGRO_DEPTH_SIZE);
   MAYBE_SET(ALLEGRO_STENCIL_SIZE);
   MAYBE_SET(ALLEGRO_SAMPLE_BUFFERS);
   MAYBE_SET(ALLEGRO_SAMPLES);

   #undef MAYBE_SET
}

static int select_best_visual_and_update(JNIEnv *env,
   ALLEGRO_DISPLAY_ANDROID *d)
{
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref = &main_thread_display_settings;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS **eds = NULL;
   int num_configs;
   int chosen_index;
   int i;

   ALLEGRO_DEBUG("Setting required display settings.\n");
   call_initRequiredAttribs(env, d);
   set_required_attribs(env, d, ref);

   ALLEGRO_DEBUG("Calling eglChooseConfig.\n");
   num_configs = call_chooseConfig(env, d);
   if (num_configs < 1) {
      return -1;
   }

   eds = al_calloc(num_configs, sizeof(*eds));

   for (i = 0; i < num_configs; i++) {
      eds[i] = al_calloc(1, sizeof(*eds[i]));

      call_getConfigAttribs(env, d, i, eds[i]);

      eds[i]->settings[ALLEGRO_RENDER_METHOD] = 1;
      eds[i]->settings[ALLEGRO_COMPATIBLE_DISPLAY] = 1;
      eds[i]->settings[ALLEGRO_SWAP_METHOD] = 2;
      eds[i]->settings[ALLEGRO_VSYNC] = 1;

      eds[i]->index = i;
      eds[i]->score = _al_score_display_settings(eds[i], ref);
   }

   ALLEGRO_DEBUG("Sorting configs.\n");
   qsort(eds, num_configs, sizeof(*eds), _al_display_settings_sorter);

   chosen_index = eds[0]->index;
   ALLEGRO_INFO("Chose config %i\n", chosen_index);

   d->display.extra_settings = *eds[0];

   /* Don't need this any more. */
   for (i = 0; i < num_configs; i++) {
      al_free(eds[i]);
   }
   al_free(eds);

   return chosen_index;
}

/* driver implementation hooks */

static bool android_set_display_flag(ALLEGRO_DISPLAY *dpy, int flag, bool onoff)
{
   (void)dpy; (void)flag; (void)onoff;

   if (flag == ALLEGRO_FRAMELESS) {
      _jni_callVoidMethodV(_al_android_get_jnienv(),
         _al_android_activity_object(), "setAllegroFrameless", "(Z)V", onoff);
   }
   
   return false;
}

static ALLEGRO_DISPLAY *android_create_display(int w, int h)
{
   ALLEGRO_DEBUG("begin");

   int flags = al_get_new_display_flags();
#ifndef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
   if (flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
      ALLEGRO_WARN("Programmable pipeline support not built.\n");
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

   display->flags |= ALLEGRO_OPENGL;
#ifdef ALLEGRO_CFG_OPENGLES2
   display->flags |= ALLEGRO_PROGRAMMABLE_PIPELINE;
#endif
#ifdef ALLEGRO_CFG_OPENGLES
   display->flags |= ALLEGRO_OPENGL_ES_PROFILE;
#endif

   _al_event_source_init(&display->es);

   /* Java thread needs this but it's thread local.
    * For now we assume display is created and set up in main thread.
    */
   main_thread_display_settings = *_al_get_new_display_settings();

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

   ALLEGRO_DEBUG("display: %p %ix%i", display, display->w, display->h);

   _al_android_clear_current(_al_android_get_jnienv(), d);
   _al_android_make_current(_al_android_get_jnienv(), d);

   /* Don't need to repeat what this does */
   android_set_display_option(display, ALLEGRO_SUPPORTED_ORIENTATIONS,
      al_get_new_display_option(ALLEGRO_SUPPORTED_ORIENTATIONS, NULL));

   /* Fill in opengl version */
   const int v = d->display.ogl_extras->ogl_info.version;
   d->display.extra_settings.settings[ALLEGRO_OPENGL_MAJOR_VERSION] = (v >> 24) & 0xFF;
   d->display.extra_settings.settings[ALLEGRO_OPENGL_MINOR_VERSION] = (v >> 16) & 0xFF;

   if (flags & ALLEGRO_FRAMELESS) {
      android_set_display_flag(display, ALLEGRO_FRAMELESS, true);
   }

   ALLEGRO_DEBUG("end");
   return display;
}

static void android_destroy_display(ALLEGRO_DISPLAY *dpy)
{
   ALLEGRO_DISPLAY_ANDROID *d = (ALLEGRO_DISPLAY_ANDROID*)dpy;

   ALLEGRO_DEBUG("clear current");

   if (!d->created) {
      // if nativeOnDestroy was called (the app switched out) and
      // then the display is destroyed before the app switches back
      // in, there is no EGL context to destroy.
      goto already_deleted;
   }

   _al_android_clear_current(_al_android_get_jnienv(), d);

   al_lock_mutex(d->mutex);

   d->is_destroy_display = true;

   _al_android_destroy_surface(_al_android_get_jnienv(), d, true);

   /* I don't think we can use a condition for this, because there are two
    * possibilities of how a nativeOnDestroy/surfaceDestroyed callback can be
    * called. One is from here, manually, and one happens automatically and is
    * out of our hands.
    */
   while (d->created) {
   	al_rest(0.001);
   }

   _al_event_source_free(&dpy->es);

already_deleted:
   // XXX: this causes a crash, no idea why as of yet
   //ALLEGRO_DEBUG("destroy backbuffer");
   //_al_ogl_destroy_backbuffer(al_get_backbuffer(dpy));

   ALLEGRO_DEBUG("destroy mutex");
   al_destroy_mutex(d->mutex);

   ALLEGRO_DEBUG("destroy cond");
   al_destroy_cond(d->cond);

   ALLEGRO_DEBUG("free ogl_extras");
   al_free(dpy->ogl_extras);

   ALLEGRO_DEBUG("remove display from system list");
   ALLEGRO_SYSTEM *s = al_get_system_driver();
   _al_vector_find_and_delete(&s->displays, &d);

   _al_vector_free(&dpy->bitmaps);
   al_free(dpy->vertex_cache);

   ALLEGRO_DEBUG("free display");
   al_free(d);

   ALLEGRO_DEBUG("done");
}

static bool android_set_current_display(ALLEGRO_DISPLAY *dpy)
{
   ALLEGRO_DEBUG("make current %p", dpy);

   if (al_get_current_display() != NULL){
     _al_android_clear_current(_al_android_get_jnienv(),
        (ALLEGRO_DISPLAY_ANDROID *)al_get_current_display());
   }

   if (dpy) {
      _al_android_make_current(_al_android_get_jnienv(),
         (ALLEGRO_DISPLAY_ANDROID *)dpy);
   }

   _al_ogl_update_render_state(dpy);

   return true;
}

static void android_unset_current_display(ALLEGRO_DISPLAY *dpy)
{
   ALLEGRO_DEBUG("unset current %p", dpy);
   _al_android_clear_current(_al_android_get_jnienv(),
      (ALLEGRO_DISPLAY_ANDROID *)dpy);
}

static void android_flip_display(ALLEGRO_DISPLAY *dpy)
{
   // Some Androids crash if you swap buffers with an fbo bound
   // so temporarily change target to the backbuffer.
   ALLEGRO_BITMAP *old_target = al_get_target_bitmap();
   al_set_target_backbuffer(dpy);

   _jni_callVoidMethod(_al_android_get_jnienv(),
      ((ALLEGRO_DISPLAY_ANDROID *)dpy)->surface_object, "egl_SwapBuffers");

   al_set_target_bitmap(old_target);

   /* Backup bitmaps created without ALLEGRO_NO_PRESERVE_TEXTURE that are
    * dirty, to system memory.
    */
   al_backup_dirty_bitmaps(dpy);
}

static void android_update_display_region(ALLEGRO_DISPLAY *dpy, int x, int y,
   int width, int height)
{
   (void)x;
   (void)y;
   (void)width;
   (void)height;
   android_flip_display(dpy);
}

static bool android_acknowledge_resize(ALLEGRO_DISPLAY *dpy)
{
   ALLEGRO_DISPLAY_ANDROID *d = (ALLEGRO_DISPLAY_ANDROID *)dpy;

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

static bool android_is_compatible_bitmap(ALLEGRO_DISPLAY *dpy,
   ALLEGRO_BITMAP *bmp)
{
   (void)dpy;
   (void)bmp;
   return true;
}

static bool android_resize_display(ALLEGRO_DISPLAY *dpy, int w, int h)
{
   (void)dpy;
   (void)w;
   (void)h;
   return false;
}

static void android_set_icons(ALLEGRO_DISPLAY *dpy, int num_icons,
   ALLEGRO_BITMAP *bmps[])
{
   (void)dpy;
   (void)num_icons;
   (void)bmps;
}

static void android_set_window_title(ALLEGRO_DISPLAY *dpy, const char *title)
{
   (void)dpy; (void)title;
}

static void android_set_window_position(ALLEGRO_DISPLAY *dpy, int x, int y)
{
   (void)dpy;
   (void)x;
   (void)y;
}

static void android_get_window_position(ALLEGRO_DISPLAY *dpy, int *x, int *y)
{
   (void)dpy;
   *x = *y = 0;
}

static bool android_wait_for_vsync(ALLEGRO_DISPLAY *dpy)
{
   (void)dpy;
   return false;
}

static bool android_set_mouse_cursor(ALLEGRO_DISPLAY *dpy,
   ALLEGRO_MOUSE_CURSOR *cursor)
{
   (void)dpy; (void)cursor;
   return false;
}

static bool android_set_system_mouse_cursor(ALLEGRO_DISPLAY *dpy,
   ALLEGRO_SYSTEM_MOUSE_CURSOR id)
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
      ALLEGRO_BITMAP **bptr = _al_vector_ref(&dpy->bitmaps, i);
      ALLEGRO_BITMAP *bmp = *bptr;
      int bitmap_flags = al_get_bitmap_flags(bmp);

      if (!bmp->parent &&
         !(bitmap_flags & ALLEGRO_MEMORY_BITMAP) &&
         !(bitmap_flags & ALLEGRO_NO_PRESERVE_TEXTURE))
      {
         ALLEGRO_BITMAP_EXTRA_OPENGL *extra = bmp->extra;
         al_remove_opengl_fbo(bmp);
         glDeleteTextures(1, &extra->texture);
         extra->texture = 0;
      }
   }

   ALLEGRO_DISPLAY_ANDROID *d = (ALLEGRO_DISPLAY_ANDROID *)dpy;

   _al_android_clear_current(_al_android_get_jnienv(), d);

   /* XXX mutex? */
   al_broadcast_cond(d->cond);

   ALLEGRO_DEBUG("acknowledged drawing halt");
}

static void android_broadcast_resume(ALLEGRO_DISPLAY_ANDROID *d)
{
   ALLEGRO_DEBUG("Broadcasting resume");
   d->resumed = true;
   al_broadcast_cond(d->cond);
   ALLEGRO_DEBUG("done broadcasting resume");
}

static void android_acknowledge_drawing_resume(ALLEGRO_DISPLAY *dpy)
{
   unsigned i;

   ALLEGRO_DEBUG("begin");

   ALLEGRO_DISPLAY_ANDROID *d = (ALLEGRO_DISPLAY_ANDROID *)dpy;

   _al_android_clear_current(_al_android_get_jnienv(), d);
   _al_android_make_current(_al_android_get_jnienv(), d);

   ALLEGRO_DEBUG("made current");

   if (dpy->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
      dpy->default_shader = _al_create_default_shader(dpy);
   }

   // Bitmaps can still have stale shaders attached.
   _al_glsl_unuse_shaders();

   // Restore the transformations.
   dpy->vt->update_transformation(dpy, al_get_target_bitmap());

   // Restore bitmaps
   // have to get this because new bitmaps could be created below
   for (i = 0; i < _al_vector_size(&dpy->bitmaps); i++) {
      ALLEGRO_BITMAP **bptr = _al_vector_ref(&dpy->bitmaps, i);
      ALLEGRO_BITMAP *bmp = *bptr;
      int bitmap_flags = al_get_bitmap_flags(bmp);

      if (!bmp->parent &&
         !(bitmap_flags & ALLEGRO_MEMORY_BITMAP) &&
         !(bitmap_flags & ALLEGRO_NO_PRESERVE_TEXTURE))
      {
         int format = al_get_bitmap_format(bmp);
         format = _al_pixel_format_is_compressed(format) ? ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE : format;
         _al_ogl_upload_bitmap_memory(bmp, format, bmp->memory);
         bmp->dirty = false;
      }
   }

   android_broadcast_resume(d);

   ALLEGRO_DEBUG("acknowledge_drawing_resume end");
}

static void android_set_display_option(ALLEGRO_DISPLAY *d, int o, int v)
{
   (void)d;

   if (o == ALLEGRO_SUPPORTED_ORIENTATIONS) {
      _jni_callVoidMethodV(_al_android_get_jnienv(),
         _al_android_activity_object(), "setAllegroOrientation", "(I)V", v);
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
   vt->set_icons = android_set_icons;
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

   vt->set_display_option = android_set_display_option;

   vt->update_render_state = _al_ogl_update_render_state;

   _al_ogl_add_drawing_functions(vt);
   _al_android_add_clipboard_functions(vt);

   return vt;
}

/* vim: set sts=3 sw=3 et: */
