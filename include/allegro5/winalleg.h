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

#endif          /* ifndef WIN_ALLEGRO_H */
