#include "xglx.h"

// FIXME: required and suggested setting should be treated differently

static int allegro_to_glx_setting[] = {
   GLX_RED_SIZE,
   GLX_GREEN_SIZE,
   GLX_BLUE_SIZE,
   GLX_ALPHA_SIZE,
   0 /*shifts*/,
   0,
   0,
   0,
   GLX_ACCUM_RED_SIZE,
   GLX_ACCUM_GREEN_SIZE,
   GLX_ACCUM_BLUE_SIZE,
   GLX_ACCUM_ALPHA_SIZE,
   GLX_STEREO,
   GLX_AUX_BUFFERS,
   0 /*ALLEGRO_COLOR_SIZE*/,
   GLX_DEPTH_SIZE,
   GLX_STENCIL_SIZE,
   GLX_SAMPLE_BUFFERS,
   GLX_SAMPLES,
   0, /*ALLEGRO_RENDER_METHOD*/
   0, /*GLX_RGBA_FLOAT_BIT*/
   0, /*ALLEGRO_FLOAT_DEPTH*/
   GLX_DOUBLEBUFFER,
   0, /*GLX_SWAP_METHOD_OML*/
   0  /*COMPATIBLE_DISPLAY*/
};


static char const *names[] = {
   "GLX_RED_SIZE",
   "GLX_GREEN_SIZE",
   "GLX_BLUE_SIZE",
   "GLX_ALPHA_SIZE",
   "GLX_RED_SHIFT",
   "GLX_GREEN_SHIFT",
   "GLX_BLUE_SHIFT",
   "GLX_ALPHA_SHIFT",
   "GLX_ACCUM_RED_SIZE",
   "GLX_ACCUM_GREEN_SIZE",
   "GLX_ACCUM_BLUE_SIZE",
   "GLX_ACCUM_ALPHA_SIZE",
   "GLX_STEREO",
   "GLX_AUX_BUFFERS",
   "",
   "GLX_DEPTH_SIZE",
   "GLX_STENCIL_SIZE",
   "GLX_SAMPLE_BUFFERS",
   "GLX_SAMPLES",
   "",
   "",
   "",
   "GLX_DOUBLEBUFFER",
   "",
   ""
};

static int add_glx_options(ALLEGRO_EXTRA_DISPLAY_SETTINGS *extras,
   int *attributes, int a)
{
   int i;
   if (!extras) return a;
   for (i = 0; i < ALLEGRO_DISPLAY_OPTIONS_COUNT; i++) {
      if (((extras->required & (1 << i)) ||
         (extras->suggested & (1 << i))) && allegro_to_glx_setting[i]) {
         attributes[a++] = allegro_to_glx_setting[i];
         attributes[a++] = extras->settings[i];
         TRACE("xglx: %s=%d\n", names[i], extras->settings[i]);
      }
   }
   return a;
}

/* Let GLX choose an appropriate visual. */
static void choose_visual_fbconfig(ALLEGRO_DISPLAY_XGLX *glx)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *extras;
   int attributes[256];
   int a = 0;
   int nelements;
   extras = _al_get_new_display_settings();

   a = add_glx_options(extras, attributes, a);
   attributes[a++] = None;

   glx->fbc = glXChooseFBConfig(system->gfxdisplay,
      glx->xscreen, attributes, &nelements);
   ASSERT(!glx->xvinfo);
   glx->xvinfo = glXGetVisualFromFBConfig(system->gfxdisplay, glx->fbc[0]);
}

/* Use the old manual selection to find the visual to use. */
static void choose_visual_old(ALLEGRO_DISPLAY_XGLX *glx)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *extras;
   int attributes[256];
   int a = 0;
   extras = _al_get_new_display_settings();
   
   attributes[a++] = GLX_RGBA;
   
   /* glXChooseVisual will fail for values like GLX_SAMPLE_BUFFERS
	* if the extension is not supported.
   a = add_glx_options(extras, attributes, a);*/
   if ((glx->display.flags & ALLEGRO_SINGLEBUFFER) == 0)
      attributes[a++] = GLX_DOUBLEBUFFER;
   attributes[a++] = None;
   ASSERT(!glx->xvinfo);
   glx->xvinfo = glXChooseVisual(system->gfxdisplay, glx->xscreen,
      attributes);
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
    ALLEGRO_DISPLAY *disp = (void*)glx;
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

    disp->ogl_extras->is_shared = true;

   TRACE("xglx_config: Got GLX context.\n");
}
