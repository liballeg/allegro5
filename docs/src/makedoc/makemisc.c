/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Makedoc's miscellaneous functions
 *
 *      By Shawn Hargreaves.
 *
 *      Grzegorz Adam Hankiewicz added safe memory/io functions.
 *
 *      See readme.txt for copyright information.
 *
 *      See allegro/docs/src/makedoc/format.txt for a brief description of
 *      the source of _tx files.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "makemisc.h"

#define _CHUNK 100


/* myisalum:
 */
int myisalnum(int c)
{
   return (((c >= '0') && (c <= '9')) ||
	   ((c >= 'A') && (c <= 'Z')) ||
	   ((c >= 'a') && (c <= 'z')) ||
	   (c >= 0x80) || (c < 0));
}



/* mytolower:
 */
int mytolower(int c)
{
   if ((c >= 'A') && (c <= 'Z'))
      c -= ('A' - 'a');

   return c;
}



/* mystricmp:
 */
int mystricmp(const char *s1, const char *s2)
{
   int c1, c2;

   do {
      c1 = mytolower(*(s1++));
      c2 = mytolower(*(s2++));
   } while ((c1) && (c1 == c2));

   return c1 - c2;
}



/* mystrlwr:
 */
char *mystrlwr(char *string)
{
   char *p;

   for (p=string; *p; p++)
      *p = mytolower(*p);

   return string;
}



/* strincmp:
 */
int strincmp(char *s1, char *s2)
{
   while (*s2) {
      if (mytolower(*s1) != mytolower(*s2))
	 return 1;

      s1++;
      s2++;
   }

   return 0;
}



/* myisspace:
 */
int myisspace(int c)
{
   return ((c == ' ') || (c == '\t') || (c == '\r') || 
	   (c == '\n') || (c == '\f') || (c == '\v'));
}



/* strip_html:
 */
char *strip_html(char *p)
{
   static char buf[256];
   int c;

   c = 0;
   while (*p) {
      if (*p == '<') {
	 while ((*p) && (*p != '>'))
	    p++;
	 if (*p)
	    p++;
      }
      else {
	 buf[c] = *p;
	 c++;
	 p++;
      }
   }

   buf[c] = 0;

   return buf;
}



/* is_empty:
 */
int is_empty(char *s)
{
   while (*s) {
      if (*s != ' ')
	 return 0;

      s++;
   }

   return 1;
}



/* get_filename:
 */
char *get_filename(char *path)
{
   int pos;

   for (pos=0; path[pos]; pos++)
      ; /* do nothing */ 

   while ((pos>0) && (path[pos-1] != '\\') && (path[pos-1] != '/'))
      pos--;

   return path+pos;
}



/* extension:
 */
char *extension(char *filename)
{
   int pos, end;

   for (end=0; filename[end]; end++)
      ; /* do nothing */

   pos = end;

   while ((pos>0) && (filename[pos-1] != '.') &&
	  (filename[pos-1] != '\\') && (filename[pos-1] != '/'))
      pos--;

   if (filename[pos-1] == '.')
      return filename+pos;

   return filename+end;
}



/* scmp:
 */
int scmp(const void *e1, const void *e2)
{
   char *s1 = *((char **)e1);
   char *s2 = *((char **)e2);

   return mystricmp(s1, s2);
}



/* m_xmalloc:
 * Returns the requested chunk of memory. If there's not enough
 * memory, the program will abort.
 */
void *m_xmalloc(size_t size)
{
   void *p = malloc(size);
   if (!p) m_abort(1);
   return p;
}



/* m_xrealloc:
 * Wrapper around real realloc call. Returns the new chunk of memory or
 * aborts execution if it couldn't realloc it.
 */
void *m_xrealloc(void *ptr, size_t new_size)
{
   if (!ptr)
      return m_xmalloc(new_size);
   ptr = realloc(ptr, new_size);
   if (!ptr) m_abort(1);
   return ptr;
}



/* m_abort:
 * Aborts execution with a hopefully meaningful message. If code is less
 * than 1, an undefined exit will happen. Available error codes:
 * 1: insufficient memory
 */
void m_abort(int code)
{
   switch(code) {
      case 1:	printf("Aborting due to insuficcient memory\n"); break;
      default:	printf("An undefined error caused abnormal termination\n");
		code = -1;
   }
   exit(code);
}



/* m_strdup:
 * Safe wrapper around strdup, always returns the duplicated string.
 */
char *m_strdup(const char *text)
{
   char *p = m_xmalloc(strlen(text)+1);
   return strcpy(p, text);
}



/* m_fgets:
 * Safe function to read text lines from a file. Returns the read line
 * or NULL when you reach the end of the file or there's a problem.
 */
char *m_fgets(FILE *file)
{
   int read, filled = 0, size = 0;
   char *buf = 0;

   while ((read = getc(file)) != EOF) {
      while (filled + 2 >= size) {
	 size += _CHUNK;
	 buf = m_xrealloc(buf, size);
      }
      if (read != '\r')
	 buf[filled++] = read;
      buf[filled] = 0;

      if (read == '\n')
	 break;
   }

   return buf;
}



/* m_strcat:
 * Special strcat function, which is a mixture of realloc and strcat.
 * The first parameter has to be a pointer to dynamic memory, since it's
 * space will be resized with m_xrealloc (it can be NULL). The second
 * pointer can be any type of string, and will be appended to the first
 * one. This function returns a new pointer to the memory holding both
 * strings.
 */
char *m_strcat(char *dynamic_string, const char *normal_string)
{
   int len;

   if(!dynamic_string)
      return m_strdup(normal_string);

   len = strlen(dynamic_string);
   dynamic_string = m_xrealloc(dynamic_string, 1 + len + strlen(normal_string));
   strcpy(dynamic_string + len, normal_string);
   return dynamic_string;
}



