/*
 * SYNOPSIS
 *
 * faster_search FILE PATTERN REPLACEMENT
 *
 * DESCRIPTION
 *
 * Print for lines in FILE beginning with PATTERN, replacing the PATTERN
 * with REPLACEMENT.  The pattern may match multiple lines, but the program
 * will stop at the first mismatching line after a match has been found.
 */

#include <stdio.h>
#include <string.h>

int main(int argc, const char *argv[])
{
    const char *filename;
    const char *pattern;
    const char *prefix;
    int pattern_length;
    FILE *fp;
    char line[8192];
    int found = 0;

    if (argc != 4) {
	return 1;
    }
    filename = argv[1];
    pattern = argv[2];
    prefix = argv[3];
    pattern_length = strlen(pattern);

    fp = fopen(filename, "r");
    if (!fp) {
	perror("fopen");
	return 1;
    }
    while (fgets(line, sizeof(line), fp)) {
	if (strncmp(line, pattern, pattern_length) == 0) {
	    printf("%s%s", prefix, line + pattern_length);
	    found = 1;
	} else if (found)  {
	    break;
	}
    }
    fclose(fp);
    return 0;
}
