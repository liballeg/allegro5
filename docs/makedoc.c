/*
 *    This is a little hack I wrote to simplify producing the documentation
 *    in various formats (ASCII, HTML, TexInfo, and RTF). It converts from
 *    a weird layout that is basically a kind of stripped down HTML with a 
 *    few extra markers (the _tx files), and automates a few things like 
 *    constructing indexes and splitting HTML files into multiple parts. 
 *    It's not pretty, elegant, or well written, and is liable to get upset 
 *    if it doesn't like the input data. If you change the _tx files, run 
 *    'make docs' to rebuild everything.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>


#define TEXT_FLAG             0x00000001
#define HTML_FLAG             0x00000002
#define HTML_CMD_FLAG         0x00000004
#define TEXINFO_FLAG          0x00000008
#define TEXINFO_CMD_FLAG      0x00000010
#define RTF_FLAG              0x00000020
#define MAN_FLAG              0x00000040
#define HEADING_FLAG          0x00000080
#define DEFINITION_FLAG       0x00000100
#define CONTINUE_FLAG         0x00000200
#define TOC_FLAG              0x00000400
#define INDEX_FLAG            0x00000800
#define NO_EOL_FLAG           0x00001000
#define MULTIFILE_FLAG        0x00002000
#define NOCONTENT_FLAG        0x00004000
#define NO_INDENT_FLAG        0x00008000
#define NODE_FLAG             0x00010000
#define NONODE_FLAG           0x00020000
#define STARTOUTPUT_FLAG      0x00040000
#define ENDOUTPUT_FLAG        0x00080000
#define TALLBULLET_FLAG       0x00100000
#define SHORT_TOC_FLAG        0x00200000
#define XREF_FLAG             0x00400000
#define HEADER_FLAG           0x00800000
#define START_TITLE_FLAG      0x01000000
#define END_TITLE_FLAG        0x02000000


typedef struct LINE
{
   char *text;
   struct LINE *next;
   int flags;
} LINE;


LINE *head = NULL;
LINE *tail = NULL;


typedef struct TOC
{
   char *text;
   char *alt;
   struct TOC *next;
   int root;
   int texinfoable;
   int htmlable;
   int otherfile;
} TOC;


TOC *tochead = NULL;
TOC *toctail = NULL;


int flags = TEXT_FLAG | HTML_FLAG | TEXINFO_FLAG | RTF_FLAG | MAN_FLAG;

int nostrip = 0;
int strip_indent = -1;
int last_toc_line = INT_MIN;
int last_toc_line2 = INT_MIN;
int multiwordheaders = 0;
int warn_on_long_lines = 0;

char locale[256] = "";
int dos_codepage = 1;

char fileheader[256] = "";
char filefooter[256] = "";
char rtfheader[256] = "";
char manheader[256] = "";
char mansynopsis[256] = "";

char *html_extension = "html";
char *texinfo_extension = "txi";

int mpreformat = 0;
int mpreindent = 0;



int mytolower(int c)
{
   if ((c >= 'A') && (c <= 'Z'))
      c -= ('A' - 'a');

   return c;
}



int myisspace(int c)
{
   return ((c == ' ') || (c == '\t') || (c == '\r') || 
	   (c == '\n') || (c == '\f') || (c == '\v'));
}



int myisalnum(int c)
{
   return (((c >= '0') && (c <= '9')) ||
	   ((c >= 'A') && (c <= 'Z')) ||
	   ((c >= 'a') && (c <= 'z')) ||
	   (c >= 0x80) || (c < 0));
}



char *mystrlwr(char *string)
{
   char *p;

   for (p=string; *p; p++)
      *p = mytolower(*p);

   return string;
}



int mystricmp(const char *s1, const char *s2)
{
   int c1, c2;

   do {
      c1 = mytolower(*(s1++));
      c2 = mytolower(*(s2++));
   } while ((c1) && (c1 == c2));

   return c1 - c2;
}



void set_locale(char *l)
{
   strcpy(locale, l);

   if (mystricmp(l, "china") == 0)
      dos_codepage = 0;
   else if (mystricmp(l, "korea") == 0)
      dos_codepage = 0;
}



char *extension(char *filename)
{
   int pos, end;

   for (end=0; filename[end]; end++)
      ; /* do nothing */

   pos = end;

   while ((pos>0) && (filename[pos-1] != '.') &&
	  (filename[pos-1] != '\\') && (filename[pos-1] != '/'))
      pos--;

   if (filename[pos-1] == '.')
      return filename+pos;

   return filename+end;
}



char *get_filename(char *path)
{
   int pos;

   for (pos=0; path[pos]; pos++)
      ; /* do nothing */ 

   while ((pos>0) && (path[pos-1] != '\\') && (path[pos-1] != '/'))
      pos--;

   return path+pos;
}



int is_empty(char *s)
{
   while (*s) {
      if (*s != ' ')
	 return 0;

      s++;
   }

   return 1;
}



void free_data(void)
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
}



void add_line(char *buf, int flags)
{
   int len;
   LINE *line;

   len = strlen(buf);
   line = malloc(sizeof(LINE));
   line->text = malloc(len+1);
   strcpy(line->text, buf);
   line->next = NULL;
   line->flags = flags;

   if (tail)
      tail->next = line;
   else
      head = line;

   tail = line;
}



void add_toc_line(char *buf, char *alt, int root, int num, int texinfoable, int htmlable, int otherfile)
{
   TOC *toc;

   toc = malloc(sizeof(TOC));

   toc->text = malloc(strlen(buf)+1);
   strcpy(toc->text, buf);

   if (alt) {
      toc->alt = malloc(strlen(alt)+1);
      strcpy(toc->alt , alt);
   }
   else
      toc->alt = NULL;

   toc->next = NULL;
   toc->root = root;
   toc->texinfoable = ((texinfoable) && (num > last_toc_line+1));
   toc->htmlable = htmlable;
   toc->otherfile = otherfile;

   last_toc_line = num;

   if (toctail) 
      toctail->next = toc;
   else
      tochead = toc;

   toctail = toc;
}



void add_toc(char *buf, int root, int num, int texinfoable, int htmlable)
{
   char b[256], b2[256];
   int c, d;
   int done;
   int got;

   if (root) {
      add_toc_line(buf, NULL, 1, num, texinfoable, htmlable, 0);
      strcpy(b, buf);
      sprintf(buf, "<a name=\"%s\">%s</a>", b, b);
   }
   else {
      got = 0;
      do {
	 done = 1;
	 for (c=0; buf[c]; c++) {
	    if (buf[c] == '@') {
	       for (d=0; myisalnum(buf[c+d+1]) || (buf[c+d+1] == '_') || (buf[c+d+1] == '/') || (buf[c+d+1] == '*'); d++)
		  b[d] = buf[c+d+1];
	       b[d] = 0;
	       add_toc_line(b, NULL, 0, num, texinfoable, htmlable, 0);
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

	 add_toc_line(b2, b, 0, num, texinfoable, htmlable, 0);
	 sprintf(buf, "<a name=\"%s\">%s</a>", b2, b);
      }
   }

   last_toc_line2 = num;
}



char *my_fgets(char *p, int max, FILE *f)
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



int strincmp(char *s1, char *s2)
{
   while (*s2) {
      if (mytolower(*s1) != mytolower(*s2))
	 return 1;

      s1++;
      s2++;
   }

   return 0;
}



int read_file(char *filename, char *htmlname)
{
   char buf[1024];
   char *p;
   FILE *f;
   int line = 0;

   printf("reading %s\n", filename);

   f = fopen(filename, "r");
   if (!f)
      return 1;

   while (my_fgets(buf, 1023, f)) {
      line++;

      /* strip EOL */
      p = buf+strlen(buf)-1;
      while ((p >= buf) && ((*p == '\r') || (*p == '\n'))) {
	 *p = 0;
	 p--;
      }

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
	 else if (strincmp(buf+1, "titlepage") == 0)
	    add_line("", START_TITLE_FLAG);
	 else if (strincmp(buf+1, "!titlepage") == 0)
	    add_line("", END_TITLE_FLAG);
	 else if (mystricmp(buf+1, "contents") == 0)
	    add_line("", TOC_FLAG);
	 else if (mystricmp(buf+1, "shortcontents") == 0)
	    add_line("", TOC_FLAG | SHORT_TOC_FLAG);
	 else if (mystricmp(buf+1, "index") == 0)
	    add_line("", INDEX_FLAG);
	 else if (strincmp(buf+1, "node ") == 0) {
	    add_toc_line(buf+6, NULL, 0, line, 1, 0, 0);
	    add_line(buf+6, NODE_FLAG);
	 }
	 else if (strincmp(buf+1, "hnode ") == 0) {
	    add_toc_line(buf+7, NULL, 0, line, 1, 0, 0);
	    add_line(buf+7, NODE_FLAG | HTML_FLAG);
	 }
	 else if (strincmp(buf+1, "nonode") == 0)
	    flags |= NONODE_FLAG;
	 else if (strincmp(buf+1, "xref ") == 0) {
	    add_line(buf+6, XREF_FLAG);
	    if (last_toc_line == line-1)
	       last_toc_line = line;
	 }
         else if (strincmp(buf+1, "header ") == 0)
            add_line(buf+8, HEADER_FLAG);
	 else if (strincmp(buf+1, "startoutput ") == 0)
	    add_line(buf+13, STARTOUTPUT_FLAG);
	 else if (strincmp(buf+1, "endoutput ") == 0)
	    add_line(buf+11, ENDOUTPUT_FLAG);
	 else if (strincmp(buf+1, "tallbullet") == 0)
	    add_line("", TALLBULLET_FLAG);
	 else if ((mytolower(buf[1]=='h')) && (buf[2]=='='))
	    strcpy(fileheader, buf+3);
	 else if ((mytolower(buf[1]=='f')) && (buf[2]=='='))
	    strcpy(filefooter, buf+3);
	 else if ((mytolower(buf[1]=='f')) && (buf[2]=='1') && (buf[3]=='='))
	    sprintf(filefooter, "%s%s", buf+4, get_filename(htmlname));
	 else if ((mytolower(buf[1]=='f')) && (buf[2]=='2') && (buf[3]=='='))
	    strcat(filefooter, buf+4);
	 else if (strincmp(buf+1, "rtfh=") == 0)
	    strcpy(rtfheader, buf+6);
	 else if (strincmp(buf+1, "manh=") == 0)
	    strcpy(manheader, buf+6);
	 else if (strincmp(buf+1, "mans=") == 0)
	    strcpy(mansynopsis, buf+6);
	 else if (strincmp(buf+1, "locale=") == 0)
	    set_locale(buf+8);
	 else if (strincmp(buf+1, "htmlindex") == 0)
	    add_toc_line(buf+11, NULL, 1, line, 0, 1, 1);
	 else if (strincmp(buf+1, "multiwordheaders") == 0)
	    multiwordheaders = 1;
	 else if (buf[1] == '<')
	    add_line(buf+1, (flags | HTML_FLAG | HTML_CMD_FLAG | NO_EOL_FLAG ) & (~TEXT_FLAG));
	 else if (buf[1] == '$')
	    add_line(buf+2, TEXINFO_FLAG | TEXINFO_CMD_FLAG);
	 else if (buf[1] == '@') {
	    add_toc(buf+2, 0, line, !(flags & NONODE_FLAG), 1);
	    add_line(buf+2, flags | DEFINITION_FLAG);
	    flags &= ~NONODE_FLAG;
	 }
	 else if (buf[1] == '\\') {
	    add_toc(buf+2, 0, line, !(flags & NONODE_FLAG), 1);
	    add_line(buf+2, flags | DEFINITION_FLAG | CONTINUE_FLAG);
	    flags &= ~NONODE_FLAG;
	 }
	 else if (buf[1] == '#') {
	    /* comment */
	 }
	 else {
	    fclose(f);
	    fprintf(stderr, "Unknown token '%s'\n", buf);
	    return 1; 
	 }
      }
      else {
	 /* some actual text */
	 if (flags & HEADING_FLAG)
	    add_toc(buf, 1, line, !(flags & NONODE_FLAG), (flags & HTML_FLAG));

	 add_line(buf, flags);

	 if (warn_on_long_lines) {
	    int len = strlen (buf);
	    if (len > 77)
	       printf("Warning: line %d exceded in %d caracters '...%s'\n",
		      line, len - 77, buf + 77);
	 }

	 flags &= ~(HEADING_FLAG | NOCONTENT_FLAG | NONODE_FLAG);
      }
   }

   fclose(f);
   return 0;
}



char *strip_html(char *p)
{
   static char buf[256];
   int c;

   c = 0;
   while (*p) {
      if (*p == '<') {
	 while ((*p) && (*p != '>'))
	    p++;
	 if (*p)
	    p++;
      }
      else {
	 buf[c] = *p;
	 c++;
	 p++;
      }
   }

   buf[c] = 0;

   return buf;
}



unsigned char dos_latin_mapping[] =
{
   0x20, 0xAD, 0xBD, 0x9C, 0xCF, 0xBE, 0xDD, 0xF5, 
   0xF9, 0xB8, 0xA6, 0xAE, 0xAA, 0xF0, 0xA9, 0xEE, 
   0xF8, 0xF1, 0xFD, 0xFC, 0xEF, 0xE6, 0xF4, 0xFA, 
   0xF7, 0xFB, 0xA7, 0xAF, 0xAC, 0xAB, 0xF3, 0xA8, 
   0xB7, 0xB5, 0xB6, 0xC7, 0x8E, 0x8F, 0x92, 0x80, 
   0xD4, 0x90, 0xD2, 0xD3, 0xDE, 0xD6, 0xD7, 0xD8, 
   0xD1, 0xA5, 0xE3, 0xE0, 0xE2, 0xE5, 0x99, 0x9E, 
   0x9D, 0xEB, 0xE9, 0xEA, 0x9A, 0xED, 0xE8, 0xE1, 
   0x85, 0xA0, 0x83, 0xC6, 0x84, 0x86, 0x91, 0x87, 
   0x8A, 0x82, 0x88, 0x89, 0x8D, 0xA1, 0x8C, 0x8B, 
   0xD0, 0xA4, 0x95, 0xA2, 0x93, 0xE4, 0x94, 0xF6, 
   0x9B, 0x97, 0xA3, 0x96, 0x81, 0xEC, 0xE7, 0x98
};



void afputc(unsigned char c, FILE *file)
{
   if (dos_codepage) {
      if (c >= 0xA0)
	 c = dos_latin_mapping[c-0xA0];
   }

   fputc(c, file);
}



void afputs(char *string, FILE *file)
{
   while (*string) {
      afputc((unsigned char)*string, file);
      string++;
   }
}



void hfputs(char *string, FILE *file)
{
   while (*string) {
      if ((string[0] == '&') && 
	  ((string[1] == 'l') || (string[1] == 'g')) &&
	  (string[2] == 't')) {

	 fputc('&', file);
	 fputc(string[1], file);
	 fputc('t', file);
	 fputc(';', file);

	 string += 3;
      }
      else { 
	 fputc(*string, file);
	 string++;
      }
   }
}



void hfprintf(FILE *file, char *format, ...)
{
   char buf[1024];

   va_list ap;
   va_start(ap, format);
   vsprintf(buf, format, ap);
   va_end(ap);

   hfputs(buf, file);
}



void tfputc(unsigned char c, FILE *file)
{
   afputc(c, file);
}



void tfputs(char *string, FILE *file)
{
   while (*string) {
      tfputc((unsigned char)*string, file);
      string++;
   }
}



void tfprintf(FILE *file, char *format, ...)
{
   char buf[1024];

   va_list ap;
   va_start(ap, format);
   vsprintf(buf, format, ap);
   va_end(ap);

   tfputs(buf, file);
}



void rfputc(unsigned char c, FILE *file)
{
   fputc(c, file);
}



void rfputs(char *string, FILE *file)
{
   while (*string) {
      rfputc((unsigned char)*string, file);
      string++;
   }
}



int toc_size(int part)
{
   TOC *toc = tochead;
   int section_number = 0;
   int count = 0;

   if (toc)
      toc = toc->next;

   while ((toc) && (section_number <= part)) {
      if ((toc->root) && (!toc->otherfile))
	 section_number++;
      toc = toc->next;
   }

   while ((toc) && (!toc->root)) {
      if (toc->htmlable)
	 count++;
      toc = toc->next;
   }

   return count;
}



void output_toc(FILE *f, char *filename, int root, int body, int part)
{
   char name[256];
   char *s;
   TOC *toc;
   int nested = 0;
   int section_number = 0;

   #define ALT_TEXT(toc)   ((toc->alt) ? toc->alt : toc->text)

   if (root) {
      toc = tochead;
      if (toc)
	 toc = toc->next;

      fprintf(f, "<ul><h4>\n");

      while (toc) {
	 if ((toc->root) && (toc->htmlable)) {
	    if (toc->otherfile) {
	       sprintf(name, "%s.%s", toc->text, html_extension);
	       mystrlwr(name);
	       hfprintf(f, "<li><a href=\"%s\">%s</a>\n", name, ALT_TEXT(toc));
	    }
	    else if (body) {
	       hfprintf(f, "<li><a href=\"#%s\">%s</a>\n", toc->text, ALT_TEXT(toc));
	       section_number++;
	    }
	    else {
	       strcpy(name, filename);
	       s = extension(name)-1;
	       if ((int)s - (int)get_filename(name) > 5)
		  s = get_filename(name)+5;
	       sprintf(s, "%03d.%s", section_number, html_extension);
	       hfprintf(f, "<li><a href=\"%s\">%s</a>\n", get_filename(name), ALT_TEXT(toc));
	       section_number++;
	    }
	 }
	 toc = toc->next;
      }

      fprintf(f, "</h4></ul><p>\n");
   }

   if (body) {
      toc = tochead;
      if (toc)
	 toc = toc->next;

      if (part <= 0) {
	 if (root)
	    fprintf(f, "<ul><h2>\n");
	 else
	    fprintf(f, "<ul><h4>\n");

	 while (toc) {
	    if ((toc->htmlable) && (!toc->otherfile)) {
	       if (!toc->root) {
		  if (!nested) {
		     fprintf(f, "<ul><h4>\n");
		     nested = 1;
		  }
	       }
	       else {
		  if (nested) {
		     fprintf(f, "</h4></ul><p>\n");
		     nested = 0;
		  }
	       }

	       hfprintf(f, "<li><a href=\"#%s\">%s</a>\n", toc->text, ALT_TEXT(toc));
	    }

	    toc = toc->next;
	 }

	 if (nested)
	    fprintf(f, "</h4></ul>\n");

	 if (root)
	    fprintf(f, "</h2></ul>\n");
	 else
	    fprintf(f, "</h4></ul>\n");
      }
      else {
	 section_number = 0;
	 fprintf(f, "<p>\n<ul><h4>\n");

	 while ((toc) && (section_number < part)) {
	    if ((toc->root) && (!toc->otherfile))
	       section_number++;
	    toc = toc->next;
	 }

	 while ((toc) && (!toc->root)) {
	    if (toc->htmlable)
	       hfprintf(f, "<li><a href=\"#%s\">%s</a>\n", toc->text, ALT_TEXT(toc));
	    toc = toc->next;
	 }

	 fprintf(f, "</h4></ul>\n<p><br><br>\n");
      }
   }
}



char *first_word(char *s)
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



int scmp(const void *e1, const void *e2)
{
   char *s1 = *((char **)e1);
   char *s2 = *((char **)e2);

   return mystricmp(s1, s2);
}



void output_texinfo_toc(FILE *f, int root, int body, int part)
{
   #define TOC_SIZE     8192

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
	    s = first_word(toc->text);
	    tfprintf(f, "* %s::", s);
	    for (i=strlen(s); i<24; i++)
	       fputc(' ', f);
	    tfprintf(f, "%s\n", toc->text);
	 }
	 toc = toc->next;
      }
   }

   if (body) {
      toc = tochead;
      if (toc)
	 toc = toc->next;

      if (part <= 0) {
	 ptr = malloc(TOC_SIZE * sizeof(char *));
	 i = 0;

	 while (toc) {
	    if ((!toc->root) && (toc->texinfoable) && (i < TOC_SIZE))
	       ptr[i++] = toc->text;

	    toc = toc->next;
	 }

	 if (i > 1)
	    qsort(ptr, i, sizeof(char *), scmp);

	 for (j=0; j<i; j++)
	    tfprintf(f, "* %s::\n", ptr[j]);

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
	       tfprintf(f, "* %s::\n", toc->text);
	    toc = toc->next;
	 }
      }
   }

   fprintf(f, "@end menu\n");
}



int valid_texinfo_node(char *s, char **next, char **prev)
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



char *node_name(int i)
{
   TOC *toc = tochead;

   if (toc)
      toc = toc->next;

   while (toc) {
      if ((toc->root) && (toc->texinfoable)) {
	 i--;
	 if (!i)
	    return first_word(toc->text);
      }

      toc = toc->next;
   }

   return "";
}



int write_txt(char *filename, int partial)
{
   LINE *line = head;
   char *p;
   int c, len;
   FILE *f;
   int outputting = !partial;

   printf("writing %s\n", filename);

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
      else if ((line->flags & TEXT_FLAG) && (outputting)) {
	 p = line->text;

	 if (line->flags & HEADING_FLAG) {
	    /* output a section heading */
	    p = strip_html(p);
	    len = strlen(p);
	    for (c=0; c<len+26; c++)
	       fputc('=', f);
	    afputs("\n============ ", f);
	    afputs(p, f);
	    afputs(" ============\n", f);
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
		  afputc(*p, f);
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



void output_headfooter(FILE *f, char *s, char *t)
{
   char buf[256];
   int i, n;

   strcpy(buf, s);

   if (buf[0]) {
      for (i=0; buf[i]; i++) {
	 if (strincmp(buf+i, ".html") == 0) {
	    i++;
	    n = strlen(html_extension);
	    memcpy(buf+i, html_extension, n);
	    if (n < 4)
	       memmove(buf+i+n, buf+i+4, strlen(buf+4)+1);
	    i += n;
	 }
      }

      for (i=0; buf[i]; i++) {
	 if (buf[i] == '#')
	    fputs(strip_html(t), f);
	 else
	    fputc(buf[i], f);
      }
      fputs("\n", f);
   }
}



int write_html(char *filename)
{
   char buf[256];
   int empty_count = 0;
   int section_number = 0;
   LINE *line = head;
   FILE *f;
   char *s, *s2, *s3;
   int i;

   printf("writing %s\n", filename);

   f = fopen(filename, "w");
   if (!f)
      return 1;

   while (line) {
      if (line->flags & HTML_FLAG) {
	 if (line->flags & HEADING_FLAG) {
	    /* output a section heading, switching file as required */
	    if ((flags & MULTIFILE_FLAG) && (section_number > 0)) {
	       if (section_number > 1)
		  output_headfooter(f, filefooter, "");
	       fclose(f);
	       strcpy(buf, filename);
	       s = extension(buf)-1;
	       if ((int)s - (int)get_filename(buf) > 5)
		  s = get_filename(buf)+5;
	       sprintf(s, "%03d.%s", section_number-1, html_extension);
	       printf("writing %s\n", buf);
	       f = fopen(buf, "w");
	       if (!f)
		  return 1;
	       output_headfooter(f, fileheader, line->text);
	    }
	    hfprintf(f, "<h1>%s</h1>\n", line->text);
	    empty_count = 0;
	    if ((flags & MULTIFILE_FLAG) && 
		(!(line->flags & NOCONTENT_FLAG)) &&
		(section_number > 0)) {
	       output_toc(f, filename, 0, 1, section_number);
	    }
	    section_number++;
	 }
	 else if (line->flags & NODE_FLAG) {
	    /* output a node marker */
	    fprintf(f, "<p><hr><p>\n");
	 }
	 else if (line->flags & DEFINITION_FLAG) {
	    /* output a function definition */
	    hfprintf(f, "<b>%s</b>", line->text);
	    if (!(line->flags & CONTINUE_FLAG))
	       fputs("<br>", f);
	    fputs("\n", f);
	    empty_count = 0;
	 }
	 else if ((!(line->flags & NO_EOL_FLAG)) && 
		  (is_empty(strip_html(line->text)))) {
	    /* output an empty line */
	    if (empty_count)
	       hfprintf(f, "<br>%s\n", line->text);
	    else
	       hfprintf(f, "<p>%s\n", line->text);
	    empty_count++;
	 }
	 else if (strstr(line->text, "<email>") || strstr(line->text, "<link>")) {
	    /* special munging for links */
	    s = line->text;

	    do {
	       s2 = strstr(s, "<email>");
	       s3 = strstr(s, "<link>");

	       if ((s2) || (s3)) {
		  if ((s3) && ((!s2) || (s3 < s2)))
		     s2 = s3;

		  i = 0;
		  while (s < s2)
		     buf[i++] = *(s++);

		  buf[i] = 0;
		  hfputs(buf, f);

		  while (*s != '>')
		     s++;

		  s++;
		  i = 0;

		  while ((*s) && (*s != '<'))
		     buf[i++] = *(s++);

		  buf[i] = 0;

		  if (s2[1] == 'e')
		     hfprintf(f, "<a href=\"mailto:%s\">%s", buf, buf);
		  else
		     hfprintf(f, "<a href=\"%s\">%s", buf, buf);
	       }
	    } while (s2);

	    hfputs(s, f);
	    fputs("\n", f);
	    empty_count = 0;
	 }
	 else {
	    /* output a normal line */
	    hfputs(line->text, f);
	    fputs("\n", f);
	    empty_count = 0;
	 }
      }
      else if (line->flags & TOC_FLAG) {
	 if (flags & MULTIFILE_FLAG)
	    output_toc(f, filename, 1, 0, -1);
	 else if (line->flags & SHORT_TOC_FLAG)
	    output_toc(f, filename, 0, 1, -1);
	 else
	    output_toc(f, filename, 1, 1, -1);
      }

      line = line->next;
   }

   if ((flags & MULTIFILE_FLAG) && (section_number > 1))
      output_headfooter(f, filefooter, "");

   fclose(f);
   return 0;
}



void html2texinfo(FILE *f, char *p)
{
   char buf[256];
   int i = 0;

   while ((*p) && (*p != '>'))
      buf[i++] = *(p++);

   buf[i] = 0;

   if (mystricmp(buf, "pre") == 0) {
      fputs("@example\n", f);
      nostrip = 1;
      strip_indent = -1;
   }
   else if (mystricmp(buf, "/pre") == 0) {
      fputs("@end example\n", f);
      nostrip = 0;
      strip_indent = -1;
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



void write_textinfo_xref(FILE *f, char *xref)
{
   char *tok;

   tok = strtok(xref, ",;");

   while (tok) {
      while ((*tok) && (myisspace(*tok)))
	 tok++;
      tfprintf(f, "@xref{%s}.@*\n", tok);
      tok = strtok(NULL, ",;");
   }
}



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
		   (valid_texinfo_node(buf, &next, &prev))) {
		  if (toc_waiting) {
		     output_texinfo_toc(f, 0, 1, section_number-1);
		     toc_waiting = 0;
		  }
		  if (xrefs > 0) {
		     fputs("See also:@*\n", f);
		     for (i=0; i<xrefs; i++)
			write_textinfo_xref(f, xref[i]);
		     xrefs = 0;
		  }
		  tfprintf(f, "@node %s, %s, %s, %s\n", buf, next, prev, node_name(section_number-1));
		  tfprintf(f, "@section %s\n", buf);
	       }

	       fputs("@ftable @asis\n", f);
	       in_item = 1;
	    }
	 }

	 /* munge the indentation to make it look nicer */
	 if (!(line->flags & NO_INDENT_FLAG)) {
	    if (nostrip) {
	       if (strip_indent >= 0) {
		  for (i=0; (i<strip_indent) && (*p) && (myisspace(*p)); i++)
		     p++;
	       }
	       else {
		  if (!is_empty(p)) {
		     strip_indent = 0;
		     while ((*p) && (myisspace(*p))) {
			strip_indent++;
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
	    tfputs(p, f);
	    fputs("\n", f);
	 }
	 else if (line->flags & HTML_CMD_FLAG) {
	    /* process HTML commands */
	    while (*p) {
	       if (p[0] == '<') {
		  html2texinfo(f, p+1);
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
		     write_textinfo_xref(f, xref[i]);
		  xrefs = 0;
	       }
	       p = strip_html(p);
	       tfprintf(f, "@node %s, ", node_name(section_number));
	       tfprintf(f, "%s, ", node_name(section_number+1));
	       tfprintf(f, "%s, Top\n", node_name(section_number-1));
	       tfprintf(f, "@chapter %s\n", p);
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
		  html2texinfo(f, p+1);
		  while ((*p) && (*p != '>'))
		     p++;
		  if (*p)
		     p++;
	       }
	       /* output other characters */
	       else {
		  if ((*p == '{') || (*p == '}') || (*p == '@'))
		     fputc('@', f);
		  tfputc((unsigned char)*p, f);
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
	       output_texinfo_toc(f, 0, 1, section_number-1);
	       toc_waiting = 0;
	    }
	 }

	 if (valid_texinfo_node(line->text, &next, &prev)) {
	    if (xrefs > 0) {
	       fputs("See also:@*\n", f);
	       for (i=0; i<xrefs; i++)
		  write_textinfo_xref(f, xref[i]);
	       xrefs = 0;
	    }
	    tfprintf(f, "@node %s, %s, %s, %s\n", line->text, next, prev, node_name(section_number-1));
	    tfprintf(f, "@section %s\n", line->text);
	 }

	 fputs("@ftable @asis\n", f);
	 in_item = 1;
      }
      else if (line->flags & TOC_FLAG) {
	 output_texinfo_toc(f, 1, 0, 0);
      }
      else if (line->flags & INDEX_FLAG) {
	 output_texinfo_toc(f, 0, 1, 0);
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

   printf("writing %s\n", filename);

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

   if (mystricmp(locale, "china") == 0) {
      fputs("{\\fonttbl{\\f0\\froman\\fcharset134\\fprq2 _GB2312;}\n", f);
      fputs("{\\f1\\fmodern\\fcharset134\\fprq1 ;}\n", f);
      fputs("{\\f2\\froman\\fcharset134\\fprq2 ;}}\n", f);
   }
   else if (mystricmp(locale, "korea") == 0) {
      fputs("{\\fonttbl{\\f0\\froman\\fcharset129\\fprq2 ±¼¸²;}\n", f);
      fputs("{\\f1\\fmodern\\fcharset129\\fprq1 ;}\n", f);
      fputs("{\\f2\\froman\\fcharset129\\fprq2 ;}}\n", f);
   }
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
	       while ((l->next) && (is_empty(strip_html(l->text))))
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
		  rfputc((unsigned char)*p, f);
		  p++;
	       }
	       fputs("}", f);
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
	       rfputc((unsigned char)*p, f);
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
	 rfputs(line->text, f);
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
	 while ((l->next) && (is_empty(l->text)) && (l->next->flags == l->flags)) {
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



char *man_name(char *p)
{
   static char buf[256];
   int i = 0;

   while ((*p) && (*p != '"'))
      p++;

   if (*p == '"') {
      p++;

      while ((*p) && (*p != '"')) {
	 if ((!myisalnum(*p)) && (*p != '_'))
	    return NULL;

	 buf[i++] = *(p++);
      }

      if (i) {
	 buf[i] = 0;
	 return buf;
      }
   }

   return NULL;
}



void mfputs(char *p, FILE *f)
{
   int state = 0;
   int i;

   if (mpreformat) {
      if (mpreindent < 0) {
	 mpreindent = 0;

	 while ((*p) && (myisspace(*p))) {
	    mpreindent++;
	    p++;
	 }
      }
      else {
	 for (i=0; i<mpreindent; i++) {
	    if ((!*p) || (!myisspace(*p)))
	       break;
	    p++;
	 }
      }

      fputs("   ", f);
   }
   else {
      while ((*p) && (myisspace(*p)))
	 p++;
   }

   while (*p) {
      /* less-than HTML tokens */
      if ((p[0] == '&') && 
	  (mytolower(p[1]) == 'l') && 
	  (mytolower(p[2]) == 't')) {
	 fputc('<', f);
	 p += 3;
	 state = 1;
      }
      /* greater-than HTML tokens */
      else if ((p[0] == '&') && 
	       (mytolower(p[1]) == 'g') && 
	       (mytolower(p[2]) == 't')) {
	 fputc('>', f);
	 p += 3;
	 state = 1;
      }
      /* bold HTML tokens */
      else if ((p[0] == '<') &&
	       (mytolower(p[1]) == 'b') &&
	       (p[2] == '>')) {
	 if (state)
	    putc('\n', f);
	 fputs(".B ", f);
	 p += 3;
      }
      /* end bold HTML tokens */
      else if ((p[0] == '<') &&
	       (mytolower(p[1]) == '/') &&
	       (mytolower(p[2]) == 'b') &&
	       (p[3] == '>')) {
	 if (state)
	    fputc('\n', f);
	 state = -1;
	 p += 4;
      }
      /* line break HTML tokens */
      else if ((p[0] == '<') &&
	       (mytolower(p[1]) == 'b') &&
	       (mytolower(p[2]) == 'r') &&
	       (p[3] == '>')) {
	 if (state >= 0) {
	    fputc('\n', f);
	    state = 0;
	 }
	 p += 4;
      }
      /* enter preformatted mode */
      else if ((p[0] == '<') &&
	       (mytolower(p[1]) == 'p') &&
	       (mytolower(p[2]) == 'r') &&
	       (mytolower(p[3]) == 'e') &&
	       (p[4] == '>')) {
	 fputs("\n.nf\n", f);
	 state = -1;
	 mpreformat = 1;
	 mpreindent = -1;
	 p += 5;
      }
      /* leave preformatted mode */
      else if ((p[0] == '<') &&
	       (mytolower(p[1]) == '/') &&
	       (mytolower(p[2]) == 'p') &&
	       (mytolower(p[3]) == 'r') &&
	       (mytolower(p[4]) == 'e') &&
	       (p[5] == '>')) {
	 fputs("\n.fi\n", f);
	 state = -1;
	 mpreformat = 0;
	 p += 6;
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
	 if ((*p == '\'') || (*p == '\\'))
	    fputc('\\', f);

	 fputc(*p, f);
	 p++;
	 state = 1;
      }
   }

   if (state >= 0)
      fputc('\n', f);
}



void write_man_xref(FILE *f, char *xref, char *ext, int notfirst)
{
   char *tok, *p;

   tok = strtok(xref, ",;");

   while (tok) {
      while ((*tok) && (myisspace(*tok)))
	 tok++;

      p = tok;

      while (*p) {
	 if ((!myisalnum(*p)) && (*p != '_'))
	    break;
	 p++;
      }

      if (!*p) {
	 if (notfirst)
	    fprintf(f, ",\n");

	 fprintf(f, ".BR %s (%s)", tok, ext);
	 notfirst = 1;
      }

      tok = strtok(NULL, ",;");
   }
}



int write_man(char *filename)
{
   char buf[256], buf2[256];
   char *xref[256];
   int xrefs = 0;
   LINE *line = head;
   LINE *l2;
   FILE *f = NULL;
   FILE *f2;
   char *p;
   int i;

   while (line) {
      if (line->flags & (HEADING_FLAG | DEFINITION_FLAG | NODE_FLAG)) {
	 /* close off any previous manpage */
	 if (f) {
	    if (xrefs > 0) {
	       fprintf(f, ".SH SEE ALSO\n");
	       for (i=0; i<xrefs; i++)
		  write_man_xref(f, xref[i], extension(filename), i);
	       fprintf(f, "\n");
	       xrefs = 0;
	    }

	    fclose(f);
	    f = NULL;
	 }
      }

      if (line->flags & MAN_FLAG) {
	 if (line->flags & DEFINITION_FLAG) {
	    /* start a manpage */
	    p = man_name(line->text);

	    if (p) {
	       strcpy(buf, filename);
	       strcpy(get_filename(buf), p);
	       strcat(buf, ".");
	       strcat(buf, extension(filename));

	       printf("writing %s\n", buf);
	       f = fopen(buf, "w");
	       if (!f)
		  return 1;

	       fprintf(f, ".\\\" Generated by the Allegro makedoc utility\n");
	       fprintf(f, ".TH %s %s %s\n", p, extension(filename), manheader);
	       fprintf(f, ".SH NAME\n");

	       fprintf(f, "%s", p);

	       if (!(line->flags & CONTINUE_FLAG)) {
		  l2 = line->next;

		  while ((l2) && (l2->flags & DEFINITION_FLAG)) {
		     p = man_name(l2->text);

		     if (p)
			fprintf(f, ", %s", p);

		     l2 = l2->next;
		  }
	       }

	       fprintf(f, "\n.SH SYNOPSIS\n");

	       if (mansynopsis[0])
		  fprintf(f, ".B %s\n\n", mansynopsis);

               /* go ahead and find all the header lines */
               l2 = line;
               while (l2 && l2->flags & CONTINUE_FLAG) l2=l2->next;

               if (l2) {
                  l2 = l2->next;
                  while (l2 && (l2->flags & HEADER_FLAG)) {
                    fprintf(f, ".B %s\n", l2->text);
                    l2 = l2->next;
                    if (l2 && (l2->flags & HEADER_FLAG)) {
                       fprintf(f, ".br\n");
                    }
                  }
               }
               fprintf(f, ".sp\n");

	       fputs(".B ", f);
	       mfputs(line->text, f);

	       i = (line->flags & CONTINUE_FLAG);

	       while ((line->next) && (line->next->flags & DEFINITION_FLAG)) {
		  line = line->next;

		  if (!i) {
		     /* multiple entries require a crosslink file */
		     p = man_name(line->text);

		     if (p) {
			strcpy(buf2, filename);
			strcpy(get_filename(buf2), p);
			strcat(buf2, ".");
			strcat(buf2, extension(filename));

			printf("writing %s\n", buf2);
			f2 = fopen(buf2, "w");
			if (!f2)
			   return 1;

			fprintf(f2, ".so man%s/%s\n", extension(filename), get_filename(buf));
			fclose(f2);
		     }

		     fputc('\n', f);
		  }

		  fputs(".B ", f);
		  mfputs(line->text, f);
	       }

	       fprintf(f, ".SH DESCRIPTION\n");

	       mpreformat = 0;
	    }

	    xrefs = 0;
	 }
	 else {
	    /* normal output mode */
	    if (f)
	       mfputs(line->text, f);
	 }
      }
      else if (line->flags & XREF_FLAG) {
	 xref[xrefs++] = line->text;
      }

      line = line->next;
   }

   return 0;
}



int main(int argc, char *argv[])
{
   char filename[256] = "";
   char txtname[256] = "";
   char htmlname[256] = "";
   char texinfoname[256] = "";
   char rtfname[256] = "";
   char manname[256] = "";
   int err = 0;
   int partial = 0;
   int i;

   for (i=1; i<argc; i++) {
      if ((mystricmp(argv[i], "-ascii") == 0) && (i<argc-1)) {
	 strcpy(txtname, argv[++i]);
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
	 warn_on_long_lines = 1;
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
      printf("Usage: makedoc [outputs] [-part] [-warn] filename._tx\n\n");
      printf("Output formats:\n");
      printf("\t-ascii filename.txt\n");
      printf("\t-htm[l] filename.htm[l]\n");
      printf("\t-rtf filename.rtf\n");
      printf("\t-t[e]xi filename.t[e]xi\n");
      printf("\t-man filename.3\n");
      return 1;
   }

   if ((!txtname[0]) && (!htmlname[0]) && (!texinfoname[0]) && (!rtfname[0]) && (!manname[0])) {
      strcpy(txtname, filename);
      strcpy(extension(txtname), "txt");

      strcpy(htmlname, filename);
      strcpy(extension(htmlname), html_extension);

      strcpy(texinfoname, filename);
      strcpy(extension(texinfoname), texinfo_extension);

      strcpy(rtfname, filename);
      strcpy(extension(rtfname), "rtf");
   }

   if (read_file(filename, htmlname) != 0) {
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

   getout:

   free_data();

   return err;
}

