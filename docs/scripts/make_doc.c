/*
 * make_doc OPTIONS... [--] DOC-FILES
 *
 * Options are:
 *
 *    --protos PROTOS-FILE
 *    --to FORMAT (html, man, latex, texinfo, etc.)
 *    --raise-sections
 *
 * Unknown options are passed through to Pandoc.
 *
 * Environment variables: PANDOC
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#if defined(_BSD_SOURCE) || defined(_SVID_SOURCE) || (_XOPEN_SOURCE >= 500)
   #include <unistd.h>
#endif

#include "aatree.h"
#include "dawk.h"
#include "make_doc.h"


dstr pandoc          = "pandoc";
dstr pandoc_options  = "";
dstr protos_file     = "protos";
dstr to_format       = "html";
bool raise_sections  = false;
char tmp_preprocess_output[80];
char tmp_pandoc_output[80];

static Aatree *protos = &aa_nil;


static int process_options(int argc, char *argv[]);
static void load_prototypes(const char *filename);
static void generate_temp_file(char *filename);
static void remove_temp_files(void);


int main(int argc, char *argv[])
{
   argc = process_options(argc, argv);
   load_prototypes(protos_file);

   generate_temp_file(tmp_preprocess_output);
   generate_temp_file(tmp_pandoc_output);
   d_cleanup = remove_temp_files;

   if (0 == strcmp(to_format, "man"))
      make_man_pages(argc, argv);
   else
      make_single_doc(argc, argv);

   d_cleanup();
   return 0;
}


static int process_options(int argc, char *argv[])
{
   char *p;
   int i;

   p = getenv("PANDOC");
   if (p) {
      d_assign(pandoc, p);
   }

   for (i = 1; i < argc; ) {
      if (streq(argv[i], "--")) {
         i++;
         break;
      }
      if (argv[i][0] != '-') {
         break;
      }
      if (streq(argv[i], "--protos")) {
         d_assign(protos_file, argv[i + 1]);
         i += 2;
      }
      else if (streq(argv[i], "--to")) {
         d_assign(to_format, argv[i + 1]);
         i += 2;
      }
      else if (streq(argv[i], "--raise-sections")) {
         raise_sections = true;
         i++;
      }
      else {
         /* Other options are assumed to be Pandoc options. */
         strcat(pandoc_options, " ");
         strcat(pandoc_options, argv[i]);
         i++;
         if (i < argc && argv[i][0] != '-') {
            strcat(pandoc_options, " ");
            strcat(pandoc_options, argv[i]);
            i++;
         }
      }
   }

   /* Shift arguments, including sentinel, but not the command name. */
   memmove(argv + 1, argv + i, (argc - i + 1) * sizeof(char *));
   return argc - (i - 1);
}


static void load_prototypes(const char *filename)
{
   dstr line;
   const char *name;
   const char *newtext;
   dstr text;

   d_open_input(filename);

   while (d_getline(line)) {
      if (d_match(line, "([^:]*): (.*)")) {
         name = d_submatch(1);
         newtext = d_submatch(2);

         d_assign(text, lookup_prototype(name));
         strcat(text, "\n    ");
         strcat(text, newtext);
         protos = aa_insert(protos, name, text);
      }
   }

   d_close_input();
}


const char *lookup_prototype(const char *name)
{
   const char *r = aa_search(protos, name);
   return (r) ? r : "";
}


void generate_temp_file(char *filename)
{
   /* gcc won't shut up if we use tmpnam() so we'll use mkstemp() if it is
    * likely to be available.
    */
#if defined(_BSD_SOURCE) || defined(_SVID_SOURCE) || (_XOPEN_SOURCE >= 500)
   int fd;
   strcpy(filename, ",make_doc_tmp.XXXXXX");
   fd = mkstemp(filename);
   if (fd == -1) {
      d_abort("could not generate temporary file name", "");
   }
   close(fd);
#else
   if (!tmpnam(filename)) {
      d_abort("could not generate temporary file name", "");
   }
#endif
}


static void remove_temp_files(void)
{
   remove(tmp_preprocess_output);
   remove(tmp_pandoc_output);
}


void call_pandoc(const char *input, const char *output,
   const char *extra_options)
{
   dstr cmd;
   dstr input_native;
   char *p;

   strcpy(input_native, input);

   /* Use native Windows syntax to avoid "c:/foo.txt" being treated as a
    * remote URI by Pandoc 1.5 and 1.6.
    */
   if (strlen(input_native) > 2
         && isalpha(input_native[0])
         && input_native[1] == ':') {
      for (p = input_native; *p != '\0'; p++) {
         if (*p == '/')
            *p = '\\';
      }
   }

   sprintf(cmd, "%s %s %s %s --to %s --output %s",
      pandoc, input_native, pandoc_options, extra_options, to_format, output);
   if (system(cmd) != 0) {
      d_abort("system call failed: ", cmd);
   }
}

/* vim: set sts=3 sw=3 et: */
