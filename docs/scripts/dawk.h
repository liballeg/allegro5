#ifndef __included_dawk_h
#define __included_dawk_h

#ifdef _MSC_VER
   #define bool int
   #define true 1
   #define false 0
#else
   #include <stdbool.h>
#endif
#include <stdio.h>

/* One line of the TexInfo output is nearly this long as it doesn't perform
 * word wrapping.
 */
#define MAX_STRING   3000
#define MAX_MATCH    8

typedef char         dstr[MAX_STRING];

extern void          (*d_cleanup)(void);
extern dstr          d_filename;
extern int           d_line_num;
extern FILE          *d_stdout;
extern dstr          d_before_match;
extern dstr          d_after_match;

#define D_STDOUT     (d_stdout ? d_stdout : stdout)

#define d_abort(msg1, msg2)   (d_doabort(__FILE__, __LINE__, (msg1), (msg2)))

extern void d_doabort(const char *filename, int line, const char *msg1,
   const char *msg2);

extern void d_init(int argc, char *argv[]);
extern void d_open_input(const char *filename);
extern void d_close_input(void);
extern bool d_getline(dstr var);

extern void d_open_output(const char *filename);
extern void d_close_output(void);
extern void d_print(const char *s);
extern void d_printf(const char *format, ...);

extern void d_assign(dstr to, const dstr from);
extern void d_assignn(dstr to, const dstr from, size_t n);

extern bool d_match(dstr line, const char *regex);
extern const char *d_submatch(int i);

extern void d_basename(const char *filename, const char *newext, dstr output);
extern void d_tolower(const dstr src, dstr dest);
extern void d_delchr(dstr str, char c);

#endif

/* vim: set sts=3 sw=3 et: */
