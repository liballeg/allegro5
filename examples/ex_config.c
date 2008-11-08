/*
 *    Example program for the Allegro library.
 *
 *    Test config file reading and writing.
 */

#include <stdio.h>
#include "allegro5/allegro5.h"

int main(void)
{
   ALLEGRO_CONFIG *cfg;
   const char *value;

   if (!al_init()) {
      return 1;
   }

   cfg = al_config_read("data/sample.cfg");
   if (!cfg) {
      printf("Couldn't load data/sample.cfg\n");
      return 1;
   }

   value = al_config_get_value(cfg, NULL, "old_var");
   printf("global old_var = %s\n", value);

   value = al_config_get_value(cfg, "section", "old_var");
   printf("section old_var = %s\n", value);

   al_config_set_value(cfg, "", "new_var", "new value");
   al_config_set_value(cfg, "section", "old_var", "new value");

   al_config_write(cfg, "test.cfg");

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
