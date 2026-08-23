#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "allegro5/allegro.h"
#include "allegro5/keyboard.h"
#include "allegro5/mouse.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/internal/aintern_mouse.h"
#include "allegro5/internal/aintern_wl.h"
#include "allegro5/internal/aintern_wldisplay.h"
#include "allegro5/internal/aintern_wlinput.h"
#include "allegro5/internal/aintern_wlsystem.h"
#include "allegro5/platform/cursor-shape-client-protocol.h"
#include "allegro5/platform/pointer-constraints-client-protocol.h"

ALLEGRO_DEBUG_CHANNEL("wlinput")

/* Sentinel for "unable to map to an Allegro key". */
#define ALLEGRO_KEY_NONE 0

typedef struct ALLEGRO_KEYBOARD_WAYLAND {
    ALLEGRO_KEYBOARD parent;
    ALLEGRO_KEYBOARD_STATE state;

    struct wl_keyboard *wl_keyboard;
    /* The context itself lives in the system struct; it must not be looked
     * up via al_get_system_driver() from protocol handlers, because they
     * can run during system initialisation when that is not valid yet. */
    struct xkb_context *xkb_context;
    struct xkb_keymap *keymap;
    struct xkb_state *xkb_state;

    /* key repeat */
    int repeat_rate;             /* chars per second, 0 = disabled */
    int repeat_delay;            /* ms before the first repeat */
    int repeat_key;
    uint32_t repeat_unichar;
    double repeat_time;          /* next repeat time, from al_get_time */

    bool installed;

    ALLEGRO_DISPLAY *display;    /* display with keyboard focus */
} ALLEGRO_KEYBOARD_WAYLAND;

typedef struct ALLEGRO_MOUSE_WAYLAND {
    ALLEGRO_MOUSE parent;
    ALLEGRO_MOUSE_STATE state;

    struct wl_pointer *wl_pointer;
    struct wp_cursor_shape_device_v1 *cursor_shape;
    ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id;
    uint32_t cursor_serial;
    ALLEGRO_MOUSE_CURSOR_WAYLAND *custom_cursor;
    bool cursor_hidden;
    bool installed;

    ALLEGRO_DISPLAY *display;    /* display the pointer is over */
} ALLEGRO_MOUSE_WAYLAND;

static ALLEGRO_KEYBOARD_WAYLAND the_keyboard;
static ALLEGRO_MOUSE_WAYLAND the_mouse;

static void wl_keyboard_handle_keymap(void *data, struct wl_keyboard *wl_keyboard,
    uint32_t format, int32_t fd, uint32_t size);


/* Map a surface to the display it belongs to.  Only the content surfaces of
 * our displays are recognised; libdecor's decoration surfaces are ignored.
 * Must be called with the system lock held (which the event thread is when
 * these handlers run). */
static ALLEGRO_DISPLAY *surface_to_display(struct wl_surface *surface)
{
    ALLEGRO_SYSTEM_WAYLAND *s = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    int i;

    if (!surface)
        return NULL;

    for (i = 0; i < (int)_al_vector_size(&s->system.displays); i++) {
        ALLEGRO_DISPLAY_WAYLAND **dptr;
        dptr = _al_vector_ref(&s->system.displays, i);
        if ((*dptr)->surface == surface)
            return (ALLEGRO_DISPLAY *)*dptr;
    }
    return NULL;
}


/*-------------------------------------------------------------------------*/
/* Keysym to ALLEGRO_KEY translation, same table as the X11 backend (the
 * keysym namespace is shared between X11 and xkbcommon). */

typedef xkb_keysym_t KeySym; /* X11 name used by the shared table */

#include "keysym_table.inc"


static int keysym_to_allegro(xkb_keysym_t sym)
{
    int i;

    for (i = 0; i < (int)(sizeof(translation_table) / sizeof(translation_table[0])); i++) {
        if (translation_table[i].keysym == (KeySym)sym)
            return translation_table[i].allegro_key;
    }
    return ALLEGRO_KEY_NONE;
}


/*-------------------------------------------------------------------------*/
/* Keyboard */

/* Modifier state from xkb (mod indices follow the standard X11 layout). */
static unsigned int xkb_mods_to_allegro(struct xkb_state *state)
{
    xkb_mod_mask_t mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
    unsigned int m = 0;

    if (mods & (1u << 0)) m |= ALLEGRO_KEYMOD_SHIFT;
    if (mods & (1u << 1)) m |= ALLEGRO_KEYMOD_CAPSLOCK;
    if (mods & (1u << 2)) m |= ALLEGRO_KEYMOD_CTRL;
    if (mods & (1u << 3)) m |= ALLEGRO_KEYMOD_ALT;
    if (mods & (1u << 4)) m |= ALLEGRO_KEYMOD_NUMLOCK;
    if (mods & (1u << 5)) m |= ALLEGRO_KEYMOD_SCROLLLOCK;
    if (mods & (1u << 6)) m |= ALLEGRO_KEYMOD_LWIN | ALLEGRO_KEYMOD_RWIN;
    if (mods & (1u << 7)) m |= ALLEGRO_KEYMOD_ALTGR;

    return m;
}


static void wl_keyboard_handle_keymap(void *data, struct wl_keyboard *wl_keyboard,
    uint32_t format, int32_t fd, uint32_t size)
{
    ALLEGRO_KEYBOARD_WAYLAND *kbd = data;
    const char *map;

    (void)wl_keyboard;

    if (format != XKB_KEYMAP_FORMAT_TEXT_V1) {
        ALLEGRO_WARN("wlinput: unsupported keymap format %u\n", format);
        close(fd);
        return;
    }

    /* Map the keymap file (the canonical way to consume it). */
    map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        ALLEGRO_ERROR("wlinput: failed to mmap keymap: %s\n", strerror(errno));
        close(fd);
        return;
    }

    if (kbd->keymap)
        xkb_keymap_unref(kbd->keymap);
    if (kbd->xkb_state)
        xkb_state_unref(kbd->xkb_state);

    kbd->keymap = xkb_keymap_new_from_buffer(kbd->xkb_context, map, size,
        XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

    munmap((void *)map, size);
    close(fd);

    if (!kbd->keymap) {
        ALLEGRO_ERROR("wlinput: failed to compile keymap\n");
        return;
    }
    kbd->xkb_state = xkb_state_new(kbd->keymap);

    ALLEGRO_INFO("wlinput: keymap loaded\n");
}


static void wl_keyboard_handle_enter(void *data, struct wl_keyboard *wl_keyboard,
    uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
    ALLEGRO_KEYBOARD_WAYLAND *kbd = data;
    (void)wl_keyboard;
    (void)serial;
    (void)keys;

    kbd->display = surface_to_display(surface);
    if (kbd->display)
        kbd->state.display = kbd->display;
}


static void wl_keyboard_handle_leave(void *data, struct wl_keyboard *wl_keyboard,
    uint32_t serial, struct wl_surface *surface)
{
    ALLEGRO_KEYBOARD_WAYLAND *kbd = data;
    (void)wl_keyboard;
    (void)serial;
    (void)surface;

    kbd->display = NULL;
    kbd->state.display = NULL;

    /* Any held keys are gone once focus is lost. */
    _al_event_source_lock(&kbd->parent.es);
    memset(&kbd->state.__key_down__internal__, 0,
        sizeof kbd->state.__key_down__internal__);
    _al_event_source_unlock(&kbd->parent.es);

    kbd->repeat_key = ALLEGRO_KEY_NONE;
    kbd->repeat_time = 0;
}


static void wl_keyboard_handle_key(void *data, struct wl_keyboard *wl_keyboard,
    uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    ALLEGRO_KEYBOARD_WAYLAND *kbd = data;
    bool down = (state == WL_KEYBOARD_KEY_STATE_PRESSED);
    xkb_keysym_t sym;
    uint32_t utf32;
    int mycode;
    unsigned int modifiers;
    bool is_repeat;

    (void)wl_keyboard;
    (void)serial;
    (void)time;

    /* key is a hardware keycode, offset by 8 like X11. */
    key += 8;

    if (!kbd->xkb_state)
        return;

    xkb_state_update_key(kbd->xkb_state, key,
        down ? XKB_KEY_DOWN : XKB_KEY_UP);

    sym = xkb_state_key_get_one_sym(kbd->xkb_state, key);
    utf32 = xkb_keysym_to_utf32(sym);

    mycode = keysym_to_allegro(sym);
    modifiers = xkb_mods_to_allegro(kbd->xkb_state);

    _al_event_source_lock(&kbd->parent.es);

    if (down) {
        is_repeat = (mycode == kbd->repeat_key);

        if (mycode > 0)
            _AL_KEYBOARD_STATE_SET_KEY_DOWN(kbd->state, mycode);

        if (_al_event_source_needs_to_generate_event(&kbd->parent.es)) {
            ALLEGRO_EVENT event;
            event.keyboard.type = ALLEGRO_EVENT_KEY_DOWN;
            event.keyboard.timestamp = al_get_time();
            event.keyboard.display = kbd->display;
            event.keyboard.keycode = mycode;
            event.keyboard.unichar = 0;
            event.keyboard.modifiers = modifiers;
            event.keyboard.repeat = false;

            if (mycode > 0 && !is_repeat)
                _al_event_source_emit_event(&kbd->parent.es, &event);

            if (mycode < ALLEGRO_KEY_MODIFIERS) {
                event.keyboard.type = ALLEGRO_EVENT_KEY_CHAR;
                event.keyboard.unichar = utf32;
                event.keyboard.modifiers = modifiers;
                event.keyboard.repeat = is_repeat;
                _al_event_source_emit_event(&kbd->parent.es, &event);
            }
        }

        /* Arm key repeat (only for keys producing characters). */
        if (mycode > 0 && mycode < ALLEGRO_KEY_MODIFIERS && utf32 != 0
            && kbd->repeat_rate > 0) {
            kbd->repeat_key = mycode;
            kbd->repeat_unichar = utf32;
            kbd->repeat_time = al_get_time()
                + kbd->repeat_delay / 1000.0;
        }
        else if (!is_repeat) {
            kbd->repeat_key = ALLEGRO_KEY_NONE;
            kbd->repeat_time = 0;
        }
    }
    else {
        if (mycode > 0)
            _AL_KEYBOARD_STATE_CLEAR_KEY_DOWN(kbd->state, mycode);

        if (mycode == kbd->repeat_key) {
            kbd->repeat_key = ALLEGRO_KEY_NONE;
            kbd->repeat_time = 0;
        }

        if (_al_event_source_needs_to_generate_event(&kbd->parent.es)) {
            ALLEGRO_EVENT event;
            event.keyboard.type = ALLEGRO_EVENT_KEY_UP;
            event.keyboard.timestamp = al_get_time();
            event.keyboard.display = kbd->display;
            event.keyboard.keycode = mycode;
            event.keyboard.unichar = 0;
            event.keyboard.modifiers = modifiers;
            event.keyboard.repeat = false;
            if (mycode > 0)
                _al_event_source_emit_event(&kbd->parent.es, &event);
        }
    }

    _al_event_source_unlock(&kbd->parent.es);
}


static void wl_keyboard_handle_modifiers(void *data, struct wl_keyboard *wl_keyboard,
    uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched,
    uint32_t mods_locked, uint32_t group)
{
    ALLEGRO_KEYBOARD_WAYLAND *kbd = data;
    (void)wl_keyboard;
    (void)serial;

    if (!kbd->xkb_state)
        return;

    xkb_state_update_mask(kbd->xkb_state, mods_depressed, mods_latched,
        mods_locked, group, 0, 0);
}


static void wl_keyboard_handle_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
    int32_t rate, int32_t delay)
{
    ALLEGRO_KEYBOARD_WAYLAND *kbd = data;
    (void)wl_keyboard;

    kbd->repeat_rate = rate > 0 ? rate : 0;
    kbd->repeat_delay = delay > 0 ? delay : 0;
}


static const struct wl_keyboard_listener keyboard_listener = {
    .keymap = wl_keyboard_handle_keymap,
    .enter = wl_keyboard_handle_enter,
    .leave = wl_keyboard_handle_leave,
    .key = wl_keyboard_handle_key,
    .modifiers = wl_keyboard_handle_modifiers,
    .repeat_info = wl_keyboard_handle_repeat_info,
};


/* Emit KEY_CHAR repeats for a held key.  Called from the event thread's
 * polling loop, which also receives the actual key events. */
void _al_wl_keyboard_repeat_tick(void)
{
    ALLEGRO_KEYBOARD_WAYLAND *kbd = &the_keyboard;

    if (!kbd->installed || !kbd->repeat_key || kbd->repeat_rate <= 0)
        return;

    double now = al_get_time();
    if (now < kbd->repeat_time)
        return;

    _al_event_source_lock(&kbd->parent.es);
    if (_al_event_source_needs_to_generate_event(&kbd->parent.es)) {
        ALLEGRO_EVENT event;
        event.keyboard.type = ALLEGRO_EVENT_KEY_CHAR;
        event.keyboard.timestamp = now;
        event.keyboard.display = kbd->display;
        event.keyboard.keycode = kbd->repeat_key;
        event.keyboard.unichar = kbd->repeat_unichar;
        event.keyboard.modifiers = 0;
        event.keyboard.repeat = true;
        _al_event_source_emit_event(&kbd->parent.es, &event);
    }
    _al_event_source_unlock(&kbd->parent.es);

    kbd->repeat_time = now + 1.0 / kbd->repeat_rate;
}


/*-------------------------------------------------------------------------*/
/* Pointer/mouse */

static bool wl_cursor_shape_for_id(ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id,
    uint32_t *shape)
{
    switch (cursor_id) {
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT:
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_ARROW:
        *shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT;
        break;
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_BUSY:
        *shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_WAIT;
        break;
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_QUESTION:
        *shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_HELP;
        break;
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_EDIT:
        *shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_TEXT;
        break;
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_MOVE:
        *shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_MOVE;
        break;
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_N:
        *shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_N_RESIZE;
        break;
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_W:
        *shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_W_RESIZE;
        break;
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_S:
        *shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_S_RESIZE;
        break;
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_E:
        *shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_E_RESIZE;
        break;
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_NW:
        *shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NW_RESIZE;
        break;
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_SW:
        *shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_SW_RESIZE;
        break;
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_SE:
        *shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_SE_RESIZE;
        break;
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_NE:
        *shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NE_RESIZE;
        break;
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_PROGRESS:
        *shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_PROGRESS;
        break;
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_PRECISION:
        *shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_CROSSHAIR;
        break;
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_LINK:
        *shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER;
        break;
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_ALT_SELECT:
        *shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_GRAB;
        break;
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_UNAVAILABLE:
        *shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NOT_ALLOWED;
        break;
    case ALLEGRO_SYSTEM_MOUSE_CURSOR_NONE:
    default:
        return false;
    }
    return true;
}

static void wl_mouse_apply_cursor_shape(ALLEGRO_MOUSE_WAYLAND *mouse,
    uint32_t serial)
{
    uint32_t shape;

    if (mouse->cursor_shape
        && wl_cursor_shape_for_id(mouse->cursor_id, &shape)) {
        wp_cursor_shape_device_v1_set_shape(mouse->cursor_shape, serial,
            shape);
    }
}

static bool wl_mouse_apply_cursor(ALLEGRO_MOUSE_WAYLAND *mouse,
    uint32_t serial)
{
    if (!mouse->wl_pointer)
        return false;

    if (mouse->cursor_hidden) {
        if (serial)
            wl_pointer_set_cursor(mouse->wl_pointer, serial, NULL, 0, 0);
        return true;
    }

    if (mouse->custom_cursor) {
        if (serial)
            wl_pointer_set_cursor(mouse->wl_pointer, serial,
                mouse->custom_cursor->surface,
                mouse->custom_cursor->x_focus, mouse->custom_cursor->y_focus);
        return true;
    }
    if (mouse->cursor_shape) {
        if (serial)
            wl_mouse_apply_cursor_shape(mouse, serial);
        return true;
    }

    /* Without either a custom cursor or wp_cursor_shape, there is no
     * portable cursor image to restore after a hide request. */
    return false;
}

static void wl_pointer_handle_enter(void *data, struct wl_pointer *wl_pointer,
    uint32_t serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy)
{
    ALLEGRO_MOUSE_WAYLAND *mouse = data;
    ALLEGRO_DISPLAY *display;
    (void)wl_pointer;
    (void)serial;

    display = surface_to_display(surface);
    if (!display)
        return;

    /* The enter serial is required by wp_cursor_shape_device_v1.  Reapply
     * the application's requested shape because the pointer may have just
     * crossed from a libdecor decoration surface, which can use its own
     * resize cursor. */
    mouse->cursor_serial = serial;
    if (mouse->cursor_id == ALLEGRO_SYSTEM_MOUSE_CURSOR_NONE)
        mouse->cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT;
    wl_mouse_apply_cursor(mouse, serial);

    mouse->display = display;
    mouse->state.display = display;
    mouse->state.x = wl_fixed_to_int(sx);
    mouse->state.y = wl_fixed_to_int(sy);

    _al_event_source_lock(&mouse->parent.es);
    if (_al_event_source_needs_to_generate_event(&mouse->parent.es)) {
        ALLEGRO_EVENT event;
        event.mouse.type = ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY;
        event.mouse.timestamp = al_get_time();
        event.mouse.display = display;
        event.mouse.x = mouse->state.x;
        event.mouse.y = mouse->state.y;
        event.mouse.z = mouse->state.z;
        event.mouse.w = mouse->state.w;
        event.mouse.button = 0;
        event.mouse.pressure = 0.0;
        _al_event_source_emit_event(&mouse->parent.es, &event);
    }
    _al_event_source_unlock(&mouse->parent.es);
}


static void wl_pointer_handle_leave(void *data, struct wl_pointer *wl_pointer,
    uint32_t serial, struct wl_surface *surface)
{
    ALLEGRO_MOUSE_WAYLAND *mouse = data;
    ALLEGRO_DISPLAY *display = mouse->display;
    (void)wl_pointer;
    (void)serial;
    (void)surface;

    if (!display)
        return;

    mouse->display = NULL;
    mouse->state.display = NULL;
    mouse->state.buttons = 0;

    _al_event_source_lock(&mouse->parent.es);
    if (_al_event_source_needs_to_generate_event(&mouse->parent.es)) {
        ALLEGRO_EVENT event;
        event.mouse.type = ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY;
        event.mouse.timestamp = al_get_time();
        event.mouse.display = display;
        event.mouse.x = mouse->state.x;
        event.mouse.y = mouse->state.y;
        event.mouse.button = 0;
        event.mouse.pressure = 0.0;
        _al_event_source_emit_event(&mouse->parent.es, &event);
    }
    _al_event_source_unlock(&mouse->parent.es);
}


static void wl_pointer_handle_motion(void *data, struct wl_pointer *wl_pointer,
    uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
    ALLEGRO_MOUSE_WAYLAND *mouse = data;
    int x = wl_fixed_to_int(sx);
    int y = wl_fixed_to_int(sy);
    (void)wl_pointer;
    (void)time;

    if (!mouse->display)
        return;

    int dx = x - mouse->state.x;
    int dy = y - mouse->state.y;
    mouse->state.x = x;
    mouse->state.y = y;

    _al_event_source_lock(&mouse->parent.es);
    if (_al_event_source_needs_to_generate_event(&mouse->parent.es)) {
        ALLEGRO_EVENT event;
        event.mouse.type = ALLEGRO_EVENT_MOUSE_AXES;
        event.mouse.timestamp = al_get_time();
        event.mouse.display = mouse->display;
        event.mouse.x = x;
        event.mouse.y = y;
        event.mouse.z = mouse->state.z;
        event.mouse.w = mouse->state.w;
        event.mouse.dx = dx;
        event.mouse.dy = dy;
        event.mouse.dz = 0;
        event.mouse.dw = 0;
        event.mouse.button = 0;
        event.mouse.pressure = mouse->state.buttons ? 1.0 : 0.0;
        _al_event_source_emit_event(&mouse->parent.es, &event);
    }
    _al_event_source_unlock(&mouse->parent.es);
}


static int wl_button_to_allegro(uint32_t button)
{
    /* Linux evdev button codes: 0x110 left, 0x111 right, 0x112 middle. */
    switch (button) {
    case 0x110: return 1;
    case 0x111: return 2;
    case 0x112: return 3;
    default:    return button > 0x112 ? button - 0x10f : 0;
    }
}


static void emit_mouse_event(unsigned int type, unsigned int button,
    int dx, int dy, int dz, int dw)
{
    ALLEGRO_MOUSE_WAYLAND *mouse = &the_mouse;

    if (!_al_event_source_needs_to_generate_event(&mouse->parent.es))
        return;

    ALLEGRO_EVENT event;
    event.mouse.type = type;
    event.mouse.timestamp = al_get_time();
    event.mouse.display = mouse->display;
    event.mouse.x = mouse->state.x;
    event.mouse.y = mouse->state.y;
    event.mouse.z = mouse->state.z;
    event.mouse.w = mouse->state.w;
    event.mouse.dx = dx;
    event.mouse.dy = dy;
    event.mouse.dz = dz;
    event.mouse.dw = dw;
    event.mouse.button = button;
    event.mouse.pressure = mouse->state.buttons ? 1.0 : 0.0;
    _al_event_source_emit_event(&mouse->parent.es, &event);
}


static void wl_pointer_handle_button(void *data, struct wl_pointer *wl_pointer,
    uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    ALLEGRO_MOUSE_WAYLAND *mouse = data;
    int al_button;
    bool down = (state == WL_POINTER_BUTTON_STATE_PRESSED);
    (void)wl_pointer;
    (void)serial;
    (void)time;

    if (!mouse->display)
        return;

    al_button = wl_button_to_allegro(button);
    if (al_button == 0)
        return;

    _al_event_source_lock(&mouse->parent.es);

    if (down)
        mouse->state.buttons |= (1u << (al_button - 1));
    else
        mouse->state.buttons &= ~(1u << (al_button - 1));

    emit_mouse_event(down ? ALLEGRO_EVENT_MOUSE_BUTTON_DOWN
                          : ALLEGRO_EVENT_MOUSE_BUTTON_UP,
        al_button, 0, 0, 0, 0);

    _al_event_source_unlock(&mouse->parent.es);
}


static void wl_pointer_handle_axis(void *data, struct wl_pointer *wl_pointer,
    uint32_t time, uint32_t axis, wl_fixed_t value)
{
    ALLEGRO_MOUSE_WAYLAND *mouse = data;
    int dz = 0, dw = 0;
    (void)wl_pointer;
    (void)time;

    if (!mouse->display)
        return;

    /* axis_discrete may not be sent; fall back to the continuous value,
     * for which libinput roughly uses 10.0 per notch. */
    int notches = (int)round(wl_fixed_to_double(value) / 10.0);
    if (notches == 0 && value != 0)
        notches = value > 0 ? 1 : -1;

    int precision = al_get_mouse_wheel_precision();
    if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
        dz = notches * precision;
    else if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
        dw = notches * precision;
    else
        return;

    _al_event_source_lock(&mouse->parent.es);
    mouse->state.z += dz;
    mouse->state.w += dw;
    emit_mouse_event(ALLEGRO_EVENT_MOUSE_AXES, 0, 0, 0, dz, dw);
    _al_event_source_unlock(&mouse->parent.es);
}


static void wl_pointer_handle_axis_discrete(void *data, struct wl_pointer *wl_pointer,
    uint32_t axis, int32_t discrete)
{
    ALLEGRO_MOUSE_WAYLAND *mouse = data;
    int dz = 0, dw = 0;
    (void)wl_pointer;

    if (!mouse->display)
        return;

    int precision = al_get_mouse_wheel_precision();
    if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
        dz = discrete * precision;
    else if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
        dw = discrete * precision;
    else
        return;

    _al_event_source_lock(&mouse->parent.es);
    mouse->state.z += dz;
    mouse->state.w += dw;
    emit_mouse_event(ALLEGRO_EVENT_MOUSE_AXES, 0, 0, 0, dz, dw);
    _al_event_source_unlock(&mouse->parent.es);
}


static void wl_pointer_handle_frame(void *data, struct wl_pointer *wl_pointer)
{
    (void)data;
    (void)wl_pointer;
    /* Events are emitted immediately, so there is nothing to batch up. */
}


static void wl_pointer_handle_axis_source(void *data, struct wl_pointer *wl_pointer,
    uint32_t axis_source)
{
    (void)data;
    (void)wl_pointer;
    (void)axis_source;
}


static void wl_pointer_handle_axis_stop(void *data, struct wl_pointer *wl_pointer,
    uint32_t time, uint32_t axis)
{
    (void)data;
    (void)wl_pointer;
    (void)time;
    (void)axis;
}


static void wl_pointer_handle_axis_value120(void *data, struct wl_pointer *wl_pointer,
    uint32_t axis, int32_t value120)
{
    ALLEGRO_MOUSE_WAYLAND *mouse = data;
    int dz = 0, dw = 0;
    (void)wl_pointer;

    if (!mouse->display)
        return;

    int notches = value120 / 120;
    int precision = al_get_mouse_wheel_precision();
    if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
        dz = notches * precision;
    else if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
        dw = notches * precision;
    else
        return;

    _al_event_source_lock(&mouse->parent.es);
    mouse->state.z += dz;
    mouse->state.w += dw;
    emit_mouse_event(ALLEGRO_EVENT_MOUSE_AXES, 0, 0, 0, dz, dw);
    _al_event_source_unlock(&mouse->parent.es);
}


static const struct wl_pointer_listener pointer_listener = {
    .enter = wl_pointer_handle_enter,
    .leave = wl_pointer_handle_leave,
    .motion = wl_pointer_handle_motion,
    .button = wl_pointer_handle_button,
    .axis = wl_pointer_handle_axis,
    .frame = wl_pointer_handle_frame,
    .axis_source = wl_pointer_handle_axis_source,
    .axis_stop = wl_pointer_handle_axis_stop,
    .axis_discrete = wl_pointer_handle_axis_discrete,
    .axis_value120 = wl_pointer_handle_axis_value120,
};


/* Set and remember a system cursor.  Wayland only changes a shape while
 * the pointer is focused on one of the client's surfaces, so remember the
 * request and apply it again from the next pointer-enter handler. */
bool _al_wl_set_system_mouse_cursor(ALLEGRO_DISPLAY *display,
    ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id)
{
    ALLEGRO_SYSTEM_WAYLAND *s =
        (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    uint32_t shape;
    bool supported;

    if (!wl_cursor_shape_for_id(cursor_id, &shape))
        return false;
    (void)shape;

    _al_mutex_lock(&s->lock);
    supported = s->cursor_shape_manager != NULL;
    if (supported) {
        the_mouse.cursor_id = cursor_id;
        the_mouse.custom_cursor = NULL;
        if (the_mouse.display == display)
            wl_mouse_apply_cursor(&the_mouse, the_mouse.cursor_serial);
    }
    _al_mutex_unlock(&s->lock);

    return supported;
}


/* Create an anonymous file suitable for wl_shm.  The Wayland compositor
 * opens this fd itself, so the file needs to be backed by shared memory. */
static int wl_create_shm_file(size_t size)
{
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    char *path;
    size_t path_size;
    int fd;

    if (!runtime_dir || !runtime_dir[0])
        runtime_dir = "/tmp";

    path_size = strlen(runtime_dir) + strlen("/allegro-cursor-XXXXXX") + 1;
    path = al_malloc(path_size);
    if (!path)
        return -1;

    snprintf(path, path_size, "%s/allegro-cursor-XXXXXX", runtime_dir);
    fd = mkstemp(path);
    unlink(path);
    al_free(path);
    if (fd < 0)
        return -1;

    if (ftruncate(fd, (off_t)size) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}


ALLEGRO_MOUSE_CURSOR *_al_wl_create_mouse_cursor(ALLEGRO_BITMAP *bmp,
    int x_focus, int y_focus)
{
    ALLEGRO_SYSTEM_WAYLAND *s =
        (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    ALLEGRO_MOUSE_CURSOR_WAYLAND *cursor;
    ALLEGRO_LOCKED_REGION *region = NULL;
    bool locked_here = false;
    int width, height, stride;
    size_t shm_size;
    int fd = -1;
    void *shm_data = MAP_FAILED;
    struct wl_shm_pool *pool = NULL;
    struct wl_surface *surface = NULL;
    struct wl_buffer *buffer = NULL;
    int x, y;

    if (!s->shm || !s->compositor)
        return NULL;

    width = al_get_bitmap_width(bmp);
    height = al_get_bitmap_height(bmp);
    if (width <= 0 || height <= 0 || width > INT_MAX / 4
        || x_focus < 0 || x_focus >= width
        || y_focus < 0 || y_focus >= height)
        return NULL;

    stride = width * 4;
    shm_size = (size_t)stride * (size_t)height;
    /* wl_shm_create_pool takes an int32_t size. */
    if (shm_size > INT32_MAX)
        return NULL;

    fd = wl_create_shm_file(shm_size);
    if (fd < 0)
        return NULL;

    shm_data = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_data == MAP_FAILED)
        goto fail;

    if (!al_is_bitmap_locked(bmp)) {
        region = al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ANY,
            ALLEGRO_LOCK_READONLY);
        if (!region)
            goto fail;
        locked_here = true;
    }

    /* WL_SHM_FORMAT_ARGB8888 is 0xAARRGGBB; on the little-endian targets
     * supported by this backend, this writes B,G,R,A bytes as required by
     * Wayland. */
    for (y = 0; y < height; y++) {
        uint32_t *row = (uint32_t *)shm_data + y * width;
        for (x = 0; x < width; x++) {
            ALLEGRO_COLOR color = al_get_pixel(bmp, x, y);
            unsigned char r, g, b, a;
            al_unmap_rgba(color, &r, &g, &b, &a);
            row[x] = ((uint32_t)a << 24) | ((uint32_t)r << 16)
                | ((uint32_t)g << 8) | b;
        }
    }

    if (locked_here) {
        al_unlock_bitmap(bmp);
        locked_here = false;
    }

    _al_mutex_lock(&s->lock);
    pool = wl_shm_create_pool(s->shm, fd, (int)shm_size);
    if (pool)
        buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
            WL_SHM_FORMAT_ARGB8888);
    if (pool)
        wl_shm_pool_destroy(pool);
    pool = NULL;
    close(fd);
    fd = -1;

    if (buffer)
        surface = wl_compositor_create_surface(s->compositor);
    if (surface) {
        wl_surface_attach(surface, buffer, 0, 0);
        wl_surface_damage(surface, 0, 0, width, height);
        wl_surface_commit(surface);
    }
    _al_mutex_unlock(&s->lock);

    if (!buffer || !surface)
        goto fail;

    cursor = al_calloc(1, sizeof *cursor);
    if (!cursor) {
        _al_mutex_lock(&s->lock);
        wl_surface_destroy(surface);
        wl_buffer_destroy(buffer);
        _al_mutex_unlock(&s->lock);
        munmap(shm_data, shm_size);
        return NULL;
    }

    cursor->surface = surface;
    cursor->buffer = buffer;
    cursor->shm_data = shm_data;
    cursor->shm_size = shm_size;
    cursor->x_focus = x_focus;
    cursor->y_focus = y_focus;
    return (ALLEGRO_MOUSE_CURSOR *)cursor;

fail:
    if (locked_here)
        al_unlock_bitmap(bmp);
    if (pool)
        wl_shm_pool_destroy(pool);
    if (fd >= 0)
        close(fd);
    if (surface || buffer) {
        _al_mutex_lock(&s->lock);
        if (surface)
            wl_surface_destroy(surface);
        if (buffer)
            wl_buffer_destroy(buffer);
        _al_mutex_unlock(&s->lock);
    }
    if (shm_data != MAP_FAILED)
        munmap(shm_data, shm_size);
    return NULL;
}


void _al_wl_destroy_mouse_cursor(ALLEGRO_MOUSE_CURSOR *cursor)
{
    ALLEGRO_SYSTEM_WAYLAND *s =
        (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    ALLEGRO_MOUSE_CURSOR_WAYLAND *wl_cursor =
        (ALLEGRO_MOUSE_CURSOR_WAYLAND *)cursor;

    _al_mutex_lock(&s->lock);
    if (the_mouse.custom_cursor == wl_cursor) {
        the_mouse.custom_cursor = NULL;
        if (the_mouse.display)
            wl_mouse_apply_cursor(&the_mouse, the_mouse.cursor_serial);
    }
    if (wl_cursor->surface)
        wl_surface_destroy(wl_cursor->surface);
    if (wl_cursor->buffer)
        wl_buffer_destroy(wl_cursor->buffer);
    _al_mutex_unlock(&s->lock);

    if (wl_cursor->shm_data)
        munmap(wl_cursor->shm_data, wl_cursor->shm_size);
    al_free(wl_cursor);
}


bool _al_wl_set_mouse_cursor(ALLEGRO_DISPLAY *display,
    ALLEGRO_MOUSE_CURSOR *cursor)
{
    ALLEGRO_SYSTEM_WAYLAND *s =
        (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    ALLEGRO_MOUSE_CURSOR_WAYLAND *wl_cursor =
        (ALLEGRO_MOUSE_CURSOR_WAYLAND *)cursor;

    if (!wl_cursor || !wl_cursor->surface)
        return false;

    _al_mutex_lock(&s->lock);
    the_mouse.custom_cursor = wl_cursor;
    if (the_mouse.display == display)
        wl_mouse_apply_cursor(&the_mouse, the_mouse.cursor_serial);
    _al_mutex_unlock(&s->lock);

    return true;
}


bool _al_wl_hide_mouse_cursor(ALLEGRO_DISPLAY *display)
{
    ALLEGRO_SYSTEM_WAYLAND *s =
        (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    bool supported;

    _al_mutex_lock(&s->lock);
    supported = the_mouse.wl_pointer != NULL;
    if (supported) {
        /* wl_pointer.set_cursor(NULL) is the protocol-defined hidden
         * cursor.  If the pointer is not focused yet, the flag is applied
         * by the next pointer-enter handler with its fresh serial. */
        the_mouse.cursor_hidden = true;
        if (the_mouse.display == display)
            wl_mouse_apply_cursor(&the_mouse, the_mouse.cursor_serial);
    }
    _al_mutex_unlock(&s->lock);

    return supported;
}


bool _al_wl_show_mouse_cursor(ALLEGRO_DISPLAY *display)
{
    ALLEGRO_SYSTEM_WAYLAND *s =
        (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    bool supported;

    _al_mutex_lock(&s->lock);
    supported = the_mouse.wl_pointer != NULL
        && (the_mouse.custom_cursor != NULL
            || the_mouse.cursor_shape != NULL);
    if (supported) {
        the_mouse.cursor_hidden = false;
        if (the_mouse.display == display)
            supported = wl_mouse_apply_cursor(&the_mouse,
                the_mouse.cursor_serial);
    }
    _al_mutex_unlock(&s->lock);

    return supported;
}


/*-------------------------------------------------------------------------*/
/* Seat */

static void seat_handle_capabilities(void *data, struct wl_seat *seat,
    uint32_t capabilities)
{
    ALLEGRO_SYSTEM_WAYLAND *s = data;
    (void)s;
    (void)seat;

    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        if (!the_keyboard.wl_keyboard) {
            the_keyboard.wl_keyboard = wl_seat_get_keyboard(seat);
            the_keyboard.xkb_context = s->xkb_context;
            wl_keyboard_add_listener(the_keyboard.wl_keyboard,
                &keyboard_listener, &the_keyboard);
            ALLEGRO_INFO("wlinput: keyboard device added\n");
        }
    }
    if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
        if (!the_mouse.wl_pointer) {
            the_mouse.wl_pointer = wl_seat_get_pointer(seat);
            wl_pointer_add_listener(the_mouse.wl_pointer,
                &pointer_listener, &the_mouse);
            if (s->cursor_shape_manager) {
                the_mouse.cursor_shape =
                    wp_cursor_shape_manager_v1_get_pointer(
                        s->cursor_shape_manager, the_mouse.wl_pointer);
            }
            ALLEGRO_INFO("wlinput: pointer device added\n");
        }
    }
}


static void seat_handle_name(void *data, struct wl_seat *seat, const char *name)
{
    (void)data;
    (void)seat;
    (void)name;
}


static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_handle_capabilities,
    .name = seat_handle_name,
};


void _al_wl_seat_add(ALLEGRO_SYSTEM_WAYLAND *s, struct wl_seat *seat)
{
    s->seat = seat;
    wl_seat_add_listener(seat, &seat_listener, s);
}


void _al_wl_input_shutdown(ALLEGRO_SYSTEM_WAYLAND *s)
{
    (void)s;

    if (the_keyboard.wl_keyboard) {
        wl_keyboard_destroy(the_keyboard.wl_keyboard);
        the_keyboard.wl_keyboard = NULL;
    }
    if (the_keyboard.keymap) {
        xkb_keymap_unref(the_keyboard.keymap);
        the_keyboard.keymap = NULL;
    }
    if (the_keyboard.xkb_state) {
        xkb_state_unref(the_keyboard.xkb_state);
        the_keyboard.xkb_state = NULL;
    }
    if (the_mouse.wl_pointer) {
        wl_pointer_destroy(the_mouse.wl_pointer);
        the_mouse.wl_pointer = NULL;
    }
    if (the_mouse.cursor_shape) {
        wp_cursor_shape_device_v1_destroy(the_mouse.cursor_shape);
        the_mouse.cursor_shape = NULL;
    }
}


/*-------------------------------------------------------------------------*/
/* Keyboard driver */

static bool wl_keyboard_init(void)
{
    if (the_keyboard.installed)
        return true;

    _al_event_source_init(&the_keyboard.parent.es);
    memset(&the_keyboard.state, 0, sizeof the_keyboard.state);
    the_keyboard.repeat_key = ALLEGRO_KEY_NONE;
    the_keyboard.installed = true;
    return true;
}


static void wl_keyboard_exit(void)
{
    if (!the_keyboard.installed)
        return;
    _al_event_source_free(&the_keyboard.parent.es);
    memset(&the_keyboard, 0, sizeof the_keyboard);
}


static ALLEGRO_KEYBOARD *wl_keyboard_get_keyboard(void)
{
    return &the_keyboard.parent;
}


static bool wl_keyboard_set_leds(int leds)
{
    (void)leds;
    /* Wayland has no LED control. */
    return false;
}


static const char *wl_keyboard_keycode_to_name(int keycode)
{
    if (keycode >= 0 && keycode < ALLEGRO_KEY_MAX)
        return _al_keyboard_common_names[keycode];
    return "";
}


static void wl_keyboard_get_state(ALLEGRO_KEYBOARD_STATE *ret_state)
{
    *ret_state = the_keyboard.state;
}


static void wl_keyboard_clear_state(void)
{
    _al_event_source_lock(&the_keyboard.parent.es);
    memset(&the_keyboard.state, 0, sizeof the_keyboard.state);
    _al_event_source_unlock(&the_keyboard.parent.es);
}


static ALLEGRO_KEYBOARD_DRIVER wl_keyboard_driver = {
    AL_ID('W','L','K','B'),
    "", "",
    "Wayland keyboard",
    wl_keyboard_init,
    wl_keyboard_exit,
    wl_keyboard_get_keyboard,
    wl_keyboard_set_leds,
    wl_keyboard_keycode_to_name,
    wl_keyboard_get_state,
    wl_keyboard_clear_state
};


ALLEGRO_KEYBOARD_DRIVER *_al_wl_keyboard_driver(void)
{
    return &wl_keyboard_driver;
}


/*-------------------------------------------------------------------------*/
/* Mouse driver */

static bool wl_mouse_init(void)
{
    if (the_mouse.installed)
        return true;

    _al_event_source_init(&the_mouse.parent.es);
    memset(&the_mouse.state, 0, sizeof the_mouse.state);
    the_mouse.cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT;
    the_mouse.cursor_serial = 0;
    the_mouse.installed = true;
    return true;
}


static void wl_mouse_exit(void)
{
    if (!the_mouse.installed)
        return;
    _al_event_source_free(&the_mouse.parent.es);
    memset(&the_mouse, 0, sizeof the_mouse);
}


static ALLEGRO_MOUSE *wl_mouse_get_mouse(void)
{
    return &the_mouse.parent;
}

/* This is hardcoded based on evdev mapping 8 mouse buttons 
 * and the fact that Wayland doesn't really keep track of
 * these things. We could just use evdev here though.
 */
static unsigned int wl_mouse_get_num_buttons(void)
{
    return 8;
}

/* See above. */
static unsigned int wl_mouse_get_num_axes(void)
{
    return 2;
}


static bool wl_mouse_set_xy(ALLEGRO_DISPLAY *display, int x, int y)
{
    ALLEGRO_SYSTEM_WAYLAND *s = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    ALLEGRO_DISPLAY_WAYLAND *d = (ALLEGRO_DISPLAY_WAYLAND *)display;
    struct zwp_locked_pointer_v1 *lock;
    int w, h;

    /* Limited implementation of mouse warping that requires locking
     * the pointer and then moving it. This only works while the
     * pointer is over the display. */

    if (the_mouse.display != display)
        return false;

    if (!s->pointer_constraints || !the_mouse.wl_pointer || !d->surface)
        return false;

    /* The compositor rejects hints outside the surface. */
    w = al_get_display_width(display);
    h = al_get_display_height(display);
    if (x < 0 || y < 0 || x >= w || y >= h)
        return false;

    lock = zwp_pointer_constraints_v1_lock_pointer(s->pointer_constraints,
        d->surface, the_mouse.wl_pointer, NULL,
        ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT);
    if (!lock)
        return false;

    zwp_locked_pointer_v1_set_cursor_position_hint(lock,
        wl_fixed_from_int(x), wl_fixed_from_int(y));
    zwp_locked_pointer_v1_destroy(lock);

    /* This requires a wl_display_roundtrip, but that's handled by the
     * background thread. */
    return true;
}


static bool wl_mouse_set_axis(int which, int value)
{
    /* Wayland does not have the equivalent X11 function that
     * globally sets this, so it's entirely local. This copies 
     * the xmouse_set_mouse_axis impl otherwise! */
    if (which != 2 && which != 3)
        return false;

    _al_event_source_lock(&the_mouse.parent.es);
    {
        int z = which == 2 ? value : the_mouse.state.z;
        int w = which == 3 ? value : the_mouse.state.w;
        int dz = z - the_mouse.state.z;
        int dw = w - the_mouse.state.w;

        if (dz != 0 || dw != 0) {
            the_mouse.state.z = z;
            the_mouse.state.w = w;
            emit_mouse_event(ALLEGRO_EVENT_MOUSE_AXES, 0, 0, 0, dz, dw);
        }
    }
    _al_event_source_unlock(&the_mouse.parent.es);

    return true;
}


static void wl_mouse_get_state(ALLEGRO_MOUSE_STATE *ret_state)
{
    *ret_state = the_mouse.state;
}


static ALLEGRO_MOUSE_DRIVER wl_mouse_driver = {
    AL_ID('W','L','M','S'),
    "", "",
    "Wayland mouse",
    wl_mouse_init,
    wl_mouse_exit,
    wl_mouse_get_mouse,
    wl_mouse_get_num_buttons,
    wl_mouse_get_num_axes,
    wl_mouse_set_xy,
    wl_mouse_set_axis,
    wl_mouse_get_state
};


ALLEGRO_MOUSE_DRIVER *_al_wl_mouse_driver(void)
{
    return &wl_mouse_driver;
}