#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    char shortfilename[MAX_PATH];
    char *longfilename;
    int ret;
    FILE *fout;
    
    if (argc != 2) {
        fprintf(stderr, "No argument given!\n");
        return 1;
    }
    
    longfilename = getenv(argv[1]);
    if (longfilename == NULL) {
        fprintf(stderr, "Given argument does not correspond to any enviroment variable name!\n");
        return 2;
    }
    
    ret = GetShortPathName(longfilename, shortfilename, MAX_PATH);
    if (ret == 0 || ret > MAX_PATH) {
        fprintf(stderr, "Cannot convert to short name!\n");
        return 3;
    }
    
    fout = fopen("makefile.helper", "wt");
    if (fout == NULL) {
        fprintf(stderr, "Cannot create output file!\n");
        return 4;
    }
    
    ret = fprintf(fout, "%s = %s\n", argv[1], shortfilename);
    if (ret < 0) {
        fprintf(stderr, "Cannot write to the output file!\n");
        return 5;
    }
    
    ret = fclose(fout);
    if (ret != 0) {
        fprintf(stderr, "Cannot close the output file!\n");
        return 6;
    }
    
    return EXIT_SUCCESS;
}
