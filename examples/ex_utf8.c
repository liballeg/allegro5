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

   /* Left pos underflow */
   us2 = al_ref_ustr(&us2_info, us1, -10, 7);
   CHECK(0 == memcmp(al_cstr(us2), "aábdð", al_ustr_size(us2)));

   /* Right pos overflow */
   us2 = al_ref_ustr(&us2_info, us1, 36, INT_MAX);
   CHECK(0 == memcmp(al_cstr(us2), "þæö", al_ustr_size(us2)));

   al_ustr_free(us1);
}

/*---------------------------------------------------------------------------*/

const test_t all_tests[] =
{
   NULL, t1, t2, t3, t4, t5
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
