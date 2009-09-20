/*
 * Generate an fragment to be inserted at the bottom of HTML pages.
 * The timestamp follows RFC 3339.
 * We use UTC.
 */

#include <stdio.h>
#include <time.h>

int main(void)
{
   time_t now;
   char buf[64];

   time(&now);
   strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S UTC", gmtime(&now));

   printf("<p>\n");
   printf("Last updated: %s\n", buf);
   printf("</p>\n");
   return 0;
}

/* vim: set sts=3 sw=3 et: */
