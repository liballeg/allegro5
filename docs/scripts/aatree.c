/*
 * AA tree, a type of self-balancing search tree.
 *
 * A. Andersson. "Balanced search trees made simple."
 * In Proc. Workshop on Algorithms and Data Structures, pages 60--71.
 * Springer Verlag, 1993.
 */

#include <stdlib.h>
#include <string.h>
#include "aatree.h"

Aatree aa_nil = { 0, &aa_nil, &aa_nil, "", "" };

static char *xstrdup(const char *s)
{
   size_t n = strlen(s);
   char *p = malloc(n + 1);
   memcpy(p, s, n);
   p[n] = '\0';
   return p;
}

static Aatree *skew(Aatree *T)
{
   if (T == &aa_nil)
      return T;
   if (T->left->level == T->level) {
      Aatree *L = T->left;
      T->left = L->right;
      L->right = T;
      return L;
   }
   return T;
}

static Aatree *split(Aatree *T)
{
   if (T == &aa_nil)
      return T;
   if (T->level == T->right->right->level) {
      Aatree *R = T->right;
      T->right = R->left;
      R->left = T;
      R->level = R->level + 1;
      return R;
   }
   return T;
}

Aatree *aa_singleton(const char *key, const char *value)
{
   Aatree *T = malloc(sizeof(Aatree));
   T->level = 1;
   T->left = &aa_nil;
   T->right = &aa_nil;
   T->key = xstrdup(key);
   T->value = xstrdup(value);
   return T;
}

Aatree *aa_insert(Aatree *T, const char *key, const char *value)
{
   int cmp;
   if (T == &aa_nil) {
      return aa_singleton(key, value);
   }
   cmp = strcmp(key, T->key);
   if (cmp < 0) {
      T->left = aa_insert(T->left, key, value);
   } else if (cmp > 0) {
      T->right = aa_insert(T->right, key, value);
   } else {
      free(T->value);
      T->value = xstrdup(value);
   }
   T = skew(T);
   T = split(T);
   return T;
}

const char *aa_search(const Aatree *T, const char *key)
{
   while (T != &aa_nil) {
      int cmp = strcmp(key, T->key);
      if (cmp == 0)
         return T->value;
      T = (cmp < 0) ? T->left : T->right;
   }
   return NULL;
}

void aa_destroy(Aatree *T)
{
   if (T != &aa_nil) {
      aa_destroy(T->left);
      aa_destroy(T->right);
      free(T->key);
      free(T->value);
      free(T);
   }
}

/* vim: set sts=3 sw=3 et: */
