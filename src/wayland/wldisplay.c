#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_wldisplay.h"
#include "allegro5/internal/aintern_display.h"

#include <wayland-client.h>

static ALLEGRO_DISPLAY_INTERFACE wldpy_vt;

typedef struct ALLEGRO_DISPLAY_WL ALLEGRO_DISPLAY_WL;

/* ALLEGRO_DISPLAY with Wayland-specific data */
struct ALLEGRO_DISPLAY_WL {
    /* Needs to be first member */
    ALLEGRO_DISPLAY display;
};

static ALLEGRO_DISPLAY *wldpy_create_display(int w, int h)
{
    ALLEGRO_DISPLAY_WL *d = al_calloc(1, sizeof *d);
    ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY*) d;

    display->w = w;
    display->h = h;
    display->vt = &wldpy_vt;

    return NULL;
}

static void wldpy_destroy_display(ALLEGRO_DISPLAY* display)
{

}