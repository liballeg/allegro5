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
#include "allegro5/internal/aintern_file.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_fshook.h"


/* Thread local storage for graphics API state */
typedef struct thread_local_state {
   /* Display parameters */
   int new_display_refresh_rate;
   int new_display_flags;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS new_display_settings;
   /* Current display */
   ALLEGRO_DISPLAY *current_display;
   /* Target bitmap */
   ALLEGRO_BITMAP *target_bitmap;
   /* Bitmap parameters */
   int new_bitmap_format;
   int new_bitmap_flags;
   /* Blending modes and color */
   int blend_source;
   int blend_dest;
   int blend_alpha_source;
   int blend_alpha_dest;
   ALLEGRO_COLOR blend_color;
   //FIXME: Might need this again for optimization purposes.
   //ALLEGRO_MEMORY_BLENDER memory_blender;
   /* Files */
   const ALLEGRO_FILE_INTERFACE *new_file_interface;
   const ALLEGRO_FS_INTERFACE *fs_interface;
   /* Error code */
   int allegro_errno;
} thread_local_state;

/* The extra sizeof(int) is for flags. */
ALLEGRO_STATIC_ASSERT(
   sizeof(ALLEGRO_STATE) > sizeof(thread_local_state) + sizeof(int));


#if defined ALLEGRO_MINGW32 && ( \
   __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 2) || \
   (__GNUC__ == 4 && __GNUC_MINOR == 2 && __GNUC_PATCHLEVEL__ < 1))

/*
 * MinGW 3.x doesn't have builtin thread local storage, so we
 * must use the Windows API.
 */

#include <windows.h>


/* Forward declaration to bypass strict warning. */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);


static DWORD tls_index;


static thread_local_state *tls_get(void)
{
   thread_local_state *t = TlsGetValue(tls_index);
   return t;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{ 
   thread_local_state *data;

   (void)hinstDLL;
   (void)fdwReason;
   (void)lpvReserved;
 
   switch (fdwReason) { 
      case DLL_PROCESS_ATTACH: 
         if ((tls_index = TlsAlloc()) == TLS_OUT_OF_INDEXES) {
            return false;
	 }
            // No break: Initialize the index for first thread.
            // The attached process creates a new thread. 

      case DLL_THREAD_ATTACH: 
          // Initialize the TLS index for this thread.
          data = _AL_MALLOC(sizeof(*data));
          if (data != NULL) {
             memset(data, 0, sizeof(*data));

             data->new_bitmap_format = ALLEGRO_PIXEL_FORMAT_ANY;
             data->blend_source = ALLEGRO_ALPHA;
             data->blend_dest = ALLEGRO_INVERSE_ALPHA;
             data->blend_alpha_source = ALLEGRO_ONE;
             data->blend_alpha_dest = ALLEGRO_ONE;
             data->blend_color.r = data->blend_color.g = data->blend_color.b
                = data->blend_color.a = 1.0f;
             //data->memory_blender = _al_blender_alpha_inverse_alpha;
             TlsSetValue(tls_index, data);
             _al_fill_display_settings(&data->new_display_settings);
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


void _al_osx_threads_init(void)
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
      _al_fill_display_settings(&ptr->new_display_settings);
   }
   return ptr;
}

#else /* not MSVC/BCC32, not OSX */

#define THREAD_LOCAL __thread
#define HAVE_NATIVE_TLS

#endif /* end not MSVC/BCC32, not OSX */


static THREAD_LOCAL thread_local_state _tls = {
   0,                                     /* new_display_refresh_rate */
   0,                                     /* new_display_flags */
   {0, 0, { 0 },
    NULL, 0, 0},                          /* new_display_settings */
   NULL,                                  /* current_display */
   NULL,                                  /* target_bitmap */
   ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA,   /* new_bitmap_format */
   0,                                     /* new_bitmap_flags */
   ALLEGRO_ALPHA,                         /* blend_source */
   ALLEGRO_INVERSE_ALPHA,                 /* blend_dest */
   ALLEGRO_ONE,                           /* blend_alpha_source */
   ALLEGRO_ONE,                           /* blend_alpha_dest */
   { 1.0f, 1.0f, 1.0f, 1.0f },            /* blend_color  */
   //_al_blender_alpha_inverse_alpha,       /* memory_blender */
   &_al_file_interface_stdio,             /* file_interface */
   &_al_fs_interface_stdio,               /* fs_interface */
   0                                      /* errno */
};


#ifdef HAVE_NATIVE_TLS
static thread_local_state *tls_get(void)
{
   static THREAD_LOCAL thread_local_state *ptr = NULL;
   if (!ptr) {
      ptr = &_tls;
      _al_fill_display_settings(&ptr->new_display_settings);
   }
   return ptr;
}
#endif /* end HAVE_NATIVE_TLS */


#endif /* end not defined ALLEGRO_MINGW32 */



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


/* Function: al_set_new_display_refresh_rate
 */
void al_set_new_display_refresh_rate(int refresh_rate)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_display_refresh_rate = refresh_rate;
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


/* Function: al_get_new_display_refresh_rate
 */
int al_get_new_display_refresh_rate(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->new_display_refresh_rate;
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



/* Function: al_set_current_display
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
 */
ALLEGRO_DISPLAY *al_get_current_display(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->current_display;
}



/* Function: al_set_target_bitmap
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
 */
ALLEGRO_BITMAP *al_get_target_bitmap(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->target_bitmap;
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



#define _STORE(x) stored->x = tls->x;
/* Function: al_store_state
 */
void al_store_state(ALLEGRO_STATE *state, int flags)
{
   thread_local_state *tls;
   thread_local_state *stored;

   if ((tls = tls_get()) == NULL)
      return;
      
   state->flags = flags;
   
   stored = (void *)&state->_tls;

   if (flags & ALLEGRO_STATE_NEW_DISPLAY_PARAMETERS) {
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
      _STORE(blend_alpha_source);
      _STORE(blend_alpha_dest);
      _STORE(blend_color);
      //_STORE(memory_blender);
   }

   if (flags & ALLEGRO_STATE_NEW_FILE_INTERFACE) {
      _STORE(new_file_interface);
      _STORE(fs_interface);
   }
}
#undef _STORE



#define _STORE(x) tls->x = stored->x;
/* Function: al_restore_state
 */
void al_restore_state(ALLEGRO_STATE const *state)
{
   thread_local_state *tls;
   thread_local_state *stored;
   int flags;

   if ((tls = tls_get()) == NULL)
      return;
   flags = state->flags;
   
   stored = (void *)&state->_tls;

   if (flags & ALLEGRO_STATE_NEW_DISPLAY_PARAMETERS) {
      _STORE(new_display_refresh_rate);
      _STORE(new_display_flags);
   }
   
   if (flags & ALLEGRO_STATE_NEW_BITMAP_PARAMETERS) {
      _STORE(new_bitmap_format);
      _STORE(new_bitmap_flags);
   }
   
   if (flags & ALLEGRO_STATE_DISPLAY) {
      _STORE(current_display);
      al_set_current_display(tls->current_display);
   }
   
   if (flags & ALLEGRO_STATE_TARGET_BITMAP) {
      _STORE(target_bitmap);
      al_set_target_bitmap(tls->target_bitmap);
   }
   
   if (flags & ALLEGRO_STATE_BLENDER) {
      _STORE(blend_source);
      _STORE(blend_dest);
      _STORE(blend_alpha_source);
      _STORE(blend_alpha_dest);
      _STORE(blend_color);
      //_STORE(memory_blender);
   }

   if (flags & ALLEGRO_STATE_NEW_FILE_INTERFACE) {
      _STORE(new_file_interface);
      _STORE(fs_interface);
   }
}
#undef _STORE



/* Function: al_set_blender
 */
void al_set_blender(int src, int dst, ALLEGRO_COLOR color)
{
   al_set_separate_blender(src, dst, src, dst, color);
}



/* Function: al_set_separate_blender
 */
void al_set_separate_blender(int src, int dst, int alpha_src,
   int alpha_dst, ALLEGRO_COLOR color)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;

   tls->blend_source = src;
   tls->blend_dest = dst;
   tls->blend_alpha_source = alpha_src;
   tls->blend_alpha_dest = alpha_dst;

   memcpy(&tls->blend_color, &color, sizeof(ALLEGRO_COLOR));

   //_al_set_memory_blender(src, dst, alpha_src, alpha_dst, &color);
}



/* Function: al_get_blender
 */
void al_get_blender(int *src, int *dst, ALLEGRO_COLOR *color)
{
   al_get_separate_blender(src, dst, NULL, NULL, color);
}



/* Function: al_get_separate_blender
 */
void al_get_separate_blender(int *src, int *dst, int *alpha_src,
   int *alpha_dst, ALLEGRO_COLOR *color)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;

   if (src)
      *src = tls->blend_source;

   if (dst)
      *dst = tls->blend_dest;

   if (alpha_src)
      *alpha_src = tls->blend_alpha_source;

   if (alpha_dst)
      *alpha_dst = tls->blend_alpha_dest;

   if (color)
      memcpy(color, &tls->blend_color, sizeof(ALLEGRO_COLOR));
}



ALLEGRO_COLOR *_al_get_blend_color(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return NULL;
   return &tls->blend_color;
}



// FIXME: Do we need this for optimization?
#if 0
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
#endif



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
    * pthread_getspecific() is called before osx_thread_init()...
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



/* Function: al_get_errno
 */
int al_get_errno(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->allegro_errno;
}



/* Function: al_set_errno
 */
void al_set_errno(int errnum)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->allegro_errno = errnum;
}


/* vim: set sts=3 sw=3 et: */
