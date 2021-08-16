#include <gtk/gtk.h>
#include <gtk/gtkx.h>

#define ALLEGRO_INTERNAL_UNSTABLE
#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_native_dialog_cfg.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xdisplay.h"
#include "allegro5/internal/aintern_xevents.h"
#include "allegro5/internal/aintern_xsystem.h"

#include "gtk_dialog.h"
#include "gtk_xgtk.h"

ALLEGRO_DEBUG_CHANNEL("gtk")


typedef struct ARGS_CREATE
{
   ARGS_BASE base; /* must be first */
   ALLEGRO_DISPLAY_XGLX *display;
   int w;
   int h;
   const char *title;
} ARGS_CREATE;

typedef struct
{
   ARGS_BASE base; /* must be first */
   ALLEGRO_DISPLAY_XGLX *display;
   bool is_last;
} ARGS_DESTROY;

typedef struct
{
   ARGS_BASE base; /* must be first */
   ALLEGRO_DISPLAY_XGLX *display;
   int w;
   int h;
} ARGS_RESIZE;

typedef struct
{
   ARGS_BASE base; /* must be first */
   ALLEGRO_DISPLAY_XGLX *display;
   const char *title;
} ARGS_TITLE;

typedef struct
{
   ARGS_BASE base; /* must be first */
   ALLEGRO_DISPLAY_XGLX *display;
   bool fullscreen;
} ARGS_FULLSCREEN_WINDOW;

typedef struct
{
   ARGS_BASE base; /* must be first */
   ALLEGRO_DISPLAY_XGLX *display;
   int x;
   int y;
} ARGS_POSITION;


/* forward declarations */
static gboolean xgtk_quit_callback(GtkWidget *widget, GdkEvent *event,
   ALLEGRO_DISPLAY *display);
static gboolean xgtk_handle_configure_event(GtkWidget *widget,
   GdkEventConfigure *event, ALLEGRO_DISPLAY *display);
static void xgtk_set_fullscreen_window(ALLEGRO_DISPLAY *display, bool onoff);

static struct ALLEGRO_XWIN_DISPLAY_OVERRIDABLE_INTERFACE xgtk_override_vt;


/* [gtk thread] */
static gboolean do_create_display_hook(gpointer data)
{
   const ARGS_CREATE *args = _al_gtk_lock_args(data);
   ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY *)args->display;
   ALLEGRO_DISPLAY_XGLX *d = args->display;
   const int w = args->w;
   const int h = args->h;

   GtkWidget *window;
   GtkWidget *vbox;
   GtkWidget *socket;

   window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   d->gtk->gtkwindow = window;
   gtk_window_set_default_size(GTK_WINDOW(window), w, h);

   g_signal_connect(G_OBJECT(window), "delete-event",
      G_CALLBACK(xgtk_quit_callback), display);
   g_signal_connect(G_OBJECT(window), "configure-event",
      G_CALLBACK(xgtk_handle_configure_event), display);

   vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
   gtk_container_add(GTK_CONTAINER(window), vbox);

   socket = gtk_socket_new();
   d->gtk->gtksocket = socket;
   gtk_box_pack_end(GTK_BOX(vbox), socket, TRUE, TRUE, 0);
   gtk_socket_add_id(GTK_SOCKET(socket), d->window);
   ALLEGRO_DEBUG("gtk_socket_add_id: window = %ld\n", d->window);

   gtk_window_set_title(GTK_WINDOW(window), args->title);

   gtk_widget_show_all(window);

   if (display->flags & ALLEGRO_RESIZABLE) {
      /* Allow socket widget to be resized smaller than initial size. */
      gtk_widget_set_size_request(socket, -1, -1);
      gtk_window_set_resizable(GTK_WINDOW(window), true);
   }
   else {
      gtk_window_set_resizable(GTK_WINDOW(window), false);
   }

   if (display->flags & ALLEGRO_FULLSCREEN_WINDOW) {
      gtk_window_fullscreen(GTK_WINDOW(window));
   }

   d->overridable_vt = &xgtk_override_vt;

   return _al_gtk_release_args(data);
}


/* [user thread] */
static bool xgtk_create_display_hook(ALLEGRO_DISPLAY *display, int w, int h)
{
   ALLEGRO_DISPLAY_XGLX *d = (ALLEGRO_DISPLAY_XGLX *)display;
   ARGS_CREATE args;

   d->gtk = al_calloc(1, sizeof(*(d->gtk)));
   if (!d->gtk) {
      ALLEGRO_WARN("Out of memory\n");
      return false;
   }

   if (!_al_gtk_ensure_thread()) {
      al_free(d->gtk);
      d->gtk = NULL;
      return false;
   }

   if (!_al_gtk_init_args(&args, sizeof(args))) {
      al_free(d->gtk);
      d->gtk = NULL;
      return false;
   }

   args.display = d;
   args.w = w;
   args.h = h;
   args.title = al_get_new_window_title();

   return _al_gtk_wait_for_args(do_create_display_hook, &args);
}


static gboolean xgtk_quit_callback(GtkWidget *widget, GdkEvent *event,
   ALLEGRO_DISPLAY *display)
{
   (void)widget;
   (void)event;
   _al_display_xglx_closebutton(display, NULL);
   return TRUE;
}


static gboolean xgtk_handle_configure_event(GtkWidget *widget,
   GdkEventConfigure *event, ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_XGLX *d = (ALLEGRO_DISPLAY_XGLX *)display;
   (void)widget;
   (void)event;

   /* Update our idea of the window position.
    * event->x, event->y is incorrect.
    */
   gtk_window_get_position(GTK_WINDOW(d->gtk->gtkwindow), &d->x, &d->y);

   return FALSE;
}


/* [gtk thread] */
static gboolean do_destroy_display_hook(gpointer data)
{
   ARGS_DESTROY *args = _al_gtk_lock_args(data);
   ALLEGRO_DISPLAY_XGLX *d = args->display;
   bool is_last = args->is_last;

   gtk_widget_destroy(d->gtk->gtkwindow);

   if (is_last) {
      gtk_main_quit();
   }

   return _al_gtk_release_args(data);
}


/* [user thread] */
static void xgtk_destroy_display_hook(ALLEGRO_DISPLAY *display, bool is_last)
{
   ALLEGRO_DISPLAY_XGLX *d = (ALLEGRO_DISPLAY_XGLX *)display;
   ARGS_DESTROY args;

   if (!_al_gtk_init_args(&args, sizeof(args)))
      return;

   args.display = d;
   args.is_last = is_last;

   _al_gtk_wait_for_args(do_destroy_display_hook, &args);

   al_free(d->gtk);
   d->gtk = NULL;
}


/* [gtk thread] */
static gboolean do_resize_display1(gpointer data)
{
   ARGS_RESIZE *args = _al_gtk_lock_args(data);
   ALLEGRO_DISPLAY_XGLX *d = args->display;
   int w = args->w;
   int h = args->h;

   /* Using gtk_window_resize by itself is wrong because it does not take
    * into account the space occupied by other widgets in the window.
    *
    * Using gtk_widget_set_size_request by itself is also insufficient as it
    * sets the *minimum* size. If the socket widget was already larger then
    * it would do nothing.
    */
   gtk_window_resize(GTK_WINDOW(d->gtk->gtkwindow), w, h);
   gtk_widget_set_size_request(d->gtk->gtksocket, w, h);

   return _al_gtk_release_args(data);
}


/* [gtk thread] */
static gboolean do_resize_display2(gpointer data)
{
   ARGS_RESIZE *args = _al_gtk_lock_args(data);
   ALLEGRO_DISPLAY_XGLX *d = args->display;

   /* Remove the minimum size constraint again. */
   gtk_widget_set_size_request(d->gtk->gtksocket, -1, -1);

   return _al_gtk_release_args(data);
}


/* [user thread] */
static bool xgtk_resize_display(ALLEGRO_DISPLAY *display, int w, int h)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *d = (ALLEGRO_DISPLAY_XGLX *)display;
   bool ret = true;

   _al_mutex_lock(&system->lock);

   if (w != display->w || h != display->h) {
      do {
         const int old_resize_count = d->resize_count;
         ARGS_RESIZE args;

         d->programmatic_resize = true;

         if (!_al_gtk_init_args(&args, sizeof(args))) {
            ret = false;
            break;
         }
         args.display = d;
         args.w = w;
         args.h = h;
         _al_gtk_wait_for_args(do_resize_display1, &args);

         _al_display_xglx_await_resize(display, old_resize_count, false);

         if (_al_gtk_init_args(&args, sizeof(args))) {
            args.display = d;
            _al_gtk_wait_for_args(do_resize_display2, &args);
         }

         d->programmatic_resize = false;
      } while (0);
   }

   _al_mutex_unlock(&system->lock);

   return ret;
}


/* [gtk thread] */
static gboolean do_set_window_title(gpointer data)
{
   ARGS_TITLE *args = _al_gtk_lock_args(data);
   ALLEGRO_DISPLAY_XGLX *d = args->display;
   const char *title = args->title;

   gtk_window_set_title(GTK_WINDOW(d->gtk->gtkwindow), title);

   return _al_gtk_release_args(data);
}


/* [user thread] */
static void xgtk_set_window_title(ALLEGRO_DISPLAY *display, const char *title)
{
   ARGS_TITLE args;

   if (_al_gtk_init_args(&args, sizeof(args))) {
      args.display = (ALLEGRO_DISPLAY_XGLX *)display;
      args.title = title;
      _al_gtk_wait_for_args(do_set_window_title, &args);
   }
}


/* [gtk thread] */
static gboolean do_set_fullscreen_window(gpointer data)
{
   ARGS_FULLSCREEN_WINDOW *args = _al_gtk_lock_args(data);
   ALLEGRO_DISPLAY_XGLX *d = args->display;
   bool fullscreen = args->fullscreen;

   if (fullscreen) {
      gtk_window_fullscreen(GTK_WINDOW(d->gtk->gtkwindow));
   }
   else {
      gtk_window_unfullscreen(GTK_WINDOW(d->gtk->gtkwindow));
   }

   return _al_gtk_release_args(args);
}


/* [user thread] */
static void xgtk_set_fullscreen_window(ALLEGRO_DISPLAY *display, bool onoff)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *d = (ALLEGRO_DISPLAY_XGLX *)display;

   if (onoff == (display->flags & ALLEGRO_FULLSCREEN_WINDOW)) {
      return;
   }

   _al_mutex_lock(&system->lock);
   {
      int old_resize_count;
      ARGS_FULLSCREEN_WINDOW args;

      display->flags ^= ALLEGRO_FULLSCREEN_WINDOW;
      old_resize_count = d->resize_count;
      d->programmatic_resize = true;

      if (_al_gtk_init_args(&args, sizeof(args))) {
         args.display = d;
         args.fullscreen = onoff;
         _al_gtk_wait_for_args(do_set_fullscreen_window, &args);

         _al_display_xglx_await_resize(display, old_resize_count,
            (display->flags & ALLEGRO_FULLSCREEN));
      }

      d->programmatic_resize = false;
   }
   _al_mutex_unlock(&system->lock);
}


/* [gtk thread] */
static gboolean do_set_window_position(gpointer data)
{
   ARGS_POSITION *args = _al_gtk_lock_args(data);

   gtk_window_move(GTK_WINDOW(args->display->gtk->gtkwindow),
      args->x, args->y);

   return _al_gtk_release_args(args);
}


/* [user thread] */
static void xgtk_set_window_position(ALLEGRO_DISPLAY *display, int x, int y)
{
   ARGS_POSITION args;

   if (_al_gtk_init_args(&args, sizeof(args))) {
      args.display = (ALLEGRO_DISPLAY_XGLX *)display;
      args.x = x;
      args.y = y;
      _al_gtk_wait_for_args(do_set_window_position, &args);
   }
}


/* [user thread] */
static bool xgtk_set_window_constraints(ALLEGRO_DISPLAY *display,
   int min_w, int min_h, int max_w, int max_h)
{
   // FIXME
   (void)display;
   (void)min_w;
   (void)min_h;
   (void)max_w;
   (void)max_h;
   return true;
}


static struct ALLEGRO_XWIN_DISPLAY_OVERRIDABLE_INTERFACE xgtk_override_vt =
{
   xgtk_create_display_hook,
   xgtk_destroy_display_hook,
   xgtk_resize_display,
   xgtk_set_window_title,
   xgtk_set_fullscreen_window,
   xgtk_set_window_position,
   xgtk_set_window_constraints
};


bool _al_gtk_set_display_overridable_interface(bool on)
{
   return _al_xwin_set_gtk_display_overridable_interface(ALLEGRO_VERSION_INT,
      (on) ? &xgtk_override_vt : NULL);
}


GtkWidget *_al_gtk_get_window(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_XGLX *d = (ALLEGRO_DISPLAY_XGLX *)display;

   if (d->overridable_vt == &xgtk_override_vt) {
      return d->gtk->gtkwindow;
   }

   ALLEGRO_WARN("Not display created with GTK.\n");
   return NULL;
}


/* vim: set sts=3 sw=3 et: */
