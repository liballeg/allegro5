/*
 * make_index HTML-REFS-FILE...
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

#include <string.h>
#include <stdlib.h>
#include "dawk.h"
#include "aatree.h"

static char *xstrdup(const char *s)
{
   size_t n = strlen(s);
   char *p = malloc(n + 1);
   memcpy(p, s, n);
   p[n] = '\0';
   return p;
}


static void print_it(const char * value){
   /* We need an extra newline to make pandoc know this is the only thing in the link */
   d_printf("[%s][%s]\n\n", value, value);
}

static void pre_order_traversal(Aatree * node, void (*doit)(const char *)){
   if (node->left != &aa_nil){
      pre_order_traversal(node->left, doit);
   }

   doit(node->value);

   if (node->right != &aa_nil){
      pre_order_traversal(node->right, doit);
   }
}

int main(int argc, char *argv[])
{
   dstr line;
   Aatree * root = &aa_nil;

   d_init(argc, argv);

   while ((d_getline(line))) {
      if (d_match(line, "^\\[([^\\]]*)")) {
         const char *ref = d_submatch(1);
         root = aa_insert(root, xstrdup(ref), xstrdup(ref));
      }
   }

   pre_order_traversal(root, print_it);

   aa_destroy(root);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
