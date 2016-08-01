#include <allegro5/allegro.h>
#include <allegro5/allegro_opengl.h>
#include <allegro5/internal/aintern_iphone.h>
#include <allegro5/internal/aintern_opengl.h>
#include <allegro5/internal/aintern_vector.h>
#include <math.h>

#include "iphone.h"
#include "allegroAppDelegate.h"

ALLEGRO_DEBUG_CHANNEL("iphone")

static ALLEGRO_DISPLAY_INTERFACE *vt;

static float _screen_iscale = 1.0;
static float _screen_x, _screen_y;
static float _screen_w, _screen_h;
static bool _screen_hack;

bool _al_iphone_is_display_connected(ALLEGRO_DISPLAY *display);
void _al_iphone_connect_airplay(void);
extern ALLEGRO_MUTEX *_al_iphone_display_hotplug_mutex;

void _al_iphone_setup_opengl_view(ALLEGRO_DISPLAY *d, bool manage_backbuffer)
{
   int w, h;

   w = d->w;
   h = d->h;

   _al_iphone_reset_framebuffer(d);

   _screen_w = w;
   _screen_h = h;

   if (manage_backbuffer) {
      _al_ogl_setup_gl(d);
   }
}

void _al_iphone_translate_from_screen(ALLEGRO_DISPLAY *d, int *x, int *y)
{
   if (!_screen_hack) return;
   // See _al_iphone_setup_opengl_view
   float ox = *x, oy = *y;

   if (d->w >= d->h) {
      *x = _screen_x + oy * _screen_iscale;
      *y = _screen_y + (_screen_w - ox) * _screen_iscale;
   }
   else {
      // TODO
   }
}

void _al_iphone_clip(ALLEGRO_BITMAP const *bitmap, int x_1, int y_1, int x_2, int y_2)
{
   ALLEGRO_BITMAP *oglb = (void *)(bitmap->parent ? bitmap->parent : bitmap);
   int h = oglb->h;
   glScissor(x_1, h - y_2, x_2 - x_1, y_2 - y_1);
}

static void set_rgba8888(ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds)
{
   eds->settings[ALLEGRO_RED_SIZE] = 8;
   eds->settings[ALLEGRO_GREEN_SIZE] = 8;
   eds->settings[ALLEGRO_BLUE_SIZE] = 8;
   eds->settings[ALLEGRO_ALPHA_SIZE] = 8;
   eds->settings[ALLEGRO_RED_SHIFT] = 0;
   eds->settings[ALLEGRO_GREEN_SHIFT] = 8;
   eds->settings[ALLEGRO_BLUE_SHIFT] = 16;
   eds->settings[ALLEGRO_ALPHA_SHIFT] = 24;
   eds->settings[ALLEGRO_COLOR_SIZE] = 32;
}

static void set_rgb565(ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds)
{
   eds->settings[ALLEGRO_RED_SIZE] = 5;
   eds->settings[ALLEGRO_GREEN_SIZE] = 6;
   eds->settings[ALLEGRO_BLUE_SIZE] = 5;
   eds->settings[ALLEGRO_ALPHA_SIZE] = 0;
   eds->settings[ALLEGRO_RED_SHIFT] = 0;
   eds->settings[ALLEGRO_GREEN_SHIFT] = 5;
   eds->settings[ALLEGRO_BLUE_SHIFT] = 11;
   eds->settings[ALLEGRO_ALPHA_SHIFT] = 0;
   eds->settings[ALLEGRO_COLOR_SIZE] = 16;
}

#define VISUALS_COUNT 6
void _al_iphone_update_visuals(void)
{
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref;
   ALLEGRO_SYSTEM_IPHONE *system = (void *)al_get_system_driver();

   ref = _al_get_new_display_settings();

   /* If we aren't called the first time, only updated scores. */
   if (system->visuals) {
      for (int i = 0; i < system->visuals_count; i++) {
         ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds = system->visuals[i];
         eds->score = _al_score_display_settings(eds, ref);
      }
      return;
   }

   system->visuals = al_calloc(1, VISUALS_COUNT * sizeof(*system->visuals));
   system->visuals_count = VISUALS_COUNT;

   for (int i = 0; i < VISUALS_COUNT; i++) {
      ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds = al_calloc(1, sizeof *eds);
      eds->settings[ALLEGRO_RENDER_METHOD] = 1;
      eds->settings[ALLEGRO_COMPATIBLE_DISPLAY] = 1;
      eds->settings[ALLEGRO_SWAP_METHOD] = 2;
      eds->settings[ALLEGRO_VSYNC] = 1;
      eds->settings[ALLEGRO_SUPPORTED_ORIENTATIONS] =
         ref->settings[ALLEGRO_SUPPORTED_ORIENTATIONS];
      switch (i) {
         case 0:
            set_rgba8888(eds);
            break;
         case 1:
            set_rgb565(eds);
            break;
         case 2:
            set_rgba8888(eds);
            eds->settings[ALLEGRO_DEPTH_SIZE] = 16;
            break;
         case 3:
            set_rgb565(eds);
            eds->settings[ALLEGRO_DEPTH_SIZE] = 16;
            break;
         case 4:
            set_rgba8888(eds);
            eds->settings[ALLEGRO_DEPTH_SIZE] = 24;
            eds->settings[ALLEGRO_STENCIL_SIZE] = 8;
            break;
         case 5:
            set_rgb565(eds);
            eds->settings[ALLEGRO_DEPTH_SIZE] = 24;
            eds->settings[ALLEGRO_STENCIL_SIZE] = 8;
            break;

      }
      eds->score = _al_score_display_settings(eds, ref);
      eds->index = i;
      system->visuals[i] = eds;
   }
}

static ALLEGRO_DISPLAY *iphone_create_display(int w, int h)
{
    ALLEGRO_DISPLAY_IPHONE *d = al_calloc(1, sizeof *d);
    ALLEGRO_DISPLAY *display = (void*)d;
    ALLEGRO_OGL_EXTRAS *ogl = al_calloc(1, sizeof *ogl);
    display->ogl_extras = ogl;
    display->vt = _al_get_iphone_display_interface();
    display->flags = al_get_new_display_flags();
    int adapter;

    adapter = al_get_new_display_adapter();
    if (adapter < 0)
       adapter = 0;

    if (adapter == 1) {
        _al_iphone_connect_airplay();
    }

    if (display->flags & ALLEGRO_FULLSCREEN_WINDOW) {
        _al_iphone_get_screen_size(adapter, &w, &h);
    }

    display->w = w;
    display->h = h;

    ALLEGRO_SYSTEM_IPHONE *system = (void *)al_get_system_driver();

    /* Add ourself to the list of displays. */
    ALLEGRO_DISPLAY_IPHONE **add;
    add = _al_vector_alloc_back(&system->system.displays);
    *add = d;

    /* Each display is an event source. */
    _al_event_source_init(&display->es);

   _al_iphone_update_visuals();

   ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds[system->visuals_count];
   memcpy(eds, system->visuals, sizeof(*eds) * system->visuals_count);
   qsort(eds, system->visuals_count, sizeof(*eds), _al_display_settings_sorter);

   ALLEGRO_INFO("Chose visual no. %i\n", eds[0]->index);

   memcpy(&display->extra_settings, eds[0], sizeof(ALLEGRO_EXTRA_DISPLAY_SETTINGS));

   /* This will add an OpenGL view with an OpenGL context, then return. */
   if (!_al_iphone_add_view(display)) {
      /* FIXME: cleanup */
      return NULL;
   }

   _al_iphone_make_view_current(display);

   /* FIXME: there is some sort of race condition somewhere. */
   al_rest(1.0);

   _al_ogl_manage_extensions(display);
   _al_ogl_set_extensions(ogl->extension_api);
   _al_iphone_setup_opengl_view(display, true);

   display->flags |= ALLEGRO_OPENGL;

   int ndisplays = system->system.displays._size;
   [[UIApplication sharedApplication] setIdleTimerDisabled:(ndisplays > 1)];

   return display;
}

static void iphone_destroy_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_IPHONE *idisplay = (ALLEGRO_DISPLAY_IPHONE *)d;
   ALLEGRO_DISPLAY_IPHONE_EXTRA *extra = idisplay->extra;
   bool disconnected = extra->disconnected;

   _al_set_current_display_only(d);

   while (d->bitmaps._size > 0) {
      ALLEGRO_BITMAP **bptr = (ALLEGRO_BITMAP **)_al_vector_ref_back(&d->bitmaps);
      ALLEGRO_BITMAP *b = *bptr;
      _al_convert_to_memory_bitmap(b);
   }

   _al_event_source_free(&d->es);
   _al_iphone_destroy_screen(d);

   if (disconnected) {
      _al_iphone_disconnect(d);
   }

   ALLEGRO_SYSTEM_IPHONE *system = (void *)al_get_system_driver();
   _al_vector_find_and_delete(&system->system.displays, &d);

   [[UIApplication sharedApplication] setIdleTimerDisabled:FALSE];
}

static bool iphone_set_current_display(ALLEGRO_DISPLAY *d)
{
   (void)d;
   _al_iphone_make_view_current(d);
   _al_ogl_update_render_state(d);
   return true;
}

static int iphone_get_orientation(ALLEGRO_DISPLAY *d)
{
   return _al_iphone_get_orientation(d);
}


/* There can be only one window and only one OpenGL context, so all bitmaps
 * are compatible.
 */
static bool iphone_is_compatible_bitmap(ALLEGRO_DISPLAY *display,
                                      ALLEGRO_BITMAP *bitmap)
{
    (void)display;
    (void)bitmap;
    return true;
}

/* Resizing is not possible. */
static bool iphone_resize_display(ALLEGRO_DISPLAY *d, int w, int h)
{
    (void)d;
    (void)w;
    (void)h;
    return false;
}

/* The icon must be provided in the Info.plist file, it cannot be changed
 * at runtime.
 */
static void iphone_set_icons(ALLEGRO_DISPLAY *d, int num_icons, ALLEGRO_BITMAP *bitmaps[])
{
    (void)d;
    (void)num_icons;
    (void)bitmaps;
}

/* There is no way to leave fullscreen so no window title is visible. */
static void iphone_set_window_title(ALLEGRO_DISPLAY *display, char const *title)
{
    (void)display;
    (void)title;
}

/* The window always spans the entire screen right now. */
static void iphone_set_window_position(ALLEGRO_DISPLAY *display, int x, int y)
{
    (void)display;
    (void)x;
    (void)y;
}

/* The window cannot be constrained. */
static bool iphone_set_window_constraints(ALLEGRO_DISPLAY *display,
   int min_w, int min_h, int max_w, int max_h)
{
   (void)display;
   (void)min_w;
   (void)min_h;
   (void)max_w;
   (void)max_h;
   return false;
}

/* Always fullscreen. */
static bool iphone_set_display_flag(ALLEGRO_DISPLAY *display,
   int flag, bool onoff)
{
   (void)display;
   (void)flag;
   (void)onoff;
   return false;
}

static void iphone_get_window_position(ALLEGRO_DISPLAY *display, int *x, int *y)
{
    (void)display;
    *x = 0;
    *y = 0;
}

/* The window cannot be constrained. */
static bool iphone_get_window_constraints(ALLEGRO_DISPLAY *display,
   int *min_w, int *min_h, int *max_w, int *max_h)
{
   (void)display;
   (void)min_w;
   (void)min_h;
   (void)max_w;
   (void)max_h;
   return false;
}

static bool iphone_wait_for_vsync(ALLEGRO_DISPLAY *display)
{
    (void)display;
    return false;
}

static void iphone_flip_display(ALLEGRO_DISPLAY *d)
{
    _al_iphone_flip_view(d);
}

static void iphone_update_display_region(ALLEGRO_DISPLAY *d, int x, int y,
                                       int w, int h)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    iphone_flip_display(d);
}

static bool iphone_acknowledge_resize(ALLEGRO_DISPLAY *d)
{
   _al_iphone_recreate_framebuffer(d);
   _al_iphone_setup_opengl_view(d, true);
   return true;
}

static bool iphone_set_mouse_cursor(ALLEGRO_DISPLAY *display,
                                  ALLEGRO_MOUSE_CURSOR *cursor)
{
    (void)display;
    (void)cursor;
    return false;
}

static bool iphone_set_system_mouse_cursor(ALLEGRO_DISPLAY *display,
                                         ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id)
{
    (void)display;
    (void)cursor_id;
    return false;
}

static bool iphone_show_mouse_cursor(ALLEGRO_DISPLAY *display)
{
    (void)display;
    return false;
}

static bool iphone_hide_mouse_cursor(ALLEGRO_DISPLAY *display)
{
    (void)display;
    return false;
}

/* Obtain a reference to this driver. */
ALLEGRO_DISPLAY_INTERFACE *_al_get_iphone_display_interface(void)
{
    if (vt)
        return vt;

    vt = al_calloc(1, sizeof *vt);

    vt->create_display = iphone_create_display;
    vt->destroy_display = iphone_destroy_display;
    vt->set_current_display = iphone_set_current_display;
    vt->flip_display = iphone_flip_display;
    vt->update_display_region = iphone_update_display_region;
    vt->acknowledge_resize = iphone_acknowledge_resize;
    vt->create_bitmap = _al_ogl_create_bitmap;
    vt->get_backbuffer = _al_ogl_get_backbuffer;
    vt->set_target_bitmap = _al_ogl_set_target_bitmap;

    vt->get_orientation = iphone_get_orientation;

    vt->is_compatible_bitmap = iphone_is_compatible_bitmap;
    vt->resize_display = iphone_resize_display;
    vt->set_icons = iphone_set_icons;
    vt->set_window_title = iphone_set_window_title;
    vt->set_window_position = iphone_set_window_position;
    vt->get_window_position = iphone_get_window_position;
    vt->set_window_constraints = iphone_set_window_constraints;
    vt->get_window_constraints = iphone_get_window_constraints;
    vt->set_display_flag = iphone_set_display_flag;
    vt->wait_for_vsync = iphone_wait_for_vsync;

    vt->set_mouse_cursor = iphone_set_mouse_cursor;
    vt->set_system_mouse_cursor = iphone_set_system_mouse_cursor;
    vt->show_mouse_cursor = iphone_show_mouse_cursor;
    vt->hide_mouse_cursor = iphone_hide_mouse_cursor;

    vt->acknowledge_drawing_halt = _al_iphone_acknowledge_drawing_halt;
    vt->update_render_state = _al_ogl_update_render_state;

    _al_ogl_add_drawing_functions(vt);
    _al_iphone_add_clipboard_functions(vt);

    return vt;
}
