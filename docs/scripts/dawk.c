/* Support for awk-style processing in C. */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* I'd prefer to use the POSIX regexp interface, but for portability we need a
 * fallback, and supporting two regexp implementations is asking for trouble.
 * T-Rex is small and quick enough so we use it everywhere.  However it
 * doesn't seem to behave properly on some expressions (which we can work
 * around when the regexps are fixed) so I couldn't recommend it in general.
 */
#include "dawk.h"
#include "trex.h"

typedef struct {
   const char     *regex;
   TRex           *reg;
} re_cache_t;

void              (*d_cleanup)(void);

static int        d_argc;
static char       **d_argv;
static int        d_file_num;

static FILE       *d_file;
dstr              d_filename;
int               d_line_num;

FILE              *d_stdout;
#define D_STDOUT  (d_stdout ? d_stdout : stdout)

#define MAX_RE_CACHE 16
static re_cache_t d_regex_cache[MAX_RE_CACHE];
static dstr       d_submatches[MAX_MATCH];
dstr              d_before_match;
dstr              d_after_match;


/* Abort with an error message. */
void d_doabort(const char *filename, int line, const char *msg1,
   const char *msg2)
{
   fprintf(stderr, "%s:%d: %s%s\n", filename, line, msg1, msg2);
   if (d_cleanup) {
      d_cleanup();
   }
   exit(EXIT_FAILURE);
}


/* Prepare to read input from the files listed in argv. */
void d_init(int argc, char *argv[])
{
   d_argc = argc;
   d_argv = argv;
   d_file_num = 0;
   d_close_input();
}


/* Open a single file for reading. */
void d_open_input(const char *filename)
{
   if (d_file) {
      fclose(d_file);
   }
   d_file = fopen(filename, "r");
   if (!d_file) {
      d_abort("could not open file for reading: ", filename);
   }
   d_assign(d_filename, filename);
   d_line_num = 0;
   d_file_num = -1;
}


/* Close input file. */
void d_close_input(void)
{
   if (d_file) {
      fclose(d_file);
   }
   d_file = NULL;
   d_assign(d_filename, "");
}


/* Read the next line from the current input file(s). */
bool d_getline(dstr var)
{
   char *p = var;
   int c;

   /* Open the next file if necessary. */
   if (!d_file) {
      if (d_file_num == -1 || d_file_num + 1 >= d_argc) {
         return false;
      }
      d_file_num++;
      d_file = fopen(d_argv[d_file_num], "r");
      if (!d_file) {
         d_abort("could not open file for reading: ", d_argv[d_file_num]);
      }
      d_assign(d_filename, d_argv[d_file_num]);
      d_line_num = 0;
   }

   for (;;) {
      c = fgetc(d_file);
      if (c == EOF || c == '\n') {
         break;
      }
      *p++ = c;
      if (p - var >= MAX_STRING - 1) {
         fprintf(stderr, "dawk: string length limit reached\n");
         break;
      }
   }
   *p = '\0';

   if (c == EOF) {
      fclose(d_file);
      d_file = NULL;
      if (p == var) {
         return d_getline(var);
      }
   }
   else if (c == '\n') {
      d_line_num++;
      /* Remove trailing CR if present. */
      if (p > var && p[-1] == '\r') {
         p[-1] = '\0';
      }
   }

   return true;
}


/* Open a file for writing. */
void d_open_output(const char *filename)
{
   FILE *f;

   d_close_output();
   f = fopen(filename, "w");
   if (!f) {
      d_abort("error opening file for writing: ", filename);
   }
   d_stdout = f;
}


/* Close the output file, reverting to standard output. */
void d_close_output(void)
{
   if (d_stdout && d_stdout != stdout) {
      fclose(d_stdout);
   }
   d_stdout = NULL;
}


/* Print a line to the output file, with newline. */
void d_print(const char *s)
{
   fprintf(D_STDOUT, "%s\n", s);
}


/* Print formatted output to the output file. */
void d_printf(const char *format, ...)
{
   va_list ap;
   va_start(ap, format);
   vfprintf(D_STDOUT, format, ap);
   va_end(ap);
}


/* Assign a string. */
void d_assign(dstr to, const dstr from)
{
   /* Might be overlapping. */
   memmove(to, from, strlen(from) + 1);
}


/* Assign a length-delimited string. */
void d_assignn(dstr to, const dstr from, size_t n)
{
   /* Might be overlapping. */
   memmove(to, from, n);
   to[n] = '\0';
}


static re_cache_t *compile_regex(const char *regex)
{
   re_cache_t *re;
   int i;

   for (i = 0; i < MAX_RE_CACHE; i++) {
      re = &d_regex_cache[i];
      if (re->regex == NULL) {
         re->regex = regex;
         re->reg = trex_compile(regex, NULL);
         if (re->reg == NULL) {
            d_abort("error compiling regular expression: ", regex);
         }
      }
      if (re->regex == regex) {
         return re;
      }
   }

   d_abort("too many regular expressions", "");
   return NULL;
}


/* Match a string against the given regular expression.
 * Returns true on a successful match.
 */
bool d_match(dstr line, const char *regex)
{
   re_cache_t *re;
   TRexMatch match;
   int i;

   re = compile_regex(regex);

   if (!trex_search(re->reg, line, NULL, NULL)) {
      return false;
   }

   trex_getsubexp(re->reg, 0, &match);
   d_assignn(d_before_match, line, match.begin - line);
   d_assign(d_after_match, match.begin + match.len);

   for (i = 0; i < MAX_MATCH; i++) {
      if (trex_getsubexp(re->reg, i, &match)) {
         strncpy(d_submatches[i], match.begin, match.len);
         d_submatches[i][match.len] = '\0';
      }
      else {
         d_submatches[i][0] = '\0';
      }
   }

   return true;
}


/* Return a submatch from the previous d_match call. */
const char *d_submatch(int i)
{
   return d_submatches[i];
}


static const char *strrchr2(const char *s, char c1, char c2)
{
   const char *p = s + strlen(s);
   for (; p >= s; p--) {
      if (*p == c1 || *p == c2)
         return p;
   }
   return NULL;
}


void d_basename(const char *filename, const char *newext, dstr output)
{
   const char *start;
   char *dot;

   start = strrchr2(filename, '/', '\\');
   if (start)
      strcpy(output, start + 1);
   else
      strcpy(output, filename);

   if (newext) {
      dot = strrchr(output, '.');
      if (dot)
         strcpy(dot, newext);
      else
         strcat(output, newext);
   }
}


void d_tolower(const dstr src, dstr dest)
{
   const char *s = src;
   char *d = dest;
   for (; *s; s++, d++) {
      *d = tolower(*s);
   }
   *d = '\0';
}


void d_delchr(dstr str, char c)
{
   const char *r = str;
   char *w = str;
   for (; *r; r++) {
      if (*r != c) {
         *w = *r;
         w++;
      }
   }
   *w = '\0';
}

/* vim: set sts=3 sw=3 et: */
