
#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/platform/aintosx.h"

#ifdef __LP64__
/* FIXME: the following prototype and enum definition appear to needed in
 * 64 bit mode on 10.5 (Leopard). Apple's documentation indicates that
 * "deprecated features" are not available in 64 bit, but
 * UpdateSystemActivity is not deprecated and the documentation likewise
 * suggests that all that should be required is to #include CoreServices.h
 * or Power.h. However, this does not appear to work... for now, this
 * should work ok.
 * On 10.6 (Snow Leopard) these defines cause a problem, so they are
 * disabled.
 */
extern OSErr UpdateSystemActivity(uint8_t activity);

#if 0
enum {
    OverallAct                    = 0,    /* Delays idle sleep by small amount                 */
    UsrActivity                   = 1,    /* Delays idle sleep and dimming by timeout time          */
    NetActivity                   = 2,    /* Delays idle sleep and power cycling by small amount         */
    HDActivity                    = 3,    /* Delays hard drive spindown and idle sleep by small amount  */
    IdleActivity                  = 4     /* Delays idle sleep by timeout time                 */
};
#endif

#endif

extern NSBundle *_al_osx_bundle;

/* For compatibility with the unix code */
static int    __crt0_argc;
static char **__crt0_argv;
static int (*user_main)(int, char **);

static char *arg0, *arg1 = NULL;

static BOOL in_bundle(void)
{
    /* This comes from the ADC tips & tricks section: how to detect if the app
     * lives inside a bundle
     */
    FSRef processRef;
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    FSCatalogInfo processInfo;
    GetProcessBundleLocation(&psn, &processRef);
    FSGetCatalogInfo(&processRef, kFSCatInfoNodeFlags, &processInfo, NULL, NULL, NULL);
    if (processInfo.nodeFlags & kFSNodeIsDirectoryMask)
        return YES;
    else
        return NO;
}


@interface AllegroAppDelegate : NSObject <NSApplicationDelegate>
{
    NSTimer* activity;
}
- (void) dealloc;
- (BOOL)application: (NSApplication *)theApplication openFile: (NSString *)filename;
- (void)applicationDidFinishLaunching: (NSNotification *)aNotification;
- (void)applicationDidChangeScreenParameters: (NSNotification *)aNotification;
+ (void)app_main: (id)arg;
- (NSApplicationTerminateReply) applicationShouldTerminate: (id)sender;
- (void) updateSystemActivity:(NSTimer*) timer;
- (void) setInhibitScreenSaver: (NSNumber*) inhibit;
@end

@interface AllegroWindowDelegate : NSObject
- (BOOL)windowShouldClose: (id)sender;
- (void)windowDidDeminiaturize: (NSNotification *)aNotification;
@end

@implementation AllegroAppDelegate

/* setInhibitScreenSaver:
 * If inhibit is YES, set up an infrequent timer to call
 * updateSystemActivity: to prevent the screen saver from activating
 * Must be called from the main thread (osx_inhibit_screensaver ensures
 * this)
 * Has no effect if inhibit is YES and the timer is already active
 * or if inhibit is NO and the timer is not active
 */
-(void) setInhibitScreenSaver: (NSNumber *) inhibit
{
    if ([inhibit boolValue] == YES) {
        if (activity == nil) {
            // Schedule every 30 seconds
            activity = [NSTimer scheduledTimerWithTimeInterval:30.0
                                                        target:self
                                                      selector:@selector(updateSystemActivity:)
                                                      userInfo:nil
                                                       repeats:YES];
            [activity retain];
        }
        // else already active
    }
    else {
        // OK to send message to nil if timer wasn't set.
        [activity invalidate];
        [activity release];
        activity = nil;
    }
}
/* updateSystemActivity:
 * called by a timer to inform the system that there is still activity and
 * therefore do not dim the screen/start the screensaver
 */
-(void) updateSystemActivity: (NSTimer*) timer
{
    (void)timer;
    UpdateSystemActivity(UsrActivity);
}

-(void) dealloc
{
    [activity invalidate];
    [activity release];
    [super dealloc];
}
- (BOOL)application: (NSApplication *)theApplication openFile: (NSString *)filename
{
    NSData* data = [filename dataUsingEncoding:NSASCIIStringEncoding allowLossyConversion:YES];
    (void)theApplication;
    if (data != nil) {
        unsigned int len = 1 + [data length];
        arg1 = al_malloc(len);
        memset(arg1, 0, len);
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 110000
        [data getBytes: arg1 length:len];
#else
        [data getBytes: arg1];
#endif
        return YES;
    }
    else {
        return NO;
    }
}



/* applicationDidFinishLaunching:
 *  Called when the app is ready to run.
 */
- (void)applicationDidFinishLaunching: (NSNotification *)aNotification
{
    NSString* exename, *resdir;
    NSFileManager* fm;
    BOOL isDir;

    (void)aNotification;

    if (in_bundle() == YES)
    {
        /* In a bundle, so chdir to the containing directory,
         * or to the 'magic' resource directory if it exists.
         * (see the readme.osx file for more info)
         */
        _al_osx_bundle = [NSBundle mainBundle];
        exename = [[_al_osx_bundle executablePath] lastPathComponent];
        resdir = [[_al_osx_bundle resourcePath] stringByAppendingPathComponent: exename];
        fm = [NSFileManager defaultManager];
        if ([fm fileExistsAtPath: resdir isDirectory: &isDir] && isDir) {
            /* Yes, it exists inside the bundle */
            [fm changeCurrentDirectoryPath: resdir];
        }
        else {
            /* No, change to the 'standard' OSX resource directory if it exists*/
            if ([fm fileExistsAtPath: [_al_osx_bundle resourcePath] isDirectory: &isDir] && isDir)
            {
                [fm changeCurrentDirectoryPath: [_al_osx_bundle resourcePath]];
            }
            /* It doesn't exist - this is unusual for a bundle. Don't chdir */
        }
        arg0 = strdup([[_al_osx_bundle bundlePath] fileSystemRepresentation]);
        if (arg1) {
            static char *args[2];
            args[0] = arg0;
            args[1] = arg1;
            __crt0_argv = args;
            __crt0_argc = 2;
        }
        else {
            __crt0_argv = &arg0;
            __crt0_argc = 1;
        }
    }
    /* else: not in a bundle so don't chdir */

    [NSThread detachNewThreadSelector: @selector(app_main:)
                             toTarget: [AllegroAppDelegate class]
                           withObject: nil];
    return;
}



/* applicationDidChangeScreenParameters:
 *  Invoked when the screen did change resolution/color depth.
 */
- (void)applicationDidChangeScreenParameters: (NSNotification *)aNotification
{
    /* no-op */
    (void)aNotification;
}



/* Call the user main() */
static void call_user_main(void)
{
    exit(user_main(__crt0_argc, __crt0_argv));
}



/* app_main:
 *  Thread dedicated to the user program; real main() gets called here.
 */
+ (void)app_main: (id)arg
{
    (void)arg;
    call_user_main();
}



/* applicationShouldTerminate:
 *  Called upon Command-Q or "Quit" menu item selection.
 *  Post a message but do not quit directly
 */
- (NSApplicationTerminateReply) applicationShouldTerminate: (id)sender
{
    (void)sender;
    _al_osx_post_quit();
    return NSTerminateCancel;
}

/* end of AllegroAppDelegate implementation */
@end



/* This prevents warnings that 'NSApplication might not
 * respond to setAppleMenu' on OS X 10.4+
 */
@interface NSApplication(AllegroOSX)
- (void)setAppleMenu:(NSMenu *)menu;
@end

/* Helper macro to add entries to the menu */
#define add_menu(name, sel, eq)                                         \
        [menu addItem: [[[NSMenuItem alloc]   \
                                    initWithTitle: name                 \
                                           action: @selector(sel)       \
                                    keyEquivalent: eq] autorelease]]                 \

int _al_osx_run_main(int argc, char **argv,
   int (*real_main)(int, char **))
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    AllegroAppDelegate *app_delegate = [[AllegroAppDelegate alloc] init];
    NSMenu *menu;
    NSMenuItem *temp_item;

    user_main = real_main;
    __crt0_argc = argc;
    __crt0_argv = argv;

#ifdef OSX_BOOTSTRAP_DETECTION
    if (!_al_osx_bootstrap_ok()) /* not safe to use NSApplication */
        call_user_main();
#endif

    [NSApplication sharedApplication];

    /* Load the main menu nib if possible */
    if ((!in_bundle()) || ([NSBundle loadNibNamed: @"MainMenu"
                                            owner: NSApp] == NO))
    {
        /* Didn't load the nib; create a default menu programmatically */
        NSString* title = nil;
        NSDictionary* app_dictionary = [[NSBundle mainBundle] infoDictionary];
        if (app_dictionary)
        {
            title = [app_dictionary objectForKey: @"CFBundleName"];
        }
        if (title == nil)
        {
            title = [[NSProcessInfo processInfo] processName];
        }
        NSMenu* main_menu = [[NSMenu alloc] initWithTitle: @""];
        [NSApp setMainMenu: main_menu];

        /* Add application ("Apple") menu */
        menu = [[NSMenu alloc] initWithTitle: @"Apple menu"];
        temp_item = [[NSMenuItem alloc]
                     initWithTitle: @""
                     action: NULL
                     keyEquivalent: @""];
        [main_menu addItem: temp_item];
        [main_menu setSubmenu: menu forItem: temp_item];
        [temp_item release];
        add_menu([@"Hide " stringByAppendingString: title], hide:, @"h");
        add_menu(@"Hide Others", hideOtherApplications:, @"");
        add_menu(@"Show All", unhideAllApplications:, @"");
        [menu addItem: [NSMenuItem separatorItem]];
        add_menu([@"Quit " stringByAppendingString: title], terminate:, @"q");
        [NSApp setAppleMenu: menu];
        [menu release];

        /* Add "Window" menu */
        menu = [[NSMenu alloc]
                        initWithTitle: @"Window"];
        temp_item = [[NSMenuItem alloc]
                                 initWithTitle: @""
                                        action: NULL
                                 keyEquivalent: @""];
        [main_menu addItem: temp_item];
        [main_menu setSubmenu: menu forItem: temp_item];
        [temp_item release];
        /* Add menu entries */
        add_menu(@"Minimize", performMiniaturize:, @"M");
        add_menu(@"Bring All to Front", arrangeInFront:, @"");
        [NSApp setWindowsMenu:menu];
        [menu release];

        [main_menu release];
    }
    // setDelegate: doesn't retain the delegate here (a Cocoa convention)
    // therefore we don't release it.
    [NSApp setDelegate: app_delegate];
    [pool drain];
    [NSApp run];
    /* Can never get here */
    [app_delegate release];
    return 0;
}
