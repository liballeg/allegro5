/*
 * scan_examples OPTIONS EXAMPLE-FILES
 *
 * Load a prototypes file containing API elements
 * and scan files for examples using those elements
 * Output is to standard output and comprises lines
 * of the form "API: FILE:LINE FILE:LINE FILE:LINE"
 * No more than 3 examples will be output for each
 * API.
 *
 * Options are:
 *    --protos PROTOS-FILE
 *
 * It is possible to use a response file to specify
 * options or files. Including @FILENAME on the
 * command line will read that file line-by-line
 * and insert each line as if it were specified on
 * the command line. This is to avoid very long
 * command lines.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dawk.h"

#define SL_INITIAL_CAPACITY 64
#define EXAMPLES_PER_API 3

static dstr protos;

typedef struct {
   char **items;
   int count;
   int capacity;
} slist_t;

static void sl_init(slist_t *);
static void sl_free(slist_t *);
static void sl_clear(slist_t *);
static void sl_append(slist_t *, const char *);

static void cleanup(void);
static void load_api_list(void);
static slist_t apis;

static char **responsefile(int* pargc, char **argv);


/* Table of which APIs are in which examples */
static int *table;
/* Lookup to index the best examples */
typedef struct {
   /* Index into table */
   int index;
   /* How many APIs are used in each example */
   int count;
   /* Store a pointer to the file string for comparison purposes */
   char *filename;
} lookup_t;
static lookup_t *lookup;
static int number_of_items = 0;
static int compare(const void *pa, const void *pb) {
   int val = ((const lookup_t *) pa)->count - ((const lookup_t *) pb)->count;

   // if different count values, sort according to this.
   if (val != 0) return val;

   // But if the count value is the same, sort according to filename
   return strcmp( (char*)((const lookup_t *) pa)->filename, (char*)((const lookup_t *) pb)->filename);
}

int main(int argc, char* argv[])
{
   dstr line;
   int i, j;
   argv = responsefile(&argc, argv);
   /* Handle --protos flag */
   if (argc >=3 && strcmp(argv[1], "--protos") == 0) {
      strcpy(protos, argv[2]);
      memmove(&argv[1], &argv[3], (&argv[argc] - &argv[3]) * sizeof(char*));
      argc -= 2;
   } else {
      strcpy(protos, "protos");
   }
   d_cleanup = cleanup;
   sl_init(&apis);
   load_api_list();
   table = calloc(apis.count * argc, sizeof(int));

   lookup = calloc(argc, sizeof(lookup_t));
   for (j = 0; j < argc; ++j) {
      lookup[j].index = j;
      lookup[j].filename = argv[j];
   }

   number_of_items = argc;

   for (j = 1; j < argc; ++j) {
      d_open_input(argv[j]);
      while (d_getline(line)) {
         for (i = 0; i < apis.count; ++i) {
            if (strstr(line, apis.items[i])) {
               int *ptr = &table[i + j * apis.count];
               if (*ptr == 0) {
                  *ptr = d_line_num;
                  ++lookup[j].count;
               }
            }
         }
      }
      d_close_input();
   }
   /* Sort the files */
   qsort(lookup, argc, sizeof(lookup_t), compare);
   /* Output the EXAMPLES_PER_API (three) 'best' examples.
    *
    * The 'best' is defined as the probability of having a usage of an API in
    * the set of all other API usages in the example. Effectively, this means
    * that examples with fewer other APIs showcased get selected.
    */
   for (i = 0; i < apis.count; ++i) {
      int found = 0;
      for (j = 0; j < argc && found < EXAMPLES_PER_API; ++j) {
            int index = lookup[j].index;
            int line_num = table[i + index * apis.count];
            if (line_num != 0) {
               if (found == 0) {
                     d_printf("%s: ", apis.items[i]);
               }
               ++found;
               d_printf("%s:%d ", argv[index], line_num);
            }
      }
      if (found > 0) {
            d_print("\n");
      }
   }
   d_cleanup();
   return 0;
}

void cleanup(void)
{
   free(table);
   free(lookup);
   sl_free(&apis);
}

void sl_init(slist_t* s)
{
   s->items = NULL;
   s->count = s->capacity = 0;
}

void sl_append(slist_t *s, const char *item)
{
   if (s->count == s->capacity) {
      int capacity = s->capacity == 0 ? SL_INITIAL_CAPACITY : (s->capacity * 2);
      s->items = realloc(s->items, capacity * sizeof(char*));
      s->capacity = capacity;
   }
   s->items[s->count++] = strcpy(malloc(1+strlen(item)), item);
}

void sl_free(slist_t *s)
{
   sl_clear(s);
   free(s->items);
   s->items = NULL;
   s->capacity = 0;
}

void sl_clear(slist_t *s)
{
   int i;
   for (i = 0; i < s->count; ++i) {
         free(s->items[i]);
   }
   s->count = 0;
}

void load_api_list(void)
{
   dstr line;
   d_open_input(protos);
   while (d_getline(line)) {
         int i;
         bool found = false;
         char *ptr = strchr(line, ':');
         if (ptr) {
            *ptr = '\0';
            for (i = apis.count - 1; i >=0; --i) {
               if (strcmp(line, apis.items[i]) == 0) {
                  found = true;
                  break;
               }
            }
         }
         if (!found) {
            sl_append(&apis, line);
         }
   }
   d_close_input();
}

/* Re-process the command line args by loading any response files */
char **responsefile(int *pargc, char **argv)
{
   static slist_t args;
   static char** new_argv;
   int argc = *pargc;
   int i;
   bool found_at = false;
   for (i = 1; i < argc; ++i) {
         if (*argv[i] == '@') {
            found_at = true;
            break;
         }
   }
   if (!found_at) {
         /* Nothing to do */
         return argv;
   }
   sl_clear(&args);
   for (i = 0; i < argc; ++i) {
         if (*argv[i] == '@') {
            d_open_input(argv[i] + 1);
            dstr line;
            while (d_getline(line)) {
                  sl_append(&args, line);
            }
            d_close_input();
         } else {
            sl_append(&args, argv[i]);
         }
   }
   /* Make a copy because code might alter the argv array */
   new_argv = realloc(new_argv, args.count * sizeof(char*));
   memcpy(new_argv, args.items, args.count * sizeof(char*));
   *pargc = args.count;
   return new_argv;
}



/* Local Variables: */
/* c-basic-offset: 3 */
/* indent-tabs-mode: nil */
/* End: */
/* vim: set sts=3 sw=3 et: */
