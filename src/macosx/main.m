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
 *      main() function replacement for MacOS X.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
   #error something is wrong with the makefile
#endif

#include "allegro/internal/alconfig.h"
#undef main


/* For compatibility with the unix code */
int    __crt0_argc;
char **__crt0_argv;
extern void *_mangled_main_address;



/* These are used to warn the dock about the application */
typedef struct CPSProcessSerNum
{
   UInt32 lo;
   UInt32 hi;
} CPSProcessSerNum;

extern OSErr CPSGetCurrentProcess( CPSProcessSerNum *psn);
extern OSErr CPSEnableForegroundOperation( CPSProcessSerNum *psn, UInt32 _arg2, UInt32 _arg3, UInt32 _arg4, UInt32 _arg5);
extern OSErr CPSSetFrontProcess( CPSProcessSerNum *psn);



@implementation AllegroAppDelegate

/* applicationShouldTerminate:
 *  Called when the user asks for app termination.
 */
- (NSApplicationTerminateReply)applicationShouldTerminate: (NSApplication *)sender
{
   if (osx_window_close_hook)
      osx_window_close_hook();
   return NSTerminateCancel;
}




/* applicationDidFinishLaunching:
 *  Called when the app is ready to run. This runs the system events pump and
 *  updates the app window if it exists.
 */
- (void)applicationDidFinishLaunching: (NSNotification *)aNotification
{
   NSAutoreleasePool *pool = NULL;
   
   _unix_bg_man = &_bg_man_pthreads;
   if (_unix_bg_man->init())
      return;
   
   pthread_mutex_init(&osx_event_mutex, NULL);
   
   pool = [[NSAutoreleasePool alloc] init];
   
   [NSThread detachNewThreadSelector: @selector(app_main:)
      toTarget: [AllegroAppDelegate class]
      withObject: nil];
   
//   [NSThread setThreadPriority: [NSThread threadPriority] + 0.1];
   
   while (1) {
      _unix_bg_man->disable_interrupts();
      pthread_mutex_lock(&osx_event_mutex);
      osx_event_handler();
      if (osx_gfx_mode == OSX_GFX_WINDOW)
         osx_update_dirty_lines();
      pthread_mutex_unlock(&osx_event_mutex);
      _unix_bg_man->enable_interrupts();
      usleep(1000000 / 70);
   }
   
   [pool release];
   pthread_mutex_destroy(&osx_event_mutex);
   _unix_bg_man->exit();
}



/* app_main:
 *  Thread dedicated to the user program; real main() gets called here.
 */
+ (void)app_main: (id)arg
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   int (*real_main) (int, char*[]) = (int (*) (int, char*[])) _mangled_main_address;
   
   /* Wait for the app to become active */
   while (![NSApp isActive]);
   
   /* Call the user main() */
   exit(real_main(__crt0_argc, __crt0_argv));
   
   [pool release];
}

@end



/* main:
 *  Replacement for main function.
 */
int main(int argc, char *argv[])
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   AllegroAppDelegate *app_delegate = [[AllegroAppDelegate alloc] init];
   CPSProcessSerNum psn;
   
   __crt0_argc = argc;
   __crt0_argv = argv;
   
   [NSApplication sharedApplication];
      
   /* Tell the dock about us */
   if ((!CPSGetCurrentProcess(&psn)) &&
       (!CPSEnableForegroundOperation(&psn, 0x03, 0x3C, 0x2C, 0x1103)) &&
       (!CPSSetFrontProcess(&psn)))
      [NSApplication sharedApplication];
   
   [NSApp setMainMenu: [[NSMenu alloc] init]];
   [NSApp setDelegate: app_delegate];
   
   [NSApp run];
   /* Can never get here */
   
   return 0;
}
