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

/* Title: Thread local storage
 *
 * Some of Allegro's "global" state is thread-specific.  For example, each
 * thread in the program will have it's own target bitmap, and calling
 * <al_set_target_bitmap> will only change the target bitmap of the calling
 * thread.  This reduces the problems with global state in multithreaded
 * programs.
 */


#include <string.h>
#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_memory.h"


/* Thread local storage for graphics API state */
typedef struct thread_local_state {
   /* Display parameters */
   int new_display_format;
   int new_display_refresh_rate;
   int new_display_flags;
   /* Current display */
   ALLEGRO_DISPLAY *current_display;
   /* Target bitmap */
   ALLEGRO_BITMAP *target_bitmap;
   /* For pushing/popping target bitmap */
   ALLEGRO_BITMAP *target_bitmap_backup;
   /* Bitmap parameters */
   int new_bitmap_format;
   int new_bitmap_flags;
   /* For pushing/popping bitmap parameters */
   int new_bitmap_format_backup;
   int new_bitmap_flags_backup;
   /* Blending modes and color */
   int blend_source;
   int blend_dest;
   ALLEGRO_COLOR blend_color;
   ALLEGRO_MEMORY_BLENDER memory_blender;
   int src_blender_backup;
   int dst_blender_backup;
   ALLEGRO_COLOR blend_color_backup;
   /* Error code */
   int allegro_errno;
} thread_local_state;


#if defined ALLEGRO_MINGW32 && (__GNUC__ < 4 || __GNUC_MINOR__ < 2 \
   || __GNUC_PATCHLEVEL__ < 1)


/*
 * MinGW doesn't have builtin thread local storage, so we
 * must use the Windows API. This means that the MinGW
 * build must be built as a DLL.
 */

#include "allegro5/winalleg.h"


static DWORD tls_index;


static thread_local_state *tls_get(void)
{
   thread_local_state *t = TlsGetValue(tls_index);
   return t;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{ 
   thread_local_state *data;
 
   switch (fdwReason) { 
      case DLL_PROCESS_ATTACH: 
         if ((tls_index = TlsAlloc()) == TLS_OUT_OF_INDEXES)
            return false;
            // No break: Initialize the index for first thread.
            // The attached process creates a new thread. 

      case DLL_THREAD_ATTACH: 
          // Initialize the TLS index for this thread.
          data = _AL_MALLOC(sizeof(*data));
          if (data != NULL) {
             memset(data, 0, sizeof(*data));
             data->new_bitmap_format = ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA;
             data->blend_source = ALLEGRO_ALPHA;
             data->blend_dest = ALLEGRO_INVERSE_ALPHA;
             data->blend_color.r = data->blend_color.g = data->blend_color.b
                = data->blend_color.a = 1.0f;
             data->memory_blender = _al_blender_alpha_inverse_alpha;
             TlsSetValue(tls_index, data); 
          }
             break; 
 
        // The thread of the attached process terminates.
      case DLL_THREAD_DETACH: 
         // Release the allocated memory for this thread.
         data = TlsGetValue(tls_index); 
         if (data != NULL) 
            _AL_FREE(data);
 
         break; 
 
      // DLL unload due to process termination or FreeLibrary. 
      case DLL_PROCESS_DETACH: 
         // Release the allocated memory for this thread.
         data = TlsGetValue(tls_index); 
         if (data != NULL) 
            _AL_FREE(data);
         // Release the TLS index.
         TlsFree(tls_index); 
         break; 
 
      default: 
         break; 
   } 
 
   return true; 
}


#else /* not defined ALLEGRO_MINGW32 */

#if defined(ALLEGRO_MSVC) || defined(ALLEGRO_BCC32)

#define THREAD_LOCAL __declspec(thread)
#define HAVE_NATIVE_TLS

#elif defined ALLEGRO_MACOSX

#define THREAD_LOCAL

static pthread_key_t tls_key = 0;

static void osx_thread_destroy(void* ptr)
{
   _AL_FREE(ptr);
}


void osx_threads_init(void)
{
   pthread_key_create(&tls_key, osx_thread_destroy);
}

static thread_local_state _tls;

static thread_local_state* osx_thread_init(void)
{
   /* Allocate and copy the 'template' object */
   thread_local_state* ptr = (thread_local_state*)_AL_MALLOC(sizeof(thread_local_state));
   memcpy(ptr, &_tls, sizeof(thread_local_state));
   pthread_setspecific(tls_key, ptr);
   return ptr;
}

/* This function is short so it can hopefully be inlined. */
static thread_local_state* tls_get(void)
{
   thread_local_state* ptr = (thread_local_state*)pthread_getspecific(tls_key);
   if (ptr == NULL)
   {
      /* Must create object */
      ptr = osx_thread_init();
   }
   return ptr;
}

#else /* not MSVC/BCC32, not OSX */

#define THREAD_LOCAL __thread
#define HAVE_NATIVE_TLS

#endif /* end not MSVC/BCC32, not OSX */


static THREAD_LOCAL thread_local_state _tls = {
   0,                                     /* new_display_format */
   0,                                     /* new_display_refresh_rate */
   0,                                     /* new_display_flags */
   NULL,                                  /* current_display */
   NULL,                                  /* target_bitmap */
   NULL,                                  /* target_bitmap_backup */
   ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA,   /* new_bitmap_format */
   0,                                     /* new_bitmap_flags */
   0,                                     /* new_bitmap_format_backup */
   0,                                     /* new_bitmap_flags_backup */
   ALLEGRO_ALPHA,                         /* blend_source */
   ALLEGRO_INVERSE_ALPHA,                 /* blend_dest */
   { 1.0f, 1.0f, 1.0f, 1.0f },            /* blend_color  */
   _al_blender_alpha_inverse_alpha,       /* memory_blender */
   0,                                     /* src_blender_backup */
   0,                                     /* dst_blender_backup */
   { 1.0f, 1.0f, 1.0f, 1.0f },            /* blend_color_backup */
   0                                      /* errno */
};


#ifdef HAVE_NATIVE_TLS
static thread_local_state *tls_get(void)
{
   return &_tls;
}
#endif /* end HAVE_NATIVE_TLS */


#endif /* end not defined ALLEGRO_MINGW32 */



/* Function: al_set_new_display_format
 *
 * Set the pixel format for al displays created after this call.
 */
void al_set_new_display_format(int format)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_display_format = format;
}



/* Function: al_set_new_display_refresh_rate
 *
 * Sets the refresh rate to use for newly created displays. If the refresh rate
 * is not available, al_create_display will fail. A list of modes with refresh
 * rates can be found with <al_get_num_display_modes> and <al_get_display_mode>,
 * documented above.
 *
 * See Also:
 *    <al_get_display_mode>
 */
void al_set_new_display_refresh_rate(int refresh_rate)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_display_refresh_rate = refresh_rate;
}



/* Function: al_set_new_display_flags
 *
 * Sets various flags for display creation. flags is a bitfield containing any
 * reasonable combination of the following:
 *
 * ALLEGRO_WINDOWED - prefer a windowed mode
 *
 * ALLEGRO_FULLSCREEN - prefer a fullscreen mode
 *
 * ALLEGRO_RESIZABLE - the display is resizable (only applicable if combined
 * with ALLEGRO_WINDOWED)
 *
 * ALLEGRO_OPENGL - require the driver to provide an initialized opengl context
 * after returning successfully
 *
 * ALLEGRO_DIRECT3D - require the driver to do rendering with Direct3D and
 * provide a Direct3D device
 *
 * ALLEGRO_DOUBLEBUFFER - use double buffering
 *
 * ALLEGRO_PAGEFLIP - use page flipping
 *
 * ALLEGRO_SINGLEBUFFER - use only one buffer (front and back buffer are the
 * same) 
 * 
 * ALLEGRO_NOFRAME - Try to create a window without a frame (i.e. no border or
 * titlebar). This usualy does nothing for fullscreen modes, and even in
 * windowed moded it depends on the underlying platform whether it is supported
 * or not.
 * 
 * 0 can be used for default values. 
 */
void al_set_new_display_flags(int flags)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_display_flags = flags;
}



/* Function: al_get_new_display_format
 *
 * Gets the current pixel format used for newly created displays.
 */
int al_get_new_display_format(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->new_display_format;
}



/* Function: al_get_new_display_refresh_rate
 *
 * Gets the current refresh rate used for newly created displays.
 */
int al_get_new_display_refresh_rate(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->new_display_refresh_rate;
}



/* Function: al_get_new_display_flags
 *
 * Gets the current flags used for newly created displays.
 */
int al_get_new_display_flags(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->new_display_flags;
}



/* Function: al_set_current_display
 *
 * Change the current display for the calling thread.
 * Also sets the target bitmap to the display's
 * backbuffer. Returns true on success.
 */
bool al_set_current_display(ALLEGRO_DISPLAY *display)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return false;

   if (display) {
      if (display->vt->set_current_display(display)) {
         tls->current_display = display;
         al_set_target_bitmap(al_get_backbuffer());
         return true;
      }
      else {
         return false;
      }
   }
   else {
      tls->current_display = display;
   }

   return true;
}



/* Function: al_get_current_display
 *
 * Query for the current display in the calling thread.
 */
ALLEGRO_DISPLAY *al_get_current_display(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->current_display;
}



/* Function: al_set_target_bitmap
 *
 * Select the bitmap to which all subsequent drawing operations in the calling
 * thread will draw. 
 */
void al_set_target_bitmap(ALLEGRO_BITMAP *bitmap)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;

   tls->target_bitmap = bitmap;
   if (bitmap != NULL) {
      if (!(bitmap->flags & ALLEGRO_MEMORY_BITMAP)) {
         ALLEGRO_DISPLAY *display = tls->current_display;
         ASSERT(display);
         display->vt->set_target_bitmap(display, bitmap);
      }
   }
}



/* Function: al_get_target_bitmap
 *
 * Return the target bitmap of the current display.
 */
ALLEGRO_BITMAP *al_get_target_bitmap(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->target_bitmap;
}



void _al_push_target_bitmap(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->target_bitmap_backup = tls->target_bitmap;
}



void _al_pop_target_bitmap(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->target_bitmap = tls->target_bitmap_backup;
   al_set_target_bitmap(tls->target_bitmap);
}



void _al_push_blender(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;

   al_get_blender(
      &tls->src_blender_backup,
      &tls->dst_blender_backup,
      &tls->blend_color_backup);
}



void _al_pop_blender(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;

   al_set_blender(
      tls->src_blender_backup,
      tls->dst_blender_backup,
      tls->blend_color_backup);
}



/* Function: al_set_new_bitmap_format
 *
 * Sets the pixel format for newly created bitmaps. format
 * is one of the same values as used for <al_set_new_display_format>.
 * The default format is 0 and means the display driver will choose
 * the best format.
 */
void al_set_new_bitmap_format(int format)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_bitmap_format = format;
}



/* Function: al_set_new_bitmap_flags
 *
 * Sets the flags to use for newly created bitmaps.
 * Valid flags are:
 *
 * ALLEGRO_MEMORY_BITMAP - The bitmap will use a format most closely resembling
 * the format used in the bitmap file and al_create_memory_bitmap will be used
 * to create it. If this flag is not specified, al_create_bitmap will be used
 * instead and the display driver will determine the format.
 *
 * ALLEGRO_KEEP_BITMAP_FORMAT - Only used when loading bitmaps from disk files,
 * forces the resulting ALLEGRO_BITMAP to use the same format as the file.
 *
 * ALLEGRO_FORCE_LOCKING - When drawing to a bitmap with this flag set, always
 * use pixel locking and draw to it using Allegro's software drawing primitives.
 * This should never be used as it may cause severe performance penalties, but
 * can be useful for debugging.
 */
void al_set_new_bitmap_flags(int flags)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_bitmap_flags = flags;
}



/* Function: al_get_new_bitmap_format
 *
 * Returns the format used for newly created bitmaps.
 */
int al_get_new_bitmap_format(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->new_bitmap_format;
}



/* Function: al_get_new_bitmap_flags
 *
 * Returns the flags used for newly created bitmaps.
 */
int al_get_new_bitmap_flags(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->new_bitmap_flags;
}



void _al_push_new_bitmap_parameters(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_bitmap_format_backup = tls->new_bitmap_format;
   tls->new_bitmap_flags_backup = tls->new_bitmap_flags;
}



void _al_pop_new_bitmap_parameters(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_bitmap_format = tls->new_bitmap_format_backup;
   tls->new_bitmap_flags = tls->new_bitmap_flags_backup;
}



#define _STORE(x) stored->x = tls->x;
/* Function: al_store_state
 * 
 * Stores part of the state of the current thread in the given <ALLEGRO_STATE>
 * objects. The flags parameter can take any bit-combination of the flags
 * described under <ALLEGRO_STATE_FLAGS>.
 */
void al_store_state(ALLEGRO_STATE *state, int flags)
{
   thread_local_state *tls;
   if ((tls = tls_get()) == NULL)
      return;
      
   ASSERT(sizeof(ALLEGRO_STATE) > sizeof(thread_local_state) + sizeof(int));
   state->flags = flags;
   
   thread_local_state *stored = (void *)&state->_tls;

   if (flags & ALLEGRO_STATE_NEW_DISPLAY_PARAMETERS) {
      _STORE(new_display_format);
      _STORE(new_display_refresh_rate);
      _STORE(new_display_flags);
   }
   
   if (flags & ALLEGRO_STATE_NEW_BITMAP_PARAMETERS) {
      _STORE(new_bitmap_format);
      _STORE(new_bitmap_flags);
   }
   
   if (flags & ALLEGRO_STATE_DISPLAY) {
      _STORE(current_display);
   }
   
   if (flags & ALLEGRO_STATE_TARGET_BITMAP) {
      _STORE(target_bitmap);
   }
   
   if (flags & ALLEGRO_STATE_BLENDER) {
      _STORE(blend_source);
      _STORE(blend_dest);
      _STORE(blend_color);
      _STORE(memory_blender);
   }
};
#undef _STORE



#define _STORE(x) tls->x = stored->x;
/* Function: al_restore_state
 * 
 * Restores part of the state of the current thread from the given
 * <ALLEGRO_STATE> object.
 */
void al_restore_state(ALLEGRO_STATE const *state)
{
   thread_local_state *tls;
   int flags;
   if ((tls = tls_get()) == NULL)
      return;
   flags = state->flags;
   
   thread_local_state *stored = (void *)&state->_tls;

   if (flags & ALLEGRO_STATE_NEW_DISPLAY_PARAMETERS) {
      _STORE(new_display_format);
      _STORE(new_display_refresh_rate);
      _STORE(new_display_flags);
   }
   
   if (flags & ALLEGRO_STATE_NEW_BITMAP_PARAMETERS) {
      _STORE(new_bitmap_format);
      _STORE(new_bitmap_flags);
   }
   
   if (flags & ALLEGRO_STATE_DISPLAY) {
      _STORE(current_display);
   }
   
   if (flags & ALLEGRO_STATE_TARGET_BITMAP) {
      _STORE(target_bitmap);
   }
   
   if (flags & ALLEGRO_STATE_BLENDER) {
      _STORE(blend_source);
      _STORE(blend_dest);
      _STORE(blend_color);
      _STORE(memory_blender);
   }
};
#undef _STORE



/* Function: al_set_blender
 *
 * Sets the function to use for blending for the current thread.
 *
 * Blending means, the source and destination colors are combined in drawing
 * operations.
 *
 * Assume, the source color (e.g. color of a rectangle to draw, or pixel of a
 * bitmap to draw), is given as its red/green/blue/alpha components:
 * sr, sg, sb, sa
 * And this color is drawn to a destination, which already has a color:
 * dr, dg, db, da
 *
 * The conceptional formula used by Allegro to draw any pixel then is:
 *
 * > r = dr * dst + sr * src * color.r
 * > g = dg * dst + sg * src * color.g
 * > b = db * dst + sb * src * color.b
 * > a = da * dst + sa * src * color.a
 *
 * The src, dst and color values are specified by this function.
 * Valid values for src and dst are:
 *
 * ALLEGRO_ZERO - src = 0 / dst = 0
 * ALLEGRO_ONE - src = 1 / dst = 1
 * ALLEGRO_ALPHA - src = sa / dst = sa
 * ALLEGRO_INVERSE_ALPHA - src = (1 - sa) / dst = (1 - sa)
 *
 * So for example, to restore the default of using alpha blending, you would
 * use (pseudo code):
 * > al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, {1, 1, 1, 1})
 * This means, the color is not modified, all source components are
 * multipled with the alpha component and combined with the destination
 * according to this same alpha value.
 *
 * If you want to tint the result to some specific color, you can also
 * change the color from white to some other color.
 *
 * Additive blending would be achieved with:
 * > al_set_blender(ALLEGRO_ONE, ALLEGRO_ONE, {1, 1, 1, 1})
 *
 * Copying the source to the destination (including alpha) unmodified:
 * > al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, {1, 1, 1, 1})
 *
 */
void al_set_blender(int src, int dst, ALLEGRO_COLOR color)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;

   tls->blend_source = src;
   tls->blend_dest = dst;

   memcpy(&tls->blend_color, &color, sizeof(ALLEGRO_COLOR));

   _al_set_memory_blender(src, dst, &color);
}



/* Function: al_get_blender
 *
 * Returns the active blender for the current thread.
 */
void al_get_blender(int *src, int *dst, ALLEGRO_COLOR *color)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;

   if (src)
      *src = tls->blend_source;

   if (dst)
      *dst = tls->blend_dest;

   if (color)
      memcpy(color, &tls->blend_color, sizeof(ALLEGRO_COLOR));
}



ALLEGRO_COLOR *_al_get_blend_color()
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return NULL;
   return &tls->blend_color;
}



void _al_set_memory_blender(int src, int dst, ALLEGRO_COLOR *color)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;

   switch (src) {

      case ALLEGRO_ZERO:
         switch (dst) {
            case ALLEGRO_ZERO:
               tls->memory_blender = _al_blender_zero_zero;
               break;
            case ALLEGRO_ONE:
               tls->memory_blender = _al_blender_zero_one;
               break;
            case ALLEGRO_ALPHA:
               tls->memory_blender = _al_blender_zero_alpha;
               break;
            case ALLEGRO_INVERSE_ALPHA:
               tls->memory_blender = _al_blender_zero_inverse_alpha;
               break;
         }
         break;

      case ALLEGRO_ONE:
         switch (dst) {
            case ALLEGRO_ZERO:
               tls->memory_blender = _al_blender_one_zero;
               break;
            case ALLEGRO_ONE:
               tls->memory_blender = _al_blender_one_one;
               break;
            case ALLEGRO_ALPHA:
               tls->memory_blender = _al_blender_one_alpha;
               break;
            case ALLEGRO_INVERSE_ALPHA:
               tls->memory_blender = _al_blender_one_inverse_alpha;
               break;
         }
         break;

      case ALLEGRO_ALPHA:
         switch (dst) {
            case ALLEGRO_ZERO:
               tls->memory_blender = _al_blender_alpha_zero;
               break;
            case ALLEGRO_ONE:
               tls->memory_blender = _al_blender_alpha_one;
               break;
            case ALLEGRO_ALPHA:
               tls->memory_blender = _al_blender_alpha_alpha;
               break;
            case ALLEGRO_INVERSE_ALPHA:
               tls->memory_blender = _al_blender_alpha_inverse_alpha;
               break;
         }
         break;

      case ALLEGRO_INVERSE_ALPHA:
         switch (dst) {
            case ALLEGRO_ZERO:
               tls->memory_blender = _al_blender_inverse_alpha_zero;
               break;
            case ALLEGRO_ONE:
               tls->memory_blender = _al_blender_inverse_alpha_one;
               break;
            case ALLEGRO_ALPHA:
               tls->memory_blender = _al_blender_inverse_alpha_alpha;
               break;
            case ALLEGRO_INVERSE_ALPHA:
               tls->memory_blender = _al_blender_inverse_alpha_inverse_alpha;
               break;
         }
         break;
   }
}



ALLEGRO_MEMORY_BLENDER _al_get_memory_blender()
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return NULL;
   return tls->memory_blender;
}



/* Function: al_get_errno
 *  Some Allegro functions will set an error number as well as returning an
 *  error code.  Call this function to retrieve the last error number set
 *  for the calling thread.
 */
int al_get_errno(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->allegro_errno;
}



/* Function: al_get_errno
 *  Set the error number for for the calling thread.
 */
void al_set_errno(int errnum)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->allegro_errno = errnum;
}
