#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_wlclipboard.h"

#define WL_CLIPBOARD_TIMEOUT 5000
#define WL_CLIPBOARD_MAX_SIZE (16 * 1024 * 1024)
#define WL_CLIPBOARD_MAX_TRANSFERS 8

typedef struct ALLEGRO_WL_CLIPBOARD_OFFER {
    struct wl_data_offer *proxy;
    unsigned int refs;
    bool retired;
    bool utf8_mime;
    bool utf8_string_mime;
    bool plain_mime;
    bool text_mime;
    bool string_mime;
} ALLEGRO_WL_CLIPBOARD_OFFER;

typedef struct ALLEGRO_WL_CLIPBOARD_SOURCE {
    ALLEGRO_SYSTEM_WAYLAND *system;
    struct wl_data_source *proxy;
    char *text;
} ALLEGRO_WL_CLIPBOARD_SOURCE;

/* Workers own a text snapshot and never touch Wayland objects or the system
 * lock. The event thread owns this list and joins workers before freeing it. */
typedef struct ALLEGRO_WL_CLIPBOARD_TRANSFER {
    struct ALLEGRO_WL_CLIPBOARD_TRANSFER *next;
    pthread_t thread;
    volatile uint32_t cancelled;
    volatile uint32_t done;
    int fd;
    char *text;
} ALLEGRO_WL_CLIPBOARD_TRANSFER;

static const char *clipboard_offer_get_mime(ALLEGRO_WL_CLIPBOARD_OFFER *offer)
{
    if (offer->utf8_mime)
        return "text/plain;charset=utf-8";
    if (offer->utf8_string_mime)
        return "UTF8_STRING";
    if (offer->plain_mime)
        return "text/plain";
    if (offer->text_mime)
        return "TEXT";
    if (offer->string_mime)
        return "STRING";
    return NULL;
}

/* X11's STRING target is Latin-1, unlike the UTF-8 text targets. */
static char *clipboard_decode_text(char *text, size_t size, const char *mime)
{
    char *utf8;
    size_t i, used = 0;

    if (!text || strcmp(mime, "STRING") != 0)
        return text;
    utf8 = al_malloc(size * 2 + 1);
    if (utf8) {
        for (i = 0; i < size; i++) {
            unsigned char c = (unsigned char)text[i];
            if (c >= 128) {
                utf8[used++] = (char)(0xc0 | (c >> 6));
                utf8[used++] = (char)(0x80 | (c & 0x3f));
            }
            else {
                utf8[used++] = (char)c;
            }
        }
        utf8[used] = '\0';
    }
    al_free(text);
    return utf8;
}

static void offer_destroy(ALLEGRO_WL_CLIPBOARD_OFFER *offer)
{
    if (offer->proxy)
        wl_data_offer_destroy(offer->proxy);
    al_free(offer);
}

static void offer_retire(ALLEGRO_WL_CLIPBOARD_OFFER *offer)
{
    if (!offer)
        return;
    offer->retired = true;
    if (offer->refs == 0)
        offer_destroy(offer);
}

static void clipboard_offer_mime(void *data, struct wl_data_offer *proxy,
    const char *mime_type)
{
    ALLEGRO_WL_CLIPBOARD_OFFER *offer = data;
    (void)proxy;

    if (strcmp(mime_type, "text/plain;charset=utf-8") == 0)
        offer->utf8_mime = true;
    else if (strcmp(mime_type, "UTF8_STRING") == 0)
        offer->utf8_string_mime = true;
    else if (strcmp(mime_type, "text/plain") == 0)
        offer->plain_mime = true;
    else if (strcmp(mime_type, "TEXT") == 0)
        offer->text_mime = true;
    else if (strcmp(mime_type, "STRING") == 0)
        offer->string_mime = true;
}

static void clipboard_offer_source_actions(void *data,
    struct wl_data_offer *proxy, uint32_t actions)
{
    (void)data;
    (void)proxy;
    (void)actions;
}

static void clipboard_offer_action(void *data, struct wl_data_offer *proxy,
    uint32_t action)
{
    (void)data;
    (void)proxy;
    (void)action;
}

static const struct wl_data_offer_listener clipboard_offer_listener = {
    .offer = clipboard_offer_mime,
    .source_actions = clipboard_offer_source_actions,
    .action = clipboard_offer_action,
};

static void clipboard_device_offer(void *data, struct wl_data_device *device,
    struct wl_data_offer *proxy)
{
    ALLEGRO_SYSTEM_WAYLAND *system = data;
    ALLEGRO_WL_CLIPBOARD_OFFER *offer = al_calloc(1, sizeof *offer);
    (void)device;

    if (!offer) {
        wl_data_offer_destroy(proxy);
        return;
    }
    offer->proxy = proxy;
    if (wl_data_offer_add_listener(proxy, &clipboard_offer_listener, offer) < 0) {
        wl_data_offer_destroy(proxy);
        al_free(offer);
        return;
    }

    if (system->pending_clipboard_offer) {
        ALLEGRO_WL_CLIPBOARD_OFFER *pending =
            system->pending_clipboard_offer;
        if (pending != system->clipboard_offer)
            offer_retire(pending);
    }
    system->pending_clipboard_offer = offer;
}

static void clipboard_device_enter(void *data, struct wl_data_device *device,
    uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y,
    struct wl_data_offer *offer)
{
    (void)data;
    (void)device;
    (void)serial;
    (void)surface;
    (void)x;
    (void)y;
    (void)offer;
}

static void clipboard_device_leave(void *data, struct wl_data_device *device)
{
    (void)data;
    (void)device;
}

static void clipboard_device_motion(void *data, struct wl_data_device *device,
    uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    (void)data;
    (void)device;
    (void)time;
    (void)x;
    (void)y;
}

static void clipboard_device_drop(void *data, struct wl_data_device *device)
{
    (void)data;
    (void)device;
}

static void clipboard_device_selection(void *data, struct wl_data_device *device,
    struct wl_data_offer *proxy)
{
    ALLEGRO_SYSTEM_WAYLAND *system = data;
    ALLEGRO_WL_CLIPBOARD_OFFER *new_offer = NULL;
    ALLEGRO_WL_CLIPBOARD_OFFER *old_offer = system->clipboard_offer;
    ALLEGRO_WL_CLIPBOARD_OFFER *pending_offer =
        system->pending_clipboard_offer;
    (void)device;

    if (proxy && old_offer && old_offer->proxy == proxy)
        new_offer = old_offer;
    else if (proxy && system->pending_clipboard_offer) {
        ALLEGRO_WL_CLIPBOARD_OFFER *pending =
            system->pending_clipboard_offer;
        if (pending->proxy == proxy)
            new_offer = pending;
    }

    system->pending_clipboard_offer = NULL;
    system->clipboard_offer = new_offer;
    if (pending_offer && pending_offer != new_offer && pending_offer != old_offer)
        offer_retire(pending_offer);
    if (old_offer && old_offer != new_offer)
        offer_retire(old_offer);
}

static const struct wl_data_device_listener clipboard_device_listener = {
    .data_offer = clipboard_device_offer,
    .enter = clipboard_device_enter,
    .leave = clipboard_device_leave,
    .motion = clipboard_device_motion,
    .drop = clipboard_device_drop,
    .selection = clipboard_device_selection,
};

static double clipboard_monotonic_time(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec + now.tv_nsec / 1000000000.0;
}

static void *clipboard_transfer_thread(void *data)
{
    ALLEGRO_WL_CLIPBOARD_TRANSFER *transfer = data;
    size_t remaining = strlen(transfer->text);
    const char *text = transfer->text;
    double deadline = clipboard_monotonic_time() +
        WL_CLIPBOARD_TIMEOUT / 1000.0;
    struct pollfd pfd = { transfer->fd, POLLOUT, 0 };
    sigset_t sigpipe_set;
    sigset_t old_set;
    sigset_t pending;
    bool blocked = false;
    bool had_pending = false;
    bool broken_pipe = false;

    sigemptyset(&sigpipe_set);
    sigaddset(&sigpipe_set, SIGPIPE);
    if (pthread_sigmask(SIG_BLOCK, &sigpipe_set, &old_set) == 0) {
        blocked = true;
        if (sigpending(&pending) == 0)
            had_pending = sigismember(&pending, SIGPIPE) == 1;
    }

    /* If signal masking failed, don't risk terminating the application. */
    while (blocked && remaining > 0 &&
        !__sync_val_compare_and_swap(&transfer->cancelled, 0, 0)) {
        ssize_t written;
        int ready;
        if (clipboard_monotonic_time() >= deadline)
            break;
        /* Short waits bound cancellation latency during shutdown. */
        ready = poll(&pfd, 1, 50);
        if (ready < 0 && errno == EINTR)
            continue;
        if (ready < 0 || (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)))
            break;
        if (!(pfd.revents & POLLOUT))
            continue;
        written = write(transfer->fd, text, remaining);
        if (written < 0 && (errno == EINTR || errno == EAGAIN ||
            errno == EWOULDBLOCK))
            continue;
        if (written < 0 && errno == EPIPE)
            broken_pipe = true;
        if (written <= 0)
            break;
        text += written;
        remaining -= (size_t)written;
    }
    close(transfer->fd);

    if (blocked) {
        if (broken_pipe && !had_pending) {
            struct timespec no_wait = { 0, 0 };
            while (sigtimedwait(&sigpipe_set, NULL, &no_wait) < 0 &&
                errno == EINTR) {
                /* Consume the SIGPIPE generated by our failed write. */
            }
        }
        pthread_sigmask(SIG_SETMASK, &old_set, NULL);
    }
    __sync_lock_test_and_set(&transfer->done, 1);
    return NULL;
}

/* Caller holds system->lock, or has already stopped the event thread. */
static void clipboard_reap_transfers(ALLEGRO_SYSTEM_WAYLAND *system,
    bool cancel)
{
    ALLEGRO_WL_CLIPBOARD_TRANSFER **link = &system->clipboard_transfers;
    if (cancel) {
        ALLEGRO_WL_CLIPBOARD_TRANSFER *transfer;
        for (transfer = *link; transfer; transfer = transfer->next)
            __sync_lock_test_and_set(&transfer->cancelled, 1);
    }
    while (*link) {
        ALLEGRO_WL_CLIPBOARD_TRANSFER *transfer = *link;
        if (cancel || __sync_val_compare_and_swap(&transfer->done, 0, 0)) {
            pthread_join(transfer->thread, NULL);
            *link = transfer->next;
            al_free(transfer->text);
            al_free(transfer);
        }
        else {
            link = &transfer->next;
        }
    }
}

void _al_wl_clipboard_reap_transfers(ALLEGRO_SYSTEM_WAYLAND *system)
{
    clipboard_reap_transfers(system, false);
}

static void clipboard_source_target(void *data, struct wl_data_source *proxy,
    const char *mime_type)
{
    (void)data;
    (void)proxy;
    (void)mime_type;
}

static void clipboard_source_send(void *data, struct wl_data_source *proxy,
    const char *mime_type, int32_t fd)
{
    ALLEGRO_WL_CLIPBOARD_SOURCE *source = data;
    ALLEGRO_SYSTEM_WAYLAND *system = source->system;
    ALLEGRO_WL_CLIPBOARD_TRANSFER *transfer, *pending;
    size_t count = 0;
    int flags;
    (void)proxy;

    if (strcmp(mime_type, "text/plain;charset=utf-8") != 0 &&
        strcmp(mime_type, "UTF8_STRING") != 0 &&
        strcmp(mime_type, "text/plain") != 0) {
        close(fd);
        return;
    }
    clipboard_reap_transfers(system, false);
    for (pending = system->clipboard_transfers; pending; pending = pending->next)
        count++;
    if (count >= WL_CLIPBOARD_MAX_TRANSFERS) {
        close(fd);
        return;
    }
    flags = fcntl(fd, F_GETFL);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0 ||
        fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) {
        close(fd);
        return;
    }
    transfer = al_calloc(1, sizeof *transfer);
    if (!transfer) {
        close(fd);
        return;
    }
    transfer->fd = fd;
    transfer->text = _al_strdup(source->text);
    if (!transfer->text ||
        pthread_create(&transfer->thread, NULL, clipboard_transfer_thread,
            transfer) != 0) {
        close(fd);
        al_free(transfer->text);
        al_free(transfer);
        return;
    }
    transfer->next = system->clipboard_transfers;
    system->clipboard_transfers = transfer;
}

static void clipboard_source_destroy(ALLEGRO_SYSTEM_WAYLAND *system)
{
    ALLEGRO_WL_CLIPBOARD_SOURCE *source = system->clipboard_source_state;
    if (!source)
        return;
    system->clipboard_source_state = NULL;
    wl_data_source_destroy(source->proxy);
    al_free(source->text);
    al_free(source);
}

static void clipboard_source_cancelled(void *data,
    struct wl_data_source *proxy)
{
    ALLEGRO_WL_CLIPBOARD_SOURCE *source = data;
    ALLEGRO_SYSTEM_WAYLAND *system = source->system;

    if (system->clipboard_source_state == source) {
        system->clipboard_source_state = NULL;
    }
    wl_data_source_destroy(proxy);
    al_free(source->text);
    al_free(source);
}

static void clipboard_source_dnd_event(void *data,
    struct wl_data_source *proxy)
{
    (void)data;
    (void)proxy;
}

static void clipboard_source_action(void *data, struct wl_data_source *proxy,
    uint32_t action)
{
    (void)data;
    (void)proxy;
    (void)action;
}

static const struct wl_data_source_listener clipboard_source_listener = {
    .target = clipboard_source_target,
    .send = clipboard_source_send,
    .cancelled = clipboard_source_cancelled,
    .dnd_drop_performed = clipboard_source_dnd_event,
    .dnd_finished = clipboard_source_dnd_event,
    .action = clipboard_source_action,
};

void _al_wl_clipboard_seat_changed(ALLEGRO_SYSTEM_WAYLAND *system)
{
    if (!system)
        return;

    clipboard_reap_transfers(system, true);
    system->input_serial = 0;
    if (system->data_device) {
        wl_data_device_destroy(system->data_device);
        system->data_device = NULL;
    }

    {
        ALLEGRO_WL_CLIPBOARD_OFFER *current = system->clipboard_offer;
        ALLEGRO_WL_CLIPBOARD_OFFER *pending =
            system->pending_clipboard_offer;
        system->clipboard_offer = NULL;
        system->pending_clipboard_offer = NULL;
        if (current)
            offer_retire(current);
        if (pending && pending != current)
            offer_retire(pending);
    }

    if (system->seat && system->data_device_manager) {
        system->data_device = wl_data_device_manager_get_data_device(
            system->data_device_manager, system->seat);
        if (system->data_device &&
            wl_data_device_add_listener(system->data_device,
                &clipboard_device_listener, system) < 0) {
            wl_data_device_destroy(system->data_device);
            system->data_device = NULL;
        }
    }
    else if (!system->seat) {
        clipboard_source_destroy(system);
    }
}

void _al_wl_clipboard_shutdown(ALLEGRO_SYSTEM_WAYLAND *system)
{
    if (!system)
        return;

    if (system->data_device) {
        wl_data_device_destroy(system->data_device);
        system->data_device = NULL;
    }
    {
        ALLEGRO_WL_CLIPBOARD_OFFER *current = system->clipboard_offer;
        ALLEGRO_WL_CLIPBOARD_OFFER *pending =
            system->pending_clipboard_offer;
        system->clipboard_offer = NULL;
        system->pending_clipboard_offer = NULL;
        if (current)
            offer_retire(current);
        if (pending && pending != current)
            offer_retire(pending);
    }

    clipboard_source_destroy(system);
    clipboard_reap_transfers(system, true);
}

static char *wl_get_clipboard_text(ALLEGRO_DISPLAY *display)
{
    ALLEGRO_SYSTEM_WAYLAND *system =
        (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    ALLEGRO_WL_CLIPBOARD_OFFER *offer;
    ALLEGRO_WL_CLIPBOARD_SOURCE *source;
    struct pollfd pfd;
    char *text = NULL;
    size_t used = 0;
    size_t capacity = 4096;
    const char *mime_type;
    int fds[2];
    int timeout;
    double deadline;
    (void)display;

    if (!system)
        return NULL;

    _al_mutex_lock(&system->lock);
    source = system->clipboard_source_state;
    if (source) {
        text = _al_strdup(source->text);
        _al_mutex_unlock(&system->lock);
        return text;
    }

    offer = system->clipboard_offer;
    if (!offer || offer->refs != 0) {
        _al_mutex_unlock(&system->lock);
        return NULL;
    }

    mime_type = clipboard_offer_get_mime(offer);
    if (!mime_type) {
        _al_mutex_unlock(&system->lock);
        return NULL;
    }

    if (pipe(fds) != 0) {
        _al_mutex_unlock(&system->lock);
        return NULL;
    }
    offer->refs++;
    wl_data_offer_receive(offer->proxy, mime_type, fds[1]);
    close(fds[1]);
    wl_display_flush(system->display);
    _al_mutex_unlock(&system->lock);

    text = al_malloc(capacity);
    if (!text) {
        close(fds[0]);
        goto done;
    }

    pfd.fd = fds[0];
    pfd.events = POLLIN | POLLHUP;
    deadline = al_get_time() + WL_CLIPBOARD_TIMEOUT / 1000.0;
    for (;;) {
        ssize_t n;
        double remaining = deadline - al_get_time();
        if (remaining <= 0) {
            al_free(text);
            text = NULL;
            break;
        }
        timeout = (int)(remaining * 1000.0);
        if (poll(&pfd, 1, timeout) < 0) {
            if (errno == EINTR)
                continue;
            al_free(text);
            text = NULL;
            break;
        }
        if (pfd.revents == 0) {
            al_free(text);
            text = NULL;
            break;
        }
        if (used == WL_CLIPBOARD_MAX_SIZE) {
            char extra;
            n = read(fds[0], &extra, 1);
            if (n < 0 && errno == EINTR)
                continue;
            /* EOF is allowed at exactly the size limit. */
            if (n != 0) {
                al_free(text);
                text = NULL;
            }
            break;
        }
        if (capacity - used < 2048) {
            size_t new_capacity = capacity * 2;
            char *new_text;
            if (new_capacity > WL_CLIPBOARD_MAX_SIZE + 1)
                new_capacity = WL_CLIPBOARD_MAX_SIZE + 1;
            new_text = al_realloc(text, new_capacity);
            if (!new_text) {
                al_free(text);
                text = NULL;
                break;
            }
            text = new_text;
            capacity = new_capacity;
        }
        n = read(fds[0], text + used, capacity - used - 1);
        if (n < 0 && errno == EINTR)
            continue;
        if (n < 0) {
            al_free(text);
            text = NULL;
            break;
        }
        if (n == 0)
            break;
        used += (size_t)n;
    }
    close(fds[0]);
    if (text) {
        text[used] = '\0';
        text = clipboard_decode_text(text, used, mime_type);
    }

done:
    _al_mutex_lock(&system->lock);
    offer->refs--;
    if (offer->retired && offer->refs == 0)
        offer_destroy(offer);
    _al_mutex_unlock(&system->lock);
    return text;
}

static bool wl_set_clipboard_text(ALLEGRO_DISPLAY *display, const char *text)
{
    ALLEGRO_SYSTEM_WAYLAND *system =
        (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    ALLEGRO_WL_CLIPBOARD_SOURCE *source;
    (void)display;

    if (!system || !text || strlen(text) > WL_CLIPBOARD_MAX_SIZE)
        return false;

    source = al_calloc(1, sizeof *source);
    if (!source)
        return false;
    source->text = _al_strdup(text);
    if (!source->text) {
        al_free(source);
        return false;
    }
    source->system = system;

    _al_mutex_lock(&system->lock);
    if (!system->data_device_manager || !system->data_device ||
        system->input_serial == 0) {
        _al_mutex_unlock(&system->lock);
        al_free(source->text);
        al_free(source);
        return false;
    }

    source->proxy = wl_data_device_manager_create_data_source(
        system->data_device_manager);
    if (!source->proxy) {
        _al_mutex_unlock(&system->lock);
        al_free(source->text);
        al_free(source);
        return false;
    }
    if (wl_data_source_add_listener(source->proxy,
        &clipboard_source_listener, source) < 0) {
        wl_data_source_destroy(source->proxy);
        _al_mutex_unlock(&system->lock);
        al_free(source->text);
        al_free(source);
        return false;
    }
    wl_data_source_offer(source->proxy, "text/plain;charset=utf-8");
    wl_data_source_offer(source->proxy, "UTF8_STRING");
    wl_data_source_offer(source->proxy, "text/plain");
    wl_data_device_set_selection(system->data_device, source->proxy,
        system->input_serial);
    /* Retire the old source even if its cancelled event is still queued.
     * Any outgoing transfer already owns a separate text snapshot. */
    clipboard_source_destroy(system);
    system->clipboard_source_state = source;
    wl_display_flush(system->display);
    _al_mutex_unlock(&system->lock);
    return true;
}

static bool wl_has_clipboard_text(ALLEGRO_DISPLAY *display)
{
    ALLEGRO_SYSTEM_WAYLAND *system =
        (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    ALLEGRO_WL_CLIPBOARD_OFFER *offer;
    bool result = false;
    (void)display;

    if (!system)
        return false;
    _al_mutex_lock(&system->lock);
    if (system->clipboard_source_state) {
        result = true;
    }
    else {
        offer = system->clipboard_offer;
        result = offer && clipboard_offer_get_mime(offer) != NULL;
    }
    _al_mutex_unlock(&system->lock);
    return result;
}

void _al_wl_clipboard_add_functions(ALLEGRO_DISPLAY_INTERFACE *vt)
{
    vt->get_clipboard_text = wl_get_clipboard_text;
    vt->set_clipboard_text = wl_set_clipboard_text;
    vt->has_clipboard_text = wl_has_clipboard_text;
}
