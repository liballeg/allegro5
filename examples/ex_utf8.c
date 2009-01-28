/*
 *	Example program for the Allegro library.
 *
 *	Test UTF-8 string routines.
 */

#include <allegro5/allegro5.h>
#include <allegro5/utf8.h>
#include <stdio.h>

typedef void (*test_t)(void);

int error = 0;

#define CHECK(x)                                                            \
   do {                                                                     \
      bool ok = (x);                                                        \
      if (!ok) {                                                            \
         printf("FAIL %s\n", #x);                                           \
         error++;                                                           \
      } else {                                                              \
         printf("OK   %s\n", #x);                                           \
      }                                                                     \
   } while (0)

/* Test that we can create and destroy strings and get their data and size. */
void t1(void)
{
   ALLEGRO_USTR us1 = al_ustr_new("");
   ALLEGRO_USTR us2 = al_ustr_new("áƵ");

   CHECK(0 == strcmp(al_cstr(us1), ""));
   CHECK(0 == strcmp(al_cstr(us2), "áƵ"));
   CHECK(4 == al_ustr_size(us2));

   al_ustr_free(us1);
   al_ustr_free(us2);
}

void t2(void)
{
   CHECK(0 == al_ustr_size(al_ustr_empty_string()));
   CHECK(0 == strcmp(al_cstr(al_ustr_empty_string()), ""));
}

/* Test that we make strings which reference other C strings. */
/* No memory needs to be freed. */
void t3(void)
{
   ALLEGRO_USTR_INFO info;
   ALLEGRO_USTR us = al_ref_cstr(&info, "A static string.");

   CHECK(0 == strcmp(al_cstr(us), "A static string."));
}

/* Test that we can make strings which reference arbitrary memory blocks. */
/* No memory needs to be freed. */
void t4(void)
{
   const char *s = "This contains an embedded NUL: \0 <-- here";
   ALLEGRO_USTR_INFO info;
   ALLEGRO_USTR us = al_ref_buffer(&info, s, sizeof(s));

   CHECK(al_ustr_size(us) == sizeof(s));
   CHECK(0 == memcmp(al_cstr(us), s, sizeof(s)));
}

/* Test that we can make strings which reference (parts of) other strings. */
void t5(void)
{
   ALLEGRO_USTR us1;
   ALLEGRO_USTR us2;
   ALLEGRO_USTR_INFO us2_info;

   us1 = al_ustr_new("aábdðeéfghiíjklmnoóprstuúvxyýþæö");

   us2 = al_ref_ustr(&us2_info, us1, 36, 36 + 4);
   CHECK(0 == memcmp(al_cstr(us2), "þæ", al_ustr_size(us2)));

   /* Start pos underflow */
   us2 = al_ref_ustr(&us2_info, us1, -10, 7);
   CHECK(0 == memcmp(al_cstr(us2), "aábdð", al_ustr_size(us2)));

   /* End pos overflow */
   us2 = al_ref_ustr(&us2_info, us1, 36, INT_MAX);
   CHECK(0 == memcmp(al_cstr(us2), "þæö", al_ustr_size(us2)));

   /* Start > end */
   us2 = al_ref_ustr(&us2_info, us1, 36 + 4, 36);
   CHECK(0 == al_ustr_size(us2));

   al_ustr_free(us1);
}

/* Test al_ustr_dup. */
void t6(void)
{
   ALLEGRO_USTR us1 = al_ustr_new("aábdðeéfghiíjklmnoóprstuúvxyýþæö");
   ALLEGRO_USTR us2 = al_ustr_dup(us1);

   CHECK(al_ustr_size(us1) == al_ustr_size(us2));
   CHECK(0 == memcmp(al_cstr(us1), al_cstr(us2), al_ustr_size(us1)));

   al_ustr_free(us1);
   al_ustr_free(us2);
}

/* Test al_ustr_dup_substr. */
void t7(void)
{
   ALLEGRO_USTR us1;
   ALLEGRO_USTR us2;

   us1 = al_ustr_new("aábdðeéfghiíjklmnoóprstuúvxyýþæö");

   /* Cut out part of a string.  Check for NUL terminator. */
   us2 = al_ustr_dup_substr(us1, 36, 36 + 4);
   CHECK(al_ustr_size(us2) == 4);
   CHECK(0 == strcmp(al_cstr(us2), "þæ"));
   al_ustr_free(us2);

   /* Under and overflow */
   us2 = al_ustr_dup_substr(us1, INT_MIN, INT_MAX);
   CHECK(al_ustr_size(us2) == al_ustr_size(us1));
   CHECK(0 == strcmp(al_cstr(us2), al_cstr(us1)));
   al_ustr_free(us2);

   /* Start > end */
   us2 = al_ustr_dup_substr(us1, INT_MAX, INT_MIN);
   CHECK(0 == al_ustr_size(us2));
   al_ustr_free(us2);

   al_ustr_free(us1);
}

/* Test al_ustr_append, al_ustr_append_cstr. */
void t8(void)
{
   ALLEGRO_USTR us1 = al_ustr_new("aábdðeéfghiíjklm");
   ALLEGRO_USTR us2 = al_ustr_new("noóprstuú");

   CHECK(al_ustr_append(us1, us2));
   CHECK(0 == strcmp(al_cstr(us1), "aábdðeéfghiíjklmnoóprstuú"));

   CHECK(al_ustr_append_cstr(us1, "vxyýþæö"));
   CHECK(0 == strcmp(al_cstr(us1), "aábdðeéfghiíjklmnoóprstuúvxyýþæö"));

   al_ustr_free(us1);
   al_ustr_free(us2);
}

/* Test al_ustr_append with aliased strings. */
void t9(void)
{
   ALLEGRO_USTR us1;
   ALLEGRO_USTR_INFO us2_info;
   ALLEGRO_USTR us2;

   /* Append a string to itself. */
   us1 = al_ustr_new("aábdðeéfghiíjklm");
   CHECK(al_ustr_append(us1, us1));
   CHECK(0 == strcmp(al_cstr(us1), "aábdðeéfghiíjklmaábdðeéfghiíjklm"));
   al_ustr_free(us1);

   /* Append a substring of a string to itself. */
   us1 = al_ustr_new("aábdðeéfghiíjklm");
   us2 = al_ref_ustr(&us2_info, us1, 5, 5 + 11); /* ð...í */
   CHECK(al_ustr_append(us1, us2));
   CHECK(0 == strcmp(al_cstr(us1), "aábdðeéfghiíjklmðeéfghií"));
   al_ustr_free(us1);
}

/* Test al_ustr_equal. */
void t10(void)
{
   ALLEGRO_USTR us1;
   ALLEGRO_USTR us2;
   ALLEGRO_USTR us3;
   ALLEGRO_USTR_INFO us3_info;
   const char us3_data[] = "aábdð\0eéfgh";

   us1 = al_ustr_new("aábdð");
   us2 = al_ustr_dup(us1);
   us3 = al_ref_buffer(&us3_info, us3_data, sizeof(us3_data));

   CHECK(al_ustr_equal(us1, us2));
   CHECK(!al_ustr_equal(us1, al_ustr_empty_string()));

   /* Check comparison doesn't stop at embedded NUL. */
   CHECK(!al_ustr_equal(us1, us3));

   al_ustr_free(us1);
   al_ustr_free(us2);
}

/* Test al_ustr_insert. */
void t11(void)
{
   ALLEGRO_USTR us1;
   ALLEGRO_USTR us2;
   bool rc;
   size_t sz;

   /* Insert in middle. */
   us1 = al_ustr_new("aábdðeéfghiíjkprstuúvxyýþæö");
   us2 = al_ustr_new("lmnoó");
   al_ustr_insert(us1, 18, us2);
   CHECK(0 == strcmp(al_cstr(us1), "aábdðeéfghiíjklmnoóprstuúvxyýþæö"));

   /* Insert into itself. */
   al_ustr_insert(us2, 3, us2);
   CHECK(0 == strcmp(al_cstr(us2), "lmnlmnoóoó"));

   /* Insert before start (not allowed). */
   rc = al_ustr_insert(us2, -1, us2);
   CHECK(! al_ustr_insert(us2, -1, us2));

   /* Insert past end (will be padded with NULs). */
   sz = al_ustr_size(us2);
   al_ustr_insert(us2, sz + 3, us2);
   CHECK(al_ustr_size(us2) == sz + sz + 3);
   CHECK(0 == memcmp(al_cstr(us2), "lmnlmnoóoó\0\0\0lmnlmnoóoó", sz + sz + 3));

   al_ustr_free(us1);
   al_ustr_free(us2);
}

/* Test al_ustr_insert_cstr. */
void t12(void)
{
   ALLEGRO_USTR us1 = al_ustr_new("aábeéf");
   CHECK(al_ustr_insert_cstr(us1, 4, "dð"));
   CHECK(0 == strcmp(al_cstr(us1), "aábdðeéf"));
   al_ustr_free(us1);
}

/* Test al_ustr_remove_range. */
void t13(void)
{
   ALLEGRO_USTR us1 = al_ustr_new("aábdðeéfghiíjkprstuúvxyýþæö");

   /* Remove from middle of string. */
   CHECK(al_ustr_remove_range(us1, 5, 30));
   CHECK(0 == strcmp(al_cstr(us1), "aábdþæö"));

   /* Removing past end. */
   CHECK(al_ustr_remove_range(us1, 100, 120));
   CHECK(0 == strcmp(al_cstr(us1), "aábdþæö"));

   /* Start > End. */
   CHECK(! al_ustr_remove_range(us1, 3, 0));
   CHECK(0 == strcmp(al_cstr(us1), "aábdþæö"));

   al_ustr_free(us1);
}

/* Test al_ustr_truncate. */
void t14(void)
{
   ALLEGRO_USTR us1 = al_ustr_new("aábdðeéfghiíjkprstuúvxyýþæö");

   /* Truncate from middle of string. */
   CHECK(al_ustr_truncate(us1, 30));
   CHECK(0 == strcmp(al_cstr(us1), "aábdðeéfghiíjkprstuúvxyý"));

   /* Truncate past end (allowed). */
   CHECK(al_ustr_truncate(us1, 100));
   CHECK(0 == strcmp(al_cstr(us1), "aábdðeéfghiíjkprstuúvxyý"));

   /* Truncate before start (not allowed). */
   CHECK(! al_ustr_truncate(us1, -1));
   CHECK(0 == strcmp(al_cstr(us1), "aábdðeéfghiíjkprstuúvxyý"));

   al_ustr_free(us1);
}

/* Test whitespace trim functions. */
void t15(void)
{
   ALLEGRO_USTR us1 = al_ustr_new(" \f\n\r\t\vhello \f\n\r\t\v");
   ALLEGRO_USTR us2 = al_ustr_new(" \f\n\r\t\vhello \f\n\r\t\v");

   CHECK(al_ustr_ltrim_ws(us1));
   CHECK(0 == strcmp(al_cstr(us1), "hello \f\n\r\t\v"));

   CHECK(al_ustr_rtrim_ws(us1));
   CHECK(0 == strcmp(al_cstr(us1), "hello"));

   CHECK(al_ustr_trim_ws(us2));
   CHECK(0 == strcmp(al_cstr(us2), "hello"));

   al_ustr_free(us1);
   al_ustr_free(us2);
}

/* Test whitespace trim functions (edge cases). */
void t16(void)
{
   ALLEGRO_USTR us1;

   /* Check return value when passed empty strings. */
   CHECK(! al_ustr_ltrim_ws(al_ustr_empty_string()));
   CHECK(! al_ustr_rtrim_ws(al_ustr_empty_string()));
   CHECK(! al_ustr_trim_ws(al_ustr_empty_string()));

   /* Check nothing bad happens if the whole string is whitespace. */
   us1 = al_ustr_new(" \f\n\r\t\v");
   CHECK(al_ustr_ltrim_ws(us1));
   CHECK(al_ustr_size(us1) == 0);
   al_ustr_free(us1);

   us1 = al_ustr_new(" \f\n\r\t\v");
   CHECK(al_ustr_rtrim_ws(us1));
   CHECK(al_ustr_size(us1) == 0);
   al_ustr_free(us1);

   us1 = al_ustr_new(" \f\n\r\t\v");
   CHECK(al_ustr_trim_ws(us1));
   CHECK(al_ustr_size(us1) == 0);
   al_ustr_free(us1);
}

/*---------------------------------------------------------------------------*/

const test_t all_tests[] =
{
   NULL, t1, t2, t3, t4, t5, t6, t7, t8, t9,
   t10, t11, t12, t13, t14, t15, t16
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
