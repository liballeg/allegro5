#ifndef AINTERN_CONFIG_H
#define AINTERN_CONFIG_H

struct ALLEGRO_CONFIG_ENTRY {
   char *key;
   char *value;
   ALLEGRO_CONFIG_ENTRY *next;
};


struct ALLEGRO_CONFIG_SECTION {
   char *name;
   ALLEGRO_CONFIG_ENTRY *head;
   ALLEGRO_CONFIG_SECTION *next;
};

struct ALLEGRO_CONFIG {
   ALLEGRO_CONFIG_SECTION *head;
   ALLEGRO_CONFIG_ENTRY *globals;
};


#endif /* AINTERN_CONFIG_H */

