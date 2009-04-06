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
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/platform/aintosx.h"
#include <sys/stat.h>

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
static ALLEGRO_PATH *osx_get_path(int id);


/* Global variables */
int    __crt0_argc;
char **__crt0_argv;
NSBundle *osx_bundle = NULL;
struct _AL_MUTEX osx_event_mutex;
//AllegroWindow *osx_window = NULL;
//char osx_window_title[ALLEGRO_MESSAGE_SIZE];
void (*osx_window_close_hook)(void) = NULL;
//int osx_emulate_mouse_buttons = false;
//int osx_window_first_expose = false;
static _AL_VECTOR osx_display_modes;
static ALLEGRO_SYSTEM osx_system;

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



/* _al_osx_bootstrap_ok:
 *  Check if the current bootstrap context is privilege. If it's not, we can't
 *  use NSApplication, and instead have to go directly to main.
 *  Returns 1 if ok, 0 if not.
 */
#ifdef OSX_BOOTSTRAP_DETECTION
int _al_osx_bootstrap_ok(void)
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
#endif



/* osx_sys_init:
 *  Initalizes the MacOS X system driver.
 */
static ALLEGRO_SYSTEM* osx_sys_init(int flags)
{
   int v1 = 0, v2 = 0, v3 = 0; // version numbers read from ProductVersion
   (void)flags;
   
#ifdef OSX_BOOTSTRAP_DETECTION
   /* If we're in the 'dead bootstrap' environment, the Mac driver won't work. */
   if (!_al_osx_bootstrap_ok()) {
      return NULL;
   }
#endif
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
//   os_multitasking = true;
   
   
   _al_osx_threads_init();
   /* Mark the beginning of time. */
   _al_unix_init_time();

   _al_vector_init(&osx_display_modes, sizeof(ALLEGRO_DISPLAY_MODE));

   return &osx_system;
}



/* osx_sys_exit:
 *  Shuts down the system driver.
 */
static void osx_sys_exit(void)
{
   _al_vector_free(&osx_display_modes);
}



/*
 * _al_osx_get_num_display_modes:
 *  Gets the number of available display modes
 */
static int _al_osx_get_num_display_modes(void)
{
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *extras = _al_get_new_display_settings();
   ALLEGRO_EXTRA_DISPLAY_SETTINGS temp;
   int refresh_rate = al_get_new_display_refresh_rate();
   int adapter = al_get_current_video_adapter();
   int depth = 0;
   CGDirectDisplayID display;
   CFArrayRef modes;
   CFIndex i;
   
   if (extras)
      depth = extras->settings[ALLEGRO_COLOR_SIZE];
   memset(&temp, 0, sizeof(ALLEGRO_EXTRA_DISPLAY_SETTINGS));
   
   display = CGMainDisplayID();
   /* Get display ID for the requested display */
   if (adapter > 0) {
      NSScreen *screen = [[NSScreen screens] objectAtIndex: adapter];
      NSDictionary *dict = [screen deviceDescription];
      NSNumber *display_id = [dict valueForKey: @"NSScreenNumber"];
      display = (CGDirectDisplayID) [display_id pointerValue];
   }
   
   _al_vector_free(&osx_display_modes);
   modes = CGDisplayAvailableModes(display);
   for (i = 0; i < CFArrayGetCount(modes); i++) {
      ALLEGRO_DISPLAY_MODE *mode;
      CFDictionaryRef dict = (CFDictionaryRef)CFArrayGetValueAtIndex(modes, i);
      CFNumberRef number;
      int value, samples;
      number = CFDictionaryGetValue(dict, kCGDisplayBitsPerPixel);
      CFNumberGetValue(number, kCFNumberIntType, &value);
      if (depth && value != depth)
         continue;
      number = CFDictionaryGetValue(dict, kCGDisplayRefreshRate);
      CFNumberGetValue(number, kCFNumberIntType, &value);
      if (refresh_rate && value != refresh_rate)
         continue;
      mode = (ALLEGRO_DISPLAY_MODE *)_al_vector_alloc_back(&osx_display_modes);
      number = CFDictionaryGetValue(dict, kCGDisplayWidth);
      CFNumberGetValue(number, kCFNumberIntType, &mode->width);
      number = CFDictionaryGetValue(dict, kCGDisplayHeight);
      CFNumberGetValue(number, kCFNumberIntType, &mode->height);
      number = CFDictionaryGetValue(dict, kCGDisplayRefreshRate);
      CFNumberGetValue(number, kCFNumberIntType, &mode->refresh_rate);
      
      number = CFDictionaryGetValue(dict, kCGDisplayBitsPerPixel);
      CFNumberGetValue(number, kCFNumberIntType, &temp.settings[ALLEGRO_COLOR_SIZE]);
      number = CFDictionaryGetValue(dict, kCGDisplaySamplesPerPixel);
      CFNumberGetValue(number, kCFNumberIntType, &samples);
      number = CFDictionaryGetValue(dict, kCGDisplayBitsPerSample);
      CFNumberGetValue(number, kCFNumberIntType, &value);
      if (samples >= 3) {
         temp.settings[ALLEGRO_RED_SIZE] = value;
         temp.settings[ALLEGRO_GREEN_SIZE] = value;
         temp.settings[ALLEGRO_BLUE_SIZE] = value;
         if (samples == 4)
            temp.settings[ALLEGRO_ALPHA_SIZE] = value;
      }
      _al_fill_display_settings(&temp);
      mode->format = _al_deduce_color_format(&temp);
   }
   return _al_vector_size(&osx_display_modes);
}



/*
 * _al_osx_get_num_display_modes:
 *  Gets the number of available display modes
 */
static ALLEGRO_DISPLAY_MODE *_al_osx_get_display_mode(int index, ALLEGRO_DISPLAY_MODE *mode)
{
   if ((unsigned)index >= _al_vector_size(&osx_display_modes))
      return NULL;
   memcpy(mode, _al_vector_ref(&osx_display_modes, index), sizeof(ALLEGRO_DISPLAY_MODE));
   return mode;
}



/* osx_get_num_video_adapters:
 * Return the number of video adapters i.e displays
 */
static int osx_get_num_video_adapters(void) 
{
   NSArray *screen_list;
   screen_list = [NSScreen screens];
   if (screen_list)
      return [screen_list count];
   else
      return 0;
/*
   CGDisplayCount count;
   CGError err = CGGetActiveDisplayList(0, NULL, &count);
   if (err == kCGErrorSuccess) {
      return (int) count;
   }
   else {
      return 0;
   }
*/
}
/* osx_get_monitor_info:
 * Return the details of one monitor
 */
static void osx_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO* info) 
{
/*
   int count = osx_get_num_video_adapters();
   if (adapter < count) {
      NSScreen *screen = [[NSScreen screens] objectAtIndex: adapter];
      NSRect rc = [screen frame];
      info->x1 = (int) rc.origin.x;
      info->x2 = (int) (rc.origin.x + rc.size.width);
      info->y1 = (int) rc.origin.y;
      info->y2 = (int) (rc.origin.y + rc.size.height);
   }
*/
   CGDisplayCount count;
   // Assume no more than 16 monitors connected
   static const int max_displays = 16;
   CGDirectDisplayID displays[max_displays];
   CGError err = CGGetActiveDisplayList(max_displays, displays, &count);
   if (err == kCGErrorSuccess && adapter >= 0 && adapter < (int) count) {
      CGRect rc = CGDisplayBounds(displays[adapter]);
      info->x1 = (int) rc.origin.x;
      info->x2 = (int) (rc.origin.x + rc.size.width);
      info->y1 = (int) rc.origin.y;
      info->y2 = (int) (rc.origin.y + rc.size.height);
   }
}
/* osx_inhibit_screensaver:
 * Stops the screen dimming/screen saver activation if inhibit is true
 * otherwise re-enable normal behaviour. The last call takes force (i.e 
 * it does not count the calls to inhibit/uninhibit.
 * Always returns true
 */
static bool osx_inhibit_screensaver(bool inhibit)
{
	// Send a message to the App's delegate always on the main thread
	[[NSApp delegate] performSelectorOnMainThread: @selector(setInhibitScreenSaver:)
		withObject: [NSNumber numberWithBool:inhibit ? YES : NO]
		waitUntilDone: NO];
	return true;
}
/* NSImageFromAllegroBitmap:
 * Create an NSImage from an Allegro bitmap
 * This could definitely be speeded up if necessary.
 */
NSImage* NSImageFromAllegroBitmap(ALLEGRO_BITMAP* bmp)
{
   int w = al_get_bitmap_width(bmp);
   int h = al_get_bitmap_height(bmp);
   NSImage* img = [[NSImage alloc] initWithSize: NSMakeSize((float) w, (float) h)];
   NSBitmapImageRep* rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes: NULL // Allocate memory yourself
      pixelsWide:w 
      pixelsHigh:h 
      bitsPerSample: 8 
      samplesPerPixel: 4 
      hasAlpha:YES 
      isPlanar:NO 
      colorSpaceName:NSDeviceRGBColorSpace 
      bytesPerRow: 0 // Calculate yourself
      bitsPerPixel:0 ];// Calculate yourself
   int x, y;
   for (y = 0; y<h; ++y) {
      for (x = 0; x<w; ++x) {
         ALLEGRO_COLOR c = al_get_pixel(bmp, x, y);
         unsigned char* ptr = [rep bitmapData] + y * [rep bytesPerRow] + x * ([rep bitsPerPixel]/8);
         al_unmap_rgba(c, ptr, ptr+1, ptr+2, ptr+3);
         // NSImage should be premultiplied alpha
         ptr[0] *= c.a;
         ptr[1] *= c.a;
         ptr[2] *= c.a;
      }
   }
   [img addRepresentation:rep];
   [rep release];
   return [img autorelease];
}

/* This works as long as there is only one screen */
/* Not clear from docs how mouseLocation works if > 1 */
static bool osx_get_cursor_position(int *x, int *y) 
{
   NSPoint p = [NSEvent mouseLocation];
   NSRect r = [[NSScreen mainScreen] frame];
   *x = p.x;
   *y = r.size.height - p.y;
   return true;
}
/* Internal function to get a reference to this driver. */
ALLEGRO_SYSTEM_INTERFACE *_al_system_osx_driver(void)
{
	static ALLEGRO_SYSTEM_INTERFACE* vt = NULL;
   if (vt == NULL) {
      vt = _AL_MALLOC(sizeof(*vt));
      memset(vt, 0, sizeof(*vt));
      vt->initialize = osx_sys_init;
      vt->get_display_driver = _al_osx_get_display_driver;
      vt->get_keyboard_driver = _al_osx_get_keyboard_driver;
      vt->get_mouse_driver = _al_osx_get_mouse_driver;
      vt->get_joystick_driver = _al_osx_get_joystick_driver; 
	  vt->get_num_display_modes = _al_osx_get_num_display_modes;
      vt->get_display_mode = _al_osx_get_display_mode;
      vt->shutdown_system = osx_sys_exit;
      vt->get_num_video_adapters = osx_get_num_video_adapters;
      vt->get_monitor_info = osx_get_monitor_info;
      vt->get_cursor_position = osx_get_cursor_position;
      vt->get_path = osx_get_path;
	  vt->inhibit_screensaver = osx_inhibit_screensaver;
	};
		
	return vt;
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

/* _find_executable_file:
 *  Helper function: searches path and current directory for executable.
 *  Returns 1 on succes, 0 on failure.
 */
static int _find_executable_file(const char *filename, char *output, int size)
{
   char *path;

   /* If filename has an explicit path, search current directory */
   if (strchr(filename, '/')) {
      if (filename[0] == '/') {
         /* Full path; done */
         do_uconvert(filename, U_ASCII, output, U_UTF8, size);
         return 1;
      }
      else {
         struct stat finfo;
         char pathname[1024];
         int len;

         /* Prepend current directory */
         getcwd(pathname, sizeof(pathname));
         len = strlen(pathname);
         pathname[len] = '/';
         _al_sane_strncpy(pathname+len+1, filename, strlen(filename)+1);

         if ((stat(pathname, &finfo)==0) && (!S_ISDIR (finfo.st_mode))) {
            do_uconvert(pathname, U_ASCII, output, U_UTF8, size);
            return 1;
         }
      }
   }
   /* If filename has no explicit path, but we do have $PATH, search there */
   else if ((path = getenv("PATH"))) {
      char *start = path, *end = path, *buffer = NULL, *temp;
      struct stat finfo;

      while (*end) {
         end = strchr(start, ':');
         if (!end) end = strchr(start, '\0');

         /* Resize `buffer' for path component, slash, filename and a '\0' */
         temp = _AL_REALLOC (buffer, end - start + 1 + strlen (filename) + 1);
         if (temp) {
            buffer = temp;

            _al_sane_strncpy(buffer, start, end - start);
            *(buffer + (end - start)) = '/';
            _al_sane_strncpy(buffer + (end - start) + 1, filename, end - start + 1 + strlen (filename) + 1);

            if ((stat(buffer, &finfo)==0) && (!S_ISDIR (finfo.st_mode))) {
               do_uconvert(buffer, U_ASCII, output, U_UTF8, size);
               _AL_FREE (buffer);
               return 1;
            }
         } /* else... ignore the failure; `buffer' is still valid anyway. */

         start = end + 1;
      }
      /* Path search failed */
      _AL_FREE (buffer);
   }

   return 0;
}

/* Implentation of get_path */
static AL_CONST char *_fixme_osx_get_path(uint32_t id, char* path, size_t length)
{
   NSString* ans = nil;
   NSArray* paths = nil;
   NSString *orgname = [[NSString alloc] initWithUTF8String: al_get_orgname()];
   NSString *appname = [[NSString alloc] initWithUTF8String: al_get_appname()];
   BOOL ok = NO;
   switch (id) {
      case ALLEGRO_PROGRAM_PATH:
         ans = [[NSBundle mainBundle] bundlePath];
         break;
      case ALLEGRO_TEMP_PATH:
         ans = NSTemporaryDirectory();
         break;
      case ALLEGRO_SYSTEM_DATA_PATH:
         ans = [[NSBundle mainBundle] resourcePath];
         if (ans != nil) {
            /* Append program name */
            ans = [[ans stringByAppendingPathComponent: orgname]
                                     stringByAppendingPathComponent: appname];
         }
         break;
      case ALLEGRO_USER_DATA_PATH:
         paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory,
            NSUserDomainMask,
            YES);
         if ([paths count] > 0) ans = [paths objectAtIndex: 0];
         if (ans != nil) {
            /* Append program name */
            ans = [[ans stringByAppendingPathComponent: orgname]
                                     stringByAppendingPathComponent: appname];
         }
         break;
      case ALLEGRO_USER_HOME_PATH:
         ans = NSHomeDirectory();
         break;
      case ALLEGRO_EXENAME_PATH:
         /* If the application lives in a bundle, return the bundle path as
          * the executable path, since this is probably what is expected.
          */
         if (osx_bundle) {
            ans = [[NSBundle mainBundle] bundlePath];
         } else {
            /* OS X path names seem to always be UTF8.
             * Should we use the Darwin/BSD function getprogname() instead?
             */
            if (__crt0_argv[0][0] == '/') {
               ans = [NSString stringWithUTF8String: __crt0_argv[0]];
            } else {
               if (_find_executable_file(__crt0_argv[0], path, length))
                  ans = [NSString stringWithUTF8String: path];
            }
         }
         break;
      case ALLEGRO_USER_SETTINGS_PATH:
         paths =
         NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,
            NSUserDomainMask,
            YES);
         if ([paths count] > 0) ans = [paths objectAtIndex: 0];
         if (ans != nil) {
            /* Append program name */
            ans = [[ans stringByAppendingPathComponent: orgname]
                                     stringByAppendingPathComponent: appname];
         }
         break;
      case ALLEGRO_SYSTEM_SETTINGS_PATH:
         paths =
         NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,
            NSLocalDomainMask,
            YES);
         if ([paths count] > 0) ans = [paths objectAtIndex: 0];
         if (ans != nil) {
            /* Append program name */
            ans = [[ans stringByAppendingPathComponent: orgname]
                                     stringByAppendingPathComponent: appname];
         }
         break;
      default:
      break;
   }
   [orgname release];
   [appname release];
   if ((ans != nil) && (path != NULL)) {
      _al_sane_strncpy(path, [ans UTF8String], length);
   }
   return ok == YES ? path : NULL;
}

static ALLEGRO_PATH *osx_get_path(int id)
{
   // FIXME: get rid of arbitrary length
   char temp[PATH_MAX];
   _fixme_osx_get_path(id, temp, sizeof temp);
   return al_path_create(temp);
}

/* _al_osx_post_quit
 * called when the user clicks the quit menu or cmd-Q.
 * Currently just sends a window close event to all windows.
 * This is a bit unsatisfactory
 */
void _al_osx_post_quit(void) 
{
   unsigned int i;
   _AL_VECTOR* dpys = &al_system_driver()->displays;
   // Iterate through all existing displays 
   for (i = 0; i < _al_vector_size(dpys); ++i) {
      ALLEGRO_DISPLAY* dpy = *(ALLEGRO_DISPLAY**) _al_vector_ref(dpys, i);
      ALLEGRO_EVENT_SOURCE* src = &(dpy->es);
      _al_event_source_lock(src);
      ALLEGRO_EVENT evt;
      evt.type = ALLEGRO_EVENT_DISPLAY_CLOSE;
      // Send event
      _al_event_source_emit_event(src, &evt);
      _al_event_source_unlock(src);
   }
}

/* Local variables:       */
/* c-basic-offset: 3      */
/* indent-tabs-mode: nil  */
/* End:                   */
