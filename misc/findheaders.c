#include <stdio.h>
#include <malloc.h>
#include <string.h>

static char *read_header(char *name)
{
  long size;
  char *header;
  FILE *f=fopen(name,"r");
  if (!f) {
    perror(name);
    return strdup("");
  }
  fseek(f,0,SEEK_END);
  size=ftell(f);
  fseek(f,0,SEEK_SET);
  header=malloc(size+1L);
  fread(header,size,1,f);
  header[size]=0;
  fclose(f);
  return header;
}

int main(int argc,char **argv)
{
  static char s[4096];
  int size;
  char **headers;
  int header_count;
  int n;
  char *header_name;

  headers=NULL;
  for (header_count=0;header_count<argc-1;header_count++) {
    headers=realloc(headers,(header_count+1)*sizeof(char*));
    headers[header_count]=read_header(argv[header_count+1]);
  }

  while ("false") {
    fgets(s,sizeof(s),stdin);
    if (feof(stdin)) break;
    size=strlen(s);
    if (s[size-1]=='\n') s[size-1]=0;

    header_name="";
    for (n=0;n<header_count;n++) {
      if (strstr(headers[n],s)) {
        header_name=argv[n+1];
        break;
      }
    }
    fprintf(stdout,"%s\n",header_name);
    fflush(stdout);
  }

  while (header_count) free(headers[--header_count]);

  return 0;
}
