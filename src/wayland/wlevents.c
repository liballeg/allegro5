#include <sys/poll.h>

#include "allegro5/allegro.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/internal/aintern_wl.h"
#include "allegro5/internal/aintern_wlinput.h"
#include "allegro5/internal/aintern_wlsystem.h"
#include "allegro5/internal/aintern_wlevents.h"

ALLEGRO_DEBUG_CHANNEL("xevents")

void _al_wl_background_thread(_AL_THREAD *self, void *arg)
{
    ALLEGRO_SYSTEM_WAYLAND *s = arg;
    int fd = wl_display_get_fd(s->display);
    struct pollfd pfd = { fd, POLLIN, 0 };
    
    while (!_al_get_thread_should_stop(self)) {
        _al_mutex_lock(&s->lock);

        while (wl_display_prepare_read(s->display) != 0)
            wl_display_dispatch_pending(s->display);
        wl_display_flush(s->display);

        _al_mutex_unlock(&s->lock);

        poll(&pfd, 1, 100);

        _al_mutex_lock(&s->lock);

        if (pfd.revents & POLLIN)
            wl_display_read_events(s->display);
        else
            wl_display_cancel_read(s->display);
        wl_display_dispatch_pending(s->display);

        _al_mutex_unlock(&s->lock);

        /* Emit key-repeat events for a held key.  Runs on this thread so
         * it is serialised with the key event handlers above. */
        _al_wl_keyboard_repeat_tick();
    }
}