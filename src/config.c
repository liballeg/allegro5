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
#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_aatree.h"
#include "allegro5/internal/aintern_config.h"



static int cmp_ustr(void const *a, void const *b)
{
   return al_ustr_compare(a, b);
}


/* Function: al_create_config
 */
A5O_CONFIG *al_create_config(void)
{
   A5O_CONFIG *config = al_calloc(1, sizeof(A5O_CONFIG));
   ASSERT(config);

   return config;
}


static A5O_CONFIG_SECTION *find_section(const A5O_CONFIG *config,
   const A5O_USTR *section)
{
   return _al_aa_search(config->tree, section, cmp_ustr);
}


static A5O_CONFIG_ENTRY *find_entry(const A5O_CONFIG_SECTION *section,
   const A5O_USTR *key)
{
   return _al_aa_search(section->tree, key, cmp_ustr);
}


static void get_key_and_value(const A5O_USTR *buf,
   A5O_USTR *key, A5O_USTR *value)
{
   int eq = al_ustr_find_chr(buf, 0, '=');

   if (eq == -1) {
      al_ustr_assign(key, buf);
      al_ustr_assign_cstr(value, "");
   }
   else {
      al_ustr_assign_substr(key, buf, 0, eq);
      al_ustr_assign_substr(value, buf, eq + 1, al_ustr_size(buf));
   }

   al_ustr_trim_ws(key);
   al_ustr_trim_ws(value);
}


static A5O_CONFIG_SECTION *config_add_section(A5O_CONFIG *config,
   const A5O_USTR *name)
{
   A5O_CONFIG_SECTION *sec = config->head;
   A5O_CONFIG_SECTION *section;

   if ((section = find_section(config, name)))
      return section;

   section = al_calloc(1, sizeof(A5O_CONFIG_SECTION));
   section->name = al_ustr_dup(name);

   if (sec == NULL) {
      config->head = section;
      config->last = section;
   }
   else {
      ASSERT(config->last->next == NULL);
      config->last->next = section;
      section->prev = config->last;
      config->last = section;
   }

   config->tree = _al_aa_insert(config->tree, section->name, section, cmp_ustr);

   return section;
}


/* Function: al_add_config_section
 */
void al_add_config_section(A5O_CONFIG *config, const char *name)
{
   A5O_USTR_INFO name_info;
   const A5O_USTR *uname;

   uname = al_ref_cstr(&name_info, name);
   config_add_section(config, uname);
}


static void config_set_value(A5O_CONFIG *config,
   const A5O_USTR *section, const A5O_USTR *key,
   const A5O_USTR *value)
{
   A5O_CONFIG_SECTION *s;
   A5O_CONFIG_ENTRY *entry;

   s = find_section(config, section);
   if (s) {
      entry = find_entry(s, key);
      if (entry) {
         al_ustr_assign(entry->value, value);
         al_ustr_trim_ws(entry->value);
         return;
      }
   }

   entry = al_calloc(1, sizeof(A5O_CONFIG_ENTRY));
   entry->is_comment = false;
   entry->key = al_ustr_dup(key);
   entry->value = al_ustr_dup(value);
   al_ustr_trim_ws(entry->value);

   if (!s) {
      s = config_add_section(config, section);
   }

   if (s->head == NULL) {
      s->head = entry;
      s->last = entry;
   }
   else {
      ASSERT(s->last->next == NULL);
      s->last->next = entry;
      entry->prev = s->last;
      s->last = entry;
   }

   s->tree = _al_aa_insert(s->tree, entry->key, entry, cmp_ustr);
}


/* Function: al_set_config_value
 */
void al_set_config_value(A5O_CONFIG *config,
   const char *section, const char *key, const char *value)
{
   A5O_USTR_INFO section_info;
   A5O_USTR_INFO key_info;
   A5O_USTR_INFO value_info;
   const A5O_USTR *usection;
   const A5O_USTR *ukey;
   const A5O_USTR *uvalue;

   if (section == NULL) {
      section = "";
   }

   ASSERT(key);
   ASSERT(value);

   usection = al_ref_cstr(&section_info, section);
   ukey = al_ref_cstr(&key_info, key);
   uvalue = al_ref_cstr(&value_info, value);

   config_set_value(config, usection, ukey, uvalue);
}


static void config_add_comment(A5O_CONFIG *config,
   const A5O_USTR *section, const A5O_USTR *comment)
{
   A5O_CONFIG_SECTION *s;
   A5O_CONFIG_ENTRY *entry;

   s = find_section(config, section);

   entry = al_calloc(1, sizeof(A5O_CONFIG_ENTRY));
   entry->is_comment = true;
   entry->key = al_ustr_dup(comment);

   /* Replace all newline characters by spaces, otherwise the written comment
    * file will be corrupted.
    */
   al_ustr_find_replace_cstr(entry->key, 0, "\n", " ");

   if (!s) {
      s = config_add_section(config, section);
   }

   if (s->head == NULL) {
      s->head = entry;
      s->last = entry;
   }
   else {
      ASSERT(s->last->next == NULL);
      s->last->next = entry;
      entry->prev = s->last;
      s->last = entry;
   }
}


/* Function: al_add_config_comment
 */
void al_add_config_comment(A5O_CONFIG *config,
   const char *section, const char *comment)
{
   A5O_USTR_INFO section_info;
   A5O_USTR_INFO comment_info;
   const A5O_USTR *usection;
   const A5O_USTR *ucomment;

   if (section == NULL) {
      section = "";
   }

   ASSERT(comment);

   usection = al_ref_cstr(&section_info, section);
   ucomment = al_ref_cstr(&comment_info, comment);

   config_add_comment(config, usection, ucomment);
}


static bool config_get_value(const A5O_CONFIG *config,
   const A5O_USTR *section, const A5O_USTR *key,
   const A5O_USTR **ret_value)
{
   A5O_CONFIG_SECTION *s;
   A5O_CONFIG_ENTRY *e;

   s = find_section(config, section);
   if (!s)
      return false;

   e = find_entry(s, key);
   if (!e)
      return false;

   *ret_value = e->value;
   return true;
}


/* Function: al_get_config_value
 */
const char *al_get_config_value(const A5O_CONFIG *config,
   const char *section, const char *key)
{
   A5O_USTR_INFO section_info;
   A5O_USTR_INFO key_info;
   const A5O_USTR *usection;
   const A5O_USTR *ukey;
   const A5O_USTR *value;

   if (section == NULL) {
      section = "";
   }

   usection = al_ref_cstr(&section_info, section);
   ukey = al_ref_cstr(&key_info, key);

   if (config_get_value(config, usection, ukey, &value))
      return al_cstr(value);
   else
      return NULL;
}


static bool readline(A5O_FILE *file, A5O_USTR *line)
{
   char buf[128];

   if (!al_fgets(file, buf, sizeof(buf))) {
      return false;
   }

   do {
      al_ustr_append_cstr(line, buf);
      if (al_ustr_has_suffix_cstr(line, "\n"))
         break;
   } while (al_fgets(file, buf, sizeof(buf)));

   return true;
}


/* Function: al_load_config_file
 */
A5O_CONFIG *al_load_config_file(const char *filename)
{
   A5O_FILE *file;
   A5O_CONFIG *cfg = NULL;

   file = al_fopen(filename, "r");
   if (file) {
      cfg = al_load_config_file_f(file);
      al_fclose(file);
   }

   return cfg;
}


/* Function: al_load_config_file_f
 */
A5O_CONFIG *al_load_config_file_f(A5O_FILE *file)
{
   A5O_CONFIG *config;
   A5O_CONFIG_SECTION *current_section = NULL;
   A5O_USTR *line;
   A5O_USTR *section;
   A5O_USTR *key;
   A5O_USTR *value;
   ASSERT(file);

   config = al_create_config();
   if (!config) {
      return NULL;
   }

   line = al_ustr_new("");
   section = al_ustr_new("");
   key = al_ustr_new("");
   value = al_ustr_new("");

   while (1) {
      al_ustr_assign_cstr(line, "");
      if (!readline(file, line))
         break;
      al_ustr_trim_ws(line);

      if (al_ustr_has_prefix_cstr(line, "#") || al_ustr_size(line) == 0) {
         /* Preserve comments and blank lines */
         const A5O_USTR *name;
         if (current_section)
            name = current_section->name;
         else
            name = al_ustr_empty_string();
         config_add_comment(config, name, line);
      }
      else if (al_ustr_has_prefix_cstr(line, "[")) {
         int rbracket = al_ustr_rfind_chr(line, al_ustr_size(line), ']');
         if (rbracket == -1)
            rbracket = al_ustr_size(line);
         al_ustr_assign_substr(section, line, 1, rbracket);
         current_section = config_add_section(config, section);
      }
      else {
         get_key_and_value(line, key, value);
         if (current_section == NULL)
            config_set_value(config, al_ustr_empty_string(), key, value);
         else
            config_set_value(config, current_section->name, key, value);
      }
   }

   al_ustr_free(line);
   al_ustr_free(section);
   al_ustr_free(key);
   al_ustr_free(value);

   return config;
}


static bool config_write_section(A5O_FILE *file,
   const A5O_CONFIG_SECTION *s)
{
   A5O_CONFIG_ENTRY *e;

   if (al_ustr_size(s->name) > 0) {
      al_fputc(file, '[');
      al_fputs(file, al_cstr(s->name));
      al_fputs(file, "]\n");
      if (al_ferror(file)) {
         return false;
      }
   }

   e = s->head;
   while (e != NULL) {
      if (e->is_comment) {
         if (al_ustr_size(e->key) > 0) {
            if (!al_ustr_has_prefix_cstr(e->key, "#")) {
               al_fputs(file, "# ");
            }
            al_fputs(file, al_cstr(e->key));
         }
         al_fputc(file, '\n');
      }
      else {
         al_fputs(file, al_cstr(e->key));
         al_fputc(file, '=');
         al_fputs(file, al_cstr(e->value));
         al_fputc(file, '\n');
      }
      if (al_ferror(file)) {
         return false;
      }

      e = e->next;
   }

   return !al_feof(file);
}


/* Function: al_save_config_file
 */
bool al_save_config_file(const char *filename, const A5O_CONFIG *config)
{
   A5O_FILE *file;

   file = al_fopen(filename, "w");
   if (file) {
      bool retsave = al_save_config_file_f(file, config);
      bool retclose = al_fclose(file);
      return retsave && retclose;
   }

   return false;
}


/* Function: al_save_config_file_f
 */
bool al_save_config_file_f(A5O_FILE *file, const A5O_CONFIG *config)
{
   A5O_CONFIG_SECTION *s;

   /* Save global section */
   s = config->head;
   while (s != NULL) {
      if (al_ustr_size(s->name) == 0) {
         if (!config_write_section(file, s)) {
            return false;
         }
         break;
      }
      s = s->next;
   }

   /* Save other sections */
   s = config->head;
   while (s != NULL) {
      if (al_ustr_size(s->name) > 0) {
         if (!config_write_section(file, s)) {
            return false;
         }
      }
      s = s->next;
   }

   return !al_feof(file);
}


/* do_config_merge_into:
 *  Helper function for merging.
 */
static void do_config_merge_into(A5O_CONFIG *master,
   const A5O_CONFIG *add, bool merge_comments)
{
   A5O_CONFIG_SECTION *s;
   A5O_CONFIG_ENTRY *e;
   ASSERT(master);

   if (!add) {
      return;
   }

   /* Save each section */
   s = add->head;
   while (s != NULL) {
      config_add_section(master, s->name);
      e = s->head;
      while (e != NULL) {
         if (!e->is_comment) {
            config_set_value(master, s->name, e->key, e->value);
         }
         else if (merge_comments) {
            config_add_comment(master, s->name, e->key);
         }
         e = e->next;
      }
      s = s->next;
   }
}


/* Function: al_merge_config_into
 */
void al_merge_config_into(A5O_CONFIG *master, const A5O_CONFIG *add)
{
   do_config_merge_into(master, add, false);
}


/* Function: al_merge_config
 */
A5O_CONFIG *al_merge_config(const A5O_CONFIG *cfg1,
    const A5O_CONFIG *cfg2)
{
   A5O_CONFIG *config = al_create_config();

   do_config_merge_into(config, cfg1, true);
   do_config_merge_into(config, cfg2, false);

   return config;
}


static void destroy_entry(A5O_CONFIG_ENTRY *e)
{
   al_ustr_free(e->key);
   al_ustr_free(e->value);
   al_free(e);
}


static void destroy_section(A5O_CONFIG_SECTION *s)
{
   A5O_CONFIG_ENTRY *e = s->head;
   while (e) {
      A5O_CONFIG_ENTRY *tmp = e->next;
      destroy_entry(e);
      e = tmp;
   }
   al_ustr_free(s->name);
   _al_aa_free(s->tree);
   al_free(s);
}


/* Function: al_destroy_config
 */
void al_destroy_config(A5O_CONFIG *config)
{
   A5O_CONFIG_SECTION *s;

   if (!config) {
      return;
   }

   s = config->head;
   while (s) {
      A5O_CONFIG_SECTION *tmp = s->next;
      destroy_section(s);
      s = tmp;
   }

   _al_aa_free(config->tree);
   al_free(config);
}


/* Function: al_get_first_config_section
 */
char const *al_get_first_config_section(A5O_CONFIG const *config,
   A5O_CONFIG_SECTION **iterator)
{
   A5O_CONFIG_SECTION *s;

   if (!config)
      return NULL;
   s = config->head;
   if (iterator) *iterator = s;
   return s ? al_cstr(s->name) : NULL;
}


/* Function: al_get_next_config_section
 */
char const *al_get_next_config_section(A5O_CONFIG_SECTION **iterator)
{
   A5O_CONFIG_SECTION *s;

   if (!iterator)
      return NULL;
   s = *iterator;
   if (s)
      s = s->next;
   *iterator = s;
   return s ? al_cstr(s->name) : NULL;
}


/* Function: al_get_first_config_entry
 */
char const *al_get_first_config_entry(A5O_CONFIG const *config,
   char const *section, A5O_CONFIG_ENTRY **iterator)
{
   A5O_USTR_INFO section_info;
   const A5O_USTR *usection;
   A5O_CONFIG_SECTION *s;
   A5O_CONFIG_ENTRY *e;

   if (!config)
      return NULL;

   if (section == NULL)
      section = "";

   usection = al_ref_cstr(&section_info, section);
   s = find_section(config, usection);
   if (!s)
      return NULL;
   e = s->head;
   while (e && e->is_comment)
      e = e->next;
   if (iterator) *iterator = e;
   return e ? al_cstr(e->key) : NULL;
}


/* Function: al_get_next_config_entry
 */
char const *al_get_next_config_entry(A5O_CONFIG_ENTRY **iterator)
{
   A5O_CONFIG_ENTRY *e;

   if (!iterator)
      return NULL;
   e = *iterator;
   if (e)
      e = e->next;
   while (e && e->is_comment)
      e = e->next;
   *iterator = e;
   return e ? al_cstr(e->key) : NULL;
}


/* Function: al_remove_config_section
 */
bool al_remove_config_section(A5O_CONFIG *config, char const *section)
{
   A5O_USTR_INFO section_info;
   A5O_USTR const *usection;
   void *value;
   A5O_CONFIG_SECTION *s;

   if (section == NULL)
      section = "";

   usection = al_ref_cstr(&section_info, section);

   value = NULL;
   config->tree = _al_aa_delete(config->tree, usection, cmp_ustr, &value);
   if (!value)
      return false;

   s = value;

   if (s->prev) {
      s->prev->next = s->next;
   }
   else {
      ASSERT(config->head == s);
      config->head = s->next;
   }

   if (s->next) {
      s->next->prev = s->prev;
   }
   else {
      ASSERT(config->last == s);
      config->last = s->prev;
   }

   destroy_section(s);
   return true;
}


/* Function: al_remove_config_key
 */
bool al_remove_config_key(A5O_CONFIG *config, char const *section,
   char const *key)
{
   A5O_USTR_INFO section_info;
   A5O_USTR_INFO key_info;
   A5O_USTR const *usection;
   A5O_USTR const *ukey = al_ref_cstr(&key_info, key);
   void *value;
   A5O_CONFIG_ENTRY * e;

   if (section == NULL)
      section = "";

   usection = al_ref_cstr(&section_info, section);

   A5O_CONFIG_SECTION *s = find_section(config, usection);
   if (!s)
      return false;

   value = NULL;
   s->tree = _al_aa_delete(s->tree, ukey, cmp_ustr, &value);
   if (!value)
      return false;

   e = value;

   if (e->prev) {
      e->prev->next = e->next;
   }
   else {
      ASSERT(s->head == e);
      s->head = e->next;
   }

   if (e->next) {
      e->next->prev = e->prev;
   }
   else {
      ASSERT(s->last == e);
      s->last = e->prev;
   }

   destroy_entry(e);

   return true;
}

/* vim: set sts=3 sw=3 et: */
