/*
 *    Example program for the Allegro library.
 *
 *    Test config file reading and writing.
 */

#include <stdio.h>
#include "allegro5/allegro5.h"

#define TEST(name, expr) \
   if (expr) printf("PASS - %s\n", name); \
   else      printf("FAIL - %s\n", name);

int main(void)
{
   ALLEGRO_CONFIG *cfg;
   const char *value;

   if (!al_init()) {
      return 1;
   }

   cfg = al_load_config_file("data/sample.cfg");
   if (!cfg) {
      printf("Couldn't load data/sample.cfg\n");
      return 1;
   }

   value = al_get_config_value(cfg, NULL, "old_var");
   TEST("global var", value && !strcmp(value, "old global value"));

   value = al_get_config_value(cfg, "section", "old_var");
   TEST("section var", value && !strcmp(value, "old section value"));

   value = al_get_config_value(cfg, "", "mysha.xpm");
   TEST("long value", strlen(value) == 1394);

   value = al_get_config_value(cfg, "adição", "€");
   TEST("unicode", value && !strcmp(value, "¿"));

   al_set_config_value(cfg, "", "new_var", "new value");
   al_set_config_value(cfg, "section", "old_var", "new value");

   al_save_config_file(cfg, "test.cfg");

   al_destroy_config(cfg);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
