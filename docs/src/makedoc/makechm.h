#ifndef MAKECHM_H
#define MAKECHM_H

int write_chm(char *filename);
char *get_section_filename(char *buf, const char *path, int section_number);

#endif
