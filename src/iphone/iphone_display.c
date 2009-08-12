#include <allegro5/allegro5.h>
#include <allegro5/allegro_opengl.h>
#include <allegro5/internal/aintern_iphone.h>
#include <allegro5/internal/aintern_memory.h>
#include <allegro5/internal/aintern_opengl.h>

static ALLEGRO_DISPLAY_INTERFACE *vt;

void _al_iphone_setup_opengl_view(ALLEGRO_DISPLAY *d)
{
    _al_iphone_reset_framebuffer();
    glViewport(0, 0, 320, 480);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    glOrthof(0, 320, 480, 0, -1, 1);
   
    /* We automatically adjust the view if the user doesn't use 320x480. Users
     * of the iphone port are adviced to provide a 320x480 mode and do their
     * own adjustment - but for the sake of allowing ports without knowing
     * any OpenGL and not having to change a single character in your
     * application - here you go.
     */
    if (d->w != 320 || d->h != 480) {
       double scale;
       if (d->w >= d->h) {
          if (d->w * 320 > d->h * 480.0) {
             scale = 480.0 / d->w;
             glTranslatef((320.0 - d->h * scale) * 0.5, 0, 0);
          }
          else {
             scale = 320.0 / d->h;
             glTranslatef(0, (480.0 - d->w * scale) * 0.5, 0);
          }
          glTranslatef(320, 0, 0);
          glRotatef(90, 0, 0, 1);
          glScalef(scale, scale, 1);
       }
       else {
          // TODO
       }
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

/* Helper to set up GL state as we want it. */
static void setup_gl(ALLEGRO_DISPLAY *d)
{
    ALLEGRO_OGL_EXTRAS *ogl = d->ogl_extras;

    if (ogl->backbuffer)
        _al_ogl_resize_backbuffer(ogl->backbuffer, d->w, d->h);
    else
        ogl->backbuffer = _al_ogl_create_backbuffer(d);

    _al_iphone_setup_opengl_view(d);
}

static ALLEGRO_DISPLAY *iphone_create_display(int w, int h)
{
    ALLEGRO_DISPLAY_IPHONE *d = _AL_MALLOC(sizeof *d);
    ALLEGRO_DISPLAY *display = (void*)d;
    ALLEGRO_OGL_EXTRAS *ogl = _AL_MALLOC(sizeof *ogl);
    memset(d, 0, sizeof *d);
    memset(ogl, 0, sizeof *ogl);
    display->ogl_extras = ogl;
    display->vt = _al_get_iphone_display_interface();
    display->w = w;
    display->h = h;

    ALLEGRO_SYSTEM_IPHONE *system = (void *)al_system_driver();

    /* Add ourself to the list of displays. */
    ALLEGRO_DISPLAY_IPHONE **add;
    add = _al_vector_alloc_back(&system->system.displays);
    *add = d;
    
    /* Each display is an event source. */
    _al_event_source_init(&display->es);
    
    /* This will add an OpenGL view with an OpenGL context, then return. */
    _al_iphone_add_view(display);
    _al_iphone_make_view_current();

    _al_ogl_manage_extensions(display);
    _al_ogl_set_extensions(ogl->extension_api);
    setup_gl(display);
    
    display->flags |= ALLEGRO_OPENGL;


    return display;
}

static void iphone_destroy_display(ALLEGRO_DISPLAY *d)
{
    // FIXME: not supported yet
}

static bool iphone_set_current_display(ALLEGRO_DISPLAY *d)
{
    _al_iphone_make_view_current();
    return true;
}

/* There can be only one window and only one OpenGL context, so all bitmaps
 * are compatible.
 */
static bool iphone_is_compatible_bitmap(ALLEGRO_DISPLAY *display,
                                      ALLEGRO_BITMAP *bitmap)
{
    return true;
}

/* Resizing is not possible. */
static bool iphone_resize_display(ALLEGRO_DISPLAY *d, int w, int h)
{
    return false;
}

/* The icon must be provided in the Info.plist file, it cannot be changed
 * at runtime.
 */
static void iphone_set_icon(ALLEGRO_DISPLAY *d, ALLEGRO_BITMAP *bitmap)
{
}

/* There is no way to leave fullscreen so no window title is visible. */
static void iphone_set_window_title(ALLEGRO_DISPLAY *display, char const *title)
{
}

/* The window always spans the entire screen right now. */
static void iphone_set_window_position(ALLEGRO_DISPLAY *display, int x, int y)
{
}

/* Always fullscreen. */
static void iphone_toggle_frame(ALLEGRO_DISPLAY *display, bool onoff)
{
}

static void iphone_get_window_position(ALLEGRO_DISPLAY *display, int *x, int *y)
{
    *x = 0;
    *y = 0;
}

static bool iphone_wait_for_vsync(ALLEGRO_DISPLAY *display)
{
    return false;
}

static void iphone_flip_display(ALLEGRO_DISPLAY *d)
{
    _al_iphone_flip_view();
}

static void iphone_update_display_region(ALLEGRO_DISPLAY *d, int x, int y,
                                       int w, int h)
{
    iphone_flip_display(d);
}

static bool iphone_acknowledge_resize(ALLEGRO_DISPLAY *d)
{
    return false;
}


static ALLEGRO_MOUSE_CURSOR *iphone_create_mouse_cursor(
    ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bmp, int x_focus, int y_focus)
{
    return NULL;
}



static void iphone_destroy_mouse_cursor(ALLEGRO_DISPLAY *display,
                                      ALLEGRO_MOUSE_CURSOR *cursor)
{
}

static bool iphone_set_mouse_cursor(ALLEGRO_DISPLAY *display,
                                  ALLEGRO_MOUSE_CURSOR *cursor)
{
    return false;
}

static bool iphone_set_system_mouse_cursor(ALLEGRO_DISPLAY *display,
                                         ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id)
{
    return false;
}

static bool iphone_show_mouse_cursor(ALLEGRO_DISPLAY *display)
{
    return false;
}

static bool iphone_hide_mouse_cursor(ALLEGRO_DISPLAY *display)
{
    return false;
}

/* Obtain a reference to this driver. */
ALLEGRO_DISPLAY_INTERFACE *_al_get_iphone_display_interface(void)
{
    if (vt)
        return vt;
    
    vt = _AL_MALLOC(sizeof *vt);
    memset(vt, 0, sizeof *vt);
    
    vt->create_display = iphone_create_display;
    vt->destroy_display = iphone_destroy_display;
    vt->set_current_display = iphone_set_current_display;
    vt->flip_display = iphone_flip_display;
    vt->update_display_region = iphone_update_display_region;
    vt->acknowledge_resize = iphone_acknowledge_resize;
    vt->create_bitmap = _al_ogl_create_bitmap;
    vt->create_sub_bitmap = _al_ogl_create_sub_bitmap;
    vt->get_backbuffer = _al_ogl_get_backbuffer;
    vt->get_frontbuffer = _al_ogl_get_backbuffer;
    vt->set_target_bitmap = _al_ogl_set_target_bitmap;

    vt->is_compatible_bitmap = iphone_is_compatible_bitmap;
    vt->resize_display = iphone_resize_display;
    vt->set_icon = iphone_set_icon;
    vt->set_window_title = iphone_set_window_title;
    vt->set_window_position = iphone_set_window_position;
    vt->get_window_position = iphone_get_window_position;
    vt->toggle_frame = iphone_toggle_frame;
    vt->wait_for_vsync = iphone_wait_for_vsync;
    
    vt->create_mouse_cursor = iphone_create_mouse_cursor;
    vt->destroy_mouse_cursor = iphone_destroy_mouse_cursor;
    vt->set_mouse_cursor = iphone_set_mouse_cursor;
    vt->set_system_mouse_cursor = iphone_set_system_mouse_cursor;
    vt->show_mouse_cursor = iphone_show_mouse_cursor;
    vt->hide_mouse_cursor = iphone_hide_mouse_cursor;

    _al_ogl_add_drawing_functions(vt);
    
    return vt;
}