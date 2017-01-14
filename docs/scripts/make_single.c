#include <string.h>
#include <stdlib.h>
#include "dawk.h"
#include "make_doc.h"

static void preprocess(void);
static void postprocess_latex(void);
static void cat(void);
static void print_sans_backslashes(const char *p);

void make_single_doc(int argc, char *argv[])
{
   d_init(argc, argv);

   /* Hidden format for debugging. */
   if (streq(to_format, "preprocess")) {
      preprocess();
      return;
   }

   d_open_output(tmp_preprocess_output);
   preprocess();
   d_close_output();

   call_pandoc(tmp_preprocess_output, tmp_pandoc_output, "");

   d_open_input(tmp_pandoc_output);
   if (streq(to_format, "latex"))
      postprocess_latex();
   else
      cat();
   d_close_input();
}

static void preprocess(void)
{
   dstr line;

   while (d_getline(line)) {
      /* Raise sections by one level. Top-most becomes the document title. */
      if (line[0] == '#' && raise_sections) {
         if (line[1] == ' ') {
            line[0] = '%';
         }
         else {
            char *p = strchr(line, ' ');
            if (p) {
               p[-1] = ' ';
            }
         }
      }

      /* Make sure there is a blank line between input files so paragraphs
       * across files don't get merged. But don't break document titles which
       * must be on the first line.
       */
      if (d_line_num == 1 && line[0] != '%') {
         d_print("");
      }

      if (d_match(line, "^(#+) +API: *")) {
         const char *hashes = d_submatch(1);
         const char *name = d_after_match;
         const char *text = lookup_prototype(name);
         const char *source = lookup_source(name);

         d_printf("%s %s\n", hashes, name);
         if (strcmp(text, "") != 0) {
            d_printf("~~~~c");
            d_print(text);
            d_printf("~~~~");
         }
         d_printf("\n[Source Code](%s)\n", source);
      }
      else if (d_match(line, "^__ALLEGRO_5_CFG")) {
         char *allegro5_cfg = load_allegro5_cfg();
         d_print(allegro5_cfg);
         free(allegro5_cfg);
      }
      else {
         d_print(line);
      }
   }
}

static void postprocess_latex(void)
{
   dstr line;

   while (d_getline(line)) {
      /* Insert \label{id} for all sections which are probably API entries.
       * T-Rex doesn't seem to backtrack properly if we write "(sub)*".
       */
      if (d_match(line,
	    "\\\\(sub)?(sub)?(sub)?(sub)?section\\{((al|ALLEGRO)[^}]+)")) {
         d_print(line);
         d_printf("\\label{");
         print_sans_backslashes(d_submatch(5));
         d_printf("}\n");
         continue;
      }

      /* Replace `verbatim' environment by `Verbatim' from fancyvrb,
       * which gives us some flexibility over the formatting.
       */
      if (d_match(line, "\\\\begin\\{verbatim\\}$")) {
         d_printf("%s", d_before_match);
         d_print("\\begin{Verbatim}");
         continue;
      }
      if (d_match(line, "\\\\end\\{verbatim\\}$")) {
         d_printf("%s", d_before_match);
         d_print("\\end{Verbatim}");
         continue;
      }

      d_print(line);
   }
}

static void cat(void)
{
   dstr line;
   while (d_getline(line))
      d_print(line);
}

static void print_sans_backslashes(const char *p)
{
   for (; *p; p++) {
      if (*p != '\\')
         fputc(*p, D_STDOUT);
   }
}

/* vim: set sts=3 sw=3 et: */
