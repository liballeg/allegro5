#ifndef MAKEMISC_H
#define MAKEMISC_H

int myisalnum(int c);
int mytolower(int c);
int mystricmp(const char *s1, const char *s2);
char *mystristr(const char *HAYSTACK, const char *NEEDLE);
char *mystrlwr(char *string);
int strincmp(const char *s1, const char *s2);
int myisspace(int c);
char *strip_html(char *p);
int is_empty(char *s);
char *get_extension(const char *filename);
char *get_filename(const char *path);
int scmp(const void *e1, const void *e2);
char *get_clean_ref_token(const char *text);

/* Safe memory functions */

void *m_xmalloc(size_t size);
void *m_xrealloc(void *ptr, size_t new_size);
void m_abort(int code);
char *m_strdup(const char *text);
char *m_strcat(char *dynamic_string, const char *normal_string);
char *m_fgets(FILE *file);
char *m_replace_extension(const char *path, const char *extension);
char *m_replace_filename(const char *path, const char *filename);

#endif
