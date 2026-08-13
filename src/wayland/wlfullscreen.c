#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_wl.h"
#include "allegro5/internal/aintern_wldisplay.h"
#include "allegro5/internal/aintern_wlfullscreen.h"
#include "allegro5/internal/aintern_wlsystem.h"

int _al_wayland_get_num_video_adapters(ALLEGRO_SYSTEM_WAYLAND *s)
{
    EGLint num_devices;
    PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT =
(PFNEGLQUERYDEVICESEXTPROC)eglGetProcAddress("eglQueryDevicesEXT");
}

int _al_wayland_get_default_adapter(ALLEGRO_SYSTEM_WAYLAND *s)
{
    return 0;
}