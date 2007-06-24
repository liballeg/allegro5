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

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/internal/aintern_bitmap.h"

/* Thread local storage for graphics API state */
typedef struct thread_local_state {
   /* Display parameters */
   int new_display_format;
   int new_display_refresh_rate;
   int new_display_flags;
   /* Current display */
   AL_DISPLAY *current_display;
   /* Target bitmap */
   AL_BITMAP *target_bitmap;
   /* For pushing/popping target bitmap */
   AL_BITMAP *target_bitmap_backup;
   /* Bitmap parameters */
   int new_bitmap_format;
   int new_bitmap_flags;
   /* For pushing/popping bitmap parameters */
   int new_bitmap_format_backup;
   int new_bitmap_flags_backup;
   /* Mask color */
   AL_COLOR mask_color;
} thread_local_state;

#if defined ALLEGRO_MINGW32

/*
 * MinGW doesn't have builtin thread local storage, so we
 * must use the Windows API. This means that the MinGW
 * build must be built as a DLL.
 */

#include "winalleg.h"

static DWORD tls_index;
static thread_local_state *tls;
static AL_COLOR black = { { 0, 0, 0, 0 } };

static thread_local_state *tls_get(void)
{
   thread_local_state *t = TlsGetValue(tls_index);
   return t;
}

void al_set_new_display_format(int format)
{
   if ((tls = tls_get()) == NULL) return;
   tls->new_display_format = format;
}

void al_set_new_display_refresh_rate(int refresh_rate)
{
   if ((tls = tls_get()) == NULL) return;
   tls->new_display_refresh_rate = refresh_rate;
}

void al_set_new_display_flags(int flags)
{
   if ((tls = tls_get()) == NULL) return;
   tls->new_display_flags = flags;
}

int al_get_new_display_format(void)
{
   if ((tls = tls_get()) == NULL) return 0;
   return tls->new_display_format;
}

int al_get_new_display_refresh_rate(void)
{
   if ((tls = tls_get()) == NULL) return 0;
   return tls->new_display_refresh_rate;
}

int al_get_new_display_flags(void)
{
   if ((tls = tls_get()) == NULL) return 0;
   return tls->new_display_flags;
}

/*
 * Make a display the current display. All the following Allegro commands in
 * the same thread will implicitly use this display from now on.
 */
void al_set_current_display(AL_DISPLAY *display)
{
   if (display)
      display->vt->set_current_display(display);
   if ((tls = tls_get()) == NULL) return;
   tls->current_display = display;
}

/*
 * Get the current display.
 */
AL_DISPLAY *al_get_current_display(void)
{
   if ((tls = tls_get()) == NULL) return 0;
   return tls->current_display;
}

/*
 * Select the bitmap to which all subsequent drawing operation
 * will draw.
 */
void al_set_target_bitmap(AL_BITMAP *bitmap)
{
   if ((tls = tls_get()) == NULL) return;
   tls->target_bitmap = bitmap;
   if (!(bitmap->flags & AL_MEMORY_BITMAP))
      _al_current_display->vt->set_target_bitmap(_al_current_display, bitmap);
}

/*
 * Retrieve the target for drawing operations.
 */
AL_BITMAP *al_get_target_bitmap(void)
{
   if ((tls = tls_get()) == NULL) return 0;
   return tls->target_bitmap;
}

void _al_push_target_bitmap(void)
{
   if ((tls = tls_get()) == NULL) return;
   tls->target_bitmap_backup = tls->target_bitmap;
}

void _al_pop_target_bitmap(void)
{
   if ((tls = tls_get()) == NULL) return;
   tls->target_bitmap = tls->target_bitmap_backup;
   al_set_target_bitmap(tls->target_bitmap);
}

void al_set_new_bitmap_format(int format)
{
   if ((tls = tls_get()) == NULL) return;
   tls->new_bitmap_format = format;
}

void al_set_new_bitmap_flags(int flags)
{
   if ((tls = tls_get()) == NULL) return;
   tls->new_bitmap_flags = flags;
}

int al_get_new_bitmap_format(void)
{
   if ((tls = tls_get()) == NULL) return 0;
   return tls->new_bitmap_format;
}

int al_get_new_bitmap_flags(void)
{
   if ((tls = tls_get()) == NULL) return 0;
   return tls->new_bitmap_flags;
}

void _al_push_bitmap_parameters()
{
   if ((tls = tls_get()) == NULL) return;
   tls->new_bitmap_format_backup = tls->new_bitmap_format;
   tls->new_bitmap_flags_backup = tls->new_bitmap_flags;
}

void _al_pop_bitmap_parameters()
{
   if ((tls = tls_get()) == NULL) return;
   tls->new_bitmap_format = tls->new_bitmap_format_backup;
   tls->new_bitmap_flags = tls->new_bitmap_flags_backup;
}

void al_set_mask_color(AL_COLOR *color)
{
   if ((tls = tls_get()) == NULL) return;
   memcpy(&tls->mask_color, color, sizeof(AL_COLOR));
}

AL_COLOR *al_get_mask_color(AL_COLOR *color)
{
   if ((tls = tls_get()) == NULL) return &black;
   memcpy(color, &tls->mask_color, sizeof(AL_COLOR));
   return color;
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
          memset(data, 0, sizeof(*data));
          if (data != NULL) 
             TlsSetValue(tls_index, data); 
 
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

#else

#if defined ALLEGRO_MSVC
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL __thread
#endif

static THREAD_LOCAL thread_local_state tls = {
   0,
   0,
   0
   NULL,
   NULL,
   NULL,
   0,
   0,
   0,
   0,
   { 0, 0, 0, 0 }
};

void al_set_new_display_format(int format)
{
   tls.new_display_format = format;
}

void al_set_new_display_refresh_rate(int refresh_rate)
{
   tls.new_display_refresh_rate = refresh_rate;
}

void al_set_new_display_flags(int flags)
{
   tls.new_display_flags = flags;
}

int al_get_new_display_format(void)
{
   return tls.new_display_format;
}

int al_get_new_display_refresh_rate(void)
{
   return tls.new_display_refresh_rate;
}

int al_get_new_display_flags(void)
{
   return tls.new_display_flags;
}

/*
 * Make a display the current display. All the following Allegro commands in
 * the same thread will implicitly use this display from now on.
 */
void al_set_current_display(AL_DISPLAY *display)
{
   if (display)
      display->vt->set_current_display(display);
   tls.current_display = display;
}

/*
 * Get the current display.
 */
AL_DISPLAY *al_get_current_display(void)
{
   return tls.current_display;
}

/*
 * Select the bitmap to which all subsequent drawing operation
 * will draw.
 */
void al_set_target_bitmap(AL_BITMAP *bitmap)
{
   tls.target_bitmap = bitmap;
   if (!(bitmap->flags & AL_MEMORY_BITMAP))
      _al_current_display->vt->set_target_bitmap(_al_current_display, bitmap);
}

/*
 * Retrieve the target for drawing operations.
 */
AL_BITMAP *al_get_target_bitmap(void)
{
   return tls.target_bitmap;
}

void _al_push_target_bitmap(void)
{
   tls.target_bitmap_backup = tls.target_bitmap;
}

void _al_pop_target_bitmap(void)
{
   tls.target_bitmap = tls.target_bitmap_backup;
   al_set_target_bitmap(tls.target_bitmap);
}

void al_set_new_bitmap_format(int format)
{
   tls.new_bitmap_format = format;
}

void al_set_new_bitmap_flags(int flags)
{
   tls.new_bitmap_flags = flags;
}

int al_get_new_bitmap_format(void)
{
   return tls.new_bitmap_format;
}

int al_get_new_bitmap_flags(void)
{
   return tls.new_bitmap_flags;
}

void _al_push_bitmap_parameters()
{
   tls.new_bitmap_format_backup = tls.new_bitmap_format;
   tls.new_bitmap_flags_backup = tls.new_bitmap_flags;
}

void _al_pop_bitmap_parameters()
{
   tls.new_bitmap_format = tls.new_bitmap_format_backup;
   tls.new_bitmap_flags = tls.new_bitmap_flags_backup;
}

void al_set_mask_color(AL_COLOR *color)
{
   memcpy(&tls.mask_color, color, sizeof(AL_COLOR));
}

AL_COLOR *al_get_mask_color(AL_COLOR *color)
{
   memcpy(color, &tls.mask_color, sizeof(AL_COLOR));
   return color;
}

#endif
