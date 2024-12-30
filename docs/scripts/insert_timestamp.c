/*
 * Generate an fragment to be inserted at the bottom of HTML pages.
 * The timestamp follows RFC 3339.
 * We use UTC.
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include "dawk.h"

int main(int argc, char **argv)
{
   time_t now;
   char buf[64];
   char const *version = "unknown";

   char *source_date_epoch;
   unsigned long long epoch;
   char *endptr;

   source_date_epoch = getenv("SOURCE_DATE_EPOCH");
   if (source_date_epoch) {
      errno = 0;
      epoch = strtoull(source_date_epoch, &endptr, 10);
      if ((errno == ERANGE && (epoch == ULLONG_MAX || epoch == 0))
            || (errno != 0 && epoch == 0)) {
         fprintf(stderr, "Environment variable $SOURCE_DATE_EPOCH: strtoull: %s\n", strerror(errno));
         exit(EXIT_FAILURE);
      }
      if (endptr == source_date_epoch) {
         fprintf(stderr, "Environment variable $SOURCE_DATE_EPOCH: No digits were found: %s\n", endptr);
         exit(EXIT_FAILURE);
      }
      if (*endptr != '\0') {
         fprintf(stderr, "Environment variable $SOURCE_DATE_EPOCH: Trailing garbage: %s\n", endptr);
         exit(EXIT_FAILURE);
      }
      if (epoch > ULONG_MAX) {
         fprintf(stderr, "Environment variable $SOURCE_DATE_EPOCH: value must be smaller than or equal to %lu but was found to be: %llu \n", ULONG_MAX, epoch);
         exit(EXIT_FAILURE);
      }
      now = epoch;
   } else {
      now = time(NULL);
   }
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
