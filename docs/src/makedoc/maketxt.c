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
 *      Makedoc's txt output routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 *
 *      See allegro/docs/src/makedoc/format.txt for a brief description of
 *      the source of _tx files.
 */

#include <stdio.h>
#include <string.h>

#include "maketxt.h"
#include "makedoc.h"
#include "makemisc.h"



/* write_txt:
 */
int write_txt(char *filename, int partial)
{
   LINE *line = head;
   char *p;
   int c, len;
   FILE *f;
   int outputting = !partial;

   /*printf("writing %s\n", filename);*/

   f = fopen(filename, "w");
   if (!f)
      return 1;

   while (line) {
      if (line->flags & (STARTOUTPUT_FLAG | ENDOUTPUT_FLAG)) {
	 if (mystricmp(line->text, get_filename(filename)) == 0) {
	    if (line->flags & STARTOUTPUT_FLAG)
	       outputting = 1;
	    else
	       outputting = 0;
	 }
      }
      else if (line->flags & NODE_FLAG) {
	 p = line->text;
	 len = strlen(p);
	 fputs(p, f);
	 fputs("\n", f);
	 for (c=0; c<len; c++)
	    fputc('-', f);
	 fputs("\n\n", f);
      }
      else if ((line->flags & TEXT_FLAG) && (outputting)) {
	 p = line->text;

	 if (line->flags & HEADING_FLAG) {
	    /* output a section heading */
	    p = strip_html(p);
	    len = strlen(p);
	    for (c=0; c<len+26; c++)
	       fputc('=', f);
	    fputs("\n============ ", f);
	    fputs(p, f);
	    fputs(" ============\n", f);
	    for (c=0; c<len+26; c++)
	       fputc('=', f);
	    fputs("\n", f);
	 }
	 else {
	    while (*p) {
	       /* less-than HTML tokens */
	       if ((p[0] == '&') && 
		   (mytolower(p[1]) == 'l') && 
		   (mytolower(p[2]) == 't')) {
		  fputc('<', f);
		  p += 3;
	       }
	       /* greater-than HTML tokens */
	       else if ((p[0] == '&') && 
			(mytolower(p[1]) == 'g') && 
			(mytolower(p[2]) == 't')) {
		  fputc('>', f);
		  p += 3;
	       }
	       /* strip other HTML tokens */
	       else if (p[0] == '<') {
		  while ((*p) && (*p != '>'))
		     p++;
		  if (*p)
		     p++;
	       }
	       /* output other characters */
	       else {
		  fputc(*p, f);
		  p++;
	       }
	    }
	    fputs("\n", f);
	 }
      }

      line = line->next;
   }

   fclose(f);
   return 0;
}




