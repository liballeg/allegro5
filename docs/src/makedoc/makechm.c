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
 *      Makedoc's chm output routines.
 *
 *      By Elias Pschernig, based on makehtml.
 *
 *      See readme.txt for copyright information.
 *
 *      See allegro/docs/src/makedoc/format.txt for a brief description of
 *      the source of _tx files.
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "makechm.h"
#include "makemisc.h"
#include "makedoc.h"

#define ALT_TEXT(toc)   ((toc->alt) ? toc->alt : toc->text)
#define PREV_ROOT 1
#define PREV_SUB  2

extern const char *html_extension;



/* get_stripped_filename:
 *  Copies filename without path and extension to buf.
 */
static void get_stripped_filename(char *buf, char *filename, int size)
{
   int i = 0;
   char *p;

   for (p = get_filename(filename); (*p && (*p != '.')); p++) {
      buf[i++] = *p;
      if (i >= size-1) {
	 fprintf(stderr, "buffer overflow");
	 exit(1);
      }
   }
   buf[i] = 0;
}



/* _get_section_name:
 */
static char *_get_section_name(char *buf, const char *path, int section_number)
{
   int len;
   char *filename;

   filename = get_filename((char *)path);

   strncpy(buf, filename, 5);

   len = strlen(filename);
   if (len > 5)
      len = 5;

   sprintf(buf+len, "%03d.%s", section_number, html_extension);
   return buf;
}



/* _write_toc:
 * Outputs a .hhc or .hhk file which are needed by the HTML Help compiler
 * (along with a .hhp and the standard HTML docs) to generate the CHM docs.
 */
static int _write_toc(const char *filename, int is_index)
{
   FILE *file = fopen(filename, "w");
   TOC *toc;
   int section_number = -1;
   int prev = 0;
   if (!file)
      return 1;

   fprintf(file, "<html>\n");
   fprintf(file, "<head>\n");
   fprintf(file, "<title>\n");
   if (is_index)
      fprintf(file, "Index\n");
   else
      fprintf(file, "Contents\n");
   fprintf(file, "</title>\n");
   fprintf(file, "</head>\n");

   fprintf(file, "<body>\n");

   fprintf(file, "<ul>\n");

   toc = tochead;
   if (toc)
      toc = toc->next;

   for (; toc; toc = toc->next) {
      char name[256];

      if (toc->htmlable) {
	 if (toc->root) {

	    if (is_index) {
	       fprintf(file, "</li>\n");
	    }
	    else {
	       if (prev == PREV_SUB)
		  fprintf(file, "</li></ul></li>\n");
	       if (prev == PREV_ROOT)
		  fprintf(file, "</li>\n");
	    }

	    if (toc->otherfile) {
	       sprintf(name, "%s.%s", toc->text, html_extension);
	    }
	    else {
	       section_number++;
	       _get_section_name(name, filename, section_number);
	    }

	    fprintf(file, "<li>");
	    prev = PREV_ROOT;
	 }
	 else {

	    if (is_index) {
	       fprintf(file, "</li>\n");
	    }
	    else {
	       if (prev == PREV_SUB)
		  fprintf(file, "</li>\n");
	       if (prev == PREV_ROOT)
		  fprintf(file, "<ul>\n");
	    }

	    _get_section_name(name, filename, section_number);
	    strcat(name, "#");
	    strcat(name, toc->text);

	    fprintf(file, "<li>");
	    prev = PREV_SUB;
	 }

	 mystrlwr(name);

	 fprintf(file, "<object type=\"text/sitemap\">\n");

	 fprintf(file, "<param name=\"Name\" value=\"%s\">\n", ALT_TEXT(toc));
	 fprintf(file, "<param name=\"Local\" value=\"%s\">\n", name);

	 fprintf(file, "</object>\n");
      }

   }

   if (prev == PREV_SUB)
      fprintf(file, "</li></ul></li>\n");
   if (prev == PREV_ROOT)
      fprintf(file, "</li>\n");

   fprintf(file, "</ul>\n");

   fprintf(file, "</body>\n");
   fprintf(file, "</html>\n");

   fclose(file);

   return 0;
}



/* _write_hhp:
 */
static int _write_hhp(const char *filename)
{
   FILE *file = fopen(filename, "w");
   TOC *toc;
   int section_number = -1;
   char stripped_filename[256];
   char title[256];

   if (!file)
      return 1;

   get_stripped_filename(stripped_filename, (char *)filename,
                         sizeof(stripped_filename));
   strcpy(title, stripped_filename);
   title[0] = toupper(((unsigned char *)title)[0]);

   fprintf(file, "[OPTIONS]\n");
   fprintf(file, "Compiled file=%s.chm\n", stripped_filename);
   fprintf(file, "Contents file=%s.hhc\n", stripped_filename);
   fprintf(file, "Default topic=%s.%s\n", stripped_filename, html_extension);
   fprintf(file, "Full-text search=Yes\n");
   fprintf(file, "Index file=%s.hhk\n", stripped_filename);
   fprintf(file, "Language=0x409 English (USA)\n");
   fprintf(file, "Title=%s\n", title);
   fprintf(file, "\n");
   fprintf(file, "[FILES]\n");

   toc = tochead;
   if (toc)
      toc = toc->next;

   for (; toc; toc = toc->next) {
      if (toc->htmlable && toc->root) {
	 char name[256];

	 if (toc->otherfile) {
	    sprintf(name, "%s.%s", toc->text, html_extension);
	 }
	 else {
	    section_number++;
	    _get_section_name(name, filename, section_number);
	 }
	 mystrlwr(name);

	 fprintf(file, "%s\n", name);

      }
   }
   return 0;
}



/* _replace_extension:
 *  Replaces extension in path with different one.
 */
static void _replace_extension(char *dest, char *path, char *ext, int size)
{
   char *ext_p;
   int pos;

   ext_p = strrchr(path, '.');
   if (ext_p == NULL)
      ext_p = path;
   else
      ext_p++;
   pos = ext_p - path;

   if (pos+strlen(ext)+1 > size) {
      fprintf(stderr, "Buffer overfull");
      exit(1);
   }

   memcpy(dest, path, pos);
   strcpy(dest+pos, ext);
}



/* write_chm:
 */
int write_chm(char *filename)
{
   FILE *file;
   int found_signature = 0;
   char buf[1024];
   
   if (!strcmp(extension(filename), "htm"))
      html_extension = "html";

   if (!(flags & MULTIFILE_FLAG)) {
      printf("The chm version of the documentation requires that you generate \n");
      printf("multifile output in the HTML version. Please delete the generated\n");
      printf("HTML files and put @multiplefiles in allegro._tx.\n\n");
      return 1;
   }
      
   system("hhc.exe > tempfile");
   file = fopen("tempfile", "r");
   if (file) {
      char buf[256];
      int n = fread(buf, 1, 255, file);
      buf[n] = 0;
      fclose(file);
      if (strstr(buf, "HTML Help"))
	 found_signature++;
   }
   remove("tempfile");
   
   if (!found_signature) {
      printf("\nCannot find the HTML Help Compiler necessary to generate .chm output.\n");
      printf("You can try to obtain it from Microsoft:\n");
      printf("http://msdn.microsoft.com/library/en-us/htmlhelp/html/htmlhelp.exe\n\n");
      return 1;
   }

   _replace_extension(buf, filename, "hhp", sizeof(buf));
   printf("writing '%s'\n", buf);
   _write_hhp(buf);
   _replace_extension(buf, filename, "hhc", sizeof(buf));
   printf("writing '%s'\n", buf);
   _write_toc(buf, 0);
   _replace_extension(buf, filename, "hhk", sizeof(buf));
   printf("writing '%s'\n", buf);
   _write_toc(buf, 1);

   return 0;
}

