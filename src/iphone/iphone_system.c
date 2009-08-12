#include <allegro5/internal/aintern_iphone.h>
#include <allegro5/internal/aintern_memory.h>
#include <allegro5/platform/aintunix.h>

ALLEGRO_SYSTEM_IPHONE *iphone;
static ALLEGRO_SYSTEM_INTERFACE *vt;

/* al_init will call this. */
ALLEGRO_SYSTEM *iphone_initialize(int flags)
{
    iphone = _AL_MALLOC(sizeof *iphone);
    memset(iphone, 0, sizeof *iphone);
    
    ALLEGRO_SYSTEM *sys = &iphone->system;

    iphone->mutex = al_create_mutex();
    iphone->cond = al_create_cond();
    sys->vt = _al_get_iphone_system_interface();
    _al_vector_init(&sys->displays, sizeof (ALLEGRO_DISPLAY_IPHONE *));

    _al_unix_init_time();
    _al_iphone_init_path();
    return sys;
}

static ALLEGRO_DISPLAY_INTERFACE *iphone_get_display_driver(void)
{
    return _al_get_iphone_display_interface();
}

ALLEGRO_SYSTEM_INTERFACE *_al_get_iphone_system_interface(void)
{
    if (vt) return vt;
    
    vt = _AL_MALLOC(sizeof *vt);
    memset(vt, 0, sizeof *vt);
    
    vt->initialize = iphone_initialize;
    vt->get_display_driver = iphone_get_display_driver;
    vt->get_keyboard_driver = _al_get_iphone_keyboard_driver;
    vt->get_mouse_driver = _al_get_iphone_mouse_driver;
    //xglx_vt->get_joystick_driver = xglx_get_joystick_driver;
    //xglx_vt->get_num_display_modes = _al_xglx_get_num_display_modes;
    //xglx_vt->get_display_mode = _al_xglx_get_display_mode;
    //xglx_vt->shutdown_system = xglx_shutdown_system;
    //xglx_vt->get_num_video_adapters = xglx_get_num_video_adapters;
    //xglx_vt->get_monitor_info = xglx_get_monitor_info;
    //xglx_vt->get_cursor_position = xglx_get_cursor_position;
    vt->get_path = _al_iphone_get_path;
    //xglx_vt->inhibit_screensaver = xglx_inhibit_screensaver;
    //xglx_vt->get_num_display_formats = xglx_get_num_display_formats;
    //xglx_vt->get_display_format_option = xglx_get_display_format_option;
    //xglx_vt->set_new_display_format = xglx_set_new_display_format;
    
    return vt;
}

/* This is a function each platform must define to register all available
 * system drivers.
 */
void _al_register_system_interfaces(void)
{
    ALLEGRO_SYSTEM_INTERFACE **add;
    
    add = _al_vector_alloc_back(&_al_system_interfaces);
    *add = _al_get_iphone_system_interface();
}
