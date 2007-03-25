#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>


#define MAKEFILE_HELPER_FILENAME "makefile.helper"


int main(int argc, char *argv[])
{
    char shortfilename[MAX_PATH];
    char *longfilename;
    int ret;
    FILE *fout;
    
    if (argc != 2) {
        fprintf(stderr, "msvchelp: No argument given!\n");
        return 1;
    }
    
    longfilename = getenv(argv[1]);
    if (longfilename == NULL) {
        fprintf(stderr, "msvchelp: Given argument does not correspond to any enviroment variable name!\n");
        return 2;
    }
    
    ret = GetShortPathName(longfilename, shortfilename, MAX_PATH);
    if (ret == 0 || ret > MAX_PATH) {
        fprintf(stderr, "msvchelp: Cannot convert to short name!\n");
        return 3;
    }
    
    fout = fopen(MAKEFILE_HELPER_FILENAME, "wt");
    if (fout == NULL) {
        fprintf(stderr, "msvchelp: Cannot create output file '%s'!\n", MAKEFILE_HELPER_FILENAME);
        return 4;
    }
    
    ret = fprintf(fout, "%s = %s\n", argv[1], shortfilename);
    if (ret < 0) {
        fprintf(stderr, "msvchelp: Cannot write to the output file '%s'!\n", MAKEFILE_HELPER_FILENAME);
        return 5;
    }
    
    ret = fclose(fout);
    if (ret != 0) {
        fprintf(stderr, "msvchelp: Cannot close the output file '%s'!\n", MAKEFILE_HELPER_FILENAME);
        return 6;
    }
    
    return EXIT_SUCCESS;
}
