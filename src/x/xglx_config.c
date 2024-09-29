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

A5O_DEBUG_CHANNEL("xglx_config")

#ifdef DEBUGMODE
static void display_pixel_format(A5O_EXTRA_DISPLAY_SETTINGS *eds)
{
   A5O_DEBUG("Single-buffer: %s\n", eds->settings[A5O_SINGLE_BUFFER] ? "yes" : "no");
   if (eds->settings[A5O_SWAP_METHOD] > 0)
      A5O_DEBUG("Swap method: %s\n", eds->settings[A5O_SWAP_METHOD] == 2 ? "flip" : "copy");
   else
      A5O_DEBUG("Swap method: undefined\n");
   A5O_DEBUG("Color format: r%i g%i b%i a%i, %i bit\n",
      eds->settings[A5O_RED_SIZE],
      eds->settings[A5O_GREEN_SIZE],
      eds->settings[A5O_BLUE_SIZE],
      eds->settings[A5O_ALPHA_SIZE],
      eds->settings[A5O_COLOR_SIZE]);
   A5O_DEBUG("Depth buffer: %i bits\n", eds->settings[A5O_DEPTH_SIZE]);
   A5O_DEBUG("Sample buffers: %s\n", eds->settings[A5O_SAMPLE_BUFFERS] ? "yes" : "no");
   A5O_DEBUG("Samples: %i\n", eds->settings[A5O_SAMPLES]);
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

static void figure_out_colors(A5O_EXTRA_DISPLAY_SETTINGS *eds, XVisualInfo *v)
{
   eds->settings[A5O_RED_SHIFT]   = get_shift(v->red_mask);
   eds->settings[A5O_GREEN_SHIFT] = get_shift(v->green_mask);
   eds->settings[A5O_BLUE_SHIFT]  = get_shift(v->blue_mask);
   eds->settings[A5O_ALPHA_SHIFT] = 0;

   eds->settings[A5O_COLOR_SIZE] = 0;

   if (eds->settings[A5O_RED_SIZE] == 3
    && eds->settings[A5O_GREEN_SIZE] == 3
    && eds->settings[A5O_BLUE_SIZE] == 2) {
      eds->settings[A5O_COLOR_SIZE] = 8;
   }

   if (eds->settings[A5O_RED_SIZE] == 5
    && eds->settings[A5O_BLUE_SIZE] == 5) {
      if (eds->settings[A5O_GREEN_SIZE] == 5) {
         eds->settings[A5O_COLOR_SIZE] = 15;
      }
      if (eds->settings[A5O_GREEN_SIZE] == 6) {
         eds->settings[A5O_COLOR_SIZE] = 16;
      }
   }

   if (eds->settings[A5O_RED_SIZE] == 8
    && eds->settings[A5O_GREEN_SIZE] == 8
    && eds->settings[A5O_BLUE_SIZE] == 8) {
      if (eds->settings[A5O_ALPHA_SIZE] == 0) {
         eds->settings[A5O_COLOR_SIZE] = 24;
      }
      if (eds->settings[A5O_ALPHA_SIZE] == 8) {
         eds->settings[A5O_COLOR_SIZE] = 32;
         /* small hack that tries to guess alpha shifting */
         eds->settings[A5O_ALPHA_SHIFT] = 48 - eds->settings[A5O_RED_SHIFT]
                                                 - eds->settings[A5O_GREEN_SHIFT]
                                                 - eds->settings[A5O_BLUE_SHIFT];
      }
   }
}

static A5O_EXTRA_DISPLAY_SETTINGS* read_fbconfig(Display *dpy,
                                                     GLXFBConfig fbc)
{
   int render_type, visual_type, buffer_size, sbuffers, samples;
   int drawable_type, renderable, swap_method, double_buffer;
   A5O_EXTRA_DISPLAY_SETTINGS *eds;
   XVisualInfo *v;

   eds = al_calloc(1, sizeof(A5O_EXTRA_DISPLAY_SETTINGS));
   eds->settings[A5O_RENDER_METHOD] = 2;

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
                     &eds->settings[A5O_DEPTH_SIZE])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_STEREO,
                     &eds->settings[A5O_STEREO])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_RED_SIZE,
                     &eds->settings[A5O_RED_SIZE])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_GREEN_SIZE,
                     &eds->settings[A5O_GREEN_SIZE])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_BLUE_SIZE,
                     &eds->settings[A5O_BLUE_SIZE])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_ALPHA_SIZE,
                     &eds->settings[A5O_ALPHA_SIZE])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_DOUBLEBUFFER,
                     &double_buffer)
    || glXGetFBConfigAttrib (dpy, fbc, GLX_AUX_BUFFERS,
                     &eds->settings[A5O_AUX_BUFFERS])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_STENCIL_SIZE,
                     &eds->settings[A5O_STENCIL_SIZE])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_ACCUM_RED_SIZE,
                     &eds->settings[A5O_ACC_RED_SIZE])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_ACCUM_GREEN_SIZE,
                     &eds->settings[A5O_ACC_GREEN_SIZE])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_ACCUM_BLUE_SIZE,
                     &eds->settings[A5O_ACC_BLUE_SIZE])
    || glXGetFBConfigAttrib (dpy, fbc, GLX_ACCUM_ALPHA_SIZE,
                     &eds->settings[A5O_ACC_ALPHA_SIZE])) {
      A5O_DEBUG("Incomplete glX mode ...\n");
      al_free(eds);
      return NULL;
   }
   eds->settings[A5O_SINGLE_BUFFER] = !double_buffer;

   if (!(render_type & GLX_RGBA_BIT) && !(render_type & GLX_RGBA_FLOAT_BIT_ARB)) {
      A5O_DEBUG("Not RGBA mode\n");
      al_free(eds);
      return NULL;
   }

   if (!(drawable_type & GLX_WINDOW_BIT)) {
      A5O_DEBUG("Cannot render to a window.\n");
      al_free(eds);
      return NULL;
   }

   if (renderable == False) {
      A5O_DEBUG("GLX windows not supported.\n");
      al_free(eds);
      return NULL;
   }

   if (visual_type != GLX_TRUE_COLOR && visual_type != GLX_DIRECT_COLOR) {
      A5O_DEBUG("visual type other than TrueColor and "
                  "DirectColor.\n");
      al_free(eds);
      return NULL;
   }

   /* Floating-point depth is not supported as glx extension (yet). */
   eds->settings[A5O_FLOAT_DEPTH] = 0;

   eds->settings[A5O_FLOAT_COLOR] = (render_type & GLX_RGBA_FLOAT_BIT_ARB);

   v = glXGetVisualFromFBConfig(dpy, fbc);
   if (!v) {
      A5O_DEBUG("Cannot get associated visual for the FBConfig.\n");
      al_free(eds);
      return NULL;
   }
   figure_out_colors(eds, v);

   if (glXGetConfig(dpy, v, GLX_SAMPLE_BUFFERS, &sbuffers)) {
      /* Multisample extension is not supported */
      eds->settings[A5O_SAMPLE_BUFFERS] = 0;
   }
   else {
      eds->settings[A5O_SAMPLE_BUFFERS] = sbuffers;
   }
   if (glXGetConfig(dpy, v, GLX_SAMPLES, &samples)) {
      /* Multisample extension is not supported */
      eds->settings[A5O_SAMPLES] = 0;
   }
   else {
      eds->settings[A5O_SAMPLES] = samples;
   }
   if (glXGetFBConfigAttrib(dpy, fbc, GLX_SWAP_METHOD_OML,
                            &swap_method) == GLX_BAD_ATTRIBUTE) {
       /* GLX_OML_swap_method extension is not supported */
       eds->settings[A5O_SWAP_METHOD] = 0;
   }
   else {
       eds->settings[A5O_SWAP_METHOD] = swap_method;
   }

   /* We can only guarantee vsync is off. */
   eds->settings[A5O_VSYNC] = 2;

   eds->settings[A5O_COMPATIBLE_DISPLAY] = (_al_deduce_color_format(eds)
                                               != A5O_PIXEL_FORMAT_ANY);

   XFree(v);

   return eds;
}


static A5O_EXTRA_DISPLAY_SETTINGS** get_visuals_new(A5O_DISPLAY_XGLX *glx, int *eds_count)
{
   int num_fbconfigs, i, j;
   GLXFBConfig *fbconfig;
   A5O_EXTRA_DISPLAY_SETTINGS *ref;
   A5O_EXTRA_DISPLAY_SETTINGS **eds = NULL;
   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   ref = _al_get_new_display_settings();

   fbconfig = glXGetFBConfigs(system->gfxdisplay, glx->xscreen, &num_fbconfigs);
   if (!fbconfig || !num_fbconfigs) {
      if (fbconfig) {
         XFree(fbconfig);
      }
      A5O_DEBUG("glXGetFBConfigs(xscreen=%d) returned NULL.\n", glx->xscreen);
      return NULL;
   }

   eds = al_malloc(num_fbconfigs * sizeof(*eds));

   A5O_INFO("%i formats.\n", num_fbconfigs);

   for (i = j = 0; i < num_fbconfigs; i++) {
      A5O_DEBUG("-- \n");
      A5O_DEBUG("Decoding visual no. %i...\n", i);
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
   A5O_INFO("%i visuals are good enough.\n", j);
   if (j == 0) {
      al_free(eds);
      eds = NULL;
   }

   XFree(fbconfig);

   return eds;
}


static A5O_EXTRA_DISPLAY_SETTINGS* read_xvisual(Display *dpy,
                                                    XVisualInfo *v)
{
   int rgba, buffer_size, use_gl, sbuffers, samples, double_buffer;
   A5O_EXTRA_DISPLAY_SETTINGS *eds;

   /* We can only support TrueColor and DirectColor visuals --
    * we only support RGBA mode */
   if (v->class != TrueColor && v->class != DirectColor)
      return NULL;

   eds = al_calloc(1, sizeof(A5O_EXTRA_DISPLAY_SETTINGS));
   eds->settings[A5O_RENDER_METHOD] = 2;

   if (glXGetConfig (dpy, v, GLX_RGBA,         &rgba)
    || glXGetConfig (dpy, v, GLX_USE_GL,       &use_gl)
    || glXGetConfig (dpy, v, GLX_BUFFER_SIZE,  &buffer_size)
    || glXGetConfig (dpy, v, GLX_RED_SIZE,     &eds->settings[A5O_RED_SIZE])
    || glXGetConfig (dpy, v, GLX_GREEN_SIZE,   &eds->settings[A5O_GREEN_SIZE])
    || glXGetConfig (dpy, v, GLX_BLUE_SIZE,    &eds->settings[A5O_BLUE_SIZE])
    || glXGetConfig (dpy, v, GLX_ALPHA_SIZE,   &eds->settings[A5O_ALPHA_SIZE])
    || glXGetConfig (dpy, v, GLX_DOUBLEBUFFER, &double_buffer)
    || glXGetConfig (dpy, v, GLX_STEREO,       &eds->settings[A5O_STEREO])
    || glXGetConfig (dpy, v, GLX_AUX_BUFFERS,  &eds->settings[A5O_AUX_BUFFERS])
    || glXGetConfig (dpy, v, GLX_DEPTH_SIZE,   &eds->settings[A5O_DEPTH_SIZE])
    || glXGetConfig (dpy, v, GLX_STENCIL_SIZE, &eds->settings[A5O_STENCIL_SIZE])
    || glXGetConfig (dpy, v, GLX_ACCUM_RED_SIZE,
                     &eds->settings[A5O_ACC_RED_SIZE])
    || glXGetConfig (dpy, v, GLX_ACCUM_GREEN_SIZE,
                     &eds->settings[A5O_ACC_GREEN_SIZE])
    || glXGetConfig (dpy, v, GLX_ACCUM_BLUE_SIZE,
                     &eds->settings[A5O_ACC_BLUE_SIZE])
    || glXGetConfig (dpy, v, GLX_ACCUM_ALPHA_SIZE,
                     &eds->settings[A5O_ACC_ALPHA_SIZE])) {
      A5O_DEBUG("Incomplete glX mode ...\n");
      al_free(eds);
      return NULL;
   }
   eds->settings[A5O_SINGLE_BUFFER] = !double_buffer;

   if (!rgba) {
      A5O_DEBUG("Not RGBA mode\n");
      al_free(eds);
      return NULL;
   }

   if (!use_gl) {
      A5O_DEBUG("OpenGL Unsupported\n");
      al_free(eds);
      return NULL;
   }

   eds->settings[A5O_COLOR_SIZE] = 0;
   figure_out_colors(eds, v);

   eds->settings[A5O_FLOAT_COLOR] = 0;
   eds->settings[A5O_FLOAT_DEPTH] = 0;

   if (glXGetConfig(dpy, v, GLX_SAMPLE_BUFFERS, &sbuffers) == GLX_BAD_ATTRIBUTE) {
      /* Multisample extension is not supported */
      eds->settings[A5O_SAMPLE_BUFFERS] = 0;
   }
   else {
      eds->settings[A5O_SAMPLE_BUFFERS] = sbuffers;
   }
   if (glXGetConfig(dpy, v, GLX_SAMPLES, &samples) == GLX_BAD_ATTRIBUTE) {
      /* Multisample extension is not supported */
      eds->settings[A5O_SAMPLES] = 0;
   }
   else {
      eds->settings[A5O_SAMPLES] = samples;
   }

   eds->settings[A5O_SWAP_METHOD] = 0;
   eds->settings[A5O_COMPATIBLE_DISPLAY] = (_al_deduce_color_format(eds)
                                               != A5O_PIXEL_FORMAT_ANY);

   return eds;
}

static A5O_EXTRA_DISPLAY_SETTINGS** get_visuals_old(int *eds_count)
{
   int i, j, num_visuals;
   XVisualInfo *xv;
   A5O_EXTRA_DISPLAY_SETTINGS *ref;
   A5O_EXTRA_DISPLAY_SETTINGS **eds;
   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   ref = _al_get_new_display_settings();

   xv = XGetVisualInfo(system->gfxdisplay, 0, NULL, &num_visuals);
   if (!xv || !num_visuals)
      return NULL;

   eds = al_malloc(num_visuals * sizeof(*eds));

   A5O_INFO("%i formats.\n", num_visuals);

   for (i = j = 0; i < num_visuals; i++) {
      A5O_DEBUG("-- \n");
      A5O_DEBUG("Decoding visual no. %i...\n", i);
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
   A5O_INFO("%i visuals are good enough.\n", j);
   if (j == 0) {
      al_free(eds);
      eds = NULL;
   }

   XFree(xv);

   return eds;
}


static void select_best_visual(A5O_DISPLAY_XGLX *glx,
                               A5O_EXTRA_DISPLAY_SETTINGS* *eds, int eds_count,
                               bool using_fbc)
{
   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   qsort(eds, eds_count, sizeof(*eds), _al_display_settings_sorter);

   ASSERT(eds_count > 0);
   ASSERT(eds[0] != NULL);

   if (!eds[0]->info) {
      A5O_ERROR("No matching displays found.\n");
      glx->xvinfo = NULL;
      return;
   }

   A5O_INFO("Chose visual no. %i\n", eds[0]->index);
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

   A5O_INFO("Corresponding X11 visual id: %lu\n", glx->xvinfo->visualid);
   memcpy(&glx->display.extra_settings, eds[0], sizeof(A5O_EXTRA_DISPLAY_SETTINGS));
}



void _al_xglx_config_select_visual(A5O_DISPLAY_XGLX *glx)
{
   A5O_EXTRA_DISPLAY_SETTINGS **eds;
   int eds_count, i;
   bool force_old = false;
   bool using_fbc;

   const char *selection_mode;
   selection_mode = al_get_config_value(al_get_system_config(), "graphics",
                       "config_selection");
   if (selection_mode && selection_mode[0] != '\0') {
      if (!_al_stricmp(selection_mode, "old")) {
         A5O_WARN("Forcing OLD visual selection method.\n");
         force_old = true;
      }
      else if (!_al_stricmp(selection_mode, "new"))
         force_old = false;
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
      A5O_ERROR("Failed to get any visual info.\n");
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
   GLXContext ctx, bool forward_compat, bool want_es, bool core_profile, int major, int minor)
{
   typedef GLXContext (*GCCA_PROC) (Display*, GLXFBConfig, GLXContext, Bool, const int*);
   GCCA_PROC _xglx_glXCreateContextAttribsARB = NULL;

   if (ver >= 140) {
      /* GLX 1.4 should have this, if it's defined, use it directly. */
      /* OTOH it *could* be there but only available through dynamic loading. */
      /* In that case, fallback to calling glxXGetProcAddress. */
#ifdef glXCreateContextAttribsARB
      _xglx_glXCreateContextAttribsARB = glXCreateContextAttribsARB;
#endif // glXCreateContextAttribsARB
   }
   if (!_xglx_glXCreateContextAttribsARB) {
      /* Load the extension manually. */
      _xglx_glXCreateContextAttribsARB =
         (GCCA_PROC)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
   }
   if (!_xglx_glXCreateContextAttribsARB) {
      A5O_ERROR("GLX_ARB_create_context not supported and needed for OpenGL 3\n");
      return NULL;
   }

   int attrib[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, major,
                   GLX_CONTEXT_MINOR_VERSION_ARB, minor,
                   GLX_CONTEXT_FLAGS_ARB, 0,
                   0, 0,
                   0};
   if (forward_compat)
      attrib[5] = GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
   if (want_es) {
      attrib[6] = GLX_CONTEXT_PROFILE_MASK_ARB;
      attrib[7] = GLX_CONTEXT_ES_PROFILE_BIT_EXT;
   }
   else if (core_profile) {
      attrib[6] = GLX_CONTEXT_PROFILE_MASK_ARB;
      attrib[7] = GLX_CONTEXT_CORE_PROFILE_BIT_ARB;
   }
   return _xglx_glXCreateContextAttribsARB(dpy, fb, ctx, True, attrib);
}

bool _al_xglx_config_create_context(A5O_DISPLAY_XGLX *glx)
{
   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   A5O_DISPLAY *disp = (void*)glx;
   GLXContext existing_ctx = NULL;

   /* Find an existing context with which to share display lists. */
   if (_al_vector_size(&system->system.displays) > 1) {
      A5O_DISPLAY_XGLX **existing_dpy;
      existing_dpy = _al_vector_ref_front(&system->system.displays);
      if (*existing_dpy != glx)
         existing_ctx = (*existing_dpy)->context;
   }

   int major = al_get_new_display_option(A5O_OPENGL_MAJOR_VERSION, 0);
   int minor = al_get_new_display_option(A5O_OPENGL_MINOR_VERSION, 0);

   if (glx->fbc) {
      bool forward_compat = (disp->flags & A5O_OPENGL_FORWARD_COMPATIBLE) != 0;
      bool core_profile = (disp->flags & A5O_OPENGL_CORE_PROFILE) != 0;
      /* Create a GLX context from FBC. */
      if (disp->flags & A5O_OPENGL_ES_PROFILE) {
         if (major == 0)
            major = 2;
         glx->context = create_context_new(glx->glx_version,
            system->gfxdisplay, *glx->fbc, existing_ctx, forward_compat,
            true, core_profile, major, minor);
      }
      else if ((disp->flags & A5O_OPENGL_3_0) || major != 0 || core_profile) {
         if (major == 0)
            major = 3;
         if (core_profile && major == 3 && minor < 2) // core profile requires at least 3.2
            minor = 2;
         glx->context = create_context_new(glx->glx_version,
            system->gfxdisplay, *glx->fbc, existing_ctx, forward_compat,
               false, core_profile, major, minor);
         /* TODO: Right now Allegro's own OpenGL driver only works with a 3.0+
          * context when using the programmable pipeline, for some reason. All
          * that's missing is probably a default shader though.
          */
         disp->extra_settings.settings[A5O_COMPATIBLE_DISPLAY] = 1;
         if (forward_compat && !(disp->flags & A5O_PROGRAMMABLE_PIPELINE))
            disp->extra_settings.settings[A5O_COMPATIBLE_DISPLAY] = 0;
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
      A5O_ERROR("Failed to create GLX context.\n");
      return false;
   }

   disp->ogl_extras->is_shared = true;

   A5O_DEBUG("Got GLX context.\n");
   return true;
}
