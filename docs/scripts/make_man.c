#include <stdio.h>
#include "aatree.h"
#include "dawk.h"
#include "make_doc.h"

static const char *SECTION = "3";
static const char *MANUAL = "Allegro reference manual";
static dstr last_header;

static void man_page_header(const char *name)
{
   d_printf("%% %s(%s) %s\n", name, SECTION, MANUAL);
   d_print("# NAME");
   d_print(name);
   d_print("\n# SYNOPSIS");
   d_print(last_header);
   d_print(lookup_prototype(name));
   d_print("\n# DESCRIPTION");
}

static void call_pandoc_for_man(const char *input, const char *name)
{
   dstr output_filename;

   sprintf(output_filename, "%s.%s", name, SECTION);
   call_pandoc(input, output_filename, "--standalone");
}

void make_man_pages(int argc, char *argv[])
{
   bool output = false;
   dstr name = "";
   dstr line;

   d_init(argc, argv);

   while (d_getline(line)) {
      if (line[0] == '#' && output) {
         d_close_output();
         call_pandoc_for_man(tmp_preprocess_output, name);
         output = false;
      }

      if (d_match(line, "^ *#include ") && !output) {
         d_assign(last_header, line);
         continue;
      }

      if (d_match(line, "^#+ API: *")) {
         d_assign(name, d_after_match);
         d_open_output(tmp_preprocess_output);
         man_page_header(name);
         output = true;
         continue;
      }

      if (!output) {
         continue;
      }

      if (d_match(line, "^\\*?Return [Vv]alue:\\*?")) {
         d_print("# RETURN VALUE");
         d_assign(line, d_after_match);
      }

      if (d_match(line, "^\\*?See [Aa]lso:\\*?")) {
         d_print("# SEE ALSO");
         d_assign(line, d_after_match);
      }

      /* Replace [al_foo] and [ALLEGRO_foo] by al_foo(3) and ALLEGRO_foo(3). */
      while (d_match(line, "\\[((al_|ALLEGRO_)[^\\]]*)\\]")) {
         d_print(d_before_match);
         d_printf("%s(%s)", d_submatch(1), SECTION);
         d_assign(line, d_after_match);
      }
      d_print(line);
   }

   if (output) {
      d_close_output();
      call_pandoc_for_man(tmp_preprocess_output, name);
      output = false;
   }
}

/* vim: set sts=3 sw=3 et: */
