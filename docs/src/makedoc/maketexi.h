#ifndef MAKETEXI_H
#define MAKETEXI_H

extern int multiwordheaders;

int write_texinfo(char *filename);
char **build_types_lookup_table(char **found_table);
void destroy_types_lookup_table(char **auto_types, char *found_auto_types);
char *strstruct(char *line, char **auto_types, int *length, char *found_auto_types);

#endif
