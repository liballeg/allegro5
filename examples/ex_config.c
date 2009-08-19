/*
 *    Example program for the Allegro library.
 *
 *    Test config file reading and writing.
 */

#include <stdio.h>
#include "allegro5/allegro5.h"

#include "common.c"

static int passed = true;
#define TEST(name, expr) \
   if (expr) printf(" PASS - %s\n", name); \
   else      {printf("!FAIL - %s\n", name); passed = false;}

int main(void)
{
   ALLEGRO_CONFIG *cfg;
   const char *value;
   void *iterator, *iterator2;

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
   TEST("long value", value && strlen(value) == 1394);
   
   /* Test whether iterating through our whole sample.cfg returns all
    * sections and entries, in order.
    */

   value = al_get_config_value(cfg, "adição", "€");
   TEST("unicode", value && !strcmp(value, "¿"));

   value = al_get_first_config_section(cfg, &iterator);
   TEST("section1", value && !strcmp(value, ""));
   
   value = al_get_first_config_entry(cfg, value, &iterator2);
   TEST("entry1", value && !strcmp(value, "old_var"));
   
   value = al_get_next_config_entry(&iterator2);
   TEST("entry2", value && !strcmp(value, "mysha.xpm"));
   
   value = al_get_next_config_entry(&iterator2);
   TEST("entry3", value == NULL);

   value = al_get_next_config_section(&iterator);
   TEST("section2", value && !strcmp(value, "section"));
   
   value = al_get_first_config_entry(cfg, value, &iterator2);
   TEST("entry4", value && !strcmp(value, "old_var"));
   
   value = al_get_next_config_entry(&iterator2);
   TEST("entry5", value == NULL);

   value = al_get_next_config_section(&iterator);
   TEST("section3", value && !strcmp(value, "adição"));
   
   value = al_get_first_config_entry(cfg, value, &iterator2);
   TEST("entry6", value && !strcmp(value, "€"));
   
   value = al_get_next_config_entry(&iterator2);
   TEST("entry7", value == NULL);
   
   value = al_get_next_config_section(&iterator);
   TEST("section4", value == NULL);
   
   

   al_set_config_value(cfg, "", "new_var", "new value");
   al_set_config_value(cfg, "section", "old_var", "new value");

   TEST("save_config", al_save_config_file(cfg, "test.cfg"));

   al_destroy_config(cfg);

   return passed ? 0 : 1;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
