#include <stdio.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_vector.h"
#include "allegro5/internal/aintern_raspberrypi.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xwindow.h"

#include <GLES/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <bcm_host.h>

#include "picursor.h"
#define DEFAULT_CURSOR_WIDTH 17
#define DEFAULT_CURSOR_HEIGHT 28

static A5O_DISPLAY_INTERFACE *vt;

static EGLDisplay egl_display;
static EGLSurface egl_window;
static EGLContext egl_context;
static DISPMANX_UPDATE_HANDLE_T dispman_update;
static DISPMANX_RESOURCE_HANDLE_T cursor_resource;
static DISPMANX_DISPLAY_HANDLE_T dispman_display;
static DISPMANX_ELEMENT_HANDLE_T cursor_element;
static VC_RECT_T dst_rect;
static VC_RECT_T src_rect;
static bool cursor_added = false;
static float mouse_scale_ratio_x = 1.0f, mouse_scale_ratio_y = 1.0f;

struct A5O_DISPLAY_RASPBERRYPI_EXTRA {
};

static int pot(int n)
{
	int i = 1;
	while (i < n) {
		i = i * 2;
	}
	return i;
}

static void set_cursor_data(A5O_DISPLAY_RASPBERRYPI *d, uint32_t *data, int width, int height)
{
   al_free(d->cursor_data);
   int spitch = sizeof(uint32_t) * width;
   int dpitch = pot(spitch);
   d->cursor_data = al_malloc(dpitch * height);
   int y;
   for (y = 0; y < height; y++) {
   	uint8_t *p1 = (uint8_t *)d->cursor_data + y * dpitch;
	uint8_t *p2 = (uint8_t *)data + y * spitch;
	memcpy(p1, p2, spitch);
   }
   d->cursor_width = width;
   d->cursor_height = height;
}

static void delete_cursor_data(A5O_DISPLAY_RASPBERRYPI *d)
{
      al_free(d->cursor_data);
      d->cursor_data = NULL;
}

static void show_cursor(A5O_DISPLAY_RASPBERRYPI *d)
{
   if (d->cursor_data == NULL) {
      return;
   }

   int width = d->cursor_width;
   int height = d->cursor_height;

   if (cursor_added == false) {
      uint32_t unused;
      cursor_resource = vc_dispmanx_resource_create(VC_IMAGE_ARGB8888, width, height, &unused);

      VC_RECT_T r;
      r.x = 0;
      r.y = 0;
      r.width = width;
      r.height = height;

      int dpitch = pot(sizeof(uint32_t) * width);

      dispman_update = vc_dispmanx_update_start(0);
      vc_dispmanx_resource_write_data(cursor_resource, VC_IMAGE_ARGB8888, dpitch, d->cursor_data, &r);
      vc_dispmanx_update_submit_sync(dispman_update);

      A5O_MOUSE_STATE state;
      al_get_mouse_state(&state);

      dst_rect.x = state.x+d->cursor_offset_x;
      dst_rect.y = state.y+d->cursor_offset_y;
      dst_rect.width = width;
      dst_rect.height = height;
      src_rect.x = 0;
      src_rect.y = 0;
      src_rect.width = width << 16;
      src_rect.height = height << 16;

      dispman_update = vc_dispmanx_update_start(0);

      cursor_element = vc_dispmanx_element_add(
         dispman_update,
         dispman_display,
         0/*layer*/,
         &dst_rect,
         cursor_resource,
         &src_rect,
         DISPMANX_PROTECTION_NONE,
         0 /*alpha*/,
         0/*clamp*/,
         0/*transform*/
      );

      vc_dispmanx_update_submit_sync(dispman_update);

      cursor_added = true;
   }
   else {
      A5O_DISPLAY *disp = (A5O_DISPLAY *)d;
      VC_RECT_T src, dst;
      src.x = 0;
      src.y = 0;
      src.width = d->cursor_width << 16;
      src.height = d->cursor_height << 16;
      A5O_MOUSE_STATE st;
      al_get_mouse_state(&st);
      st.x = (st.x+0.5) * d->screen_width / disp->w;
      st.y = (st.y+0.5) * d->screen_height / disp->h;
      dst.x = st.x+d->cursor_offset_x;
      dst.y = st.y+d->cursor_offset_y;
      dst.width = d->cursor_width;
      dst.height = d->cursor_height;
         dispman_update = vc_dispmanx_update_start(0);
      vc_dispmanx_element_change_attributes(
         dispman_update,
         cursor_element,
         0,
         0,
         0,
         &dst,
         &src,
         0,
         0
      );
      vc_dispmanx_update_submit_sync(dispman_update);
   }
}

static void hide_cursor(A5O_DISPLAY_RASPBERRYPI *d)
{
   (void)d;

   if (!cursor_added) {
      return;
   }

   dispman_update = vc_dispmanx_update_start(0);
   vc_dispmanx_element_remove(dispman_update, cursor_element);
   vc_dispmanx_update_submit_sync(dispman_update);
   vc_dispmanx_resource_delete(cursor_resource);
   cursor_added = false;
}

/* Helper to set up GL state as we want it. */
static void setup_gl(A5O_DISPLAY *d)
{
    A5O_OGL_EXTRAS *ogl = d->ogl_extras;

    if (ogl->backbuffer)
        _al_ogl_resize_backbuffer(ogl->backbuffer, d->w, d->h);
    else
        ogl->backbuffer = _al_ogl_create_backbuffer(d);
}

void _al_raspberrypi_get_screen_info(int *dx, int *dy,
   int *screen_width, int *screen_height)
{
   graphics_get_display_size(0 /* LCD */, (uint32_t *)screen_width, (uint32_t *)screen_height);

   /* On TV-out the visible area (area used by X and console)
    * is different from that reported by the bcm functions. We
    * have to 1) read fbwidth and fbheight from /proc/cmdline
    * and also overscan parameters from /boot/config.txt so our
    * displays are the same size and overlap perfectly.
    */
   *dx = 0;
   *dy = 0;
   FILE *cmdline = fopen("/proc/cmdline", "r");
   if (cmdline) {
      char line[1000];
      int i;
      for (i = 0; i < 999; i++) {
         int c = fgetc(cmdline);
         if (c == EOF) break;
         line[i] = c;
      }
      line[i] = 0;
      const char *p = strstr(line, "fbwidth=");
      if (p) {
         const char *p2 = strstr(line, "fbheight=");
         if (p2) {
            p += strlen("fbwidth=");
            p2 += strlen("fbheight=");
            int w = atoi(p);
            int h = atoi(p2);
            const A5O_FILE_INTERFACE *file_interface = al_get_new_file_interface();
            al_set_standard_file_interface();
            A5O_CONFIG *cfg = al_load_config_file("/boot/config.txt");
            if (cfg) {
               const char *disable_overscan =
                  al_get_config_value(
                     cfg, "", "disable_overscan"
                  );
               // If overscan parameters are disabled don't process
               if (!disable_overscan ||
                   (disable_overscan &&
                    (!strcasecmp(disable_overscan, "false") ||
                     atoi(disable_overscan) == 0))) {
                  const char *left = al_get_config_value(
                     cfg, "", "overscan_left");
                  const char *right = al_get_config_value(
                     cfg, "", "overscan_right");
                  const char *top = al_get_config_value(
                     cfg, "", "overscan_top");
                  const char *bottom = al_get_config_value(
                     cfg, "", "overscan_bottom");
                  int xx = left ? atoi(left) : 0;
                  xx -= right ? atoi(right) : 0;
                  int yy = top ? atoi(top) : 0;
                  yy -= bottom ? atoi(bottom) : 0;
                  *dx = (*screen_width - w + xx) / 2;
                  *dy = (*screen_height - h + yy) / 2;
                  *screen_width = w;
                  *screen_height = h;
               }
               else {
                  *dx = (*screen_width - w) / 2;
                  *dy = (*screen_height - h) / 2;
                  *screen_width = w;
                  *screen_height = h;
               }
               al_destroy_config(cfg);
            }
            else {
               printf("Couldn't open /boot/config.txt\n");
            }
            al_set_new_file_interface(file_interface);
         }
      }
      fclose(cmdline);
   }
}

void _al_raspberrypi_get_mouse_scale_ratios(float *x, float *y)
{
	*x = mouse_scale_ratio_x;
	*y = mouse_scale_ratio_y;
}

static bool pi_create_display(A5O_DISPLAY *display)
{
   A5O_DISPLAY_RASPBERRYPI *d = (void *)display;
   A5O_EXTRA_DISPLAY_SETTINGS *eds = _al_get_new_display_settings();

   egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   if (egl_display == EGL_NO_DISPLAY) {
      return false;
   }

   int major, minor;
   if (!eglInitialize(egl_display, &major, &minor)) {
      return false;
   }

   static EGLint attrib_list[] =
   {
      EGL_DEPTH_SIZE, 0,
      EGL_STENCIL_SIZE, 0,
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_NONE
   };

   attrib_list[1] = eds->settings[A5O_DEPTH_SIZE];
   attrib_list[3] = eds->settings[A5O_STENCIL_SIZE];

   if (eds->settings[A5O_RED_SIZE] || eds->settings[A5O_GREEN_SIZE] ||
         eds->settings[A5O_BLUE_SIZE] ||
         eds->settings[A5O_ALPHA_SIZE]) {
      attrib_list[5] = eds->settings[A5O_RED_SIZE];
      attrib_list[7] = eds->settings[A5O_GREEN_SIZE];
      attrib_list[9] = eds->settings[A5O_BLUE_SIZE];
      attrib_list[11] = eds->settings[A5O_ALPHA_SIZE];
   }
   else if (eds->settings[A5O_COLOR_SIZE] == 16) {
      attrib_list[5] = 5;
      attrib_list[7] = 6;
      attrib_list[9] = 5;
      attrib_list[11] = 0;
   }

   EGLConfig config;
   int num_configs;

   if (!eglChooseConfig(egl_display, attrib_list, &config, 1, &num_configs)) {
      return false;
   }

   eglBindAPI(EGL_OPENGL_ES_API);

   int es_ver = (display->flags & A5O_PROGRAMMABLE_PIPELINE) ?
      2 : 1;

   static EGLint ctxattr[3] = {
      EGL_CONTEXT_CLIENT_VERSION, 0xDEADBEEF,
      EGL_NONE
   };

   ctxattr[1] = es_ver;

   egl_context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, ctxattr);
   if (egl_context == EGL_NO_CONTEXT) {
      return false;
   }

   static EGL_DISPMANX_WINDOW_T nativewindow;
   DISPMANX_ELEMENT_HANDLE_T dispman_element;

   int dx, dy, screen_width, screen_height;
   _al_raspberrypi_get_screen_info(&dx, &dy, &screen_width, &screen_height);

   mouse_scale_ratio_x = (float)display->w / screen_width;
   mouse_scale_ratio_y = (float)display->h / screen_height;

   d->cursor_offset_x = dx;
   d->cursor_offset_y = dy;

   dst_rect.x = dx;
   dst_rect.y = dy;
   dst_rect.width = screen_width;
   dst_rect.height = screen_height;

   d->screen_width = screen_width;
   d->screen_height = screen_height;

   src_rect.x = 0;
   src_rect.y = 0;
   src_rect.width = display->w << 16;
   src_rect.height = display->h << 16;

   dispman_display = vc_dispmanx_display_open(0 /* LCD */);
   dispman_update = vc_dispmanx_update_start(0);

   dispman_element = vc_dispmanx_element_add (
      dispman_update,
      dispman_display,
      0/*layer*/,
      &dst_rect,
      0/*src*/,
      &src_rect,
      DISPMANX_PROTECTION_NONE,
      0 /*alpha*/,
      0/*clamp*/,
      DISPMANX_NO_ROTATE/*transform*/
   );

   nativewindow.element = dispman_element;
   nativewindow.width = display->w;
   nativewindow.height = display->h;
   vc_dispmanx_update_submit_sync(dispman_update);

   egl_window = eglCreateWindowSurface(
      egl_display, config, &nativewindow, NULL);
   if (egl_window == EGL_NO_SURFACE) {
      return false;
   }

   if (!eglMakeCurrent(egl_display, egl_window, egl_window, egl_context)) {
      return false;
   }

   if (!getenv("DISPLAY")) {
      _al_evdev_set_mouse_range(0, 0, display->w-1, display->h-1);
   }

   return true;
}

static A5O_DISPLAY *raspberrypi_create_display(int w, int h)
{
    A5O_DISPLAY_RASPBERRYPI *d = al_calloc(1, sizeof *d);
    A5O_DISPLAY *display = (void*)d;
    A5O_OGL_EXTRAS *ogl = al_calloc(1, sizeof *ogl);
    display->ogl_extras = ogl;
    display->vt = _al_get_raspberrypi_display_interface();
    display->flags = al_get_new_display_flags();

    A5O_SYSTEM_RASPBERRYPI *system = (void *)al_get_system_driver();

    /* Add ourself to the list of displays. */
    A5O_DISPLAY_RASPBERRYPI **add;
    add = _al_vector_alloc_back(&system->system.displays);
    *add = d;

    /* Each display is an event source. */
    _al_event_source_init(&display->es);

    display->extra_settings.settings[A5O_COMPATIBLE_DISPLAY] = 1;

   display->w = w;
   display->h = h;

   display->flags |= A5O_OPENGL;
#ifdef A5O_CFG_OPENGLES2
   display->flags |= A5O_PROGRAMMABLE_PIPELINE;
#endif
#ifdef A5O_CFG_OPENGLES
   display->flags |= A5O_OPENGL_ES_PROFILE;
#endif

   if (!pi_create_display(display)) {
      // FIXME: cleanup
      return NULL;
   }

   if (getenv("DISPLAY")) {
      _al_mutex_lock(&system->lock);
      Window root = RootWindow(
         system->x11display, DefaultScreen(system->x11display));
      XWindowAttributes attr;
      XGetWindowAttributes(system->x11display, root, &attr);
      d->window = XCreateWindow(
         system->x11display,
         root,
         0,
         0,
         attr.width,
         attr.height,
         0, 0,
         InputOnly,
         DefaultVisual(system->x11display, 0),
         0,
         NULL
      );
      XGetWindowAttributes(system->x11display, d->window, &attr);
      XSelectInput(
         system->x11display,
         d->window,
         PointerMotionMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask
      );
      XMapWindow(system->x11display, d->window);
      _al_xwin_reset_size_hints(display);
      _al_xwin_set_fullscreen_window(display, 2);
      _al_xwin_set_size_hints(display, INT_MAX, INT_MAX);
      d->wm_delete_window_atom = XInternAtom(system->x11display,
         "WM_DELETE_WINDOW", False);
      XSetWMProtocols(system->x11display, d->window, &d->wm_delete_window_atom, 1);
      _al_mutex_unlock(&system->lock);
   }

   al_grab_mouse(display);

   _al_ogl_manage_extensions(display);
   _al_ogl_set_extensions(ogl->extension_api);

   setup_gl(display);

   al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);

   if (al_is_mouse_installed() && !getenv("DISPLAY")) {
      _al_evdev_set_mouse_range(0, 0, display->w-1, display->h-1);
   }

   set_cursor_data(d, default_cursor, DEFAULT_CURSOR_WIDTH, DEFAULT_CURSOR_HEIGHT);

   /* Fill in opengl version */
   const int v = display->ogl_extras->ogl_info.version;
   display->extra_settings.settings[A5O_OPENGL_MAJOR_VERSION] = (v >> 24) & 0xFF;
   display->extra_settings.settings[A5O_OPENGL_MINOR_VERSION] = (v >> 16) & 0xFF;

   return display;
}

static void raspberrypi_destroy_display(A5O_DISPLAY *d)
{
   A5O_DISPLAY_RASPBERRYPI *pidisplay = (A5O_DISPLAY_RASPBERRYPI *)d;

   hide_cursor(pidisplay);
   delete_cursor_data(pidisplay);

   _al_set_current_display_only(d);

   while (d->bitmaps._size > 0) {
      A5O_BITMAP **bptr = (A5O_BITMAP **)_al_vector_ref_back(&d->bitmaps);
      A5O_BITMAP *b = *bptr;
      _al_convert_to_memory_bitmap(b);
   }

   _al_event_source_free(&d->es);

   A5O_SYSTEM_RASPBERRYPI *system = (void *)al_get_system_driver();
   _al_vector_find_and_delete(&system->system.displays, &d);

   eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
   eglDestroySurface(egl_display, egl_window);
   eglDestroyContext(egl_display, egl_context);
   eglTerminate(egl_display);

   if (getenv("DISPLAY")) {
      _al_mutex_lock(&system->lock);
      XUnmapWindow(system->x11display, pidisplay->window);
      XDestroyWindow(system->x11display, pidisplay->window);
      _al_mutex_unlock(&system->lock);
   }

   if (system->mouse_grab_display == d) {
      system->mouse_grab_display = NULL;
   }
}

static bool raspberrypi_set_current_display(A5O_DISPLAY *d)
{
   (void)d;
// FIXME
   _al_ogl_update_render_state(d);
   return true;
}

static int raspberrypi_get_orientation(A5O_DISPLAY *d)
{
   (void)d;
   return A5O_DISPLAY_ORIENTATION_0_DEGREES;
}


/* There can be only one window and only one OpenGL context, so all bitmaps
 * are compatible.
 */
static bool raspberrypi_is_compatible_bitmap(
   A5O_DISPLAY *display,
   A5O_BITMAP *bitmap
) {
    (void)display;
    (void)bitmap;
    return true;
}

/* Resizing is not possible. */
static bool raspberrypi_resize_display(A5O_DISPLAY *d, int w, int h)
{
    (void)d;
    (void)w;
    (void)h;
    return false;
}

/* The icon must be provided in the Info.plist file, it cannot be changed
 * at runtime.
 */
static void raspberrypi_set_icons(A5O_DISPLAY *d, int num_icons, A5O_BITMAP *bitmaps[])
{
    (void)d;
    (void)num_icons;
    (void)bitmaps;
}

/* There is no way to leave fullscreen so no window title is visible. */
static void raspberrypi_set_window_title(A5O_DISPLAY *display, char const *title)
{
    (void)display;
    (void)title;
}

/* The window always spans the entire screen right now. */
static void raspberrypi_set_window_position(A5O_DISPLAY *display, int x, int y)
{
    (void)display;
    (void)x;
    (void)y;
}

/* The window cannot be constrained. */
static bool raspberrypi_set_window_constraints(A5O_DISPLAY *display,
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
static bool raspberrypi_set_display_flag(A5O_DISPLAY *display,
   int flag, bool onoff)
{
   (void)display;
   (void)flag;
   (void)onoff;
   return false;
}

static void raspberrypi_get_window_position(A5O_DISPLAY *display, int *x, int *y)
{
    (void)display;
    *x = 0;
    *y = 0;
}

/* The window cannot be constrained. */
static bool raspberrypi_get_window_constraints(A5O_DISPLAY *display,
   int *min_w, int *min_h, int *max_w, int *max_h)
{
   (void)display;
   (void)min_w;
   (void)min_h;
   (void)max_w;
   (void)max_h;
   return false;
}

static bool raspberrypi_wait_for_vsync(A5O_DISPLAY *display)
{
    (void)display;
    return false;
}

static void raspberrypi_flip_display(A5O_DISPLAY *disp)
{
   eglSwapBuffers(egl_display, egl_window);

   if (cursor_added) {
      show_cursor((A5O_DISPLAY_RASPBERRYPI *)disp);
   }
}

static void raspberrypi_update_display_region(A5O_DISPLAY *d, int x, int y,
                                       int w, int h)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    raspberrypi_flip_display(d);
}

static bool raspberrypi_acknowledge_resize(A5O_DISPLAY *d)
{
   setup_gl(d);
   return true;
}

static bool raspberrypi_show_mouse_cursor(A5O_DISPLAY *display)
{
   A5O_DISPLAY_RASPBERRYPI *d = (void *)display;
   hide_cursor(d);
   show_cursor(d);
   return true;
}

static bool raspberrypi_hide_mouse_cursor(A5O_DISPLAY *display)
{
   A5O_DISPLAY_RASPBERRYPI *d = (void *)display;
   hide_cursor(d);
   return true;
}

static bool raspberrypi_set_mouse_cursor(A5O_DISPLAY *display, A5O_MOUSE_CURSOR *cursor)
{
   A5O_DISPLAY_RASPBERRYPI *d = (void *)display;
   A5O_MOUSE_CURSOR_RASPBERRYPI *pi_cursor = (void *)cursor;
   int w = al_get_bitmap_width(pi_cursor->bitmap);
   int h = al_get_bitmap_height(pi_cursor->bitmap);
   int pitch = w * sizeof(uint32_t);
   uint32_t *data = al_malloc(pitch * h);
   A5O_LOCKED_REGION *lr = al_lock_bitmap(pi_cursor->bitmap, A5O_PIXEL_FORMAT_ARGB_8888, A5O_LOCK_READONLY);
   int y;
   for (y = 0; y < h; y++) {
      uint8_t *p = (uint8_t *)lr->data + lr->pitch * y;
      uint8_t *p2 = (uint8_t *)data + pitch * y;
      memcpy(p2, p, pitch);
   }
   al_unlock_bitmap(pi_cursor->bitmap);
   delete_cursor_data(d);
   set_cursor_data(d, data, w, h);
   al_free(data);
   if (cursor_added) {
   	hide_cursor(d);
	show_cursor(d);
   }
   return true;
}

static bool raspberrypi_set_system_mouse_cursor(A5O_DISPLAY *display, A5O_SYSTEM_MOUSE_CURSOR cursor_id)
{
   (void)cursor_id;
   A5O_DISPLAY_RASPBERRYPI *d = (void *)display;
   delete_cursor_data(d);
   set_cursor_data(d, default_cursor, DEFAULT_CURSOR_WIDTH, DEFAULT_CURSOR_HEIGHT);
   return true;
}

/* Obtain a reference to this driver. */
A5O_DISPLAY_INTERFACE *_al_get_raspberrypi_display_interface(void)
{
    if (vt)
        return vt;

    vt = al_calloc(1, sizeof *vt);

    vt->create_display = raspberrypi_create_display;
    vt->destroy_display = raspberrypi_destroy_display;
    vt->set_current_display = raspberrypi_set_current_display;
    vt->flip_display = raspberrypi_flip_display;
    vt->update_display_region = raspberrypi_update_display_region;
    vt->acknowledge_resize = raspberrypi_acknowledge_resize;
    vt->create_bitmap = _al_ogl_create_bitmap;
    vt->get_backbuffer = _al_ogl_get_backbuffer;
    vt->set_target_bitmap = _al_ogl_set_target_bitmap;

    vt->get_orientation = raspberrypi_get_orientation;

    vt->is_compatible_bitmap = raspberrypi_is_compatible_bitmap;
    vt->resize_display = raspberrypi_resize_display;
    vt->set_icons = raspberrypi_set_icons;
    vt->set_window_title = raspberrypi_set_window_title;
    vt->set_window_position = raspberrypi_set_window_position;
    vt->get_window_position = raspberrypi_get_window_position;
    vt->set_window_constraints = raspberrypi_set_window_constraints;
    vt->get_window_constraints = raspberrypi_get_window_constraints;
    vt->set_display_flag = raspberrypi_set_display_flag;
    vt->wait_for_vsync = raspberrypi_wait_for_vsync;

    vt->update_render_state = _al_ogl_update_render_state;

    _al_ogl_add_drawing_functions(vt);

    vt->set_mouse_cursor = raspberrypi_set_mouse_cursor;
    vt->set_system_mouse_cursor = raspberrypi_set_system_mouse_cursor;
    vt->show_mouse_cursor = raspberrypi_show_mouse_cursor;
    vt->hide_mouse_cursor = raspberrypi_hide_mouse_cursor;

    return vt;
}

