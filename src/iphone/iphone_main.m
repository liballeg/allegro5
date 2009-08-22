#import <UIKit/UIKit.h>
#import "allegroAppDelegate.h"
#include <allegro5/allegro5.h>
#include <allegro5/internal/aintern_iphone.h>
#include <pthread.h>

ALLEGRO_DEBUG_CHANNEL("iphone")

/* Not that there could ever be any arguments on iphone... */
static int global_argc;
static char **global_argv;

extern int (*_mangled_main_address)(int, char **);
/* We run the user's "main" in its own thread. */
static void *user_main(ALLEGRO_THREAD *thread, void *arg)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    ALLEGRO_INFO("Starting user main.\n");
    (*_mangled_main_address)(global_argc, global_argv);
    [pool release];
    ALLEGRO_INFO("User main has returned.\n");
    ALLEGRO_SYSTEM_IPHONE *iphone = (void *)al_system_driver();
    al_lock_mutex(iphone->mutex);
    iphone->has_shutdown = true;
    al_signal_cond(iphone->cond);
    al_unlock_mutex(iphone->mutex);
    return arg;
}

/* There really is no point running the user main before the application
 * has finished launching, so this function is called back when we receive
 * the applicationDidFinishLaunching message.
 */
void _al_iphone_run_user_main(void)
{
    ALLEGRO_THREAD *thread = al_create_thread(user_main, NULL);
    al_start_thread(thread);
}

void _al_iphone_init_path(void)
{
    /* On iphone, data always can only be in the resource bundle. So we set
     * the initial path to that. As each app is sandboxed, there is not much
     * else to access anyway.
     */
    NSFileManager *fm = [NSFileManager defaultManager];
    NSBundle *mainBundle = [NSBundle mainBundle];
    NSString *string = [mainBundle resourcePath];
    [fm changeCurrentDirectoryPath:string];
}

int main(int argc, char **argv) {
    global_argc = argc;
    global_argv = argv;
    [allegroAppDelegate run:argc:argv];
    ASSERT(false); /* can never reach */
    return 0;
}
