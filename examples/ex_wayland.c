#include "allegro5/allegro.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    al_set_config_value(al_get_system_config(), "trace", "level", "debug");
    al_init();

    ALLEGRO_DISPLAY *d = al_create_display(600, 400);

    return 0;
}