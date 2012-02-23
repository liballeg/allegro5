/*
 * make_index HTML-REFS-FILE...
 *
 * Generate a file containing a list of links to all API entries.
 * The links are sorted using strcmp.
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

static void print_link(const char *value)
{
   d_printf("* [%s]\n", value);

   /* Work around Pandoc issue #182. */
   d_printf("<!-- -->\n");
}

static void pre_order_traversal(Aatree *node, void (*doit)(const char *))
{
   if (node->left != &aa_nil) {
      pre_order_traversal(node->left, doit);
   }

   doit(node->value);

   if (node->right != &aa_nil) {
      pre_order_traversal(node->right, doit);
   }
}

int main(int argc, char *argv[])
{
   dstr line;
   Aatree * root = &aa_nil;

   d_init(argc, argv);

   d_printf("# Index\n");

   while ((d_getline(line))) {
      if (d_match(line, "^\\[([^\\]]*)")) {
         const char *ref = d_submatch(1);
         char *s = xstrdup(ref);
         root = aa_insert(root, s, s);
      }
   }

   pre_order_traversal(root, print_link);

   aa_destroy(root);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
