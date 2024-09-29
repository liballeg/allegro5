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
 *      CPU and hardware information.
 *
 *      By Beoran.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/cpu.h"
#include "allegro5/internal/aintern.h"

/* 
* The CPU and pysical memory detection functions below use 
* sysconf() if the right define is available as a parameter, 
* otherwise they use sysctl(), again if the right define is available. 
* This was chosen so because sysconf is POSIX and (in theory) 
* more portable than sysctl(). 
* On Windows, of course, the Windows API is always used.
*/

#ifdef A5O_HAVE_SYSCONF
#include <unistd.h>
#endif

#ifdef A5O_HAVE_SYSCTL
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#ifdef A5O_WINDOWS
#ifndef WINVER
#define WINVER 0x0500
#endif
#include <windows.h>
#endif


/* Function: al_get_cpu_count
 */
int al_get_cpu_count(void)
{
#if defined(A5O_HAVE_SYSCONF) && defined(_SC_NPROCESSORS_ONLN)
   return (int)sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(A5O_HAVE_SYSCTL)
   #if defined(HW_AVAILCPU)
      int mib[2] = {CTL_HW, HW_AVAILCPU};
   #elif defined(HW_NCPU)
      int mib[2] = {CTL_HW, HW_NCPU};
   #else
      return -1;
   #endif
   int ncpu = 1;
   size_t len = sizeof(ncpu);
   if (sysctl(mib, 2, &ncpu, &len, NULL, 0) == 0) { 
      return ncpu;
   }
#elif defined(A5O_WINDOWS)
   SYSTEM_INFO info;
   GetSystemInfo(&info);
   return info.dwNumberOfProcessors;
#endif
   return -1;
}

/* Function: al_get_ram_size
 */
int al_get_ram_size(void)
{
#if defined(A5O_HAVE_SYSCONF) && defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
   uint64_t aid = (uint64_t) sysconf(_SC_PHYS_PAGES);
   aid         *= (uint64_t) sysconf(_SC_PAGESIZE);
   aid         /= (uint64_t) (1024 * 1024);
   return (int)(aid);
#elif defined(A5O_HAVE_SYSCTL)
   #ifdef HW_REALMEM
      int mib[2] = {CTL_HW, HW_REALMEM};
   #elif defined(HW_PHYSMEM)
      int mib[2] = {CTL_HW, HW_PHYSMEM};
   #elif defined(HW_MEMSIZE)
      int mib[2] = {CTL_HW, HW_MEMSIZE};
   #else
      return -1;
   #endif
   uint64_t memsize = 0;
   size_t len = sizeof(memsize);
   if (sysctl(mib, 2, &memsize, &len, NULL, 0) == 0) { 
      return (int)(memsize / (1024*1024));
   }
#elif defined(A5O_WINDOWS)
   MEMORYSTATUSEX status;
   status.dwLength = sizeof(status);
   if (GlobalMemoryStatusEx(&status)) {
      return (int)(status.ullTotalPhys / (1024 * 1024));
   }
#endif
   return -1;
}


/* vi: set ts=4 sw=4 expandtab: */
      
