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
 *      Windows header file for the Allegro library.
 *
 *      It must be included by Allegro programs that need to use
 *      direct Win32 API calls and by Win32 programs that need to
 *      interface with Allegro.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef WIN_ALLEGRO_H
#define WIN_ALLEGRO_H

#ifndef ALLEGRO_H
   #error Please include allegro.h before winalleg.h!
#endif

#ifdef ALLEGRO_SRC
   #define WIN32_LEAN_AND_MEAN   /* to save compilation time */
#endif



/* bodges to avoid conflicts between Allegro and Windows */
#define BITMAP WINDOWS_BITMAP

#if (!defined SCAN_EXPORT) && (!defined SCAN_DEPEND)
   #ifdef ALLEGRO_AND_MFC
      #ifdef DEBUGMODE
         #define AL_ASSERT(condition)     { if (!(condition)) al_assert(__FILE__, __LINE__); }
         #define AL_TRACE                 al_trace
      #else
         #define AL_ASSERT(condition)
         #define AL_TRACE                 1 ? (void) 0 : al_trace
      #endif

      #undef TRACE
      #undef ASSERT

      #include <afxwin.h>
   #else
      #include <windows.h>
   #endif
#endif

#define WINDOWS_RGB(r,g,b)  ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

#undef BITMAP
#undef RGB



/* Allegro's Win32 specific interface */
#ifdef __cplusplus
   extern "C" {
#endif


/* D3D stuff */


#if defined(ALLEGRO_CFG_D3D)

#ifndef SCAN_DEPEND
#include <d3d9.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

AL_FUNC(LPDIRECT3DDEVICE9, al_d3d_get_device, (ALLEGRO_DISPLAY *));
AL_FUNC(HWND, al_d3d_get_hwnd, (ALLEGRO_DISPLAY *));
AL_FUNC(LPDIRECT3DTEXTURE9, al_d3d_get_system_texture, (ALLEGRO_BITMAP *));
AL_FUNC(LPDIRECT3DTEXTURE9, al_d3d_get_video_texture, (ALLEGRO_BITMAP *));
AL_FUNC(bool, al_d3d_supports_non_pow2_textures, (void));
AL_FUNC(bool, al_d3d_supports_non_square_textures, (void));

#ifdef __cplusplus
}
#endif

#ifdef ALLEGRO_BCC32
#define sqrtf (float)sqrt
#endif

#endif //ALLEGRO_CFG_D3D

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef WIN_ALLEGRO_H */
