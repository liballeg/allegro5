#include <stdio.h>
#include <ctype.h>
#include <allegro5/allegro5.h>
#include <allegro5/internal/aintern_config.h>

const int MAXSIZE = 1024;


static char *skip_whitespace(char *s)
{
   while (isspace(*s))
      s++;

   return s;
}


static ALLEGRO_CONFIG_SECTION *find_section(ALLEGRO_CONFIG *config, AL_CONST char *section)
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


static ALLEGRO_CONFIG_ENTRY *find_entry(ALLEGRO_CONFIG_ENTRY *section_head, AL_CONST char *key)
{
   ALLEGRO_CONFIG_ENTRY *p = section_head;

   while (p != NULL) {
      if (!strcmp(p->key, key)) {
         return p;
      }
      p = p->next;
   }

   return NULL;
}


static void get_key_and_value(char *buf, char *key, char *value)
{
   char *p = skip_whitespace(buf);
   int i, j;

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

   for (i = 0; p[i] && i < MAXSIZE-1 && p[i] != '\n'; i++) {
      value[i] = p[i];
   }
   value[i] = 0;
}


void al_config_add_section(ALLEGRO_CONFIG *config, AL_CONST char *name)
{
   ALLEGRO_CONFIG_SECTION *sec = config->head;
   ALLEGRO_CONFIG_SECTION *section;

   if (find_section(config, name))
      return;

   section = calloc(1, sizeof(ALLEGRO_CONFIG_SECTION));
   section->name = strdup(name);

   if (sec == NULL) {
      config->head = section;
   }
   else {
      while (sec->next != NULL)
         sec = sec->next;
      sec->next = section;
   }
}


void al_config_set_global(ALLEGRO_CONFIG *config, AL_CONST char *key, AL_CONST char *value)
{
   ALLEGRO_CONFIG_ENTRY *p;
   ALLEGRO_CONFIG_ENTRY *e = find_entry(config->globals, key);

   if (e) {
      free(e->value);
      e->value = strdup(value);
      return;
   }
   
   e = calloc(1, sizeof(ALLEGRO_CONFIG_ENTRY));

   e->key = strdup(key);
   e->value = strdup(value);

   if (config->globals == NULL) {
      config->globals = e;
   }
   else {
      p = config->globals;
      while (p->next != NULL) {
         p = p->next;
      }
      p->next = e;
   }
}


void al_config_set_value(ALLEGRO_CONFIG *config, AL_CONST char *section, AL_CONST char *key, AL_CONST char *value)
{
   ALLEGRO_CONFIG_SECTION *s = find_section(config, section);
   ALLEGRO_CONFIG_ENTRY *entry = find_entry(s->head, key);

   if (entry) {
      free(entry->value);
      entry->value = strdup(value);
      return;
   }
   
   entry = calloc(1, sizeof(ALLEGRO_CONFIG_ENTRY));

   entry->key = strdup(key);
   entry->value = strdup(value);
   
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


/*
 * Gets a pointer to an internal character buffer that will
 * only remain valid as long as the ALLEGRO_CONFIG structure
 * is not destroyed. Call stdup if you need a copy.
 */
AL_CONST char *al_config_get_value(ALLEGRO_CONFIG *config, AL_CONST char *section, AL_CONST char *key)
{
   ALLEGRO_CONFIG_ENTRY *e;

   if (section == NULL) {
      e = config->globals;
   }
   else {
      ALLEGRO_CONFIG_SECTION *s = find_section(config, section);
      if (!s)
         return NULL;
      e = s->head;
   }

   e = find_entry(e, key);
   if (!e)
      return NULL;

   return e->value;
}



ALLEGRO_CONFIG *al_config_read(AL_CONST char *filename)
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
   
   config = calloc(1, sizeof(ALLEGRO_CONFIG));
   if (!config) {
      fclose(file);
      return NULL;
   }

   while (fgets(buffer, MAXSIZE, file)) {
      char *ptr = skip_whitespace(buffer);
      if (*ptr == 0)
         continue;
      if (*ptr == '#')
         continue;
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
            al_config_set_global(config, key, value);
         else
            al_config_set_value(config, current_section->name, key, value);
      }
   }

   fclose(file);

   return config;
}


int al_config_write(ALLEGRO_CONFIG *config, AL_CONST char *filename)
{
   ALLEGRO_CONFIG_SECTION *s = config->head;
   ALLEGRO_CONFIG_ENTRY *e;
   FILE *file = fopen(filename, "w");

   if (!file) {
      return 1;
   }

   /* Save globals */
   e = config->globals;
   while (e != NULL) {
      fprintf(file, "%s=%s\n", e->key, e->value);
      e = e->next;
   }

   /* Save each section */
   while (s != NULL) {
      fprintf(file, "[%s]\n", s->name);
      e = s->head;
      while (e != NULL) {
         fprintf(file, "%s=%s\n", e->key, e->value);
         e = e->next;
      }
      s = s->next;
   }

   fclose(file);

   return 0;
}


static void add_config(ALLEGRO_CONFIG *master, ALLEGRO_CONFIG *add)
{
   ALLEGRO_CONFIG_SECTION *s = add->head;
   ALLEGRO_CONFIG_ENTRY *e;

   /* Save globals */
   e = add->globals;
   while (e != NULL) {
      al_config_set_global(master, e->key, e->value);
      e = e->next;
   }

   /* Save each section */
   while (s != NULL) {
      al_config_add_section(master, s->name);
      e = s->head;
      while (e != NULL) {
         al_config_set_value(master, s->name, e->key, e->value);
         e = e->next;
      }
      s = s->next;
   }
}


ALLEGRO_CONFIG *al_config_merge(ALLEGRO_CONFIG *cfg1, ALLEGRO_CONFIG *cfg2)
{
   ALLEGRO_CONFIG *config = calloc(1, sizeof(ALLEGRO_CONFIG));

   add_config(config, cfg1);
   add_config(config, cfg2);

   return config;
}


void al_config_destroy(ALLEGRO_CONFIG *config)
{
   ALLEGRO_CONFIG_ENTRY *e;
   ALLEGRO_CONFIG_SECTION *s;

   e = config->globals;
   while (e) {
      ALLEGRO_CONFIG_ENTRY *tmp = e->next;
      free(e->key);
      free(e->value);
      free(e);
      e = tmp;
   }

   s = config->head;
   while (s) {
      ALLEGRO_CONFIG_SECTION *tmp = s->next;
      e = s->head;
      while (e) {
         ALLEGRO_CONFIG_ENTRY *tmp = e->next;
         free(e->key);
         free(e->value);
         free(e);
         e = tmp;
      }
      free(s->name);
      free(s);
      s = tmp;
   }
}

