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
 *      Makedoc's html output routines.
 *
 *      By Shawn Hargreaves.
 *
 *      Grzegorz Adam Hankiewicz made the output valid HTML 4.0, made sorted
 *      TOCs, added support for cross references and optional CSS to color
 *      output, added support for auto types' cross references generation,
 *      and a few smaller details more.
 *
 *      Robert J. Ohannessian did some cosmetic improvements to the HTML
 *      output and separated the CSS file.
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
#include <assert.h>

#include "makehtml.h"
#include "makemisc.h"
#include "makedoc.h"
#include "maketexi.h"

#define TOKEN_NORMAL               'n'
#define TOKEN_TEXT                 't'

typedef struct t_post POST;

struct t_post
{
   char filename[1024];
   char **token;
   int num;
};

int html_flags;
char charset[256]="iso-8859-1";
const char *html_extension = "html";
char *html_document_title;
char *html_footer;
char *html_see_also_text;
char *html_examples_using_this_text;
char *html_css_filename;
char *html_return_value_text;
char *html_text_substitution[256];

static POST **_post;
static FILE *_file;
static char _filename[1024];
static char *_xref[256], *_eref[256];
static char *_full_eref_list;
static int _xrefs, _erefs;
static int _empty_count;
static char *_word_substitution[256];
static const char *_codeblock_start = "<blockquote class=\"code\"><pre>";
static const char *_codeblock_end = "</pre></blockquote>";
static const int _codeblock_start_len = 30;
static const int _codeblock_end_len = 19;

/* MSVC doesn't like long strings. */
static const char *_css_data1 = "\
A.xref:link {\n\
\tcolor:           blue;\n\
\ttext-decoration: none;\n\
\tbackground:      transparent;\n\
}\n\
A.xref:visited {\n\
\tcolor:           blue;\n\
\ttext-decoration: none;\n\
\tbackground:      transparent;\n\
}\n\
A.xref:hover {\n\
\tcolor:           blue;\n\
\ttext-decoration: underline;\n\
\tbackground:      rgb(220, 220, 220);\n\
}\n\
A.xref:active {\n\
\tcolor:           red;\n\
\ttext-decoration: none;\n\
\tbackground:      rgb(255, 204, 50);\n\
}\n\
A.eref:link {\n\
\tcolor:           blue;\n\
\ttext-decoration: none;\n\
\tbackground:      transparent;\n\
}\n\
A.eref:visited {\n\
\tcolor:           blue;\n\
\ttext-decoration: none;\n\
\tbackground:      transparent;\n\
}\n\
A.eref:hover {\n\
\tcolor:           blue;\n\
\ttext-decoration: underline;\n\
\tbackground:      rgb(255, 165, 214);\n\
}\n\
A.eref:active {\n\
\tcolor:           red;\n\
\ttext-decoration: none;\n\
\tbackground:      rgb(255, 204, 50);\n\
}\n\
A.autotype:link {\n\
\tcolor:           rgb(64, 64, 255);\n\
\ttext-decoration: none;\n\
\tbackground:      transparent;\n\
}\n\
A.autotype:visited {\n\
\tcolor:           rgb(64, 64, 255);\n\
\ttext-decoration: none;\n\
\tbackground:      transparent;\n\
}\n\
A.autotype:hover {\n\
\tcolor:           rgb(64, 64, 255);\n\
\ttext-decoration: underline;\n\
\tbackground:      transparent;\n\
}\n\
";

static const char *_css_data2 = "\
A.autotype:active {\n\
\tcolor:           red;\n\
\ttext-decoration: none;\n\
\tbackground:      transparent;\n\
}\n\
blockquote.xref {\n\
\tfont-family:     helvetica, verdana, serif;\n\
\tfont-size:       smaller;\n\
\tborder:          thin solid rgb(220, 220, 220);\n\
\tcolor:           black;\n\
\tbackground:      rgb(240, 240, 240);\n\
}\n\
blockquote.eref {\n\
\tfont-family:     helvetica, verdana, serif;\n\
\tfont-size:       smaller;\n\
\tborder:          thin solid rgb(255, 165, 214);\n\
\tcolor:           black;\n\
\tbackground:      rgb(255, 201, 230);\n\
}\n\
blockquote.code {\n\
\tborder:          thin solid rgb(255, 234, 190);\n\
\tcolor:           black;\n\
\tbackground:      rgb(255, 255, 205);\n\
}\n\
blockquote.text {\n\
\tborder:          thin solid rgb(190, 234, 255);\n\
\tcolor:           black;\n\
\tbackground:      rgb(225, 255, 255);\n\
}\n\
div.al-api {\n\
\tpadding-left:    0.5em;\n\
\tcolor:           black;\n\
\tbackground:      rgb(255, 234, 100);\n\
\tfont-weight:     bold;\n\
\tborder-bottom:   medium solid rgb(255, 204, 51);\n\
\tborder-left:     medium solid rgb(255, 204, 51);\n\
\tmargin-top:      2em;\n\
}\n\
div.al-api-cont {\n\
\tpadding-left:    0.5em;\n\
\tcolor:           black;\n\
\tbackground:      rgb(255, 234, 100);\n\
\tfont-weight:     bold;\n\
\tborder-bottom:   medium solid rgb(255, 204, 51);\n\
\tborder-left:     medium solid rgb(255, 204, 51);\n\
\tmargin-top:      -1em;\n\
}\n\
div.faq-shift-to-right {\n\
\tmargin-left: 2em;\n\
}\n\
div.al-back-to-contents {\n\
\ttext-align:      center;\n\
\tfont-family:     sans-serif;\n\
\tmargin-top:      1em;\n\
\tfont-weight:     bold;\n\
\t/* Removing the  comments of the next line create longer pages */\n\
\t/* margin-bottom:   100em; */\n\
}\n\
";

/* Internal functions */

static void _write_html_ref_list(char **ref, int *refs, const char *type);
static void _write_html_ref(char *ref, const char *type);
static void _output_html_footer(char *main_filename);
static void _output_toc(char *filename, int root, int body, int part);
static void _hfprintf(char *format, ...);
static void _hfputs(char *string);
static int _toc_scmp(const void *e1, const void *e2);
static int _str_cmp(const void *s1, const void *s2);
static int _output_section_heading(LINE *line, char *filename, int section_number);
static void _output_custom_markers(LINE *line);
static void _output_buffered_text(void);
static void _output_html_header(char *section);
static void _add_post_process_ref(const LINE *token, char type);
static void _post_process_pending_refs(void);
static POST *_search_post_section(const char *filename);
static POST *_search_post_section_with_token(const char *token);
static POST *_create_post_section(const char *filename);
static void _destroy_post_page(POST *p);
static void _post_process_filename(char *filename);
static int _verify_correct_input(void);
static void _close_html_file(FILE *file);
static void _output_sorted_nested_toc(TOC **list, unsigned int num_items);
static int _detect_non_paragraph_sections(const char *text);
static char *_mark_up_auto_types(char *line, char **auto_types);
static void _write_css_file(char *html_path, char *css_filename);
static void _prepare_html_text_substitution(void);
static void _free_html_text_substitution_memory(void);
static char *_do_text_substitution(char *input);
static void _output_symbol_index(void);
static const char *_find_short_description(const LINE *line);
static const char *_look_up_short_description(const char *token, int len);
static char *_scan_line_for_code_tokens(char *line, int *code_scanning);
static char *_replace_code_tokens(char *line, int column, int *end);
static char *_find_potential_token(char *line, int column, int end, int *found_size);



/* write_html:
 * Entry point to the function which translates makedoc's format
 * to correct html output.
 */
int write_html(char *filename)
{
   int block_empty_lines = 0, section_number = 0, ret;
   int last_line_was_a_definition = 0;
   LINE *line = head;
   char **auto_types;

   if (_verify_correct_input())
      return 1;

   /* English default text initialization */
   if (!html_see_also_text)
      html_see_also_text = m_strdup("See also:");
   if (!html_examples_using_this_text)
      html_examples_using_this_text = m_strdup("Examples using this:");
   if (!html_return_value_text)
      html_return_value_text = m_strdup("Return value:");
   
   /*printf("writing %s\n", filename);*/

   _file = fopen(filename, "w");
   if (!_file)
      return 1;

   /* build up a table with Allegro's structures' names (spelling?) */
   auto_types = build_types_lookup_table(0);
   
   strcpy(_filename, filename);
   if(html_document_title) _output_html_header(0);
   /* write out css if specified and allowed */
   if(html_css_filename && !(html_flags & HTML_IGNORE_CSS))
      _write_css_file(filename, html_css_filename);

   while (line) {
      if (line->flags & HTML_FLAG) {
	 if (line->flags & (HEADING_FLAG | NODE_FLAG | DEFINITION_FLAG)) {
	    _write_html_ref_list(_xref, &_xrefs, "xref");
	    _write_html_ref_list(_eref, &_erefs, "eref");
	 }

	 if (line->flags & HEADING_FLAG) {
	    _output_buffered_text();
	    if (_output_section_heading(line, filename, section_number))
	       return 1;
	    _add_post_process_ref(line, TOKEN_TEXT);
	    section_number++;
	 }
	 else if (line->flags & RETURN_VALUE_FLAG) {
	    assert(html_return_value_text);
	    fputs("<p><b>", _file);
	    fputs(html_return_value_text, _file);
	    fputs("</b>\n", _file);
	 }
	 else if (line->flags & NODE_FLAG) {
	    _output_buffered_text();
	    /* output a node marker, which will be followed by a chunk of text inside a chapter */
	    fprintf(_file, "<br><center><h2><a name=\"%s\">%s</a></h2></center><p>\n",
	       line->text, line->text);
	    _add_post_process_ref(line, TOKEN_TEXT);
	 }
	 else if (line->flags & DEFINITION_FLAG) {
	    static int prev_continued = 0;
	    char *temp = m_strdup(line->text);
	    if (_empty_count && !block_empty_lines) _empty_count++;
	    _output_buffered_text();
	    /* output a function definition */
	    _add_post_process_ref(line, TOKEN_NORMAL);
	    temp = _mark_up_auto_types(temp, auto_types);
	    
	    if (!prev_continued) {
	       if (!(html_flags & HTML_IGNORE_CSS)) {
	          if (!last_line_was_a_definition) {
		     fputs("<div class=\"al-api\"><b>", _file);
		  }
		  else {
		     fputs("<div class=\"al-api-cont\"><b>", _file);
		  }
	       }
	       else {
	          if (!last_line_was_a_definition) {
		     fputs("<hr><font size=\"+1\"><b>", _file);
		  }
		  else {
		     fputs("<font size=\"+1\"><b>", _file);
		  }
	       }
	    }
	    _hfprintf("%s", temp);
	    if (!(line->flags & CONTINUE_FLAG)) {
	       if (!(html_flags & HTML_IGNORE_CSS)) {
		  fputs("</b></div><br>", _file);
	       }
	       else {
		  fputs("</b></font><br>", _file);
	       }
	       prev_continued = 0;
	    }
	    else {
	       prev_continued = 1;
	    }
	    fputs("\n", _file);
	    free(temp);
	 }
	 else if ((!(line->flags & NO_EOL_FLAG)) &&
		  (is_empty(strip_html(line->text))) &&
		  (!strstr(line->text, "<li>"))) {
	    _hfprintf("%s\n", line->text);
	    if (!block_empty_lines)
	       _empty_count++;
	 }
	 else if (strstr(line->text, "<email>") || strstr(line->text, "<link>")) {
	    _output_buffered_text();
	    _output_custom_markers(line);
	 }
	 else {
	    /* output a normal line */
	    _output_buffered_text();
	    if ((html_flags & HTML_SPACED_LI) && strstr(line->text, "<li>"))
	       fputs("<br><br>", _file);
	    _hfputs(line->text);
	    fputs("\n", _file);
	 }

	 /* mutex to control extra line output in preformated sections */
	 ret = _detect_non_paragraph_sections(line->text);
	 if (ret) {
	    if(!block_empty_lines)
	       _empty_count = 0;
	    block_empty_lines += ret;
	 }
      }
      else if (line->flags & TOC_FLAG) {
	 _write_html_ref_list(_xref, &_xrefs, "xref");
	 _write_html_ref_list(_eref, &_erefs, "eref");
	 if (flags & MULTIFILE_FLAG) {
	    _output_buffered_text();
	    _output_toc(filename, 1, 0, -1);
	 }
	 else if (line->flags & SHORT_TOC_FLAG) {
	    _output_buffered_text();
	    _output_toc(filename, 0, 1, -1);
	 }
	 else {
	    _output_buffered_text();
	    _output_toc(filename, 1, 1, -1);
	 }
      }
      else if (line->flags & XREF_FLAG) {
	 if (line->flags & EREF_FLAG)
	    _eref[_erefs++] = line->text; /* buffer example reference */
	 else
	    _xref[_xrefs++] = line->text; /* buffer cross reference */
      }
      else if (line->flags & INDEX_FLAG) {
	 _output_buffered_text();
	 _output_symbol_index();
      }

      /* Keep track of continuous definitions */	 
      last_line_was_a_definition = (line->flags & DEFINITION_FLAG);

      line = line->next;
   }

   if ((flags & MULTIFILE_FLAG) && (section_number > 1))
      _output_html_footer(filename);

   _close_html_file(_file);
   
   _post_process_pending_refs();
   free(html_see_also_text);
   free(html_examples_using_this_text);
   free(html_return_value_text);
   free(_full_eref_list);
   destroy_types_lookup_table(auto_types, 0);

   return 0;
}



/* _output_custom_markers:
 * Converts the <email> and <link> custom markers to proper html code.
 */
static void _output_custom_markers(LINE *line)
{
   char *s2, *s3;
   /* special munging for links */
   char *s = line->text;

   do {
      s2 = strstr(s, "<email>");
      s3 = strstr(s, "<link>");

      if ((s2) || (s3)) {
	 int i;
	 char buf[256];
	 
	 if ((s3) && ((!s2) || (s3 < s2)))
	    s2 = s3;

	 i = 0;
	 while (s < s2)
	    buf[i++] = *(s++);

	 buf[i] = 0;
	 _hfputs(buf);

	 while (*s != '>')
	    s++;

	 s++;
	 i = 0;

	 while ((*s) && (*s != '<'))
	    buf[i++] = *(s++);

	 buf[i] = 0;

	 if (s2[1] == 'e')
	    _hfprintf("<a href=\"mailto:%s\">%s", buf, buf);
	 else
	    _hfprintf("<a href=\"%s\">%s", buf, buf);
      }
   } while (s2);

   _hfputs(s);
   fputs("\n", _file);
}



/* _write_html_ref_list:
 * Recieves an array of pointers and a pointer to the variable holding
 * the number of buffered text lines. Returns if there are none waiting
 * to be printed, otherwise sorts the list and writes a block of cross
 * references. Type should be the string "xref" or "eref".
 */
static void _write_html_ref_list(char **ref, int *refs, const char *type)
{
   int i;
   assert(html_see_also_text);
   assert(html_examples_using_this_text);
   assert(type);
   assert(*type);

   if (!(*refs))
      return;

   fputs("\n<blockquote", _file);
   if (!(html_flags & HTML_IGNORE_CSS))
      fprintf(_file, " class=\"%s\"><em><b>", type);
   else
      fputs("><font size=\"-1\" face=\"helvetica,verdana\"><em><b>", _file);

   /* select different wording depending on ref type */
   if (*type == 'e')
      fputs(html_examples_using_this_text, _file);
   else
      fputs(html_see_also_text, _file);
   fputs("</b></em>\n", _file);
   
   for (i=0; i<(*refs); i++) {
      if (i) _hfprintf(",\n");
      _write_html_ref(ref[i], type);
   }
   *refs = 0;
   _hfprintf(".%s</blockquote>\n",
      (html_flags & HTML_IGNORE_CSS) ? "</font>" : "");
   _empty_count = 0;
}



/* _write_html_ref:
 * Writes a single cross reference splitting ref if needed.
 * Type should be the string "xref" or "eref".
 */
static void _write_html_ref(char *ref, const char *type)
{
   char *tok, first = 1;

   tok = strtok(ref, ",;");

   while (tok) {
      while ((*tok) && (myisspace(*tok)))
	 tok++;
      if (!first) _hfprintf(",\n");
      first = 0;
      _hfprintf("<a ");
      if (!(html_flags & HTML_IGNORE_CSS))
	 _hfprintf("class=\"%s\" ", type);

      /* If they are erefs, they aren't on the list, and don't have spaces
	 in them, add them to the full list of already seen erefs */
      if (*type == 'e' && !strchr(tok, ' ') && (!_full_eref_list ||
	  !strstr(_full_eref_list, tok))) {
	 _full_eref_list = m_strcat(_full_eref_list, tok);
	 _full_eref_list = m_strcat(_full_eref_list, " ");
      }
    
      if (flags & MULTIFILE_FLAG)
	 _hfprintf("href=\"post_process#%s\" title=\"@SHORTDESC %s@\">%s</a>",
	    tok, tok, tok);
      else
	 _hfprintf("href=\"#%s\" title=\"@SHORTDESC %s@\">%s</a>",
	    tok, tok, tok);

      tok = strtok(NULL, ",;");
   }
}

/* _write_css_file:
 * Writes a separate CSS file. Requires two path components: the first
 * one is the normal html path, the second is a relative path indicating
 * where to put the css file. The final path will be a composition of
 * both.
 */
static void _write_css_file(char *html_path, char *css_filename)
{
   FILE *file;
   char *full_path;
   
   assert(html_path);
   assert(css_filename);

   full_path = m_replace_filename(html_path, css_filename);
   file = fopen(full_path, "wt");
   
   if (!file) {
      free(full_path);
      return;
   }

   fputs(_css_data1, file);
   fputs(_css_data2, file);
   fclose(file);
   free(full_path);
   
   return;
}



/* _output_html_footer:
 * Writes that "Back to contets" message at the end of a file with a link.
 */
static void _output_html_footer(char *main_filename)
{
   assert(html_footer);
   if (html_flags & HTML_OLD_F_TAG_FLAG)
      fprintf(_file, html_footer, get_filename(main_filename));
   else
      fprintf(_file, "<hr><div class=\"al-back-to-contents\"><a href"
	 "=\"%s\">%s</a></div>\n", get_filename(main_filename), html_footer);
}



/* _output_toc:
 */
static void _output_toc(char *filename, int root, int body, int part)
{
   char name[256];
   char *s;
   TOC *toc;
   int nested = 0;

   #define ALT_TEXT(toc)   ((toc->alt) ? toc->alt : toc->text)

   if (root) {
      int section_number = 0;
      toc = tochead;
      if (toc)
	 toc = toc->next;

      fprintf(_file, "<ul>\n");

      while (toc) {
	 if (toc->root) {
            if (toc->htmlable) {
               if (toc->otherfile) {
                  sprintf(name, "%s.%s", toc->text, html_extension);
                  mystrlwr(name);
                  _hfprintf("<li><a href=\"%s\">%s</a>\n", name, ALT_TEXT(toc));
               }
               else if (body) {
                  _hfprintf("<li><a href=\"#%s\">%s</a>\n", toc->text, ALT_TEXT(toc));
                  section_number++;
               }
               else {
                  strcpy(name, filename);
                  s = get_extension(name)-1;
                  if (s - get_filename(name) > 5)
                     s = get_filename(name)+5;
                  sprintf(s, "%03d.%s", section_number, html_extension);
                  _hfprintf("<li><a href=\"%s\">%s</a>\n", get_filename(name), ALT_TEXT(toc));
                  section_number++;
               }
            }
            else if (toc->root == 2) {
               _hfprintf("</ul><p>%s</p><ul>\n", ALT_TEXT(toc));
            }
            else if (toc->root == 3) {
               _hfprintf("</ul><ul>\n");
            }
	 }
	 toc = toc->next;
      }

      fprintf(_file, "</ul>\n");
   }

   if (body) {
      toc = tochead;
      if (toc)
	 toc = toc->next;

      if (part <= 0) {
	 TOC *ptr[TOC_SIZE];
	 int i = 0;
	 if (root)
	    fprintf(_file, "<ul>\n");
	 else
	    fprintf(_file, "<ul>\n");

	 while (toc) {
	    if ((toc->htmlable) && (!toc->otherfile)) {
	       if (!toc->root) {
		  if (!nested) {
		     fprintf(_file, "<ul>\n");
		     nested = 1;
		  }
	       }
	       else {
		  if (nested) {
		     _output_sorted_nested_toc(ptr, i);
		     nested = i = 0;
		  }
	       }

	       if (nested) {
		  if (i < TOC_SIZE)
		     ptr[i++] = toc;
	       }
	       else
		  _hfprintf("<li><a href=\"#%s\">%s</a>\n", toc->text, ALT_TEXT(toc));
	    }

	    toc = toc->next;
	 }

	 if (nested)
	    _output_sorted_nested_toc(ptr, i);

	 if (root)
	    fprintf(_file, "</ul>\n");
	 else
	    fprintf(_file, "</ul>\n");
      }
      else if (!(html_flags & HTML_OPTIMIZE_FOR_CHM) &&
               !(html_flags & HTML_OPTIMIZE_FOR_DEVHELP)) {
	 /* Outputs the function TOC for the current section. */
	 TOC *ptr[TOC_SIZE];
	 int j, i = 0;
	 int section_number = 0;
	 fprintf(_file, "\n<ul>\n");

	 while ((toc) && (section_number < part)) {
	    if ((toc->htmlable) && (toc->root) && (!toc->otherfile))
	       section_number++;
	    toc = toc->next;
	 }

	 while ((toc) && (!toc->root) && (i < TOC_SIZE)) {
	    if (toc->htmlable)
	       ptr[i++] = toc;
	    toc = toc->next;
	 }

	 if (i > 1)
	    qsort(ptr, i, sizeof(TOC *), _toc_scmp);

	 for (j = 0; j < i; j++) {
	    _hfprintf("<li><a href=\"#%s\">%s</a>", ptr[j]->text, ALT_TEXT(ptr[j]));
	    fprintf(_file, " &mdash; ");
	    _hfprintf("@SHORTDESC %s@\n", ALT_TEXT(ptr[j]));
	 }

	 fprintf(_file, "</ul>\n");
      }
   }
}



/* _hfprintf:
 */
static void _hfprintf(char *format, ...)
{
   char buf[1024];

   va_list ap;
   va_start(ap, format);
   vsprintf(buf, format, ap);
   va_end(ap);

   _hfputs(buf);
}



/* _hfputs:
 * Outputs string to _file. At the same time some things are substituted:
 * (&gt) for (&gt;), (&lt) for (&lt;), (&) for (&amp;) and (`word') for
 * (<tt>`word'</tt>).
 */
static void _hfputs(char *string)
{
   char *end = 0;
   while (*string) {
      /* Found simple text comma? */
      if ((end == 0) && (string[0] == '`')) {
	 end = strchr(string + 1, 39);
	 /* Is there a closing comma? */
	 if (end) {
	    char *p = string + 1;
	    /* Check for a specific character range */
	    while ((p < end) && ((*p >= '.') && (*p <= 'z')))
	       p++;
	        
	    if ((p == end) && (p - string > 1)) {
	       fputs("<tt>`", _file);
	       string++;
	       continue;
	    } else {
	       end = 0;
	    }
	 }
      }
      /* Is closing comma search active? */
      if ((string[0] == 39) && end == string) {
	 fputs("'</tt>", _file);
	 string++;
	 end = 0;
	 continue;
      }
      if ((string[0] == '&') &&
	  ((string[1] == 'l') || (string[1] == 'g')) &&
	  (string[2] == 't')) {

	 fputc('&', _file);
	 fputc(string[1], _file);
	 fputc('t', _file);
	 fputc(';', _file);

	 string += 3;
      }
      else if ((string[0] == '&') && (string[1] != 'l') &&
	  (string[1] != 'g')) {

	 fputs("&amp;", _file);
	 string++;
      }
      else {
	 fputc(*string, _file);
	 string++;
      }
   }
}



/* _toc_scmp:
 * qsort helper which compares two TOC nodes alphabeticaly.
 */
static int _toc_scmp(const void *e1, const void *e2)
{
   TOC *t1 = *((TOC **)e1);
   TOC *t2 = *((TOC **)e2);

   return mystricmp(t1->text, t2->text);
}



/* _str_cmp:
 * qsort helper which compares two tokens alphabetically.
 */
static int _str_cmp(const void *s1, const void *s2)
{
   const char *t1 = *((const char **)s1);
   const char *t2 = *((const char **)s2);
   return strcmp(t1, t2);
}



/* _output_section_heading:
 * Used to mark a new section of the document. In multifile output closes
 * the actual file and writes a new one.
 */
static int _output_section_heading(LINE *line, char *filename, int section_number)
{
   char buf[256], *s;
   
   /* output a section heading, switching file as required */
   if ((flags & MULTIFILE_FLAG) && (section_number > 0)) {
      if (section_number > 1)
	 _output_html_footer(filename);

      _close_html_file(_file);
      strcpy(buf, filename);
      s = get_extension(buf)-1;
      if (s - get_filename(buf) > 5)
	 s = get_filename(buf)+5;
      sprintf(s, "%03d.%s", section_number-1, html_extension);
      /*printf("writing %s\n", buf);*/
      _file = fopen(buf, "w");
      if (!_file)
	 return 1;
      strcpy(_filename, buf);
      _output_html_header(line->text);
   }
   _hfprintf("<h1>%s</h1>\n", line->text);
   if ((flags & MULTIFILE_FLAG) && 
       (!(line->flags & NOCONTENT_FLAG)) &&
       (section_number > 0)) {
      _output_toc(filename, 0, 1, section_number);
   }
   return 0;
}



/* _output_html_header:
 * The header of an HTML document is a little bit complex, so it deserves
 * it's own function. In the header you have the title of the document,
 * the character set definition and the style sheet specification. This
 * function just outputs to _file the correct tags, it presumes _file
 * has just been opened. The title is specified in the source document
 * with a @document_title= line, and section can be another text which will
 * be appended to the title.
 */
static void _output_html_header(char *section)
{
   assert(html_document_title);
   
   if (html_flags & HTML_OLD_H_TAG_FLAG) {
      if (section && strlen(strip_html(section))) {
	 int i;
	 for (i=0; html_document_title[i]; i++) {
	    if (html_document_title[i] == '#')
	       fputs(strip_html(section), _file);
	    else
	       fputc(html_document_title[i], _file);
	 }
      }
   }
   else {
      fputs("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\" \"http://www.w3.org/TR/REC-html40/loose.dtd\">\n", _file);
      fputs("<html><head><title>\n", _file);
      fputs(html_document_title, _file);
      if (section && strlen(strip_html(section))) {
	 fputs(": ", _file);
	 fputs(strip_html(section), _file);
      }
      fputs("\n</title>\n", _file);
      fputs("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=", _file);
      fputs(charset, _file);
      fputs("\">\n", _file);

      /* optional style sheet output */
      if (!(html_flags & HTML_IGNORE_CSS)) {
	 fputs("<meta http-equiv=\"Content-Style-Type\" content=\"text/css\">\n", _file);
	 if (html_css_filename) {
	    fprintf(_file, "<link rel=\"stylesheet\" title=\"Default\" type=\"text/css\" href=\"%s\">", html_css_filename);
	 }
	 else {
	    fputs("<style type=\"text/css\" title=\"Default\"><!--\n", _file);
	    fputs(_css_data1, _file);
	    fputs(_css_data2, _file);
	    fputs("\n\n--></style>\n", _file);
	 }
      }

      /* header end and body start */
      fputs("</head><body bgcolor=white text=black link=\"#0000ee\" alink=\"#ff0000\" vlink=\"#551a8b\">\n", _file);
   }
}



/* _output_buffered_text:
 * Outputs buffered text, which might be a set of cross references
 * followed by a number of empty lines, which will be transformed
 * to apropiate HTML tags.
 */
static void _output_buffered_text(void)
{
   if (_empty_count) {
      if (_empty_count > 1)
	 fputs("<p><br>\n", _file);
      else
	 fputs("<p>\n", _file);

      _empty_count = 0;
   }
}



/* _post_process_pending_refs:
 * Reopens all the created files and scans for post process tags, which
 * will be replaced with correct filenames according to the post process
 * info in memory. Additionally, it will also look for text substitutions
 * placed in the html_text_substitution global array.
 */
static void _post_process_pending_refs(void)
{
   int f;
   if (!_post) return ;

   _prepare_html_text_substitution();
   for(f = 0; _post[f]; f++)
      _post_process_filename(_post[f]->filename);
   
   for(f = 0; _post[f]; f++)
      _destroy_post_page(_post[f]);
      
   _free_html_text_substitution_memory();
   free(_post);
   _post = 0;
}



/* _add_post_process_ref:
 * Adds a token to a buffer with the current filename for a post process.
 * The type of the reference will be stored as the first character of
 * the clean token, and a comma will separate its short description if
 * there is any available.
 */
static void _add_post_process_ref(const LINE *line, char type)
{
   const char *token, *short_desc;
   char *clean_token;
   POST *p;
   assert(line);

   token = line->text;
   clean_token = get_clean_ref_token(token);
   /* ignore empty tokens, or those with space at the beginning */
   if (!clean_token[0] || myisspace(clean_token[0])) {
      free(clean_token);
      return ;
   }

   p = _search_post_section(_filename);
   if (!p)
      p = _create_post_section(_filename);

   short_desc = _find_short_description(line);

   /* Reserve memory for the list, and the token with type. */
   p->token = m_xrealloc(p->token, sizeof(char*) * (1 + p->num));
   p->token[p->num] = m_strcat(m_strdup(" "), clean_token);
   p->token[p->num][0] = type;
   /* If we found a short description, add it with a comma. */
   if (short_desc) {
      p->token[p->num] = m_strcat(p->token[p->num], ",");
      p->token[p->num] = m_strcat(p->token[p->num], short_desc);
   }
   p->num++;
}



/* _search_post_section:
 * Searches the globals for an existant POST page which matches filename.
 * Returns the POST page or NULL if none was found.
 */
static POST *_search_post_section(const char *filename)
{
   int f;
   
   if (!_post)
      return 0;

   for(f = 0; _post[f]; f++)
      if (!strcmp(_post[f]->filename, filename))
	 return _post[f];
	 
   return 0;
}



/* _search_post_section_with_token:
 * Similar to _search_post_section, but instead of matching filenames,
 * returns the page containing the specified token, NULL if none was found.
 */
static POST *_search_post_section_with_token(const char *token)
{
   int f, g, len;
   if (!_post)
      return 0;

   len = strlen(token);

   for(f = 0; _post[f]; f++) {
      for(g = 0; g < _post[f]->num; g++) {
	 if (!strncmp(_post[f]->token[g] + 1, token, len)) {
	    /* Still, we have to verify the tokens have correct size. */
	    const char *desc = strchr(_post[f]->token[g], ',');
	    if (desc && (desc - _post[f]->token[g] - 1) == len)
	       return _post[f];
	    else if (!desc && strlen(_post[f]->token[g] + 1) == (size_t)len)
	       return _post[f];
	 }
      }
   }
   return 0;
}



/* _look_up_short_description:
 * Retrieves from the cached short description the one which matches
 * len characters from the string token. Returns NULL if none is found.
 */
static const char *_look_up_short_description(const char *token, int len)
{
   int f, g;
   if (!_post)
      return 0;

   for(f = 0; _post[f]; f++) {
      for(g = 0; g < _post[f]->num; g++) {
	 /* First see if this entry has a short description. */
	 const char *desc = strchr(_post[f]->token[g], ',');
	 if (!desc)
	    continue;
	 /* Does the found token have the lenght we are looking for? */
	 if ((desc - _post[f]->token[g] - 1) != len)
	    continue;
	 /* Verify that the token is the same and has correct length. */
	 if (!strncmp(_post[f]->token[g] + 1, token, len))
	    return desc + 1; /* Avoid the comma. */
      }
   }
   return 0;
}



/* _create_post_section:
 * Adds one POST section matching filename to the global array of pages
 * after creating a new POST page, which will be returned.
 */
static POST *_create_post_section(const char *filename)
{
   int num;
   POST *p;

   p = m_xmalloc(sizeof(POST));
   p->token = m_xmalloc(sizeof(char*));
   p->num = 0;
   strcpy(p->filename, filename);

   if (!_post) {
      _post = m_xmalloc(sizeof(POST*));
      _post[0] = 0;
   }

   for(num = 0; _post[num]; num++)
      ;

   _post = m_xrealloc(_post, sizeof(POST*) * (2 + num));
   _post[num++] = p;
   _post[num] = 0;

   return p;
}



/* _destroy_post_page:
 * Frees the memory used by a post page.
 */
static void _destroy_post_page(POST *p)
{
   int f;
   for(f = 0; f < p->num; f++)
      free(p->token[f]);

   free(p->token);
   free(p);
}



/* _post_process_filename:
 * Opens the filename and replaces the post process tags.
 */
static void _post_process_filename(char *filename)
{
   int code_scanning = 0;
   char *new_filename, *p;
   char *line;
   FILE *f1 = 0, *f2 = 0;

   new_filename = m_strdup(filename);
   if (!new_filename || strlen(new_filename) < 2) goto cancel;
   new_filename[strlen(filename)-1] = 'x';

   f1 = fopen(filename, "rt");
   f2 = fopen(new_filename, "wt");
   if (!f1 || !f2) goto cancel;

   /*printf("post processing %s\n", filename);*/

   code_scanning = 0;

   while ((line = m_fgets(f1))) {
      /* Transform read lines into hyperlinks with post process marks. */
      line = _scan_line_for_code_tokens(line, &code_scanning);
      
      /* Loop over pending filename post process marks. */
      while ((p = strstr(line, "\"post_process#"))) {
	 char *clean_token = get_clean_ref_token(p + 2);
	 POST *page = _search_post_section_with_token(clean_token);
	 if (!page) {
	    printf("Didn't find ref for %s (clean: %s)\n", line, clean_token);
	    memmove(p, p + 14, strlen(p + 14) + 1);
	 }
	 else {
	    char *temp = m_xmalloc(1 + strlen(line) + 13 +
			 strlen(get_filename(page->filename)));
	    strncpy(temp, line, p - line + 1);

	    if (!(html_flags & HTML_OPTIMIZE_FOR_DEVHELP) && !strcmp(filename, page->filename))
	       strcpy(&temp[p - line + 1], p + 13);
	    else {
	       strcpy(&temp[p - line + 1], get_filename(page->filename));
	       strcat(temp, p + 13);
	    }
	    free(line);
	    line = temp;
	 }
	 free(clean_token);
      }

      /* Loop over pending short description marks. */
      while ((p = strstr(line, "@SHORTDESC "))) {
	 const char *desc;
	 char *end = strchr(p + 11, '@');
	 if (!end) {
	    printf("Didn't find closing of @SHORTDESC!\n");
	    abort();
	 }
	 desc = _look_up_short_description(p + 11, end - p - 11);
	 if (desc) {
	    /* Substitute with found test. */
	    char *temp = m_xmalloc(1 + strlen(line) + strlen(desc));
	    strncpy(temp, line, p - line);
	    strcpy(temp + (p - line), desc);
	    strcat(temp, end + 1); /* Don't copy the @ from end. */
	    free(line);
	    line = temp;
	 }
	 else {
	    /* Remove the description then. */
	    if (strncmp(p - 8, "&mdash; ", 8) == 0) {
	       /* Remove the previous dash too. */
	       p -= 8;
	    }
	    memmove(p, end + 1, strlen(end));
	 }
      }
      line = _do_text_substitution(line);
      fputs(line, f2);
      free(line);
   }

   fclose(f1);
   fclose(f2);
   f1 = f2 = 0;

   if (remove(filename)) goto cancel;
   rename(new_filename, filename);

   cancel:
   if (f1) fclose(f1);
   if (f2) fclose(f2);
   if (new_filename) {
      remove(new_filename);
      free(new_filename);
   }
}



/* _verify_correct_input:
 * Goes through some bits marked during _tx read and verifies that the
 * input file is ok, returning non-zero with bad files.
 */
static int _verify_correct_input(void)
{
   int ret = 0;
   
   if (html_flags & HTML_OLD_H_TAG_FLAG) {
      printf("The tag '@h=<html><head><title>#</title></head><body>' and its variant ");
      printf("is obsolete.\nPlease use '@document_title blah blah blah' instead.\n");
      printf("Otherwise the output won't be valid HTML code and won't use CSS.\n");
      printf("And make sure you aren't using any <head>,<title> or <body> tags!.\n");
   }

   if (!(html_flags & HTML_DOCUMENT_TITLE_FLAG) && (flags & MULTIFILE_FLAG)) {
      printf("Missing tag for document title. You should use one of these:\n");
      printf("@document_title=Allegro manual. (this is recommended)\n ...or...\n");
      printf("@h=<html><head><title>#</title></head><body>.\n...and later...\n");
      printf("@<html>\n@<head>\n@<title>Allegro - a game programming library</title></head>\n@<body>\n");
      ret++;
   }

   if (html_flags & HTML_OLD_F_TAG_FLAG) {
      printf("The tags '@f=blah', '@f1=blah' and '@f2=blah' are obsolete.\n");
      printf("Please use '@html_footer=Back to contents' or something similar instead.\n");
      printf("And make sure you aren't using any <head>,<title> or <body> tags!.\n");
   }

   if (!(html_flags & HTML_FOOTER_FLAG) && (flags & MULTIFILE_FLAG)) {
      printf("For multifile documents please use '@html_footer=Back to contents'.\n");
   }

   return ret;
}



/* _close_html_file:
 * Closes the stream after writting the apropiate html close tags.
 */
static void _close_html_file(FILE *file)
{
   fputs("\n</body>\n</html>\n", file);
   fclose(file);
}



/* _output_sorted_nested_toc:
 * Outputs the buffered TOC list, according to num_items. After the
 * list a </ul> is appended to the text.
 */
static void _output_sorted_nested_toc(TOC **list, unsigned int num_items)
{
   unsigned int f;

   if (num_items > 1)
      qsort(list, num_items, sizeof(TOC *), _toc_scmp);
      
   for (f = 0; f < num_items; f++) {
      _hfprintf("<li><a href=\"#%s\">%s</a>", list[f]->text, ALT_TEXT(list[f]));
      fprintf(_file, " &mdash; ");
      _hfprintf("@SHORTDESC %s@\n", ALT_TEXT(list[f]));
   }
   
   fprintf(_file, "</ul>\n");
}



/* _detect_non_paragraph_sections:
 * Used on a line of text, detects if the line contains tags after which the
 * paragraph tag <p> can't exist because it would break html validity.
 * The return value is a bit special: it
 * will be zero if no tags where found. For each opening tag the function
 * will add one to the return value. Equally, each closing tag will substract
 * one from the return value. Examples:
 *     <pre>..</pre>...<pre>      will return 1
 *     <pre>..<pre>...<pre>       will return 3 (note that HTML isn't valid)
 *     ..</pre>..                 will return -1
 */
static int _detect_non_paragraph_sections(const char *text)
{
   static const char *tags[] = {"pre>", "ul>", "h1>", "h2>", "h3>", "h4>", "h5>", "h6>",
      "i>", "strong>", "em>", "b>", 0};
   const char *p = text;
   int f, ret = 0;
   assert(text);

   while ((p = strchr(p, '<'))) {
      p++;
      if (*p == '/')
	 p++;

      for (f = 0; tags[f]; f++) {
	 if (strncmp(p, tags[f], strlen(tags[f])) == 0) {
	    if (*(p-1) == '/')
	       ret--;
	    else
	       ret++;
	 }
      }
   }
   return ret;
}
         

/* _mark_up_auto_types:
 * Instead of flooding the xref section with detected autotypes, and in
 * order to obtain a higher coolness factor, autotypes are detected at
 * definition lines and turned into hyperlinks. For this to happen you
 * need to pass the text line you would output for a normal definition
 * line, but it HAS to be dynamic memory (ie: m_strdup'ed). Pass a
 * previously generated auto_types table.
 * line has to be dynamic because it will be expanded whenever auto types
 * are found. Since this is done reallocating, this function will return
 * the new line pointer you should be using after calling this.
 */
static char *_mark_up_auto_types(char *line, char **auto_types)
{
   int length;
   char *p;
   assert(line);
   assert(auto_types);

   /* Did we find some auto type? */
   while ((p = strstruct(line, auto_types, &length, 0))) {
      char *temp = m_strdup(p);
      *p = 0;
      if (flags & MULTIFILE_FLAG) {
	 int offset = 22;
	 char *mark = m_xmalloc(3 * length + 47 + 21);
	 if (html_flags & HTML_IGNORE_CSS)
	    strcpy(mark, "<a href=\"post_process#");
	 else {
	    strcpy(mark, "<a class=\"autotype\" href=\"post_process#");
	    offset += 17;
	 }
	 strncpy(mark + offset, temp, length);
	 offset += length;
	 strcpy(mark + offset, "\" title=\"@SHORTDESC ");
	 offset += 20;
	 strncpy(mark + offset, temp, length);
	 offset += length;
	 strcpy(mark + offset, "@\">");
	 offset += 3;
	 strncpy(mark + offset, temp, length);
	 offset += length;
	 strcpy(mark + offset, "</a>");
	 line = m_strcat(line, mark);
	 free(mark);
	 line = m_strcat(line, temp + length);
      }
      else {
	 int offset = 10;
	 char *mark = m_xmalloc(3 * length + 35 + 21);
	 if (html_flags & HTML_IGNORE_CSS)
	    strcpy(mark, "<a href=\"#");
	 else {
	    strcpy(mark, "<a class=\"autotype\" href=\"#");
	    offset += 17;
	 }
	 strncpy(mark + offset, temp, length);
	 offset += length;
	 strcpy(mark + offset, "\" title=\"@SHORTDESC ");
	 offset += 20;
	 strncpy(mark + offset, temp, length);
	 offset += length;
	 strcpy(mark + offset, "@\">");
	 offset += 3;
	 strncpy(mark + offset, temp, length);
	 offset += length;
	 strcpy(mark + offset, "</a>");
	 line = m_strcat(line, mark);
	 free(mark);
	 line = m_strcat(line, temp + length);
      }
      free(temp);
   }
   return line;
}



/* _prepare_html_text_substitution():
 * The html_text_substitution array has pointers to text lines in the form
 * word|text. This function fills a global array with duplicates of word,
 * and at the same time makes html_text_substitution point directly at
 * text. With two arrays it is easier to search for words and replace with
 * the text when they are found.
 */
static void _prepare_html_text_substitution(void)
{
   int f;
   char *p;
   
   for (f = 0; html_text_substitution[f]; f++) {
      assert(f < 256);
      p = strchr(html_text_substitution[f] + 1, '|');
      assert(p);
      *p = 0;
      _word_substitution[f] = m_strdup(html_text_substitution[f]);
      p = m_strdup(p + 1);
      free(html_text_substitution[f]);
      html_text_substitution[f] = p;
   }
}



/* _free_html_text_substitution_memory:
 * Frees memory stored in the _word_substitution global array.
 */
static void _free_html_text_substitution_memory(void)
{
   int f;
   
   for (f = 0; _word_substitution[f]; f++) {
      free(html_text_substitution[f]);
      free(_word_substitution[f]);
   }
}



/* _do_text_substitution:
 * Given a malloc'ed string, searches all _word_substitution in it and
 * replaces them with html_text_substition if found. Returns a new
 * string or the one you passed if no changes were made.
 */
static char *_do_text_substitution(char *input)
{
   int start, end, middle, f;
   char *temp, *found, *reader;

   for (f = 0; _word_substitution[f]; f++) {
      reader = input;
      while ((found = strstr(reader, _word_substitution[f]))) {
	 /* Find lengths */
	 start = found - input;
	 middle = strlen(html_text_substitution[f]);
	 end = strlen(found + strlen(_word_substitution[f]));
	 /* Create new string and free old one */
	 temp = m_xmalloc(start + middle + end + 1);
	 strncpy(temp, input, start);
	 strcpy(temp + start, html_text_substitution[f]);
	 strcat(temp, found + strlen(_word_substitution[f]));
	 free(input);
	 /* Prepare for next loop */
	 input = temp;
	 reader = temp + start + middle;
      }
   }
   
   return input;
}



/* _output_symbol_index:
 * Called at the end of the first html pass, generates a list of links
 * to the so far found tokens in the text, which will be hyperlinked
 * correctly during the second pass.
 */
static void _output_symbol_index(void)
{
   int f, g, num = 0;
   char last_letter;
   char **list = NULL;
   
   for(f = 0; _post[f]; f++) {
      for(g = 0; g < _post[f]->num; g++) {
	 char *tok = _post[f]->token[g];
	 /* filter text tokens, they are not interesting */
	 if (tok[0] == TOKEN_TEXT)
	    continue;
	 tok++;
	 /* filter normal tokens from the example list */
	 if (_full_eref_list && strstr(_full_eref_list, tok))
	    continue;
	 list = m_xrealloc(list, sizeof(char *) * (num + 1));
	 list[num++] = tok;
      }
   }

   if (num < 1)
      return ;

   qsort(list, num, sizeof(char *), _str_cmp);

   /* Create minitoc scanning first letters of sorted list. */
   fprintf(_file, "<div class='mini_toc' id='top'>\n\t");
   for (f = 0, last_letter = -1; f < num; f++) {
      /* Valid range? */
      if (list[f][0] >= 'A' && list[f][0] <= 'z') {
	 /* Different letter? */
	 if (last_letter != list[f][0]) {
	    last_letter = list[f][0];
	    fprintf(_file, "<a href='#mini_toc_%c%d'>%c</a>\n\t",
	       last_letter, last_letter > '_', last_letter);
	 }
      }
   }
   fprintf(_file, "\n</div>\n\n");

   for (f = 0, last_letter = -1; f < num; f++) {
      char *p, *temp;

      /* Did we change letter section? */
      if (list[f][0] >= 'A' && list[f][0] <= 'z' && list[f][0] != last_letter) {
	 /* Is this the first section or another one? */
	 if (last_letter != -1)
	    fprintf(_file, "\n</ul>\n");
            
	 last_letter = list[f][0];
	 fprintf(_file, "<h1 class='mini_toc' "
	    "id='mini_toc_%c%d'><a href='#top'>%c</a></h1>\n<ul>\n",
	    last_letter, last_letter > '_', last_letter);
      }
      
      /* Before writing the string, remove possible short descriptions. */
      temp = m_strdup(list[f]);
      p = strchr(temp, ',');
      if (p)
	 *p = 0;

      _hfprintf("<li><a href=\"post_process#%s\">%s</a>", temp, temp);
      fprintf(_file, " &mdash; ");
      _hfprintf("@SHORTDESC %s@\n", temp);
   }
   fprintf(_file, "</ul>");
}



/* _find_short_description:
 * Given a line, looks forward enough to find a @shortdesc definition.
 * Returns a const pointer to the text of the line, or NULL if no
 * short definition was found.
 */
static const char *_find_short_description(const LINE *line)
{
   assert(line);
   while (1) {
      line = line->next;
      
      if (!line)
	 return 0;

      /* So, did we find a short description? */
      if (line->flags & SHORT_DESC_FLAG)
	 return line->text;

      /* Continue parsing through continuations or definitions. */
      if (line->flags & (CONTINUE_FLAG | DEFINITION_FLAG))
	 continue;

      /* Abort on beginning of new chapters, nodes, and paragraphs. */
      if (line->flags & (HEADING_FLAG | NODE_FLAG | TEXT_FLAG))
	 return 0;

      /* Safe abort on (nearl) empty lines. */
      if (strlen(line->text) < 2)
	 return 0;
   }
   return 0;
}



/* _scan_line_for_code_tokens:
 * Transforms line looking for code tokens. line has to be a dynamic
 * reallocable string. The code_scanning variable should point to a
 * state variable indicating whether the line we are processing should be
 * looked for code tokens or not. When looking for code tokens, anything
 * in the global token cache will be hyperlinked.
 */
static char *_scan_line_for_code_tokens(char *line, int *code_scanning)
{
   int column = 0;
   
   assert(line);
   assert(code_scanning);

   while (1) {
      if (*code_scanning) {
	 int end;
	 /* Substituting text and looking for closing tags. */
	 char *p = strstr(line + column, _codeblock_end);
	 if (!p) {
	    end = strlen(line);
	    return _replace_code_tokens(line, column, &end);
	 }
	 /* Looks like we have to change state. */
	 end = p - line;
	 line = _replace_code_tokens(line, column, &end);
	 column = end;
	 *code_scanning = 0;
      }
      else {
	 /* Looking for code chunks. */
	 char *p = strstr(line + column, _codeblock_start);
	 if (!p)
	    return line;
	 /* Looks like we did find something. Change state. */
	 column = (p - line) + _codeblock_start_len;
	 *code_scanning = 1;
      }
   }
}



/* _replace_code_tokens:
 * Given a dynamic line, a beginning and an end, this function replaces
 * all known tokens with hyperlinks to be post processed. When the new
 * line is returned, end will have been updated to point at the new
 * end character in the returned string (because it will grow).
 */
static char *_replace_code_tokens(char *line, int column, int *end)
{
   int found_size;
   POST *page;
   char buf[256];
   char *p, *new_line;
   int expanded;

   while ((p = _find_potential_token(line, column, *end, &found_size))) {
      assert(found_size < 255);
      /* Extract potential token into separate buffer. */
      strncpy(buf, p, found_size);
      buf[found_size] = 0;

      page = _search_post_section_with_token(buf);
      if (!page) {
	 column = p - line + found_size;
	 continue;
      }

      column = p - line;
      /* Found a token. Resize line and replace token with hyperlink. */
      new_line = m_xmalloc(strlen(line) + found_size * 3 + 69);
      strncpy(new_line, line, column);
      new_line[column] = 0;
      strcat(new_line, "<a href=\"post_process#");
      strcat(new_line, buf);
      strcat(new_line, "\" class=\"autotype\" title=\"@SHORTDESC ");
      strcat(new_line, buf);
      strcat(new_line, "@\">");
      strcat(new_line, buf);
      strcat(new_line, "</a>");
      expanded = strlen(new_line) - column - found_size;
      strcat(new_line, line + column + found_size);

      free(line);
      line = new_line;
      column += expanded + found_size;
      *end = *end + expanded;
   }

   return line;
}



/* _find_potential_token:
 * In aline, starting at column and endign at end, tries to find potential
 * tokens, which is words composed of alphanumeric characters and the
 * underscore. If found, a pointer to the token inside the line is
 * returned and the length stored in found_size.
 */
static char *_find_potential_token(char *line, int column, int end, int *found_size)
{
   char *p = line + column;
   *found_size = 0;

   #define VALID_CHARACTER()            \
      (*p == '_' ||                     \
      (*p >= 'A' && *p <= 'Z') ||       \
      (*p >= 'a' && *p <= 'z') ||       \
      (*p >= '0' && *p <= '9'))

   /* Discard bad characters. */
   while (!VALID_CHARACTER()) {
      p++;
      column++;
      if (p - line >= end)
         return 0;
   }

   /* Now accept good ones. */
   while (VALID_CHARACTER()) {
      p++;
      *found_size = *found_size + 1;
      if (column + *found_size == end)
         break;
   }

   #undef VALID_CHARACTER

   if (*found_size > 0)
      return line + column;
   else
      return 0;
}
