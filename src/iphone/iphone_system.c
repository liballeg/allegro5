#include <allegro5/internal/aintern_iphone.h>
#include <allegro5/platform/aintiphone.h>
#include <allegro5/platform/aintunix.h>

A5O_DEBUG_CHANNEL("iphone")

A5O_SYSTEM_IPHONE *iphone;
static A5O_SYSTEM_INTERFACE *vt;

extern A5O_MUTEX *_al_iphone_display_hotplug_mutex;

/* al_init will call this. */
A5O_SYSTEM *iphone_initialize(int flags)
{
    (void)flags;
    iphone = al_calloc(1, sizeof *iphone);
    
    A5O_SYSTEM *sys = &iphone->system;

    iphone->mutex = al_create_mutex();
    iphone->cond = al_create_cond();
    sys->vt = _al_get_iphone_system_interface();
    _al_vector_init(&sys->displays, sizeof (A5O_DISPLAY_IPHONE *));

    _al_unix_init_time();
    _al_iphone_init_path();

    return sys;
}

static void iphone_shutdown_system(void)
{
    al_destroy_mutex(_al_iphone_display_hotplug_mutex);
}

/* This is called from the termination message - it has to return soon as the
 * user expects the app to close when it is closed.
 */
void _al_iphone_await_termination(void)
{
    A5O_INFO("Application awaiting termination.\n");
    al_lock_mutex(iphone->mutex);
    while (!iphone->has_shutdown) {
        al_wait_cond(iphone->cond, iphone->mutex);
    }
    al_unlock_mutex(iphone->mutex);
}

static A5O_DISPLAY_INTERFACE *iphone_get_display_driver(void)
{
    return _al_get_iphone_display_interface();
}

static int iphone_get_num_video_adapters(void)
{
   return _al_iphone_get_num_video_adapters();
}

static bool iphone_get_monitor_info(int adapter, A5O_MONITOR_INFO *info)
{
   int w, h;
   _al_iphone_get_screen_size(adapter, &w, &h);
   info->x1 = 0;
   info->y1 = 0;
   info->x2 = w;
   info->y2 = h;
   return true;
}

/* There is no cursor. */
static bool iphone_get_cursor_position(int *ret_x, int *ret_y)
{
   (void)ret_x;
   (void)ret_y;
   return false;
}

A5O_SYSTEM_INTERFACE *_al_get_iphone_system_interface(void)
{
    if (vt)
       return vt;
    
    vt = al_calloc(1, sizeof *vt);
    
    vt->id = A5O_SYSTEM_ID_IPHONE;
    vt->initialize = iphone_initialize;
    vt->get_display_driver = iphone_get_display_driver;
    vt->get_keyboard_driver = _al_get_iphone_keyboard_driver;
    vt->get_mouse_driver = _al_get_iphone_mouse_driver;
    vt->get_touch_input_driver = _al_get_iphone_touch_input_driver;
    vt->get_joystick_driver = _al_get_iphone_joystick_driver;
    //xglx_vt->get_num_display_modes = _al_xglx_get_num_display_modes;
    //xglx_vt->get_display_mode = _al_xglx_get_display_mode;
    //xglx_vt->shutdown_system = xglx_shutdown_system;
    vt->shutdown_system = iphone_shutdown_system;
    vt->get_num_video_adapters = iphone_get_num_video_adapters;
    vt->get_monitor_info = iphone_get_monitor_info;
    vt->get_cursor_position = iphone_get_cursor_position;
    vt->get_path = _al_iphone_get_path;
    //xglx_vt->inhibit_screensaver = xglx_inhibit_screensaver;
    
    return vt;
}

/* This is a function each platform must define to register all available
 * system drivers.
 */
void _al_register_system_interfaces(void)
{
    A5O_SYSTEM_INTERFACE **add;
    
    add = _al_vector_alloc_back(&_al_system_interfaces);
    *add = _al_get_iphone_system_interface();
}
