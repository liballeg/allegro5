#ifndef __al_included_allegro5_aintern_wl_h
#define __al_included_allegro5_aintern_wl_h

#include <wayland-client.h>

/* The Wayland backend uses EGL for rendering, and the EGL display is
 * derived from the wl_display, so all Wayland code can rely on the
 * EGL headers being available. */
#include <EGL/egl.h>
#include <EGL/eglext.h>

/* libdecor handles window decorations (and xdg surface management).
 * Only opaque pointers live in our structs; the full API is used from
 * the .c files via <libdecor.h>. */
struct libdecor;
struct libdecor_frame;

typedef struct ALLEGRO_SYSTEM_WAYLAND ALLEGRO_SYSTEM_WAYLAND;
typedef struct ALLEGRO_DISPLAY_WAYLAND ALLEGRO_DISPLAY_WAYLAND;

#endif