/*
 * make_index HTML-REFS-FILE...
 *
 * Generate a file containing real pandoc links for each link reference found.
 * The links are sorted using strcmp.
 *
 * Input:
 *   [al_create_bitmap]: graphics#al_create_bitmap
 *
 * Output:
 *   [al_create_bitmap][al_create_bitmap]
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
