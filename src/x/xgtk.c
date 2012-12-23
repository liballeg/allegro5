#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_xglx.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_x.h"

#ifdef ALLEGRO_CFG_USE_GTK

#include <gtk/gtk.h>
#include "allegro5/internal/aintern_xgtk.h"

ALLEGRO_DEBUG_CHANNEL("xgtk")


/* GTK is not thread safe.  We launch a single thread which runs the GTK main
 * loop, and it is the only thread which calls into GTK.  (g_timeout_add may be
 * called from other threads without locking.)
 *
 * We used to attempt to use gdk_threads_enter/gdk_threads_leave but hit
 * some problems with deadlocks so switched to this.
 */

// G_STATIC_MUTEX_INIT causes a warning about a missing initializer, so if we
// have version 2.32 or newer don't use it to avoid the warning.
#if GLIB_CHECK_VERSION(2, 32, 0)
   #define NEWER_GLIB   1
#else
   #define NEWER_GLIB   0
#endif
#if NEWER_GLIB
   static GMutex nd_gtk_mutex;
   void nd_gtk_lock(void)    { g_mutex_lock(&nd_gtk_mutex); }
   void nd_gtk_unlock(void)  { g_mutex_unlock(&nd_gtk_mutex); }
#else
   static GStaticMutex nd_gtk_mutex = G_STATIC_MUTEX_INIT;
   void nd_gtk_lock(void)    { g_static_mutex_lock(&nd_gtk_mutex); }
   void nd_gtk_unlock(void)  { g_static_mutex_unlock(&nd_gtk_mutex); }
#endif
static GThread *nd_gtk_thread = NULL;


#define ACK_OK       ((void *)0x1111)


static void *nd_gtk_thread_func(void *data)
{
   GAsyncQueue *queue = data;

   ALLEGRO_DEBUG("GLIB %d.%d.%d\n",
      GLIB_MAJOR_VERSION,
      GLIB_MINOR_VERSION,
      GLIB_MICRO_VERSION);

   g_async_queue_push(queue, ACK_OK);

   gtk_main();

   ALLEGRO_INFO("GTK stopped.\n");
   return NULL;
}


bool _al_gtk_ensure_thread(void)
{
   bool ok = true;

#if !NEWER_GLIB
   if (!g_thread_supported())
      g_thread_init(NULL);
#endif

#ifndef ALLEGRO_CFG_USE_GTKGLEXT
   int argc = 0;
   char **argv = NULL;
   gtk_init(&argc, &argv);
#endif

   nd_gtk_lock();

   if (!nd_gtk_thread) {
      GAsyncQueue *queue = g_async_queue_new();
#if NEWER_GLIB
      nd_gtk_thread = g_thread_new("gtk thread", nd_gtk_thread_func, queue);
#else
      bool joinable = FALSE;
      nd_gtk_thread = g_thread_create(nd_gtk_thread_func, queue, joinable, NULL);
#endif
      if (!nd_gtk_thread) {
         ok = false;
      }
      else {
         ok = (g_async_queue_pop(queue) == ACK_OK);
      }
      g_async_queue_unref(queue);
   }

   nd_gtk_unlock();

   return ok;
}

#endif /* ALLEGRO_CFG_USE_GTK */


#ifdef ALLEGRO_CFG_USE_GTKGLEXT

#include <gdk/x11/gdkglx.h>

/* forward declarations */
static gboolean xgtk_quit_callback(GtkWidget *widget, GdkEvent *event,
   ALLEGRO_DISPLAY *display);
static gboolean xgtk_window_state_callback(GtkWidget *widget,
   GdkEventWindowState *event, ALLEGRO_DISPLAY *display);
static gboolean xgtk_handle_motion_event(GtkWidget *drawing_area,
   GdkEventMotion *event, ALLEGRO_DISPLAY *display);
static gboolean xgtk_handle_button_press_event(GtkWidget *drawing_area,
   GdkEventButton *event, ALLEGRO_DISPLAY *display);
static gboolean xgtk_handle_button_release_event(GtkWidget *drawing_area,
   GdkEventButton *event, ALLEGRO_DISPLAY *display);
static gboolean xgtk_handle_key_press_event(GtkWidget *drawing_area,
   GdkEventKey *event, ALLEGRO_DISPLAY *display);
static gboolean xgtk_handle_key_release_event(GtkWidget *drawing_area,
   GdkEventKey *event, ALLEGRO_DISPLAY *display);
static gboolean xgtk_handle_configure_event(GtkWidget *widget,
   GdkEventConfigure *event, ALLEGRO_DISPLAY *display);


ALLEGRO_DISPLAY *_al_gtk_create_display(int w, int h)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   _al_mutex_lock(&system->lock);

   ALLEGRO_DISPLAY_XGLX *d = al_calloc(1, sizeof *d);
   ALLEGRO_DISPLAY *display = (void*)d;
   ALLEGRO_OGL_EXTRAS *ogl = al_calloc(1, sizeof *ogl);
   display->ogl_extras = ogl;

   display->ogl_extras->is_shared = true;

   /* FIXME: Use user preferences */
   display->extra_settings.settings[ALLEGRO_COMPATIBLE_DISPLAY] = 1;
   display->extra_settings.settings[ALLEGRO_RED_SIZE] = 8;
   display->extra_settings.settings[ALLEGRO_GREEN_SIZE] = 8;
   display->extra_settings.settings[ALLEGRO_BLUE_SIZE] = 8;
   display->extra_settings.settings[ALLEGRO_ALPHA_SIZE] = 8;
   display->extra_settings.settings[ALLEGRO_COLOR_SIZE] = 32;
   display->extra_settings.settings[ALLEGRO_RED_SHIFT] = 16;
   display->extra_settings.settings[ALLEGRO_GREEN_SHIFT] = 8;
   display->extra_settings.settings[ALLEGRO_BLUE_SHIFT] = 0;
   display->extra_settings.settings[ALLEGRO_ALPHA_SHIFT] = 24;

   int argc = 0;
   char **argv = NULL;
   gtk_init(&argc, &argv);
   gtk_gl_init(&argc, &argv);

   //system->x11display = gdk_x11_get_default_xdisplay();

   GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   d->gtkwindow = window;

   g_signal_connect(G_OBJECT(window), "delete-event",
      G_CALLBACK(xgtk_quit_callback), display);
   g_signal_connect(G_OBJECT(window), "window-state-event",
      G_CALLBACK(xgtk_window_state_callback), display);

   gtk_widget_set_events(window, GDK_STRUCTURE_MASK);

   /* Get automatically redrawn if any of their children changed allocation. */
   gtk_container_set_reallocate_redraws (GTK_CONTAINER (window), TRUE);

   display->w = w;
   display->h = h;
   display->vt = _al_display_xglx_driver();
   display->refresh_rate = 60;
   display->flags = al_get_new_display_flags();
   display->flags |= ALLEGRO_OPENGL;

   d->ignore_configure_event = true;
   d->is_fullscreen = false;

   gtk_window_set_resizable(GTK_WINDOW(d->gtkwindow), true);

   GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
   gtk_container_add(GTK_CONTAINER(window), vbox);

   d->gtkdrawing_area = gtk_drawing_area_new();

   gtk_widget_set_size_request(d->gtkdrawing_area, w, h);

   /* Set OpenGL-capability to the widget. */
   GdkGLConfig *glconfig =
      gdk_gl_config_new_by_mode(GDK_GL_MODE_RGB | GDK_GL_MODE_DOUBLE);

   gtk_widget_set_gl_capability (d->gtkdrawing_area, glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE);

   g_signal_connect(G_OBJECT(d->gtkdrawing_area), "motion-notify-event",
      G_CALLBACK(xgtk_handle_motion_event), display);
   g_signal_connect(G_OBJECT(d->gtkdrawing_area), "button-press-event",
      G_CALLBACK(xgtk_handle_button_press_event), display);
   g_signal_connect(G_OBJECT(d->gtkdrawing_area), "button-release-event",
      G_CALLBACK(xgtk_handle_button_release_event), display);
   g_signal_connect(G_OBJECT(d->gtkdrawing_area), "key-press-event",
      G_CALLBACK(xgtk_handle_key_press_event), display);
   g_signal_connect(G_OBJECT(d->gtkdrawing_area), "key-release-event",
      G_CALLBACK(xgtk_handle_key_release_event), display);
   g_signal_connect(G_OBJECT(d->gtkdrawing_area), "configure-event",
      G_CALLBACK(xgtk_handle_configure_event), display);

   gtk_box_pack_start(GTK_BOX(vbox), d->gtkdrawing_area, TRUE, TRUE, 0);
   gtk_widget_set_events(d->gtkdrawing_area,
      GDK_POINTER_MOTION_MASK |
      GDK_BUTTON_PRESS_MASK |
      GDK_BUTTON_RELEASE_MASK |
      GDK_KEY_PRESS_MASK |
      GDK_KEY_RELEASE_MASK |
      GDK_STRUCTURE_MASK
   );
   GTK_WIDGET_SET_FLAGS(d->gtkdrawing_area, GTK_CAN_FOCUS);

   gtk_widget_show_all(window);

   _al_gtk_ensure_thread();

   d->gtkcontext = gtk_widget_get_gl_context(d->gtkdrawing_area);
   d->gtkdrawable = gtk_widget_get_gl_drawable(d->gtkdrawing_area);
   d->context = gdk_x11_gl_context_get_glxcontext(d->gtkcontext);

   d->window = GDK_WINDOW_XWINDOW(d->gtkdrawing_area->window);

   ALLEGRO_DISPLAY_XGLX **add;
   add = _al_vector_alloc_back(&system->system.displays);
   *add = d;

   d->resize_count = 0;
   d->programmatic_resize = false;

   if (!(display->flags & ALLEGRO_RESIZABLE))
      _al_xwin_set_size_hints(display, 0, 0);

#if 0
   d->is_mapped = false;
   _al_cond_init(&d->mapped);

   XLockDisplay(system->x11display);

   d->wm_delete_window_atom = XInternAtom(system->x11display,
      "WM_DELETE_WINDOW", False);
   XSetWMProtocols(system->x11display, d->window, &d->wm_delete_window_atom, 1);

   XMapWindow(system->x11display, d->window);
   ALLEGRO_DEBUG("X11 window mapped.\n");

   XUnlockDisplay(system->x11display);
#endif

   /* Each display is an event source. */
   _al_event_source_init(&display->es);

   gdk_gl_drawable_gl_begin (d->gtkdrawable, d->gtkcontext);

   _al_ogl_manage_extensions(display);
   _al_ogl_set_extensions(ogl->extension_api);

   _al_ogl_setup_gl(display);

   d->invisible_cursor = None; /* Will be created on demand. */
   d->current_cursor = None; /* Initially, we use the root cursor. */
   d->cursor_hidden = false;

   d->icon = None;
   d->icon_mask = None;

   _al_mutex_unlock(&system->lock);

   if (display->flags & ALLEGRO_FULLSCREEN_WINDOW) {
      display->flags &= ~ALLEGRO_FULLSCREEN_WINDOW;
      _al_gtk_set_fullscreen_window(display, true);
   }

   return display;
}


static gboolean xgtk_quit_callback(GtkWidget *widget, GdkEvent *event,
   ALLEGRO_DISPLAY *display)
{
   (void)widget;
   (void)event;
   _al_display_xglx_closebutton(display, NULL);
   return TRUE;
}


static gboolean xgtk_window_state_callback(GtkWidget *widget,
   GdkEventWindowState *event, ALLEGRO_DISPLAY *display)
{
   (void)widget;
   ALLEGRO_DISPLAY_XGLX *d = (void *)display;
   if (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) {
      d->is_fullscreen = !d->is_fullscreen;
      return TRUE;
   }
   return FALSE;
}


static gboolean xgtk_handle_motion_event(GtkWidget *drawing_area,
   GdkEventMotion *event, ALLEGRO_DISPLAY *display)
{
   (void)drawing_area;
   _al_xwin_mouse_motion_notify_handler(event->x, event->y, display);
   return TRUE;
}


static gboolean xgtk_handle_button_press_event(GtkWidget *drawing_area,
   GdkEventButton *event, ALLEGRO_DISPLAY *display)
{
   (void)drawing_area;
   _al_xwin_mouse_button_press_handler(event->button, display);
   return TRUE;
}


static gboolean xgtk_handle_button_release_event(GtkWidget *drawing_area,
   GdkEventButton *event, ALLEGRO_DISPLAY *display)
{
   (void)drawing_area;
   _al_xwin_mouse_button_release_handler(event->button, display);
   return TRUE;
}


static gboolean xgtk_handle_key_press_event(GtkWidget *drawing_area,
   GdkEventKey *event, ALLEGRO_DISPLAY *display)
{
   uint32_t unichar = 0;
   (void)drawing_area;

   memcpy(&unichar, event->string, _ALLEGRO_MIN(4, event->length));
   _al_xwin_keyboard_handler_alternative(true, event->hardware_keycode,
      unichar, display);
   return TRUE;
}


static gboolean xgtk_handle_key_release_event(GtkWidget *drawing_area,
   GdkEventKey *event, ALLEGRO_DISPLAY *display)
{
   (void)drawing_area;

   _al_xwin_keyboard_handler_alternative(false, event->hardware_keycode,
      0, display);
   return TRUE;
}


static gboolean xgtk_handle_configure_event(GtkWidget *widget,
   GdkEventConfigure *event, ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_XGLX *d = (void *)display;
   (void)widget;
   if (d->ignore_configure_event) {
      d->ignore_configure_event = false;
      return TRUE;
   }
   d->cfg_w = event->width;
   d->cfg_h = event->height;
   _al_xglx_display_configure(display, event->x, event->y, d->cfg_w, d->cfg_h,
      false /* FIXME: don't know what to pass for this */);
   return TRUE;
}


void _al_gtk_set_fullscreen_window(ALLEGRO_DISPLAY *display, bool onoff)
{
   ALLEGRO_DISPLAY_XGLX *d = (void *)display;
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX*)al_get_system_driver();
   if (onoff == (display->flags & ALLEGRO_FULLSCREEN_WINDOW)) {
      return;
   }
   display->flags ^= ALLEGRO_FULLSCREEN_WINDOW;
   d->ignore_configure_event = true;
   _al_xwin_reset_size_hints(display);
   if (onoff) {
      ALLEGRO_MONITOR_INFO mi;
      d->toggle_w = display->w;
      d->toggle_h = display->h;
      gtk_window_fullscreen(GTK_WINDOW(d->gtkwindow));
      _al_xglx_get_monitor_info(system, d->adapter, &mi);
      d->cfg_w = mi.x2 - mi.x1;
      d->cfg_h = mi.y2 - mi.y1;
   }
   else {
      gtk_window_unfullscreen(GTK_WINDOW(d->gtkwindow));
      d->cfg_w = d->toggle_w;
      d->cfg_h = d->toggle_h;
   }

   _al_gtk_acknowledge_resize(display);

   /* window-state-event sets this upon fullscreen change */
   while (d->is_fullscreen != onoff) {
      al_rest(0.001);
   }

   _al_xwin_set_size_hints(display, INT_MAX, INT_MAX);
}


bool _al_gtk_acknowledge_resize(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;

   _al_mutex_lock(&system->lock);

   if (glx->context) {
      d->w = glx->cfg_w;
      d->h = glx->cfg_h;

      _al_ogl_setup_gl(d);
   }

   _al_mutex_unlock(&system->lock);

   return true;
}


GtkWidget *_al_gtk_get_window(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_XGLX *d = (void *)display;
   return d->gtkwindow;
}


#endif /* ALLEGRO_CFG_USE_GTKGLEXT */

/* vim: set sts=3 sw=3 et: */
