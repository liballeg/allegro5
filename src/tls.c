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

/* File: Thread local storage
 */


#include <string.h>
#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/internal/aintern_bitmap.h"
#include "allegro/internal/aintern_color.h"



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
   ALLEGRO_INDEPENDANT_COLOR blend_color;
   ALLEGRO_COLOR orig_blend_color;
   ALLEGRO_MEMORY_BLENDER memory_blender;
} thread_local_state;



#if defined FOO && (__GNUC__ < 4 || __GNUC_MINOR__ < 2 \
   || __GNUC_PATCHLEVEL__ < 1)

/*
 * MinGW doesn't have builtin thread local storage, so we
 * must use the Windows API. This means that the MinGW
 * build must be built as a DLL.
 */

#include "winalleg.h"

static DWORD tls_index;
static thread_local_state *tls;



static thread_local_state *tls_get(void)
{
   thread_local_state *t = TlsGetValue(tls_index);
   return t;
}



/* Function: al_set_new_display_format
 *
 * Set the pixel format for al displays created after this call.
 *
 * Parameters:
 *    format - The format to use
 */
void al_set_new_display_format(int format)
{
   if ((tls = tls_get()) == NULL)
      return;
   tls->new_display_format = format;
}



/* Function: al_set_new_display_refresh_rate
 *
 * Sets the refresh rate to use for newly created displays. If the refresh rate
 * is not available, al_create_display will fail. A list of modes with refresh
 * rates can be found with al_get_num_display_modes and al_get_display_mode,
 * documented above.
 *
 * See Also:
 *    <al_get_display_mode>
 */
void al_set_new_display_refresh_rate(int refresh_rate)
{
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
 * 0 can be used for default values. 
 */
void al_set_new_display_flags(int flags)
{
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
   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->new_display_format;
}



/* Function al_get_new_display_refresh_rate
 *
 * Gets the current refresh rate used for newly created displays.
 */
int al_get_new_display_refresh_rate(void)
{
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
   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->new_display_flags;
}



/* Function: al_set_current_display
 *
 * Change the current display for the calling thread.
 * Also sets the target bitmap to the display's
 * backbuffer.
 */
void al_set_current_display(ALLEGRO_DISPLAY *display)
{
   if (display)
      display->vt->set_current_display(display);

   if ((tls = tls_get()) == NULL)
      return;
   tls->current_display = display;

   if (display)
      al_set_target_bitmap(al_get_backbuffer());
}



/* Function: al_get_current_display
 *
 * Query for the current display in the calling thread.
 */
ALLEGRO_DISPLAY *al_get_current_display(void)
{
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
   if ((tls = tls_get()) == NULL)
      return;
   tls->target_bitmap = bitmap;

   if (!(bitmap->flags & ALLEGRO_MEMORY_BITMAP))
      _al_current_display->vt->set_target_bitmap(_al_current_display, bitmap);
}



/* Function: al_get_target_bitmap
 *
 * Return the target bitmap of the current display.
 */
ALLEGRO_BITMAP *al_get_target_bitmap(void)
{
   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->target_bitmap;
}



void _al_push_target_bitmap(void)
{
   if ((tls = tls_get()) == NULL)
      return;
   tls->target_bitmap_backup = tls->target_bitmap;
}



void _al_pop_target_bitmap(void)
{
   if ((tls = tls_get()) == NULL)
      return;
   tls->target_bitmap = tls->target_bitmap_backup;
   al_set_target_bitmap(tls->target_bitmap);
}



/* Function: al_set_new_bitmap_format
 *
 * Sets the pixel format for newly created bitmaps. format
 * is one of the same values as used for al_set_new_display_format.
 * The default format is 0 and means the display driver will choose
 * the best format.
 */
void al_set_new_bitmap_format(int format)
{
   if ((tls = tls_get()) == NULL)
      return;
   tls->new_bitmap_format = format;
}



/* Function: al_set_new_bitmap_flags
 *
 * Sets the flags to use for newly created bitmaps. Valid flags are:
 *
 * ALLEGRO_MEMORY_BITMAP - The bitmap will use a format most closely resembling
 * the format used in the bitmap file and al_create_memory_bitmap will be used
 * to create it. If this flag is not specified, al_create_bitmap will be used
 * instead and the display driver will determine the format.
 *
 * ALLEGRO_SYNC_MEMORY_COPY - When modifying the bitmap, always keep the memory
 * copy synchronized. This may mean copying back from the texture after
 * render-to-texture operations which is slow, or it may mean doing each
 * operation twice, once with render-to-texture, and once with a software
 * renderer on the memory copy. Some drivers also may always have to do
 * software rendering first then upload the modified parts, so this flag would
 * have no effect.
 *
 * ALLEGRO_USE_ALPHA - Use alpha blending when drawing the bitmap
 *
 * ALLEGRO_KEEP_BITMAP_FORMAT - Only used when loading bitmaps from disk files,
 * forces the resulting ALLEGRO_BITMAP to use the same format as the file. 
 */
void al_set_new_bitmap_flags(int flags)
{
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
   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->new_bitmap_flags;
}



void _al_push_new_bitmap_parameters(void)
{
   if ((tls = tls_get()) == NULL)
      return;
   tls->new_bitmap_format_backup = tls->new_bitmap_format;
   tls->new_bitmap_flags_backup = tls->new_bitmap_flags;
}



void _al_pop_new_bitmap_parameters(void)
{
   if ((tls = tls_get()) == NULL)
      return;
   tls->new_bitmap_format = tls->new_bitmap_format_backup;
   tls->new_bitmap_flags = tls->new_bitmap_flags_backup;
}



/* Function: al_set_blender
 *
 * Sets the function to use for blending for the current thread.
 * Valid values for src and dst are:
 *
 * > ALLEGRO_ZERO
 * > ALLEGRO_ONE
 * > ALLEGRO_ALPHA
 * > ALLEGRO_INVERSE_ALPHA
 */
void al_set_blender(int src, int dst, ALLEGRO_COLOR *color)
{
   if ((tls = tls_get()) == NULL)
      return;

   tls->blend_source = src;
   tls->blend_dest = dst;

   al_unmap_rgba_f(color,
      &tls->blend_color.r,
      &tls->blend_color.g,
      &tls->blend_color.b,
      &tls->blend_color.a);

   memcpy(&tls->orig_blend_color, color, sizeof(ALLEGRO_COLOR));

   _al_set_memory_blender(src, dst, color);
}



/* Function: al_get_blender
 *
 * Returns the active blender for the current thread.
 */
void al_get_blender(int *src, int *dst, ALLEGRO_COLOR *color)
{
   if ((tls = tls_get()) == NULL)
      return;

   if (src)
      *src = tls->blend_source;

   if (dst)
      *dst = tls->blend_dest;

   if (color)
      memcpy(color, &tls->orig_blend_color, sizeof(ALLEGRO_COLOR));
}



ALLEGRO_INDEPENDANT_COLOR *_al_get_blend_color()
{
   if ((tls = tls_get()) == NULL)
      return NULL;
   return &tls->blend_color;
}



void _al_set_memory_blender(int src, int dst, ALLEGRO_COLOR *color)
{
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
   if ((tls = tls_get()) == NULL)
      return NULL;
   return tls->memory_blender;
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

#if defined ALLEGRO_MSVC
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL __thread
#endif



static THREAD_LOCAL thread_local_state tls = {
   0,
   0,
   0,
   NULL,
   NULL,
   NULL,
   0,
   0,
   0,
   0,
   ALLEGRO_ALPHA,
   ALLEGRO_INVERSE_ALPHA,
   { 1.0f, 1.0f, 1.0f, 1.0f },
   {.raw = {0, 0, 0, 0}},
   _al_blender_alpha_inverse_alpha
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



/* al_set_current_display:
 *
 * See documentation for previous definition of this function.
 */
void al_set_current_display(ALLEGRO_DISPLAY *display)
{
   if (display)
      display->vt->set_current_display(display);
   tls.current_display = display;

   if (display)
      al_set_target_bitmap(al_get_backbuffer());
}



/* al_get_current_display:
 *
 * See documentation for previous definition of this function.
 */
ALLEGRO_DISPLAY *al_get_current_display(void)
{
   return tls.current_display;
}



/* al_set_target_bitmap:
 *
 * See documentation for previous definition of this function.
 */
void al_set_target_bitmap(ALLEGRO_BITMAP *bitmap)
{
   tls.target_bitmap = bitmap;
   if (!(bitmap->flags & ALLEGRO_MEMORY_BITMAP))
      _al_current_display->vt->set_target_bitmap(_al_current_display, bitmap);
}



/* al_get_target_bitmap:
 *
 * See documentation for previous definition of this function.
 */
ALLEGRO_BITMAP *al_get_target_bitmap(void)
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



void _al_push_new_bitmap_parameters(void)
{
   tls.new_bitmap_format_backup = tls.new_bitmap_format;
   tls.new_bitmap_flags_backup = tls.new_bitmap_flags;
}



void _al_pop_new_bitmap_parameters(void)
{
   tls.new_bitmap_format = tls.new_bitmap_format_backup;
   tls.new_bitmap_flags = tls.new_bitmap_flags_backup;
}



void al_set_blender(int src, int dst, ALLEGRO_COLOR *color)
{
   tls.blend_source = src;
   tls.blend_dest = dst;

   al_unmap_rgba_f(color,
      &tls.blend_color.r,
      &tls.blend_color.g,
      &tls.blend_color.b,
      &tls.blend_color.a);

   memcpy(&tls.orig_blend_color, color, sizeof(ALLEGRO_COLOR));

   _al_set_memory_blender(src, dst, color);
}



void al_get_blender(int *src, int *dst, ALLEGRO_COLOR *color)
{
   if (src)
     *src = tls.blend_source;

   if (dst)
      *dst = tls.blend_dest;

   if (color)
     memcpy(color, &tls.orig_blend_color, sizeof(ALLEGRO_COLOR));
}



ALLEGRO_INDEPENDANT_COLOR *_al_get_blend_color()
{
   return &tls.blend_color;
}



void _al_set_memory_blender(int src, int dst, ALLEGRO_COLOR *color)
{
   switch (src) {

      case ALLEGRO_ZERO:
         switch (dst) {
            case ALLEGRO_ZERO:
               tls.memory_blender = _al_blender_zero_zero;
               break;
            case ALLEGRO_ONE:
               tls.memory_blender = _al_blender_zero_one;
               break;
            case ALLEGRO_ALPHA:
               tls.memory_blender = _al_blender_zero_alpha;
               break;
            case ALLEGRO_INVERSE_ALPHA:
               tls.memory_blender = _al_blender_zero_inverse_alpha;
               break;
         }
         break;

      case ALLEGRO_ONE:
         switch (dst) {
            case ALLEGRO_ZERO:
               tls.memory_blender = _al_blender_one_zero;
               break;
            case ALLEGRO_ONE:
               tls.memory_blender = _al_blender_one_one;
               break;
            case ALLEGRO_ALPHA:
               tls.memory_blender = _al_blender_one_alpha;
               break;
            case ALLEGRO_INVERSE_ALPHA:
               tls.memory_blender = _al_blender_one_inverse_alpha;
               break;
         }
         break;

      case ALLEGRO_ALPHA:
         switch (dst) {
            case ALLEGRO_ZERO:
               tls.memory_blender = _al_blender_alpha_zero;
               break;
            case ALLEGRO_ONE:
               tls.memory_blender = _al_blender_alpha_one;
               break;
            case ALLEGRO_ALPHA:
               tls.memory_blender = _al_blender_alpha_alpha;
               break;
            case ALLEGRO_INVERSE_ALPHA:
               tls.memory_blender = _al_blender_alpha_inverse_alpha;
               break;
         }
         break;

      case ALLEGRO_INVERSE_ALPHA:
         switch (dst) {
            case ALLEGRO_ZERO:
               tls.memory_blender = _al_blender_inverse_alpha_zero;
               break;
            case ALLEGRO_ONE:
               tls.memory_blender = _al_blender_inverse_alpha_one;
               break;
            case ALLEGRO_ALPHA:
               tls.memory_blender = _al_blender_inverse_alpha_alpha;
               break;
            case ALLEGRO_INVERSE_ALPHA:
               tls.memory_blender = _al_blender_inverse_alpha_inverse_alpha;
               break;
         }
         break;
   }
}



ALLEGRO_MEMORY_BLENDER _al_get_memory_blender()
{
   return tls.memory_blender;
}

#endif /* not defined ALLEGRO_MINGW32 */ 
