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
 *      Grzegorz Adam Hankiewicz added support for auto types' cross
 *      references generation.
 *
 *      See readme.txt for copyright information.
 *
 *      See allegro/docs/src/makedoc/format.txt for a brief description of
 *      the source of _tx files.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "maketexi.h"
#include "makertf.h"
#include "makedoc.h"
#include "makemisc.h"


int multiwordheaders = 0;

static int _no_strip;
static int _strip_indent = -1;


static char **_build_multi_identifier_nodes_table(void);
static void _destroy_multi_identifier_nodes_table(char **table);
static void _write_textinfo_xref(FILE *f, char *xref, char **chapter_nodes, char **secondary_nodes);
static int _valid_texinfo_node(char *s, char **next, char **prev);
static void _output_texinfo_toc(FILE *f, int root, int body, int part);
static char *_node_name(int i);
static char *_first_word(char *s);
static void _html_to_texinfo(FILE *f, char *p);
static void _write_auto_types_xrefs(FILE *f, char **auto_types, char *found_auto_types);
static int _is_auto_type_starting_letter(char c);
static int _is_auto_type_allowed_letter(char c);



/* write_texinfo:
 */
int write_texinfo(char *filename)
{
   char buf[256];
   LINE *line = head, *title_line = 0;
   char *p, *str, *next, *prev;
   char *xref[256], *chapter_nodes[256], **auto_types;
   char *found_auto_types, **multi_identifier_nodes;
   int xrefs = 0;
   int in_item = 0;
   int section_number = 0;
   int toc_waiting = 0;
   int continue_def = 0;
   int title_pass = 0;
   int in_chapter = 0;
   int i = 0;
   char up_target[256] = "Top";
   FILE *f;

   /*printf("writing %s\n", filename);*/

   f = fopen(filename, "w");
   if (!f)
      return 1;

   /* First scan all the chapters of the documents and build a lookup table
    * with their text lines. This lookup table will be used during the
    * generation of @xref texinfo commands, due to textinfo's restriction
    * that nodes can't contain spaces, and hence remain as a single word.
    */
   chapter_nodes[0] = 0;
   while (line) {
      if (line->flags & TEXINFO_FLAG) {
	 p = line->text;

	 if (line->flags & (HEADING_FLAG | CHAPTER_FLAG)) {
	    chapter_nodes[i++] = p;
	    chapter_nodes[i] = 0; 
	 }
      }
      line = line->next;
   }

   /* Now scan all the definitions and see where are more than one
    * definition per texi node, building a lookup table for the
    * identifiers which don't have their own node so we can reference them
    * correctly from other nodes.
    */

   multi_identifier_nodes = _build_multi_identifier_nodes_table();

   /* Build up a table with Allegro's structures' names (spelling?) */
   auto_types = build_types_lookup_table(&found_auto_types);
   
   /* Now reinit values and let real scanning start */
   line = head;

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
	       for (i=0; str[i] && str[i] != '\"'; i++)
		  buf[i] = str[i];
	       buf[i] = 0;

	       if (!(line->flags & NONODE_FLAG) &&
		  /* Output a symbol (@@void @func()). */
		   (_valid_texinfo_node(buf, &next, &prev))) {
		  if (toc_waiting) {
		     _output_texinfo_toc(f, 0, 1, section_number-1);
		     toc_waiting = 0;
		  }
		  if (xrefs > 0) {
		     fputs("See also:@*\n", f);
		     for (i=0; i<xrefs; i++)
			_write_textinfo_xref(f, xref[i], chapter_nodes, multi_identifier_nodes);
		     xrefs = 0;
		  }
		  _write_auto_types_xrefs(f, auto_types, found_auto_types);
		  fprintf(f, "@node %s, %s, %s, %s\n", buf, next, prev, _node_name(section_number-1));
		  fprintf(f, "%s %s\n", in_chapter ? "@subsection" : "@section", buf);
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
	 else if (line->flags & (HEADING_FLAG | CHAPTER_FLAG)) {

	    if (toc_waiting) {
	       _output_texinfo_toc(f, toc_waiting == 2 ? 1 : 0, 1, section_number-1);
	       toc_waiting = 0;
	    }
	    /* output a section heading */
	    if (section_number > 0) {
	       if (line->flags & CHAPTER_FLAG) {
		   in_chapter = 0;
		   strcpy(up_target, "Top");
	       }
	       if (in_item) {
		  fputs("@end ftable\n", f);
		  in_item = 0;
	       }
	       if (xrefs > 0) {
		  fputs("See also:@*\n", f);
		  for (i=0; i<xrefs; i++)
		     _write_textinfo_xref(f, xref[i], chapter_nodes, multi_identifier_nodes);
		  xrefs = 0;
	       }
	       _write_auto_types_xrefs(f, auto_types, found_auto_types);
	       p = strip_html(p);
	       fprintf(f, "@node %s, ", _node_name(section_number));
	       fprintf(f, "%s, ", _node_name(section_number+1));
	       fprintf(f, "%s, %s\n", _node_name(section_number-1), up_target);
	       fprintf(f, "%s %s\n", in_chapter ? "@section" : "@chapter", p);
	       if (!(line->flags & NOCONTENT_FLAG))
		  toc_waiting = 1;
	       if (line->flags & CHAPTER_FLAG)
	       {
		   in_chapter = 1;
		   strcpy(up_target, _node_name(section_number));
		   toc_waiting = 2;
	       }
	    }
	    section_number++;
	 }
	 else {
	    if (line->flags & DEFINITION_FLAG) {
	       /* This will detect all the possible auto types of the line.
		* The detected types will be marked in the found_auto_types
		* table for later delayed output.
		*/
	       int length;
	       char *temp = strstruct(line->text, auto_types, &length, found_auto_types);
	       while(temp)
		  temp = strstruct(temp+length, auto_types, &length, found_auto_types);
	    }
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
      else if (line->flags & CHAPTER_END_FLAG) {
	 in_chapter = 0;
	 strcpy(up_target, "Top");
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
	    /* Outputs a @hnode. */
	    if (xrefs > 0) {
	       fputs("See also:@*\n", f);
	       for (i=0; i<xrefs; i++)
		  _write_textinfo_xref(f, xref[i], chapter_nodes, multi_identifier_nodes);
	       xrefs = 0;
	    }
	    if (toc_waiting) {
	       _output_texinfo_toc(f, 0, 1, section_number-1);
	       toc_waiting = 0;
	    }
	    _write_auto_types_xrefs(f, auto_types, found_auto_types);
	    fprintf(f, "@node %s, %s, %s, %s\n", line->text, next, prev, _node_name(section_number-1));
	    fprintf(f, "%s %s\n", in_chapter ? "@subsection" : "@section", line->text);
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
   _destroy_multi_identifier_nodes_table(multi_identifier_nodes);
   destroy_types_lookup_table(auto_types, found_auto_types);
   return 0;
}



/* _write_textinfo_xref:
 * Writes the cross references found in the line pointed by xref.
 * The line will be split with strtok to find individual references.
 * Before a reference is written, it's compared against the table of
 * chapter nodes. If it's there, a special more complete xref command is
 * written, since nodes are internally a single word, but to the user they
 * look like normal complete sentences. If the word is not there, it's
 * compared this time agains the secondary_nodes table. If the reference
 * is in this pair table (see _build_multi_identifier_nodes_table), a
 * special xref will be written to access the primary node correctly.
 */
static void _write_textinfo_xref(FILE *f, char *xref, char **chapter_nodes, char **secondary_nodes)
{
   char *tok;
   assert(f);
   assert(xref);
   assert(chapter_nodes);

   tok = strtok(xref, ",;");

   while (tok) {
      char **p = chapter_nodes;
      while ((*tok) && (myisspace(*tok)))
	 tok++;
      while(*p) {
	 if(!strincmp(tok, strip_html(*p)))
	    break;
	 p++;
      }
      if(*p)
	 fprintf(f, "@xref{%s, %s}.@*\n", _first_word(tok), strip_html(*p));
      else {
	 p = secondary_nodes;
	 while (*p) {
	    if (!strcmp(tok, *p))
	       break;
	    p += 2;
	 }
	 if (*p)
	    fprintf(f, "@xref{%s, %s}.@*\n", *(p + 1), tok);
	 else
	    fprintf(f, "@xref{%s}.@*\n", tok);
      }
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
	 if (strcmp(toc->text, s) == 0) {
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
   int in_chapter = 0;

   fprintf(f, "@menu\n");

   if (root == 1 && body == 0) {
      toc = tochead;
      if (toc)
	 toc = toc->next;

      while (toc) {
	 if (toc->root == 2 || toc->root == 3)
	    in_chapter = 0;
	 if ((toc->root) && (toc->texinfoable) && !in_chapter) {
	    s = _first_word(toc->text);
	    fprintf(f, "* %s::", s);
	    for (i=strlen(s); i<24; i++)
	       fputc(' ', f);
	    fprintf(f, "%s\n", toc->text);
	 }
	 if (toc->root == 2)
	    in_chapter = 1;
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
	    if ((toc->root) && (!toc->otherfile) && (toc->texinfoable))
	       section_number++;
	    toc = toc->next;
	 }

         if (root) {
	    while (toc) {
	       if (toc->texinfoable && toc->root) {
		  s = _first_word(toc->text);
		  fprintf(f, "* %s::", s);
		  for (i=strlen(s); i<24; i++)
		     fputc(' ', f);
		  fprintf(f, "%s\n", toc->text);
	       }
	       toc = toc->next;
	       if (toc->root == 2 || toc->root == 3)
		  break;
	    }
	 }
	 else {
	    while ((toc) && (!toc->root)) {
	       if (toc->texinfoable)
		  fprintf(f, "* %s::\n", toc->text);
	       toc = toc->next;
	    }
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



/* build_types_lookup_table:
 * Automatic function which will scan the document from the beginning in
 * search of structures/types, which will be returned in a NULL terminated
 * array. Optionally, if found_table is not NULL, it will contain a simple
 * array with that many entries like the returned table. This array will
 * later be used to mark which autotypes were found (useful for texinfo).
 */
char **build_types_lookup_table(char **found_table)
{
   LINE *line = head;
   int i = 0;
   char **table;

   table = m_xmalloc(sizeof(char*));
   table[0] = 0;

   /* Scan document in memory finding definition lines with upper case types */
   while (line) {
      if ((line->flags & STRUCT_FLAG)) {
	 char *p = strchr(line->text, '\"'); /* Find start of <a name=... */
	 assert(p);
	 table = m_xrealloc(table, sizeof(char*) * (i + 2));
	 table[i] = m_strdup(++p); /* Duplicate rest of line */
	 assert(strchr(table[i], '\"'));
	 *strchr(table[i], '\"') = 0; /* Eliminate >... part after name */
	 table[++i] = 0;
      }
      line = line->next;
   }
   if (found_table)
      *found_table = memset(m_xmalloc(i + 1), 0, i + 1);
   return table;
}



/* _build_multi_identifier_nodes_table:
 * Automatic function which will scan the document from the beginning in
 * search of nodes which feature multiple identifiers. Since only the first
 * identifier of the texinfo node is indexed, all the others have to be
 * noted down. In the case of referencing them, a special xref has to be
 * written, which points to the secondary identifier through the first one.
 * Last lines of _write_texinfo_xref show how this is done. This function
 * returns a table with pairs of strings in the form (secondary, primary),
 * where secondary is the secondary identifier name, and primary is the
 * texinfo indexed identifier. The table has to be freed some time later
 * along with it's data. It's null terminated.
 */
static char **_build_multi_identifier_nodes_table(void)
{
   LINE *line = head;
   int i = 0;
   char **table;

   table = m_xmalloc(sizeof(char*)*2);
   table[0] = 0;
   table[1] = 0;

   /* Scan document in memory finding definition lines creating multi
    * identifier nodes
    */
   while (line) {
      if ((line->flags & DEFINITION_FLAG)) {
	 char *first_identifier = get_clean_ref_token(line->text);
	 line = line->next;
	 while (line->flags & DEFINITION_FLAG) {
	    if (strstr(line->text, "<a name")) {
	       table = m_xrealloc(table, sizeof(char*) * (i+4));
	       table[i++] = get_clean_ref_token(line->text);
	       table[i++] = m_strdup(first_identifier);
	       table[i] = table[i+1] = 0;
	    }
	    line = line->next;
	 }
	 free(first_identifier);
      }
      line = line->next;
   }

   return table;
}



/* _destroy_multi_identifier_nodes_table:
 * Given a null terminated table, frees it's string data and later the
 * table itself.
 */
static void _destroy_multi_identifier_nodes_table(char **table)
{
   int f;
   assert(table);
   for (f = 0; table[f]; f++)
      free(table[f]);
   free(table);
}



/* destroy_types_lookup_table:
 * Get's rid of the previously created auto_types and found_table.
 * found_auto_types can be NULL.
 */
void destroy_types_lookup_table(char **auto_types, char *found_auto_types)
{
   char **p = auto_types;
   assert(auto_types);
   
   if(found_auto_types)
      free(found_auto_types);
   while(*p) {
      free(*p);
      p++;
   }
   free(auto_types);
}


/* strstruct:
 * Complex function which replicates the logic behind the strstr function:
 * It will search in line, any of existant auto_types. If no auto_type is
 * found, it returns NULL. Otherwise will return a pointer to the beginning
 * of the auto_type in line, and the length of the auto_type will be stored
 * at the variable pointed by length. found_auto_types can be NULL, or a
 * table with as many entries as auto_types. In the latter case, the found
 * auto_type entry will be marked with a positive number.
 */
char *strstruct(char *line, char **auto_types, int *length, char *found_auto_types)
{
   assert(line);
   assert(auto_types);
   assert(length);
   
   while(*line) {
      if(_is_auto_type_starting_letter(*line)) {
	 char *end = line;
	 char **compare = auto_types;
	 int found = 0;
	 /* Find the end of the presumably found auto_type */
	 while(_is_auto_type_allowed_letter(*end))
	    end++;
	 *length = end - line;
	 /* Now compare with the auto_types table */
	 while(*compare) {
	    if(*length == (signed)strlen(*compare) && !strncmp(line, *compare, *length)) {
	       /* The auto_type must be followed by a blank space */
	       if(*(line + *length) == ' ') {
		  if(found_auto_types)
		     found_auto_types[found] = 1;
		  return line; /* Found, return it's position. */
	       }
	    }
	    compare++;
	    found++;
	 }
	 line = end;
      }
      else
	 line++;
   }
   return 0;
}



/* _write_auto_types_xrefs:
 * After normal references have been written, call this with a table of
 * auto_types, and a table of found_auto_types till the moment. xrefs will
 * be generated, and all the entries of the found_auto_types will be
 * zeroed for the next run.
 */
static void _write_auto_types_xrefs(FILE *f, char **auto_types, char *found_auto_types)
{
   while(*auto_types) {
      if(*found_auto_types) {
	 *found_auto_types = 0;
	 fprintf(f, "@xref{%s}.@*\n", *auto_types);
      }
      auto_types++;
      found_auto_types++;
   }
}



/* _is_auto_type_starting_letter:
 * Detects if the letter could be the beginning of an auto_type. Note the
 * ugly hack used for Allegro's fixed and al_ffblk types, which should be
 * capitalized.
 */
static int _is_auto_type_starting_letter(char c)
{
   if (c == 'f' || c == 'a' || (c >= 'A' && c <= 'Z')) return 1;
   return 0;
}



/* _is_auto_type_starting_letter:
 * Detects if the letter could be part of an auto_type. Note the ugly hack
 * used for Allegro's fixed and al_ffblk types, which should be capitalized.
 */
static int _is_auto_type_allowed_letter(char c)
{
   if (_is_auto_type_starting_letter(c))
      return 1;
   if (c >= '0' && c <= '9')
      return 1;
   if (c == '_' || c == 'i' || c == 'x' || c == 'e' || c == 'd' || c == 'l' ||
       c == 'b' || c == 'k')
      return 1;

   return 0;
}

