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
 *      New display driver.
 *
 *      By Elias Pschernig.
 *
 *      Modified by Trent Gamblin.
 *
 *      See readme.txt for copyright information.
 */

/* Title: Display routines
 */



#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_shader.h"
#include "allegro5/internal/aintern_system.h"


A5O_DEBUG_CHANNEL("display")


/* Function: al_create_display
 */
A5O_DISPLAY *al_create_display(int w, int h)
{
   A5O_SYSTEM *system;
   A5O_DISPLAY_INTERFACE *driver;
   A5O_DISPLAY *display;
   A5O_EXTRA_DISPLAY_SETTINGS *settings;
   int64_t flags;

   system = al_get_system_driver();
   driver = system->vt->get_display_driver();
   if (!driver) {
      A5O_ERROR("Failed to create display (no display driver)\n");
      return NULL;
   }

   display = driver->create_display(w, h);
   if (!display) {
      A5O_ERROR("Failed to create display (NULL)\n");
      return NULL;
   }

   ASSERT(display->vt);

   settings = &display->extra_settings;
   flags = settings->required | settings->suggested;
   if (!(flags & (1 << A5O_AUTO_CONVERT_BITMAPS))) {
      settings->settings[A5O_AUTO_CONVERT_BITMAPS] = 1;
   }
   settings->settings[A5O_DEFAULT_SHADER_PLATFORM] =
      _al_get_new_display_settings()->settings[A5O_DEFAULT_SHADER_PLATFORM];

   display->min_w = 0;
   display->min_h = 0;
   display->max_w = 0;
   display->max_h = 0;
   display->use_constraints = false;

   display->vertex_cache = 0;
   display->num_cache_vertices = 0;
   display->cache_enabled = false;
   display->vertex_cache_size = 0;
   display->cache_texture = 0;
   al_identity_transform(&display->projview_transform);

   display->default_shader = NULL;

   _al_vector_init(&display->display_invalidated_callbacks, sizeof(void *));
   _al_vector_init(&display->display_validated_callbacks, sizeof(void *));

   display->render_state.write_mask = A5O_MASK_RGBA | A5O_MASK_DEPTH;
   display->render_state.depth_test = false;
   display->render_state.depth_function = A5O_RENDER_LESS;
   display->render_state.alpha_test = false;
   display->render_state.alpha_function = A5O_RENDER_ALWAYS;
   display->render_state.alpha_test_value = 0;

   _al_vector_init(&display->bitmaps, sizeof(A5O_BITMAP*));

   if (settings->settings[A5O_COMPATIBLE_DISPLAY]) {
      al_set_target_bitmap(al_get_backbuffer(display));
   }
   else {
      A5O_DEBUG("A5O_COMPATIBLE_DISPLAY not set\n");
      _al_set_current_display_only(display);
   }

   if (display->flags & A5O_PROGRAMMABLE_PIPELINE) {
      display->default_shader = _al_create_default_shader(display);
      if (!display->default_shader) {
         al_destroy_display(display);
         return NULL;
      }
      al_use_shader(display->default_shader);
   }

   /* Clear the screen */
   if (settings->settings[A5O_COMPATIBLE_DISPLAY]) {
      al_clear_to_color(al_map_rgb(0, 0, 0));

      /* TODO:
       * on iphone, don't kill the initial splashscreen - in fact, it's also
       * annoying in linux to have an extra black frame as first frame and I
       * suppose we never really want it
       */
#if 0
      al_flip_display();
#endif
   }

   if (settings->settings[A5O_AUTO_CONVERT_BITMAPS]) {
      /* We convert video bitmaps to memory bitmaps when the display is
       * destroyed, so seems only fair to re-convertt hem when the
       * display is re-created again.
       */
      al_convert_memory_bitmaps();
   }

   return display;
}



/* Function: al_destroy_display
 */
void al_destroy_display(A5O_DISPLAY *display)
{
   if (display) {
      /* This causes warnings and potential errors on Android because
       * it clears the context and Android needs this thread to have
       * the context bound in its destroy function and to destroy the
       * shader. Just skip this part on Android.
       */
#ifndef A5O_ANDROID
      A5O_BITMAP *bmp;

      bmp = al_get_target_bitmap();
      if (bmp && _al_get_bitmap_display(bmp) == display)
         al_set_target_bitmap(NULL);

      /* This can happen if we have a current display, but the target bitmap
       * was a memory bitmap.
       */
      if (display == al_get_current_display())
         _al_set_current_display_only(NULL);
#endif

      al_destroy_shader(display->default_shader);
      display->default_shader = NULL;

      ASSERT(display->vt);
      display->vt->destroy_display(display);
   }
}



/* Function: al_get_backbuffer
 */
A5O_BITMAP *al_get_backbuffer(A5O_DISPLAY *display)
{
   if (display) {
      ASSERT(display->vt);
      return display->vt->get_backbuffer(display);
   }
   return NULL;
}



/* Function: al_flip_display
 */
void al_flip_display(void)
{
   A5O_DISPLAY *display = al_get_current_display();

   if (display) {
      ASSERT(display->vt);
      display->vt->flip_display(display);
   }
}



/* Function: al_update_display_region
 */
void al_update_display_region(int x, int y, int width, int height)
{
   A5O_DISPLAY *display = al_get_current_display();

   if (display) {
      ASSERT(display->vt);
      display->vt->update_display_region(display, x, y, width, height);
   }
}



/* Function: al_acknowledge_resize
 */
bool al_acknowledge_resize(A5O_DISPLAY *display)
{
   ASSERT(display);
   ASSERT(display->vt);

   if (!(display->flags & A5O_FULLSCREEN)) {
      if (display->vt->acknowledge_resize) {
         return display->vt->acknowledge_resize(display);
      }
   }
   return false;
}



/* Function: al_resize_display
 */
bool al_resize_display(A5O_DISPLAY *display, int width, int height)
{
   ASSERT(display);
   ASSERT(display->vt);

   A5O_INFO("Requested display resize %dx%d\n", width, height);

   if (display->vt->resize_display) {
      return display->vt->resize_display(display, width, height);
   }
   return false;
}



/* Function: al_is_compatible_bitmap
 */
bool al_is_compatible_bitmap(A5O_BITMAP *bitmap)
{
   A5O_DISPLAY *display = al_get_current_display();
   ASSERT(bitmap);

   if (display) {
      ASSERT(display->vt);
      return display->vt->is_compatible_bitmap(display, bitmap);
   }

   return false;
}



/* Function: al_get_display_width
 */
int al_get_display_width(A5O_DISPLAY *display)
{
   ASSERT(display);

   return display->w;
}



/* Function: al_get_display_height
 */
int al_get_display_height(A5O_DISPLAY *display)
{
   ASSERT(display);

   return display->h;
}


/* Function: al_get_display_format
 */
int al_get_display_format(A5O_DISPLAY *display)
{
   ASSERT(display);

   return display->backbuffer_format;
}


/* Function: al_get_display_refresh_rate
 */
int al_get_display_refresh_rate(A5O_DISPLAY *display)
{
   ASSERT(display);

   return display->refresh_rate;
}



/* Function: al_get_display_flags
 */
int al_get_display_flags(A5O_DISPLAY *display)
{
   ASSERT(display);

   return display->flags;
}


/* Function: al_get_display_orientation
 */
int al_get_display_orientation(A5O_DISPLAY* display)
{
   if (display && display->vt->get_orientation)
      return display->vt->get_orientation(display);
   else
      return A5O_DISPLAY_ORIENTATION_UNKNOWN;
}


/* Function: al_wait_for_vsync
 */
bool al_wait_for_vsync(void)
{
   A5O_DISPLAY *display = al_get_current_display();
   ASSERT(display);

   if (display->vt->wait_for_vsync)
      return display->vt->wait_for_vsync(display);
   else
      return false;
}



/* Function: al_set_display_icon
 */
void al_set_display_icon(A5O_DISPLAY *display, A5O_BITMAP *icon)
{
   A5O_BITMAP *icons[1] = { icon };

   al_set_display_icons(display, 1, icons);
}



/* Function: al_set_display_icons
 */
void al_set_display_icons(A5O_DISPLAY *display,
   int num_icons, A5O_BITMAP *icons[])
{
   int i;

   ASSERT(display);
   ASSERT(num_icons >= 1);
   ASSERT(icons);
   for (i = 0; i < num_icons; i++) {
      ASSERT(icons[i]);
   }

   if (display->vt->set_icons) {
      display->vt->set_icons(display, num_icons, icons);
   }
}

/* Function: al_set_window_position
 */
void al_set_window_position(A5O_DISPLAY *display, int x, int y)
{
   ASSERT(display);

   if (display && display->flags & A5O_FULLSCREEN) {
      return;
   }

   if (display && display->vt && display->vt->set_window_position) {
      display->vt->set_window_position(display, x, y);
   }
}


/* Function: al_get_window_position
 */
void al_get_window_position(A5O_DISPLAY *display, int *x, int *y)
{
   ASSERT(x);
   ASSERT(y);

   if (display && display->vt && display->vt->get_window_position) {
      display->vt->get_window_position(display, x, y);
   }
   else {
      *x = *y = -1;
   }
}


/* Function: al_get_window_borders
 */
bool al_get_window_borders(A5O_DISPLAY *display, int *left, int *top, int *right, int *bottom)
{
   if (display && display->vt && display->vt->get_window_borders) {
      return display->vt->get_window_borders(display, left, top, right, bottom);
   }
   else {
      return false;
   }
}


/* Function: al_set_window_constraints
 */
bool al_set_window_constraints(A5O_DISPLAY *display,
   int min_w, int min_h, int max_w, int max_h)
{
   ASSERT(display);

  /* Perform some basic checks. */
   if (min_w < 0 || min_h < 0 || max_w < 0 || max_h < 0) {
      return false;
   }
   if (min_w > 0 && max_w > 0 && max_w < min_w) {
      return false;
   }
   if (min_h > 0 && max_h > 0 && max_h < min_h) {
      return false;
   }

   /* Cannot constrain when fullscreen. */
   if (display->flags & A5O_FULLSCREEN) {
      return false;
   }

   /* Cannot constrain if not resizable. */
   if (!(display->flags & A5O_RESIZABLE)) {
      return false;
   }

   if (display && display->vt && display->vt->set_window_constraints) {
      return display->vt->set_window_constraints(display, min_w, min_h,
         max_w, max_h);
   }
   else {
      return false;
   }
}


/* Function: al_get_window_constraints
 */
bool al_get_window_constraints(A5O_DISPLAY *display,
   int *min_w, int *min_h, int *max_w, int *max_h)
{
   ASSERT(display);
   ASSERT(min_w);
   ASSERT(min_h);
   ASSERT(max_w);
   ASSERT(max_h);

   if (display && display->vt && display->vt->get_window_constraints) {
      return display->vt->get_window_constraints(display, min_w, min_h,
         max_w, max_h);
   }
   else {
      return false;
   }
}


/* Function: al_set_display_flag
 */
bool al_set_display_flag(A5O_DISPLAY *display, int flag, bool onoff)
{
   ASSERT(display);

   if (display && display->vt && display->vt->set_display_flag) {
      return display->vt->set_display_flag(display, flag, onoff);
   }
   return false;
}


/* Function: al_set_window_title
 */
void al_set_window_title(A5O_DISPLAY *display, const char *title)
{
   if (display && display->vt && display->vt->set_window_title)
      display->vt->set_window_title(display, title);
}


/* Function: al_get_display_event_source
 */
A5O_EVENT_SOURCE *al_get_display_event_source(A5O_DISPLAY *display)
{
   return &display->es;
}

/* Function: al_hold_bitmap_drawing
 */
void al_hold_bitmap_drawing(bool hold)
{
   A5O_DISPLAY *current_display = al_get_current_display();

   if (current_display) {
      if (hold && !current_display->cache_enabled) {
         /*
          * Set the hardware transformation to identity, but keep the bitmap
          * transform the same as it was. Relies on the fact that when bitmap
          * holding is turned on, al_use_transform does not update the hardware
          * transformation.
          */
         A5O_TRANSFORM old, ident;
         al_copy_transform(&old, al_get_current_transform());
         al_identity_transform(&ident);

         al_use_transform(&ident);
         current_display->cache_enabled = hold;
         al_use_transform(&old);
      }
      else {
         current_display->cache_enabled = hold;
      }

      if (!hold) {
         current_display->vt->flush_vertex_cache(current_display);
         /*
          * Reset the hardware transform to match the stored transform.
          */
         al_use_transform(al_get_current_transform());
      }
   }
}

/* Function: al_is_bitmap_drawing_held
 */
bool al_is_bitmap_drawing_held(void)
{
   A5O_DISPLAY *current_display = al_get_current_display();

   if (current_display)
      return current_display->cache_enabled;
   else
      return false;
}

void _al_add_display_invalidated_callback(A5O_DISPLAY* display, void (*display_invalidated)(A5O_DISPLAY*))
{
   if (_al_vector_find(&display->display_invalidated_callbacks, display_invalidated) >= 0) {
      return;
   }
   else {
      void (**callback)(A5O_DISPLAY *) = _al_vector_alloc_back(&display->display_invalidated_callbacks);
      *callback = display_invalidated;
   }
}

void _al_add_display_validated_callback(A5O_DISPLAY* display, void (*display_validated)(A5O_DISPLAY*))
{
   if (_al_vector_find(&display->display_validated_callbacks, display_validated) >= 0) {
      return;
   }
   else {
      void (**callback)(A5O_DISPLAY *) = _al_vector_alloc_back(&display->display_validated_callbacks);
      *callback = display_validated;
   }
}

void _al_remove_display_invalidated_callback(A5O_DISPLAY *display, void (*callback)(A5O_DISPLAY *))
{
   _al_vector_find_and_delete(&display->display_invalidated_callbacks, &callback);
}

void _al_remove_display_validated_callback(A5O_DISPLAY *display, void (*callback)(A5O_DISPLAY *))
{
   _al_vector_find_and_delete(&display->display_validated_callbacks, &callback);
}

/* Function: al_acknowledge_drawing_halt
 */
void al_acknowledge_drawing_halt(A5O_DISPLAY *display)
{
   if (display->vt->acknowledge_drawing_halt) {
      display->vt->acknowledge_drawing_halt(display);
   }
}

/* Function: al_acknowledge_drawing_resume
 */
void al_acknowledge_drawing_resume(A5O_DISPLAY *display)
{
   if (display->vt->acknowledge_drawing_resume) {
      display->vt->acknowledge_drawing_resume(display);
   }
}

/* Function: al_set_render_state
 */
void al_set_render_state(A5O_RENDER_STATE state, int value)
{
   A5O_DISPLAY *display = al_get_current_display();

   if (!display)
      return;

   switch (state) {
      case A5O_ALPHA_TEST:
         display->render_state.alpha_test = value;
         break;
      case A5O_WRITE_MASK:
         display->render_state.write_mask = value;
         break;
      case A5O_DEPTH_TEST:
         display->render_state.depth_test = value;
         break;
      case A5O_DEPTH_FUNCTION:
         display->render_state.depth_function = value;
         break;
      case A5O_ALPHA_FUNCTION:
         display->render_state.alpha_function = value;
         break;
      case A5O_ALPHA_TEST_VALUE:
         display->render_state.alpha_test_value = value;
         break;
      default:
         A5O_WARN("unknown state to change: %d\n", state);
         break;
   }

   if (display->vt && display->vt->update_render_state) {
      display->vt->update_render_state(display);
   }
}

/* Function: al_backup_dirty_bitmaps
 */
void al_backup_dirty_bitmaps(A5O_DISPLAY *display)
{
   unsigned int i;

   for (i = 0; i < display->bitmaps._size; i++) {
      A5O_BITMAP **bptr = (A5O_BITMAP **)_al_vector_ref(&display->bitmaps, i);
      A5O_BITMAP *bmp = *bptr;
      if (_al_get_bitmap_display(bmp) == display) {
         if (bmp->vt && bmp->vt->backup_dirty_bitmap) {
            bmp->vt->backup_dirty_bitmap(bmp);
	 }
      }
   }
}

/* Function: al_apply_window_constraints
 */
void al_apply_window_constraints(A5O_DISPLAY *display, bool onoff)
{
   display->use_constraints = onoff;

   if (display->vt && display->vt->apply_window_constraints)
      display->vt->apply_window_constraints(display, onoff);
}

/* vim: set sts=3 sw=3 et: */
