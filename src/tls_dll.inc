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
 *      Thread local storage.
 *
 *      See LICENSE.txt for copyright information.
 */

#include <windows.h>


/* Forward declaration to bypass strict warning. */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);


static DWORD tls_index;


void _al_tls_init_once(void)
{
   /* nothing */
}


static thread_local_state *tls_get(void)
{
   thread_local_state *t = TlsGetValue(tls_index);
   return t;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
   thread_local_state *data;

   (void)hinstDLL;
   (void)fdwReason;
   (void)lpvReserved;

   switch (fdwReason) {
      case DLL_PROCESS_ATTACH:
         if ((tls_index = TlsAlloc()) == TLS_OUT_OF_INDEXES) {
            return false;
	 }
         // No break: Initialize the index for first thread.
         // The attached process creates a new thread.

      case DLL_THREAD_ATTACH:
          // Initialize the TLS index for this thread.
          data = al_malloc(sizeof(*data));
          if (data != NULL) {
             TlsSetValue(tls_index, data);
             initialize_tls_values(data);
          }
          break;

        // The thread of the attached process terminates.
      case DLL_THREAD_DETACH:
         // Release the allocated memory for this thread.
         data = TlsGetValue(tls_index);
         if (data != NULL)
            al_free(data);

         break;

      // DLL unload due to process termination or FreeLibrary.
      case DLL_PROCESS_DETACH:
         // Release the allocated memory for this thread.
         data = TlsGetValue(tls_index);
         if (data != NULL)
            al_free(data);
         // Release the TLS index.
         TlsFree(tls_index);
         break;

      default:
         break;
   }

   return true;
}


/* vim: set ft=c sts=3 sw=3: */
