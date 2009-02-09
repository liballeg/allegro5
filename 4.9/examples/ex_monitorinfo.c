#include "allegro5/allegro5.h"
#include <stdio.h>

int main(void)
{
   ALLEGRO_MONITOR_INFO info;
   int num_adapters;
   int i;

   if (!al_init()) {
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   num_adapters = al_get_num_video_adapters();

   printf("%d adapters found...\n", num_adapters);

   for (i = 0; i < num_adapters; i++) {
      al_get_monitor_info(i, &info);
      printf("Adapter %d: ", i);
      printf("(%d, %d) - (%d, %d)\n", info.x1, info.y1, info.x2, info.y2);
   }

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
