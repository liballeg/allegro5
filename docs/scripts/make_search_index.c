/*
 * make_search_index html_refs > search_index.js
 *
 * Read html_refs and convert it to a Javascript index for autosuggest.js.
 */

#include <string.h>
#include <stdlib.h>
#include "dawk.h"

dstr *names;
dstr *urls;
int num_entries = 0;

static int add_entry(void) {
   int i = num_entries++;
   names = realloc(names, num_entries * sizeof *names);
   urls = realloc(urls, num_entries * sizeof *urls);
   return i;
}

int main(int argc, char *argv[])
{
   dstr line;
   int i;

   d_init(argc, argv);

   while (d_getline(line)) {
      if (d_match(line, "^\\[(.*)\\]: (.*)")) {
         int i = add_entry();
         strcpy(names[i], d_submatch(1));
         strcpy(urls[i], d_submatch(2));
      }
   }

   printf("var search_index = [\n");
   for (i = 0; i < num_entries; i++) {
      printf("'%s',", names[i]);
   }
   printf("]\n");

   printf("var search_urls = [\n");
   for (i = 0; i < num_entries; i++) {
      printf("'%s',", urls[i]);
   }
   printf("]\n");

   return 0;
}

/* vim: set sts=3 sw=3 et: */
