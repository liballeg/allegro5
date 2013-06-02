#include "allegro5/allegro.h"
#include <stdio.h>

#include "common.c"

int main(void)
{
   ALLEGRO_MONITOR_INFO info;
   int num_adapters;
   int i, j;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   open_log();

   num_adapters = al_get_num_video_adapters();

   log_printf("%d adapters found...\n", num_adapters);

   for (i = 0; i < num_adapters; i++) {
      al_get_monitor_info(i, &info);
      log_printf("Adapter %d: ", i);
      log_printf("(%d, %d) - (%d, %d)\n", info.x1, info.y1, info.x2, info.y2);
      al_set_new_display_adapter(i);
      log_printf("   Available fullscreen display modes:\n");
      for (j = 0; j < al_get_num_display_modes(); j++) {
         ALLEGRO_DISPLAY_MODE mode;

         al_get_display_mode(j, &mode);

         log_printf("   Mode %3d: %4d x %4d, %d Hz\n",
            j, mode.width, mode.height, mode.refresh_rate);
      }
   }

   close_log(true);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
