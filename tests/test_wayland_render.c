/* Requires a running Wayland compositor. Scale checks inject renderer
 * metadata only; they don't change the desktop's output configuration. */
#include <stdio.h>
#include <stdlib.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_wldisplay.h"
#include "allegro5/internal/aintern_wlsystem.h"

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); \
} } while (0)

static int scaled(int value, uint32_t scale)
{
    return (int)(((uint64_t)value * scale + 60) / 120);
}

static void test_scale(ALLEGRO_DISPLAY *display)
{
    const uint32_t scales[] = {120, 150, 240};
    uint32_t saved = __sync_val_compare_and_swap(
        &display->ogl_extras->drawable_scale_120, 0, 0);
    ALLEGRO_BITMAP *texture = al_create_bitmap(32, 32);
    ALLEGRO_BITMAP *subtexture;
    ALLEGRO_BITMAP *subbackbuffer;
    size_t i;
    CHECK(texture);
    subtexture = al_create_sub_bitmap(texture, 3, 5, 11, 13);
    subbackbuffer = al_create_sub_bitmap(al_get_backbuffer(display),
        3, 5, 11, 13);
    CHECK(subtexture && subbackbuffer);
    for (i = 0; i < sizeof scales / sizeof scales[0]; i++) {
        GLint v[4];
        __sync_lock_test_and_set(&display->ogl_extras->drawable_scale_120,
            scales[i]);
        al_set_target_bitmap(texture);
        glGetIntegerv(GL_VIEWPORT, v);
        CHECK(v[0] == 0 && v[1] == 0 && v[2] == 32 && v[3] == 32);
        al_set_target_bitmap(subtexture);
        glGetIntegerv(GL_VIEWPORT, v);
        CHECK(v[0] == 3 && v[1] == 14 && v[2] == 11 && v[3] == 13);
        al_set_target_backbuffer(display);
        al_set_clipping_rectangle(10, 10, 20, 20);
        glGetIntegerv(GL_SCISSOR_BOX, v);
        CHECK(v[0] == scaled(10, scales[i]));
        CHECK(v[1] == scaled(34, scales[i]));
        CHECK(v[2] == scaled(30, scales[i]) - scaled(10, scales[i]));
        CHECK(v[3] == scaled(54, scales[i]) - scaled(34, scales[i]));
        al_set_target_bitmap(subbackbuffer);
        glGetIntegerv(GL_VIEWPORT, v);
        CHECK(v[0] == scaled(3, scales[i]));
        CHECK(v[1] == scaled(46, scales[i]));
        CHECK(v[2] == scaled(14, scales[i]) - scaled(3, scales[i]));
        CHECK(v[3] == scaled(59, scales[i]) - scaled(46, scales[i]));
    }
    __sync_lock_test_and_set(&display->ogl_extras->drawable_scale_120, saved);
    al_set_target_backbuffer(display);
    al_reset_clipping_rectangle();
    al_destroy_bitmap(subbackbuffer);
    al_destroy_bitmap(subtexture);
    al_destroy_bitmap(texture);
}

static void test_backbuffer_readback(ALLEGRO_DISPLAY *display, uint32_t scale)
{
    ALLEGRO_SYSTEM_WAYLAND *system = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    ALLEGRO_DISPLAY_WAYLAND *d = (ALLEGRO_DISPLAY_WAYLAND *)display;
    ALLEGRO_BITMAP *backbuffer = al_get_backbuffer(display);
    ALLEGRO_BITMAP *copy = al_create_bitmap(32, 32);
    uint32_t saved;
    unsigned char r, g, b;
    unsigned char pixels[16];
    ALLEGRO_LOCKED_REGION *lock;
    CHECK(copy);
    if (scale % 120 != 0 && !d->use_fractional_scale) {
        al_destroy_bitmap(copy);
        return;
    }

    /* Map at the compositor's native scale before injecting a larger
     * drawable, so initial surface-enter events cannot undo the test. */
    al_set_target_backbuffer(display);
    al_flip_display();
    al_rest(0.2);
    saved = __sync_val_compare_and_swap(
        &display->ogl_extras->drawable_scale_120, 0, 0);
    _al_mutex_lock(&system->lock);
    if (wl_proxy_get_version((struct wl_proxy *)d->surface) >= 3)
        wl_surface_set_buffer_scale(d->surface,
            d->use_fractional_scale ? 1 : (int)(scale / 120));
    wl_egl_window_resize(d->egl_window, scaled(96, scale), scaled(64, scale), 0, 0);
    __sync_lock_test_and_set(&display->ogl_extras->drawable_scale_120, scale);
    _al_mutex_unlock(&system->lock);
    CHECK(eglSwapBuffers(system->egl_display, d->egl_surface));
    display->vt->update_transformation(display, backbuffer);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, scaled(32, scale), scaled(48, scale), scaled(32, scale));
    glClearColor(1, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    al_unmap_rgb(al_get_pixel(backbuffer, 24, 24), &r, &g, &b);
    CHECK(r == 255 && g == 0 && b == 0);
    al_set_target_bitmap(copy);
    al_draw_bitmap_region(backbuffer, 0, 0, 32, 32, 0, 0, 0);
    al_unmap_rgb(al_get_pixel(copy, 24, 24), &r, &g, &b);
    CHECK(r == 255 && g == 0 && b == 0);

    al_set_target_backbuffer(display);
    al_reset_clipping_rectangle();
    lock = al_lock_bitmap_region(backbuffer, 10, 12, 1, 1,
        ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);
    CHECK(lock);
    ((unsigned char *)lock->data)[0] = 0;
    ((unsigned char *)lock->data)[1] = 255;
    ((unsigned char *)lock->data)[2] = 0;
    ((unsigned char *)lock->data)[3] = 255;
    al_unlock_bitmap(backbuffer);
    {
        int x = scaled(10, scale), y = scaled(51, scale);
        int w = scaled(11, scale) - x, h = scaled(52, scale) - y;
        int last = (w * h - 1) * 4;
        glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        CHECK(pixels[0] == 0 && pixels[1] == 255 && pixels[2] == 0);
        CHECK(pixels[last] == 0 && pixels[last + 1] == 255 && pixels[last + 2] == 0);
    }

    _al_mutex_lock(&system->lock);
    if (wl_proxy_get_version((struct wl_proxy *)d->surface) >= 3)
        wl_surface_set_buffer_scale(d->surface,
            d->use_fractional_scale ? 1 : (int)((saved + 60) / 120));
    wl_egl_window_resize(d->egl_window, scaled(96, saved), scaled(64, saved), 0, 0);
    __sync_lock_test_and_set(&display->ogl_extras->drawable_scale_120, saved);
    _al_mutex_unlock(&system->lock);
    CHECK(eglSwapBuffers(system->egl_display, d->egl_surface));
    al_set_target_backbuffer(display);
    al_destroy_bitmap(copy);
}

static void check_color(ALLEGRO_BITMAP *bitmap)
{
    unsigned char r, g, b;
    ALLEGRO_LOCKED_REGION *lock = al_lock_bitmap(bitmap,
        ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);
    CHECK(lock);
    al_unmap_rgb(al_get_pixel(bitmap, 8, 8), &r, &g, &b);
    CHECK(r == 200 && g == 50 && b == 10);
    al_unlock_bitmap(bitmap);
}

static int run_tests(int flags)
{
    ALLEGRO_DISPLAY *first, *second;
    ALLEGRO_BITMAP *bitmap;
    CHECK(al_init());
    if (al_get_system_id() != ALLEGRO_SYSTEM_ID_WAYLAND) {
        fprintf(stderr, "This test requires a Wayland compositor.\n");
        al_uninstall_system();
        return 77;
    }
    al_set_new_display_flags(ALLEGRO_OPENGL | ALLEGRO_FRAMELESS | flags);
    first = al_create_display(96, 64);
    CHECK(first);
    test_scale(first);
    test_backbuffer_readback(first, 150);
    test_backbuffer_readback(first, 240);

    bitmap = al_create_bitmap(32, 32);
    CHECK(bitmap);
    al_set_target_bitmap(bitmap);
    al_clear_to_color(al_map_rgb(200, 50, 10));
    CHECK(al_get_opengl_fbo(bitmap) != 0);
    second = al_create_display(96, 64);
    CHECK(second);
    al_set_target_backbuffer(second);
    al_destroy_display(first);
    CHECK(al_get_current_display() == second);
    CHECK(_al_get_bitmap_display(bitmap) == second);
    CHECK(((ALLEGRO_BITMAP_EXTRA_OPENGL *)bitmap->extra)->fbo_info == NULL);
    check_color(bitmap);
    /* Recreating an FBO in the surviving context must also work. */
    al_set_target_bitmap(bitmap);
    al_clear_to_color(al_map_rgb(200, 50, 10));
    al_set_target_backbuffer(second);
    al_destroy_display(second);
    CHECK(al_get_bitmap_flags(bitmap) & ALLEGRO_MEMORY_BITMAP);
    CHECK(_al_get_bitmap_display(bitmap) == NULL);
    check_color(bitmap);
    al_destroy_bitmap(bitmap);
    al_uninstall_system();
    return 0;
}

int main(void)
{
    int result = run_tests(0);
    if (result)
        return result;
    result = run_tests(ALLEGRO_PROGRAMMABLE_PIPELINE);
    if (!result)
        puts("Wayland render and bitmap lifetime tests passed (fixed and programmable).");
    return result;
}
