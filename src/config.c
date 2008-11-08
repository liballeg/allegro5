/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Configuration routines.
 *
 *      By Trent Gamblin.
 */

/* Title: Configuration routines
 */


#include <stdio.h>
#include <ctype.h>
#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_config.h"
#include "allegro5/internal/aintern_memory.h"


#define MAXSIZE 1024


static void *local_calloc1(size_t size)
{
   void *p;

   p = _AL_MALLOC(size);
   if (p) {
      memset(p, 0, size);
   }
   return p;
}


static char *local_strdup(const char *s)
{
   size_t size = strlen(s);
   char *copy;

   copy = _AL_MALLOC_ATOMIC(size + 1);
   if (copy) {
      memcpy(copy, s, size + 1);
   }
   return copy;
}


static char *skip_whitespace(char *s)
{
   while (*s && isspace(*s))
      s++;

   return s;
}


/* Function: al_config_create
 *  Create an empty configuration structure.
 */
ALLEGRO_CONFIG *al_config_create(void)
{
   ALLEGRO_CONFIG *config = local_calloc1(sizeof(ALLEGRO_CONFIG));
   ASSERT(config);

   return config;
}


static ALLEGRO_CONFIG_SECTION *find_section(const ALLEGRO_CONFIG *config,
   const char *section)
{
   ALLEGRO_CONFIG_SECTION *p = config->head;

   if (!p)
      return NULL;

   while (p != NULL) {
      if (!strcmp(p->name, section))
         return p;
      p = p->next;
   }

   return NULL;
}


static ALLEGRO_CONFIG_ENTRY *find_entry(ALLEGRO_CONFIG_ENTRY *section_head,
   const char *key)
{
   ALLEGRO_CONFIG_ENTRY *p = section_head;

   while (p != NULL) {
      if (p->key && !strcmp(p->key, key)) {
         return p;
      }
      p = p->next;
   }

   return NULL;
}


static void get_key_and_value(char *buf, char *key, char *value)
{
   char *p = skip_whitespace(buf);
   int i;

   /* Error */
   if (*p == 0) {
      strcpy(key, "");
      strcpy(value, "");
   }

   /* Read key */
   for (i = 0; p[i] && i < MAXSIZE-1 && !isspace(p[i]) && p[i] != '='; i++) {
      key[i] = p[i];
   }
   key[i] = 0;

   /* Error */
   if (!p[i]) {
      strcpy(value, "");
      return;
   }

   /* Skip past = */
   if (p[i] == '=')
      p += i+1;
   else {
      p += i;
      p = skip_whitespace(p);
      /* Error */
      if (*p != '=') {
         strcpy(value, "");
         return;
      }
      p++;
   }

   /* Read value, stripping leading and trailing whitespace */
   p = skip_whitespace(p);
   for (i = 0; p[i] && i < MAXSIZE-1 && p[i] != '\n'; i++) {
      value[i] = p[i];
   }
   while (i > 0 && isspace(value[i-1]))
      i--;
   value[i] = 0;
}


/* Function: al_config_add_section
 *  Add a section to a configuration structure.
 */
void al_config_add_section(ALLEGRO_CONFIG *config, const char *name)
{
   ALLEGRO_CONFIG_SECTION *sec = config->head;
   ALLEGRO_CONFIG_SECTION *section;

   if (find_section(config, name))
      return;

   section = local_calloc1(sizeof(ALLEGRO_CONFIG_SECTION));
   section->name = local_strdup(name);

   if (sec == NULL) {
      config->head = section;
   }
   else {
      while (sec->next != NULL)
         sec = sec->next;
      sec->next = section;
   }
}


/* Function: al_config_set_value
 *  Set a value in a section of a configuration.  If the section doesn't yet
 *  exist, it will be created.  If a value already existed for the given key,
 *  it will be overwritten.
 *  The section can be NULL or "" for the global section.
 */
void al_config_set_value(ALLEGRO_CONFIG *config,
   const char *section, const char *key, const char *value)
{
   ALLEGRO_CONFIG_SECTION *s;
   ALLEGRO_CONFIG_ENTRY *entry;

   if (section == NULL) {
      section = "";
   }

   ASSERT(key);
   ASSERT(value);

   s = find_section(config, section);
   if (s) {
      entry = find_entry(s->head, key);
   }
   else {
      entry = NULL;
   }

   if (entry) {
      _AL_FREE(entry->value);
      entry->value = local_strdup(value);
      return;
   }
   
   entry = local_calloc1(sizeof(ALLEGRO_CONFIG_ENTRY));

   entry->key = local_strdup(key);
   entry->value = local_strdup(value);
   
   if (!s) {
      al_config_add_section(config, section);
      s = find_section(config, section);
   }

   if (s->head == NULL) {
      s->head = entry;
   }
   else {
      ALLEGRO_CONFIG_ENTRY *p = s->head;
      while (p->next != NULL)
         p = p->next;
      p->next = entry;
   }
}


/* Function: al_config_add_comment
 *  Add a comment in a section of a configuration.  If the section doesn't yet
 *  exist, it will be created.
 *  The section can be NULL or "" for the global section.
 */
void al_config_add_comment(ALLEGRO_CONFIG *config,
   const char *section, const char *comment)
{
   ALLEGRO_CONFIG_SECTION *s;
   ALLEGRO_CONFIG_ENTRY *entry;

   if (section == NULL) {
      section = "";
   }

   ASSERT(comment);

   s = find_section(config, section);

   entry = local_calloc1(sizeof(ALLEGRO_CONFIG_ENTRY));

   entry->comment = local_strdup(comment);
   
   if (!s) {
      al_config_add_section(config, section);
      s = find_section(config, section);
   }

   if (s->head == NULL) {
      s->head = entry;
   }
   else {
      ALLEGRO_CONFIG_ENTRY *p = s->head;
      while (p->next != NULL)
         p = p->next;
      p->next = entry;
   }
}


/* Function: al_config_get_value
 *  Gets a pointer to an internal character buffer that will only remain valid
 *  as long as the ALLEGRO_CONFIG structure is not destroyed. Copy the value
 *  if you need a copy.
 *  The section can be NULL or "" for the global section.
 *  Returns NULL if the section or key do not exist.
 */
const char *al_config_get_value(const ALLEGRO_CONFIG *config,
   const char *section, const char *key)
{
   ALLEGRO_CONFIG_SECTION *s;
   ALLEGRO_CONFIG_ENTRY *e;

   if (section == NULL) {
      section = "";
   }

   s = find_section(config, section);
   if (!s)
      return NULL;
   e = s->head;

   e = find_entry(e, key);
   if (!e)
      return NULL;

   return e->value;
}


/* Function: al_config_read
 *  Read a configuration file.
 *  Returns NULL on error.
 */
ALLEGRO_CONFIG *al_config_read(const char *filename)
{
   ALLEGRO_CONFIG *config;
   ALLEGRO_CONFIG_SECTION *current_section = NULL;
   char buffer[MAXSIZE], section[MAXSIZE], key[MAXSIZE], value[MAXSIZE];
   FILE *file;
   int i;

   file = fopen(filename, "r");
   if (!file) {
      return NULL;
   }
   
   config = al_config_create();
   if (!config) {
      fclose(file);
      return NULL;
   }

   while (fgets(buffer, MAXSIZE, file)) {
      char *ptr = skip_whitespace(buffer);
      if (*ptr == '#' || *ptr == 0) {
         /* Preserve comments */
         const char *name = (current_section && current_section->name) ?
            current_section->name : "";
         const char *comment = (*ptr == 0) ? "\n" : ptr;
         al_config_add_comment(config, name, comment);
      }
      else if (*ptr == '[') {
         strcpy(section, ptr+1);
         for (i = strlen(section)-1; i >= 0; i--) {
            if (section[i] == ']') {
               section[i] = 0;
               break;
            }
         }
         al_config_add_section(config, section);
         current_section = find_section(config, section);
      }
      else {
         get_key_and_value(buffer, key, value);
         if (current_section == NULL)
            al_config_set_value(config, "", key, value);
         else
            al_config_set_value(config, current_section->name, key, value);
      }
   }

   fclose(file);

   return config;
}


/* Function: al_config_write
 *  Write out a configuration file.
 *  Returns zero on success, non-zero on error.
 */
int al_config_write(const ALLEGRO_CONFIG *config, const char *filename)
{
   ALLEGRO_CONFIG_SECTION *s;
   ALLEGRO_CONFIG_ENTRY *e;
   FILE *file = fopen(filename, "w");

   if (!file) {
      return 1;
   }

   /* Save global section */
   s = config->head;
   while (s != NULL) {
      if (strcmp(s->name, "") == 0) {
         e = s->head;
         while (e != NULL) {
            if (e->comment != NULL) {
               if (!fprintf(file, "%s", e->comment)) {
                  goto Error;
               }
            }
            else {
               if (fprintf(file, "%s=%s\n", e->key, e->value) < 0) {
                  goto Error;
               }
            }
            e = e->next;
         }
         break;
      }
      s = s->next;
   }

   /* Save other sections */
   s = config->head;
   while (s != NULL) {
      if (strcmp(s->name, "") != 0) {
         if (fprintf(file, "[%s]\n", s->name) < 0) {
            goto Error;
         }
         e = s->head;
         while (e != NULL) {
            if (e->comment != NULL) {
               if (!fprintf(file, "%s", e->comment)) {
                  goto Error;
               }
            }
            else {
               if (fprintf(file, "%s=%s\n", e->key, e->value) < 0) {
                  goto Error;
               }
            }
            e = e->next;
         }
      }
      s = s->next;
   }

   if (fclose(file) == EOF) {
      /* XXX do we delete the incomplete file? */
      return 1;
   }

   return 0;

Error:

   /* XXX do we delete the incomplete file? */
   fclose(file);
   return 1;
}


/* do_config_merge_into:
 *  Helper function for merging.
 */
void do_config_merge_into(ALLEGRO_CONFIG *master, const ALLEGRO_CONFIG *add,
   bool merge_comments)
{
   ALLEGRO_CONFIG_SECTION *s;
   ALLEGRO_CONFIG_ENTRY *e;
   ASSERT(master);

   if (!add) {
      return;
   }

   /* Save each section */
   s = add->head;
   while (s != NULL) {
      al_config_add_section(master, s->name);
      e = s->head;
      while (e != NULL) {
         if (e->key) {
            al_config_set_value(master, s->name, e->key, e->value);
         }
         else if (merge_comments) {
            ASSERT(e->comment);
            al_config_add_comment(master, s->name, e->comment);
         }
         e = e->next;
      }
      s = s->next;
   }
}


/* Function: al_config_merge_into
 *  Merge one configuration structure into another.
 *  Values in configuration 'add' override those in 'master'.
 *  'master' is modified.
 *  Comments from 'add' are not retained.
 */
void al_config_merge_into(ALLEGRO_CONFIG *master, const ALLEGRO_CONFIG *add)
{
   do_config_merge_into(master, add, false);
}


/* Function: al_config_merge
 *  Merge two configuration structures, and return the result as a new
 *  configuration.  Values in configuration 'cfg2' override those in 'cfg1'.
 *  Neither of the input configuration structures are
 *  modified.
 *  Comments from 'cfg2' are not retained.
 */
ALLEGRO_CONFIG *al_config_merge(const ALLEGRO_CONFIG *cfg1,
    const ALLEGRO_CONFIG *cfg2)
{
   ALLEGRO_CONFIG *config = al_config_create();

   do_config_merge_into(config, cfg1, true);
   do_config_merge_into(config, cfg2, false);

   return config;
}


/* Function: al_config_destroy
 *  Free the resources used by a configuration structure.
 *  Does nothing if passed NULL.
 */
void al_config_destroy(ALLEGRO_CONFIG *config)
{
   ALLEGRO_CONFIG_ENTRY *e;
   ALLEGRO_CONFIG_SECTION *s;

   if (!config) {
      return;
   }

   s = config->head;
   while (s) {
      ALLEGRO_CONFIG_SECTION *tmp = s->next;
      e = s->head;
      while (e) {
         ALLEGRO_CONFIG_ENTRY *tmp = e->next;
         _AL_FREE(e->key);
         _AL_FREE(e->value);
         _AL_FREE(e);
         e = tmp;
      }
      _AL_FREE(s->name);
      _AL_FREE(s);
      s = tmp;
   }

   _AL_FREE(config);
}

/* vim: set sts=3 sw=3 et: */
