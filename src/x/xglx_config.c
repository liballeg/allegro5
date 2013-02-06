#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/opengl/gl_ext.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xdisplay.h"
#include "allegro5/internal/aintern_xglx_config.h"
#include "allegro5/internal/aintern_xsystem.h"

ALLEGRO_DEBUG_CHANNEL("xglx_config")

#ifdef DEBUGMODE
static void display_pixel_format(ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds)
{
   ALLEGRO_DEBUG("Single-buffer: %s\n", eds->settings[ALLEGRO_SINGLE_BUFFER] ? "yes" : "no");
   if (eds->settings[ALLEGRO_SWAP_METHOD] > 0)
      ALLEGRO_DEBUG("Swap method: %s\n", eds->settings[ALLEGRO_SWAP_METHOD] == 2 ? "flip" : "copy");
   else
      ALLEGRO_DEBUG("Swap method: undefined\n");
   ALLEGRO_DEBUG("Color format: r%i g%i b%i a%i, %i bit\n",
      eds->settings[ALLEGRO_RED_SIZE],
      eds->settings[ALLEGRO_GREEN_SIZE],
      eds->settings[ALLEGRO_BLUE_SIZE],
      eds->settings[ALLEGRO_ALPHA_SIZE],
      eds->settings[ALLEGRO_COLOR_SIZE]);
   ALLEGRO_DEBUG("Depth buffer: %i bits\n", eds->settings[ALLEGRO_DEPTH_SIZE]);
   ALLEGRO_DEBUG("Sample buffers: %s\n", eds->settings[ALLEGRO_SAMPLE_BUFFERS] ? "yes" : "no");
   ALLEGRO_DEBUG("Samples: %i\n", eds->settings[ALLEGRO_SAMPLES]);
}
#endif

static int get_shift(int mask)
{
   int i = 0, j = 1;
   if (!mask)
      return -1;
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
   int drawable_type, renderable, swap_method, double_buffer;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds;
   XVisualInfo *v;

   eds = al_calloc(1, sizeof(ALLEGRO_EXTRA_DISPLAY_SETTINGS));
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
                     &double_buffer)
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
      ALLEGRO_DEBUG("Incomplete glX mode ...\n");
      al_free(eds);
      return NULL;
   }
   eds->settings[ALLEGRO_SINGLE_BUFFER] = !double_buffer;

   if (!(render_type & GLX_RGBA_BIT) && !(render_type & GLX_RGBA_FLOAT_BIT_ARB)) {
      ALLEGRO_DEBUG("Not RGBA mode\n");
      al_free(eds);
      return NULL;
   }

   if (!(drawable_type & GLX_WINDOW_BIT)) {
      ALLEGRO_DEBUG("Cannot render to a window.\n");
      al_free(eds);
      return NULL;
   }

   if (renderable == False) {
      ALLEGRO_DEBUG("GLX windows not supported.\n");
      al_free(eds);
      return NULL;
   }

   if (visual_type != GLX_TRUE_COLOR && visual_type != GLX_DIRECT_COLOR) {
      ALLEGRO_DEBUG("visual type other than TrueColor and "
                  "DirectColor.\n");
      al_free(eds);
      return NULL;
   }

   /* Floating-point depth is not supported as glx extension (yet). */
   eds->settings[ALLEGRO_FLOAT_DEPTH] = 0;

   eds->settings[ALLEGRO_FLOAT_COLOR] = (render_type & GLX_RGBA_FLOAT_BIT_ARB);

   v = glXGetVisualFromFBConfig(dpy, fbc);
   if (!v) {
      ALLEGRO_DEBUG("Cannot get associated visual for the FBConfig.\n");
      al_free(eds);
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

   /* We can only guarantee vsync is off. */
   eds->settings[ALLEGRO_VSYNC] = 2;

   eds->settings[ALLEGRO_COMPATIBLE_DISPLAY] = (_al_deduce_color_format(eds)
                                               != ALLEGRO_PIXEL_FORMAT_ANY);

   XFree(v);

   return eds;
}


static ALLEGRO_EXTRA_DISPLAY_SETTINGS** get_visuals_new(ALLEGRO_DISPLAY_XGLX *glx, int *eds_count)
{
   int num_fbconfigs, i, j;
   GLXFBConfig *fbconfig;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS **eds = NULL;
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   ref = _al_get_new_display_settings();

   fbconfig = glXGetFBConfigs(system->gfxdisplay, glx->xscreen, &num_fbconfigs);
   if (!fbconfig || !num_fbconfigs) {
      if (fbconfig) {
         XFree(fbconfig);
      }
      ALLEGRO_DEBUG("glXGetFBConfigs(xscreen=%d) returned NULL.\n", glx->xscreen);
      return NULL;
   }

   eds = al_malloc(num_fbconfigs * sizeof(*eds));

   ALLEGRO_INFO("%i formats.\n", num_fbconfigs);

   for (i = j = 0; i < num_fbconfigs; i++) {
      ALLEGRO_DEBUG("-- \n");
      ALLEGRO_DEBUG("Decoding visual no. %i...\n", i);
      eds[j] = read_fbconfig(system->gfxdisplay, fbconfig[i]);
      if (!eds[j])
         continue;
#ifdef DEBUGMODE
      display_pixel_format(eds[j]);
#endif
      eds[j]->score = _al_score_display_settings(eds[j], ref);
      if (eds[j]->score == -1) {
         al_free(eds[j]);
         continue;
      }
      eds[j]->index = i;
      eds[j]->info = al_malloc(sizeof(GLXFBConfig));
      memcpy(eds[j]->info, &fbconfig[i], sizeof(GLXFBConfig));
      j++;
   }

   *eds_count = j;
   ALLEGRO_INFO("%i visuals are good enough.\n", j);
   if (j == 0) {
      al_free(eds);
      eds = NULL;
   }

   XFree(fbconfig);

   return eds;
}


static ALLEGRO_EXTRA_DISPLAY_SETTINGS* read_xvisual(Display *dpy,
                                                    XVisualInfo *v)
{
   int rgba, buffer_size, use_gl, sbuffers, samples, double_buffer;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds;

   /* We can only support TrueColor and DirectColor visuals --
    * we only support RGBA mode */
   if (v->class != TrueColor && v->class != DirectColor)
      return NULL;

   eds = al_calloc(1, sizeof(ALLEGRO_EXTRA_DISPLAY_SETTINGS));
   eds->settings[ALLEGRO_RENDER_METHOD] = 2;

   if (glXGetConfig (dpy, v, GLX_RGBA,         &rgba)
    || glXGetConfig (dpy, v, GLX_USE_GL,       &use_gl)
    || glXGetConfig (dpy, v, GLX_BUFFER_SIZE,  &buffer_size)
    || glXGetConfig (dpy, v, GLX_RED_SIZE,     &eds->settings[ALLEGRO_RED_SIZE])
    || glXGetConfig (dpy, v, GLX_GREEN_SIZE,   &eds->settings[ALLEGRO_GREEN_SIZE])
    || glXGetConfig (dpy, v, GLX_BLUE_SIZE,    &eds->settings[ALLEGRO_BLUE_SIZE])
    || glXGetConfig (dpy, v, GLX_ALPHA_SIZE,   &eds->settings[ALLEGRO_ALPHA_SIZE])
    || glXGetConfig (dpy, v, GLX_DOUBLEBUFFER, &double_buffer)
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
      ALLEGRO_DEBUG("Incomplete glX mode ...\n");
      al_free(eds);
      return NULL;
   }
   eds->settings[ALLEGRO_SINGLE_BUFFER] = !double_buffer;

   if (!rgba) {
      ALLEGRO_DEBUG("Not RGBA mode\n");
      al_free(eds);
      return NULL;
   }

   if (!use_gl) {
      ALLEGRO_DEBUG("OpenGL Unsupported\n");
      al_free(eds);
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

static ALLEGRO_EXTRA_DISPLAY_SETTINGS** get_visuals_old(int *eds_count)
{
   int i, j, num_visuals;
   XVisualInfo *xv;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS **eds;
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   ref = _al_get_new_display_settings();

   xv = XGetVisualInfo(system->gfxdisplay, 0, NULL, &num_visuals);
   if (!xv || !num_visuals)
      return NULL;

   eds = al_malloc(num_visuals * sizeof(*eds));

   ALLEGRO_INFO("%i formats.\n", num_visuals);

   for (i = j = 0; i < num_visuals; i++) {
      ALLEGRO_DEBUG("-- \n");
      ALLEGRO_DEBUG("Decoding visual no. %i...\n", i);
      eds[j] = read_xvisual(system->gfxdisplay, xv + i);
      if (!eds[j])
         continue;
#ifdef DEBUGMODE
      display_pixel_format(eds[j]);
#endif
      eds[j]->score = _al_score_display_settings(eds[j], ref);
      if (eds[j]->score == -1) {
         al_free(eds[j]);
         continue;
      }
      eds[j]->index = i;
      /* Seems that XVinfo is static. */
      eds[j]->info = al_malloc(sizeof(XVisualInfo));
      memcpy(eds[j]->info, xv + i, sizeof(XVisualInfo));
      j++;
   }

   *eds_count = j;
   ALLEGRO_INFO("%i visuals are good enough.\n", j);
   if (j == 0) {
      al_free(eds);
      eds = NULL;
   }

   XFree(xv);

   return eds;
}


static void select_best_visual(ALLEGRO_DISPLAY_XGLX *glx,
                               ALLEGRO_EXTRA_DISPLAY_SETTINGS* *eds, int eds_count,
                               bool using_fbc)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   qsort(eds, eds_count, sizeof(*eds), _al_display_settings_sorter);

   ASSERT(eds_count > 0);
   ASSERT(eds[0] != NULL);

   if (!eds[0]->info) {
      ALLEGRO_ERROR("No matching displays found.\n");
      glx->xvinfo = NULL;
      return;
   }

   ALLEGRO_INFO("Chose visual no. %i\n", eds[0]->index);
#ifdef DEBUGMODE
   display_pixel_format(eds[0]);
#endif
   if (using_fbc) {
      glx->fbc = al_malloc(sizeof(GLXFBConfig));
      memcpy(glx->fbc, eds[0]->info, sizeof(GLXFBConfig));

      glx->xvinfo = glXGetVisualFromFBConfig(system->gfxdisplay, *glx->fbc);
   }
   else {
      glx->xvinfo = al_malloc(sizeof(XVisualInfo));
      memcpy(glx->xvinfo, eds[0]->info, sizeof(XVisualInfo));
   }

   ALLEGRO_INFO("Corresponding X11 visual id: %lu\n", glx->xvinfo->visualid);
   memcpy(&glx->display.extra_settings, eds[0], sizeof(ALLEGRO_EXTRA_DISPLAY_SETTINGS));
}



void _al_xglx_config_select_visual(ALLEGRO_DISPLAY_XGLX *glx)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   ALLEGRO_EXTRA_DISPLAY_SETTINGS **eds;
   int eds_count, i;
   bool force_old = false;
   bool using_fbc;

   if (system->system.config) {
      const char *selection_mode;
      selection_mode = al_get_config_value(system->system.config, "graphics",
                          "config_selection");
      if (selection_mode && selection_mode[0] != '\0') {
         if (!_al_stricmp(selection_mode, "old")) {
            ALLEGRO_WARN("Forcing OLD visual selection method.\n");
            force_old = true;
         }
         else if (!_al_stricmp(selection_mode, "new"))
            force_old = false;
      }
   }

   if (glx->glx_version >= 130 && !force_old)
      eds = get_visuals_new(glx, &eds_count);
   else
      eds = NULL;

   if (!eds) {
      eds = get_visuals_old(&eds_count);
      using_fbc = false;
   }
   else
      using_fbc = true;

   if (!eds) {
      ALLEGRO_ERROR("Failed to get any visual info.\n");
      return;
   }

   select_best_visual(glx, eds, eds_count, using_fbc);

   for (i = 0; i < eds_count; i++) {
      if (eds[i]) {
         al_free(eds[i]->info);
         al_free(eds[i]);
      }
   }
   al_free(eds);
}

static GLXContext create_context_new(int ver, Display *dpy, GLXFBConfig fb,
   GLXContext ctx, bool forward_compat, int major, int minor)
{
   typedef GLXContext (*GCCA_PROC) (Display*, GLXFBConfig, GLXContext, Bool, const int*);
   GCCA_PROC _xglx_glXCreateContextAttribsARB = NULL;

   if (ver >= 140) {
      /* GLX 1.4 should have this */
      _xglx_glXCreateContextAttribsARB = glXCreateContextAttribsARB;
   }
   if (!_xglx_glXCreateContextAttribsARB) {
      /* Load the extension manually. */
      _xglx_glXCreateContextAttribsARB =
         (GCCA_PROC)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
   }
   if (!_xglx_glXCreateContextAttribsARB) {
      ALLEGRO_ERROR("GLX_ARB_create_context not supported and needed for OpenGL 3\n");
      return NULL;
   }

   int attrib[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, major,
                   GLX_CONTEXT_MINOR_VERSION_ARB, minor,
                   GLX_CONTEXT_FLAGS_ARB, 0,
                   0};
   if (forward_compat)
      attrib[5] = GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
   return _xglx_glXCreateContextAttribsARB(dpy, fb, ctx, True, attrib);
}

bool _al_xglx_config_create_context(ALLEGRO_DISPLAY_XGLX *glx)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
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
      if (disp->flags & ALLEGRO_OPENGL_3_0) {
         bool forward_compat = (disp->flags & ALLEGRO_OPENGL_FORWARD_COMPATIBLE) != 0;
         glx->context = create_context_new(glx->glx_version,
            system->gfxdisplay, *glx->fbc, existing_ctx, forward_compat, 3, 0);
         disp->extra_settings.settings[ALLEGRO_COMPATIBLE_DISPLAY] = !forward_compat;
      }
      else {
         glx->context = glXCreateNewContext(system->gfxdisplay, *glx->fbc,
            GLX_RGBA_TYPE, existing_ctx, True);
      }

      /* Create a GLX subwindow inside our window. */
      glx->glxwindow = glXCreateWindow(system->gfxdisplay, *glx->fbc,
         glx->window, 0);
   }
   else {
      /* Create a GLX context from visual info. */
      glx->context = glXCreateContext(system->gfxdisplay, glx->xvinfo,
         existing_ctx, True);
      glx->glxwindow = glx->window;
   }

   if (!glx->context || !glx->glxwindow) {
      ALLEGRO_ERROR("Failed to create GLX context.\n");
      return false;
   }

   disp->ogl_extras->is_shared = true;

   ALLEGRO_DEBUG("Got GLX context.\n");
   return true;
}
