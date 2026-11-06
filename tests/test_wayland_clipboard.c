/* Headless tests of clipboard transfer ownership and wheel-frame handling.
 * Include the implementation to exercise private protocol callbacks without
 * accessing or changing the desktop clipboard. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/wayland/wlclipboard.c"
#include "allegro5/internal/aintern_wlscroll.h"

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); \
} } while (0)

static void test_mime(void)
{
    const char *mimes[] = { "STRING", "TEXT", "text/plain", "UTF8_STRING",
        "text/plain;charset=utf-8" };
    size_t i;
    for (i = 0; i < sizeof mimes / sizeof mimes[0]; i++) {
        ALLEGRO_WL_CLIPBOARD_OFFER offer = {0};
        clipboard_offer_mime(&offer, NULL, mimes[i]);
        CHECK(strcmp(clipboard_offer_get_mime(&offer), mimes[i]) == 0);
    }
    {
        ALLEGRO_WL_CLIPBOARD_OFFER offer = {0};
        char *text = _al_strdup("caf\xe9");
        clipboard_offer_mime(&offer, NULL, "application/octet-stream");
        CHECK(clipboard_offer_get_mime(&offer) == NULL);
        text = clipboard_decode_text(text, 4, "STRING");
        CHECK(text && strcmp(text, "caf\xc3\xa9") == 0);
        al_free(text);
    }
}

static void test_transfer(bool stalled)
{
    ALLEGRO_SYSTEM_WAYLAND system = {0};
    ALLEGRO_WL_CLIPBOARD_SOURCE source = {0};
    const size_t size = 1024 * 1024;
    int fds[2];
    double start;
    CHECK(pipe(fds) == 0);
    source.system = &system;
    source.text = al_malloc(size + 1);
    CHECK(source.text);
    memset(source.text, 'x', size);
    source.text[size] = '\0';

    start = clipboard_monotonic_time();
    clipboard_source_send(&source, NULL, "text/plain;charset=utf-8", fds[1]);
    CHECK(clipboard_monotonic_time() - start < 0.5);
    CHECK(system.clipboard_transfers);
    /* Simulate selection replacement/cancellation while a transfer is live. */
    al_free(source.text);

    if (!stalled) {
        char buffer[4096];
        size_t received = 0;
        for (;;) {
            struct pollfd pfd = { fds[0], POLLIN | POLLHUP, 0 };
            ssize_t n;
            size_t i;
            CHECK(poll(&pfd, 1, 3000) > 0);
            n = read(fds[0], buffer, sizeof buffer);
            CHECK(n >= 0);
            if (!n)
                break;
            for (i = 0; i < (size_t)n; i++)
                CHECK(buffer[i] == 'x');
            received += (size_t)n;
        }
        CHECK(received == size);
    }
    start = clipboard_monotonic_time();
    _al_wl_clipboard_shutdown(&system);
    CHECK(clipboard_monotonic_time() - start < 0.5);
    CHECK(system.clipboard_transfers == NULL);
    close(fds[0]);
}

static void test_transfer_timeout(void)
{
    ALLEGRO_SYSTEM_WAYLAND system = {0};
    ALLEGRO_WL_CLIPBOARD_SOURCE source = {0};
    int fds[2];
    double deadline;
    CHECK(pipe(fds) == 0);
    source.system = &system;
    source.text = al_malloc(1024 * 1024 + 1);
    CHECK(source.text);
    memset(source.text, 'x', 1024 * 1024);
    source.text[1024 * 1024] = '\0';
    clipboard_source_send(&source, NULL, "text/plain", fds[1]);
    CHECK(system.clipboard_transfers);
    deadline = clipboard_monotonic_time() + WL_CLIPBOARD_TIMEOUT / 1000.0 + 1.0;
    while (!__sync_val_compare_and_swap(&system.clipboard_transfers->done, 0, 0)) {
        struct timespec pause = {0, 10000000};
        CHECK(clipboard_monotonic_time() < deadline);
        nanosleep(&pause, NULL);
    }
    _al_wl_clipboard_reap_transfers(&system);
    CHECK(system.clipboard_transfers == NULL);
    close(fds[0]);
    al_free(source.text);
}

static void test_transfer_limit(void)
{
    ALLEGRO_SYSTEM_WAYLAND system = {0};
    ALLEGRO_WL_CLIPBOARD_SOURCE source = {0};
    int reads[WL_CLIPBOARD_MAX_TRANSFERS + 1];
    int i;
    source.system = &system;
    source.text = al_malloc(1024 * 1024 + 1);
    CHECK(source.text);
    memset(source.text, 'x', 1024 * 1024);
    source.text[1024 * 1024] = '\0';
    for (i = 0; i <= WL_CLIPBOARD_MAX_TRANSFERS; i++) {
        int fds[2];
        CHECK(pipe(fds) == 0);
        reads[i] = fds[0];
        clipboard_source_send(&source, NULL, "text/plain", fds[1]);
    }
    {
        char c;
        CHECK(read(reads[WL_CLIPBOARD_MAX_TRANSFERS], &c, 1) == 0);
    }
    _al_wl_clipboard_shutdown(&system);
    for (i = 0; i <= WL_CLIPBOARD_MAX_TRANSFERS; i++)
        close(reads[i]);
    al_free(source.text);
}

static void test_broken_pipe(void)
{
    ALLEGRO_SYSTEM_WAYLAND system = {0};
    ALLEGRO_WL_CLIPBOARD_SOURCE source = {0};
    int fds[2];
    CHECK(pipe(fds) == 0);
    source.system = &system;
    source.text = "clipboard text";
    close(fds[0]);
    clipboard_source_send(&source, NULL, "text/plain", fds[1]);
    _al_wl_clipboard_shutdown(&system);
}

static void test_scroll(void)
{
    ALLEGRO_WL_SCROLL_AXIS axis = {0};
    int i;
    axis.continuous = 10;
    axis.discrete = 1;
    axis.have_discrete = true;
    CHECK(_al_wl_scroll_flush(&axis, 1) == 1);
    CHECK(_al_wl_scroll_flush(&axis, 1) == 0);
    for (i = 0; i < 4; i++) {
        axis.continuous = 2.5;
        axis.value120 = 30;
        axis.have_value120 = true;
        CHECK(_al_wl_scroll_flush(&axis, 1) == (i == 3 ? 1 : 0));
    }
    for (i = 0; i < 4; i++) {
        axis.continuous = -2.5;
        axis.value120 = -30;
        axis.have_value120 = true;
        CHECK(_al_wl_scroll_flush(&axis, 1) == (i == 3 ? -1 : 0));
    }
    for (i = 0; i < 4; i++) {
        axis.continuous = 2.5;
        CHECK(_al_wl_scroll_flush(&axis, 1) == (i == 3 ? 1 : 0));
    }
    axis.value120 = 30;
    axis.have_value120 = true;
    CHECK(_al_wl_scroll_flush(&axis, 4) == 1);
}

int main(void)
{
    test_mime();
    test_transfer(false);
    test_transfer(true);
    test_transfer_timeout();
    test_transfer_limit();
    test_broken_pipe();
    test_scroll();
    puts("Wayland clipboard and scroll tests passed.");
    return 0;
}
