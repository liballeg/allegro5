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



/* _get_section_name:
 */
static char *_get_section_name(char *buf, int section_number)
{
   sprintf(buf, "alleg%03d.%s", section_number, html_extension);
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
	       _get_section_name(name, section_number);
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

	    _get_section_name(name, section_number);
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

   if (!file)
      return 1;

   fprintf(file, "[OPTIONS]\n");
   fprintf(file, "Compiled file=allegro.chm\n");
   fprintf(file, "Contents file=allegro.hhc\n");
   fprintf(file, "Default topic=allegro.%s\n", html_extension);
   fprintf(file, "Full-text search=Yes\n");
   fprintf(file, "Index file=allegro.hhk\n");
   fprintf(file, "Language=0x409 Englisch (USA)\n");
   fprintf(file, "Title=Allegro\n");
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
	    _get_section_name(name, section_number);
	 }
	 mystrlwr(name);

	 fprintf(file, "%s\n", name);

      }
   }
   return 0;
}



/* write_chm:
 */
int write_chm(char *filename)
{
   FILE *file;
   int found_signature = 0;
   
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

   printf("writing 'docs/html/allegro.hhp'\n");
   _write_hhp("docs/html/allegro.hhp");
   printf("writing 'docs/html/allegro.hhc'\n");
   _write_toc("docs/html/allegro.hhc", 0);
   printf("writing 'docs/html/allegro.hhk'\n");
   _write_toc("docs/html/allegro.hhk", 1);

   return 0;
}

