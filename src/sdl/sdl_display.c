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
         event.display.type = ALLEGRO_EVENT_DISPLAY_RESIZE;
         event.display.width = e->window.data1;
         event.display.height = e->window.data2;
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
   int flags = SDL_WINDOW_OPENGL;
   if (d->flags & ALLEGRO_FULLSCREEN)
      flags |= SDL_WINDOW_FULLSCREEN;
   if (d->flags & ALLEGRO_FULLSCREEN_WINDOW)
      flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
   if (d->flags & ALLEGRO_FRAMELESS)
      flags |= SDL_WINDOW_BORDERLESS;
   if (d->flags & ALLEGRO_RESIZABLE)
      flags |= SDL_WINDOW_RESIZABLE;

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

   sdl->window = SDL_CreateWindow(sdl->title, sdl->x, sdl->y,
      d->w, d->h, flags);
   if (!sdl->window) {
      ALLEGRO_ERROR("SDL_CreateWindow failed: %s", SDL_GetError());
      return NULL;
   }
   flags =
      SDL_RENDERER_ACCELERATED |
      SDL_RENDERER_PRESENTVSYNC |
      SDL_RENDERER_TARGETTEXTURE;
   sdl->renderer = SDL_CreateRenderer(sdl->window, -1, flags);
   sdl->context = SDL_GL_CreateContext(sdl->window);
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

static void sdl_destroy_display_locked(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_SDL *sdl = (void *)d;
   ALLEGRO_SYSTEM *system = al_get_system_driver();
   _al_event_source_free(&d->es);
   _al_vector_find_and_delete(&system->displays, &d);
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
   SDL_RenderPresent(sdl->renderer);

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
   SDL_RenderPresent(sdl->renderer);
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
   (void)display;
   (void)cursor;
   return false;
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
   return false;
}

static bool sdl_hide_mouse_cursor(ALLEGRO_DISPLAY *display)
{
   (void)display;
   return false;
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
   SDL_GetWindowSize(sdl->window, &display->w, &display->h);

   _al_ogl_setup_gl(display);

   if (display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
      display->default_shader = _al_create_default_shader(display->flags);
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
   SDL_SetWindowSize(sdl->window, width, height);
   sdl_acknowledge_resize(display);
   return true;
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
   /*vt->set_icons = sdl_set_icons;*/
   vt->set_window_position = sdl_set_window_position;
   vt->get_window_position = sdl_get_window_position;
   /*vt->set_window_constraints = sdl_set_window_constraints;
   vt->get_window_constraints = sdl_get_window_constraints;
   vt->set_display_flag = sdl_set_display_flag;*/
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
