/*
 *    Example program for the Allegro library.
 *
 *    Stress test path routines.
 */

#include <allegro5/allegro.h>
#include <stdio.h>

#include "common.c"

#ifdef A5O_MSVC
   #pragma warning (disable: 4066)
#endif

typedef void (*test_t)(void);

int error = 0;

#define CHECK(x)                                                            \
   do {                                                                     \
      bool ok = (bool)(x);                                                  \
      if (!ok) {                                                            \
         log_printf("FAIL %s\n", #x);                                       \
         error++;                                                           \
      } else {                                                              \
         log_printf("OK   %s\n", #x);                                       \
      }                                                                     \
   } while (0)

#define CHECK_EQ(x,y)   CHECK(0 == strcmp(x, y))

/*---------------------------------------------------------------------------*/

/* Test al_create_path, al_get_path_num_components, al_get_path_component,
 * al_get_path_drive, al_get_path_filename, al_destroy_path.
 */
static void t1(void)
{
   A5O_PATH *path;

   CHECK(path = al_create_path(NULL));
   al_destroy_path(path);

   CHECK(path = al_create_path(""));
   CHECK(al_get_path_num_components(path) == 0);
   CHECK_EQ(al_get_path_drive(path), "");
   CHECK_EQ(al_get_path_filename(path), "");
   al_destroy_path(path);

   /* . is a directory component. */
   CHECK(path = al_create_path("."));
   CHECK(al_get_path_num_components(path) == 1);
   CHECK_EQ(al_get_path_component(path, 0), ".");
   CHECK_EQ(al_get_path_drive(path), "");
   CHECK_EQ(al_get_path_filename(path), "");
   al_destroy_path(path);

   /* .. is a directory component. */
   CHECK(path = al_create_path(".."));
   CHECK(al_get_path_num_components(path) == 1);
   CHECK_EQ(al_get_path_component(path, 0), "..");
   CHECK_EQ(al_get_path_drive(path), "");
   CHECK_EQ(al_get_path_filename(path), "");
   al_destroy_path(path);

   /* Relative path. */
   CHECK(path = al_create_path("abc/def/.."));
   CHECK(al_get_path_num_components(path) == 3);
   CHECK_EQ(al_get_path_component(path, 0), "abc");
   CHECK_EQ(al_get_path_component(path, 1), "def");
   CHECK_EQ(al_get_path_component(path, 2), "..");
   CHECK_EQ(al_get_path_drive(path), "");
   CHECK_EQ(al_get_path_filename(path), "");
   al_destroy_path(path);

   /* Absolute path. */
   CHECK(path = al_create_path("/abc/def/.."));
   CHECK(al_get_path_num_components(path) == 4);
   CHECK_EQ(al_get_path_component(path, 0), "");
   CHECK_EQ(al_get_path_component(path, 1), "abc");
   CHECK_EQ(al_get_path_component(path, 2), "def");
   CHECK_EQ(al_get_path_component(path, 3), "..");
   CHECK_EQ(al_get_path_drive(path), "");
   CHECK_EQ(al_get_path_filename(path), "");
   al_destroy_path(path);

   /* Directories + filename. */
   CHECK(path = al_create_path("/abc/def/ghi"));
   CHECK(al_get_path_num_components(path) == 3);
   CHECK_EQ(al_get_path_component(path, 0), "");
   CHECK_EQ(al_get_path_component(path, 1), "abc");
   CHECK_EQ(al_get_path_component(path, 2), "def");
   CHECK_EQ(al_get_path_drive(path), "");
   CHECK_EQ(al_get_path_filename(path), "ghi");
   al_destroy_path(path);
}

/* Test parsing UNC paths. */
static void t2(void)
{
#ifdef A5O_WINDOWS
   A5O_PATH *path;

   /* The mixed slashes are deliberate. */
   /* Good paths. */
   CHECK(path = al_create_path("//server\\share name/dir/filename"));
   CHECK_EQ(al_get_path_drive(path), "//server");
   CHECK(al_get_path_num_components(path) == 2);
   CHECK_EQ(al_get_path_component(path, 0), "share name");
   CHECK_EQ(al_get_path_component(path, 1), "dir");
   CHECK_EQ(al_get_path_filename(path), "filename");
   al_destroy_path(path);

   /* Bad paths. */
   CHECK(! al_create_path("//"));
   CHECK(! al_create_path("//filename"));
   CHECK(! al_create_path("///share/name/filename"));
#else
   log_printf("Skipping Windows-only test...\n");
#endif
}

/* Test parsing drive letter paths. */
static void t3(void)
{
#ifdef A5O_WINDOWS
   A5O_PATH *path;

   /* The mixed slashes are deliberate. */

   CHECK(path = al_create_path("c:abc\\def/ghi"));
   CHECK_EQ(al_get_path_drive(path), "c:");
   CHECK(al_get_path_num_components(path) == 2);
   CHECK_EQ(al_get_path_component(path, 0), "abc");
   CHECK_EQ(al_get_path_component(path, 1), "def");
   CHECK_EQ(al_get_path_filename(path), "ghi");
   CHECK_EQ(al_path_cstr(path, '\\'), "c:abc\\def\\ghi");
   al_destroy_path(path);

   CHECK(path = al_create_path("c:\\abc/def\\ghi"));
   CHECK_EQ(al_get_path_drive(path), "c:");
   CHECK(al_get_path_num_components(path) == 3);
   CHECK_EQ(al_get_path_component(path, 0), "");
   CHECK_EQ(al_get_path_component(path, 1), "abc");
   CHECK_EQ(al_get_path_component(path, 2), "def");
   CHECK_EQ(al_get_path_filename(path), "ghi");
   CHECK_EQ(al_path_cstr(path, '\\'), "c:\\abc\\def\\ghi");
   al_destroy_path(path);
#else
   log_printf("Skipping Windows-only test...\n");
#endif
}

/* Test al_append_path_component. */
static void t4(void)
{
   A5O_PATH *path = al_create_path(NULL);

   CHECK(al_get_path_num_components(path) == 0);

   al_append_path_component(path, "abc");
   al_append_path_component(path, "def");
   al_append_path_component(path, "ghi");

   CHECK(al_get_path_num_components(path) == 3);

   CHECK_EQ(al_get_path_component(path, 0), "abc");
   CHECK_EQ(al_get_path_component(path, 1), "def");
   CHECK_EQ(al_get_path_component(path, 2), "ghi");

   CHECK_EQ(al_get_path_component(path, -1), "ghi");
   CHECK_EQ(al_get_path_component(path, -2), "def");
   CHECK_EQ(al_get_path_component(path, -3), "abc");

   al_destroy_path(path);
}

/* Test al_replace_path_component. */
static void t5(void)
{
   A5O_PATH *path = al_create_path(NULL);

   al_append_path_component(path, "abc");
   al_append_path_component(path, "INKY");
   al_append_path_component(path, "def");
   al_append_path_component(path, "BLINKY");
   al_append_path_component(path, "ghi");

   CHECK(al_get_path_num_components(path) == 5);

   al_replace_path_component(path, 1, "PINKY");
   al_replace_path_component(path, -2, "CLYDE");

   CHECK(al_get_path_num_components(path) == 5);

   CHECK_EQ(al_get_path_component(path, 0), "abc");
   CHECK_EQ(al_get_path_component(path, 1), "PINKY");
   CHECK_EQ(al_get_path_component(path, 2), "def");
   CHECK_EQ(al_get_path_component(path, 3), "CLYDE");
   CHECK_EQ(al_get_path_component(path, 4), "ghi");

   al_destroy_path(path);
}

/* Test al_remove_path_component. */
static void t6(void)
{
   A5O_PATH *path = al_create_path(NULL);

   al_append_path_component(path, "abc");
   al_append_path_component(path, "INKY");
   al_append_path_component(path, "def");
   al_append_path_component(path, "BLINKY");
   al_append_path_component(path, "ghi");

   CHECK(al_get_path_num_components(path) == 5);

   al_remove_path_component(path, 1);
   CHECK(al_get_path_num_components(path) == 4);

   al_remove_path_component(path, -2);
   CHECK(al_get_path_num_components(path) == 3);

   CHECK_EQ(al_get_path_component(path, 0), "abc");
   CHECK_EQ(al_get_path_component(path, 1), "def");
   CHECK_EQ(al_get_path_component(path, 2), "ghi");

   al_destroy_path(path);
}

/* Test al_insert_path_component. */
static void t7(void)
{
   A5O_PATH *path = al_create_path("INKY/BLINKY/");

   al_insert_path_component(path, 0, "abc");
   al_insert_path_component(path, 2, "def");
   al_insert_path_component(path, 4, "ghi");

   CHECK(al_get_path_num_components(path) == 5);
   CHECK_EQ(al_get_path_component(path, 0), "abc");
   CHECK_EQ(al_get_path_component(path, 1), "INKY");
   CHECK_EQ(al_get_path_component(path, 2), "def");
   CHECK_EQ(al_get_path_component(path, 3), "BLINKY");
   CHECK_EQ(al_get_path_component(path, 4), "ghi");

   al_destroy_path(path);
}

/* Test al_get_path_tail, al_drop_path_tail. */
static void t8(void)
{
   A5O_PATH *path = al_create_path(NULL);

   CHECK(! al_get_path_tail(path));

   al_append_path_component(path, "abc");
   al_append_path_component(path, "def");
   al_append_path_component(path, "ghi");
   CHECK_EQ(al_get_path_tail(path), "ghi");

   al_drop_path_tail(path);
   CHECK_EQ(al_get_path_tail(path), "def");

   al_drop_path_tail(path);
   al_drop_path_tail(path);
   CHECK(! al_get_path_tail(path));

   /* Drop tail from already empty path. */
   al_drop_path_tail(path);
   CHECK(! al_get_path_tail(path));

   al_destroy_path(path);
}

/* Test al_set_path_drive, al_set_path_filename, al_path_cstr.
 */
static void t9(void)
{
   A5O_PATH *path = al_create_path(NULL);

   CHECK_EQ(al_path_cstr(path, '/'), "");

   /* Drive letters. */
   al_set_path_drive(path, "c:");
   CHECK_EQ(al_path_cstr(path, '/'), "c:");
   CHECK_EQ(al_get_path_drive(path), "c:");

   al_set_path_drive(path, "d:");
   CHECK_EQ(al_path_cstr(path, '/'), "d:");

   /* Plus directory components. */
   al_append_path_component(path, "abc");
   al_append_path_component(path, "def");
   CHECK_EQ(al_path_cstr(path, '/'), "d:abc/def/");

   /* Plus filename. */
   al_set_path_filename(path, "uvw");
   CHECK_EQ(al_path_cstr(path, '/'), "d:abc/def/uvw");
   CHECK_EQ(al_get_path_filename(path), "uvw");

   /* Replace filename. */
   al_set_path_filename(path, "xyz");
   CHECK_EQ(al_path_cstr(path, '/'), "d:abc/def/xyz");

   /* Remove drive. */
   al_set_path_drive(path, NULL);
   CHECK_EQ(al_path_cstr(path, '/'), "abc/def/xyz");

   /* Remove filename. */
   al_set_path_filename(path, NULL);
   CHECK_EQ(al_path_cstr(path, '/'), "abc/def/");

   al_destroy_path(path);
}

/* Test al_join_paths. */
static void t10(void)
{
   A5O_PATH *path1;
   A5O_PATH *path2;

   /* Both empty. */
   path1 = al_create_path(NULL);
   path2 = al_create_path(NULL);
   al_join_paths(path1, path2);
   CHECK_EQ(al_path_cstr(path1, '/'), "");
   al_destroy_path(path1);
   al_destroy_path(path2);

   /* Both just filenames. */
   path1 = al_create_path("file1");
   path2 = al_create_path("file2");
   al_join_paths(path1, path2);
   CHECK_EQ(al_path_cstr(path1, '/'), "file2");
   al_destroy_path(path1);
   al_destroy_path(path2);

   /* Both relative paths. */
   path1 = al_create_path("dir1a/dir1b/file1");
   path2 = al_create_path("dir2a/dir2b/file2");
   al_join_paths(path1, path2);
   CHECK_EQ(al_path_cstr(path1, '/'),
      "dir1a/dir1b/dir2a/dir2b/file2");
   al_destroy_path(path1);
   al_destroy_path(path2);

#ifdef A5O_WINDOWS
   /* Both relative paths with drive letters. */
   path1 = al_create_path("d:dir1a/dir1b/file1");
   path2 = al_create_path("e:dir2a/dir2b/file2");
   al_join_paths(path1, path2);
   CHECK_EQ(al_path_cstr(path1, '/'), "d:dir1a/dir1b/dir2a/dir2b/file2");
   al_destroy_path(path1);
   al_destroy_path(path2);
#endif

   /* Path1 absolute, path2 relative. */
   path1 = al_create_path("/dir1a/dir1b/file1");
   path2 = al_create_path("dir2a/dir2b/file2");
   al_join_paths(path1, path2);
   CHECK_EQ(al_path_cstr(path1, '/'), "/dir1a/dir1b/dir2a/dir2b/file2");
   al_destroy_path(path1);
   al_destroy_path(path2);

   /* Both paths absolute. */
   path1 = al_create_path("/dir1a/dir1b/file1");
   path2 = al_create_path("/dir2a/dir2b/file2");
   al_join_paths(path1, path2);
   CHECK_EQ(al_path_cstr(path1, '/'), "/dir1a/dir1b/file1");
   al_destroy_path(path1);
   al_destroy_path(path2);
}

/* Test al_rebase_path. */
static void t11(void)
{
   A5O_PATH *path1;
   A5O_PATH *path2;

   /* Both empty. */
   path1 = al_create_path(NULL);
   path2 = al_create_path(NULL);
   al_rebase_path(path1, path2);
   CHECK_EQ(al_path_cstr(path2, '/'), "");
   al_destroy_path(path1);
   al_destroy_path(path2);

   /* Both just filenames. */
   path1 = al_create_path("file1");
   path2 = al_create_path("file2");
   al_rebase_path(path1, path2);
   CHECK_EQ(al_path_cstr(path2, '/'), "file2");
   al_destroy_path(path1);
   al_destroy_path(path2);

   /* Both relative paths. */
   path1 = al_create_path("dir1a/dir1b/file1");
   path2 = al_create_path("dir2a/dir2b/file2");
   al_rebase_path(path1, path2);
   CHECK_EQ(al_path_cstr(path2, '/'),
      "dir1a/dir1b/dir2a/dir2b/file2");
   al_destroy_path(path1);
   al_destroy_path(path2);

#ifdef A5O_WINDOWS
   /* Both relative paths with drive letters. */
   path1 = al_create_path("d:dir1a/dir1b/file1");
   path2 = al_create_path("e:dir2a/dir2b/file2");
   al_rebase_path(path1, path2);
   CHECK_EQ(al_path_cstr(path2, '/'), "d:dir1a/dir1b/dir2a/dir2b/file2");
   al_destroy_path(path1);
   al_destroy_path(path2);
#endif

   /* Path1 absolute, path2 relative. */
   path1 = al_create_path("/dir1a/dir1b/file1");
   path2 = al_create_path("dir2a/dir2b/file2");
   al_rebase_path(path1, path2);
   CHECK_EQ(al_path_cstr(path2, '/'), "/dir1a/dir1b/dir2a/dir2b/file2");
   al_destroy_path(path1);
   al_destroy_path(path2);

   /* Both paths absolute. */
   path1 = al_create_path("/dir1a/dir1b/file1");
   path2 = al_create_path("/dir2a/dir2b/file2");
   al_rebase_path(path1, path2);
   CHECK_EQ(al_path_cstr(path2, '/'), "/dir2a/dir2b/file2");
   al_destroy_path(path1);
   al_destroy_path(path2);
}

/* Test al_set_path_extension, al_get_path_extension. */
static void t12(void)
{
   A5O_PATH *path = al_create_path(NULL);

   /* Get null extension. */
   CHECK_EQ(al_get_path_extension(path), "");

   /* Set extension on null filename. */
   CHECK(! al_set_path_extension(path, "ext"));
   CHECK_EQ(al_get_path_filename(path), "");

   /* Set extension on extension-less filename. */
   al_set_path_filename(path, "abc");
   CHECK(al_set_path_extension(path, ".ext"));
   CHECK_EQ(al_get_path_filename(path), "abc.ext");

   /* Replacing extension. */
   al_set_path_filename(path, "abc.def");
   CHECK(al_set_path_extension(path, ".ext"));
   CHECK_EQ(al_get_path_filename(path), "abc.ext");
   CHECK_EQ(al_get_path_extension(path), ".ext");

   /* Filename with multiple dots. */
   al_set_path_filename(path, "abc.def.ghi");
   CHECK(al_set_path_extension(path, ".ext"));
   CHECK_EQ(al_get_path_filename(path), "abc.def.ext");
   CHECK_EQ(al_get_path_extension(path), ".ext");

   al_destroy_path(path);
}

/* Test al_get_path_basename. */
static void t13(void)
{
   A5O_PATH *path = al_create_path(NULL);

   /* No filename. */
   al_set_path_filename(path, NULL);
   CHECK_EQ(al_get_path_basename(path), "");

   /* No extension. */
   al_set_path_filename(path, "abc");
   CHECK_EQ(al_get_path_basename(path), "abc");

   /* Filename with a single dot. */
   al_set_path_filename(path, "abc.ext");
   CHECK_EQ(al_get_path_basename(path), "abc");

   /* Filename with multiple dots. */
   al_set_path_filename(path, "abc.def.ghi");
   CHECK_EQ(al_get_path_basename(path), "abc.def");

   al_destroy_path(path);
}

/* Test al_clone_path. */
static void t14(void)
{
   A5O_PATH *path1;
   A5O_PATH *path2;

   path1 = al_create_path("/abc/def/ghi");
   path2 = al_clone_path(path1);

   CHECK_EQ(al_path_cstr(path1, '/'), al_path_cstr(path2, '/'));

   al_replace_path_component(path2, 2, "DEF");
   al_set_path_filename(path2, "GHI");
   CHECK_EQ(al_path_cstr(path1, '/'), "/abc/def/ghi");
   CHECK_EQ(al_path_cstr(path2, '/'), "/abc/DEF/GHI");

   al_destroy_path(path1);
   al_destroy_path(path2);
}

static void t15(void)
{
   /* nothing */
   log_printf("Skipping empty test...\n");
}

static void t16(void)
{
   /* nothing */
   log_printf("Skipping empty test...\n");
}

/* Test al_make_path_canonical. */
static void t17(void)
{
   A5O_PATH *path;

   path = al_create_path("/../.././abc/./def/../../ghi/jkl");
   CHECK(al_make_path_canonical(path));
   CHECK(al_get_path_num_components(path) == 6);
   CHECK_EQ(al_path_cstr(path, '/'), "/abc/def/../../ghi/jkl");
   al_destroy_path(path);

   path = al_create_path("../.././abc/./def/../../ghi/jkl");
   CHECK(al_make_path_canonical(path));
   CHECK(al_get_path_num_components(path) == 7);
   CHECK_EQ(al_path_cstr(path, '/'), "../../abc/def/../../ghi/jkl");
   al_destroy_path(path);
}

/*---------------------------------------------------------------------------*/

const test_t all_tests[] =
{
   NULL, t1, t2, t3, t4, t5, t6, t7, t8, t9,
   t10, t11, t12, t13, t14, t15, t16, t17
};

#define NUM_TESTS (int)(sizeof(all_tests) / sizeof(all_tests[0]))

int main(int argc, char **argv)
{
   int i;

   if (!al_init()) {
      abort_example("Could not initialise Allegro.\n");
   }
   open_log();

   if (argc < 2) {
      for (i = 1; i < NUM_TESTS; i++) {
         log_printf("# t%d\n\n", i);
         all_tests[i]();
         log_printf("\n");
      }
   }
   else {
      i = atoi(argv[1]);
      if (i > 0 && i < NUM_TESTS) {
         all_tests[i]();
      }
   }
   log_printf("Done\n");

   close_log(true);

   if (error) {
      exit(EXIT_FAILURE);
   }

   return 0;
}

/* vim: set sts=3 sw=3 et: */
