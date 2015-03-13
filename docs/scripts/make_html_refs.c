/*
 * make_html_refs DOC-FILE...
 *
 * Generate a file containing (HTML-specific) link definitions for each API
 * entry found.  e.g. if foo.txt contains "# API: bar" then we generate:
 *
 *   [bar]: foo.html#bar
 */

#include <string.h>
#include "dawk.h"

/* Replicate pandoc's function to create a label anchor from the section
 * name.
 */
static void replace_spaces(char *s)
{
    while (*s) {
        if (*s == ' ') *s = '-';
        s++;
    }
}

int main(int argc, char *argv[])
{
   dstr line;
   dstr file;
   const char *name;
   dstr sec_id;

   d_init(argc, argv);

   while (d_getline(line)) {
      if (d_match(line, "^#+( API:| ) *")) {
         d_basename(d_filename, ".html", file);
         name = d_after_match;
         d_tolower(name, sec_id);
         replace_spaces(sec_id);
         d_printf("[%s]: %s#%s\n", name, file, sec_id);
      }
   }

   return 0;
}

/* vim: set sts=3 sw=3 et: */
