#ifndef __al_included_allegro5_aintern_config_h
#define __al_included_allegro5_aintern_config_h

#include "allegro5/internal/aintern_aatree.h"

struct A5O_CONFIG_ENTRY {
   bool is_comment;
   A5O_USTR *key;    /* comment if is_comment is true */
   A5O_USTR *value;
   A5O_CONFIG_ENTRY *prev, *next;
};

struct A5O_CONFIG_SECTION {
   A5O_USTR *name;
   A5O_CONFIG_ENTRY *head;
   A5O_CONFIG_ENTRY *last;
   _AL_AATREE *tree;
   A5O_CONFIG_SECTION *prev, *next;
};

struct A5O_CONFIG {
   A5O_CONFIG_SECTION *head;
   A5O_CONFIG_SECTION *last;
   _AL_AATREE *tree;
};


#endif

