#include "xglx.h"

/* Let GLX choose an appropriate visual. */
static void choose_visual_fbconfig(ALLEGRO_DISPLAY_XGLX *glx)
{
    ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();
    int double_buffer = True;
    int red_bits = 0;
    int green_bits = 0;
    int blue_bits = 0;
    int alpha_bits = 0;
    if (glx->ogl_display.display.flags & ALLEGRO_SINGLEBUFFER)
        double_buffer = False;
    int attributes[]  = {
        GLX_DOUBLEBUFFER, double_buffer,
        GLX_RED_SIZE, red_bits,
        GLX_GREEN_SIZE, green_bits,
        GLX_BLUE_SIZE, blue_bits,
        GLX_ALPHA_SIZE, alpha_bits, 
        None};
    int nelements;
    glx->fbc = glXChooseFBConfig(system->xdisplay,
        glx->xscreen, attributes, &nelements);
    glx->xvinfo = glXGetVisualFromFBConfig(system->xdisplay, glx->fbc[0]);
}

/* Use the old manual selection to find the visual to use. */
static void choose_visual_old(ALLEGRO_DISPLAY_XGLX *glx)
{
    ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();
    int double_buffer = True;
    int red_bits = 0;
    int green_bits = 0;
    int blue_bits = 0;
    int alpha_bits = 0;
    if (glx->ogl_display.display.flags & ALLEGRO_SINGLEBUFFER)
        double_buffer = False;
    int attributes[] = {
        GLX_RGBA,
        GLX_RED_SIZE, red_bits,
        GLX_GREEN_SIZE, green_bits,
        GLX_BLUE_SIZE, blue_bits,
        GLX_ALPHA_SIZE, alpha_bits, 
        double_buffer ? GLX_DOUBLEBUFFER : None,
        None,
        };
    glx->xvinfo = glXChooseVisual(system->xdisplay, glx->xscreen, attributes);
}

void _xglx_config_select_visual(ALLEGRO_DISPLAY_XGLX *glx)
{
    if (glx->glx_version < 1.3)
        choose_visual_old(glx);
    else
        choose_visual_fbconfig(glx);
}

void _xglx_config_create_context(ALLEGRO_DISPLAY_XGLX *glx)
{
    ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();
    if (glx->fbc) {
        /* Create a GLX context from FBC. */
        glx->context = glXCreateNewContext(system->xdisplay, glx->fbc[0],
            GLX_RGBA_TYPE, NULL, True);
        /* Create a GLX subwindow inside our window. */
        glx->glxwindow = glXCreateWindow(system->xdisplay, glx->fbc[0],
            glx->window, 0);
    }
    else {
        /* Create a GLX context from visual info. */
        glx->context = glXCreateContext(system->xdisplay, glx->xvinfo,
            NULL, True);
        glx->glxwindow = glx->window;
    }

   TRACE("xglx_config: Got GLX context.\n");
}
