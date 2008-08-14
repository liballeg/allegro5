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
    glx->fbc = glXChooseFBConfig(system->gfxdisplay,
        glx->xscreen, attributes, &nelements);
    glx->xvinfo = glXGetVisualFromFBConfig(system->gfxdisplay, glx->fbc[0]);
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
    glx->xvinfo = glXChooseVisual(system->gfxdisplay, glx->xscreen, attributes);
}

void _al_xglx_config_select_visual(ALLEGRO_DISPLAY_XGLX *glx)
{
    if (glx->glx_version < 130)
        choose_visual_old(glx);
    else
        choose_visual_fbconfig(glx);
}

void _al_xglx_config_create_context(ALLEGRO_DISPLAY_XGLX *glx)
{
    ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();
    GLXContext existing_ctx = NULL;

    /* Find an existing context with which to share display lists. */
    if (_al_vector_size(&system->system.displays) > 1) {
        ALLEGRO_DISPLAY_XGLX **existing_dpy;
        existing_dpy = _al_vector_ref_front(&system->system.displays);
        if (*existing_dpy != glx)
            existing_ctx = (*existing_dpy)->context;
    }

    if (glx->fbc) {
        /* Create a GLX context from FBC. */
        glx->context = glXCreateNewContext(system->gfxdisplay, glx->fbc[0],
            GLX_RGBA_TYPE, existing_ctx, True);
        /* Create a GLX subwindow inside our window. */
        glx->glxwindow = glXCreateWindow(system->gfxdisplay, glx->fbc[0],
            glx->window, 0);
    }
    else {
        /* Create a GLX context from visual info. */
        glx->context = glXCreateContext(system->gfxdisplay, glx->xvinfo,
            existing_ctx, True);
        glx->glxwindow = glx->window;
    }

   TRACE("xglx_config: Got GLX context.\n");
}
