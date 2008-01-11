#include <allegro.h>
#include <stdlib.h>
#include "../include/virtctl.h"


void destroy_vcontroller(VCONTROLLER * controller, const char *config_path)
{
   if (controller == 0) {
      return;
   }

   /* save assignments */
   if (controller->write_config != 0) {
      controller->write_config(controller, config_path);
   }

   /* free private data */
   if (controller->private_data != 0) {
      free(controller->private_data);
      controller->private_data = 0;
   }

   /* free the actual pointer */
   free(controller);
   controller = 0;
}
