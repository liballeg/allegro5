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
 *      Grzegorz Adam Hankiewicz made sorted TOCs.
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
#include <assert.h>

#include "makechm.h"
#include "makehtml.h"
#include "makemisc.h"
#include "makedoc.h"

#define ALT_TEXT(toc)   ((toc->alt) ? toc->alt : toc->text)
#define PREV_ROOT 1
#define PREV_SUB  2

/* Buffered tocs. Only the 'local' pointer should be freed */
typedef struct BTOC
{
   const char *name;
   char *local;
} BTOC;

extern const char *html_extension;


static int _write_toc(const char *filename, int is_index);
static int _write_hhp(const char *filename);
static void _write_object(FILE *file, const char *name, const char *local);
static void _output_btoc(FILE *file, BTOC *btoc, int *btoc_prev, int *num_btoc);
static int _qsort_btoc_helper(const void *e1, const void *e2);



/* _write_toc:
 * Outputs a .hhc or .hhk file which are needed by the HTML Help compiler
 * (along with a .hhp and the standard HTML docs) to generate the CHM docs.
 * is_index is a boolean, and tells if the output file should be a chm
 * index file. The main difference is that the internal structure is
 * slightly different, and it doesn't need toc sorting.
 */
static int _write_toc(const char *filename, int is_index)
{
   BTOC btoc[TOC_SIZE];
   FILE *file = fopen(filename, "wt");
   TOC *toc;
   int btoc_prev[TOC_SIZE];
   int section_number = -1, prev = 0, num_btoc = 0;
   int in_chapter = 0;
   
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
	       _output_btoc(file, btoc, btoc_prev, &num_btoc);
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
	       get_section_filename(name, filename, section_number);
	    }

	    prev = PREV_ROOT;

	    _write_object(file, ALT_TEXT(toc), mystrlwr(name));
	 }
	 else {

	    if (is_index) {
	       fprintf(file, "</li>\n");
	    }
	    else {
	       btoc_prev[num_btoc] = prev;
	    }

	    get_section_filename(name, filename, section_number);
	    strcat(name, "#");
	    strcat(name, toc->text);

	    prev = PREV_SUB;
	    if(!is_index) {
	       /* Buffer toc for posterior output */
	       btoc[num_btoc].local = m_strdup(mystrlwr(name));
	       btoc[num_btoc++].name = ALT_TEXT(toc);
	    }
	    else
	       _write_object(file, ALT_TEXT(toc), name);
	 }
      }
      else if ((toc->root == 2 || toc->root == 3) && !is_index) {
         _output_btoc(file, btoc, btoc_prev, &num_btoc);
	 if (prev == PREV_SUB)
	    fprintf(file, "</li></ul></li>\n");
         if (prev == PREV_ROOT)
	    fprintf(file, "</li>\n");
         if (in_chapter) {
            fprintf(file, "</ul>\n");
            in_chapter = 0;
         }
         if (toc->root == 2) {
            fprintf(file, "<li><object type=\"text/sitemap\">\n");
            fprintf(file, "<param name=\"Name\" value=\"%s\">\n", ALT_TEXT(toc));
            fprintf(file, "</object>\n</li>\n");
            fprintf(file, "<ul>\n");
            in_chapter = 1;
         }
         prev = 0;
      }
   }

   if (in_chapter) {
      fprintf(file, "</ul>\n");
   }

   _output_btoc(file, btoc, btoc_prev, &num_btoc);
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



/* _output_btoc:
 * Sorts all the buffered tocs read so far and frees their memory after
 * writting them to the file. Can be safely called at any moment, as long
 * as the pointer to num_btoc is meaningfull and contains the number of
 * buffered tocs in the btoc table. Note that btoc_prev is NOT sorted.
 */
static void _output_btoc(FILE *file, BTOC *btoc, int *btoc_prev, int *num_btoc)
{
   int num;

   assert(btoc);
   assert(num_btoc);
   assert(btoc_prev);

   qsort(btoc, *num_btoc, sizeof(BTOC), _qsort_btoc_helper);

   for(num = 0; num < *num_btoc; num++) {
      if (btoc_prev[num] == PREV_SUB)
	 fprintf(file, "</li>\n");
      if (btoc_prev[num] == PREV_ROOT)
	 fprintf(file, "<ul>\n");

      _write_object(file, btoc[num].name, btoc[num].local);
      free(btoc[num].local);
   }
   *num_btoc = 0;
}



/* _qsort_btoc_helper:
 * qsort helper. BTOC elements are sorted through the name value.
 */
static int _qsort_btoc_helper(const void *e1, const void *e2)
{
   BTOC *s1 = (BTOC *)e1;
   BTOC *s2 = (BTOC *)e2;

   return mystricmp(s1->name, s2->name);
}



/* _write_object:
 * Writes to the file a record for a new object. Note that the object
 * record is not closed, so other's can be nested. This forces the caller
 * to close the object before writting another.
 */
static void _write_object(FILE *file, const char *name, const char *local)
{
   assert(file);
   assert(name);
   assert(local);
   
   fprintf(file, "<li><object type=\"text/sitemap\">\n");
   fprintf(file, "<param name=\"Name\" value=\"%s\">\n", name);
   fprintf(file, "<param name=\"Local\" value=\"%s\">\n", local);
   fprintf(file, "</object>\n");
}



/* _write_hhp:
 */
static int _write_hhp(const char *filename)
{        
   FILE *file = fopen(filename, "wt");
   TOC *toc;
   int section_number = -1;
   char *stripped_filename, *p;

   if (!file)
      return 1;

   stripped_filename = m_strdup(get_filename(filename));
   if ((p = get_extension(stripped_filename)))
      *(p-1) = 0;

   fprintf(file, "[OPTIONS]\n");
   fprintf(file, "Compiled file=%s.chm\n", stripped_filename);
   fprintf(file, "Contents file=%s.hhc\n", stripped_filename);
   fprintf(file, "Default topic=%s.%s\n", stripped_filename, html_extension);
   fprintf(file, "Full-text search=Yes\n");
   fprintf(file, "Index file=%s.hhk\n", stripped_filename);
   fprintf(file, "Language=0x409 English (USA)\n");
   if((html_flags & HTML_DOCUMENT_TITLE_FLAG) && !(html_flags & HTML_OLD_H_TAG_FLAG))
      fprintf(file, "Title=%s\n", html_document_title);
   else {
      /* Write the filename without extension and capitalized */
      fputc(toupper(*stripped_filename), file);
      fprintf(file, "%s", stripped_filename+1);
   }
   fprintf(file, "\n");
   fprintf(file, "[FILES]\n");

   free(stripped_filename);

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
	    get_section_filename(name, filename, section_number);
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
   char *temp;

   if (!strcmp(get_extension(filename), "htm"))
      html_extension = "html";

   if (!(flags & MULTIFILE_FLAG)) {
      printf("The chm version of the documentation requires that you generate \n");
      printf("multifile output in the HTML version. Please delete the generated\n");
      printf("HTML files and put @multiplefiles in allegro._tx.\n\n");
      return 1;
   }

   temp = m_replace_extension(filename, "hhp");
   printf("writing '%s'\n", temp);
   if (_write_hhp(temp))
      return 1;
   free(temp);

   temp = m_replace_extension(filename, "hhc");
   printf("writing '%s'\n", temp);
   _write_toc(temp, 0);
   free(temp);

   temp = m_replace_extension(filename, "hhk");
   printf("writing '%s'\n", temp);
   _write_toc(temp, 1);
   free(temp);

   return 0;
}



/* get_section_filename:
 * Extracts the filename from path and transforms it to the format
 * 5+3.ext, where 5 characters are taken from path, next three represent
 * the section number, and ext is the html extension. buf shuold have
 * enough space to hold 16 characters. Returns a pointer to buf.
 */
char *get_section_filename(char *buf, const char *path, int section_number)
{
   int len;
   assert(buf);
   assert(path);
   assert(section_number >= 0);

   len = strlen(strncpy(buf, get_filename(path), 5));
   if (len > 5)
      len = 5;

   sprintf(buf+len, "%03d.%s", section_number, html_extension);
   return buf;
}

