/*
 * make_html_refs DOC-FILE...
 *
 * Generate a file containing (HTML-specific) link definitions for each API
 * entry found.  e.g. if foo.txt contains "# API: bar" then we generate:
 *
 *   [bar]: foo.html#bar
 */

#include "dawk.h"

int main(int argc, char *argv[])
{
   dstr line;
   dstr file;
   const char *name;
   dstr sec_id;

   d_init(argc, argv);

   while (d_getline(line)) {
      if (d_match(line, "^#+ API: *")) {
         d_basename(d_filename, ".html", file);
         name = d_after_match;
         d_tolower(name, sec_id);
         /* The extra blank lines are to work around Pandoc issue #182. */
         d_printf("[%s]: %s#%s\n\n", name, file, sec_id);
      }
   }

   return 0;
}

/* vim: set sts=3 sw=3 et: */
