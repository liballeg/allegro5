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
 *      SDL display implementation.
 *
 *      See LICENSE.txt for copyright information.
 */
#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/system.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_vector.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/internal/aintern_shader.h"
#include "allegro5/platform/allegro_internal_sdl.h"

ALLEGRO_DEBUG_CHANNEL("display")

int _al_win_determine_adapter(void);

static ALLEGRO_DISPLAY_INTERFACE *vt;

float _al_sdl_get_display_pixel_ratio(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_SDL *sdl = (void *)display;
   int window_width, drawable_width, h;
   SDL_GetWindowSize(sdl->window, &window_width, &h);
   SDL_GL_GetDrawableSize(sdl->window, &drawable_width, &h);
   return drawable_width / (float)window_width;
}

ALLEGRO_DISPLAY *_al_sdl_find_display(uint32_t window_id) {
   unsigned int i;
   ALLEGRO_SYSTEM *s = al_get_system_driver();
   for (i = 0; i < _al_vector_size(&s->displays); i++) {
      void **v = (void **)_al_vector_ref(&s->displays, i);
      ALLEGRO_DISPLAY_SDL *d = *v;
      if (SDL_GetWindowID(d->window) == window_id) {
         return &d->display;
         break;
      }
   }
   return NULL;
}

void _al_sdl_display_event(SDL_Event *e)
{
   ALLEGRO_EVENT event;
   event.display.timestamp = al_get_time();

   ALLEGRO_DISPLAY *d = NULL;

   if (e->type == SDL_WINDOWEVENT) {
      d = _al_sdl_find_display(e->window.windowID);
      if (e->window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
         event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_IN;
      }
      if (e->window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
         event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_OUT;
      }
      if (e->window.event == SDL_WINDOWEVENT_CLOSE) {
         event.display.type = ALLEGRO_EVENT_DISPLAY_CLOSE;
      }
      if (e->window.event == SDL_WINDOWEVENT_RESIZED) {
         float ratio = _al_sdl_get_display_pixel_ratio(d);
         event.display.type = ALLEGRO_EVENT_DISPLAY_RESIZE;
         event.display.width = e->window.data1 * ratio;
         event.display.height = e->window.data2 * ratio;
      }
   }
   if (e->type == SDL_QUIT) {
      event.display.type = ALLEGRO_EVENT_DISPLAY_CLOSE;
      /* Use the first display as event source if we have any displays. */
      ALLEGRO_SYSTEM *s = al_get_system_driver();
      if (_al_vector_size(&s->displays) > 0) {
         void **v = (void **)_al_vector_ref(&s->displays, 0);
         d = *v;
      }
   }

   if (!d)
      return;
   ALLEGRO_EVENT_SOURCE *es = &d->es;
   _al_event_source_lock(es);
   _al_event_source_emit_event(es, &event);
   _al_event_source_unlock(es);
}

static void GLoption(int allegro, int sdl)
{
   int i;
   int x = al_get_new_display_option(allegro, &i);
   if (i == ALLEGRO_DONTCARE)
      return;
   SDL_GL_SetAttribute(sdl, x);
}

static ALLEGRO_DISPLAY *sdl_create_display_locked(int w, int h)
{
   ALLEGRO_DISPLAY_SDL *sdl = al_calloc(1, sizeof *sdl);
   ALLEGRO_DISPLAY *d = (void *)sdl;
   d->w = w;
   d->h = h;
   d->flags = al_get_new_display_flags();
   d->flags |= ALLEGRO_OPENGL;
#ifdef ALLEGRO_CFG_OPENGLES2
   d->flags |= ALLEGRO_PROGRAMMABLE_PIPELINE;
#endif
#ifdef ALLEGRO_CFG_OPENGLES
   d->flags |= ALLEGRO_OPENGL_ES_PROFILE;
#endif
   int flags = SDL_WINDOW_OPENGL;
   if (d->flags & ALLEGRO_FULLSCREEN)
      flags |= SDL_WINDOW_FULLSCREEN;
   if (d->flags & ALLEGRO_FULLSCREEN_WINDOW)
      flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
   if (d->flags & ALLEGRO_FRAMELESS)
      flags |= SDL_WINDOW_BORDERLESS;
   if (d->flags & ALLEGRO_RESIZABLE)
      flags |= SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

   if (d->flags & ALLEGRO_OPENGL_ES_PROFILE) {
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#ifdef ALLEGRO_CFG_OPENGLES1
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
#else
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
#endif
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
   }

   GLoption(ALLEGRO_COLOR_SIZE, SDL_GL_BUFFER_SIZE);
   GLoption(ALLEGRO_RED_SIZE, SDL_GL_RED_SIZE);
   GLoption(ALLEGRO_GREEN_SIZE, SDL_GL_GREEN_SIZE);
   GLoption(ALLEGRO_BLUE_SIZE, SDL_GL_BLUE_SIZE);
   GLoption(ALLEGRO_ALPHA_SIZE, SDL_GL_ALPHA_SIZE);
   GLoption(ALLEGRO_ACC_RED_SIZE, SDL_GL_ACCUM_RED_SIZE);
   GLoption(ALLEGRO_ACC_GREEN_SIZE, SDL_GL_ACCUM_GREEN_SIZE);
   GLoption(ALLEGRO_ACC_BLUE_SIZE, SDL_GL_ACCUM_BLUE_SIZE);
   GLoption(ALLEGRO_ACC_ALPHA_SIZE, SDL_GL_ACCUM_ALPHA_SIZE);
   GLoption(ALLEGRO_STEREO, SDL_GL_STEREO);
   GLoption(ALLEGRO_DEPTH_SIZE, SDL_GL_DEPTH_SIZE);
   GLoption(ALLEGRO_STENCIL_SIZE, SDL_GL_STENCIL_SIZE);
   GLoption(ALLEGRO_SAMPLE_BUFFERS, SDL_GL_MULTISAMPLEBUFFERS);
   GLoption(ALLEGRO_SAMPLES, SDL_GL_MULTISAMPLESAMPLES);
   GLoption(ALLEGRO_OPENGL_MAJOR_VERSION, SDL_GL_CONTEXT_MAJOR_VERSION);
   GLoption(ALLEGRO_OPENGL_MINOR_VERSION, SDL_GL_CONTEXT_MINOR_VERSION);

   SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

   sdl->window = SDL_CreateWindow(al_get_new_window_title(), sdl->x, sdl->y,
      d->w, d->h, flags);
   if (!sdl->window) {
      ALLEGRO_ERROR("SDL_CreateWindow failed: %s", SDL_GetError());
      return NULL;
   }

   sdl->context = SDL_GL_CreateContext(sdl->window);

   SDL_GL_GetDrawableSize(sdl->window, &d->w, &d->h);

   // there's no way to query pixel ratio before creating the window, so we
   // have to compensate afterwards
   if (d->flags & ALLEGRO_RESIZABLE &&
        !(d->flags & ALLEGRO_FULLSCREEN || d->flags & ALLEGRO_FULLSCREEN_WINDOW)) {
      int window_width, window_height;
      SDL_GetWindowSize(sdl->window, &window_width, &window_height);
      float ratio = _al_sdl_get_display_pixel_ratio(d);

      ALLEGRO_DEBUG("resizing the display to %dx%d to match the scaling factor %f\n", (int)(window_width / ratio), (int)(window_height / ratio), ratio);

      SDL_SetWindowSize(sdl->window, window_width / ratio, window_height / ratio);

      SDL_GL_GetDrawableSize(sdl->window, &d->w, &d->h);
   }

   ALLEGRO_DISPLAY **add;
   ALLEGRO_SYSTEM *system = al_get_system_driver();
   add = _al_vector_alloc_back(&system->displays);
   *add = d;

   _al_event_source_init(&d->es);
   d->vt = vt;

   d->extra_settings.settings[ALLEGRO_COMPATIBLE_DISPLAY] = true;

   d->ogl_extras = al_calloc(1, sizeof *d->ogl_extras);
   _al_ogl_manage_extensions(d);
   _al_ogl_set_extensions(d->ogl_extras->extension_api);

   _al_ogl_setup_gl(d);

   /* Fill in opengl version */
   const int v = d->ogl_extras->ogl_info.version;
   d->extra_settings.settings[ALLEGRO_OPENGL_MAJOR_VERSION] = (v >> 24) & 0xFF;
   d->extra_settings.settings[ALLEGRO_OPENGL_MINOR_VERSION] = (v >> 16) & 0xFF;

   return d;
}

static ALLEGRO_DISPLAY *sdl_create_display(int w, int h)
{
   ALLEGRO_SYSTEM_SDL *s = (void *)al_get_system_driver();
   al_lock_mutex(s->mutex);
   ALLEGRO_DISPLAY *d = sdl_create_display_locked(w, h);
   al_unlock_mutex(s->mutex);
   return d;
}

static void convert_display_bitmaps_to_memory_bitmap(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DEBUG("converting display bitmaps to memory bitmaps.\n");

   while (d->bitmaps._size > 0) {
      ALLEGRO_BITMAP **bptr = _al_vector_ref_back(&d->bitmaps);
      ALLEGRO_BITMAP *b = *bptr;
      _al_convert_to_memory_bitmap(b);
   }
}

static void transfer_display_bitmaps_to_any_other_display(
   ALLEGRO_SYSTEM *s, ALLEGRO_DISPLAY *d)
{
   size_t i;
   ALLEGRO_DISPLAY *living = NULL;
   ASSERT(s->displays._size > 1);

   for (i = 0; i < s->displays._size; i++) {
      ALLEGRO_DISPLAY **slot = _al_vector_ref(&s->displays, i);
      living = *slot;
      if (living != d)
         break;
   }

   ALLEGRO_DEBUG("transferring display bitmaps to other display.\n");

   for (i = 0; i < d->bitmaps._size; i++) {
      ALLEGRO_BITMAP **add = _al_vector_alloc_back(&(living->bitmaps));
      ALLEGRO_BITMAP **ref = _al_vector_ref(&d->bitmaps, i);
      *add = *ref;
      (*add)->_display = living;
   }
}

static void sdl_destroy_display_locked(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_SDL *sdl = (void *)d;
   ALLEGRO_SYSTEM *system = al_get_system_driver();
   ALLEGRO_OGL_EXTRAS *ogl = d->ogl_extras;
   bool is_last;

   ALLEGRO_DEBUG("destroying display.\n");

   /* If we're the last display, convert all bitmaps to display independent
    * (memory) bitmaps. Otherwise, pass all bitmaps to any other living
    * display. We assume all displays are compatible.)
    */

   is_last = (system->displays._size == 1);
   if (is_last)
      convert_display_bitmaps_to_memory_bitmap(d);
   else
      transfer_display_bitmaps_to_any_other_display(system, d);

   _al_ogl_unmanage_extensions(d);
   ALLEGRO_DEBUG("unmanaged extensions.\n");

   if (ogl->backbuffer) {
      _al_ogl_destroy_backbuffer(ogl->backbuffer);
      ogl->backbuffer = NULL;
      ALLEGRO_DEBUG("destroy backbuffer.\n");
   }

   _al_vector_free(&d->bitmaps);
   _al_event_source_free(&d->es);

   al_free(d->ogl_extras);
   al_free(d->vertex_cache);

   _al_event_source_free(&d->es);
   _al_vector_find_and_delete(&system->displays, &d);

   SDL_GL_DeleteContext(sdl->context);
   SDL_DestroyWindow(sdl->window);
   al_free(sdl);
}

static void sdl_destroy_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_SDL *s = (void *)al_get_system_driver();
   al_lock_mutex(s->mutex);
   sdl_destroy_display_locked(d);
   al_unlock_mutex(s->mutex);
}

static bool sdl_set_current_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_SDL *sdl = (void *)d;
   SDL_GL_MakeCurrent(sdl->window, sdl->context);
   return true;
}

static void sdl_unset_current_display(ALLEGRO_DISPLAY *d)
{
   (void)d;
}

static void sdl_flip_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_SDL *sdl = (void *)d;
   SDL_GL_SwapWindow(sdl->window);

   // SDL loses texture contents, for example on resize.
   al_backup_dirty_bitmaps(d);
}

static void sdl_update_display_region(ALLEGRO_DISPLAY *d, int x, int y,
   	int width, int height)
{
   ALLEGRO_DISPLAY_SDL *sdl = (void *)d;
   (void)x;
   (void)y;
   (void)width;
   (void)height;
   SDL_GL_SwapWindow(sdl->window);
}

static bool sdl_is_compatible_bitmap(ALLEGRO_DISPLAY *display,
      ALLEGRO_BITMAP *bitmap)
{
   (void)display;
   (void)bitmap;
   return true;
}

static bool sdl_set_mouse_cursor(ALLEGRO_DISPLAY *display,
      ALLEGRO_MOUSE_CURSOR *cursor)
{
   ALLEGRO_MOUSE_CURSOR_SDL *sdl_cursor = (ALLEGRO_MOUSE_CURSOR_SDL *) cursor;
   (void)display;
   SDL_SetCursor(sdl_cursor->cursor);
   return true;
}

static bool sdl_set_system_mouse_cursor(ALLEGRO_DISPLAY *display,
      ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id)
{
   (void)display;
   (void)cursor_id;
   return false;
}

static bool sdl_show_mouse_cursor(ALLEGRO_DISPLAY *display)
{
   (void)display;
   return SDL_ShowCursor(SDL_ENABLE) == SDL_ENABLE;
}

static bool sdl_hide_mouse_cursor(ALLEGRO_DISPLAY *display)
{
   (void)display;
   return SDL_ShowCursor(SDL_DISABLE) == SDL_DISABLE;
}

static void sdl_set_window_position(ALLEGRO_DISPLAY *display, int x, int y)
{
   ALLEGRO_DISPLAY_SDL *sdl = (void *)display;
   SDL_SetWindowPosition(sdl->window, x, y);
}

static void sdl_get_window_position(ALLEGRO_DISPLAY *display, int *x, int *y)
{
   ALLEGRO_DISPLAY_SDL *sdl = (void *)display;
   SDL_GetWindowPosition(sdl->window, x, y);
}

static void recreate_textures(ALLEGRO_DISPLAY *display)
{
   unsigned int i;
   for (i = 0; i < _al_vector_size(&display->bitmaps); i++) {
      ALLEGRO_BITMAP **bptr = _al_vector_ref(&display->bitmaps, i);
      ALLEGRO_BITMAP *bitmap = *bptr;
      int bitmap_flags = al_get_bitmap_flags(bitmap);
      if (bitmap->parent)
         continue;
      if (bitmap_flags & ALLEGRO_MEMORY_BITMAP)
         continue;
      if (bitmap_flags & ALLEGRO_NO_PRESERVE_TEXTURE)
         continue;
      _al_ogl_upload_bitmap_memory(bitmap, _al_get_bitmap_memory_format(
         bitmap), bitmap->memory);
   }
}

static bool sdl_acknowledge_resize(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_SDL *sdl = (void *)display;
   SDL_GL_GetDrawableSize(sdl->window, &display->w, &display->h);

   _al_ogl_setup_gl(display);

   if (display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
      display->default_shader = _al_create_default_shader(display);
      al_use_shader(display->default_shader);
   }

   recreate_textures(display);

   _al_glsl_unuse_shaders();

   return true;
}

static void sdl_set_window_title(ALLEGRO_DISPLAY *display, char const *title)
{
   ALLEGRO_DISPLAY_SDL *sdl = (void *)display;
   SDL_SetWindowTitle(sdl->window, title);
}

static bool sdl_resize_display(ALLEGRO_DISPLAY *display, int width, int height)
{
   ALLEGRO_DISPLAY_SDL *sdl = (void *)display;

   // Allegro uses pixels everywhere, while SDL uses screen space for window size
   int window_width, drawable_width, h;
   SDL_GetWindowSize(sdl->window, &window_width, &h);
   SDL_GL_GetDrawableSize(sdl->window, &drawable_width, &h);
   float ratio = drawable_width / (float)window_width;

   SDL_SetWindowSize(sdl->window, width / ratio, height / ratio);
   sdl_acknowledge_resize(display);
   return true;
}

static bool sdl_set_display_flag(ALLEGRO_DISPLAY *display, int flag,
   bool flag_onoff)
{
   ALLEGRO_DISPLAY_SDL *sdl = (void *)display;
   switch (flag) {
      case ALLEGRO_FRAMELESS:
         /* The ALLEGRO_FRAMELESS flag is backwards. */
         SDL_SetWindowBordered(sdl->window, !flag_onoff);
         return true;
      case ALLEGRO_FULLSCREEN_WINDOW:
         SDL_SetWindowFullscreen(sdl->window, flag_onoff ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
         return true;
      case ALLEGRO_MAXIMIZED:
          if (flag_onoff) {
             SDL_MaximizeWindow(sdl->window);
          } else {
             SDL_RestoreWindow(sdl->window);
          }
         return true;
   }
   return false;
}

static void sdl_set_icons(ALLEGRO_DISPLAY *display, int num_icons, ALLEGRO_BITMAP *bitmaps[]) {
   ALLEGRO_DISPLAY_SDL *sdl = (void *)display;
   int w = al_get_bitmap_width(bitmaps[0]);
   int h = al_get_bitmap_height(bitmaps[0]);
   int data_size = w * h * 4;
   (void)num_icons;

   unsigned char* data = al_malloc(data_size * sizeof(data[0]));

   Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
   rmask = 0xff000000;
   gmask = 0x00ff0000;
   bmask = 0x0000ff00;
   amask = 0x000000ff;
#else // little endian, like x86
   rmask = 0x000000ff;
   gmask = 0x0000ff00;
   bmask = 0x00ff0000;
   amask = 0xff000000;
#endif

   ALLEGRO_LOCKED_REGION *lock = al_lock_bitmap(bitmaps[0], ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_READONLY);
   if (lock) {
      int i = 0, y = 0;
      for (y = 0; y < h; y++) {
         int x = 0;
         for (x = 0; x < w; x++) {
            ALLEGRO_COLOR c = al_get_pixel(bitmaps[0], x, y);
            al_unmap_rgba(c, data+i, data+i+1, data+i+2, data+i+3);
            i += 4;
         }
      }
      al_unlock_bitmap(bitmaps[0]);

      SDL_Surface *icon = SDL_CreateRGBSurfaceFrom(data, w, h, 4 * 8, w * 4, rmask, gmask, bmask, amask);
      SDL_SetWindowIcon(sdl->window, icon);
      SDL_FreeSurface(icon);
   }

   al_free(data);
}

ALLEGRO_DISPLAY_INTERFACE *_al_sdl_display_driver(void)
{
   if (vt)
      return vt;

   vt = al_calloc(1, sizeof *vt);
   vt->id = AL_ID('S', 'D', 'L', '2');
   vt->create_display = sdl_create_display;
   vt->destroy_display = sdl_destroy_display;
   vt->set_current_display = sdl_set_current_display;
   vt->unset_current_display = sdl_unset_current_display;
   //vt->clear = GL
   //vt->draw_pixel = GL
   vt->flip_display = sdl_flip_display;
   vt->update_display_region = sdl_update_display_region;
   vt->acknowledge_resize = sdl_acknowledge_resize;
   vt->resize_display = sdl_resize_display;
   /*vt->quick_size = sdl_quick_size;
   vt->get_orientation = sdl_get_orientation;*/
   vt->create_bitmap = _al_ogl_create_bitmap;
   vt->set_target_bitmap = _al_ogl_set_target_bitmap;
   vt->get_backbuffer = _al_ogl_get_backbuffer;
   vt->is_compatible_bitmap = sdl_is_compatible_bitmap;
   /*vt->switch_out = sdl_switch_out;
   vt->switch_in = sdl_switch_in;
   vt->draw_memory_bitmap_region = sdl_draw_memory_bitmap_region;
   vt->wait_for_vsync = sdl_wait_for_vsync;*/
   vt->set_mouse_cursor = sdl_set_mouse_cursor;
   vt->set_system_mouse_cursor = sdl_set_system_mouse_cursor;
   vt->show_mouse_cursor = sdl_show_mouse_cursor;
   vt->hide_mouse_cursor = sdl_hide_mouse_cursor;
   vt->set_icons = sdl_set_icons;
   vt->set_window_position = sdl_set_window_position;
   vt->get_window_position = sdl_get_window_position;
   /*vt->set_window_constraints = sdl_set_window_constraints;
   vt->get_window_constraints = sdl_get_window_constraints;*/
   vt->set_display_flag = sdl_set_display_flag;
   vt->set_window_title = sdl_set_window_title;
   //vt->flush_vertex_cache = GL
   //vt->prepare_vertex_cache = GL
   //vt->update_transformation = GL
   //vt->set_projection = GL
   /*vt->shutdown = sdl_shutdown;
   vt->acknowledge_drawing_halt = sdl_acknowledge_drawing_halt;
   vt->acknowledge_drawing_resume = sdl_acknowledge_drawing_resume;
   vt->set_display_option = sdl_set_display_option;*/
   //vt->clear_depth_buffer = GL
   vt->update_render_state = _al_ogl_update_render_state;

   _al_ogl_add_drawing_functions(vt);

   return vt;
}
