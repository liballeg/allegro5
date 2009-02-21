/*
 *    Example program for the Allegro library.
 *
 *    Stress test path routines.
 */

#define ALLEGRO_USE_CONSOLE
#include <allegro5/allegro5.h>
#include <stdio.h>

#ifdef ALLEGRO_MSVC
   #pragma warning (disable: 4066)
#endif

typedef void (*test_t)(void);

int error = 0;

#define CHECK(x)                                                            \
   do {                                                                     \
      bool ok = (bool)(x);                                                  \
      if (!ok) {                                                            \
         printf("FAIL %s\n", #x);                                           \
         error++;                                                           \
      } else {                                                              \
         printf("OK   %s\n", #x);                                           \
      }                                                                     \
   } while (0)

#define CHECK_EQ(x,y)   CHECK(0 == strcmp(x, y))

/*---------------------------------------------------------------------------*/

/* Test al_path_create, al_path_num_components, al_path_index,
 * al_path_get_drive, al_path_get_filename, al_path_free.
 */
void t1(void)
{
   ALLEGRO_PATH *path;

   CHECK(path = al_path_create(NULL));
   al_path_free(path);

   CHECK(path = al_path_create(""));
   CHECK(al_path_num_components(path) == 0);
   CHECK_EQ(al_path_get_drive(path), "");
   CHECK_EQ(al_path_get_filename(path), "");
   al_path_free(path);

   /* . is a directory component. */
   CHECK(path = al_path_create("."));
   CHECK(al_path_num_components(path) == 1);
   CHECK_EQ(al_path_index(path, 0), ".");
   CHECK_EQ(al_path_get_drive(path), "");
   CHECK_EQ(al_path_get_filename(path), "");
   al_path_free(path);

   /* .. is a directory component. */
   CHECK(path = al_path_create(".."));
   CHECK(al_path_num_components(path) == 1);
   CHECK_EQ(al_path_index(path, 0), "..");
   CHECK_EQ(al_path_get_drive(path), "");
   CHECK_EQ(al_path_get_filename(path), "");
   al_path_free(path);

   /* Relative path. */
   CHECK(path = al_path_create("abc/def/.."));
   CHECK(al_path_num_components(path) == 3);
   CHECK_EQ(al_path_index(path, 0), "abc");
   CHECK_EQ(al_path_index(path, 1), "def");
   CHECK_EQ(al_path_index(path, 2), "..");
   CHECK_EQ(al_path_get_drive(path), "");
   CHECK_EQ(al_path_get_filename(path), "");
   al_path_free(path);

   /* Absolute path. */
   CHECK(path = al_path_create("/abc/def/.."));
   CHECK(al_path_num_components(path) == 4);
   CHECK_EQ(al_path_index(path, 0), "");
   CHECK_EQ(al_path_index(path, 1), "abc");
   CHECK_EQ(al_path_index(path, 2), "def");
   CHECK_EQ(al_path_index(path, 3), "..");
   CHECK_EQ(al_path_get_drive(path), "");
   CHECK_EQ(al_path_get_filename(path), "");
   al_path_free(path);

   /* Directories + filename. */
   CHECK(path = al_path_create("/abc/def/ghi"));
   CHECK(al_path_num_components(path) == 3);
   CHECK_EQ(al_path_index(path, 0), "");
   CHECK_EQ(al_path_index(path, 1), "abc");
   CHECK_EQ(al_path_index(path, 2), "def");
   CHECK_EQ(al_path_get_drive(path), "");
   CHECK_EQ(al_path_get_filename(path), "ghi");
   al_path_free(path);
}

/* Test parsing UNC paths. */
void t2(void)
{
#ifdef ALLEGRO_WINDOWS
   ALLEGRO_PATH *path;

   /* The mixed slashes are deliberate. */
   /* Good paths. */
   CHECK(path = al_path_create("//server\\share name/dir/filename"));
   CHECK_EQ(al_path_get_drive(path), "//server");
   CHECK(al_path_num_components(path) == 2);
   CHECK_EQ(al_path_index(path, 0), "share name");
   CHECK_EQ(al_path_index(path, 1), "dir");
   CHECK_EQ(al_path_get_filename(path), "filename");
   al_path_free(path);

   /* Bad paths. */
   CHECK(! al_path_create("//"));
   CHECK(! al_path_create("//filename"));
   CHECK(! al_path_create("///share/name/filename"));
#endif
}

/* Test parsing drive letter paths. */
void t3(void)
{
#ifdef ALLEGRO_WINDOWS
   ALLEGRO_PATH *path;
   char buf[PATH_MAX];

   /* The mixed slashes are deliberate. */

   CHECK(path = al_path_create("c:abc\\def/ghi"));
   CHECK_EQ(al_path_get_drive(path), "c:");
   CHECK(al_path_num_components(path) == 2);
   CHECK_EQ(al_path_index(path, 0), "abc");
   CHECK_EQ(al_path_index(path, 1), "def");
   CHECK_EQ(al_path_get_filename(path), "ghi");
   CHECK_EQ(al_path_to_string(path, buf, sizeof(buf), '\\'),
      "c:abc\\def\\ghi");
   al_path_free(path);

   CHECK(path = al_path_create("c:\\abc/def\\ghi"));
   CHECK_EQ(al_path_get_drive(path), "c:");
   CHECK(al_path_num_components(path) == 3);
   CHECK_EQ(al_path_index(path, 0), "");
   CHECK_EQ(al_path_index(path, 1), "abc");
   CHECK_EQ(al_path_index(path, 2), "def");
   CHECK_EQ(al_path_get_filename(path), "ghi");
   CHECK_EQ(al_path_to_string(path, buf, sizeof(buf), '\\'),
      "c:\\abc\\def\\ghi");
   al_path_free(path);
#endif
}

/* Test al_path_append. */
void t4(void)
{
   ALLEGRO_PATH *path = al_path_create(NULL);

   CHECK(al_path_num_components(path) == 0);

   al_path_append(path, "abc");
   al_path_append(path, "def");
   al_path_append(path, "ghi");

   CHECK(al_path_num_components(path) == 3);

   CHECK_EQ(al_path_index(path, 0), "abc");
   CHECK_EQ(al_path_index(path, 1), "def");
   CHECK_EQ(al_path_index(path, 2), "ghi");

   CHECK_EQ(al_path_index(path, -1), "ghi");
   CHECK_EQ(al_path_index(path, -2), "def");
   CHECK_EQ(al_path_index(path, -3), "abc");

   al_path_free(path);
}

/* Test al_path_replace. */
void t5(void)
{
   ALLEGRO_PATH *path = al_path_create(NULL);

   al_path_append(path, "abc");
   al_path_append(path, "INKY");
   al_path_append(path, "def");
   al_path_append(path, "BLINKY");
   al_path_append(path, "ghi");

   CHECK(al_path_num_components(path) == 5);

   al_path_replace(path, 1, "PINKY");
   al_path_replace(path, -2, "CLYDE");

   CHECK(al_path_num_components(path) == 5);

   CHECK_EQ(al_path_index(path, 0), "abc");
   CHECK_EQ(al_path_index(path, 1), "PINKY");
   CHECK_EQ(al_path_index(path, 2), "def");
   CHECK_EQ(al_path_index(path, 3), "CLYDE");
   CHECK_EQ(al_path_index(path, 4), "ghi");

   al_path_free(path);
}

/* Test al_path_remove. */
void t6(void)
{
   ALLEGRO_PATH *path = al_path_create(NULL);

   al_path_append(path, "abc");
   al_path_append(path, "INKY");
   al_path_append(path, "def");
   al_path_append(path, "BLINKY");
   al_path_append(path, "ghi");

   CHECK(al_path_num_components(path) == 5);

   al_path_remove(path, 1);
   CHECK(al_path_num_components(path) == 4);

   al_path_remove(path, -2);
   CHECK(al_path_num_components(path) == 3);

   CHECK_EQ(al_path_index(path, 0), "abc");
   CHECK_EQ(al_path_index(path, 1), "def");
   CHECK_EQ(al_path_index(path, 2), "ghi");

   al_path_free(path);
}

/* Test al_path_insert. */
void t7(void)
{
   ALLEGRO_PATH *path = al_path_create("INKY/BLINKY/");

   al_path_insert(path, 0, "abc");
   al_path_insert(path, 2, "def");
   al_path_insert(path, 4, "ghi");

   CHECK(al_path_num_components(path) == 5);
   CHECK_EQ(al_path_index(path, 0), "abc");
   CHECK_EQ(al_path_index(path, 1), "INKY");
   CHECK_EQ(al_path_index(path, 2), "def");
   CHECK_EQ(al_path_index(path, 3), "BLINKY");
   CHECK_EQ(al_path_index(path, 4), "ghi");

   al_path_free(path);
}

/* Test al_path_tail, al_path_drop_tail. */
void t8(void)
{
   ALLEGRO_PATH *path = al_path_create(NULL);

   CHECK(! al_path_tail(path));

   al_path_append(path, "abc");
   al_path_append(path, "def");
   al_path_append(path, "ghi");
   CHECK_EQ(al_path_tail(path), "ghi");

   al_path_drop_tail(path);
   CHECK_EQ(al_path_tail(path), "def");

   al_path_drop_tail(path);
   al_path_drop_tail(path);
   CHECK(! al_path_tail(path));

   /* Drop tail from already empty path. */
   al_path_drop_tail(path);
   CHECK(! al_path_tail(path));

   al_path_free(path);
}

/* Test al_path_set_drive, al_path_set_filename, al_path_to_string.
 */
void t9(void)
{
   ALLEGRO_PATH *path = al_path_create(NULL);
   char buf[PATH_MAX];

   CHECK_EQ(al_path_to_string(path, buf, sizeof(buf), '/'), "");

   /* Drive letters. */
   al_path_set_drive(path, "c:");
   CHECK_EQ(al_path_to_string(path, buf, sizeof(buf), '/'), "c:");
   CHECK_EQ(al_path_get_drive(path), "c:");

   al_path_set_drive(path, "d:");
   CHECK_EQ(al_path_to_string(path, buf, sizeof(buf), '/'), "d:");

   /* Plus directory components. */
   al_path_append(path, "abc");
   al_path_append(path, "def");
   CHECK_EQ(al_path_to_string(path, buf, sizeof(buf), '/'), "d:abc/def/");

   /* Plus filename. */
   al_path_set_filename(path, "uvw");
   CHECK_EQ(al_path_to_string(path, buf, sizeof(buf), '/'), "d:abc/def/uvw");
   CHECK_EQ(al_path_get_filename(path), "uvw");

   /* Replace filename. */
   al_path_set_filename(path, "xyz");
   CHECK_EQ(al_path_to_string(path, buf, sizeof(buf), '/'), "d:abc/def/xyz");

   /* Remove drive. */
   al_path_set_drive(path, NULL);
   CHECK_EQ(al_path_to_string(path, buf, sizeof(buf), '/'), "abc/def/xyz");

   /* Remove filename. */
   al_path_set_filename(path, NULL);
   CHECK_EQ(al_path_to_string(path, buf, sizeof(buf), '/'), "abc/def/");

   /* Buffer too short. */
   CHECK(! al_path_to_string(path, buf, 0, '/'));
   CHECK(! al_path_to_string(path, buf, 2, '/'));
   CHECK(! al_path_to_string(path, buf, 8, '/'));
   CHECK_EQ(al_path_to_string(path, buf, 9, '/'), "abc/def/");

   al_path_free(path);
}

/* Test al_path_concat. */
void t10(void)
{
   ALLEGRO_PATH *path1;
   ALLEGRO_PATH *path2;
   char buf[PATH_MAX];

   /* Both empty. */
   path1 = al_path_create(NULL);
   path2 = al_path_create(NULL);
   al_path_concat(path1, path2);
   CHECK_EQ(al_path_to_string(path1, buf, sizeof(buf), '/'), "");
   al_path_free(path1);
   al_path_free(path2);

   /* Both just filenames. */
   path1 = al_path_create("file1");
   path2 = al_path_create("file2");
   al_path_concat(path1, path2);
   CHECK_EQ(al_path_to_string(path1, buf, sizeof(buf), '/'), "file2");
   al_path_free(path1);
   al_path_free(path2);

   /* Both relative paths. */
   path1 = al_path_create("dir1a/dir1b/file1");
   path2 = al_path_create("dir2a/dir2b/file2");
   al_path_concat(path1, path2);
   CHECK_EQ(al_path_to_string(path1, buf, sizeof(buf), '/'),
      "dir1a/dir1b/dir2a/dir2b/file2");
   al_path_free(path1);
   al_path_free(path2);

#ifdef ALLEGRO_WINDOWS
   /* Both relative paths with drive letters. */
   path1 = al_path_create("d:dir1a/dir1b/file1");
   path2 = al_path_create("e:dir2a/dir2b/file2");
   al_path_concat(path1, path2);
   CHECK_EQ(al_path_to_string(path1, buf, sizeof(buf), '/'),
      "d:dir1a/dir1b/dir2a/dir2b/file2");
   al_path_free(path1);
   al_path_free(path2);
#endif

   /* Path1 absolute, path2 relative. */
   path1 = al_path_create("/dir1a/dir1b/file1");
   path2 = al_path_create("dir2a/dir2b/file2");
   al_path_concat(path1, path2);
   CHECK_EQ(al_path_to_string(path1, buf, sizeof(buf), '/'),
      "/dir1a/dir1b/dir2a/dir2b/file2");
   al_path_free(path1);
   al_path_free(path2);

   /* Both paths absolute. */
   path1 = al_path_create("/dir1a/dir1b/file1");
   path2 = al_path_create("/dir2a/dir2b/file2");
   al_path_concat(path1, path2);
   CHECK_EQ(al_path_to_string(path1, buf, sizeof(buf), '/'),
      "/dir1a/dir1b/file1");
   al_path_free(path1);
   al_path_free(path2);
}

/* Test al_path_set_extension, al_path_get_extension. */
void t11(void)
{
   ALLEGRO_PATH *path = al_path_create(NULL);
   char buf[PATH_MAX];

   /* Get null extension. */
   CHECK_EQ(al_path_get_extension(path, buf, sizeof(buf)), "");

   /* Set extension on null filename. */
   CHECK(! al_path_set_extension(path, "ext"));
   CHECK_EQ(al_path_get_filename(path), "");

   /* Set extension on extension-less filename. */
   al_path_set_filename(path, "abc");
   CHECK(al_path_set_extension(path, "ext"));
   CHECK_EQ(al_path_get_filename(path), "abc.ext");

   /* Replacing extension. */
   al_path_set_filename(path, "abc.def");
   CHECK(al_path_set_extension(path, "ext"));
   CHECK_EQ(al_path_get_filename(path), "abc.ext");
   CHECK_EQ(al_path_get_extension(path, buf, sizeof(buf)), "ext");

   /* Too short buffer. */
   CHECK_EQ(al_path_get_extension(path, buf, 2), "e");

   al_path_free(path);
}

/* Test al_path_get_basename. */
void t12(void)
{
   ALLEGRO_PATH *path = al_path_create(NULL);
   char buf[PATH_MAX];

   al_path_set_filename(path, "abc");

   /* Get filename without extension. */
   CHECK_EQ(al_path_get_basename(path, buf, sizeof(buf)), "abc");

   /* Get filename without extension when no extension exists. */
   CHECK(al_path_set_extension(path, NULL));
   CHECK_EQ(al_path_get_filename(path), "abc.");
   CHECK_EQ(al_path_get_basename(path, buf, sizeof(buf)), "abc");

   al_path_free(path);
}

/* Test al_path_clone. */
void t13(void)
{
   ALLEGRO_PATH *path1;
   ALLEGRO_PATH *path2;
   char buf1[PATH_MAX];
   char buf2[PATH_MAX];

   path1 = al_path_create("/abc/def/ghi");
   path2 = al_path_clone(path1);

   CHECK_EQ(
      al_path_to_string(path1, buf1, sizeof(buf1), '/'),
      al_path_to_string(path2, buf2, sizeof(buf2), '/'));

   al_path_replace(path2, 2, "DEF");
   al_path_set_filename(path2, "GHI");
   CHECK_EQ(al_path_to_string(path1, buf1, sizeof(buf1), '/'), "/abc/def/ghi");
   CHECK_EQ(al_path_to_string(path2, buf2, sizeof(buf2), '/'), "/abc/DEF/GHI");

   al_path_free(path1);
   al_path_free(path2);
}

/* Test al_path_exists. */
void t14(void)
{
   ALLEGRO_PATH *path;

   path = al_path_create("./data");
   CHECK(al_path_exists(path));

   al_path_set_extension(path, ".phony");
   CHECK(! al_path_exists(path));

   al_path_free(path);
}

/* Test al_path_emode. */
void t15(void)
{
   ALLEGRO_PATH *path;

   path = al_path_create("data");
   CHECK(al_path_emode(path, AL_FM_READ | AL_FM_EXECUTE | AL_FM_ISDIR));
   al_path_free(path);

   path = al_path_create("data/allegro.pcx");
   CHECK(al_path_emode(path, AL_FM_READ | AL_FM_ISFILE));
   CHECK(! al_path_emode(path, AL_FM_EXECUTE));
   al_path_free(path);
}

/* Test al_path_make_absolute. */
void t16(void)
{
   ALLEGRO_PATH *path;
   char cwd[PATH_MAX];
   char buf[PATH_MAX];

   path = al_path_create("abc/def");
   CHECK(al_path_make_absolute(path));

   al_getcwd(PATH_MAX, cwd);
   al_path_to_string(path, buf, sizeof(buf), ALLEGRO_NATIVE_PATH_SEP);
   CHECK(0 == strncmp(buf, cwd, strlen(cwd)));
   CHECK(0 == strcmp(buf + strlen(cwd), "abc/def") ||
         0 == strcmp(buf + strlen(cwd), "abc\\def"));

   al_path_free(path);
}

/* Test al_path_make_canonical. */
void t17(void)
{
   ALLEGRO_PATH *path;
   char buf[PATH_MAX];

   path = al_path_create("/../.././abc/./def/../../ghi/jkl");
   CHECK(al_path_make_canonical(path));
   CHECK(al_path_num_components(path) == 6);
   CHECK_EQ(al_path_to_string(path, buf, sizeof(buf), '/'),
      "/abc/def/../../ghi/jkl");
   al_path_free(path);

   path = al_path_create("../.././abc/./def/../../ghi/jkl");
   CHECK(al_path_make_canonical(path));
   CHECK(al_path_num_components(path) == 7);
   CHECK_EQ(al_path_to_string(path, buf, sizeof(buf), '/'),
      "../../abc/def/../../ghi/jkl");
   al_path_free(path);
}

/*---------------------------------------------------------------------------*/

const test_t all_tests[] =
{
   NULL, t1, t2, t3, t4, t5, t6, t7, t8, t9,
   t10, t11, t12, t13, t14, t15, t16, t17
};

#define NUM_TESTS (int)(sizeof(all_tests) / sizeof(all_tests[0]))

int main(int argc, const char *argv[])
{
   int i;

   if (argc < 2) {
      for (i = 1; i < NUM_TESTS; i++) {
         printf("# t%d\n\n", i);
         all_tests[i]();
         printf("\n");
      }
   }
   else {
      i = atoi(argv[1]);
      if (i > 0 && i < NUM_TESTS) {
         all_tests[i]();
      }
   }

   if (error) {
      exit(EXIT_FAILURE);
   }

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
