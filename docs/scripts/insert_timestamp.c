/*
 * Generate an fragment to be inserted at the bottom of HTML pages.
 * The timestamp follows RFC 3339.
 * We use UTC.
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include "dawk.h"

int main(int argc, char **argv)
{
   time_t now;
   char buf[64];
   char const *version = "unknown";

   time(&now);
   strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S UTC", gmtime(&now));
   
   if (argc > 1) {
      dstr line;
   
      d_open_input(argv[1]);

      while (d_getline(line)) {
         if (d_match(line, "#define ALLEGRO_VERSION_STR *\"([^\"]*)\"")) {
            version = d_submatch(1);
            break;
         }
      }

      d_close_input();
   }

   printf("<p class=\"timestamp\">\n");
   printf("Allegro version %s\n", version);
   printf(" - Last updated: %s\n", buf);
   printf("</p>\n");
   return 0;
}

/* vim: set sts=3 sw=3 et: */
