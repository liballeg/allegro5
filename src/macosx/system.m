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


#include "allegro5/allegro.h"
#include "allegro5/platform/aintosx.h"
#include "allegro5/internal/aintern_osxclipboard.h"
#include <sys/stat.h>

#ifndef ALLEGRO_MACOSX
   #error something is wrong with the makefile
#endif

#import <CoreFoundation/CoreFoundation.h>
#include <mach/mach_port.h>
#include <mach-o/dyld.h>
#include <servers/bootstrap.h>

ALLEGRO_DEBUG_CHANNEL("MacOSX")

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


/* Global variables */
NSBundle *_al_osx_bundle = NULL;
static _AL_VECTOR osx_display_modes;
static ALLEGRO_SYSTEM osx_system;

/* osx_tell_dock:
 *  Tell the dock about us; promote us from a console app to a graphical app
 *  with dock icon and menu
 */
static void osx_tell_dock(void)
{
   ProcessSerialNumber psn = { 0, kCurrentProcess };
   TransformProcessType(&psn, kProcessTransformToForegroundApplication);
   [[NSApplication sharedApplication] performSelectorOnMainThread: @selector(activateIgnoringOtherApps:)
      withObject: [NSNumber numberWithBool:YES]
      waitUntilDone: YES];
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

   if (_al_osx_bundle == NULL) {
       /* If in a bundle, the dock will recognise us automatically */
       osx_tell_dock();
   }

   /* Mark the beginning of time. */
   _al_unix_init_time();

   _al_vector_init(&osx_display_modes, sizeof(ALLEGRO_DISPLAY_MODE));

   ALLEGRO_DEBUG("system driver initialised.\n");
   return &osx_system;
}



/* osx_sys_exit:
 *  Shuts down the system driver.
 */
static void osx_sys_exit(void)
{
   _al_vector_free(&osx_display_modes);
   ALLEGRO_DEBUG("system driver shutdown.\n");
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
   int adapter = al_get_new_display_adapter();
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

      /* FIXME (how?): in 64 bit-mode, this generates a warning, because
       * CGDirectDisplayID is apparently 32 bit whereas a pointer is 64
       * bit. Considering that a CGDirectDisplayID is supposed to be a
       * pointer as well (according to the documentation available on-line)
       * it is not quite clear what the correct way to do this would be.
       */
      display = (CGDirectDisplayID) [display_id pointerValue];
   }

   _al_vector_free(&osx_display_modes);
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
   /* Note: modes is owned by OSX and must not be released */
   modes = CGDisplayAvailableModes(display);
   ALLEGRO_INFO("detected %d display modes.\n", (int)CFArrayGetCount(modes));
   for (i = 0; i < CFArrayGetCount(modes); i++) {
      ALLEGRO_DISPLAY_MODE *mode;
      CFDictionaryRef dict = (CFDictionaryRef)CFArrayGetValueAtIndex(modes, i);
      CFNumberRef number;
      int value, samples;

      number = CFDictionaryGetValue(dict, kCGDisplayBitsPerPixel);
      CFNumberGetValue(number, kCFNumberIntType, &value);
      ALLEGRO_INFO("Mode %d has colour depth %d.\n", (int)i, value);
      if (depth && value != depth) {
         ALLEGRO_WARN("Skipping mode %d (requested colour depth %d).\n", (int)i, depth);
         continue;
      }

      number = CFDictionaryGetValue(dict, kCGDisplayRefreshRate);
      CFNumberGetValue(number, kCFNumberIntType, &value);
      ALLEGRO_INFO("Mode %d has colour depth %d.\n", (int)i, value);
      if (refresh_rate && value != refresh_rate) {
         ALLEGRO_WARN("Skipping mode %d (requested refresh rate %d).\n", (int)i, refresh_rate);
         continue;
      }

      mode = (ALLEGRO_DISPLAY_MODE *)_al_vector_alloc_back(&osx_display_modes);
      number = CFDictionaryGetValue(dict, kCGDisplayWidth);
      CFNumberGetValue(number, kCFNumberIntType, &mode->width);
      number = CFDictionaryGetValue(dict, kCGDisplayHeight);
      CFNumberGetValue(number, kCFNumberIntType, &mode->height);
      number = CFDictionaryGetValue(dict, kCGDisplayRefreshRate);
      CFNumberGetValue(number, kCFNumberIntType, &mode->refresh_rate);
      ALLEGRO_INFO("Mode %d is %dx%d@%dHz\n", (int)i, mode->width, mode->height, mode->refresh_rate);

      number = CFDictionaryGetValue(dict, kCGDisplayBitsPerPixel);
      CFNumberGetValue(number, kCFNumberIntType, &temp.settings[ALLEGRO_COLOR_SIZE]);
      number = CFDictionaryGetValue(dict, kCGDisplaySamplesPerPixel);
      CFNumberGetValue(number, kCFNumberIntType, &samples);
      number = CFDictionaryGetValue(dict, kCGDisplayBitsPerSample);
      CFNumberGetValue(number, kCFNumberIntType, &value);
      ALLEGRO_INFO("Mode %d has %d bits per pixel, %d samples per pixel and %d bits per sample\n",
                  (int)i, temp.settings[ALLEGRO_COLOR_SIZE], samples, value);
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
#else
   modes = CGDisplayCopyAllDisplayModes(display, NULL);
   ALLEGRO_INFO("detected %d display modes.\n", (int)CFArrayGetCount(modes));
   for (i = 0; i < CFArrayGetCount(modes); i++) {
      ALLEGRO_DISPLAY_MODE *amode;
      CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);
      CFStringRef pixel_encoding = CGDisplayModeCopyPixelEncoding(mode);

      int bpp = 0, mode_refresh_rate, samples = 0, value = 0;

      /* Determine pixel format. Whever thought this was a better idea than
       * having query functions for each of these properties should be
       * spoken to very harshly in a very severe voice.
       */

      if (CFStringCompare(pixel_encoding, CFSTR(kIO16BitFloatPixels), 1) == 0) {
         bpp = 64;
         samples = 3;
         value = 16;
      }

      if (CFStringCompare(pixel_encoding, CFSTR(kIO32BitFloatPixels), 1) == 0) {
         bpp = 128;
         samples = 3;
         value = 32;
      }

      if (CFStringCompare(pixel_encoding, CFSTR(kIO64BitDirectPixels), 1) == 0) {
         bpp = 64;
         samples = 3;
         value = 16;
      }

      if (CFStringCompare(pixel_encoding, CFSTR(kIO30BitDirectPixels), 1) == 0) {
         bpp = 32;
         samples = 3;
         value = 10;
      }

      if (CFStringCompare(pixel_encoding, CFSTR(IO32BitDirectPixels), 1) == 0) {
         bpp = 32;
         samples = 3;
         value = 8;
      }

      if (CFStringCompare(pixel_encoding, CFSTR(IO16BitDirectPixels), 1) == 0) {
         bpp = 16;
         samples = 3;
         value = 5;
      }

      if (CFStringCompare(pixel_encoding, CFSTR(IO8BitIndexedPixels), 1) == 0) {
         bpp = 8;
         samples = 1;
         value = 8;
      }
	  CFRelease(pixel_encoding);

      /* Check if this mode is ok in terms of depth and refresh rate */
      ALLEGRO_INFO("Mode %d has colour depth %d.\n", (int)i, bpp);
      if (depth && bpp != depth) {
         ALLEGRO_WARN("Skipping mode %d (requested colour depth %d).\n", (int)i, depth);
         continue;
      }

      mode_refresh_rate = CGDisplayModeGetRefreshRate(mode);
      ALLEGRO_INFO("Mode %d has a refresh rate of %d.\n", (int)i, mode_refresh_rate);
      if (refresh_rate && mode_refresh_rate != refresh_rate) {
         ALLEGRO_WARN("Skipping mode %d (requested refresh rate %d).\n", (int)i, refresh_rate);
         continue;
      }

      /* Yes, it's fine */
      amode = (ALLEGRO_DISPLAY_MODE *)_al_vector_alloc_back(&osx_display_modes);
      amode->width = CGDisplayModeGetWidth(mode);
      amode->height = CGDisplayModeGetHeight(mode);
      amode->refresh_rate = mode_refresh_rate;
      ALLEGRO_INFO("Mode %d is %dx%d@%dHz\n", (int)i, amode->width, amode->height, amode->refresh_rate);

      temp.settings[ALLEGRO_COLOR_SIZE] = bpp;
      ALLEGRO_INFO("Mode %d has %d bits per pixel, %d samples per pixel and %d bits per sample\n",
                  (int)i, temp.settings[ALLEGRO_COLOR_SIZE], samples, value);
      if (samples >= 3) {
         temp.settings[ALLEGRO_RED_SIZE] = value;
         temp.settings[ALLEGRO_GREEN_SIZE] = value;
         temp.settings[ALLEGRO_BLUE_SIZE] = value;
         if (samples == 4)
            temp.settings[ALLEGRO_ALPHA_SIZE] = value;
      }
      _al_fill_display_settings(&temp);
      amode->format = _al_deduce_color_format(&temp);
   }
   CFRelease(modes);
#endif
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
   int num = 0;

   screen_list = [NSScreen screens];
   if (screen_list)
      num = [screen_list count];

   ALLEGRO_INFO("Detected %d displays\n", num);
   return num;
}

int _al_osx_get_primary_screen_y(void)
{
   int count = osx_get_num_video_adapters();
   if (count == 0) {
      return 0;
   }
   NSArray *screen_list = [NSScreen screens];
   NSScreen *primary_screen = [screen_list objectAtIndex: 0];
   NSRect primary_rc = [primary_screen frame];
   return (int) (primary_rc.origin.y + primary_rc.size.height);
}

float _al_osx_get_global_scale_factor(void)
{
   int count = osx_get_num_video_adapters();
   if (count == 0) {
      return 1.;
   }
   float global_scale_factor = 1.;
   NSArray *screen_list = [NSScreen screens];
   for (int i = 0; i < count; i++) {
      NSScreen *screen = [screen_list objectAtIndex: i];
      float scale_factor = 1.0;
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
      if ([screen respondsToSelector:@selector(backingScaleFactor)]) {
        scale_factor = [screen backingScaleFactor];
      }
#endif
      if (scale_factor > global_scale_factor) {
         global_scale_factor = scale_factor;
      }
   }
   return global_scale_factor;
}

/* osx_get_monitor_info:
 * Return the details of one monitor
 */
static bool osx_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO* info)
{
   int count = osx_get_num_video_adapters();
   int primary_y = _al_osx_get_primary_screen_y();
   float global_scale_factor = _al_osx_get_global_scale_factor();
   if (adapter < count) {
      NSScreen *screen = [[NSScreen screens] objectAtIndex: adapter];
      NSRect rc = [screen frame];
      float scale_factor = 1.0;
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
      if ([screen respondsToSelector:@selector(backingScaleFactor)]) {
        scale_factor = [screen backingScaleFactor];
      }
#endif
      /*
       * The goal is to use top-left origin, y-down coordinate system
       * where units are individual pixels. We need to make sure the monitors
       * do not overlap, as otherwise we can't implement `al_set_window_position`.
       * We do this by creating an extended 2D plane which uses a global scale
       * factor with slots for the monitors. The slots are big enough
       * to fit monitors with the largest scale factor, but otherwise contain
       * empty space. When calling `al_get/set_window_position` we
       * transform from this extended plane to the visual coordinates used by the OS.
       *
       * OSX uses bottom-left, y-up coordinate system, so we need to flip things around.
       */
      info->x1 = (int) rc.origin.x * global_scale_factor;
      info->x2 = (int) (rc.origin.x * global_scale_factor + rc.size.width * scale_factor);
      info->y1 = (int) (global_scale_factor * (primary_y - (rc.size.height + rc.origin.y)));
      info->y2 = (int) (info->y1 + scale_factor * rc.size.height);
      ALLEGRO_INFO("Display %d has coordinates (%d, %d) - (%d, %d)\n",
                   adapter, info->x1, info->y1, info->x2, info->y2);
      return true;
   }
   else {
      return false;
   }
/*
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
      ALLEGRO_INFO("Display %d has coordinates (%d, %d) - (%d, %d)\n",
         adapter, info->x1, info->y1, info->x2, info->y2);
      return true;
   }
   else {
      return false;
   }
*/
}

/* osx_get_monitor_dpi:
 * Return the dots per inch value of one monitor
 */
static int osx_get_monitor_dpi(int adapter)
{
   int count = osx_get_num_video_adapters();
   if (adapter < count) {
      NSScreen *screen = [[NSScreen screens] objectAtIndex: adapter];
      NSRect rc = [screen frame];
      rc = [screen convertRectToBacking: rc];

      NSDictionary *description = [screen deviceDescription];
      CGSize size = CGDisplayScreenSize([[description objectForKey:@"NSScreenNumber"] unsignedIntValue]);
      float dpi_hori = rc.size.width / (_AL_INCHES_PER_MM * size.width);
      float dpi_vert = rc.size.height / (_AL_INCHES_PER_MM * size.height);

      return sqrt(dpi_hori * dpi_vert);
   }
   else {
      return 0;
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
   NSObject* delegate = [NSApp delegate];
   [delegate performSelectorOnMainThread: @selector(setInhibitScreenSaver:)
                              withObject: [NSNumber numberWithBool:inhibit ? YES : NO]
                           waitUntilDone: NO];
   ALLEGRO_INFO("Stop screensaver\n");
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
   al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);
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
   al_unlock_bitmap(bmp);
   [img addRepresentation:rep];
   [rep release];
   return img;
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

static void osx_thread_init(ALLEGRO_THREAD *thread)
{
}

static void osx_thread_exit(ALLEGRO_THREAD *thread)
{
}

/* Internal function to get a reference to this driver. */
ALLEGRO_SYSTEM_INTERFACE *_al_system_osx_driver(void)
{
   static ALLEGRO_SYSTEM_INTERFACE* vt = NULL;
   if (vt == NULL) {
      vt = al_malloc(sizeof(*vt));
      memset(vt, 0, sizeof(*vt));
      vt->id = ALLEGRO_SYSTEM_ID_MACOSX;
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
      vt->get_monitor_dpi = osx_get_monitor_dpi;
      vt->create_mouse_cursor = _al_osx_create_mouse_cursor;
      vt->destroy_mouse_cursor = _al_osx_destroy_mouse_cursor;
      vt->get_cursor_position = osx_get_cursor_position;
      vt->get_path = _al_osx_get_path;
      vt->inhibit_screensaver = osx_inhibit_screensaver;
      vt->thread_init = osx_thread_init;
      vt->thread_exit = osx_thread_exit;
      vt->get_time = _al_unix_get_time;
      vt->rest = _al_unix_rest;
      vt->init_timeout = _al_unix_init_timeout;

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

/* Implementation of get_path */
ALLEGRO_PATH *_al_osx_get_path(int id)
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   NSString* ans = nil;
   NSArray* paths = nil;
   NSString *org_name = [[NSString alloc] initWithUTF8String: al_get_org_name()];
   NSString *app_name = [[NSString alloc] initWithUTF8String: al_get_app_name()];
   ALLEGRO_PATH *path = NULL;

   switch (id) {
      case ALLEGRO_RESOURCES_PATH:
         if (_al_osx_bundle) {
            ans = [_al_osx_bundle resourcePath];
            path = al_create_path_for_directory([ans UTF8String]);
         } else {
            /* Otherwise, return the executable pathname */
            path = _al_osx_get_path(ALLEGRO_EXENAME_PATH);
            al_set_path_filename(path, NULL);
         }
         break;
      case ALLEGRO_TEMP_PATH:
         ans = NSTemporaryDirectory();
         path = al_create_path_for_directory([ans UTF8String]);
         break;
      case ALLEGRO_USER_DATA_PATH:
         paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory,
            NSUserDomainMask,
            YES);
         if ([paths count] > 0)
            ans = [paths objectAtIndex: 0];
         if (ans != nil) {
            /* Append program name */
            ans = [[ans stringByAppendingPathComponent: org_name]
                                     stringByAppendingPathComponent: app_name];
         }
         path = al_create_path_for_directory([ans UTF8String]);
         break;
      case ALLEGRO_USER_HOME_PATH:
         ans = NSHomeDirectory();
         path = al_create_path_for_directory([ans UTF8String]);
         break;
      case ALLEGRO_USER_DOCUMENTS_PATH:
         paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
            NSUserDomainMask,
            YES);
         if ([paths count] > 0)
            ans = [paths objectAtIndex: 0];
         path = al_create_path_for_directory([ans UTF8String]);
         break;
      case ALLEGRO_EXENAME_PATH: {
         char exepath[PATH_MAX];
         uint32_t size = sizeof(exepath);
         if (_NSGetExecutablePath(exepath, &size) == 0)
            ans = [NSString stringWithUTF8String: exepath];

         path = al_create_path([ans UTF8String]);
         break;
      }
      case ALLEGRO_USER_SETTINGS_PATH:
         paths =
         NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,
            NSUserDomainMask,
            YES);
         if ([paths count] > 0)
            ans = [paths objectAtIndex: 0];
         if (ans != nil) {
            /* Append program name */
            ans = [[ans stringByAppendingPathComponent: org_name]
                                     stringByAppendingPathComponent: app_name];
         }
         path = al_create_path_for_directory([ans UTF8String]);
         break;
      default:
         break;
   }
   [org_name release];
   [app_name release];
   [pool drain];

   return path;
}

/* _al_osx_post_quit
 * called when the user clicks the quit menu or cmd-Q.
 * Currently just sends a window close event to all windows.
 * This is a bit unsatisfactory
 */
void _al_osx_post_quit(void)
{
   unsigned int i;
   _AL_VECTOR* dpys = &al_get_system_driver()->displays;
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
