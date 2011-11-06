/*
 * make_dummy_refs DOC-FILE...
 *
 * Generate a file containing dummy link definitions for each API
 * entry found.  e.g. for "# API: foo" we generate:
 *
 *   [foo]: DUMMYREF
 *
 * Then a reference in a source document will expand to something containing
 * "DUMMYREF" in the output.  That makes it possible to post-process generated
 * TexInfo and LaTeX documents to fix up cross-references.  It's unfortunately
 * necessary as Pandoc currently doesn't have very good cross-reference
 * support.
 */

#include "dawk.h"

int main(int argc, char *argv[])
{
   dstr line;

   d_init(argc, argv);

   while ((d_getline(line))) {
      if (d_match(line, "^#+ API: *")) {
         d_printf("[%s]: DUMMYREF\n", d_after_match);
      }
   }

   return 0;
}

/* vim: set sts=3 sw=3 et: */
