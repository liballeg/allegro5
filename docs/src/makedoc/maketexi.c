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
 *      Makedoc's texinfo output routines.
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
#include <stdlib.h>

#include "makertf.h"
#include "makedoc.h"
#include "makemisc.h"


int multiwordheaders = 0;

static int _no_strip;
static int _strip_indent = -1;


static void _write_textinfo_xref(FILE *f, char *xref);
static int _valid_texinfo_node(char *s, char **next, char **prev);
static void _output_texinfo_toc(FILE *f, int root, int body, int part);
static char *_node_name(int i);
static char *_first_word(char *s);
static void _html_to_texinfo(FILE *f, char *p);



/* write_texinfo:
 */
int write_texinfo(char *filename)
{
   char buf[256];
   LINE *line = head, *title_line = 0;
   char *p, *str, *next, *prev;
   char *xref[256];
   int xrefs = 0;
   int in_item = 0;
   int section_number = 0;
   int toc_waiting = 0;
   int continue_def = 0;
   int title_pass = 0;
   int i;
   FILE *f;

   printf("writing %s\n", filename);

   f = fopen(filename, "w");
   if (!f)
      return 1;

   while (line) {
      if (line->flags & TEXINFO_FLAG) {
	 p = line->text;

	 /* end of an ftable? */
	 if (in_item) {
	    if ((*p) && (*p != ' ') && (*p != '<')) {
	       fputs("@end ftable\n", f);
	       in_item = 0;
	    }
	 }

	 if (!in_item) {
	    /* start a new node? */
	    str = strstr(p, "<a name=\"");
	    if (str > p) {
	       str += strlen("<a name=\"");
	       for (i=0; str[i] && str[i] != '"'; i++)
		  buf[i] = str[i];
	       buf[i] = 0;

	       if (!(line->flags & NONODE_FLAG) &&
		   (_valid_texinfo_node(buf, &next, &prev))) {
		  if (toc_waiting) {
		     _output_texinfo_toc(f, 0, 1, section_number-1);
		     toc_waiting = 0;
		  }
		  if (xrefs > 0) {
		     fputs("See also:@*\n", f);
		     for (i=0; i<xrefs; i++)
			_write_textinfo_xref(f, xref[i]);
		     xrefs = 0;
		  }
		  fprintf(f, "@node %s, %s, %s, %s\n", buf, next, prev, _node_name(section_number-1));
		  fprintf(f, "@section %s\n", buf);
	       }

	       fputs("@ftable @asis\n", f);
	       in_item = 1;
	    }
	 }

	 /* munge the indentation to make it look nicer */
	 if (!(line->flags & NO_INDENT_FLAG)) {
	    if (_no_strip) {
	       if (_strip_indent >= 0) {
		  for (i=0; (i<_strip_indent) && (*p) && (myisspace(*p)); i++)
		     p++;
	       }
	       else {
		  if (!is_empty(p)) {
		     _strip_indent = 0;
		     while ((*p) && (myisspace(*p))) {
			_strip_indent++;
			p++;
		     }
		  }
	       }
	    }
	    else {
	       while ((*p) && (myisspace(*p)))
		  p++;
	    }
	 }

	 if (line->flags & TEXINFO_CMD_FLAG) {
	    /* raw output of texinfo commands */
	    fputs(p, f);
	    fputs("\n", f);
	 }
	 else if (line->flags & HTML_CMD_FLAG) {
	    /* process HTML commands */
	    while (*p) {
	       if (p[0] == '<') {
		  _html_to_texinfo(f, p+1);
		  while ((*p) && (*p != '>'))
		     p++;
		  if (*p)
		     p++;
	       }
	       else
		  p++;
	    }
	 }
	 else if (line->flags & HEADING_FLAG) {
	    /* output a section heading */
	    if (section_number > 0) {
	       if (in_item) {
		  fputs("@end ftable\n", f);
		  in_item = 0;
	       }
	       if (xrefs > 0) {
		  fputs("See also:@*\n", f);
		  for (i=0; i<xrefs; i++)
		     _write_textinfo_xref(f, xref[i]);
		  xrefs = 0;
	       }
	       p = strip_html(p);
	       fprintf(f, "@node %s, ", _node_name(section_number));
	       fprintf(f, "%s, ", _node_name(section_number+1));
	       fprintf(f, "%s, Top\n", _node_name(section_number-1));
	       fprintf(f, "@chapter %s\n", p);
	       if (!(line->flags & NOCONTENT_FLAG))
		  toc_waiting = 1;
	    }
	    section_number++;
	 }
	 else {
	    if ((line->flags & DEFINITION_FLAG) && (!continue_def))
	       fputs("@item @t{", f);

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
	       /* process other HTML tokens */
	       else if (p[0] == '<') {
		  _html_to_texinfo(f, p+1);
		  while ((*p) && (*p != '>'))
		     p++;
		  if (*p)
		     p++;
	       }
	       /* output other characters */
	       else {
		  if ((*p == '{') || (*p == '}') || (*p == '@'))
		     fputc('@', f);
		  fputc((unsigned char)*p, f);
		  p++;
	       }
	    }

	    if (line->flags & CONTINUE_FLAG)
	       fputs(" ", f);
	    else {
	       if (line->flags & DEFINITION_FLAG)
		  fputs("}", f);
	       fputs("\n", f);
	    }
	 }
      }
      else if (line->flags & NODE_FLAG) {
	 if (in_item) {
	    fputs("@end ftable\n", f);
	    if (toc_waiting) {
	       _output_texinfo_toc(f, 0, 1, section_number-1);
	       toc_waiting = 0;
	    }
	 }

	 if (_valid_texinfo_node(line->text, &next, &prev)) {
	    if (xrefs > 0) {
	       fputs("See also:@*\n", f);
	       for (i=0; i<xrefs; i++)
		  _write_textinfo_xref(f, xref[i]);
	       xrefs = 0;
	    }
	    fprintf(f, "@node %s, %s, %s, %s\n", line->text, next, prev, _node_name(section_number-1));
	    fprintf(f, "@section %s\n", line->text);
	 }

	 fputs("@ftable @asis\n", f);
	 in_item = 1;
      }
      else if (line->flags & TOC_FLAG) {
	 _output_texinfo_toc(f, 1, 0, 0);
      }
      else if (line->flags & INDEX_FLAG) {
	 _output_texinfo_toc(f, 0, 1, 0);
      }
      else if (line->flags & XREF_FLAG) {
	 xref[xrefs++] = line->text;
      }
      else if (line->flags & START_TITLE_FLAG) {
	 /* remember where the title starts */
	 title_line = line;
	 if (!title_pass)
	    fputs("@titlepage\n", f);
      }
      else if (line->flags & END_TITLE_FLAG) {
	 if (!title_pass)
	 {
	    title_pass++;
	    fputs("@end titlepage\n@ifinfo\n", f);
	    line = title_line;
	 }
	 else
	    fputs("@end ifinfo\n", f);
      }

      continue_def = (line->flags & CONTINUE_FLAG);
      line = line->next;
   }

   fclose(f);
   return 0;
}



/* _write_textinfo_xref:
 */
static void _write_textinfo_xref(FILE *f, char *xref)
{
   char *tok;

   tok = strtok(xref, ",;");

   while (tok) {
      while ((*tok) && (myisspace(*tok)))
	 tok++;
      fprintf(f, "@xref{%s}.@*\n", tok);
      tok = strtok(NULL, ",;");
   }
}



/* _valid_texinfo_node:
 */
static int _valid_texinfo_node(char *s, char **next, char **prev)
{
   TOC *toc = tochead;

   *next = *prev = "";

   while (toc) {
      if ((!toc->root) && (toc->texinfoable)) {
	 if (mystricmp(toc->text, s) == 0) {
	    do {
	       toc = toc->next;
	    } while ((toc) && ((!toc->texinfoable) || (toc->root)));

	    if (toc)
	       *next = toc->text;

	    return 1;
	 }

	 *prev = toc->text;
      }

      toc = toc->next;
   }

   return 0;
}



/* _output_texinfo_toc:
 */
static void _output_texinfo_toc(FILE *f, int root, int body, int part)
{
   TOC *toc;
   int section_number;
   char **ptr;
   char *s;
   int i, j;

   fprintf(f, "@menu\n");

   if (root) {
      toc = tochead;
      if (toc)
	 toc = toc->next;

      while (toc) {
	 if ((toc->root) && (toc->texinfoable)) {
	    s = _first_word(toc->text);
	    fprintf(f, "* %s::", s);
	    for (i=strlen(s); i<24; i++)
	       fputc(' ', f);
	    fprintf(f, "%s\n", toc->text);
	 }
	 toc = toc->next;
      }
   }

   if (body) {
      toc = tochead;
      if (toc)
	 toc = toc->next;

      if (part <= 0) {
	 ptr = m_xmalloc(TOC_SIZE * sizeof(char *));
	 i = 0;

	 while (toc) {
	    if ((!toc->root) && (toc->texinfoable) && (i < TOC_SIZE))
	       ptr[i++] = toc->text;

	    toc = toc->next;
	 }

	 if (i > 1)
	    qsort(ptr, i, sizeof(char *), scmp);

	 for (j=0; j<i; j++)
	    fprintf(f, "* %s::\n", ptr[j]);

	 free(ptr);
      }
      else {
	 section_number = 0;

	 while ((toc) && (section_number < part)) {
	    if ((toc->root) && (!toc->otherfile))
	       section_number++;
	    toc = toc->next;
	 }

	 while ((toc) && (!toc->root)) {
	    if (toc->texinfoable)
	       fprintf(f, "* %s::\n", toc->text);
	    toc = toc->next;
	 }
      }
   }

   fprintf(f, "@end menu\n");
}



/* _node_name:
 */
static char *_node_name(int i)
{
   TOC *toc = tochead;

   if (toc)
      toc = toc->next;

   while (toc) {
      if ((toc->root) && (toc->texinfoable)) {
	 i--;
	 if (!i)
	    return _first_word(toc->text);
      }

      toc = toc->next;
   }

   return "";
}



/* _first_word:
 */
static char *_first_word(char *s)
{
   static char buf[256];
   int i;

   if (multiwordheaders)
      return strncpy(buf, s, 255);

   for (i=0; s[i] && s[i] != ' '; i++)
      buf[i] = s[i];

   buf[i] = 0;
   return buf;
}



/* _html_to_texinfo:
 */
static void _html_to_texinfo(FILE *f, char *p)
{
   char buf[256];
   int i = 0;

   while ((*p) && (*p != '>'))
      buf[i++] = *(p++);

   buf[i] = 0;

   if (mystricmp(buf, "pre") == 0) {
      fputs("@example\n", f);
      _no_strip = 1;
      _strip_indent = -1;
   }
   else if (mystricmp(buf, "/pre") == 0) {
      fputs("@end example\n", f);
      _no_strip = 0;
      _strip_indent = -1;
   }
   else if (mystricmp(buf, "br") == 0) {
      fputs("@*", f);
   }
   else if (mystricmp(buf, "hr") == 0) {
      fputs("----------------------------------------------------------------------", f);
   }
   else if (mystricmp(buf, "p") == 0) {
      fputs("\n", f);
   }
   else if (mystricmp(buf, "ul") == 0) {
      fputs("\n@itemize @bullet\n", f);
   }
   else if (mystricmp(buf, "/ul") == 0) {
      fputs("@end itemize\n", f);
   }
   else if (mystricmp(buf, "li") == 0) {
      fputs("@item ", f);
   }
}




