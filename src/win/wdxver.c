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
 *      Get version of installed DirectX.
 *
 *      By Stefan Schimanski.
 *
 *      Modified get_dx_ver() from DirectX 7 SDK.
 *
 *      See readme.txt for copyright information.
 */

/* Set DIRECTX_SDK_VERSION according to the installed SDK version.
 * Note that this file requires at least the SDK version 5 to compile,
 * so that DIRECTX_SDK_VERSION must be >= 0x500.
 */
#define DIRECTX_SDK_VERSION 0x500


#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintwin.h"

#ifndef SCAN_DEPEND
   #include <ddraw.h>
   #include <dinput.h>
#endif


typedef HRESULT(WINAPI * DIRECTDRAWCREATE) (GUID *, LPDIRECTDRAW *, IUnknown *);
typedef HRESULT(WINAPI * DIRECTINPUTCREATE) (HINSTANCE, DWORD, LPDIRECTINPUT *,
					     IUnknown *);


/* get_dx_ver:
 *  returns the DirectX dx_version number:
 *
 *          0       No DirectX installed
 *          0x100   DirectX 1 installed
 *          0x200   DirectX 2 installed
 *          0x300   DirectX 3 installed
 *          0x500   At least DirectX 5 installed
 *          0x600   At least DirectX 6 installed
 *          0x700   At least DirectX 7 installed
 *
 */
int get_dx_ver(void)
{
   HRESULT hr;
   HINSTANCE ddraw_hinst = 0;
   HINSTANCE dinput_hinst = 0;
   LPDIRECTDRAW directdraw = 0;
   LPDIRECTDRAW2 directdraw2 = 0;
   DIRECTDRAWCREATE DirectDrawCreate = 0;
   DIRECTINPUTCREATE DirectInputCreate = 0;
   OSVERSIONINFO os_version;
   LPDIRECTDRAWSURFACE ddraw_surf = 0;
   LPDIRECTDRAWSURFACE3 ddraw_surf3 = 0;

#if DIRECTX_SDK_VERSION >= 0x600
   LPDIRECTDRAWSURFACE4 ddraw_surf4 = 0;

#if DIRECTX_SDK_VERSION >= 0x700
   LPDIRECTDRAWSURFACE7 ddraw_surf7 = 0;
#endif

#endif

   DDSURFACEDESC ddraw_surf_desc;
   int dx_version = 0;

   /*
    * First get the windows platform
    */
   os_version.dwOSVersionInfoSize = sizeof(os_version);
   if (!GetVersionEx(&os_version)) {
      dx_version = 0;
      return dx_version;
   }

   if (os_version.dwPlatformId == VER_PLATFORM_WIN32_NT) {
      /*
       * NT is easy... NT 4.0 is DX2, 4.0 SP3 is DX3, 5.0 is DX5
       * and no DX on earlier versions.
       */
      if (os_version.dwMajorVersion < 4) {
	 /* No DX on NT3.51 or earlier */

	 return dx_version;
      }

      if (os_version.dwMajorVersion == 4) {
	 /*
	  * NT4 up to SP2 is DX2, and SP3 onwards is DX3, so we are at least DX2
	  */
	 dx_version = 0x200;

	 /*
	  * We're not supposed to be able to tell which SP we're on, so check for dinput
	  */
	 dinput_hinst = LoadLibrary("DINPUT.DLL");
	 if (dinput_hinst == 0) {
	    /*
	     * No DInput... must be DX2 on NT 4 pre-SP3
	     */
	    OutputDebugString("Couldn't LoadLibrary DInput\r\n");
	    return dx_version;
	 }

	 DirectInputCreate = (DIRECTINPUTCREATE)
	     GetProcAddress(dinput_hinst, "DirectInputCreateA");
	 FreeLibrary(dinput_hinst);

	 if (DirectInputCreate == 0) {
	    /*
	     * No DInput... must be pre-SP3 DX2
	     */
	    return dx_version;
	 }

	 /*
	  * It must be NT4, DX2
	  */
	 dx_version = 0x300;    /* DX3 on NT4 SP3 or higher */

	 return dx_version;
      }
      /*
       * Else it's NT5 or higher, and it's DX5a or higher:
       */

      /*
       * Drop through to Win9x tests for a test of DDraw (DX6 or higher)
       */
   }

   /*
    * Now we know we are in Windows 9x (or maybe 3.1), so anything's possible.
    * First see if DDRAW.DLL even exists.
    */
   ddraw_hinst = LoadLibrary("DDRAW.DLL");
   if (ddraw_hinst == 0) {
      dx_version = 0;
      FreeLibrary(ddraw_hinst);
      return dx_version;
   }

   /*
    *  See if we can create the DirectDraw object.
    */
   DirectDrawCreate = (DIRECTDRAWCREATE)
       GetProcAddress(ddraw_hinst, "DirectDrawCreate");
   if (DirectDrawCreate == 0) {
      dx_version = 0;
      FreeLibrary(ddraw_hinst);
      return dx_version;
   }

   hr = DirectDrawCreate(NULL, &directdraw, NULL);
   if (FAILED(hr)) {
      dx_version = 0;
      FreeLibrary(ddraw_hinst);
      return dx_version;
   }

   /*
    *  So DirectDraw exists.  We are at least DX1.
    */
   dx_version = 0x100;

   /*
    *  Let's see if IID_IDirectDraw2 exists.
    */
   hr = IDirectDraw_QueryInterface(directdraw, &IID_IDirectDraw2, (LPVOID *) & directdraw2);
   if (FAILED(hr)) {
      /*
       * No IDirectDraw2 exists... must be DX1
       */
      IDirectDraw_Release(directdraw);
      FreeLibrary(ddraw_hinst);
      return dx_version;
   }
   /*
    * IDirectDraw2 exists. We must be at least DX2
    */
   IDirectDraw2_Release(directdraw2);
   dx_version = 0x200;

   /*
    *  See if we can create the DirectInput object.
    */
   dinput_hinst = LoadLibrary("DINPUT.DLL");
   if (dinput_hinst == 0) {
      /*
       * No DInput... must be DX2
       */
      IDirectDraw_Release(directdraw);
      FreeLibrary(ddraw_hinst);
      return dx_version;
   }

   DirectInputCreate = (DIRECTINPUTCREATE) GetProcAddress(dinput_hinst, "DirectInputCreateA");
   FreeLibrary(dinput_hinst);

   if (DirectInputCreate == 0) {
      /*
       * No DInput... must be DX2
       */
      FreeLibrary(ddraw_hinst);
      IDirectDraw_Release(directdraw);
      return dx_version;
   }

   /*
    * DirectInputCreate exists. That's enough to tell us that we are at least DX3
    */
   dx_version = 0x300;

   /*
    * Checks for 3a vs 3b?
    */

   /*
    * We can tell if DX5 is present by checking for the existence of IDirectDrawSurface3.
    * First we need a surface to QI off of.
    */
   memset(&ddraw_surf_desc, 0, sizeof(ddraw_surf_desc));
   ddraw_surf_desc.dwSize = sizeof(ddraw_surf_desc);
   ddraw_surf_desc.dwFlags = DDSD_CAPS;
   ddraw_surf_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

   hr = IDirectDraw_SetCooperativeLevel(directdraw, NULL, DDSCL_NORMAL);
   if (FAILED(hr)) {
      /*
       * Failure. This means DDraw isn't properly installed.
       */
      IDirectDraw_Release(directdraw);
      FreeLibrary(ddraw_hinst);
      dx_version = 0;
      return dx_version;
   }

   hr = IDirectDraw_CreateSurface(directdraw, &ddraw_surf_desc, &ddraw_surf, NULL);
   if (FAILED(hr)) {
      /*
       * Failure. This means DDraw isn't properly installed.
       */
      IDirectDraw_Release(directdraw);
      FreeLibrary(ddraw_hinst);
      dx_version = 0;
      return dx_version;
   }

   /*
    * Try for the IDirectDrawSurface3 interface. If it works, we're on DX5 at least
    */
   if (FAILED(IDirectDrawSurface_QueryInterface(ddraw_surf, &IID_IDirectDrawSurface3, (LPVOID *) &ddraw_surf3))) {
      IDirectDraw_Release(directdraw);
      FreeLibrary(ddraw_hinst);
      return dx_version;
   }

   /*
    * QI for IDirectDrawSurface3 succeeded. We must be at least DX5
    */
   dx_version = 0x500;

#if DIRECTX_SDK_VERSION >= 0x600
   /*
    * Try for the IDirectDrawSurface4 interface. If it works, we're on DX6 at least
    */
   if (FAILED(IDirectDrawSurface_QueryInterface(ddraw_surf, &IID_IDirectDrawSurface4, (LPVOID *) &ddraw_surf4))) {
      IDirectDraw_Release(directdraw);
      FreeLibrary(ddraw_hinst);
      return dx_version;
   }

   /*
    * QI for IDirectDrawSurface4 succeeded. We must be at least DX6
    */
   dx_version = 0x600;

#if DIRECTX_SDK_VERSION >= 0x700
   /*
    * Try for the IDirectDrawSurface7 interface. If it works, we're on DX7 at least
    */
   if (FAILED(IDirectDrawSurface_QueryInterface(ddraw_surf, &IID_IDirectDrawSurface7, (LPVOID *) &ddraw_surf7))) {
      IDirectDraw_Release(directdraw);
      FreeLibrary(ddraw_hinst);
      return dx_version;
   }

   /*
    * QI for IDirectDrawSurface7 succeeded. We must be at least DX7
    */
   dx_version = 0x700;
#endif

#endif

   IDirectDrawSurface_Release(ddraw_surf);
   IDirectDraw_Release(directdraw);
   FreeLibrary(ddraw_hinst);

   return dx_version;
}
