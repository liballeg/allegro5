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
 *      Makedoc's devhelp output routines.
 *
 *      By Elias Pschernig, based on makechm.
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

#include "makedevh.h"
#include "makehtml.h"
#include "makemisc.h"
#include "makechm.h"
#include "makedoc.h"

#define ALT_TEXT(toc)   ((toc->alt) ? toc->alt : toc->text)
enum PREV { PREV_NO, PREV_ROOT, PREV_SUB, PREV_CHAP };
enum TYPE { TYPE_ROOT1, TYPE_ROOT2, TYPE_SUB1, TYPE_SUB2, TYPE_FUNC };

/* Buffered tocs. Only the 'local' pointer should be freed */
typedef struct BTOC
{
   enum TYPE type;
   const char *name;
   char *local;
} BTOC;

extern const char *html_extension;

static void _write_object(FILE *file, const char *name, const char *local, int open);
static void _output_btoc(FILE *file, BTOC *btoc, int *num_btoc);
static int _qsort_btoc_helper(const void *e1, const void *e2);



/* _output_btoc:
 * Sorts all the buffered tocs read so far and frees their memory after
 * writting them to the file. Can be safely called at any moment, as long
 * as the pointer to num_btoc is meaningfull and contains the number of
 * buffered tocs in the btoc table.
 */
static void _output_btoc(FILE *file, BTOC *btoc, int *num_btoc)
{
   int num;

   assert(btoc);
   assert(num_btoc);

   qsort(btoc, *num_btoc, sizeof(BTOC), _qsort_btoc_helper);

   for(num = 0; num < *num_btoc; num++) {
      _write_object(file, btoc[num].name, btoc[num].local, btoc->type);
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
static void _write_object(FILE *file, const char *name, const char *local, int type)
{
   assert(file);
   assert(name);
   assert(local);

   fprintf(file, "%s name=\"%s\" link=\"%s\"%s>\n",
      type == TYPE_ROOT1 ? "  <sub" :
      (type == TYPE_SUB1 || type == TYPE_ROOT2) ? "    <sub" :
      type == TYPE_SUB2 ? "      <sub" : "  <function",
      name, local, (type == TYPE_ROOT1 || type == TYPE_ROOT2) ? "" : "/");
}



static char *devhelp_blank_page(char const *name, char const *filename, int num)
{
   char str[256];
   char *path;
   FILE *file;
   
   sprintf(str, "chapter%i.html", num);
   path = m_replace_filename (filename, str);
   printf("Writing %s.\n", path);
   
   file = fopen(path, "w");
   if (file) {
      /* Ideally, this would use Allegro's common HTML style and header
         and footer. */
      fprintf(file, "<html>\n<head><title>%s</title></head>\n<body>\n", name);
      fprintf(file, "<h1>%s</h1>", name);
      fprintf(file, "<p>Use the table of contents to navigate through this chapter.");
      fprintf(file, "</body>\n</html>\n");
      fclose(file);
   }
   return path;
}



/* write_devhelp:
 */
int write_devhelp(const char *filename)
{
   BTOC btoc[TOC_SIZE];
   FILE *file;
   TOC *toc;
   char *dest_name;
   int section_number = -1, num_btoc = 0;
   enum PREV prev = PREV_NO;
   int in_chapter = 0;
   int chapter_num = 1;

   if (!strcmp(get_extension(filename), "htm"))
      html_extension = "html";

   dest_name = m_replace_extension(filename, "devhelp");
   printf("writing '%s'\n", dest_name);

   file = fopen(dest_name, "wt");
   if (!file)
      return 1;
   free(dest_name);
   
   fprintf(file, "<?xml version=\"1.0\"?>\n");
   fprintf(file, "\n");
   fprintf(file, "<book xmlns=\"http://www.devhelp.net/book\"\n");
   fprintf(file, "      title=\"Allegro Game Library\"\n");
   fprintf(file, "      name=\"allegro\"\n");
   fprintf(file, "      link=\"allegro.html\">\n");
   fprintf(file, "\n");
   fprintf(file, "<chapters>\n");

   toc = tochead;
   if (toc)
      toc = toc->next;

   for (; toc; toc = toc->next) {
      char name[256];

      if (toc->htmlable || toc->root > 1) {
	 if (toc->root ) {

	    /* Write previous section. */
	    _output_btoc(file, btoc, &num_btoc);

	    if (prev == PREV_ROOT || prev == PREV_SUB)
	       fprintf(file, "%s</sub>\n", in_chapter ? "    " : "  ");

            if (toc->root == 2 || toc->root == 3) {
	       if (in_chapter)
		  fprintf(file, "  </sub>\n");
	       if (toc->root == 2) {
		  char *path;
		  path = devhelp_blank_page(ALT_TEXT(toc), filename, chapter_num++);
		  strcpy(name, get_filename(path));
		  free(path);
	       }
	       in_chapter = 0;
	    }
	    else if (toc->otherfile) {
	       sprintf(name, "%.240s.%.10s", toc->text, html_extension);
	    }
	    else {
	       section_number++;
	       get_section_filename(name, filename, section_number);
	    }

	    if (toc->root == 3) {
	       prev = PREV_NO;
	    }
	    else {

	       prev = toc->root == 1 ? PREV_ROOT : PREV_CHAP;

	       _write_object(file, ALT_TEXT(toc), name, in_chapter ? TYPE_ROOT2 : TYPE_ROOT1);

	       if (toc->root == 2)
		  in_chapter = 1;
	    }
	 }
	 else {

	    get_section_filename(name, filename, section_number);
	    strcat(name, "#");
	    strcat(name, toc->text);

	    prev = PREV_SUB;

	    /* Buffer toc for posterior output */
            btoc[num_btoc].type = in_chapter ? TYPE_SUB2 : TYPE_SUB1;
	    btoc[num_btoc].local = m_strdup(name);
	    btoc[num_btoc++].name = ALT_TEXT(toc);
	 }
      }
   }

   _output_btoc(file, btoc, &num_btoc);

   if (prev == PREV_SUB || prev == PREV_ROOT)
	       fprintf(file, "%s</sub>\n", in_chapter ? "    " : "  ");
   if (in_chapter)
      fprintf(file, "  </sub>\n");

   fprintf(file, "</chapters>\n");

   fprintf(file, "\n");
   fprintf(file, "<functions>\n");

   toc = tochead;
   if (toc)
      toc = toc->next;

   section_number = -1;
   for (; toc; toc = toc->next) {
      char name[256];
      if (!toc->htmlable)
         continue;
      if (toc->root) {
         if (!toc->otherfile) {
	    section_number++;
	 }
      } else {
         get_section_filename(name, filename, section_number);
	 strcat(name, "#");
	 strcat(name, toc->text);
         btoc[num_btoc].type = TYPE_FUNC;
         btoc[num_btoc].local = m_strdup(name);
	 btoc[num_btoc++].name = ALT_TEXT(toc);
      }
   }
   _output_btoc(file, btoc, &num_btoc);

   fprintf(file, "</functions>\n");
   fprintf(file, "\n");
   fprintf(file, "</book>\n");

   fclose(file);

   return 0;
}
