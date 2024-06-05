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
 *      Thread local storage.
 *
 *      By Trent Gamblin.
 *
 */

/* FIXME:
 *
 * There are several ways to get thread local storage:
 *
 * 1. pthreads.
 * 2. __thread keyword in gcc.
 * 3. __declspec(thread) in MSVC.
 * 4. TLS API under Windows.
 *
 * Since pthreads is available from the system everywhere except in
 * Windows, this is the only case which is problematic. It appears
 * that except for old mingw versions (before gcc 4.2) we can simply
 * use __thread, and for MSVC we can always use __declspec(thread):
 *
 * However there also is a WANT_TLS configuration variable which is on
 * by default and forces use of the TLS API instead. At the same time,
 * the implementation using the TLS API in this file does not work
 * with static linking. Someone should either completely remove
 * WANT_TLS, or fix the static linking case...
 */

#include <string.h>
#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_file.h"
#include "allegro5/internal/aintern_fshook.h"
#include "allegro5/internal/aintern_shader.h"
#include "allegro5/internal/aintern_tls.h"

#ifdef ALLEGRO_ANDROID
#include "allegro5/internal/aintern_android.h"
#endif

#if defined(ALLEGRO_MINGW32) && !defined(ALLEGRO_CFG_DLL_TLS)
   /*
    * MinGW < 4.2.1 doesn't have builtin thread local storage, so we
    * must use the Windows API.
    */
   #if __GNUC__ < 4
      #define ALLEGRO_CFG_DLL_TLS
   #elif __GNUC__ == 4 && __GNUC_MINOR__ < 2
      #define ALLEGRO_CFG_DLL_TLS
   #elif __GNUC__ == 4 && __GNUC_MINOR__ == 2 && __GNUC_PATCHLEVEL__ < 1
      #define ALLEGRO_CFG_DLL_TLS
   #endif
#endif


/* Thread local storage for various per-thread state. */
typedef struct thread_local_state {
   /* New display parameters */
   int new_display_flags;
   int new_display_refresh_rate;
   int new_display_adapter;
   int new_window_x;
   int new_window_y;
   int new_bitmap_depth;
   int new_bitmap_samples;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS new_display_settings;

   /* Current display */
   ALLEGRO_DISPLAY *current_display;

   /* Target bitmap */
   ALLEGRO_BITMAP *target_bitmap;

   /* Blender */
   ALLEGRO_BLENDER current_blender;

   /* Bitmap parameters */
   int new_bitmap_format;
   int new_bitmap_flags;
   int new_bitmap_wrap_u;
   int new_bitmap_wrap_v;

   /* Files */
   const ALLEGRO_FILE_INTERFACE *new_file_interface;
   const ALLEGRO_FS_INTERFACE *fs_interface;

   /* Error code */
   int allegro_errno;

   /* Title to use for a new window/display.
    * This is a static buffer for API reasons.
    */
   char new_window_title[ALLEGRO_NEW_WINDOW_TITLE_MAX_SIZE + 1];

#ifdef ALLEGRO_ANDROID
   JNIEnv *jnienv;
#endif

   /* Destructor ownership count */
   int dtor_owner_count;
} thread_local_state;


typedef struct INTERNAL_STATE {
   thread_local_state tls;
   ALLEGRO_BLENDER stored_blender;
   ALLEGRO_TRANSFORM stored_transform;
   ALLEGRO_TRANSFORM stored_projection_transform;
   int flags;
} INTERNAL_STATE;

ALLEGRO_STATIC_ASSERT(tls, sizeof(ALLEGRO_STATE) > sizeof(INTERNAL_STATE));


static void initialize_blender(ALLEGRO_BLENDER *b)
{
   b->blend_op = ALLEGRO_ADD;
   b->blend_source = ALLEGRO_ONE,
   b->blend_dest = ALLEGRO_INVERSE_ALPHA;
   b->blend_alpha_op = ALLEGRO_ADD;
   b->blend_alpha_source = ALLEGRO_ONE;
   b->blend_alpha_dest = ALLEGRO_INVERSE_ALPHA;
   b->blend_color = al_map_rgba_f(1.0f, 1.0f, 1.0f, 1.0f);
}


static void initialize_tls_values(thread_local_state *tls)
{
   memset(tls, 0, sizeof *tls);

   tls->new_display_adapter = ALLEGRO_DEFAULT_DISPLAY_ADAPTER;
   tls->new_window_x = INT_MAX;
   tls->new_window_y = INT_MAX;

   initialize_blender(&tls->current_blender);
   tls->new_bitmap_flags = ALLEGRO_CONVERT_BITMAP;
   tls->new_bitmap_format = ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA;
   tls->new_bitmap_wrap_u = ALLEGRO_BITMAP_WRAP_DEFAULT;
   tls->new_bitmap_wrap_v = ALLEGRO_BITMAP_WRAP_DEFAULT;
   tls->new_file_interface = &_al_file_interface_stdio;
   tls->fs_interface = &_al_fs_interface_stdio;
   memset(tls->new_window_title, 0, ALLEGRO_NEW_WINDOW_TITLE_MAX_SIZE + 1);

   _al_fill_display_settings(&tls->new_display_settings);
}

// FIXME: The TLS implementation below only works for dynamic linking
// right now - instead of using DllMain we should simply initialize
// on first request.
#ifdef ALLEGRO_STATICLINK
   #undef ALLEGRO_CFG_DLL_TLS
#endif

#if defined(ALLEGRO_CFG_DLL_TLS)
   #include "tls_dll.inc"
#elif defined(ALLEGRO_MACOSX) || defined(ALLEGRO_IPHONE) || defined(ALLEGRO_ANDROID) || defined(ALLEGRO_RASPBERRYPI)
   #include "tls_pthread.inc"
#else
   #include "tls_native.inc"
#endif



void _al_reinitialize_tls_values(void)
{
   thread_local_state *tls;
   if ((tls = tls_get()) == NULL)
      return;
   initialize_tls_values(tls);
}



void _al_set_new_display_settings(ALLEGRO_EXTRA_DISPLAY_SETTINGS *settings)
{
   thread_local_state *tls;
   if ((tls = tls_get()) == NULL)
      return;
   memmove(&tls->new_display_settings, settings, sizeof(ALLEGRO_EXTRA_DISPLAY_SETTINGS));
}



ALLEGRO_EXTRA_DISPLAY_SETTINGS *_al_get_new_display_settings(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return &tls->new_display_settings;
}


/* Function: al_set_new_window_title
 */
void al_set_new_window_title(const char *title)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;

   _al_sane_strncpy(tls->new_window_title, title, sizeof(tls->new_window_title));
}



/* Function: al_get_new_window_title
 */
const char *al_get_new_window_title(void)
{
   thread_local_state *tls;

   /* Return app name in case of error or if not set before. */
   if ((tls = tls_get()) == NULL)
      return al_get_app_name();

   if (tls->new_window_title[0] == '\0')
      return al_get_app_name();

   return (const char *)tls->new_window_title;
}




/* Function: al_set_new_display_flags
 */
void al_set_new_display_flags(int flags)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_display_flags = flags;
}



/* Function: al_get_new_display_flags
 */
int al_get_new_display_flags(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->new_display_flags;
}



/* Function: al_set_new_display_refresh_rate
 */
void al_set_new_display_refresh_rate(int refresh_rate)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_display_refresh_rate = refresh_rate;
}



/* Function: al_get_new_display_refresh_rate
 */
int al_get_new_display_refresh_rate(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->new_display_refresh_rate;
}



/* Function: al_set_new_display_adapter
 */
void al_set_new_display_adapter(int adapter)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;

   if (adapter < 0) {
      tls->new_display_adapter = ALLEGRO_DEFAULT_DISPLAY_ADAPTER;
   }
   tls->new_display_adapter = adapter;
}



/* Function: al_get_new_display_adapter
 */
int al_get_new_display_adapter(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return ALLEGRO_DEFAULT_DISPLAY_ADAPTER;
   return tls->new_display_adapter;
}



/* Function: al_set_new_window_position
 */
void al_set_new_window_position(int x, int y)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_window_x = x;
   tls->new_window_y = y;
}


/* Function: al_get_new_window_position
 */
void al_get_new_window_position(int *x, int *y)
{
   thread_local_state *tls;
   int new_window_x = INT_MAX;
   int new_window_y = INT_MAX;

   if ((tls = tls_get()) != NULL) {
      new_window_x = tls->new_window_x;
      new_window_y = tls->new_window_y;
   }

   if (x)
      *x = new_window_x;
   if (y)
      *y = new_window_y;
}



/* Make the given display current, without changing the target bitmap.
 * This is used internally to change the current display transiently.
 */
bool _al_set_current_display_only(ALLEGRO_DISPLAY *display)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return false;

   if (tls->current_display &&
         tls->current_display->vt &&
         tls->current_display->vt->unset_current_display) {
      tls->current_display->vt->unset_current_display(tls->current_display);
      tls->current_display = NULL;
   }

   if (display &&
         display->vt &&
         display->vt->set_current_display) {
      if (!display->vt->set_current_display(display))
         return false;
   }

   tls->current_display = display;
   return true;
}



/* Function: al_get_current_display
 */
ALLEGRO_DISPLAY *al_get_current_display(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return NULL;
   return tls->current_display;
}



/* Function: al_set_target_bitmap
 */
void al_set_target_bitmap(ALLEGRO_BITMAP *bitmap)
{
   thread_local_state *tls;
   ALLEGRO_DISPLAY *old_display;
   ALLEGRO_DISPLAY *new_display;
   ALLEGRO_SHADER *old_shader;
   ALLEGRO_SHADER *new_shader;
   bool same_shader;
   int bitmap_flags = bitmap ? al_get_bitmap_flags(bitmap) : 0;

   ASSERT(!al_is_bitmap_drawing_held());

   if (bitmap) {
      if (bitmap->parent) {
         bitmap->parent->dirty = true;
      }
      else {
         bitmap->dirty = true;
      }
   }

   if ((tls = tls_get()) == NULL)
      return;

   old_display = tls->current_display;

   if (tls->target_bitmap)
      old_shader = tls->target_bitmap->shader;
   else
      old_shader = NULL;

   if (bitmap == NULL) {
      /* Explicitly releasing the current rendering context. */
      new_display = NULL;
      new_shader = NULL;
   }
   else if (bitmap_flags & ALLEGRO_MEMORY_BITMAP) {
      /* Setting a memory bitmap doesn't change the rendering context. */
      new_display = old_display;
      new_shader = NULL;
   }
   else {
      new_display = _al_get_bitmap_display(bitmap);
      new_shader = bitmap->shader;
   }

   same_shader = (old_shader == new_shader && old_display == new_display);

   /* Unset the old shader if necessary. */
   if (old_shader && !same_shader) {
      old_shader->vt->unuse_shader(old_shader, old_display);
   }

   /* Change the rendering context if necessary. */
   if (old_display != new_display) {
      if (old_display &&
            old_display->vt &&
            old_display->vt->unset_current_display) {
         old_display->vt->unset_current_display(old_display);
      }

      tls->current_display = new_display;

      if (new_display &&
            new_display->vt &&
            new_display->vt->set_current_display) {
         new_display->vt->set_current_display(new_display);
      }
   }

   /* Change the target bitmap itself. */
   tls->target_bitmap = bitmap;

   if (bitmap &&
         !(bitmap_flags & ALLEGRO_MEMORY_BITMAP) &&
         new_display &&
         new_display->vt &&
         new_display->vt->set_target_bitmap)
   {
      new_display->vt->set_target_bitmap(new_display, bitmap);

      /* Set the new shader if necessary.  This should done before the
       * update_transformation call, which will also update the shader's
       * al_projview_matrix variable.
       */
      if (!same_shader || !new_shader) {
         al_use_shader(new_shader);
      }

      new_display->vt->update_transformation(new_display, bitmap);
   }
}



/* Function: al_set_target_backbuffer
 */
void al_set_target_backbuffer(ALLEGRO_DISPLAY *display)
{
   al_set_target_bitmap(al_get_backbuffer(display));
}



/* Function: al_get_target_bitmap
 */
ALLEGRO_BITMAP *al_get_target_bitmap(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->target_bitmap;
}



/* Function: al_set_blender
 */
void al_set_blender(int op, int src, int dst)
{
   al_set_separate_blender(op, src, dst, op, src, dst);
}



/* Function: al_set_blend_color
*/
void al_set_blend_color(ALLEGRO_COLOR color)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;

   tls->current_blender.blend_color = color;
}


/* Function: al_set_separate_blender
 */
void al_set_separate_blender(int op, int src, int dst,
   int alpha_op, int alpha_src, int alpha_dst)
{
   thread_local_state *tls;
   ALLEGRO_BLENDER *b;

   ASSERT(op >= 0 && op < ALLEGRO_NUM_BLEND_OPERATIONS);
   ASSERT(src >= 0 && src < ALLEGRO_NUM_BLEND_MODES);
   ASSERT(dst >= 0 && src < ALLEGRO_NUM_BLEND_MODES);
   ASSERT(alpha_op >= 0 && alpha_op < ALLEGRO_NUM_BLEND_OPERATIONS);
   ASSERT(alpha_src >= 0 && alpha_src < ALLEGRO_NUM_BLEND_MODES);
   ASSERT(alpha_dst >= 0 && alpha_dst < ALLEGRO_NUM_BLEND_MODES);

   if ((tls = tls_get()) == NULL)
      return;

   b = &tls->current_blender;

   b->blend_op = op;
   b->blend_source = src;
   b->blend_dest = dst;
   b->blend_alpha_op = alpha_op;
   b->blend_alpha_source = alpha_src;
   b->blend_alpha_dest = alpha_dst;
}



/* Function: al_get_blender
 */
void al_get_blender(int *op, int *src, int *dst)
{
   al_get_separate_blender(op, src, dst, NULL, NULL, NULL);
}



/* Function: al_get_blend_color
*/
ALLEGRO_COLOR al_get_blend_color(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return al_map_rgba(255, 255, 255, 255);

   return tls->current_blender.blend_color;
}


/* Function: al_get_separate_blender
 */
void al_get_separate_blender(int *op, int *src, int *dst,
   int *alpha_op, int *alpha_src, int *alpha_dst)
{
   thread_local_state *tls;
   ALLEGRO_BLENDER *b;

   if ((tls = tls_get()) == NULL)
      return;

   b = &tls->current_blender;

   if (op)
      *op = b->blend_op;

   if (src)
      *src = b->blend_source;

   if (dst)
      *dst = b->blend_dest;

   if (alpha_op)
      *alpha_op = b->blend_alpha_op;

   if (alpha_src)
      *alpha_src = b->blend_alpha_source;

   if (alpha_dst)
      *alpha_dst = b->blend_alpha_dest;
}



/* Function: al_set_new_bitmap_format
 */
void al_set_new_bitmap_format(int format)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_bitmap_format = format;
}



/* Function: al_set_new_bitmap_flags
 */
void al_set_new_bitmap_flags(int flags)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;

   tls->new_bitmap_flags = flags;
}



/* Function: al_add_new_bitmap_flag
 */
void al_add_new_bitmap_flag(int flag)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_bitmap_flags |= flag;
}



/* Function: al_get_new_bitmap_format
 */
int al_get_new_bitmap_format(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->new_bitmap_format;
}



/* Function: al_get_new_bitmap_flags
 */
int al_get_new_bitmap_flags(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->new_bitmap_flags;
}



/* Function: al_store_state
 */
void al_store_state(ALLEGRO_STATE *state, int flags)
{
   thread_local_state *tls;
   INTERNAL_STATE *stored;

   if ((tls = tls_get()) == NULL)
      return;

   stored = (void *)state;
   stored->flags = flags;

#define _STORE(x) (stored->tls.x = tls->x)

   if (flags & ALLEGRO_STATE_NEW_DISPLAY_PARAMETERS) {
      _STORE(new_display_flags);
      _STORE(new_display_refresh_rate);
      _STORE(new_display_adapter);
      _STORE(new_window_x);
      _STORE(new_window_y);
      _STORE(new_display_settings);
      _al_sane_strncpy(stored->tls.new_window_title, tls->new_window_title,
                       sizeof(stored->tls.new_window_title));
   }

   if (flags & ALLEGRO_STATE_NEW_BITMAP_PARAMETERS) {
      _STORE(new_bitmap_format);
      _STORE(new_bitmap_flags);
      _STORE(new_bitmap_wrap_u);
      _STORE(new_bitmap_wrap_v);
   }

   if (flags & ALLEGRO_STATE_DISPLAY) {
      _STORE(current_display);
   }

   if (flags & ALLEGRO_STATE_TARGET_BITMAP) {
      _STORE(target_bitmap);
   }

   if (flags & ALLEGRO_STATE_BLENDER) {
      stored->stored_blender = tls->current_blender;
   }

   if (flags & ALLEGRO_STATE_NEW_FILE_INTERFACE) {
      _STORE(new_file_interface);
      _STORE(fs_interface);
   }

   if (flags & ALLEGRO_STATE_TRANSFORM) {
      ALLEGRO_BITMAP *target = al_get_target_bitmap();
      if (!target)
         al_identity_transform(&stored->stored_transform);
      else
         stored->stored_transform = target->transform;
   }

   if (flags & ALLEGRO_STATE_PROJECTION_TRANSFORM) {
      ALLEGRO_BITMAP *target = al_get_target_bitmap();
      if (target) {
         stored->stored_projection_transform = target->proj_transform;
      }
   }

#undef _STORE
}



/* Function: al_restore_state
 */
void al_restore_state(ALLEGRO_STATE const *state)
{
   thread_local_state *tls;
   INTERNAL_STATE *stored;
   int flags;

   if ((tls = tls_get()) == NULL)
      return;

   stored = (void *)state;
   flags = stored->flags;

#define _RESTORE(x) (tls->x = stored->tls.x)

   if (flags & ALLEGRO_STATE_NEW_DISPLAY_PARAMETERS) {
      _RESTORE(new_display_flags);
      _RESTORE(new_display_refresh_rate);
      _RESTORE(new_display_adapter);
      _RESTORE(new_window_x);
      _RESTORE(new_window_y);
      _RESTORE(new_display_settings);
      _al_sane_strncpy(tls->new_window_title, stored->tls.new_window_title,
                       sizeof(tls->new_window_title));
   }

   if (flags & ALLEGRO_STATE_NEW_BITMAP_PARAMETERS) {
      _RESTORE(new_bitmap_format);
      _RESTORE(new_bitmap_flags);
      _RESTORE(new_bitmap_wrap_u);
      _RESTORE(new_bitmap_wrap_v);
   }

   if (flags & ALLEGRO_STATE_DISPLAY) {
      if (tls->current_display != stored->tls.current_display) {
         _al_set_current_display_only(stored->tls.current_display);
         ASSERT(tls->current_display == stored->tls.current_display);
      }
   }

   if (flags & ALLEGRO_STATE_TARGET_BITMAP) {
      if (tls->target_bitmap != stored->tls.target_bitmap) {
         al_set_target_bitmap(stored->tls.target_bitmap);
         ASSERT(tls->target_bitmap == stored->tls.target_bitmap);
      }
   }

   if (flags & ALLEGRO_STATE_BLENDER) {
      tls->current_blender = stored->stored_blender;
   }

   if (flags & ALLEGRO_STATE_NEW_FILE_INTERFACE) {
      _RESTORE(new_file_interface);
      _RESTORE(fs_interface);
   }

   if (flags & ALLEGRO_STATE_TRANSFORM) {
      ALLEGRO_BITMAP *bitmap = al_get_target_bitmap();
      if (bitmap)
         al_use_transform(&stored->stored_transform);
   }

   if (flags & ALLEGRO_STATE_PROJECTION_TRANSFORM) {
      ALLEGRO_BITMAP *bitmap = al_get_target_bitmap();
      if (bitmap)
         al_use_projection_transform(&stored->stored_projection_transform);
   }

#undef _RESTORE
}



/* Function: al_get_new_file_interface
 * FIXME: added a work-around for the situation where TLS has not yet been
 * initialised when this function is called. This may happen if Allegro
 * tries to load a configuration file before the system has been
 * initialised. Should probably rethink the logic here...
 */
const ALLEGRO_FILE_INTERFACE *al_get_new_file_interface(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return &_al_file_interface_stdio;

   /* FIXME: this situation should never arise because tls_ has the stdio
    * interface set as a default, but it arises on OS X if
    * pthread_getspecific() is called before pthreads_thread_init()...
    */
   if (tls->new_file_interface)
      return tls->new_file_interface;
   else
      return &_al_file_interface_stdio;
}



/* Function: al_set_new_file_interface
 */
void al_set_new_file_interface(const ALLEGRO_FILE_INTERFACE *file_interface)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_file_interface = file_interface;
}



/* Function: al_get_fs_interface
 */
const ALLEGRO_FS_INTERFACE *al_get_fs_interface(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return &_al_fs_interface_stdio;

   if (tls->fs_interface)
      return tls->fs_interface;
   else
      return &_al_fs_interface_stdio;
}



/* Function: al_set_fs_interface
 */
void al_set_fs_interface(const ALLEGRO_FS_INTERFACE *fs_interface)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->fs_interface = fs_interface;
}



/* Function: al_set_standard_fs_interface
 */
void al_set_standard_fs_interface(void)
{
   al_set_fs_interface(&_al_fs_interface_stdio);
}


#define SETTER(name, value) \
{ \
   thread_local_state *tls; \
   if ((tls = tls_get()) == NULL) \
      return; \
   tls->name = value; \
}

#define GETTER(name, value) \
{ \
   thread_local_state *tls; \
   if ((tls = tls_get()) == NULL) \
      return value; \
   return tls->name; \
}

/* Function: al_get_errno
 */
int al_get_errno(void)
GETTER(allegro_errno, 0)

/* Function: al_set_errno
 */
void al_set_errno(int errnum)
SETTER(allegro_errno, errnum)

/* Function: al_get_new_bitmap_depth
 */
int al_get_new_bitmap_depth(void)
GETTER(new_bitmap_depth, 0)

/* Function: al_set_new_bitmap_depth
 */
void al_set_new_bitmap_depth(int depth)
SETTER(new_bitmap_depth, depth)

/* Function: al_get_new_bitmap_samples
 */
int al_get_new_bitmap_samples(void)
GETTER(new_bitmap_samples, 0)

/* Function: al_set_new_bitmap_samples
 */
void al_set_new_bitmap_samples(int samples)
SETTER(new_bitmap_samples, samples)

/* Function: al_get_bitmap_wrap
 */
void al_get_new_bitmap_wrap(ALLEGRO_BITMAP_WRAP *u, ALLEGRO_BITMAP_WRAP *v)
{
   ASSERT(u);
   ASSERT(v);

   thread_local_state *tls;
   if ((tls = tls_get()) == NULL)
      return;

   *u = tls->new_bitmap_wrap_u;
   *v = tls->new_bitmap_wrap_v;
}

/* Function: al_set_new_bitmap_wrap
 */
void al_set_new_bitmap_wrap(ALLEGRO_BITMAP_WRAP u, ALLEGRO_BITMAP_WRAP v)
{
   thread_local_state *tls;
   if ((tls = tls_get()) == NULL)
      return;
   tls->new_bitmap_wrap_u = u;
   tls->new_bitmap_wrap_v = v;
}

#ifdef ALLEGRO_ANDROID
JNIEnv *_al_android_get_jnienv(void)
GETTER(jnienv, 0)

void _al_android_set_jnienv(JNIEnv *jnienv)
SETTER(jnienv, jnienv)
#endif


int *_al_tls_get_dtor_owner_count(void)
{
   thread_local_state *tls;

   tls = tls_get();
   return &tls->dtor_owner_count;
}


/* vim: set sts=3 sw=3 et: */
