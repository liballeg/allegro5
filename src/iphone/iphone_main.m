#import <UIKit/UIKit.h>
#import "allegroAppDelegate.h"
#include <allegro5/allegro.h>
#include <allegro5/internal/aintern_iphone.h>
#include <pthread.h>

A5O_DEBUG_CHANNEL("iphone")

/* Not that there could ever be any arguments on iphone... */
static int global_argc;
static char **global_argv;
extern int _al_mangled_main(int, char **);

/* We run the user's "main" in its own thread. */
static void *user_main(A5O_THREAD *thread, void *arg)
{
    (void)thread;
    A5O_INFO("Starting user main.\n");
    _al_mangled_main(global_argc, global_argv);
    A5O_INFO("User main has returned.\n");
    A5O_SYSTEM_IPHONE *iphone = (void *)al_get_system_driver();
    al_lock_mutex(iphone->mutex);
    iphone->has_shutdown = true;
    al_signal_cond(iphone->cond);
    al_unlock_mutex(iphone->mutex);
    /* Apple does not allow iphone applications to shutdown and provides
     * no API for it:
     * http://developer.apple.com/iphone/library/qa/qa2008/qa1561.html
     *
     * Therefore we only call exit here if the user actually returned from
     * there main - in that case crashing the app is better then freezing it.
     */
    if (!iphone->wants_shutdown)
        exit(0); /* "crash grazefully" */
    return arg;
}

/* There really is no point running the user main before the application
 * has finished launching, so this function is called back when we receive
 * the applicationDidFinishLaunching message.
 */
void _al_iphone_run_user_main(void)
{
    A5O_THREAD *thread = al_create_thread(user_main, NULL);
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
