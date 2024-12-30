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
 *      Thread management.
 *
 *      By Stefan Schimanski.
 *
 *      Synchronization functions added by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/platform/aintwin.h"

#include <objbase.h>

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif

ALLEGRO_DEBUG_CHANNEL("system")


/* COINIT_MULTITHREADED is not defined in objbase.h for MSVC */
#define _COINIT_MULTITHREADED 0

typedef HRESULT(CALLBACK * _CoInitializeEx_ptr) (LPVOID, DWORD);
static _CoInitializeEx_ptr _CoInitializeEx = NULL;
static int first_call = 1;



/* _al_win_thread_init:
 *  Initializes COM interface for the calling thread.
 *  Attempts to use Distributed COM if available (installed by default
 *  on every 32-bit Windows starting with Win98 and Win NT4).
 */
void _al_win_thread_init(void)
{
   HMODULE ole32 = NULL;

   if (first_call) {
      first_call = 0;

      ole32 = GetModuleHandle(TEXT("OLE32.DLL"));
      if (ole32 != NULL) {
         _CoInitializeEx = (_CoInitializeEx_ptr) GetProcAddress(
                                                ole32, "CoInitializeEx");
      }
      else {
         ALLEGRO_WARN("OLE32.DLL can't be loaded.\n");
      }

      if (_CoInitializeEx == NULL) {
         ALLEGRO_WARN("Microsoft Distributed COM is not installed on this system. If you have problems ");
         ALLEGRO_WARN("with this application, please install the DCOM update. You can find it on the ");
         ALLEGRO_WARN("Microsoft homepage\n");
      }
   }

   if (_CoInitializeEx != NULL)
      _CoInitializeEx(NULL, _COINIT_MULTITHREADED);
   else
      CoInitialize(NULL);
}



/* _al_win_thread_exit:
 *  Shuts down COM interface for the calling thread.
 */
void _al_win_thread_exit(void)
{
   CoUninitialize();
}


