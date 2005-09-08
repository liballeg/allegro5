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
 *      This is a little hack I wrote to simplify producing the documentation
 *      in various formats (ASCII, HTML, TexInfo, and RTF). It converts from
 *      a weird layout that is basically a kind of stripped down HTML with a
 *      few extra markers (the _tx files), and automates a few things like
 *      constructing indexes and splitting HTML files into multiple parts.
 *      It's not pretty, elegant, or well written, and is liable to get upset
 *      if it doesn't like the input data. If you change the _tx files, run
 *      'make docs' to rebuild everything.
 *
 *      By Shawn Hargreaves.
 *
 *      Grzegorz Adam Hankiewicz added email mangling, custom metatags for
 *      easier CSS HTML output, and cleaned up some code.
 *
 *      See readme.txt for copyright information.
 *
 *      See allegro/docs/src/makedoc/format.txt for a brief description of
 *      the source of _tx files.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>
#include <assert.h>

#include "makedoc.h"
#include "makehtml.h"
#include "makemisc.h"
#include "maketxt.h"
#include "makeman.h"
#include "makertf.h"
#include "maketexi.h"
#include "makechm.h"
#include "makedevh.h"
#include "makesci.h"

/* external globals */

LINE *head = NULL;
TOC *tochead = NULL;
int flags = TEXT_FLAG | HTML_FLAG | TEXINFO_FLAG | RTF_FLAG | MAN_FLAG;

/* other globals */

static int _last_toc_line = INT_MIN;
static int _warn_on_long_lines = 0;
static char *_email_mangle_at, *_email_mangle_dot;
static int _mangle_emails = 0;


/* internal functions */

static void _transform_custom_tags(char *buf);
static int _read_file(char *filename);
static void _free_data(void);
static void _add_line(char *buf, int flags);
static void _add_toc_line(const char *buf, const char *alt, int root, int num, int texinfoable, int htmlable, int otherfile);
static void _add_external_file(const char *buf, int line_number);
static char *_my_fgets(char *p, int max, FILE *f);
static void _add_toc(char *buf, int root, int num, int texinfoable, int htmlable);
static void _activate_email_mangling(const char *txt);
static void _mangle_email_links(char *buf);
static char *_mangle_email(const char *email, int len);



/* main:
 * Binary entry point.
 */
int main(int argc, char *argv[])
{
   char filename[256] = "";
   char txtname[256] = "";
   char htmlname[256] = "";
   char texinfoname[256] = "";
   char rtfname[256] = "";
   char manname[256] = "";
   char chmname[256] = "";
   char devhelpname[256] = "";
   char scitename[256] = "";
   char *texinfo_extension = "txi";
   int err = 0;
   int partial = 0;
   int i;

   for (i=1; i<argc; i++) {
      if ((mystricmp(argv[i], "-ascii") == 0) && (i<argc-1)) {
	 strcpy(txtname, argv[++i]);
      }
      else if ((mystricmp(argv[i], "-chm") == 0) && (i<argc-1)) {
	 strcpy(chmname, argv[++i]);	 
      }   
      else if ((mystricmp(argv[i], "-devhelp") == 0) && (i<argc-1)) {
	 strcpy(devhelpname, argv[++i]);	 
      }
      else if ((mystricmp(argv[i], "-scite") == 0) && (i<argc-1)) {
	 strcpy(scitename, argv[++i]);
      }
      else if ((mystricmp(argv[i], "-html") == 0) && (i<argc-1)) {
	 strcpy(htmlname, argv[++i]);
	 html_extension = "html";
      }
      else if ((mystricmp(argv[i], "-htm") == 0) && (i<argc-1)) {
	 strcpy(htmlname, argv[++i]);
	 html_extension = "htm";
      }
      else if ((mystricmp(argv[i], "-texi") == 0) && (i<argc-1)) {
	 strcpy(texinfoname, argv[++i]);
	 texinfo_extension = "texi";
      }
      else if ((mystricmp(argv[i], "-txi") == 0) && (i<argc-1)) {
	 strcpy(texinfoname, argv[++i]);
	 texinfo_extension = "txi";
      }
      else if ((mystricmp(argv[i], "-rtf") == 0) && (i<argc-1)) {
	 strcpy(rtfname, argv[++i]);
      }
      else if ((mystricmp(argv[i], "-man") == 0) && (i<argc-1)) {
	 strcpy(manname, argv[++i]);
      }
      else if (mystricmp(argv[i], "-part") == 0) {
	 partial = 1;
      }
      else if (mystricmp(argv[i], "-warn") == 0) {
	 _warn_on_long_lines = 1;
      }
      else if (mystricmp(argv[i], "-ochm") == 0) {
	 html_flags |= HTML_OPTIMIZE_FOR_CHM;
      }
      else if (mystricmp(argv[i], "-odevhelp") == 0) {
	 html_flags |= HTML_OPTIMIZE_FOR_DEVHELP | HTML_IGNORE_CSS;
      }
      else if (argv[i][0] == '-') {
	 fprintf(stderr, "Unknown option '%s'\n", argv[i]);
	 return 1;
      }
      else {
	 strcpy(filename, argv[i]);
      }
   }

   if (!filename[0]) {
      printf("Usage: makedoc [outputs] [-part] [-warn] [-ochm] [-odevhelp] filename._tx\n\n");
      printf("Output formats:\n");
      printf("\t-ascii filename.txt\n");
      printf("\t-htm[l] filename.htm[l]\n");
      printf("\t-rtf filename.rtf\n");
      printf("\t-t[e]xi filename.t[e]xi\n");
      printf("\t-man filename.3\n");
      printf("\t-chm filename.htm[l]\n");
      printf("\t-devhelp filename.htm[l]\n");
      printf("\t-scite filename.api\n");
      return 1;
   }

   if ((!txtname[0]) && (!htmlname[0]) && (!texinfoname[0]) && (!rtfname[0]) &&
      (!manname[0]) && (!chmname[0]) && (!devhelpname[0]) && (!scitename[0])) {
      strcpy(txtname, filename);
      strcpy(get_extension(txtname), "txt");

      strcpy(htmlname, filename);
      strcpy(get_extension(htmlname), html_extension);

      strcpy(texinfoname, filename);
      strcpy(get_extension(texinfoname), texinfo_extension);

      strcpy(rtfname, filename);
      strcpy(get_extension(rtfname), "rtf");
   }

   if (_read_file(filename) != 0) {
      fprintf(stderr, "Error reading input file\n");
      err = 1;
      goto getout;
   }

   if (txtname[0]) {
      if (write_txt(txtname, partial) != 0) {
	 fprintf(stderr, "Error writing ASCII output file\n");
	 err = 1;
	 goto getout;
      }
   }

   if (htmlname[0]) {
      if (write_html(htmlname) != 0) {
	 fprintf(stderr, "Error writing HTML output file\n");
	 err = 1;
	 goto getout;
      }
   }

   if (texinfoname[0]) {
      if (write_texinfo(texinfoname) != 0) {
	 fprintf(stderr, "Error writing TexInfo output file\n");
	 err = 1;
	 goto getout;
      }
   }

   if (rtfname[0]) {
      if (write_rtf(rtfname) != 0) {
	 fprintf(stderr, "Error writing RTF output file\n");
	 err = 1;
	 goto getout;
      }
   }

   if (manname[0]) {
      if (write_man(manname) != 0) {
	 fprintf(stderr, "Error writing MAN output file\n");
	 err = 1;
	 goto getout;
      }
   }
   
   if (chmname[0]) {
      if (write_chm(chmname) != 0) {
	 fprintf(stderr, "Error writing CHM output file\n");
	 err = 1;
	 goto getout;
      }
   }      
   
   if (devhelpname[0]) {
      if (write_devhelp(devhelpname) != 0) {
	 fprintf(stderr, "Error writing DevHelp output file\n");
	 err = 1;
	 goto getout;
      }
   }

   if (scitename[0]) {
      if (write_scite(scitename, filename) != 0) {
	 fprintf(stderr, "Error writing SciTE output file\n");
	 err = 1;
	 goto getout;
      }
   }

   getout:

   _free_data();

   return err;
}



/* _transform_custom_tags:
 * To simplify the task of writting colored ._tx files with CSS,
 * there are a few custom tags which are expanded to html code when
 * they are read. If CSS is ignored, they are expanded to normal
 * HTML tags.
 */
static void _transform_custom_tags(char *buf)
{
   char *p;

   while ((p = strstr(buf, "<codeblock>"))) {
      if (html_flags & HTML_IGNORE_CSS) {
	 memmove(p+5, p+11, 1 + strlen(p+11));
	 strncpy(p, "<pre>", 5);
      }
      else {
	 memmove(p+30, p+11, 1 + strlen(p+11));
	 strncpy(p, "<blockquote class=\"code\"><pre>", 30);
      }
   }

   while ((p = strstr(buf, "<textblock>"))) {
      if (html_flags & HTML_IGNORE_CSS) {
	 memmove(p+5, p+11, 1 + strlen(p+11));
	 strncpy(p, "<pre>", 5);
      }
      else {
	 memmove(p+30, p+11, 1 + strlen(p+11));
	 strncpy(p, "<blockquote class=\"text\"><pre>", 30);
      }
   }

   while ((p = strstr(buf, "<endblock>"))) {
      if (html_flags & HTML_IGNORE_CSS) {
	 memmove(p+6, p+10, 1 + strlen(p+10));
	 strncpy(p, "</pre>", 6);
      }
      else {
	 memmove(p+19, p+10, 1 + strlen(p+10));
	 strncpy(p, "</pre></blockquote>", 19);
      }
   }
}



/* _read_file:
 */
static int _read_file(char *filename)
{
   char buf[1024];
   FILE *f;
   int line = 0;

   /*printf("reading %s\n", filename);*/

   f = fopen(filename, "r");
   if (!f)
      return 1;
      
   while (_my_fgets(buf, 1023, f)) {
      char *end = strpbrk(buf, "\r\n");
      line++;

      /* strip EOL */
      if (end) *end = 0;

      _transform_custom_tags(buf);

      if (_mangle_emails)
	 _mangle_email_links(buf);

      if (buf[0] == '@') {
	 /* a marker line */
	 if (mystricmp(buf+1, "text") == 0)
	    flags |= TEXT_FLAG;
	 else if (mystricmp(buf+1, "!text") == 0)
	    flags &= ~TEXT_FLAG;
	 else if (mystricmp(buf+1, "html") == 0)
	    flags |= HTML_FLAG;
	 else if (mystricmp(buf+1, "!html") == 0)
	    flags &= ~HTML_FLAG;
	 else if (mystricmp(buf+1, "texinfo") == 0)
	    flags |= TEXINFO_FLAG;
	 else if (mystricmp(buf+1, "!texinfo") == 0)
	    flags &= ~TEXINFO_FLAG;
	 else if (mystricmp(buf+1, "rtf") == 0)
	    flags |= RTF_FLAG;
	 else if (mystricmp(buf+1, "!rtf") == 0)
	    flags &= ~RTF_FLAG;
	 else if (mystricmp(buf+1, "man") == 0)
	    flags |= MAN_FLAG;
	 else if (mystricmp(buf+1, "!man") == 0)
	    flags &= ~MAN_FLAG;
	 else if (mystricmp(buf+1, "indent") == 0)
	    flags &= ~NO_INDENT_FLAG;
	 else if (mystricmp(buf+1, "!indent") == 0)
	    flags |= NO_INDENT_FLAG;
	 else if (mystricmp(buf+1, "multiplefiles") == 0)
	    flags |= MULTIFILE_FLAG;
	 else if (mystricmp(buf+1, "headingnocontent") == 0)
	    flags |= (HEADING_FLAG | NOCONTENT_FLAG);
	 else if (mystricmp(buf+1, "heading") == 0)
	    flags |= HEADING_FLAG;
	 else if (mystricmp(buf+1, "chapter") == 0)
	    flags |= CHAPTER_FLAG;
	 else if (mystricmp(buf+1, "endchapter") == 0) {
	    _add_line(NULL, CHAPTER_END_FLAG);
	    _add_toc_line(buf, NULL, 3, line, 0, 0, 0);
	 }
	 else if (mystricmp(buf+1, "retval") == 0) {
	    _add_line("", flags | RETURN_VALUE_FLAG);
	    flags &= ~(HEADING_FLAG | NOCONTENT_FLAG | NONODE_FLAG);
	 }
	 else if (strincmp(buf+1, "shortdesc ") == 0)
	    _add_line(buf+11, SHORT_DESC_FLAG);
	 else if (strincmp(buf+1, "titlepage") == 0)
	    _add_line("", START_TITLE_FLAG);
	 else if (strincmp(buf+1, "!titlepage") == 0)
	    _add_line("", END_TITLE_FLAG);
	 else if (mystricmp(buf+1, "contents") == 0)
	    _add_line("", TOC_FLAG);
	 else if (mystricmp(buf+1, "shortcontents") == 0)
	    _add_line("", TOC_FLAG | SHORT_TOC_FLAG);
	 else if (mystricmp(buf+1, "index") == 0)
	    _add_line("", INDEX_FLAG);
	 else if (strincmp(buf+1, "node ") == 0) {
	    _add_toc_line(buf+6, NULL, 0, line, 1, 0, 0);
	    _add_line(buf+6, NODE_FLAG | HTML_FLAG);
	 }
	 else if (strincmp(buf+1, "hnode ") == 0) {
	    _add_toc_line(buf+7, NULL, 0, line, 1, 0, 0);
	    _add_line(buf+7, NODE_FLAG | HTML_FLAG);
	 }
	 else if (strincmp(buf+1, "nonode") == 0)
	    flags |= NONODE_FLAG;
	 else if (strincmp(buf+1, "xref ") == 0) {
	    _add_line(buf+6, XREF_FLAG);
	    if (_last_toc_line == line-1)
	       _last_toc_line = line;
	 }
	 else if (strincmp(buf+1, "eref ") == 0) {
	    _add_line(buf+6, XREF_FLAG | EREF_FLAG);
	    if (_last_toc_line == line-1)
	       _last_toc_line = line;
	 }
	 else if (strincmp(buf+1, "header ") == 0)
	    _add_line(buf+8, HEADER_FLAG);
	 else if (strincmp(buf+1, "startoutput ") == 0)
	    _add_line(buf+13, STARTOUTPUT_FLAG);
	 else if (strincmp(buf+1, "endoutput ") == 0)
	    _add_line(buf+11, ENDOUTPUT_FLAG);
	 else if (strincmp(buf+1, "tallbullet") == 0)
	    _add_line("", TALLBULLET_FLAG);
	 else if (strincmp(buf+1, "rtf_language_header=") == 0) {
	    rtf_language_header = m_strcat(rtf_language_header, buf+21);
	    rtf_language_header = m_strcat(rtf_language_header, "\n");
	 }
	 else if (strincmp(buf+1, "mangle_emails=") == 0) {
	    _activate_email_mangling(buf+15);
	 }
	 /* html specific tags */
	 else if (strincmp(buf+1, "charset=") == 0)
	    strcpy(charset, buf+9);
	 else if ((mytolower(buf[1]=='h')) && (buf[2]=='=')) {
	    html_document_title = m_strdup(buf+3);
	    html_flags |= HTML_OLD_H_TAG_FLAG | HTML_DOCUMENT_TITLE_FLAG | HTML_IGNORE_CSS;
	 }
	 else if (strincmp(buf+1, "document_title=") == 0) {
	    html_document_title = m_strdup(buf+16);
	    html_flags |= HTML_DOCUMENT_TITLE_FLAG;
	 }
	 else if (strincmp(buf+1, "html_examples_using_this_text=") == 0) {
	    if (html_examples_using_this_text)
	       free(html_examples_using_this_text);
	    html_examples_using_this_text = m_strdup(buf+20);
	 }
	 else if (strincmp(buf+1, "html_see_also_text=") == 0) {
	    if (html_see_also_text)
	       free(html_see_also_text);
	    html_see_also_text = m_strdup(buf+20);
	 }
	 else if (strincmp(buf+1, "html_return_value_text=") == 0) {
	    if (html_return_value_text)
	       free(html_return_value_text);
	    html_return_value_text = m_strdup(buf+24);
	 }
	 else if ((mytolower(buf[1]=='f')) && (buf[2]=='=')) {
	    html_footer = m_strdup(buf+3);
	    html_flags |= HTML_OLD_F_TAG_FLAG | HTML_FOOTER_FLAG;
	 }
	 else if ((mytolower(buf[1]=='f')) && (buf[2]=='1') && (buf[3]=='=')) {
	    html_footer = m_strdup(buf+4);
	    html_footer = m_strcat(html_footer, "%s");
	    html_flags |= HTML_OLD_F_TAG_FLAG | HTML_FOOTER_FLAG;
	 }
	 else if ((mytolower(buf[1]=='f')) && (buf[2]=='2') && (buf[3]=='=')) {
	    html_footer = m_strcat(html_footer, buf+4);
	    html_flags |= HTML_OLD_F_TAG_FLAG | HTML_FOOTER_FLAG;
	 }
	 else if (strincmp(buf+1, "html_footer=") == 0) {
	    html_footer = m_strdup(buf+13);
	    html_flags |= HTML_FOOTER_FLAG;
	 }
	 else if (strincmp(buf+1, "html_spaced_list_bullet") == 0)
	    html_flags |= HTML_SPACED_LI;
	 else if (strincmp(buf+1, "ignore_css") == 0)
	    html_flags |= HTML_IGNORE_CSS;
	 else if (strincmp(buf+1, "htmlcode") == 0)
	    _add_line(buf+10, (flags | HTML_FLAG | HTML_CMD_FLAG | NO_EOL_FLAG ) & (~TEXT_FLAG));
	 else if (buf[1] == '<')
	    _add_line(buf+1, (flags | HTML_FLAG | HTML_CMD_FLAG | NO_EOL_FLAG ) & (~TEXT_FLAG));
	 else if (strincmp(buf+1, "htmlindex ") == 0)
	    _add_toc_line(buf+11, NULL, 1, line, 0, 1, 1);
	 else if (strincmp(buf+1, "externalfile ") == 0)
	    _add_external_file(buf+14, line);
	 else if (strincmp(buf+1, "rtfh=") == 0)
	    strcpy(rtfheader, buf+6);
	 else if (strincmp(buf+1, "manh=") == 0)
	    strcpy(manheader, buf+6);
	 else if (strincmp(buf+1, "mans=") == 0)
	    strcpy(mansynopsis, buf+6);
	 else if (strincmp(buf+1, "man_shortdesc_force1=") == 0) {
	    if (man_shortdesc_force1)
	       free(man_shortdesc_force1);
	    man_shortdesc_force1 = m_strdup(buf+22);
	 }
	 else if (strincmp(buf+1, "man_shortdesc_force2=") == 0) {
	    if (man_shortdesc_force2)
	       free(man_shortdesc_force2);
	    man_shortdesc_force2 = m_strdup(buf+22);
	 }
	 else if (strincmp(buf+1, "locale=") == 0) {
	    printf("'%s' tag obsolete, will be ignored.\n"
		   "Please use @charset='' or @rtf_language_header='' instead.\n"
		   "See makedoc.c for more information about this.\n", buf);
	 }
	 else if (strincmp(buf+1, "multiwordheaders") == 0)
	    multiwordheaders = 1;
	 else if (strincmp(buf+1, "external-css=") == 0)
	    html_css_filename = m_strdup(buf+14);
	 else if (buf[1] == '$')
	    _add_line(buf+2, TEXINFO_FLAG | TEXINFO_CMD_FLAG);
	 else if (buf[1] == '@') {
	    char *found_struct_definition = strstr(buf+2, "struct @");
	    if(!found_struct_definition)
	       found_struct_definition = strstr(buf+2, "typedef ");
	    _add_toc(buf+2, 0, line, !(flags & NONODE_FLAG), 1);
	    if (found_struct_definition)
	       _add_line(buf+2, flags | DEFINITION_FLAG | STRUCT_FLAG);
	    else
	       _add_line(buf+2, flags | DEFINITION_FLAG);
	    flags &= ~NONODE_FLAG;
	 }
	 else if (buf[1] == '\\') {
	    _add_toc(buf+2, 0, line, !(flags & NONODE_FLAG), 1);
	    _add_line(buf+2, flags | DEFINITION_FLAG | CONTINUE_FLAG);
	    flags &= ~NONODE_FLAG;
	 }
	 else if (strincmp(buf+1, "html_text_substitution=") == 0 && strchr(buf+25, '|')) {
	    int i = 0;
	    while (html_text_substitution[i]) i++;
	    assert(i < 256);
	    html_text_substitution[i] = m_strdup(buf+24);
	 }
	 else if (buf[1] == '#') {
	    /* comment */
	 }
	 else
	    printf("Ignoring unknown token '%s'\n", buf);
      }
      else {
	 /* some actual text */
	 if (flags & CHAPTER_FLAG)
	    _add_toc(buf, 2, line, !(flags & NONODE_FLAG), 0);

	 if (flags & HEADING_FLAG)
	    _add_toc(buf, 1, line, !(flags & NONODE_FLAG), (flags & HTML_FLAG));

	 _add_line(buf, flags & CHAPTER_FLAG ?
            (flags & ~(HTML_FLAG | TEXT_FLAG)) : flags);

	 if (_warn_on_long_lines) {
	    int len = strlen (buf);
	    if (len > 77)
	       printf("Warning: line %d exceded in %d caracters '...%s'\n",
		      line, len - 77, buf + 77);
	 }

	 flags &= ~(CHAPTER_FLAG | HEADING_FLAG | NOCONTENT_FLAG | NONODE_FLAG);
      }
   }

   fclose(f);
   return 0;
}



/* _free_data:
 */
static void _free_data(void)
{
   LINE *line = head;
   LINE *prev;

   TOC *tocline = tochead;
   TOC *tocprev;

   while (line) {
      prev = line;
      line = line->next;

      free(prev->text);
      free(prev);
   }

   while (tocline) {
      tocprev = tocline;
      tocline = tocline->next;

      free(tocprev->text);

      if (tocprev->alt)
	 free(tocprev->alt);

      free(tocprev);
   }

   if (_email_mangle_at)
      free(_email_mangle_at);
   if (_email_mangle_dot)
      free(_email_mangle_dot);
   if (man_shortdesc_force1)
      free(man_shortdesc_force1);
   if (man_shortdesc_force2)
      free(man_shortdesc_force2);
}



/* _add_line:
 */
static void _add_line(char *buf, int flags)
{
   static LINE *tail = NULL;
   LINE *line;

   line = m_xmalloc(sizeof(LINE));
   line->text = buf ? m_strdup(buf) : NULL;
   line->next = NULL;
   line->flags = flags;

   if (tail)
      tail->next = line;
   else
      head = line;

   tail = line;
}



/* _add_toc_line:
 * Adds a line to the table of contents. buf is the filename to be linked,
 * alt can be NULL, and in such case buf will be used to represent the
 * link: if alt is not NULL, it will be the link's name. root indicates how
 * the line should be added to the TOC index - 0 is a normal entry, 1 adds
 * it to the root index, 2 as a chapter start, 3 as chapter end. num is the
 * line number. texinfoable, htmlable and otherfile are boolean values
 * which say if this line should appear in the toc of those file formats.
 */
static void _add_toc_line(const char *buf, const char *alt, int root, int num, int texinfoable, int htmlable, int otherfile)
{
   static TOC *toctail = NULL;
   TOC *toc;

   toc = m_xmalloc(sizeof(TOC));
   toc->text = m_strdup(buf);

   if (alt)
      toc->alt = m_strdup(alt);
   else
      toc->alt = NULL;

   toc->next = NULL;
   toc->root = root;
   toc->texinfoable = ((texinfoable) && (num > _last_toc_line+1));
   toc->htmlable = htmlable;
   toc->otherfile = otherfile;

   _last_toc_line = num;

   if (toctail)
      toctail->next = toc;
   else
      tochead = toc;

   toctail = toc;
}



/* _add_external_file:
 * A special wrapper around _add_toc_line. buf contains both the filename
 * and the name which should be given to the link. Split both and call the
 * real funcion.
 */
static void _add_external_file(const char *buf, int line_number)
{
   char *filename, *title;

   /* First validate buffer */
   title = strpbrk(buf, "\t ");
   if (!title || is_empty(title)) {
      printf("Line %d has externalfile without title part (%s). "
	 "Ignoring.\n", line_number, buf);
      return;
   }

   filename = m_strdup(buf);
   title = strpbrk(filename, "\t ");
   *title = 0;
   _add_toc_line(filename, title+1, 1, line_number, 0, 1, 1);
   free(filename);
}



/* _add_toc:
 */
static void _add_toc(char *buf, int root, int num, int texinfoable, int htmlable)
{
   static int last_toc_line2 = INT_MIN;
   char b[256], b2[256];
   int c, d;
   int done;
   int got;

   if (root) {
      _add_toc_line(buf, NULL, root, num, texinfoable, htmlable, 0);
      strcpy(b, buf);
      sprintf(buf, "<a name=\"%s\">%s</a>", b, b);
   }
   else {
      got = 0;
      do {
	 done = 1;
	 for (c=0; buf[c]; c++) {
	    if (buf[c] == '@') {
	       for (d=0; myisalnum(buf[c+d+1]) || (buf[c+d+1] == '_') ||
		  (buf[c+d+1] == '/') || (buf[c+d+1] == '*'); d++)
		  b[d] = buf[c+d+1];
	       b[d] = 0;
	       _add_toc_line(b, NULL, 0, num, texinfoable, htmlable, 0);
	       strcpy(b2, buf+c+d+1);
	       sprintf(buf+c, "<a name=\"%s\">%s</a>%s", b, b, b2);
	       done = 0;
	       got = 1;
	       break;
	    }
	 }
      } while (!done);

      if ((!got) && (num > last_toc_line2+1)) {
	 strcpy(b, buf);

	 c = d = 0;

	 while ((b[c]) && (myisspace(b[c])))
	    c++;

	 while (b[c]) {
	    if (myisalnum(b[c]))
	       b2[d] = b[c];
	    else
	       b2[d] = '_';

	    c++;
	    d++;
	 }

	 b2[d] = 0;

	 _add_toc_line(b2, b, 0, num, texinfoable, htmlable, 0);
	 sprintf(buf, "<a name=\"%s\">%s</a>", b2, b);
      }
   }

   last_toc_line2 = num;
}



/* _my_fgets:
 * Sometime in the future this should be replaced by the safe m_fgets
 * function. However, the rest of the functions in this file still rely
 * on the character buffer being modifiable, so cleaning this would
 * practically mean having to rewrite the other functions too.
 */
static char *_my_fgets(char *p, int max, FILE *f)
{
   int c, ch;

   if (feof(f)) {
      p[0] = 0;
      return NULL;
   }

   for (c=0; c<max-1; c++) {
      ch = getc(f);
      if (ch == EOF)
	 break;
      p[c] = ch;
      if (p[c] == '\r')
	 c--;
      else if (p[c] == '\n')
	 break;
      else if (p[c] == '\t') {
	 p[c] = ' ';
	 while ((c+1) & 7) {
	    c++;
	    p[c] = ' ';
	 }
      }
   }

   p[c] = 0;
   return p;
}



/* _activate_email_mangling:
 * Called when the input ._tx file contains @mangle_emails=x. Activates
 * the global mangling flag, and reads from txt two strings separated
 * by space character which are meant to be used for '@' and '.'
 * respectively in the email mangling.
 */
static void _activate_email_mangling(const char *txt)
{
   const char *p;
   assert(txt);
   assert(*txt);

   _mangle_emails = 1;
   /* free previous strings if they existed */
   if (_email_mangle_at) free(_email_mangle_at);
   if (_email_mangle_dot) free(_email_mangle_dot);

   /* find space separator to detect words */
   p = strchr(txt, ' ');
   assert(p);     /* format specification requires two words with space */
   _email_mangle_at = m_strdup(txt);
   *(_email_mangle_at + (p - txt)) = 0;
   assert(*(p + 1));                            /* second word required */
   _email_mangle_dot = m_strdup(p+1);
}



/* _mangle_email_links:
 * Checks the given buffer for <email>..</a> links and mangles them.
 * Modifications are made directly over buf, make sure there's enough
 * space in it.
 */
static void _mangle_email_links(char *buf)
{
   assert(buf);
   while(*buf && (buf = strstr(buf, "<email>"))) {
      char *temp, *end = strstr(buf, "</a>");
      assert(end);                       /* can't have multiline emails */
      buf += 7;
      temp = _mangle_email(buf, end - buf);
      memmove(buf + strlen(temp), end, strlen(end) + 1);
      strncpy(buf, temp, strlen(temp));
      free(temp);
   }
}


/* _mangle_email:
 * Given a string, len characters will be parsed. '@' will be substituted
 * by "' %s ', _email_mangle_at", and '.' will be substituted by "' %s ",
 * _email_mangle_dot". The returned string has to be freed by the caller.
 */
static char *_mangle_email(const char *email, int len)
{
   char *temp, buf[2];
   int pos;
   assert(email);
   assert(*email);
   assert(len > 0);

   temp = m_strdup(email);
   *temp = 0;
   buf[1] = 0;
   for(pos = 0; pos < len; pos++) {
      if(email[pos] == '@') {
         temp = m_strcat(temp, " ");
         temp = m_strcat(temp, _email_mangle_at);
         temp = m_strcat(temp, " ");
      }
      else if(email[pos] == '.') {
         temp = m_strcat(temp, " ");
         temp = m_strcat(temp, _email_mangle_dot);
         temp = m_strcat(temp, " ");
      }
      else {
         buf[0] = email[pos];
         temp = m_strcat(temp, buf);
      }
   }
   return temp;
}

