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
 *      MacOS X system driver.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro5.h"
#include "allegro5/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
   #error something is wrong with the makefile
#endif

#ifndef SCAN_DEPEND
#import <CoreFoundation/CoreFoundation.h>
#include <mach/mach_port.h>
#include <servers/bootstrap.h>
#endif


/* These are used to warn the dock about the application */
struct CPSProcessSerNum
{
   UInt32 lo;
   UInt32 hi;
};
extern OSErr CPSGetCurrentProcess(struct CPSProcessSerNum *psn);
extern OSErr CPSEnableForegroundOperation(struct CPSProcessSerNum *psn, UInt32 _arg2, UInt32 _arg3, UInt32 _arg4, UInt32 _arg5);
extern OSErr CPSSetFrontProcess(struct CPSProcessSerNum *psn);



static ALLEGRO_SYSTEM* osx_sys_init(int flags);
ALLEGRO_SYSTEM_INTERFACE *_al_system_osx_driver(void);
static void osx_sys_exit(void);
static void osx_sys_message(AL_CONST char *);
static void osx_sys_get_executable_name(char *, int);
static int osx_sys_find_resource(char *, AL_CONST char *, int);
static void osx_sys_set_window_title(AL_CONST char *);
static int osx_sys_set_close_button_callback(void (*proc)(void));
static int osx_sys_set_display_switch_mode(int mode);
static int osx_sys_desktop_color_depth(void);
static int osx_sys_get_desktop_resolution(int *width, int *height);


/* Global variables */
int    __crt0_argc;
char **__crt0_argv;
NSBundle *osx_bundle = NULL;
struct _AL_MUTEX osx_event_mutex;
NSCursor *osx_cursor = NULL;
NSCursor *osx_blank_cursor = NULL;
//AllegroWindow *osx_window = NULL;
char osx_window_title[ALLEGRO_MESSAGE_SIZE];
void (*osx_window_close_hook)(void) = NULL;
int osx_gfx_mode = OSX_GFX_NONE;
int osx_emulate_mouse_buttons = FALSE;
int osx_window_first_expose = FALSE;
static ALLEGRO_SYSTEM osx_system;


/* Stub, do nothing */
static int osx_sys_init_compat(void) {
	return 0;
}

//SYSTEM_DRIVER system_macosx =
//{
//   SYSTEM_MACOSX,
//   empty_string,
//   empty_string,
//   "MacOS X",
//   osx_sys_init_compat,
//   osx_sys_exit,
//   osx_sys_get_executable_name,
//   osx_sys_find_resource,
//   osx_sys_set_window_title,
//   osx_sys_set_close_button_callback,
//   osx_sys_message,
//   NULL,  /* AL_METHOD(void, assert, (AL_CONST char *msg)); */
//   NULL,  /* AL_METHOD(void, save_console_state, (void)); */
//   NULL,  /* AL_METHOD(void, restore_console_state, (void)); */
//   NULL,  /* AL_METHOD(struct BITMAP *, create_bitmap, (int color_depth, int width, int height)); */
//   NULL,  /* AL_METHOD(void, created_bitmap, (struct BITMAP *bmp)); */
//   NULL,  /* AL_METHOD(struct BITMAP *, create_sub_bitmap, (struct BITMAP *parent, int x, int y, int width, int height)); */
//   NULL,  /* AL_METHOD(void, created_sub_bitmap, (struct BITMAP *bmp, struct BITMAP *parent)); */
//   NULL,  /* AL_METHOD(int, destroy_bitmap, (struct BITMAP *bitmap)); */
//   NULL,  /* AL_METHOD(void, read_hardware_palette, (void)); */
//   NULL,  /* AL_METHOD(void, set_palette_range, (AL_CONST struct RGB *p, int from, int to, int retracesync)); */
//   NULL,  /* AL_METHOD(struct GFX_VTABLE *, get_vtable, (int color_depth)); */
//   osx_sys_set_display_switch_mode,
//   NULL,  /* AL_METHOD(void, display_switch_lock, (int lock, int foreground)); */
//   osx_sys_desktop_color_depth,
//   osx_sys_get_desktop_resolution,
//   osx_sys_get_gfx_safe_mode,
//   NULL,
//   NULL,  /* AL_METHOD(_DRIVER_INFO *, gfx_drivers, (void)); */
//   NULL,  /* AL_METHOD(_DRIVER_INFO *, digi_drivers, (void)); */
//   NULL,  /* AL_METHOD(_DRIVER_INFO *, midi_drivers, (void)); */
//   NULL,  /* AL_METHOD(_DRIVER_INFO *, keyboard_drivers, (void)); */
//   NULL,  /* AL_METHOD(_DRIVER_INFO *, mouse_drivers, (void)); */
//   NULL   /* AL_METHOD(_DRIVER_INFO *, joystick_drivers, (void)); */
//};
//

//_DRIVER_INFO _system_driver_list[] = {
//{1, &system_macosx, TRUE },
//{0, NULL, FALSE},
//};

/* osx_signal_handler:
 *  Used to trap various signals, to make sure things get shut down cleanly.
 */
static RETSIGTYPE osx_signal_handler(int num)
{
   _al_mutex_unlock(&osx_event_mutex);
   //_al_mutex_unlock(&osx_window_mutex);
   
   allegro_exit();
   
   _al_mutex_destroy(&osx_event_mutex);
   //_al_mutex_destroy(&osx_window_mutex);
   
   fprintf(stderr, "Shutting down Allegro due to signal #%d\n", num);
   raise(num);
}


/* osx_tell_dock:
 *  Tell the dock about us; the origins of this hack are unknown, but it's
 *  currently the only way to make a Cocoa app to work when started from a
 *  console.
 *  For the future, (10.3 and above) investigate TranformProcessType in the 
 *  HIServices framework.
 */
static void osx_tell_dock(void)
{
   struct CPSProcessSerNum psn;

   if ((!CPSGetCurrentProcess(&psn)) &&
       (!CPSEnableForegroundOperation(&psn, 0x03, 0x3C, 0x2C, 0x1103)) &&
       (!CPSSetFrontProcess(&psn)))
      [NSApplication sharedApplication];
}



/* osx_bootstrap_ok:
 *  Check if the current bootstrap context is privilege. If it's not, we can't
 *  use NSApplication, and instead have to go directly to main.
 *  Returns 1 if ok, 0 if not.
 */
int osx_bootstrap_ok(void)
{
   static int _ok = -1;
   mach_port_t bp;
   kern_return_t ret;
   CFMachPortRef cfport;

   /* If have tested once, just return that answer */
   if (_ok >= 0)
      return _ok;
   cfport = CFMachPortCreate(NULL, NULL, NULL, NULL);
   task_get_bootstrap_port(mach_task_self(), &bp);
   ret = bootstrap_register(bp, "bootstrap-ok-test", CFMachPortGetPort(cfport));
   CFRelease(cfport);
   _ok = (ret == KERN_SUCCESS) ? 1 : 0;
   return _ok;
}



/* osx_sys_init:
 *  Initalizes the MacOS X system driver.
 */
static ALLEGRO_SYSTEM* osx_sys_init(int flags)
{
   int v1 = 0, v2 = 0, v3 = 0; // version numbers read from ProductVersion
   
   /* If we're in the 'dead bootstrap' environment, the Mac driver won't work. */
   if (!osx_bootstrap_ok()) {
      return NULL;
   }
	/* Initialise the vt and display list */
	osx_system.vt = _al_system_osx_driver();
	_al_vector_init(&osx_system.displays, sizeof(ALLEGRO_DISPLAY*));
  
   if (osx_bundle == NULL) {
       /* If in a bundle, the dock will recognise us automatically */
       osx_tell_dock();
   }
   
   /* Setup OS type & version */
   NSDictionary* sysinfo = [NSDictionary dictionaryWithContentsOfFile: @"/System/Library/CoreServices/SystemVersion.plist"];
   NSArray* version = [((NSString*) [sysinfo objectForKey:@"ProductVersion"]) componentsSeparatedByString:@"."];
   switch ( [version count] ){
   	/* if there are more than 3 versions just use the first 3 */
   	default :
	/* micro */
   	case 3 : v3 = [[version objectAtIndex: 2] intValue];
	/* minor */
	case 2 : v2 = [[version objectAtIndex: 1] intValue];
	/* major */
	case 1 : v1 = [[version objectAtIndex: 0] intValue];
	/* nothing at all */
	case 0 : break;
   }
//   os_version = 10 * v1 + v2;
//   os_revision = v3;
   [version release];
//   os_multitasking = TRUE;
   
   
   osx_gfx_mode = OSX_GFX_NONE;
   
//   set_display_switch_mode(SWITCH_BACKGROUND);
   //set_window_title([[[NSProcessInfo processInfo] processName] cString]);
   
   osx_threads_init();
   /* Mark the beginning of time. */
   _al_unix_init_time();

   return &osx_system;
}



/* osx_sys_exit:
 *  Shuts down the system driver.
 */
static void osx_sys_exit(void)
{
   
}



/* osx_sys_get_executable_name:
 *  Returns the full path to the application executable name. Note that if the
 *  exe is inside a bundle, this returns the full path of the bundle.
 */
static void osx_sys_get_executable_name(char *output, int size)
{
   if (osx_bundle)
      do_uconvert([[osx_bundle bundlePath] lossyCString], U_ASCII, output, U_CURRENT, size);
   else
      do_uconvert(__crt0_argv[0], U_ASCII, output, U_CURRENT, size);
}



/* osx_sys_find_resource:
 *  Searches the resource in the bundle resource path if the app is in a
 *  bundle, otherwise calls the unix resource finder routine.
 */
static int osx_sys_find_resource(char *dest, AL_CONST char *resource, int size)
{
   const char *path;
   char buf[256], tmp[256];
   
   if (osx_bundle) {
      path = [[osx_bundle resourcePath] cString];
      append_filename(buf, uconvert_ascii(path, tmp), resource, sizeof(buf));
      if (exists(buf)) {
         ustrzcpy(dest, size, buf);
	 return 0;
      }
   }
   return -1;
}



/* osx_sys_message:
 *  Displays a message using an alert panel.
 */
static void osx_sys_message(AL_CONST char *msg)
{
   char tmp[ALLEGRO_MESSAGE_SIZE];
   NSString *ns_title, *ns_msg;
   
   fputs(uconvert_toascii(msg, tmp), stderr);
   
   do_uconvert(msg, U_CURRENT, tmp, U_UTF8, ALLEGRO_MESSAGE_SIZE);
   ns_title = [NSString stringWithUTF8String: osx_window_title];
   ns_msg = [NSString stringWithUTF8String: tmp];
   
   NSRunAlertPanel(ns_title, ns_msg, nil, nil, nil);
}



/* osx_sys_set_window_title:
 *  Sets the title for both the application menu and the window if present.
 */
static void osx_sys_set_window_title(AL_CONST char *title)
{
//   char tmp[ALLEGRO_MESSAGE_SIZE];
//   
//   _al_sane_strncpy(osx_window_title, title, ALLEGRO_MESSAGE_SIZE);
//   do_uconvert(title, U_CURRENT, tmp, U_UTF8, ALLEGRO_MESSAGE_SIZE);
//
//   NSString *ns_title = [NSString stringWithUTF8String: tmp];
//   
//   if (osx_window)
//      [osx_window setTitle: ns_title];
}



/* osx_sys_set_close_button_callback:
 *  Sets the window close callback. Also used when user hits Command-Q or
 *  selects "Quit" from the application menu.
 */
static int osx_sys_set_close_button_callback(void (*proc)(void))
{
   osx_window_close_hook = proc;
   return 0;
}

/* osx_sys_desktop_color_depth:
 *  Queries the desktop color depth.
 */
static int osx_sys_desktop_color_depth(void)
{
   CFDictionaryRef mode = NULL;
   int color_depth;
   
   mode = CGDisplayCurrentMode(kCGDirectMainDisplay);
   if (!mode)
      return -1;
   CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayBitsPerPixel), kCFNumberSInt32Type, &color_depth);
   
   return color_depth == 16 ? 15 : color_depth;
}


/* osx_sys_get_desktop_resolution:
 *  Queries the desktop resolution.
 */
static int osx_sys_get_desktop_resolution(int *width, int *height)
{
   CFDictionaryRef mode = NULL;
   
   mode = CGDisplayCurrentMode(kCGDirectMainDisplay);
   if (!mode)
      return -1;
   CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayWidth), kCFNumberSInt32Type, width);
   CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayHeight), kCFNumberSInt32Type, height);
   
   return 0;
}

/* osx_get_num_video_adapters:
 * Return the number of video adapters i.e displays
 */
static int osx_get_num_video_adapters(void) 
{
   CGDisplayCount count;
   CGError err = CGGetActiveDisplayList(0, NULL, &count);
   if (err == kCGErrorSuccess) {
      return (int) count;
   }
   else {
      return 0;
   }
}
/* osx_get_monitor_info:
 * Return the details of one monitor
 */
static void osx_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO* info) 
{
   CGDisplayCount count;
   // Assume no more than 16 monitors connected
   CGDirectDisplayID displays[16];
   CGError err = CGGetActiveDisplayList(16, displays, &count);
   if (err == kCGErrorSuccess && adapter >= 0 && adapter < (int) count) {
      CGRect rc = CGDisplayBounds(displays[adapter]);
      info->x1 = (int) rc.origin.x;
      info->x2 = (int) (rc.origin.x + rc.size.width);
      info->y1 = (int) rc.origin.y;
      info->y2 = (int) (rc.origin.y + rc.size.height);
   }
}
/* This works as long as there is only one screen */
/* Not clear from docs how mouseLocation works if > 1 */
void osx_get_cursor_position(int *x, int *y) 
{
   NSPoint p = [NSEvent mouseLocation];
   NSRect r = [[NSScreen mainScreen] frame];
   *x = p.x;
   *y = r.size.height - p.y;
}
/* Internal function to get a reference to this driver. */
ALLEGRO_SYSTEM_INTERFACE *_al_system_osx_driver(void)
{
	static ALLEGRO_SYSTEM_INTERFACE vt = {
		0,//int id;
		osx_sys_init, //ALLEGRO_SYSTEM *(*initialize)(int flags);
		osx_get_display_driver,//ALLEGRO_DISPLAY_INTERFACE *(*get_display_driver)(void);
		osx_get_keyboard_driver,//ALLEGRO_KEYBOARD_DRIVER *(*get_keyboard_driver)(void);
		osx_get_mouse_driver,//ALLEGRO_MOUSE_DRIVER *(*get_mouse_driver)(void);
		NULL,//int (*get_num_display_modes)(void);
		NULL,//ALLEGRO_DISPLAY_MODE *(*get_display_mode)(int index, ALLEGRO_DISPLAY_MODE *mode);
		osx_sys_exit,//void (*shutdown_system)(void);
      osx_get_num_video_adapters,//int (*get_num_video_adapters)(void);
      osx_get_monitor_info,//void (*get_monitor_info)(int adapter, ALLEGRO_MONITOR_INFO *info);
      osx_get_cursor_position, //void (*get_cursor_position)(int *x, int *y);
	};
		
	return &vt;
}

/* This is a function each platform must define to register all available
 * system drivers.
 */
void _al_register_system_interfaces()
{
   ALLEGRO_SYSTEM_INTERFACE **add;

   add = _al_vector_alloc_back(&_al_system_interfaces);
   *add = _al_system_osx_driver();
}

/* Temporarily put this here until it appears in the header */
enum {
        AL_PROGRAM_PATH = 0,
        AL_TEMP_PATH,
        AL_SYSTEM_DATA_PATH,
        AL_USER_DATA_PATH,
        AL_USER_HOME_PATH,
        AL_LAST_PATH // must be last
};

/* Implentation of this function, not 'officially' in allegro yet */
int32_t _al_osx_get_path(int32_t id, char* path, size_t length) 
{
   NSString* ans = nil;
   NSArray* paths = nil;
   BOOL ok = NO;
   switch(id) {
      case AL_PROGRAM_PATH:
         ans = [[NSBundle mainBundle] bundlePath];
         break;
      case AL_TEMP_PATH:
         ans = NSTemporaryDirectory();
         break;
      case AL_SYSTEM_DATA_PATH:
         ans = [[NSBundle mainBundle] resourcePath];
         break;
      case AL_USER_DATA_PATH:
         paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory,
            NSUserDomainMask,
            YES);
         if ([paths count] > 0) ans = [paths objectAtIndex: 0];
         break;
      case AL_USER_HOME_PATH:
         ans = NSHomeDirectory();
      default:
      break;
   }
   if ((ans != nil) && (path != NULL)) {
      /* 10.4 and above only */
         ok = [ans getCString:path maxLength:length encoding: NSUTF8StringEncoding];
      }
   return ok == YES ? 0 : -1;
}

/* _al_osx_post_quit
 * called when the user clicks the quit menu or cmd-Q.
 * Currently just sends a window close event to all windows.
 * This is a bit unsatisfactory
 */
void _al_osx_post_quit(void) 
{
   int i;
   _AL_VECTOR* dpys = &al_system_driver()->displays;
   // Iterate through all existing displays 
   for (i = 0; i < _al_vector_size(dpys); ++i) {
      ALLEGRO_DISPLAY* dpy = *(ALLEGRO_DISPLAY**) _al_vector_ref(dpys, i);
      ALLEGRO_EVENT_SOURCE* src = &(dpy->es);
      _al_event_source_lock(src);
      ALLEGRO_EVENT* evt = _al_event_source_get_unused_event(src);
      evt->type = ALLEGRO_EVENT_DISPLAY_CLOSE;
      // Send event
      _al_event_source_emit_event(src, evt);
      _al_event_source_unlock(src);
   }
}

/* Local variables:       */
/* c-basic-offset: 3      */
/* indent-tabs-mode: nil  */
/* End:                   */
