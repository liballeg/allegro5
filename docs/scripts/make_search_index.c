/*
 * make_search_index html_refs > search_index.js
 *
 * Read html_refs and convert it to a Javascript index for autosuggest.js.
 */

#include <string.h>
#include "dawk.h"

#define MAX_ENTRIES  1024

dstr names[MAX_ENTRIES];
dstr urls[MAX_ENTRIES];
int num_entries = 0;

int main(int argc, char *argv[])
{
   dstr line;
   int i;

   d_init(argc, argv);

   while (d_getline(line)) {
      if (d_match(line, "^\\[(.*)\\]: (.*)")) {
         strcpy(names[num_entries], d_submatch(1));
         strcpy(urls[num_entries], d_submatch(2));
         num_entries++;
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
