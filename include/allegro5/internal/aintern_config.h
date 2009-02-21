#ifndef AINTERN_CONFIG_H
#define AINTERN_CONFIG_H

struct ALLEGRO_CONFIG_ENTRY {
   bool is_comment;
   ALLEGRO_USTR *key;    /* comment if is_comment is true */
   ALLEGRO_USTR *value;
   ALLEGRO_CONFIG_ENTRY *next;
};


struct ALLEGRO_CONFIG_SECTION {
   ALLEGRO_USTR *name;
   ALLEGRO_CONFIG_ENTRY *head;
   ALLEGRO_CONFIG_SECTION *next;
};

struct ALLEGRO_CONFIG {
   ALLEGRO_CONFIG_SECTION *head;
};


#endif /* AINTERN_CONFIG_H */

