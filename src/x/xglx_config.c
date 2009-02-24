#include "xglx.h"

#define PREFIX_I                "xglx-config INFO: "
#define PREFIX_W                "xglx-config WARNING: "
#define PREFIX_E                "xglx-config ERROR: "

#ifdef DEBUGMODE
static void display_pixel_format(ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds)
{
   TRACE(PREFIX_I "Doublebuffer: %s\n", eds->settings[ALLEGRO_DOUBLEBUFFERED] ? "yes" : "no");
   if (eds->settings[ALLEGRO_SWAP_METHOD] > 0)
      TRACE(PREFIX_I "Swap method: %s\n", eds->settings[ALLEGRO_SWAP_METHOD] == 2 ? "flip" : "copy");
   else
      TRACE(PREFIX_I "Swap method: undefined\n");
   TRACE(PREFIX_I "Color format: r%i g%i b%i a%i, %i bit\n",
      eds->settings[ALLEGRO_RED_SIZE],
      eds->settings[ALLEGRO_GREEN_SIZE],
      eds->settings[ALLEGRO_BLUE_SIZE],
      eds->settings[ALLEGRO_ALPHA_SIZE],
      eds->settings[ALLEGRO_COLOR_SIZE]);
   TRACE(PREFIX_I "Depth buffer: %i bits\n", eds->settings[ALLEGRO_DEPTH_SIZE]);
   TRACE(PREFIX_I "Sample buffers: %s\n", eds->settings[ALLEGRO_SAMPLE_BUFFERS] ? "yes" : "no");
   TRACE(PREFIX_I "Samples: %i\n", eds->settings[ALLEGRO_SAMPLES]);
}
#endif

static int get_shift(int mask)
{
   int i = 0, j = 1;
   if (!mask) return -1;
   while (!(j & mask)) {
      i++;
      j <<= 1;
   }
   return i;
}

static void figure_out_colors(ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds, XVisualInfo *v)
{
   eds->settings[ALLEGRO_RED_SHIFT]   = get_shift(v->red_mask);
   eds->settings[ALLEGRO_GREEN_SHIFT] = get_shift(v->green_mask);
   eds->settings[ALLEGRO_BLUE_SHIFT]  = get_shift(v->blue_mask);
   eds->settings[ALLEGRO_ALPHA_SHIFT] = 0;

   eds->settings[ALLEGRO_COLOR_SIZE] = 0;

   if (eds->settings[ALLEGRO_RED_SIZE] == 3
    && eds->settings[ALLEGRO_GREEN_SIZE] == 3
    && eds->settings[ALLEGRO_BLUE_SIZE] == 2) {
      eds->settings[ALLEGRO_COLOR_SIZE] = 8;
   }

   if (eds->settings[ALLEGRO_RED_SIZE] == 5
    && eds->settings[ALLEGRO_BLUE_SIZE] == 5) {
      if (eds->settings[ALLEGRO_GREEN_SIZE] == 5) {
         eds->settings[ALLEGRO_COLOR_SIZE] = 15;
      }
      if (eds->settings[ALLEGRO_GREEN_SIZE] == 6) {
         eds->settings[ALLEGRO_COLOR_SIZE] = 16;
      }
   }

   if (eds->settings[ALLEGRO_RED_SIZE] == 8
    && eds->settings[ALLEGRO_GREEN_SIZE] == 8
    && eds->settings[ALLEGRO_BLUE_SIZE] == 8) {
      if (eds->settings[ALLEGRO_ALPHA_SIZE] == 0) {
         eds->settings[ALLEGRO_COLOR_SIZE] = 24;
      }
      if (eds->settings[ALLEGRO_ALPHA_SIZE] == 8) {
         eds->settings[ALLEGRO_COLOR_SIZE] = 32;
         /* small hack that tries to guess alpha shifting */
         eds->settings[ALLEGRO_ALPHA_SHIFT] = 48 - eds->settings[ALLEGRO_RED_SHIFT]
                                                 - eds->settings[ALLEGRO_GREEN_SHIFT]
                                                 - eds->settings[ALLEGRO_BLUE_SHIFT];
      }
   }
}

static ALLEGRO_EXTRA_DISPLAY_SETTINGS* read_fbconfig(Display *dpy,
                                                     GLXFBConfig fbc)
{
   int render_type, visual_type, buffer_size, sbuffers, samples;
   int drawable_type, renderable, swap_method;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds;
   XVisualInfo *v;

   eds = malloc(sizeof(ALLEGRO_EXTRA_DISPLAY_SETTINGS));
   eds->settings[ALLEGRO_RENDER_METHOD] = 2;

   if (glXGetFBConfigAttrib (dpy, fbc, GLX_RENDER_TYPE,
                     &render_type)
    || glXGetFBConfigAttrib (dpy, fbc, GLX_X_RENDERABLE,
                     &renderable)
    || glXGetFBConfigAttrib (dpy, fbc, GLX_DRAWABLE_TYPE,
                     &drawable_type)
    || glXGetFBConfigAttrib (dpy, fbc, GLX_X_VISUAL_TYPE,
                     &visual_type)
    || glXGetFBConfigAttrib (dpy, fbc, GLX_BUFFER_SIZE,
                     &buffer_size)
    || glXGetFBConfigAttrib (dpy, fbc, GLX_DEPTH_SIZE,
                     &eds->settings[ALLEGRO_DEPTH_SIZE])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_STEREO,
                     &eds->settings[ALLEGRO_STEREO])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_RED_SIZE,
                     &eds->settings[ALLEGRO_RED_SIZE])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_GREEN_SIZE,
                     &eds->settings[ALLEGRO_GREEN_SIZE])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_BLUE_SIZE,
                     &eds->settings[ALLEGRO_BLUE_SIZE])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_ALPHA_SIZE,
                     &eds->settings[ALLEGRO_ALPHA_SIZE])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_DOUBLEBUFFER,
                     &eds->settings[ALLEGRO_DOUBLEBUFFERED])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_AUX_BUFFERS,
                     &eds->settings[ALLEGRO_AUX_BUFFERS])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_STENCIL_SIZE,
                     &eds->settings[ALLEGRO_STENCIL_SIZE])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_ACCUM_RED_SIZE,
                     &eds->settings[ALLEGRO_ACC_RED_SIZE])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_ACCUM_GREEN_SIZE,
                     &eds->settings[ALLEGRO_ACC_GREEN_SIZE])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_ACCUM_BLUE_SIZE,
                     &eds->settings[ALLEGRO_ACC_BLUE_SIZE])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_ACCUM_ALPHA_SIZE,
                     &eds->settings[ALLEGRO_ACC_ALPHA_SIZE])) {
      TRACE(PREFIX_I "read_fbconfig: Incomplete glX mode ...\n");
      free(eds);
      return NULL;
   }

   if (!(render_type & GLX_RGBA_BIT) && !(render_type & GLX_RGBA_FLOAT_BIT)) {
      TRACE(PREFIX_I "read_fbconfig: Not RGBA mode\n");
      free(eds);
      return NULL;
   }

   if (!(drawable_type & GLX_WINDOW_BIT)) {
      TRACE(PREFIX_I "read_fbconfig: Cannot render to a window.\n");
      free(eds);
      return NULL;
   }

   if (renderable == False) {
      TRACE(PREFIX_I "read_fbconfig: GLX windows not supported.\n");
      free(eds);
      return NULL;
   }

   if (visual_type != GLX_TRUE_COLOR && visual_type != GLX_DIRECT_COLOR) {
      TRACE(PREFIX_I "read_fbconfig: visual type other than TrueColor and "
                  "DirectColor.\n");
      free(eds);
      return NULL;
   }

   /* Floating-point depth is not supported as glx extension (yet). */
   eds->settings[ALLEGRO_FLOAT_DEPTH] = 0;

   eds->settings[ALLEGRO_FLOAT_COLOR] = (render_type & GLX_RGBA_FLOAT_BIT);

   v = glXGetVisualFromFBConfig(dpy, fbc);
   if (!v) {
      TRACE(PREFIX_I "read_fbconfig: Cannot get associated visual for the FBConfig.\n");
      free(eds);
      return NULL;
   }
   figure_out_colors(eds, v);

   if (glXGetConfig(dpy, v, GLX_SAMPLE_BUFFERS, &sbuffers)) {
      /* Multisample extension is not supported */
      eds->settings[ALLEGRO_SAMPLE_BUFFERS] = 0;
   }
   else {
      eds->settings[ALLEGRO_SAMPLE_BUFFERS] = sbuffers;
   }
   if (glXGetConfig(dpy, v, GLX_SAMPLES, &samples)) {
      /* Multisample extension is not supported */
      eds->settings[ALLEGRO_SAMPLES] = 0;
   }
   else {
      eds->settings[ALLEGRO_SAMPLES] = samples;
   }
   if (glXGetFBConfigAttrib(dpy, fbc, GLX_SWAP_METHOD_OML,
                            &swap_method) == GLX_BAD_ATTRIBUTE) {
       /* GLX_OML_swap_method extension is not supported */
       eds->settings[ALLEGRO_SWAP_METHOD] = 0;
   }
   else {
       eds->settings[ALLEGRO_SWAP_METHOD] = swap_method;
   }

   eds->settings[ALLEGRO_COMPATIBLE_DISPLAY] = (_al_deduce_color_format(eds)
                                               != ALLEGRO_PIXEL_FORMAT_ANY);

   XFree(v);

   return eds;
}

static ALLEGRO_EXTRA_DISPLAY_SETTINGS** get_visuals_new(int *count, ALLEGRO_DISPLAY_XGLX *glx)
{
   int num_fbconfigs, i, j;
   GLXFBConfig *fbconfig;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS **eds_list = NULL;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref;
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();

   *count = 0;
   ref = _al_get_new_display_settings();

   fbconfig = glXGetFBConfigs(system->gfxdisplay, glx->xscreen, &num_fbconfigs);
   if (!fbconfig || !num_fbconfigs)
      return NULL;

   eds_list = malloc(num_fbconfigs * sizeof(*eds_list));

   TRACE(PREFIX_I "get_visuals_new: %i formats.\n", num_fbconfigs);

   for (j = i = 0; i < num_fbconfigs; i++) {
      TRACE("-- \n");
      TRACE(PREFIX_I "Decoding visual no. %i...\n", i);
      eds_list[j] = read_fbconfig(system->gfxdisplay, fbconfig[i]);
      if (!eds_list[j])
         continue;
#ifdef DEBUGMODE
      display_pixel_format(eds_list[j]);
#endif
      eds_list[j]->score = _al_score_display_settings(eds_list[j], ref);
      if (eds_list[j]->score == -1) {
         free(eds_list[j]);
         eds_list[j] = NULL;
         continue;
	  }
      eds_list[j]->index = i;
      eds_list[j]->info = glXGetVisualFromFBConfig(system->gfxdisplay, fbconfig[i]);
      j++;
   }

   TRACE(PREFIX_I "get_visuals_new(): %i visuals are good enough.\n", j);
   *count = j;
   XFree(fbconfig);
   if (j == 0) {
      for (i = 0; i < num_fbconfigs; i++)
         free(eds_list[i]);
      free(eds_list);
      return NULL;
   }
   return eds_list;
}


static ALLEGRO_EXTRA_DISPLAY_SETTINGS* read_xvisual(Display *dpy,
                                                    XVisualInfo *v)
{
   int rgba, buffer_size, use_gl, sbuffers, samples;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds;

   /* We can only support TrueColor and DirectColor visuals --
    * we only support RGBA mode */
   if (v->class != TrueColor && v->class != DirectColor)
      return NULL;

   eds = malloc(sizeof(ALLEGRO_EXTRA_DISPLAY_SETTINGS));
   eds->settings[ALLEGRO_RENDER_METHOD] = 2;

   if (glXGetConfig (dpy, v, GLX_RGBA,         &rgba)
    || glXGetConfig (dpy, v, GLX_USE_GL,       &use_gl)
    || glXGetConfig (dpy, v, GLX_BUFFER_SIZE,  &buffer_size)
    || glXGetConfig (dpy, v, GLX_RED_SIZE,     &eds->settings[ALLEGRO_RED_SIZE])
    || glXGetConfig (dpy, v, GLX_GREEN_SIZE,   &eds->settings[ALLEGRO_GREEN_SIZE])
    || glXGetConfig (dpy, v, GLX_BLUE_SIZE,    &eds->settings[ALLEGRO_BLUE_SIZE])
    || glXGetConfig (dpy, v, GLX_ALPHA_SIZE,   &eds->settings[ALLEGRO_ALPHA_SIZE])
    || glXGetConfig (dpy, v, GLX_DOUBLEBUFFER, &eds->settings[ALLEGRO_DOUBLEBUFFERED])
    || glXGetConfig (dpy, v, GLX_STEREO,       &eds->settings[ALLEGRO_STEREO])
    || glXGetConfig (dpy, v, GLX_AUX_BUFFERS,  &eds->settings[ALLEGRO_AUX_BUFFERS])
    || glXGetConfig (dpy, v, GLX_DEPTH_SIZE,   &eds->settings[ALLEGRO_DEPTH_SIZE])
    || glXGetConfig (dpy, v, GLX_STENCIL_SIZE, &eds->settings[ALLEGRO_STENCIL_SIZE])
    || glXGetConfig (dpy, v, GLX_ACCUM_RED_SIZE,
                     &eds->settings[ALLEGRO_ACC_RED_SIZE])
    || glXGetConfig (dpy, v, GLX_ACCUM_GREEN_SIZE,
                     &eds->settings[ALLEGRO_ACC_GREEN_SIZE])
    || glXGetConfig (dpy, v, GLX_ACCUM_BLUE_SIZE,
                     &eds->settings[ALLEGRO_ACC_BLUE_SIZE])
    || glXGetConfig (dpy, v, GLX_ACCUM_ALPHA_SIZE,
                     &eds->settings[ALLEGRO_ACC_ALPHA_SIZE])) {
      TRACE(PREFIX_I "read_xvisual: Incomplete glX mode ...\n");
      free(eds);
      return NULL;
   }

   if (!rgba) {
      TRACE(PREFIX_I "read_xvisual: Not RGBA mode\n");
      free(eds);
      return NULL;
   }

   if (!use_gl) {
      TRACE(PREFIX_I "read_xvisual: OpenGL Unsupported\n");
      free(eds);
      return NULL;
   }

   eds->settings[ALLEGRO_COLOR_SIZE] = 0;
   figure_out_colors(eds, v);

   eds->settings[ALLEGRO_FLOAT_COLOR] = 0;
   eds->settings[ALLEGRO_FLOAT_DEPTH] = 0;

   if (glXGetConfig(dpy, v, GLX_SAMPLE_BUFFERS, &sbuffers) == GLX_BAD_ATTRIBUTE) {
      /* Multisample extension is not supported */
      eds->settings[ALLEGRO_SAMPLE_BUFFERS] = 0;
   }
   else {
      eds->settings[ALLEGRO_SAMPLE_BUFFERS] = sbuffers;
   }
   if (glXGetConfig(dpy, v, GLX_SAMPLES, &samples) == GLX_BAD_ATTRIBUTE) {
      /* Multisample extension is not supported */
      eds->settings[ALLEGRO_SAMPLES] = 0;
   }
   else {
      eds->settings[ALLEGRO_SAMPLES] = samples;
   }

   eds->settings[ALLEGRO_SWAP_METHOD] = 0;
   eds->settings[ALLEGRO_COMPATIBLE_DISPLAY] = (_al_deduce_color_format(eds)
                                               != ALLEGRO_PIXEL_FORMAT_ANY);

   return eds;
}

static ALLEGRO_EXTRA_DISPLAY_SETTINGS** get_visuals_old(int *count)
{
   int num_visuals, i, j;
   XVisualInfo *xv;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS **eds_list = NULL;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref;
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();

   *count = 0;
   ref = _al_get_new_display_settings();

   xv = XGetVisualInfo(system->gfxdisplay, 0, NULL, &num_visuals);
   if (!xv || !num_visuals)
      return NULL;

   eds_list = malloc(num_visuals * sizeof(*eds_list));
   memset(eds_list, 0, num_visuals * sizeof(*eds_list));

   TRACE(PREFIX_I "get_visuals_new: %i formats.\n", num_visuals);

   for (j = i = 0; i < num_visuals; i++) {
      TRACE("-- \n");
      TRACE(PREFIX_I "Decoding visual no. %i...\n", i);
      eds_list[j] = read_xvisual(system->gfxdisplay, xv+i);
      if (!eds_list[j])
         continue;
#ifdef DEBUGMODE
      display_pixel_format(eds_list[j]);
#endif
      eds_list[j]->score = _al_score_display_settings(eds_list[j], ref);
      if (eds_list[j]->score == -1) {
         free(eds_list[j]);
         eds_list[j] = NULL;
         continue;
      }
      eds_list[j]->index = i;
      /* Seems that XVinfo is static. */
      eds_list[j]->info = malloc(sizeof(XVisualInfo));
      memcpy(eds_list[j]->info, xv+i, sizeof(XVisualInfo));
      j++;
   }

   TRACE(PREFIX_I "get_visuals_new(): %i visuals are good enough.\n", j);
   *count = j;
   if (j == 0) {
      for (i = 0; i < num_visuals; i++) {
         free(eds_list[i]);
      }
      free(eds_list);
      XFree(xv);
      return NULL;
   }
   return eds_list;
}


void _al_xglx_config_select_visual(ALLEGRO_DISPLAY_XGLX *glx)
{
   ALLEGRO_EXTRA_DISPLAY_SETTINGS **eds = NULL;
   int eds_count = 0;
   int i;

   if (glx->glx_version >= 130)
      eds = get_visuals_new(&eds_count, glx);
   if (!eds)
      eds = get_visuals_old(&eds_count);

   if (!eds) {
      TRACE(PREFIX_E "_al_xglx_config_select_visual(): Failed to get any visual info.\n");
      return;
   }

   qsort(eds, eds_count, sizeof(eds), _al_display_settings_sorter);

   TRACE(PREFIX_I "_al_xglx_config_select_visual(): Chose visual no. %i\n\n", eds[0]->index);
#ifdef DEBUGMODE
   display_pixel_format(eds[0]);
#endif
   glx->xvinfo = eds[0]->info;
   glx->display.format = _al_deduce_color_format(eds[0]);
   memcpy(&glx->display.extra_settings, eds[0], sizeof(ALLEGRO_EXTRA_DISPLAY_SETTINGS));

   for (i = 0; i < eds_count; i++) {
      if (glx->xvinfo != eds[i]->info)
         XFree(eds[i]->info);
      free(eds[i]);
   }
   free(eds);
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
