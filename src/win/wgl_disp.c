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

#if 0
/* Raw input */
#define _WIN32_WINNT 0x0501
#ifndef WINVER
#define WINVER 0x0501
#endif
#endif

#include <windows.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/system.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_vector.h"
#include "allegro5/internal/aintern_wclipboard.h"
#include "allegro5/platform/aintwin.h"
#include "allegro5/internal/aintern_wunicode.h"

#include "wgl.h"

#include <process.h>

ALLEGRO_DEBUG_CHANNEL("display")

static ALLEGRO_DISPLAY_INTERFACE vt;

/* Forward declarations: */
static void display_thread_proc(void *arg);
static void destroy_display_internals(ALLEGRO_DISPLAY_WGL *wgl_disp);
static bool wgl_acknowledge_resize(ALLEGRO_DISPLAY *d);

/* Prevents switching to desktop resolution when destroying the display. Used
   on full screen resize. */
static bool _wgl_do_not_change_display_mode = false;

/*
 * These parameters cannot be gotten by the display thread because
 * they're thread local. We get them in the calling thread first.
 */
typedef struct WGL_DISPLAY_PARAMETERS {
   ALLEGRO_DISPLAY_WGL *display;
   volatile bool init_failed;
   HANDLE AckEvent;
   int window_x, window_y;
   /* Not owned. */
   const char* window_title;
} WGL_DISPLAY_PARAMETERS;

static bool is_wgl_extension_supported(_ALLEGRO_wglGetExtensionsStringARB_t _wglGetExtensionsStringARB, const char *extension, HDC dc)
{
   bool ret = false;
   const GLubyte* extensions;

   if (_wglGetExtensionsStringARB) {
      extensions = (const GLubyte*)_wglGetExtensionsStringARB(dc);
   }
   else {
      /* XXX deprecated in OpenGL 3.0 */
      extensions = glGetString(GL_EXTENSIONS);
   }

   if (extensions) {
      ret = _al_ogl_look_for_an_extension(extension, extensions);
   }

   return ret;
}


static HGLRC init_temp_context(HWND wnd)
{
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
      ALLEGRO_ERROR("Unable to chose a temporary pixel format. %s\n",
        _al_win_last_error());
      return NULL;
   }

   memset(&pfd, 0, sizeof(pfd));
   if (!SetPixelFormat(dc, pf, &pfd)) {
      ALLEGRO_ERROR("Unable to set a temporary pixel format. %s\n",
        _al_win_last_error());
      return NULL;
   }

   glrc = wglCreateContext(dc);
   if (!glrc) {
      ALLEGRO_ERROR("Unable to create a render context. %s\n",
        _al_win_last_error());
      return NULL;
   }

   if (!wglMakeCurrent(dc, glrc)) {
      ALLEGRO_ERROR("Unable to set the render context as current. %s\n",
        _al_win_last_error());
      wglDeleteContext(glrc);
      return NULL;
   }

   return glrc;
}


static _ALLEGRO_wglGetPixelFormatAttribivARB_t _wglGetPixelFormatAttribivARB = NULL;
static _ALLEGRO_wglGetPixelFormatAttribivEXT_t _wglGetPixelFormatAttribivEXT = NULL;

static bool init_pixel_format_extensions(void)
{
   /* Load the ARB_p_f symbol - Note, we shouldn't use the extension
    * mechanism here, because it hasn't been initialized yet!
    */
   _wglGetPixelFormatAttribivARB =
      (_ALLEGRO_wglGetPixelFormatAttribivARB_t)wglGetProcAddress("wglGetPixelFormatAttribivARB");
   _wglGetPixelFormatAttribivEXT =
      (_ALLEGRO_wglGetPixelFormatAttribivEXT_t)wglGetProcAddress("wglGetPixelFormatAttribivEXT");

   if (!_wglGetPixelFormatAttribivARB && !_wglGetPixelFormatAttribivEXT) {
      ALLEGRO_ERROR("WGL_ARB/EXT_pf not supported.\n");
      return false;
   }

   return true;
}


static _ALLEGRO_wglCreateContextAttribsARB_t _wglCreateContextAttribsARB = NULL;

static bool init_context_creation_extensions(void)
{
   _wglCreateContextAttribsARB =
      (_ALLEGRO_wglCreateContextAttribsARB_t)wglGetProcAddress("wglCreateContextAttribsARB");

   if (!_wglCreateContextAttribsARB) {
      ALLEGRO_ERROR("wglCreateContextAttribs not supported!\n");
      return false;
   }

   return true;
}


static int get_pixel_formats_count_old(HDC dc)
{
   PIXELFORMATDESCRIPTOR pfd;
   int ret;

   ret = DescribePixelFormat(dc, 1, sizeof(pfd), &pfd);
   if (!ret) {
      ALLEGRO_ERROR("DescribePixelFormat failed! %s\n",
                     _al_win_last_error());
   }

   return ret;
}


static int get_pixel_formats_count_ext(HDC dc)
{
   int attrib[1];
   int value[1];

   attrib[0] = WGL_NUMBER_PIXEL_FORMATS_ARB;
   if ((_wglGetPixelFormatAttribivARB(dc, 0, 0, 1, attrib, value) == GL_FALSE)
    && (_wglGetPixelFormatAttribivEXT(dc, 0, 0, 1, attrib, value) == GL_FALSE)) {
        ALLEGRO_ERROR("WGL_ARB/EXT_pixel_format use failed! %s\n",
                       _al_win_last_error());
   }

   return value[0];
}


static void display_pixel_format(ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds)
{
   ALLEGRO_INFO("Accelerated: %s\n", eds->settings[ALLEGRO_RENDER_METHOD] ? "yes" : "no");
   ALLEGRO_INFO("Single-buffer: %s\n", eds->settings[ALLEGRO_SINGLE_BUFFER] ? "yes" : "no");
   if (eds->settings[ALLEGRO_SWAP_METHOD] > 0)
      ALLEGRO_INFO("Swap method: %s\n", eds->settings[ALLEGRO_SWAP_METHOD] == 2 ? "flip" : "copy");
   else
      ALLEGRO_INFO("Swap method: undefined\n");
   ALLEGRO_INFO("Color format: r%i g%i b%i a%i, %i bit\n",
      eds->settings[ALLEGRO_RED_SIZE],
      eds->settings[ALLEGRO_GREEN_SIZE],
      eds->settings[ALLEGRO_BLUE_SIZE],
      eds->settings[ALLEGRO_ALPHA_SIZE],
      eds->settings[ALLEGRO_COLOR_SIZE]);
   ALLEGRO_INFO("Depth buffer: %i bits\n", eds->settings[ALLEGRO_DEPTH_SIZE]);
   ALLEGRO_INFO("Sample buffers: %s\n", eds->settings[ALLEGRO_SAMPLE_BUFFERS] ? "yes" : "no");
   ALLEGRO_INFO("Samples: %i\n", eds->settings[ALLEGRO_SAMPLES]);
}


static int decode_pixel_format_old(PIXELFORMATDESCRIPTOR *pfd,
                                   ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds)
{
   ALLEGRO_INFO("Decoding:\n");

	/* Not interested if it doesn't support OpenGL and RGBA */
   if (!(pfd->dwFlags & PFD_SUPPORT_OPENGL)) {
      ALLEGRO_INFO("OpenGL Unsupported\n");
      return false;
   }
   if (pfd->iPixelType != PFD_TYPE_RGBA) {
      ALLEGRO_INFO("Not RGBA mode\n");
      return false;
   }

   /* hardware acceleration */
   if (((pfd->dwFlags & PFD_GENERIC_ACCELERATED) && (pfd->dwFlags & PFD_GENERIC_FORMAT))
   || (!(pfd->dwFlags & PFD_GENERIC_ACCELERATED) && !(pfd->dwFlags & PFD_GENERIC_FORMAT)))
      eds->settings[ALLEGRO_RENDER_METHOD] = 1;
   else
      eds->settings[ALLEGRO_RENDER_METHOD] = 0;

   /* Depths of colour buffers */
   eds->settings[ALLEGRO_RED_SIZE] = pfd->cRedBits;
   eds->settings[ALLEGRO_GREEN_SIZE] = pfd->cGreenBits;
   eds->settings[ALLEGRO_BLUE_SIZE] = pfd->cBlueBits;
   eds->settings[ALLEGRO_ALPHA_SIZE] = pfd->cAlphaBits;

   /* Depths of accumulation buffer */
   eds->settings[ALLEGRO_ACC_RED_SIZE] = pfd->cAccumRedBits;
   eds->settings[ALLEGRO_ACC_GREEN_SIZE] = pfd->cAccumGreenBits;
   eds->settings[ALLEGRO_ACC_BLUE_SIZE] = pfd->cAccumBlueBits;
   eds->settings[ALLEGRO_ACC_ALPHA_SIZE] = pfd->cAccumAlphaBits;

   /* Miscellaneous settings */
   eds->settings[ALLEGRO_SINGLE_BUFFER] = !(pfd->dwFlags & PFD_DOUBLEBUFFER);
   eds->settings[ALLEGRO_DEPTH_SIZE] = pfd->cDepthBits;
   eds->settings[ALLEGRO_STENCIL_SIZE] = pfd->cStencilBits;
   eds->settings[ALLEGRO_COLOR_SIZE] = pfd->cColorBits;
   eds->settings[ALLEGRO_STEREO] = pfd->dwFlags & PFD_STEREO;
   eds->settings[ALLEGRO_AUX_BUFFERS] = pfd->cAuxBuffers;

   /* These are the component shifts. */
   eds->settings[ALLEGRO_RED_SHIFT] = pfd->cRedShift;
   eds->settings[ALLEGRO_GREEN_SHIFT] = pfd->cGreenShift;
   eds->settings[ALLEGRO_BLUE_SHIFT] = pfd->cBlueShift;
   eds->settings[ALLEGRO_ALPHA_SHIFT] = pfd->cAlphaShift;

   /* Multisampling isn't supported under Windows if we don't also use
    * WGL_ARB_pixel_format or WGL_EXT_pixel_format.
    */
   eds->settings[ALLEGRO_SAMPLE_BUFFERS] = 0;
   eds->settings[ALLEGRO_SAMPLES] = 0;

   /* Swap method can't be detected without WGL_ARB_pixel_format or
    * WGL_EXT_pixel_format
    */
   eds->settings[ALLEGRO_SWAP_METHOD] = 0;

   /* Float depth/color isn't supported under Windows if we don't also use
    * AGL_ARB_pixel_format or WGL_EXT_pixel_format.
    */
   eds->settings[ALLEGRO_FLOAT_COLOR] = 0;
   eds->settings[ALLEGRO_FLOAT_DEPTH] = 0;

   // FIXME

   eds->settings[ALLEGRO_COMPATIBLE_DISPLAY] = 1;

   return true;
}


static bool decode_pixel_format_attrib(ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds, int num_attribs,
                                      const int *attrib, const int *value)
{
   int i;

   ALLEGRO_INFO("Decoding:\n");

   eds->settings[ALLEGRO_SAMPLES] = 0;
   eds->settings[ALLEGRO_SAMPLE_BUFFERS] = 0;
   eds->settings[ALLEGRO_FLOAT_DEPTH] = 0;
   eds->settings[ALLEGRO_FLOAT_COLOR] = 0;
   eds->settings[ALLEGRO_COMPATIBLE_DISPLAY] = 1;

   for (i = 0; i < num_attribs; i++) {
      /* Not interested if it doesn't support OpenGL or window drawing or RGBA. */
      if (attrib[i] == WGL_SUPPORT_OPENGL_ARB && value[i] == 0) {
         ALLEGRO_INFO("OpenGL Unsupported\n");
         return false;
      }
      else if (attrib[i] == WGL_DRAW_TO_WINDOW_ARB && value[i] == 0) {
         ALLEGRO_INFO("Can't draw to window\n");
         return false;
      }
      else if (attrib[i] == WGL_PIXEL_TYPE_ARB
         && (value[i] != WGL_TYPE_RGBA_ARB && value[i] != WGL_TYPE_RGBA_FLOAT_ARB)) {
         ALLEGRO_INFO("Not RGBA mode\n");
         return false;
      }
      /* hardware acceleration */
      else if (attrib[i] == WGL_ACCELERATION_ARB) {
         eds->settings[ALLEGRO_RENDER_METHOD] = (value[i] == WGL_NO_ACCELERATION_ARB) ? 0 : 1;
      }
      /* Depths of colour buffers */
      else if (attrib[i] == WGL_RED_BITS_ARB) {
         eds->settings[ALLEGRO_RED_SIZE] = value[i];
      }
      else if (attrib[i] == WGL_GREEN_BITS_ARB) {
         eds->settings[ALLEGRO_GREEN_SIZE] = value[i];
      }
      else if (attrib[i] == WGL_BLUE_BITS_ARB) {
         eds->settings[ALLEGRO_BLUE_SIZE] = value[i];
      }
      else if (attrib[i] == WGL_ALPHA_BITS_ARB) {
         eds->settings[ALLEGRO_ALPHA_SIZE] = value[i];
      }
      /* Shift of color components */
      else if (attrib[i] == WGL_RED_SHIFT_ARB) {
         eds->settings[ALLEGRO_RED_SHIFT] = value[i];
      }
      else if (attrib[i] == WGL_GREEN_SHIFT_ARB) {
         eds->settings[ALLEGRO_GREEN_SHIFT] = value[i];
      }
      else if (attrib[i] == WGL_BLUE_SHIFT_ARB) {
         eds->settings[ALLEGRO_BLUE_SHIFT] = value[i];
      }
      else if (attrib[i] == WGL_ALPHA_SHIFT_ARB) {
         eds->settings[ALLEGRO_ALPHA_SHIFT] = value[i];
      }
      /* Miscellaneous settings */
      else if (attrib[i] == WGL_DOUBLE_BUFFER_ARB) {
         eds->settings[ALLEGRO_SINGLE_BUFFER] = !(value[i]);
      }
      else if (attrib[i] == WGL_SWAP_METHOD_ARB) {
         if (value[i] == WGL_SWAP_UNDEFINED_ARB)
            eds->settings[ALLEGRO_SWAP_METHOD] = 0;
         else if (value[i] == WGL_SWAP_COPY_ARB)
            eds->settings[ALLEGRO_SWAP_METHOD] = 1;
         else if (value[i] == WGL_SWAP_EXCHANGE_ARB)
            eds->settings[ALLEGRO_SWAP_METHOD] = 2;
      }

      else if (attrib[i] == WGL_STEREO_ARB) {
         eds->settings[ALLEGRO_STEREO] = value[i];
      }
      else if (attrib[i] == WGL_AUX_BUFFERS_ARB) {
         eds->settings[ALLEGRO_AUX_BUFFERS] = value[i];
      }
      else if (attrib[i] == WGL_STENCIL_BITS_ARB) {
         eds->settings[ALLEGRO_STENCIL_SIZE] = value[i];
      }
      /* Depths of accumulation buffer */
      else if (attrib[i] == WGL_ACCUM_RED_BITS_ARB) {
         eds->settings[ALLEGRO_ACC_RED_SIZE] = value[i];
      }
      else if (attrib[i] == WGL_ACCUM_GREEN_BITS_ARB) {
         eds->settings[ALLEGRO_ACC_GREEN_SIZE] = value[i];
      }
      else if (attrib[i] == WGL_ACCUM_BLUE_BITS_ARB) {
         eds->settings[ALLEGRO_ACC_BLUE_SIZE] = value[i];
      }
      else if (attrib[i] == WGL_ACCUM_ALPHA_BITS_ARB) {
         eds->settings[ALLEGRO_ACC_ALPHA_SIZE] = value[i];
      }

      else if (attrib[i] == WGL_DEPTH_BITS_ARB) {
         eds->settings[ALLEGRO_DEPTH_SIZE] = value[i];
      }
      else if (attrib[i] == WGL_COLOR_BITS_ARB) {
         eds->settings[ALLEGRO_COLOR_SIZE] = value[i];
      }
      /* Multisampling bits */
      else if (attrib[i] == WGL_SAMPLE_BUFFERS_ARB) {
         eds->settings[ALLEGRO_SAMPLE_BUFFERS] = value[i];
      }
      else if (attrib[i] == WGL_SAMPLES_ARB) {
         eds->settings[ALLEGRO_SAMPLES] = value[i];
      }
      /* Float color */
      if (attrib[i] == WGL_PIXEL_TYPE_ARB && value[i] == WGL_TYPE_RGBA_FLOAT_ARB) {
         eds->settings[ALLEGRO_FLOAT_COLOR] = true;
      }
      /* Float depth */
      else if (attrib[i] == WGL_DEPTH_FLOAT_EXT) {
         eds->settings[ALLEGRO_FLOAT_DEPTH] = value[i];
      }
   }

   return true;
}


static ALLEGRO_EXTRA_DISPLAY_SETTINGS* read_pixel_format_old(int fmt, HDC dc)
{
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds = NULL;
   PIXELFORMATDESCRIPTOR pfd;
   int result;

   result = DescribePixelFormat(dc, fmt+1, sizeof(pfd), &pfd);
   if (!result) {
      ALLEGRO_WARN("DescribePixelFormat() failed. %s\n", _al_win_last_error());
      return NULL;
   }

   eds = al_calloc(1, sizeof *eds);
   if (!decode_pixel_format_old(&pfd, eds)) {
      al_free(eds);
      return NULL;
   }

   return eds;
}


static ALLEGRO_EXTRA_DISPLAY_SETTINGS* read_pixel_format_ext(_ALLEGRO_wglGetExtensionsStringARB_t _wglGetExtensionsStringARB, int fmt, HDC dc)
{
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds = NULL;

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
   int *value = (int*)al_malloc(sizeof(int) * num_attribs);
   int ret;

   if (!value)
      return NULL;

   /* If multisampling is supported, query for it. */
   if (is_wgl_extension_supported(_wglGetExtensionsStringARB, "WGL_ARB_multisample", dc)) {
      attrib[num_attribs - 3] = WGL_SAMPLE_BUFFERS_ARB;
      attrib[num_attribs - 2] = WGL_SAMPLES_ARB;
   }
   if (is_wgl_extension_supported(_wglGetExtensionsStringARB, "WGL_EXT_depth_float", dc)) {
      attrib[num_attribs - 1] = WGL_DEPTH_FLOAT_EXT;
   }

   /* Get the pf attributes */
   if (_wglGetPixelFormatAttribivARB) {
      ret = _wglGetPixelFormatAttribivARB(dc, fmt+1, 0, num_attribs, attrib, value);
   }
   else if (_wglGetPixelFormatAttribivEXT) {
      ret = _wglGetPixelFormatAttribivEXT(dc, fmt+1, 0, num_attribs, attrib, value);
   }
   else {
      ret = 0;
   }

   if (!ret) {
      ALLEGRO_ERROR("wglGetPixelFormatAttrib failed! %s\n", _al_win_last_error());
      al_free(value);
      return NULL;
   }

   eds = al_calloc(1, sizeof *eds);
   if (!decode_pixel_format_attrib(eds, num_attribs, attrib, value)) {
      al_free(eds);
      eds = NULL;
   }

   al_free(value);

   /* Hack: for some reason this happens for me under Wine. */
   if (eds &&
      eds->settings[ALLEGRO_RED_SHIFT] == 0 &&
         eds->settings[ALLEGRO_GREEN_SHIFT] == 0 &&
         eds->settings[ALLEGRO_BLUE_SHIFT] == 0 &&
         eds->settings[ALLEGRO_ALPHA_SHIFT] == 0) {
      eds->settings[ALLEGRO_RED_SHIFT] = 0;
      eds->settings[ALLEGRO_GREEN_SHIFT] = 8;
      eds->settings[ALLEGRO_BLUE_SHIFT] = 16;
      eds->settings[ALLEGRO_ALPHA_SHIFT] = 24;
   }

   return eds;
}


static bool change_display_mode(ALLEGRO_DISPLAY *d)
{
   DEVMODE dm;
   DEVMODE fallback_dm;
   DISPLAY_DEVICE dd;
   TCHAR* dev_name = NULL;
   int i, modeswitch, result;
   int fallback_dm_valid = 0;
   int bpp;
   int adapter = al_get_new_display_adapter();

   if (adapter >= 0) {
      memset(&dd, 0, sizeof(dd));
      dd.cb = sizeof(dd);
      if (EnumDisplayDevices(NULL, adapter, &dd, 0) == false)
         return false;
      dev_name = dd.DeviceName;
   }

   memset(&fallback_dm, 0, sizeof(fallback_dm));
   memset(&dm, 0, sizeof(dm));
   dm.dmSize = sizeof(DEVMODE);

   bpp = d->extra_settings.settings[ALLEGRO_COLOR_SIZE];
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
         && (dm.dmDisplayFrequency != (unsigned) d->refresh_rate))
      {
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
      ALLEGRO_ERROR("Mode not found.\n");
      return false;
   }

   if (!modeswitch && fallback_dm_valid)
      dm = fallback_dm;

   dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;
   result = ChangeDisplaySettingsEx(dev_name, &dm, NULL, CDS_FULLSCREEN, 0);

   d->refresh_rate = dm.dmDisplayFrequency;

   if (result != DISP_CHANGE_SUCCESSFUL) {
      ALLEGRO_ERROR("Unable to set mode. %s\n", _al_win_last_error());
      return false;
   }

   ALLEGRO_INFO("Mode seccessfuly set.\n");
   return true;
}


static HGLRC init_ogl_context_ex(HDC dc, bool fc, int major, int minor)
{
   HWND testwnd = NULL;
   HDC testdc   = NULL;
   HGLRC testrc = NULL;
   HGLRC old_rc = NULL;
   HDC old_dc   = NULL;
   HGLRC glrc   = NULL;

   testwnd = _al_win_create_hidden_window();
   if (!testwnd)
      return NULL;

   old_rc = wglGetCurrentContext();
   old_dc = wglGetCurrentDC();

   testdc = GetDC(testwnd);
   testrc = init_temp_context(testwnd);
   if (!testrc)
      goto bail;

   _ALLEGRO_wglGetExtensionsStringARB_t _wglGetExtensionsStringARB
      = (_ALLEGRO_wglGetExtensionsStringARB_t)wglGetProcAddress("wglGetExtensionsStringARB");
   ALLEGRO_INFO("_wglGetExtensionsStringARB %p\n", _wglGetExtensionsStringARB);

   if (is_wgl_extension_supported(_wglGetExtensionsStringARB, "WGL_ARB_create_context", testdc)) {
      int attrib[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, major,
                      WGL_CONTEXT_MINOR_VERSION_ARB, minor,
                      WGL_CONTEXT_FLAGS_ARB, 0,
                      0};
      if (fc)
         attrib[5] = WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
      if (!init_context_creation_extensions())
         goto bail;
      /* TODO: we could use the context sharing feature */
      glrc = _wglCreateContextAttribsARB(dc, 0, attrib);
   }
   else
      goto bail;

bail:
   wglMakeCurrent(NULL, NULL);
   if (testrc) {
      wglDeleteContext(testrc);
   }

   wglMakeCurrent(old_dc, old_rc);

   _wglCreateContextAttribsARB = NULL;

   if (testwnd) {
      ReleaseDC(testwnd, testdc);
      DestroyWindow(testwnd);
   }

   return glrc;
}


static ALLEGRO_EXTRA_DISPLAY_SETTINGS** get_available_pixel_formats_ext(int *count)
{
   HWND testwnd = NULL;
   HDC testdc   = NULL;
   HGLRC testrc = NULL;
   HGLRC old_rc = NULL;
   HDC old_dc   = NULL;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS **eds_list = NULL;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref;
   int maxindex;
   int i, j;

   *count = 0;
   ref =  _al_get_new_display_settings();

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

   _ALLEGRO_wglGetExtensionsStringARB_t _wglGetExtensionsStringARB
      = (_ALLEGRO_wglGetExtensionsStringARB_t)wglGetProcAddress("wglGetExtensionsStringARB");
   ALLEGRO_INFO("_wglGetExtensionsStringARB %p\n", _wglGetExtensionsStringARB);

   if (!is_wgl_extension_supported(_wglGetExtensionsStringARB, "WGL_ARB_pixel_format", testdc) &&
       !is_wgl_extension_supported(_wglGetExtensionsStringARB, "WGL_EXT_pixel_format", testdc)) {
      ALLEGRO_ERROR("WGL_ARB/EXT_pf not supported.\n");
      goto bail;
   }

   if (!init_pixel_format_extensions())
      goto bail;

   maxindex = get_pixel_formats_count_ext(testdc);
   if (maxindex < 1)
      goto bail;

   ALLEGRO_INFO("Got %i visuals.\n", maxindex);

   eds_list = al_calloc(maxindex, sizeof(*eds_list));
   if (!eds_list)
      goto bail;

   for (j = i = 0; i < maxindex; i++) {
      ALLEGRO_INFO("-- \n");
      ALLEGRO_INFO("Decoding visual no. %i...\n", i+1);
      eds_list[j] = read_pixel_format_ext(_wglGetExtensionsStringARB, i, testdc);
      if (!eds_list[j])
         continue;
      // Fill vsync setting here and enable/disable it after display creation
      eds_list[j]->settings[ALLEGRO_VSYNC] = ref->settings[ALLEGRO_VSYNC];
      display_pixel_format(eds_list[j]);
      eds_list[j]->score = _al_score_display_settings(eds_list[j], ref);
      if (eds_list[j]->score == -1) {
         al_free(eds_list[j]);
         eds_list[j] = NULL;
         continue;
      }
      /* In WinAPI first index is 1 ::) */
      eds_list[j]->index = i+1;
      j++;
   }

   ALLEGRO_INFO("%i visuals are good enough.\n", j);
   *count = j;

bail:
   wglMakeCurrent(NULL, NULL);
   if (testrc) {
      wglDeleteContext(testrc);
   }

   wglMakeCurrent(old_dc, old_rc);

   _wglGetPixelFormatAttribivARB = NULL;
   _wglGetPixelFormatAttribivEXT = NULL;

   if (testwnd) {
      ReleaseDC(testwnd, testdc);
      DestroyWindow(testwnd);
   }

   return eds_list;
}


static ALLEGRO_EXTRA_DISPLAY_SETTINGS** get_available_pixel_formats_old(int *count, HDC dc)
{
   ALLEGRO_EXTRA_DISPLAY_SETTINGS **eds_list = NULL;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref;
   int maxindex;
   int i, j;

   *count = 0;
   ref =  _al_get_new_display_settings();

   maxindex = get_pixel_formats_count_old(dc);
   if (maxindex < 1)
      return NULL;

   ALLEGRO_INFO("Got %i visuals.\n", maxindex);

   eds_list = al_calloc(maxindex, sizeof(*eds_list));
   if (!eds_list)
      return NULL;

   for (j = i = 0; i < maxindex; i++) {
      ALLEGRO_INFO("-- \n");
      ALLEGRO_INFO("Decoding visual no. %i...\n", i+1);
      eds_list[j] = read_pixel_format_old(i, dc);
      if (!eds_list[j])
         continue;
#ifdef DEBUGMODE
      display_pixel_format(eds_list[j]);
#endif
      eds_list[j]->score = _al_score_display_settings(eds_list[j], ref);
      if (eds_list[j]->score == -1) {
         al_free(eds_list[j]);
         eds_list[j] = NULL;
         continue;
      }
      /* In WinAPI first index is 1 ::) */
      eds_list[j]->index = i+1;
      j++;
   }

   ALLEGRO_INFO("%i visuals are good enough.\n", j);
   *count = j;

   return eds_list;
}


static bool select_pixel_format(ALLEGRO_DISPLAY_WGL *d, HDC dc)
{
   ALLEGRO_EXTRA_DISPLAY_SETTINGS **eds = NULL;
   ALLEGRO_CONFIG *sys_cfg = al_get_system_config();
   int eds_count = 0;
   int i;
   bool force_old = false;
   const char *selection_mode;

   selection_mode = al_get_config_value(sys_cfg, "graphics",
                       "config_selection");
   if (selection_mode && selection_mode[0] != '\0') {
      if (!_al_stricmp(selection_mode, "old")) {
         ALLEGRO_INFO("Forcing OLD visual selection method.\n");
         force_old = true;
      }
      else if (!_al_stricmp(selection_mode, "new"))
         force_old = false;
   }

   if (!force_old)
      eds = get_available_pixel_formats_ext(&eds_count);
   if (!eds)
      eds = get_available_pixel_formats_old(&eds_count, dc);

   if (!eds || !eds_count) {
      ALLEGRO_ERROR("Didn't find any suitable pixel format!\n");
      return false;
   }

   qsort(eds, eds_count, sizeof(eds[0]), _al_display_settings_sorter);

   for (i = 0; i < eds_count ; i++) {
      if (SetPixelFormat(d->dc, eds[i]->index, NULL)) {
         ALLEGRO_INFO("Chose visual no. %i\n\n", eds[i]->index);
         display_pixel_format(eds[i]);
         break;
      }
      else {
         ALLEGRO_WARN("Unable to set pixel format! %s\n", _al_win_last_error());
         ALLEGRO_WARN("Trying next one.\n");
      }
   }

   if (i == eds_count) {
      ALLEGRO_ERROR("Unable to set any pixel format! %s\n", _al_win_last_error());
      for (i = 0; i < eds_count; i++)
         al_free(eds[i]);
      al_free(eds);
      return false;
   }

   memcpy(&d->win_display.display.extra_settings, eds[i], sizeof(ALLEGRO_EXTRA_DISPLAY_SETTINGS));

   for (i = 0; i < eds_count; i++)
      al_free(eds[i]);
   if (eds)
      al_free(eds);

   return true;
}

static bool create_display_internals(ALLEGRO_DISPLAY_WGL *wgl_disp)
{
   ALLEGRO_DISPLAY     *disp     = (void*)wgl_disp;
   ALLEGRO_DISPLAY_WIN *win_disp = (void*)wgl_disp;
   WGL_DISPLAY_PARAMETERS ndp;
   int window_x, window_y;
   int major, minor;

   /* The window is created in a separate thread so we need to pass this
    * TLS on
    */
   al_get_new_window_position(&window_x, &window_y);
   ndp.window_x = window_x;
   ndp.window_y = window_y;
   ndp.window_title = al_get_new_window_title();

   /* _beginthread closes the handle automatically. */
   ndp.display = wgl_disp;
   ndp.init_failed = true;
   ndp.AckEvent = CreateEvent(NULL, false, false, NULL);
   _beginthread(display_thread_proc, 0, &ndp);

   /* Wait some _finite_ time (10 secs or so) for display thread to init, and
    * give up if something horrible happened to it, unless we're in debug mode
    * and we may have intentionally stopped the execution to analyze the code.
    */
#ifdef DEBUGMODE
   WaitForSingleObject(ndp.AckEvent, INFINITE);
#else
   WaitForSingleObject(ndp.AckEvent, 10*1000);
#endif

   CloseHandle(ndp.AckEvent);

   if (ndp.init_failed) {
      ALLEGRO_ERROR("Failed to create display.\n");
      return false;
   }

   /* WGL display lists cannot be shared with the API currently in use. */
   disp->ogl_extras->is_shared = false;

   if (!select_pixel_format(wgl_disp, wgl_disp->dc)) {
      destroy_display_internals(wgl_disp);
      return false;
   }

   major = al_get_new_display_option(ALLEGRO_OPENGL_MAJOR_VERSION, 0);
   minor = al_get_new_display_option(ALLEGRO_OPENGL_MINOR_VERSION, 0);

   // TODO: request GLES context in GLES builds
   if ((disp->flags & ALLEGRO_OPENGL_3_0) || major != 0) {
      if (major == 0)
         major = 3;
      bool fc = (disp->flags & ALLEGRO_OPENGL_FORWARD_COMPATIBLE) != 0;
      wgl_disp->glrc = init_ogl_context_ex(wgl_disp->dc, fc, major,
         minor);
   }
   else {
      wgl_disp->glrc = wglCreateContext(wgl_disp->dc);
   }

   if (!wgl_disp->glrc) {
      ALLEGRO_ERROR("Unable to create a render context! %s\n", _al_win_last_error());
      destroy_display_internals(wgl_disp);
      return false;
   }

   /* make the context the current one */
   if (!wglMakeCurrent(wgl_disp->dc, wgl_disp->glrc)) {
      ALLEGRO_ERROR("Unable to make the context current! %s\n", _al_win_last_error());
      destroy_display_internals(wgl_disp);
      return false;
   }

   _al_ogl_manage_extensions(disp);
   _al_ogl_set_extensions(disp->ogl_extras->extension_api);

   if (disp->ogl_extras->ogl_info.version < _ALLEGRO_OPENGL_VERSION_1_2) {
      ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds = _al_get_new_display_settings();
      if (eds->required & (1<<ALLEGRO_COMPATIBLE_DISPLAY)) {
         ALLEGRO_WARN("Allegro requires at least OpenGL version 1.2 to work.\n");
         destroy_display_internals(wgl_disp);
         return false;
      }
      disp->extra_settings.settings[ALLEGRO_COMPATIBLE_DISPLAY] = 0;
   }

   /* Fill in the display settings for opengl major and minor versions...*/
   const int v = disp->ogl_extras->ogl_info.version;
   disp->extra_settings.settings[ALLEGRO_OPENGL_MAJOR_VERSION] = (v >> 24) & 0xFF;
   disp->extra_settings.settings[ALLEGRO_OPENGL_MINOR_VERSION] = (v >> 16) & 0xFF;

   /* Try to enable or disable vsync as requested */
   /* NOTE: my drivers claim I don't have WGL_EXT_swap_control
    * (according to al_have_opengl_extension), but wglSwapIntervalEXT
    * does get loaded, so just check for that.
    */
   if (wglSwapIntervalEXT) {
      if (disp->extra_settings.settings[ALLEGRO_VSYNC] == 1) {
         wglSwapIntervalEXT(1);
      }
      else if (disp->extra_settings.settings[ALLEGRO_VSYNC] == 2) {
         wglSwapIntervalEXT(0);
      }
   }

   win_disp->mouse_selected_hcursor = 0;
   win_disp->mouse_cursor_shown = false;
   win_disp->can_acknowledge = false;

   _al_win_grab_input(win_disp);

   if (disp->extra_settings.settings[ALLEGRO_COMPATIBLE_DISPLAY])
      _al_ogl_setup_gl(disp);

   return true;
}


static ALLEGRO_DISPLAY* wgl_create_display(int w, int h)
{
   ALLEGRO_SYSTEM_WIN *system = (ALLEGRO_SYSTEM_WIN *)al_get_system_driver();
   ALLEGRO_DISPLAY_WGL **add;
   ALLEGRO_DISPLAY_WGL *wgl_display = al_calloc(1, sizeof *wgl_display);
   ALLEGRO_DISPLAY *ogl_display = (void*)wgl_display;
   ALLEGRO_DISPLAY     *display     = (void*)ogl_display;
   ALLEGRO_DISPLAY_WIN *win_disp = (ALLEGRO_DISPLAY_WIN *)display;

   win_disp->adapter = _al_win_determine_adapter();

   display->w = w;
   display->h = h;
   display->refresh_rate = al_get_new_display_refresh_rate();
   display->flags = al_get_new_display_flags();
#ifdef ALLEGRO_CFG_OPENGLES2
   display->flags |= ALLEGRO_PROGRAMMABLE_PIPELINE;
#endif
#ifdef ALLEGRO_CFG_OPENGLES
   display->flags |= ALLEGRO_OPENGL_ES_PROFILE;
#endif
   display->vt = &vt;

   display->ogl_extras = al_calloc(1, sizeof(ALLEGRO_OGL_EXTRAS));

   if (!create_display_internals(wgl_display)) {
      al_free(display->ogl_extras);
      al_free(display);
      return NULL;
   }

   /* Print out OpenGL version info */
   ALLEGRO_INFO("OpenGL Version: %s\n", (const char*)glGetString(GL_VERSION));
   ALLEGRO_INFO("Vendor: %s\n", (const char*)glGetString(GL_VENDOR));
   ALLEGRO_INFO("Renderer: %s\n\n", (const char*)glGetString(GL_RENDERER));

   /* Add ourself to the list of displays. */
   add = _al_vector_alloc_back(&system->system.displays);
   *add = wgl_display;

   /* Each display is an event source. */
   _al_event_source_init(&display->es);

   _al_win_set_system_mouse_cursor(display, ALLEGRO_SYSTEM_MOUSE_CURSOR_ARROW);
   _al_win_show_mouse_cursor(display);

   _al_win_post_create_window(display);

   return display;
}


static void destroy_display_internals(ALLEGRO_DISPLAY_WGL *wgl_disp)
{
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

   PostMessage(win_disp->window, _al_win_msg_suicide, (WPARAM)win_disp, 0);

   while (!win_disp->thread_ended)
      al_rest(0.001);

   if (wgl_disp->glrc) {
      wglDeleteContext(wgl_disp->glrc);
      wgl_disp->glrc = NULL;
   }
   if (wgl_disp->dc) {
      ReleaseDC(win_disp->window, wgl_disp->dc);
      wgl_disp->dc = NULL;
   }

   if (disp->flags & ALLEGRO_FULLSCREEN && !_wgl_do_not_change_display_mode) {
      ChangeDisplaySettings(NULL, 0);
   }

   if (win_disp->window) {
      DestroyWindow(win_disp->window);
      win_disp->window = NULL;
   }
}


static void wgl_destroy_display(ALLEGRO_DISPLAY *disp)
{
   ALLEGRO_SYSTEM_WIN *system = (ALLEGRO_SYSTEM_WIN *)al_get_system_driver();
   ALLEGRO_DISPLAY_WGL *wgl_disp = (ALLEGRO_DISPLAY_WGL *)disp;
   ALLEGRO_DISPLAY *old_disp = al_get_current_display();

   if (old_disp != disp)
      _al_set_current_display_only(disp);

   if (system->mouse_grab_display == disp)
      system->mouse_grab_display = NULL;

   _al_win_destroy_display_icons(disp);

   destroy_display_internals(wgl_disp);
   _al_event_source_free(&disp->es);
   _al_vector_find_and_delete(&system->system.displays, &disp);

   _al_vector_free(&disp->bitmaps);
   _al_vector_free(&((ALLEGRO_DISPLAY_WIN*) disp)->msg_callbacks);
   al_free(disp->ogl_extras);

   if (old_disp != disp)
      _al_set_current_display_only(old_disp);

   al_free(disp->vertex_cache);
   al_free(wgl_disp);
}


static bool wgl_set_current_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_WGL *wgl_disp = (ALLEGRO_DISPLAY_WGL *)d;
   HGLRC current_glrc;

   current_glrc = wglGetCurrentContext();

   if (!current_glrc || (current_glrc && current_glrc != wgl_disp->glrc)) {
      /* make the context the current one */
      if (!wglMakeCurrent(wgl_disp->dc, wgl_disp->glrc)) {
         ALLEGRO_ERROR("Unable to make the context current! %s\n",
                        _al_win_last_error());
         return false;
      }

      _al_ogl_set_extensions(d->ogl_extras->extension_api);
   }

   _al_ogl_update_render_state(d);

   return true;
}


static void wgl_unset_current_display(ALLEGRO_DISPLAY *d)
{
   (void)d;

   if (!wglMakeCurrent(NULL, NULL)) {
      ALLEGRO_ERROR("Unable unset the current context! %s\n", _al_win_last_error());
   }
}

/*
 * The window must be created in the same thread that
 * runs the message loop.
 */
static void display_thread_proc(void *arg)
{
   WGL_DISPLAY_PARAMETERS *ndp = arg;
   ALLEGRO_DISPLAY *disp = (ALLEGRO_DISPLAY*)ndp->display;
   ALLEGRO_DISPLAY_WGL *wgl_disp = (void*)disp;
   ALLEGRO_DISPLAY_WIN *win_disp = (void*)disp;
   MSG msg;

   /* So that we can call the functions using TLS from this thread. */
   al_set_new_display_flags(disp->flags);
   al_set_new_window_position(ndp->window_x, ndp->window_y);
   al_set_new_window_title(ndp->window_title);

   if (disp->flags & ALLEGRO_FULLSCREEN) {
      if (!change_display_mode(disp)) {
         win_disp->thread_ended = true;
         destroy_display_internals(wgl_disp);
         SetEvent(ndp->AckEvent);
         return;
      }
   }
   else if (disp->flags & ALLEGRO_FULLSCREEN_WINDOW) {
      ALLEGRO_MONITOR_INFO mi;
      int adapter = win_disp->adapter;
      al_get_monitor_info(adapter, &mi);

      win_disp->toggle_w = disp->w;
      win_disp->toggle_h = disp->h;

      disp->w = mi.x2 - mi.x1;
      disp->h = mi.y2 - mi.y1;

      disp->refresh_rate = al_get_monitor_refresh_rate(adapter);
   }
   else {
      int adapter = win_disp->adapter;
      win_disp->toggle_w = disp->w;
      win_disp->toggle_h = disp->h;

      disp->refresh_rate = al_get_monitor_refresh_rate(adapter);
   }

   win_disp->window = _al_win_create_window(disp, disp->w, disp->h, disp->flags);

   if (!win_disp->window) {
      win_disp->thread_ended = true;
      destroy_display_internals(wgl_disp);
      SetEvent(ndp->AckEvent);
      return;
   }

   /* FIXME: can't _al_win_create_window() do this? */
   if ((disp->flags & ALLEGRO_FULLSCREEN) ||
         (disp->flags & ALLEGRO_FULLSCREEN_WINDOW)) {
      RECT rect;
      rect.left = 0;
      rect.right = disp->w;
      rect.top  = 0;
      rect.bottom = disp->h;
      SetWindowPos(win_disp->window, 0, rect.left, rect.top,
             rect.right - rect.left, rect.bottom - rect.top,
             SWP_NOZORDER | SWP_FRAMECHANGED);
   }

   if (disp->flags & ALLEGRO_FULLSCREEN_WINDOW) {
      _al_win_set_window_frameless(disp, win_disp->window, true);
   }

   {
      DWORD lock_time;
      HWND wnd = win_disp->window;

      if ((disp->flags & ALLEGRO_FULLSCREEN) == 0) {
         ShowWindow(wnd, SW_SHOWNORMAL);
         SetForegroundWindow(wnd);
         UpdateWindow(wnd);
      }
      else {

         /* Yep, the following is really needed sometimes. */
         /* ... Or is it now that we have dumped DInput? */
         /* <rohannessian> Win98/2k/XP's window foreground rules don't let us
          * make our window the topmost window on launch. This causes issues on
          * full-screen apps, as DInput loses input focus on them.
          * We use this trick to force the window to be topmost, when switching
          * to full-screen only. Note that this only works for Win98 and greater.
          * Win95 will ignore our SystemParametersInfo() calls.
          *
          * See http://support.microsoft.com:80/support/kb/articles/Q97/9/25.asp
          * for details.
          */

#define SPI_GETFOREGROUNDLOCKTIMEOUT 0x2000
#define SPI_SETFOREGROUNDLOCKTIMEOUT 0x2001
         SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT,
               0, (LPVOID)&lock_time, 0);
         SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT,
               0, (LPVOID)0, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);

         ShowWindow(wnd, SW_SHOWNORMAL);
         SetForegroundWindow(wnd);
         /* In some rare cases, it doesn't seem to work without the loop. And we
          * absolutely need this to succeed, else we trap the user in a
          * fullscreen window without input.
          */
         while (GetForegroundWindow() != wnd) {
            al_rest(0.1);
            SetForegroundWindow(wnd);
         }
         UpdateWindow(wnd);

         SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT,
              0, (LPVOID)&lock_time, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
#undef SPI_GETFOREGROUNDLOCKTIMEOUT
#undef SPI_SETFOREGROUNDLOCKTIMEOUT
      }
   }

#if 0
   if (disp->flags & ALLEGRO_FULLSCREEN && al_is_mouse_installed()) {
      RAWINPUTDEVICE rid[1];
      rid[0].usUsagePage = 0x01;
      rid[0].usUsage = 0x02;
      rid[0].dwFlags = RIDEV_NOLEGACY;
      rid[0].hwndTarget = 0;
      if (RegisterRawInputDevices(rid, 1, sizeof(rid[0])) == FALSE) {
          ALLEGRO_ERROR(
             "Failed to init mouse. %s\n", get_error_desc(GetLastError()));
      }
   }
#endif

   /* get the device context of our window */
   wgl_disp->dc = GetDC(win_disp->window);

   win_disp->thread_ended = false;
   win_disp->end_thread = false;
   ndp->init_failed = false;
   SetEvent(ndp->AckEvent);

   while (!win_disp->end_thread) {
      /* get a message from the queue */
      if (GetMessage(&msg, NULL, 0, 0) != 0)
         DispatchMessage(&msg);
      else
         break;                 /* WM_QUIT received or error (GetMessage returned -1)  */
   }

   ALLEGRO_INFO("wgl display thread exits\n");
   win_disp->thread_ended = true;
}


static void wgl_flip_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_WGL* disp = (ALLEGRO_DISPLAY_WGL*)d;
   glFlush();
   if (!d->extra_settings.settings[ALLEGRO_SINGLE_BUFFER])
      SwapBuffers(disp->dc);
}


static void wgl_update_display_region(ALLEGRO_DISPLAY *d,
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
      return;
   }
   wgl_flip_display(d);
}


static bool wgl_resize_helper(ALLEGRO_DISPLAY *d, int width, int height)
{
   ALLEGRO_DISPLAY_WGL *wgl_disp = (ALLEGRO_DISPLAY_WGL *)d;
   ALLEGRO_DISPLAY_WIN *win_disp = (ALLEGRO_DISPLAY_WIN *)d;
   int full_w, full_h;
   ALLEGRO_MONITOR_INFO mi;
   int adapter = al_get_new_display_adapter();
   if (adapter < 0)
      adapter = 0;
   al_get_monitor_info(adapter, &mi);
   full_w = mi.x2 - mi.x1;
   full_h = mi.y2 - mi.y1;

   if ((d->flags & ALLEGRO_FULLSCREEN_WINDOW) && (full_w != width || full_h != height)) {
      win_disp->toggle_w = width;
      win_disp->toggle_h = height;
      return true;
   }

   win_disp->can_acknowledge = false;

   if (d->flags & ALLEGRO_FULLSCREEN) {
      ALLEGRO_BITMAP *target_bmp;
      _AL_VECTOR disp_bmps;
      bool was_backbuffer = false;
      size_t i;

      target_bmp = al_get_target_bitmap();
      if (target_bmp && target_bmp->vt) {
         ALLEGRO_BITMAP_EXTRA_OPENGL *extra = target_bmp->parent ?
            target_bmp->parent->extra : target_bmp->extra;
         was_backbuffer = extra->is_backbuffer;
      }

      /* Remember display bitmaps. */
      _al_vector_init(&disp_bmps, sizeof(ALLEGRO_BITMAP*));
      for (i = 0; i < _al_vector_size(&d->bitmaps); i++) {
         ALLEGRO_BITMAP **dis = _al_vector_ref(&d->bitmaps, i);
         ALLEGRO_BITMAP **mem = _al_vector_alloc_back(&disp_bmps);
         *mem = *dis;
      }

      /* This flag prevents from switching to desktop resolution in between. */
      _wgl_do_not_change_display_mode = true;
      destroy_display_internals(wgl_disp);
      _wgl_do_not_change_display_mode = false;

      d->w = width;
      d->h = height;
      if(!create_display_internals(wgl_disp))
         return false;

      /* We have a new backbuffer now. */
      if (was_backbuffer)
         al_set_target_bitmap(al_get_backbuffer(d));

      /* Reupload bitmaps. */
      while (_al_vector_is_nonempty(&disp_bmps)) {
         ALLEGRO_BITMAP **back = _al_vector_ref_back(&disp_bmps);
         _al_convert_to_display_bitmap(*back);
         _al_vector_delete_at(&disp_bmps, _al_vector_size(&disp_bmps) - 1);
      }
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

      AdjustWindowRectEx(&win_size, wi.dwStyle, GetMenu(win_disp->window) ? TRUE : FALSE, wi.dwExStyle);

      if (!SetWindowPos(win_disp->window, HWND_TOP,
         0, 0,
         win_size.right - win_size.left,
         win_size.bottom - win_size.top,
         SWP_NOMOVE|SWP_NOZORDER))
            return false;

      d->w = width;
      d->h = height;
      if (!(d->flags & ALLEGRO_FULLSCREEN_WINDOW)) {
         win_disp->toggle_w = width;
         win_disp->toggle_h = height;
      }
   }

   return true;
}

static bool wgl_resize_display(ALLEGRO_DISPLAY *d, int width, int height)
{
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *)d;
   int orig_w = d->w;
   int orig_h = d->h;
   bool ret;

   win_display->ignore_resize = true;

   if (!wgl_resize_helper(d, width, height)) {
      wgl_resize_helper(d, orig_w, orig_h);
      ret = false;
   } else {
      ret = true;
      wgl_acknowledge_resize(d);
   }

   win_display->ignore_resize = false;

   return ret;
}

static bool wgl_acknowledge_resize(ALLEGRO_DISPLAY *d)
{
   WINDOWINFO wi;
   ALLEGRO_DISPLAY_WIN *win_disp = (ALLEGRO_DISPLAY_WIN *)d;
   int w, h;
   ALLEGRO_STATE state;

   wi.cbSize = sizeof(WINDOWINFO);
   GetWindowInfo(win_disp->window, &wi);
   w = wi.rcClient.right - wi.rcClient.left;
   h = wi.rcClient.bottom - wi.rcClient.top;

   d->w = w;
   d->h = h;

   _al_ogl_setup_gl(d);
   _al_ogl_update_render_state(d);

   al_store_state(&state, ALLEGRO_STATE_DISPLAY | ALLEGRO_STATE_TARGET_BITMAP);
   al_set_target_backbuffer(d);
   al_set_clipping_rectangle(0, 0, w, h);
   al_restore_state(&state);

   return true;
}


static bool wgl_is_compatible_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   /* Note: WGL driver doesn't support sharing of resources between contexts,
    * thus all bitmaps are tied to the display which was current at the time
    * al_create_bitmap was called.
    */
   return display == _al_get_bitmap_display(bitmap);
}


static void wgl_switch_in(ALLEGRO_DISPLAY *display)
{
   (void)display;
   /*
   ALLEGRO_DISPLAY_WIN *win_disp = (ALLEGRO_DISPLAY_WIN *)display;

   if (al_is_mouse_installed())
      al_set_mouse_range(win_disp->mouse_range_x1, win_disp->mouse_range_y1,
         win_disp->mouse_range_x2, win_disp->mouse_range_y2);
   */
}


static void wgl_switch_out(ALLEGRO_DISPLAY *display)
{
   (void)display;
}


static void wgl_set_window_position(ALLEGRO_DISPLAY *display, int x, int y)
{
   _al_win_set_window_position(((ALLEGRO_DISPLAY_WIN *)display)->window, x, y);
}


static void wgl_get_window_position(ALLEGRO_DISPLAY *display, int *x, int *y)
{
   _al_win_get_window_position(((ALLEGRO_DISPLAY_WIN *)display)->window, x, y);
}


/* Obtain a reference to this driver. */
ALLEGRO_DISPLAY_INTERFACE *_al_display_wgl_driver(void)
{
   if (vt.create_display)
      return &vt;

   vt.create_display = wgl_create_display;
   vt.destroy_display = wgl_destroy_display;
   vt.resize_display = wgl_resize_display;
   vt.set_current_display = wgl_set_current_display;
   vt.unset_current_display = wgl_unset_current_display;
   vt.flip_display = wgl_flip_display;
   vt.update_display_region = wgl_update_display_region;
   vt.acknowledge_resize = wgl_acknowledge_resize;
   vt.create_bitmap = _al_ogl_create_bitmap;
   vt.get_backbuffer = _al_ogl_get_backbuffer;
   vt.set_target_bitmap = _al_ogl_set_target_bitmap;
   vt.is_compatible_bitmap = wgl_is_compatible_bitmap;
   vt.switch_in = wgl_switch_in;
   vt.switch_out = wgl_switch_out;

   vt.set_mouse_cursor = _al_win_set_mouse_cursor;
   vt.set_system_mouse_cursor = _al_win_set_system_mouse_cursor;
   vt.show_mouse_cursor = _al_win_show_mouse_cursor;
   vt.hide_mouse_cursor = _al_win_hide_mouse_cursor;

   vt.set_icons = _al_win_set_display_icons;
   vt.set_window_position = wgl_set_window_position;
   vt.get_window_position = wgl_get_window_position;
   vt.set_window_constraints = _al_win_set_window_constraints;
   vt.get_window_constraints = _al_win_get_window_constraints;
   vt.apply_window_constraints = _al_win_apply_window_constraints;
   vt.set_display_flag = _al_win_set_display_flag;
   vt.set_window_title = _al_win_set_window_title;

   vt.update_render_state = _al_ogl_update_render_state;
   _al_ogl_add_drawing_functions(&vt);
   _al_win_add_clipboard_functions(&vt);

   return &vt;
}

int _al_wgl_get_num_display_modes(int format, int refresh_rate, int flags)
{
   DEVMODE dm;
   int count = 0;

   /* FIXME: Ignoring format.
    * To get a list of pixel formats we have to create a window with a valid DC, which
    * would even require to change the video mode in fullscreen cases and we really
    * don't want to do that.
    */
   (void)format;
   (void)refresh_rate;
   (void)flags;

   memset(&dm, 0, sizeof(dm));
   dm.dmSize = sizeof(dm);

   while (EnumDisplaySettings(NULL, count, &dm) != false) {
      count++;
   }

   return count;
}


ALLEGRO_DISPLAY_MODE *_al_wgl_get_display_mode(int index, int format,
                                               int refresh_rate, int flags,
                                               ALLEGRO_DISPLAY_MODE *mode)
{
   DEVMODE dm;

   /*
    * FIXME: see the comment in _al_wgl_get_num_display_modes
    */
   (void)format;
   (void)refresh_rate;
   (void)flags;

   memset(&dm, 0, sizeof(dm));
   dm.dmSize = sizeof(dm);

   if (!EnumDisplaySettings(NULL, index, &dm))
      return NULL;

   mode->width = dm.dmPelsWidth;
   mode->height = dm.dmPelsHeight;
   mode->refresh_rate = dm.dmDisplayFrequency;
   mode->format = format;
   switch (dm.dmBitsPerPel) {
      case 32:
         if (format == ALLEGRO_PIXEL_FORMAT_ANY)
            mode->format = ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA;
         else if (format == ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA)
            mode->format = ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA;
         break;
      case 24:
         mode->format = ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA;
         break;
      case 16:
         if (format == ALLEGRO_PIXEL_FORMAT_ANY)
            mode->format = ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA;
         else if(format == ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA)
            mode->format = ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA;
         break;
      default:
         break;
   }

   return mode;
}

/* vi: set sts=3 sw=3 et: */

