/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Windows OpenGL display driver
 *
 *      By Milan Mimica.
 *      Based on AllegroGL Windows display driver.
 *
 */

#include "allegro5/allegro5.h"
#include "allegro5/a5_opengl.h"
#include "allegro5/system_new.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_vector.h"
#include "allegro5/platform/aintwin.h"

#include "wgl.h"

#include <process.h>

#define PREFIX_I                "wgl-win INFO: "
#define PREFIX_W                "wgl-win WARNING: "
#define PREFIX_E                "wgl-win ERROR: "


static ALLEGRO_DISPLAY_INTERFACE *vt = 0;

/* Forward declarations: */
static void display_thread_proc(void *arg);
static void destroy_display_internals(ALLEGRO_DISPLAY_WGL *wgl_disp);

/*
 * These parameters cannot be gotten by the display thread because
 * they're thread local. We get them in the calling thread first.
 */
typedef struct new_display_parameters {
   ALLEGRO_DISPLAY_WGL *display;
   volatile bool init_failed;
   HANDLE AckEvent;
} new_display_parameters;


/* Logs a Win32 error/warning message in the log file.
 */
static void log_win32_msg(const char *prefix, const char *func,
                          const char *error_msg, DWORD err) {

   char *err_msg = NULL;
   BOOL free_msg = TRUE;

   /* Get the formatting error string from Windows. Note that only the
    * bottom 14 bits matter - the rest are reserved for various library
    * IDs and type of error.
    */
   if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_FROM_SYSTEM
                    | FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL, err & 0x3FFF,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                     (LPTSTR) &err_msg, 0, NULL)) {
      err_msg = "(Unable to decode error code)";
      free_msg = FALSE;
   }

   /* Remove two trailing characters */
   if (free_msg && err_msg && strlen(err_msg) > 1)
      *(err_msg + strlen(err_msg) - 2) = '\0';

   TRACE("%s%s(): %s %s (0x%08lx)\n", prefix, func, error_msg ? error_msg : "",
      err_msg ? err_msg : "(null)", (unsigned long)err);

   if (free_msg) {
      LocalFree(err_msg);
   }

   return;
}


/* Logs an error */
static void log_win32_error(const char *func, const char *error_msg, DWORD err) {
   log_win32_msg(PREFIX_E, func, error_msg, err);
}


/* Logs a warning */
static void log_win32_warning(const char *func, const char *error_msg, DWORD err) {
   log_win32_msg(PREFIX_W, func, error_msg, err);
}


/* Helper to set up GL state as we want it. */
static void setup_gl(ALLEGRO_DISPLAY *d)
{
   glViewport(0, 0, d->w, d->h);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, d->w, d->h, 0, -1, 1);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}

static bool is_wgl_extension_supported(AL_CONST char *extension, HDC dc)
{
   ALLEGRO_GetExtensionsStringARB_t __wglGetExtensionsStringARB;
   int ret;

   if (!glGetString(GL_EXTENSIONS))
      return false;

   __wglGetExtensionsStringARB = (ALLEGRO_GetExtensionsStringARB_t)
                                  wglGetProcAddress("wglGetExtensionsStringARB");
   ret = _al_ogl_look_for_an_extension(extension,
         (const GLubyte*)__wglGetExtensionsStringARB(dc));

   return ret;
}


static HGLRC init_temp_context(HWND wnd) {
   PIXELFORMATDESCRIPTOR pfd;
   int pf;
   HDC dc;
   HGLRC glrc;

   dc = GetDC(wnd);

   memset(&pfd, 0, sizeof(pfd));
   pfd.nSize = sizeof(pfd);
   pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL
	           | PFD_DOUBLEBUFFER_DONTCARE | PFD_STEREO_DONTCARE;
   pfd.iPixelType = PFD_TYPE_RGBA;
   pfd.iLayerType = PFD_MAIN_PLANE;
   pfd.cColorBits = 32;

   pf = ChoosePixelFormat(dc, &pfd);
   if (!pf) {
      log_win32_error("init_pixel_format_extensions", "Unable to chose a temporary pixel format!",
                      GetLastError());
      return NULL;
   }

   memset(&pfd, 0, sizeof(pfd));
   if (!SetPixelFormat(dc, pf, &pfd)) {
      log_win32_error("init_pixel_format_extensions", "Unable to set a temporary pixel format!",
                      GetLastError());
      return NULL;
   }

   glrc = wglCreateContext(dc);
   if (!glrc) {
      log_win32_error("init_pixel_format_extensions", "Unable to create a render context!",
                      GetLastError());
      return NULL;
   }

   if (!wglMakeCurrent(dc, glrc)) {
      log_win32_error("init_pixel_format_extensions", "Unable to set the render context as current!",
                      GetLastError());
      wglDeleteContext(glrc);
      return NULL;
   }

   return glrc;
}


static ALLEGRO_GetPixelFormatAttribivARB_t __wglGetPixelFormatAttribivARB = NULL;
static ALLEGRO_GetPixelFormatAttribivEXT_t __wglGetPixelFormatAttribivEXT = NULL;

static bool init_pixel_format_extensions(void)
{
   /* Load the ARB_p_f symbol - Note, we shouldn't use the extension
    * mechanism here, because it hasn't been initialized yet!
    */
   __wglGetPixelFormatAttribivARB =
      (ALLEGRO_GetPixelFormatAttribivARB_t)wglGetProcAddress("wglGetPixelFormatAttribivARB");
   __wglGetPixelFormatAttribivEXT = 
      (ALLEGRO_GetPixelFormatAttribivEXT_t)wglGetProcAddress("wglGetPixelFormatAttribivEXT");

   if (!__wglGetPixelFormatAttribivARB && !__wglGetPixelFormatAttribivEXT) {
      TRACE(PREFIX_E "init_pixel_format_extensions(): WGL_ARB/EXT_pf not supported!\n");
      return false;
   }

   return true;
}


static int get_pixel_formats_count_old(HDC dc) {
   PIXELFORMATDESCRIPTOR pfd;
   int ret;

   ret = DescribePixelFormat(dc, 1, sizeof(pfd), &pfd);
   if (!ret) {
      log_win32_error("get_pixel_formats_count_old",
                      "DescribePixelFormat failed!", GetLastError());
   }

   return ret;
}


static int get_pixel_formats_count_ext(HDC dc) {
   int attrib[1];
   int value[1];

   attrib[0] = WGL_NUMBER_PIXEL_FORMATS_ARB;
   if ((__wglGetPixelFormatAttribivARB(dc, 0, 0, 1, attrib, value) == GL_FALSE)
    && (__wglGetPixelFormatAttribivEXT(dc, 0, 0, 1, attrib, value) == GL_FALSE)) {
        log_win32_error("get_pixel_formats_count", "WGL_ARB/EXT_pixel_format use failed!",
                     GetLastError());
   }

   return value[0];
}


#ifdef DEBUGMODE
static void display_pixel_format(OGL_PIXEL_FORMAT *pf) {
   TRACE(PREFIX_I "Accelarated: %s\n", pf->rmethod ? "yes" : "no");
   TRACE(PREFIX_I "Doublebuffer: %s\n", pf->doublebuffered ? "yes" : "no");
   if (pf->swap_method > 0)
      TRACE(PREFIX_I "Swap method: %s\n", pf->swap_method == 2 ? "flip" : "copy");
   else
      TRACE(PREFIX_I "Swap method: undefined\n");
   TRACE(PREFIX_I "Color format: r%i g%i b%i a%i, %i bit\n", pf->r_size,
      pf->g_size, pf->b_size, pf->a_size, pf->color_size);
   TRACE(PREFIX_I "Depth buffer: %i bits\n", pf->depth_size);
}
#endif


static bool deduce_color_format(OGL_PIXEL_FORMAT *pf)
{
   /* dummy value to check if the format was detected */
   pf->format = ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA;

   if (pf->r_size == 8 && pf->g_size == 8 && pf->b_size == 8) {
      if (pf->a_size == 8 && pf->color_size == 32) {
         if (pf->a_shift == 0 && pf->b_shift == 8 && pf->g_shift == 16 && pf->r_shift == 24) {
            pf->format = ALLEGRO_PIXEL_FORMAT_RGBA_8888;
         }
         else if (pf->a_shift == 24 && pf->r_shift == 0 && pf->g_shift == 8 && pf->b_shift == 16) {
            pf->format = ALLEGRO_PIXEL_FORMAT_ABGR_8888;
         }
         else if (pf->a_shift == 24 && pf->r_shift == 16 && pf->g_shift == 8 && pf->b_shift == 0) {
            pf->format = ALLEGRO_PIXEL_FORMAT_ARGB_8888;
         }
      }
      else if (pf->a_size == 0 && pf->color_size == 24) {
         if (pf->b_shift == 0 && pf->g_shift == 8 && pf->r_shift == 16) {
            pf->format = ALLEGRO_PIXEL_FORMAT_RGB_888;
         }
         else if (pf->r_shift == 0 && pf->g_shift == 8 && pf->b_shift == 16) {
            pf->format = ALLEGRO_PIXEL_FORMAT_BGR_888;
         }
      }
      else if (pf->a_size == 0 && pf->color_size == 32) {
         if (pf->b_shift == 0 && pf->g_shift == 8 && pf->r_shift == 16) {
            pf->format = ALLEGRO_PIXEL_FORMAT_XRGB_8888;
         }
         else if (pf->r_shift == 0 && pf->g_shift == 8 && pf->b_shift == 16) {
            pf->format = ALLEGRO_PIXEL_FORMAT_XBGR_8888;
         }
      }
   }
   else if (pf->r_size == 5 && pf->g_size == 6 && pf->b_size == 5) {
      if (pf->r_shift == 0 && pf->g_shift == 5 && pf->b_shift == 11) {
         pf->format = ALLEGRO_PIXEL_FORMAT_BGR_565;
      }
      else if (pf->b_shift == 0 && pf->g_shift == 5 && pf->r_shift == 11) {
         pf->format = ALLEGRO_PIXEL_FORMAT_RGB_565;
      }
   }

   if (pf->format == ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA)
      return false;

   return true;
}


static int decode_pixel_format_old(PIXELFORMATDESCRIPTOR *pfd, OGL_PIXEL_FORMAT *pf)
{
   TRACE(PREFIX_I "Decoding: \n");

	/* Not interested if it doesn't support OpenGL and RGBA */
   if (!(pfd->dwFlags & PFD_SUPPORT_OPENGL)) {
      TRACE(PREFIX_I "OpenGL Unsupported\n");
      return false;
   }
   if (pfd->iPixelType != PFD_TYPE_RGBA) {
      TRACE(PREFIX_I "Not RGBA mode\n");
      return false;
   }

   /* hardware acceleration */
   if (((pfd->dwFlags & PFD_GENERIC_ACCELERATED) && (pfd->dwFlags & PFD_GENERIC_FORMAT))
   || (!(pfd->dwFlags & PFD_GENERIC_ACCELERATED) && !(pfd->dwFlags & PFD_GENERIC_FORMAT)))
      pf->rmethod = 1;
   else
      pf->rmethod = 0;

   /* Depths of colour buffers */
   pf->r_size = pfd->cRedBits;
   pf->g_size = pfd->cGreenBits;
   pf->b_size = pfd->cBlueBits;
   pf->a_size = pfd->cAlphaBits;

   /* Miscellaneous settings */
   pf->doublebuffered = pfd->dwFlags & PFD_DOUBLEBUFFER;
   pf->depth_size = pfd->cDepthBits;
   pf->stencil_size = pfd->cStencilBits;
   pf->color_size = pfd->cColorBits;

   /* These are the component shifts. */
   pf->r_shift = pfd->cRedShift;
   pf->g_shift = pfd->cGreenShift;
   pf->b_shift = pfd->cBlueShift;
   pf->a_shift = pfd->cAlphaShift;

   /* Multisampling isn't supported under Windows if we don't also use
    * WGL_ARB_pixel_format or WGL_EXT_pixel_format.
    */
   pf->sample_buffers = 0;
   pf->samples = 0;

   /* Swap method can't be detected without WGL_ARB_pixel_format or
    * WGL_EXT_pixel_format
    */
   pf->swap_method = 0;

   /* Float depth/color isn't supported under Windows if we don't also use
    * AGL_ARB_pixel_format or WGL_EXT_pixel_format.
    */
   pf->float_color = 0;
   pf->float_depth = 0;

   /* FIXME: there is other, potetialy usefull, info in pfd. */

   if (!deduce_color_format(pf)) {
      TRACE(PREFIX_I "Color format not supported by allegro.\n");
      return false;
   }

	return true;
}


static bool decode_pixel_format_attrib(OGL_PIXEL_FORMAT *pf, int num_attribs,
                                      const int *attrib, const int *value) {
   int i;

   TRACE(PREFIX_I "Decoding: \n");

   pf->samples = 0;
   pf->sample_buffers = 0;
   pf->float_depth = 0;
   pf->float_color = 0;

   for (i = 0; i < num_attribs; i++) {
      /* Not interested if it doesn't support OpenGL or window drawing or RGBA. */
      if (attrib[i] == WGL_SUPPORT_OPENGL_ARB && value[i] == 0) {	
         TRACE(PREFIX_I "OpenGL Unsupported\n");
         return false;
      }
      else if (attrib[i] == WGL_DRAW_TO_WINDOW_ARB && value[i] == 0) {	
         TRACE(PREFIX_I "Can't draw to window\n");
         return false;
      }
      else if (attrib[i] == WGL_PIXEL_TYPE_ARB
         && (value[i] != WGL_TYPE_RGBA_ARB && value[i] != WGL_TYPE_RGBA_FLOAT_ARB)) {
         TRACE(PREFIX_I "Not RGBA mode\n");
         return false;
      }
      /* hardware acceleration */
      else if (attrib[i] == WGL_ACCELERATION_ARB) {
         pf->rmethod = (value[i] == WGL_NO_ACCELERATION_ARB) ? 0 : 1;
      }
      /* Depths of colour buffers */
      else if (attrib[i] == WGL_RED_BITS_ARB) {
         pf->r_size = value[i];
      }
      else if (attrib[i] == WGL_GREEN_BITS_ARB) {
         pf->g_size = value[i];
      }
      else if (attrib[i] == WGL_BLUE_BITS_ARB) {
         pf->b_size = value[i];
      }
      else if (attrib[i] == WGL_ALPHA_BITS_ARB) {
         pf->a_size = value[i];
      }
      /* Shift of color components */
      else if (attrib[i] == WGL_RED_SHIFT_ARB) {
         pf->r_shift = value[i];
      }
      else if (attrib[i] == WGL_GREEN_SHIFT_ARB) {
         pf->g_shift = value[i];
      }
      else if (attrib[i] == WGL_BLUE_SHIFT_ARB) {
         pf->b_shift = value[i];
      }
      else if (attrib[i] == WGL_ALPHA_SHIFT_ARB) {
         pf->a_shift = value[i];
      }
      /* Miscellaneous settings */
      else if (attrib[i] == WGL_DOUBLE_BUFFER_ARB) {
         pf->doublebuffered = value[i];
      }
      else if (attrib[i] == WGL_SWAP_METHOD_ARB) {
         if (value[i] == WGL_SWAP_UNDEFINED_ARB)
            pf->swap_method = 0;
         else if (value[i] == WGL_SWAP_COPY_ARB)
            pf->swap_method = 1;
         else if (value[i] == WGL_SWAP_EXCHANGE_ARB) 
            pf->swap_method = 2;
      }

      /* XXX: enable if needed, unused currently */
#if 0
      else if (attrib[i] == WGL_STEREO_ARB) {
         pf->stereo = value[i];
      }
      else if (attrib[i] == WGL_AUX_BUFFERS_ARB) {
         pf->aux_buffers = value[i];
      }
      else if (attrib[i] == WGL_STENCIL_BITS_ARB) {
         pf->stencil_size = value[i];
      }
      /* Depths of accumulation buffer */
      else if (attrib[i] == WGL_ACCUM_RED_BITS_ARB) {
         pf->accum_size.rgba.r = value[i];
      }
      else if (attrib[i] == WGL_ACCUM_GREEN_BITS_ARB) {
         pf->accum_size.rgba.g = value[i];
      }
      else if (attrib[i] == WGL_ACCUM_BLUE_BITS_ARB) {
         pf->accum_size.rgba.b = value[i];
      }
      else if (attrib[i] == WGL_ACCUM_ALPHA_BITS_ARB) {
         pf->accum_size.rgba.a = value[i];
      }
#endif // #if 0

      else if (attrib[i] == WGL_DEPTH_BITS_ARB) {
         pf->depth_size = value[i];
      }
      else if (attrib[i] == WGL_COLOR_BITS_ARB) {
         pf->color_size = value[i];
      }
      /* Multisampling bits */
      else if (attrib[i] == WGL_SAMPLE_BUFFERS_ARB) {
         pf->sample_buffers = value[i];
      }
      else if (attrib[i] == WGL_SAMPLES_ARB) {
         pf->samples = value[i];
      }
      /* Float color */
      if (attrib[i] == WGL_PIXEL_TYPE_ARB && value[i] == WGL_TYPE_RGBA_FLOAT_ARB) {
         pf->float_color = TRUE;
      }
      /* Float depth */
      else if (attrib[i] == WGL_DEPTH_FLOAT_EXT) {
         pf->float_depth = value[i];
      }
   }

   /* Setting some things based on what we've read out of the PFD. */

   if (!deduce_color_format(pf)) {
      TRACE(PREFIX_I "Color format not supported by allegro.\n");
      return false;
   }


   return true;
}


static OGL_PIXEL_FORMAT* read_pixel_format_old(int fmt, HDC dc) {
   OGL_PIXEL_FORMAT *pf = NULL;
   PIXELFORMATDESCRIPTOR pfd;
   int result;

   result = DescribePixelFormat(dc, fmt, sizeof(pfd), &pfd);
   if (!result) {
      log_win32_warning("read_pixel_format_old",
                        "DescribePixelFormat() failed!", GetLastError());
      return NULL;
   }

   pf = malloc(sizeof *pf);
   if (!decode_pixel_format_old(&pfd, pf)) {
      free(pf);
      return NULL;
   }

   return pf;
}


static OGL_PIXEL_FORMAT* read_pixel_format_ext(int fmt, HDC dc) {
   OGL_PIXEL_FORMAT *pf = NULL;

   /* Note: Even though we use te ARB suffix, all those enums are compatible
    * with EXT_pixel_format.
    */
   int attrib[] = {
      WGL_SUPPORT_OPENGL_ARB,
      WGL_DRAW_TO_WINDOW_ARB,
      WGL_PIXEL_TYPE_ARB,
      WGL_ACCELERATION_ARB,
      WGL_DOUBLE_BUFFER_ARB,
      WGL_DEPTH_BITS_ARB,
      WGL_SWAP_METHOD_ARB,
      WGL_COLOR_BITS_ARB,
      WGL_RED_BITS_ARB,
      WGL_GREEN_BITS_ARB,
      WGL_BLUE_BITS_ARB,
      WGL_ALPHA_BITS_ARB,
      WGL_RED_SHIFT_ARB,
      WGL_GREEN_SHIFT_ARB,
      WGL_BLUE_SHIFT_ARB,
      WGL_ALPHA_SHIFT_ARB,
      WGL_STENCIL_BITS_ARB,
      WGL_STEREO_ARB,
      WGL_ACCUM_BITS_ARB,
      WGL_ACCUM_RED_BITS_ARB,
      WGL_ACCUM_GREEN_BITS_ARB,
      WGL_ACCUM_BLUE_BITS_ARB,
      WGL_ACCUM_ALPHA_BITS_ARB,
      WGL_AUX_BUFFERS_ARB,

      /* The following are used by extensions that add to WGL_pixel_format.
       * If WGL_p_f isn't supported though, we can't use the (then invalid)
       * enums. We can't use any magic number either, so we settle for 
       * replicating one. The pixel format decoder
       * (decode_pixel_format_attrib()) doesn't care about duplicates.
       */
      WGL_AUX_BUFFERS_ARB, /* placeholder for WGL_SAMPLE_BUFFERS_ARB */
      WGL_AUX_BUFFERS_ARB, /* placeholder for WGL_SAMPLES_ARB        */
      WGL_AUX_BUFFERS_ARB, /* placeholder for WGL_DEPTH_FLOAT_EXT    */
   };

   const int num_attribs = sizeof(attrib) / sizeof(attrib[0]);
   int *value = (int*)malloc(sizeof(int) * num_attribs);
   int ret;

   if (!value)
      return NULL;

   /* If multisampling is supported, query for it. */
   if (is_wgl_extension_supported("WGL_ARB_multisample", dc)) {
      attrib[num_attribs - 3] = WGL_SAMPLE_BUFFERS_ARB;
      attrib[num_attribs - 2] = WGL_SAMPLES_ARB;
   }
   if (is_wgl_extension_supported("WGL_EXT_depth_float", dc)) {
      attrib[num_attribs - 1] = WGL_DEPTH_FLOAT_EXT;
   }

   /* Get the pf attributes */
   if (__wglGetPixelFormatAttribivARB) {
      ret = __wglGetPixelFormatAttribivARB(dc, fmt, 0, num_attribs, attrib, value);
   }
   else if (__wglGetPixelFormatAttribivEXT) {
      ret = __wglGetPixelFormatAttribivEXT(dc, fmt, 0, num_attribs, attrib, value);
   }
   else {
      ret = 0;
   }
   
   if (!ret) {
      log_win32_error("read_pixel_format_ext", "wglGetPixelFormatAttrib failed!",
                      GetLastError());
      free(value);
      return NULL;
   }

   pf = malloc(sizeof *pf);
   if (!decode_pixel_format_attrib(pf, num_attribs, attrib, value)) {
      free(pf);
      pf = NULL;
   }

   free(value);

   return pf;
}


static bool change_display_mode(ALLEGRO_DISPLAY *d) {
   DEVMODE dm;
   DEVMODE fallback_dm;
   DISPLAY_DEVICE dd;
   char* dev_name = NULL;
   int i, modeswitch, result;
   int fallback_dm_valid = 0;
   int bpp;
   int adapter = al_get_current_video_adapter();

   if (adapter != -1) {
      memset(&dd, 0, sizeof(dd));
      dd.cb = sizeof(dd);
      if (EnumDisplayDevices(NULL, adapter, &dd, 0) == FALSE)
         return false;
      dev_name = dd.DeviceName;
   }

   memset(&fallback_dm, 0, sizeof(fallback_dm));
   memset(&dm, 0, sizeof(dm));
   dm.dmSize = sizeof(DEVMODE);

   bpp = al_get_pixel_format_bits(d->format);
   if (!bpp)
      bpp = 32;

   i = 0;
   do {
      modeswitch = EnumDisplaySettings(dev_name, i, &dm);
      if (!modeswitch)
         break;

      if ((dm.dmPelsWidth  == (unsigned) d->w)
       && (dm.dmPelsHeight == (unsigned) d->h)
       && (dm.dmBitsPerPel == (unsigned) bpp)
       && (dm.dmDisplayFrequency != (unsigned) d->refresh_rate)) {
         /* Keep it as fallback if refresh rate request could not
          * be satisfied. Try to get as close to 60Hz as possible though,
          * it's a bit better for a fallback than just blindly picking
          * something like 47Hz or 200Hz.
          */
         if (!fallback_dm_valid) {
            fallback_dm = dm;
            fallback_dm_valid = 1;
         }
         else if (dm.dmDisplayFrequency >= 60) {
            if (dm.dmDisplayFrequency < fallback_dm.dmDisplayFrequency) {
                fallback_dm = dm;
            }
         }
      }
      i++;
   }
   while ((dm.dmPelsWidth  != (unsigned) d->w)
       || (dm.dmPelsHeight != (unsigned) d->h)
       || (dm.dmBitsPerPel != (unsigned) bpp)
       || (dm.dmDisplayFrequency != (unsigned) d->refresh_rate));

   if (!modeswitch && !fallback_dm_valid) {
      TRACE(PREFIX_E "change_display_mode: Mode not found.\n");
      return false;
   }

   if (!modeswitch && fallback_dm_valid)
      dm = fallback_dm;

   dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;
   result = ChangeDisplaySettingsEx(dev_name, &dm, NULL, CDS_FULLSCREEN, 0);

   if (result != DISP_CHANGE_SUCCESSFUL) {
      log_win32_error("change_display_mode", "Unable to set mode!",
                      GetLastError());
      return false;
   }

   TRACE(PREFIX_I "change_display_mode: Mode seccessfuly set.\n");
   return true;
}


static OGL_PIXEL_FORMAT** get_available_pixel_formats_ext(int *count) {
   HWND testwnd = NULL;
   HDC testdc   = NULL;
   HGLRC testrc = NULL;
   HGLRC old_rc = NULL;
   HDC old_dc   = NULL;
   OGL_PIXEL_FORMAT **pf_list = NULL;
   int maxindex;
   int i;

   /* We need to create a dummy window with a pixel format to get the
    * list of valid PFDs
    */
   testwnd = _al_win_create_hidden_window();
   if (!testwnd)
      return false;

   old_rc = wglGetCurrentContext();
   old_dc = wglGetCurrentDC();

   testdc = GetDC(testwnd);
   testrc = init_temp_context(testwnd);
   if (!testrc)
      goto bail;

   if (!init_pixel_format_extensions())
      goto bail;

   maxindex = get_pixel_formats_count_ext(testdc);
   if (maxindex < 1)
      goto bail;
   *count = maxindex;
   TRACE(PREFIX_I "get_available_pixel_formats_ext(): Got %i visuals.\n", maxindex);

   pf_list = malloc(maxindex * sizeof(*pf_list));
   if (!pf_list)
      goto bail;

   for (i = 1; i <= maxindex; i++) {
      pf_list[i-1] = read_pixel_format_ext(i, testdc);
#ifdef DEBUGMODE
      if (pf_list[i-1]) {
         display_pixel_format(pf_list[i-1]);
         TRACE("-- \n");
      }
#endif
   }

bail:
   wglMakeCurrent(NULL, NULL);
   if (testrc) {
      wglDeleteContext(testrc);
   }

   wglMakeCurrent(old_dc, old_rc);

   __wglGetPixelFormatAttribivARB = NULL;
   __wglGetPixelFormatAttribivEXT = NULL;

   if (testwnd) {
      ReleaseDC(testwnd, testdc);
      DestroyWindow(testwnd);
   }

   return pf_list;
}


static OGL_PIXEL_FORMAT** get_available_pixel_formats_old(int *count, HDC dc) {
   OGL_PIXEL_FORMAT **pf_list = NULL;
   int maxindex;
   int i;

   maxindex = get_pixel_formats_count_old(dc);
   if (maxindex < 1)
      return NULL;
   *count = maxindex;
   TRACE(PREFIX_I "get_available_pixel_formats_old(): Got %i visuals.\n", maxindex);

   pf_list = malloc(maxindex * sizeof(*pf_list));
   if (!pf_list)
      return NULL;

   for (i = 1; i <= maxindex; i++) {
      TRACE(PREFIX_I "Decoding visual no. %i...\n", i);
      pf_list[i-1] = read_pixel_format_old(i, dc);
#ifdef DEBUGMODE
      if (pf_list[i-1]) {
         display_pixel_format(pf_list[i-1]);
         TRACE("-- \n");
      }
#endif
   }

   return pf_list;
}


static bool select_pixel_format(ALLEGRO_DISPLAY_WGL *d, HDC dc) {
   OGL_PIXEL_FORMAT **pf_list = NULL;
   enum ALLEGRO_PIXEL_FORMAT format = ((ALLEGRO_DISPLAY *) d)->format;
   bool want_sb = ((ALLEGRO_DISPLAY *) d)->flags & ALLEGRO_SINGLEBUFFER;
   int maxindex = 0;
   int i;
   int pf_index_fallback = -1;
   int pf_index = -1;

   /* XXX
    * The correct and more logical way would be to first try the more
    * modern _ext mode and then fallback to the _old mode, but the _old
    * mode is less complicated and better supported by WINE. And faster.
    * We don't use any of the potential features provided by _ext anyway.
    */
/*
   pf_list = get_available_pixel_formats_ext(&maxindex);
   if (!pf_list)
      pf_list = get_available_pixel_formats_old(&maxindex, dc);
*/
   pf_list = get_available_pixel_formats_old(&maxindex, dc);
   if (!pf_list)
      pf_list = get_available_pixel_formats_ext(&maxindex);

   for (i = 1; i <= maxindex; i++) {
      OGL_PIXEL_FORMAT *pf = pf_list[i-1];
      /* TODO: implement a choice system (scoring?) */
      if (pf
         && pf->doublebuffered == !want_sb
         && _al_pixel_format_fits(pf->format, format)
         && pf->float_color == 0
         && pf->float_depth == 0
         && pf->sample_buffers == 0) {

         if (pf->rmethod && pf_index == -1) {
            pf_index = i;
         }
         else if (!pf->rmethod && pf_index_fallback == -1) {
            pf_index_fallback = i;
         }

         /* we chose our pixels formats */
         if (pf_index != -1 && pf_index_fallback != -1)
            break;
      }
   }

   if (pf_index == -1 && pf_index_fallback == -1) {
      TRACE(PREFIX_E "Couldn't find a suitable pixel format\n");
      for (i = 0; i < maxindex; i++)
         free(pf_list[i]);
      free(pf_list);
      return false;
   }

   if (pf_index == -1 && pf_index_fallback != -1)
      pf_index = pf_index_fallback;

   if (SetPixelFormat(d->dc, pf_index, NULL)) {
      OGL_PIXEL_FORMAT *pf = pf_list[pf_index - 1];
#ifdef DEBUGMODE
      TRACE(PREFIX_I "select_pixel_format(): Chose visual no. %i\n\n", pf_index);
      display_pixel_format(pf);
#endif
   }
   else {
      TRACE(PREFIX_E "Unable to set pixel format!\n");
      log_win32_error("select_pixel_format", "Unable to set pixel format!",
         GetLastError());

      for (i = 0; i < maxindex; i++)
         free(pf_list[i]);
      free(pf_list);
      return false;
   }

   d->win_display.display.format = pf_list[pf_index - 1]->format;

   for (i = 0; i < maxindex; i++)
      free(pf_list[i]);

   if (pf_list)
      free(pf_list);

   return true;
}


static bool create_display_internals(ALLEGRO_DISPLAY_WGL *wgl_disp) {
   ALLEGRO_DISPLAY     *disp     = (void*)wgl_disp;
   ALLEGRO_DISPLAY_WIN *win_disp = (void*)wgl_disp;
   new_display_parameters ndp;
   HANDLE window_thread;

   ndp.display = wgl_disp;
   ndp.init_failed = true;
   ndp.AckEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
   window_thread = (HANDLE)_beginthread(display_thread_proc, 0, &ndp);

   /* Give some time for dispaly thread do init, but don't block if
    * something horrible happened to it. */
   WaitForSingleObject(ndp.AckEvent, 10*1000);
   CloseHandle(ndp.AckEvent);

   if (ndp.init_failed) {
      TRACE(PREFIX_E "Failed to create display.\n");
      return false;
   }

   /* make the context the current one */
   if (!wglMakeCurrent(wgl_disp->dc, wgl_disp->glrc)) {
      log_win32_error("create_display_internals",
                      "Unable to make the context current!",
                       GetLastError());
      destroy_display_internals(wgl_disp);
      return false;
   }

   _al_ogl_manage_extensions(disp);
   _al_ogl_set_extensions(disp->ogl_extras->extension_api);

   disp->ogl_extras->backbuffer = _al_ogl_create_backbuffer(disp);
   if (!disp->ogl_extras->backbuffer) {
      TRACE(PREFIX_E "Failed to create a backbuffer.\n");
      destroy_display_internals(wgl_disp);
      return false;
   }
 
   win_disp->mouse_range_x1 = 0;
   win_disp->mouse_range_y1 = 0;
   win_disp->mouse_range_x2 = disp->w;
   win_disp->mouse_range_y2 = disp->h;
   /* XXX: These calls use the current display (which is not set to this
           display yet). Mouse routines semantics have to be sorted out first.
   if (al_is_mouse_installed()) {
      al_set_mouse_xy(disp->w/2, disp->h/2);
      al_set_mouse_range(0, 0, disp->w, disp->h);
   }*/

   win_disp->mouse_selected_hcursor = 0;
   win_disp->mouse_cursor_shown = false;

   _al_win_grab_input(win_disp);

   setup_gl(disp);

   return true;
}


static ALLEGRO_DISPLAY* wgl_create_display(int w, int h) {
   ALLEGRO_SYSTEM_WIN *system = (ALLEGRO_SYSTEM_WIN *)al_system_driver();
   ALLEGRO_DISPLAY_WGL **add;
   ALLEGRO_DISPLAY_WGL *wgl_display = _AL_MALLOC(sizeof *wgl_display);
   ALLEGRO_DISPLAY *ogl_display = (void*)wgl_display;
   ALLEGRO_DISPLAY     *display     = (void*)ogl_display;

   memset(display, 0, sizeof *wgl_display);
   display->w = w;
   display->h = h;
   display->format = al_get_new_display_format();
   display->refresh_rate = al_get_new_display_refresh_rate();
   display->flags = al_get_new_display_flags();
   display->vt = vt;

   display->ogl_extras = _AL_MALLOC(sizeof(ALLEGRO_OGL_EXTRAS));
   memset(display->ogl_extras, 0, sizeof(ALLEGRO_OGL_EXTRAS));

   if (!create_display_internals(wgl_display)) {
      _AL_FREE(display->ogl_extras);
      _AL_FREE(display);
      return NULL;
   }

   /* Print out OpenGL version info */
   TRACE(PREFIX_I "OpenGL Version: %s\n", (const char*)glGetString(GL_VERSION));
   TRACE(PREFIX_I "Vendor: %s\n", (const char*)glGetString(GL_VENDOR));
   TRACE(PREFIX_I "Renderer: %s\n\n", (const char*)glGetString(GL_RENDERER));

   /* Add ourself to the list of displays. */
   add = _al_vector_alloc_back(&system->system.displays);
   *add = wgl_display;

   /* Each display is an event source. */
   _al_event_source_init(&display->es);

   _al_win_set_system_mouse_cursor(display, ALLEGRO_SYSTEM_MOUSE_CURSOR_ARROW);
   _al_win_show_mouse_cursor(display);

   return display;
}


static void destroy_display_internals(ALLEGRO_DISPLAY_WGL *wgl_disp) {
   ALLEGRO_DISPLAY *disp = (ALLEGRO_DISPLAY *)wgl_disp;
   ALLEGRO_DISPLAY_WIN *win_disp = (ALLEGRO_DISPLAY_WIN *)wgl_disp;

   /* We need to convert all our bitmaps to display independent (memory)
    * bitmaps because WGL driver doesn't support sharing of resources. */
   while (disp->bitmaps._size > 0) {
      ALLEGRO_BITMAP **bptr = _al_vector_ref_back(&disp->bitmaps);
      ALLEGRO_BITMAP *bmp = *bptr;
      _al_convert_to_memory_bitmap(bmp);
   }

   if (disp->ogl_extras->backbuffer)
      _al_ogl_destroy_backbuffer(disp->ogl_extras->backbuffer);
   disp->ogl_extras->backbuffer = NULL;

   _al_ogl_unmanage_extensions(disp);

   PostMessage(win_disp->window, _al_win_msg_suicide, 0, 0);
   while (!win_disp->thread_ended)
      al_rest(0.001);
}


static void wgl_destroy_display(ALLEGRO_DISPLAY *disp)
{
   ALLEGRO_SYSTEM_WIN *system = (ALLEGRO_SYSTEM_WIN *)al_system_driver();
   ALLEGRO_DISPLAY_WGL *wgl_disp = (ALLEGRO_DISPLAY_WGL *)disp;

   destroy_display_internals(wgl_disp);
   _al_event_source_free(&disp->es);
   _al_vector_find_and_delete(&system->system.displays, &disp);

   _al_vector_free(&disp->bitmaps);
   _AL_FREE(disp->ogl_extras);
   _AL_FREE(wgl_disp);
}


static bool wgl_set_current_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_WGL *wgl_disp = (ALLEGRO_DISPLAY_WGL *)d;
   ALLEGRO_DISPLAY *ogl_disp = (ALLEGRO_DISPLAY *)d;
   HGLRC current_glrc;

   current_glrc = wglGetCurrentContext();

   if (current_glrc && current_glrc != wgl_disp->glrc) {
      /* make the context the current one */
      if (!wglMakeCurrent(wgl_disp->dc, wgl_disp->glrc)) {
         log_win32_error("wgl_set_current_display", "Unable to make the context current!",
                          GetLastError());
         return false;
      }

      _al_ogl_set_extensions(ogl_disp->ogl_extras->extension_api);
   }

   return true;
}


/*
 * The window must be created in the same thread that
 * runs the message loop.
 */
static void display_thread_proc(void *arg)
{
   new_display_parameters *ndp = arg;
   ALLEGRO_DISPLAY *disp = (ALLEGRO_DISPLAY*)ndp->display;
   ALLEGRO_DISPLAY_WGL *wgl_disp = (void*)disp;
   ALLEGRO_DISPLAY_WIN *win_disp = (void*)disp;
   MSG msg;

   /* So that we can call the functions using TLS from this thread. */
   al_set_new_display_flags(disp->flags);
   
   if (disp->flags & ALLEGRO_FULLSCREEN) {
      if (!change_display_mode(disp)) {
         win_disp->thread_ended = true;
         destroy_display_internals(wgl_disp);
         SetEvent(ndp->AckEvent);
         return;
      }
   }

   win_disp->window = _al_win_create_window(disp, disp->w, disp->h, disp->flags);

   if (!win_disp->window) {
      win_disp->thread_ended = true;
      destroy_display_internals(wgl_disp);
      SetEvent(ndp->AckEvent);
      return;
   }

   /* FIXME: can't _al_win_create_window() do this? */
   if (disp->flags & ALLEGRO_FULLSCREEN) {
      RECT rect;
      rect.left = 0;
      rect.right = disp->w;
      rect.top  = 0;
      rect.bottom = disp->h;
      SetWindowPos(win_disp->window, 0, rect.left, rect.top,
             rect.right - rect.left, rect.bottom - rect.top,
             SWP_NOZORDER | SWP_FRAMECHANGED);
   }

   /* Yep, the following is really needed sometimes. */
   /* <rohannessian> Win98/2k/XP's window forground rules don't let us
	 * make our window the topmost window on launch. This causes issues on 
	 * full-screen apps, as DInput loses input focus on them.
	 * We use this trick to force the window to be topmost, when switching
	 * to full-screen only. Note that this only works for Win98 and greater.
	 * Win95 will ignore our SystemParametersInfo() calls.
	 * 
	 * See http://support.microsoft.com:80/support/kb/articles/Q97/9/25.asp
	 * for details.
	 */
	{
		DWORD lock_time;
      HWND wnd = win_disp->window;

#define SPI_GETFOREGROUNDLOCKTIMEOUT 0x2000
#define SPI_SETFOREGROUNDLOCKTIMEOUT 0x2001
      if (disp->flags & ALLEGRO_FULLSCREEN) {
			SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT,
			                     0, (LPVOID)&lock_time, 0);
			SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT,
			                     0, (LPVOID)0,
			                     SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
		}

		ShowWindow(wnd, SW_SHOWNORMAL);
		SetForegroundWindow(wnd);
		/* In some rare cases, it doesn't seem to work without the loop. And we
		 * absolutely need this to succeed, else we trap the user in a
		 * fullscreen window without input.
		 */
		while (GetForegroundWindow() != wnd) {
			al_rest(0.01);
			SetForegroundWindow(wnd);
		}
		UpdateWindow(wnd);

		if (disp->flags & ALLEGRO_FULLSCREEN) {
			SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT,
			                     0, (LPVOID)lock_time,
			                     SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
		}
#undef SPI_GETFOREGROUNDLOCKTIMEOUT
#undef SPI_SETFOREGROUNDLOCKTIMEOUT
	}

   /* get the device context of our window */
   wgl_disp->dc = GetDC(win_disp->window);

   if (!select_pixel_format(wgl_disp, wgl_disp->dc)) {
      win_disp->thread_ended = true;
      destroy_display_internals(wgl_disp);
      SetEvent(ndp->AckEvent);
      return;
   }

   /* create an OpenGL context */
   wgl_disp->glrc = wglCreateContext(wgl_disp->dc);
   if (!wgl_disp->glrc) {
      log_win32_error("wgl_disp_display_thread_proc", "Unable to create a render context!",
                      GetLastError());
      win_disp->thread_ended = true;
      destroy_display_internals(wgl_disp);
      SetEvent(ndp->AckEvent);
      return;
   }

   win_disp->thread_ended = false;
   win_disp->end_thread = false;
   ndp->init_failed = false;
   SetEvent(ndp->AckEvent);

   while (!win_disp->end_thread) {
      if (WaitMessage()) {
         /* messages are waiting in the queue */
         while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            if (GetMessage(&msg, NULL, 0, 0)) {
               DispatchMessage(&msg);
            }
            else {
               goto End;
            }
         }
      }
      else {
         TRACE(PREFIX_E "Wait failed.\n");
         break;
      }
   }

End:
   if (wgl_disp->glrc) {
      wglDeleteContext(wgl_disp->glrc);
      wgl_disp->glrc = NULL;
   }
   if (wgl_disp->dc) {
      ReleaseDC(win_disp->window, wgl_disp->dc);
      wgl_disp->dc = NULL;
   }

   if (disp->flags & ALLEGRO_FULLSCREEN) {
      ChangeDisplaySettings(NULL, 0);
   }

   if (win_disp->window) {
      DestroyWindow(win_disp->window);
      win_disp->window = NULL;
   }

   TRACE("wgl display thread exits\n");
   win_disp->thread_ended = true;
}


static void wgl_flip_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_WGL* disp = (ALLEGRO_DISPLAY_WGL*)d;
   if (d->flags & ALLEGRO_SINGLEBUFFER)
      glFlush();
   else
      SwapBuffers(disp->dc);
}


static bool wgl_update_display_region(ALLEGRO_DISPLAY *d,
                                      int x, int y, int width, int height)
{
   if (al_get_opengl_extension_list()->ALLEGRO_WGL_WIN_swap_hint) {
      /* FIXME: This is just a driver hint and there is no guarantee that the
       * contens of the front buffer outside the given rectangle will be preserved,
       * thus we should really return false here and do nothing. */
      ALLEGRO_DISPLAY_WGL* disp = (ALLEGRO_DISPLAY_WGL*)d;
      wglAddSwapHintRectWIN(x, y, width, height);
      glFlush();
      SwapBuffers(disp->dc);
      return true;
   }
   return false;
}


static bool wgl_resize_display(ALLEGRO_DISPLAY *d, int width, int height)
{
   ALLEGRO_DISPLAY_WGL *wgl_disp = (ALLEGRO_DISPLAY_WGL *)d;
   ALLEGRO_DISPLAY *ogl_disp = (ALLEGRO_DISPLAY *)d;
   ALLEGRO_DISPLAY_WIN *win_disp = (ALLEGRO_DISPLAY_WIN *)d;

   if (d->flags & ALLEGRO_FULLSCREEN) {
      ALLEGRO_BITMAP *bmp = al_get_target_bitmap();
      bool was_backbuffer = false;
      if (bmp->vt)
         was_backbuffer = ((ALLEGRO_BITMAP_OGL*)bmp)->is_backbuffer;

      destroy_display_internals(wgl_disp);
      /* FIXME: no need to switch to desktop display mode in between.
       */
      d->w = width;
      d->h = height;
      if (!create_display_internals(wgl_disp))
         return false;
      /* FIXME: All display bitmaps are memory bitmaps now. We should
       * reuploaded them or prevent destroying them.
       */

      /* We have a new backbuffer now. */
      if (was_backbuffer)
         al_set_target_bitmap(al_get_backbuffer());
   }
   else {
      RECT win_size;
      WINDOWINFO wi;

      win_size.left = 0;
      win_size.top = 0;
      win_size.right = width;
      win_size.bottom = height;

      wi.cbSize = sizeof(WINDOWINFO);
      GetWindowInfo(win_disp->window, &wi);

      AdjustWindowRectEx(&win_size, wi.dwStyle, FALSE, wi.dwExStyle);

      if (!SetWindowPos(win_disp->window, HWND_TOP,
         0, 0,
         win_size.right - win_size.left,
         win_size.bottom - win_size.top,
         SWP_NOMOVE|SWP_NOZORDER))
            return false;

      d->w = width;
      d->h = height;

      _al_ogl_resize_backbuffer(ogl_disp->ogl_extras->backbuffer, width, height);

      setup_gl(d);
   }

   return true;
}


static bool wgl_acknowledge_resize(ALLEGRO_DISPLAY *d)
{
   WINDOWINFO wi;
   ALLEGRO_DISPLAY_WIN *win_disp = (ALLEGRO_DISPLAY_WIN *)d;
   ALLEGRO_DISPLAY *ogl_disp = (ALLEGRO_DISPLAY *)d;
   int w, h;

   wi.cbSize = sizeof(WINDOWINFO);
   GetWindowInfo(win_disp->window, &wi);
   w = wi.rcClient.right - wi.rcClient.left;
   h = wi.rcClient.bottom - wi.rcClient.top;

   d->w = w;
   d->h = h;

   _al_ogl_resize_backbuffer(ogl_disp->ogl_extras->backbuffer, w, h);

   setup_gl(d);

   return true;
}


static bool wgl_is_compatible_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   /* Note: WGL driver doesn't support sharing of resources between contexts,
    * thus all bitmaps are tied to the display which was current at the time
    * al_create_bitmap was called.
    */
   return display == bitmap->display;
}


static void wgl_switch_in(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_WIN *win_disp = (ALLEGRO_DISPLAY_WIN *)display;

   if (al_is_mouse_installed())
      al_set_mouse_range(win_disp->mouse_range_x1, win_disp->mouse_range_y1,
         win_disp->mouse_range_x2, win_disp->mouse_range_y2);
}


static void wgl_switch_out(ALLEGRO_DISPLAY *display)
{
}


static void wgl_set_window_position(ALLEGRO_DISPLAY *display, int x, int y)
{
   _al_win_set_window_position(((ALLEGRO_DISPLAY_WIN *)display)->window, x, y);
}


static void wgl_get_window_position(ALLEGRO_DISPLAY *display, int *x, int *y)
{
   _al_win_get_window_position(((ALLEGRO_DISPLAY_WIN *)display)->window, x, y);
}


static void wgl_toggle_frame(ALLEGRO_DISPLAY *display, bool onoff)
{
   _al_win_toggle_window_frame(
      display,
      ((ALLEGRO_DISPLAY_WIN *)display)->window,
      display->w, display->h, onoff);
}


/* Obtain a reference to this driver. */
ALLEGRO_DISPLAY_INTERFACE *_al_display_wgl_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->create_display = wgl_create_display;
   vt->destroy_display = wgl_destroy_display;
   vt->resize_display = wgl_resize_display;
   vt->set_current_display = wgl_set_current_display;
   vt->flip_display = wgl_flip_display;
   vt->update_display_region = wgl_update_display_region;
   vt->acknowledge_resize = wgl_acknowledge_resize;
   vt->create_bitmap = _al_ogl_create_bitmap;
   vt->create_sub_bitmap = _al_ogl_create_sub_bitmap;
   vt->get_backbuffer = _al_ogl_get_backbuffer;
   vt->get_frontbuffer = _al_ogl_get_backbuffer;
   vt->set_target_bitmap = _al_ogl_set_target_bitmap;
   vt->is_compatible_bitmap = wgl_is_compatible_bitmap;
   vt->switch_out = wgl_switch_in;
   vt->switch_in = wgl_switch_out;

   vt->create_mouse_cursor = _al_win_create_mouse_cursor;
   vt->destroy_mouse_cursor = _al_win_destroy_mouse_cursor;
   vt->set_mouse_cursor = _al_win_set_mouse_cursor;
   vt->set_system_mouse_cursor = _al_win_set_system_mouse_cursor;
   vt->show_mouse_cursor = _al_win_show_mouse_cursor;
   vt->hide_mouse_cursor = _al_win_hide_mouse_cursor;

   vt->set_icon = _al_win_set_display_icon;
   vt->set_window_position = wgl_set_window_position;
   vt->get_window_position = wgl_get_window_position;
   vt->toggle_frame = wgl_toggle_frame;
   vt->set_window_title = _al_win_set_window_title;
   _al_ogl_add_drawing_functions(vt);

   return vt;
}

int _al_wgl_get_num_display_modes(int format, int refresh_rate, int flags)
{
   /* FIXME: Ignoring format.
    * To get a list of pixel formats we have to create a window with a valid DC, which
    * would even require to change the video mode in fullscreen cases and we really
    * don't want to do that.
    */
   DEVMODE dm;   
   int count = 0;

   memset(&dm, 0, sizeof(dm));
   dm.dmSize = sizeof(dm);

   while (EnumDisplaySettings(NULL, count, &dm) != FALSE) {
      count++;
   }

   return count;
}


ALLEGRO_DISPLAY_MODE *_al_wgl_get_display_mode(int index, int format,
                                               int refresh_rate, int flags,
                                               ALLEGRO_DISPLAY_MODE *mode)
{
   /*
    * FIXME: see the comment in _al_wgl_get_num_display_modes
    */
   DEVMODE dm;   

   memset(&dm, 0, sizeof(dm));
   dm.dmSize = sizeof(dm);

   if (!EnumDisplaySettings(NULL, index, &dm))
      return NULL;

   mode->width = dm.dmPelsWidth;
   mode->height = dm.dmPanningHeight;
   mode->refresh_rate = dm.dmDisplayFrequency;

   return mode;
}


int _al_wgl_get_num_video_adapters(void)
{
   DISPLAY_DEVICE dd;
   int count = 0;
   int c = 0;

   memset(&dd, 0, sizeof(dd));
   dd.cb = sizeof(dd);

   while (EnumDisplayDevices(NULL, count, &dd, 0) != FALSE) {
      if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
         c++;
      count++;
   }

   return c;
}


void _al_wgl_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO *info)
{
   DISPLAY_DEVICE dd;
   DEVMODE dm;

   memset(&dd, 0, sizeof(dd));
   dd.cb = sizeof(dd);
   EnumDisplayDevices(NULL, adapter, &dd, 0);

   memset(&dm, 0, sizeof(dm));
   dm.dmSize = sizeof(dm);
   EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm);

   ASSERT(dm.dmFields & DM_PELSHEIGHT);
   ASSERT(dm.dmFields & DM_PELSWIDTH);
   ASSERT(dm.dmFields & DM_POSITION);

   info->x1 = dm.dmPosition.x;
   info->y1 = dm.dmPosition.y;
   info->x2 = info->x1 + dm.dmPelsWidth;
   info->y2 = info->y1 + dm.dmPelsHeight;
}

/* vi: set sts=3 sw=3 et: */

