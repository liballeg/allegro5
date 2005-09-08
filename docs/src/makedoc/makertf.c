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
 *      Makedoc's rtf output routines.
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

#include "makertf.h"
#include "makedoc.h"
#include "makemisc.h"



char rtfheader[256] = "";
char *rtf_language_header;


/* write_rtf:
 */
int write_rtf(char *filename)
{
   LINE *line = head;
   LINE *l;
   char *p, *last = 0;
   FILE *f;
   int preformat = 0;
   int prevhead = 0;
   int prevdef = 0;
   int multidef = 0;
   int pardebt = 0;
   int parloan = 0;
   int indent = 0;
   int indef = 0;
   int weakdef = 0;
   int bullet = 0;
   int inbullet = 0;
   int tallbullets = 0;

   #define PAR()                                         \
   {                                                     \
      if (pardebt > 0)                                   \
	 pardebt--;                                      \
      else                                               \
	 fputs("\\par ", f);                             \
							 \
      while (inbullet > 0) {                             \
	 fputs("\\pard ", f);                            \
	 fprintf(f, "\\li%d ", indent*INDENT_SIZE);      \
	 inbullet--;                                     \
      }                                                  \
   }

   /*printf("writing %s\n", filename);*/

   f = fopen(filename, "w");
   if (!f)
      return 1;

   /* Fonts:
    *    0 - Times New Roman
    *    1 - Courier New
    *    2 - Symbol
    *
    * Colors:
    *    1 - Black
    *    2 - Red
    *    3 - Green
    *    4 - Blue
    *
    * Styles:
    *    0 - Normal
    *    1 - Quotation
    *    2 - Heading 1
    *    3 - Heading 2
    *    4 - Header
    *    5 - TOC 1
    *    6 - Index 1
    */

   #define NORMAL_STYLE       "\\f0\\fs20 "
   #define QUOTATION_STYLE    "\\f1\\fs18 "
   #define HEADING_STYLE      "\\f0\\fs48\\sa600\\pagebb\\keepn\\ul "
   #define DEFINITION_STYLE   "\\f0\\fs24\\sb200\\keepn\\sa200\\b "
   #define HEADER_STYLE       "\\f0\\fs20\\tqc\\tx4153\\tqr\\tx8306 "
   #define TOC_STYLE          "\\f0\\fs24\\tqr\\tldot\\tx8640 "
   #define INDEX_STYLE        "\\f0\\fs20\\tqr\\tldot\\tx8640 "

   #define INDENT_SIZE        400
   #define BULLET_INDENT      250

   fputs("{\\rtf\\ansi\\deff0\\widowctrl " NORMAL_STYLE "\n", f);
   fputs("{\\colortbl;\\red0\\green0\\blue0;\\red255\\green0\\blue0;\\red0\\green255\\blue0;\\red0\\green0\\blue255;}\n", f);

   if(rtf_language_header)
      fputs(rtf_language_header, f);
   else {
      fputs("{\\fonttbl{\\f0\\froman\\fcharset0\\fprq2 Times New Roman;}\n", f);
      fputs("{\\f1\\fmodern\\fcharset0\\fprq1 Courier New;}\n", f);
      fputs("{\\f2\\froman\\fcharset2\\fprq2 Symbol;}}\n", f);
   }

   fputs("{\\stylesheet {\\widctlpar " NORMAL_STYLE "\\snext0 Normal;}\n", f);
   fputs("{\\s1\\widctlpar " QUOTATION_STYLE "\\sbasedon0\\snext1 Quotation;}\n", f);
   fputs("{\\s2\\widctlpar " HEADING_STYLE "\\sbasedon0\\snext2 Heading 1;}\n", f);
   fputs("{\\s3\\widctlpar " DEFINITION_STYLE "\\sbasedon0\\snext3 Heading 2;}\n", f);
   fputs("{\\s4\\widctlpar " HEADER_STYLE "\\sbasedon0\\snext4 Header;}\n", f);
   fputs("{\\s5\\widctlpar " TOC_STYLE "\\sbasedon0\\snext0 TOC 1;}\n", f);
   fputs("{\\s6\\widctlpar " INDEX_STYLE "\\sbasedon0\\snext0 Index 1;}}\n", f);

   if (rtfheader[0]) {
      fputs("{\\header \\pard\\plain \\s4 " HEADER_STYLE "\\pvpara\\phmrg\\posxr\\posy0 page {\\field{\\*\\fldinst PAGE}{\\fldrslt 2}}\n", f);
      fputs("\\par \\pard \\s4\\ri360 " HEADER_STYLE "{\\i ", f);
      fputs(rtfheader, f);
      fputs("} \\pard}\n", f);
   }

   while (line) {
      if (line->flags & RTF_FLAG) {
	 p = line->text;

	 if (indef) {
	    /* end the indentation from the previous definition? */
	    if (weakdef) {
	       if (is_empty(strip_html(line->text))) {
		  fputs("\\par}", f);
		  pardebt++;
		  indent--;
		  indef = 0;
		  weakdef = 0;
	       }
	    }
	    else {
	       l = line;
	       while ((l->next) && l->text && (is_empty(strip_html(l->text))))
		  l = l->next;

	       if (l->flags & (HEADING_FLAG | DEFINITION_FLAG | NODE_FLAG)) {
		  fputs("\\par}", f);
		  pardebt++;
		  indent--;
		  indef = 0;
	       }
	    }
	 }

	 if (line->flags & HEADING_FLAG) {
	    /* start a section heading */
	    fputs("{\\s2 " HEADING_STYLE, f);
	 }
	 else if (line->flags & DEFINITION_FLAG) {
	    /* start a function definition */
	    if (prevdef) {
	       if (multidef) {
		  /* continued from previous line */
		  while ((p[1]) && (myisspace(p[1])))
		     p++;
	       }
	       else {
		  /* new definition, but flush with previous one */
		  fputs("{\\s3 " DEFINITION_STYLE "\\sb0 ", f);
	       }
	    }
	    else {
	       /* new function definition */
	       fputs("{\\s3 " DEFINITION_STYLE, f);
	    }
	    if (line->flags & CONTINUE_FLAG) {
	       /* this definition continues onto the next line */
	       multidef = 1;
	    }
	    else {
	       /* this paragraph should be flush with the next */
	       multidef = 0;
	       if ((line->next) && (line->next->flags & DEFINITION_FLAG))
		  fputs("\\sa0 ", f);
	    }
	    prevdef = 1;
	 }
	 else {
	    prevdef = 0;
	    multidef = 0;

	    if (!preformat) {
	       /* skip leading spaces */
	       while ((*p) && (myisspace(*p)))
		  p++;
	    }
	 }

	 while (*p) {
	    last = 0;
	    if (strincmp(p, "<p>") == 0) {
	       /* paragraph breaks */
	       PAR();
	       p += 3;
	    }
	    else if (strincmp(p, "<br>") == 0) {
	       /* line breaks */
	       PAR();
	       p += 4;
	    }
	    else if (strincmp(p, "<pre>") == 0) {
	       /* start preformatted text */
	       fputs("\\par {\\s1 " QUOTATION_STYLE, f);
	       while (inbullet > 0) {
		  fputs("\\pard ", f);
		  fprintf(f, "\\li%d ", indent*INDENT_SIZE);
		  inbullet--;
	       }
	       preformat = 1;
	       p += 5;
	    }
	    else if (strincmp(p, "</pre>") == 0) {
	       /* end preformatted text */
	       if (strincmp(line->text, "</pre>") == 0) {
		  fputs("}", f);
		  if (indent > (indef ? 1 : 0)) {
		     fputs("\\pard ", f); 
		     fprintf(f, "\\li%d ", indent*INDENT_SIZE); 
		  }
	       }
	       else {
		  fputs("\\par}", f);
		  pardebt++;
		  if (indent > (indef ? 1 : 0))
		     inbullet++;
	       }
	       preformat = 0;
	       p += 6;
	    }
	    else if (strincmp(p, "<title>") == 0) {
	       /* start document title */
	       fputs("{\\info{\\title ", f);
	       p += 7;
	    }
	    else if (strincmp(p, "</title>") == 0) {
	       /* end document title */
	       fputs("}{\\author Allegro makedoc utility}}\n", f);
	       p += 8;
	    }
	    else if (strincmp(p, "<hr>") == 0) {
	       /* section division */
	       if (strincmp(line->text, "</pre>"))
		  PAR();
	       fputs("\\brdrb\\brdrs\\brdrw15\\brsp20 \\par \\pard \\par ", f);
	       p += 4;
	    }
	    else if (strincmp(p, "<b>") == 0) {
	       /* start bold text */
	       fputs("{\\b ", f);
	       p += 3;
	    }
	    else if (strincmp(p, "</b>") == 0) {
	       /* end bold text */
	       fputs("\\par}", f);
	       pardebt++;
	       p += 4;
	    }
	    else if (strincmp(p, "<i>") == 0) {
	       /* start italic text */
	       fputs("{\\i ", f);
	       p += 3;
	    }
	    else if (strincmp(p, "</i>") == 0) {
	       /* end italic text */
	       fputs("\\par}", f);
	       pardebt++;
	       p += 4;
	    }
	    else if (strincmp(p, "<h1>") == 0) {
	       /* start heading text */
	       fputs("{\\f0\\fs48 ", f);
	       p += 4;
	    }
	    else if (strincmp(p, "</h1>") == 0) {
	       /* end heading text */
	       fputs("\\par}", f);
	       pardebt++;
	       p += 5;
	    }
	    else if (strincmp(p, "<h4>") == 0) {
	       /* start subheading text */
	       fputs("{\\f0\\fs24\\b ", f);
	       p += 4;
	    }
	    else if (strincmp(p, "</h4>") == 0) {
	       /* end subheading text */
	       fputs("\\par}", f);
	       pardebt++;
	       p += 5;
	    }
	    else if (strincmp(p, "<center>") == 0) {
	       /* start centered text */
	       fputs("\\par {\\qc ", f);
	       pardebt++;
	       p += 8;
	    }
	    else if (strincmp(p, "</center>") == 0) {
	       /* end centered text */
	       fputs("\\par}", f);
	       pardebt++;
	       p += 9;
	    }
	    else if (strincmp(p, "<a name=\"") == 0) {
	       /* begin index entry */
	       fputs("{\\xe\\v ", f);
	       p += 9;
	       while ((*p) && (*p != '"')) {
		  fputc((unsigned char)*p, f);
		  p++;
	       }
	       fputs("}", f);
	       if (*p)
		  p += 2;
	    }
	    else if (strincmp(p, "<ul>") == 0) {
	       /* start bullet list */
	       indent++;
	       fprintf(f, "\\par {\\li%d ", indent*INDENT_SIZE);
	       pardebt++;
	       p += 4;
	    }
	    else if (strincmp(p, "</ul>") == 0) {
	       /* end bullet list */
	       indent--;
	       if (indent == (indef ? 1 : 0))
		  tallbullets = 0;
	       else
		  inbullet++;
	       fputs("\\par}", f);
	       pardebt++;
	       p += 5;
	    }
	    else if (strincmp(p, "<li>") == 0) {
	       /* bullet item */
	       bullet = 1;
	       p += 4;
	    }
	    else if (*p == '<') {
	       /* skip unknown HTML tokens */
	       while ((*p) && (*p != '>'))
		  p++;
	       if (*p)
		  p++;
	    }
	    else if (strincmp(p, "&lt") == 0) {
	       /* less-than HTML tokens */
	       fputs("<", f);
	       p += 3;
	    }
	    else if (strincmp(p, "&gt") == 0) {
	       /* greater-than HTML tokens */
	       fputs(">", f);
	       p += 3;
	    }
	    else if (*p == '\\') {
	       /* backslash escape */
	       fputs("\\\\", f);
	       p++;
	    }
	    else if (*p == '{') {
	       /* open brace escape */
	       fputs("\\{", f);
	       p++;
	    }
	    else if (*p == '}') {
	       /* close brace escape */
	       fputs("\\}", f);
	       p++;
	    }
	    else {
	       while (pardebt > 0) {
		  fputs("\\par ", f);
		  pardebt--;
	       }

	       if (bullet) {
		  /* precede this paragraph with a bullet symbol */
		  fputs("{\\pntext\\f2\\fs16 \\'b7\\tab}\n", f);
		  fprintf(f, "{\\*\\pn \\pnlvlblt\\pnf2\\pnfs16\\pnindent%d{\\pntxtb \\'b7}}\n", BULLET_INDENT);
		  fprintf(f, "\\fi%d\\li%d ", -BULLET_INDENT, indent*INDENT_SIZE);
		  if (tallbullets)
		     fputs("\\sa80 ", f);
		  bullet = 0;
		  inbullet++;
	       }

	       /* normal character */
	       fputc((unsigned char)*p, f);
	       last = p++;
	    }
	 }

	 if (line->flags & HEADING_FLAG) {
	    /* end a section heading */
	    fputs("\\par }\n", f);
	 }
	 else if (prevdef) {
	    /* end a function definition */
	    if (!multidef) {
	       fputs("\\par }\n", f);
	       if ((!indef) && (line->next) && (!(line->next->flags & DEFINITION_FLAG))) {
		  /* indent the definition body text */
		  indent++;
		  fprintf(f, "{\\li%d ", indent*INDENT_SIZE);
		  indef = 1;
		  if (line->flags & NONODE_FLAG)
		     weakdef = 1;
	       }
	    }
	 }
	 else if (preformat) {
	    /* hard CR for preformatted blocks */
	    fputs("\n", f);
	    parloan++;
	 }
	 else if (!(line->flags & NO_EOL_FLAG)) {
	    if ((is_empty(strip_html(line->text))) &&
		(!strstr(line->text, "<hr>"))) {
	       /* output an empty line */
	       if (!prevhead) {
		  parloan++;
		  if (!strstr(line->text, "</pre>"))
		     parloan++;
	       }
	    }
	    else {
	       if (last && *last != 32)
		  fputs(" ", f); /* add artificial space */

	       fputs("\n", f); /* normal EOL */
	    }
	 }
      }
      else if (line->flags & NODE_FLAG) {
	 /* index node */
	 if (line->flags & HTML_FLAG) {
	    fputs("\\brdrb\\brdrs\\brdrw15\\brsp20 \\par \\pard \\par \\par ", f);
	 }
	 fputs("{\\xe\\v ", f);
	 fputs(line->text, f);
	 fputs("}\n", f);
      }
      else if (line->flags & TOC_FLAG) {
	 /* table of contents */
	 PAR();
	 fputs("\n{\\field{\\*\\fldinst TOC \\\\t \"Heading 1\" }{\\fldrslt {\\b\\i\\ul\\fs24\\cf2 Update this field to generate the table of contents.}}}\n", f);
      }
      else if (line->flags & INDEX_FLAG) {
	 /* index */
	 PAR();
	 fputs("\n{\\field{\\*\\fldinst INDEX \\\\e \"\\tab \" \\c \"1\" }{\\fldrslt {\\b\\i\\ul\\fs24\\cf2 Update this field to generate the document index.}}}\n", f);
      }
      else if (line->flags & TALLBULLET_FLAG) {
	 /* larger spacing after bulleted paragraphs */
	 tallbullets = 1;
      }

      prevhead = (line->flags & HEADING_FLAG);

      /* advance to the next line */
      l = line->next;
      if (l) {
	 while ((l->next) && l->text && (is_empty(l->text)) && (l->next->flags == l->flags)) {
	    l = l->next;
	 }
	 if ((l->next) && (l->next->flags & HEADING_FLAG)) {
	    if (indef) {
	       fputs("\\par}", f);
	       pardebt++;
	       indent--;
	       indef = 0;
	    }
	    PAR();
	    line = l->next;
	    parloan = 0;
	    pardebt = 0;
	 }
	 else
	    line = line->next;
      }
      else
	 line = NULL;

      while (parloan > 0) {
	 PAR();
	 parloan--;
      }
   }

   fputs("}\n", f);

   fclose(f);
   return 0;
}



