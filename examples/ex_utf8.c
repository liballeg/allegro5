/*
 *	Example program for the Allegro library.
 *
 *	Test UTF-8 string routines.
 */

/* TODO: we should also be checking on inputs with surrogate characters
 * (which are not allowed)
 */

#define ALLEGRO_USE_CONSOLE
#include <allegro5/allegro5.h>
#include <allegro5/utf8.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef ALLEGRO_MSVC
   #pragma warning (disable: 4066)
#endif

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
   ALLEGRO_USTR *us1 = al_ustr_new("");
   ALLEGRO_USTR *us2 = al_ustr_new("áƵ");

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
   ALLEGRO_USTR *us = al_ref_cstr(&info, "A static string.");

   CHECK(0 == strcmp(al_cstr(us), "A static string."));
}

/* Test that we can make strings which reference arbitrary memory blocks. */
/* No memory needs to be freed. */
void t4(void)
{
   const char *s = "This contains an embedded NUL: \0 <-- here";
   ALLEGRO_USTR_INFO info;
   ALLEGRO_USTR *us = al_ref_buffer(&info, s, sizeof(s));

   CHECK(al_ustr_size(us) == sizeof(s));
   CHECK(0 == memcmp(al_cstr(us), s, sizeof(s)));
}

/* Test that we can make strings which reference (parts of) other strings. */
void t5(void)
{
   ALLEGRO_USTR *us1;
   ALLEGRO_USTR *us2;
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
   ALLEGRO_USTR *us1 = al_ustr_new("aábdðeéfghiíjklmnoóprstuúvxyýþæö");
   ALLEGRO_USTR *us2 = al_ustr_dup(us1);

   CHECK(al_ustr_size(us1) == al_ustr_size(us2));
   CHECK(0 == memcmp(al_cstr(us1), al_cstr(us2), al_ustr_size(us1)));

   al_ustr_free(us1);
   al_ustr_free(us2);
}

/* Test al_ustr_dup_substr. */
void t7(void)
{
   ALLEGRO_USTR *us1;
   ALLEGRO_USTR *us2;

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
   ALLEGRO_USTR *us1 = al_ustr_new("aábdðeéfghiíjklm");
   ALLEGRO_USTR *us2 = al_ustr_new("noóprstuú");

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
   ALLEGRO_USTR *us1;
   ALLEGRO_USTR_INFO us2_info;
   ALLEGRO_USTR *us2;

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
   ALLEGRO_USTR *us1;
   ALLEGRO_USTR *us2;
   ALLEGRO_USTR *us3;
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
   ALLEGRO_USTR *us1;
   ALLEGRO_USTR *us2;
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
   ALLEGRO_USTR *us1 = al_ustr_new("aábeéf");
   CHECK(al_ustr_insert_cstr(us1, 4, "dð"));
   CHECK(0 == strcmp(al_cstr(us1), "aábdðeéf"));
   al_ustr_free(us1);
}

/* Test al_ustr_remove_range. */
void t13(void)
{
   ALLEGRO_USTR *us1 = al_ustr_new("aábdðeéfghiíjkprstuúvxyýþæö");

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
   ALLEGRO_USTR *us1 = al_ustr_new("aábdðeéfghiíjkprstuúvxyýþæö");

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
   ALLEGRO_USTR *us1 = al_ustr_new(" \f\n\r\t\vhello \f\n\r\t\v");
   ALLEGRO_USTR *us2 = al_ustr_new(" \f\n\r\t\vhello \f\n\r\t\v");

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
   ALLEGRO_USTR *us1;

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

/* Test al_utf8_width. */
void t17(void)
{
   CHECK(al_utf8_width(0x000000) == 1);
   CHECK(al_utf8_width(0x00007f) == 1);
   CHECK(al_utf8_width(0x000080) == 2);
   CHECK(al_utf8_width(0x0007ff) == 2);
   CHECK(al_utf8_width(0x000800) == 3);
   CHECK(al_utf8_width(0x00ffff) == 3);
   CHECK(al_utf8_width(0x010000) == 4);
   CHECK(al_utf8_width(0x10ffff) == 4);

   /* These are illegal. */
   CHECK(al_utf8_width(0x110000) == 0);
   CHECK(al_utf8_width(0xffffff) == 0);
}

/* Test al_utf8_encode. */
void t18(void)
{
   char buf[4];

   CHECK(al_utf8_encode(buf, 0) == 1);
   CHECK(0 == memcmp(buf, "\x00", 1));

   CHECK(al_utf8_encode(buf, 0x7f) == 1);
   CHECK(0 == memcmp(buf, "\x7f", 1));

   CHECK(al_utf8_encode(buf, 0x80) == 2);
   CHECK(0 == memcmp(buf, "\xC2\x80", 2));

   CHECK(al_utf8_encode(buf, 0x7ff) == 2);
   CHECK(0 == memcmp(buf, "\xDF\xBF", 2));

   CHECK(al_utf8_encode(buf, 0x000800) == 3);
   CHECK(0 == memcmp(buf, "\xE0\xA0\x80", 3));

   CHECK(al_utf8_encode(buf, 0x00ffff) == 3);
   CHECK(0 == memcmp(buf, "\xEF\xBF\xBF", 3));

   CHECK(al_utf8_encode(buf, 0x010000) == 4);
   CHECK(0 == memcmp(buf, "\xF0\x90\x80\x80", 4));

   CHECK(al_utf8_encode(buf, 0x10ffff) == 4);
   CHECK(0 == memcmp(buf, "\xF4\x8f\xBF\xBF", 4));

   /* These are illegal. */
   CHECK(al_utf8_encode(buf, 0x110000) == 0);
   CHECK(al_utf8_encode(buf, 0xffffff) == 0);
}

/* Test al_ustr_insert_chr. */
void t19(void)
{
   ALLEGRO_USTR *us = al_ustr_new("");

   CHECK(al_ustr_insert_chr(us, 0, 'a') == 1);
   CHECK(al_ustr_insert_chr(us, 0, L'æ') == 2);
   CHECK(al_ustr_insert_chr(us, 2, L'€') == 3);
   CHECK(0 == strcmp(al_cstr(us), "æ€a"));

   /* Past end. */
   CHECK(al_ustr_insert_chr(us, 8, L'ö') == 2);
   CHECK(0 == memcmp(al_cstr(us), "æ€a\0\0ö", 9));

   /* Invalid code points. */
   CHECK(al_ustr_insert_chr(us, 0, -1) == 0);
   CHECK(al_ustr_insert_chr(us, 0, 0x110000) == 0);

   al_ustr_free(us);
}

/* Test al_ustr_append_chr. */
void t20(void)
{
   ALLEGRO_USTR *us = al_ustr_new("");

   CHECK(al_ustr_append_chr(us, 'a') == 1);
   CHECK(al_ustr_append_chr(us, L'æ') == 2);
   CHECK(al_ustr_append_chr(us, L'€') == 3);
   CHECK(0 == strcmp(al_cstr(us), "aæ€"));

   /* Invalid code points. */
   CHECK(al_ustr_append_chr(us, -1) == 0);
   CHECK(al_ustr_append_chr(us, 0x110000) == 0);

   al_ustr_free(us);
}

/* Test al_ustr_get. */
void t21(void)
{
   ALLEGRO_USTR_INFO info;
   ALLEGRO_USTR *us;

   us = al_ref_buffer(&info, "", 1);
   CHECK(al_ustr_get(us, 0) == 0);

   us = al_ref_cstr(&info, "\x7f");
   CHECK(al_ustr_get(us, 0) == 0x7f);

   us = al_ref_cstr(&info, "\xC2\x80");
   CHECK(al_ustr_get(us, 0) == 0x80);

   us = al_ref_cstr(&info, "\xDF\xBf");
   CHECK(al_ustr_get(us, 0) == 0x7ff);

   us = al_ref_cstr(&info, "\xE0\xA0\x80");
   CHECK(al_ustr_get(us, 0) == 0x800);

   us = al_ref_cstr(&info, "\xEF\xBF\xBF");
   CHECK(al_ustr_get(us, 0) == 0xffff);

   us = al_ref_cstr(&info, "\xF0\x90\x80\x80");
   CHECK(al_ustr_get(us, 0) == 0x010000);

   us = al_ref_cstr(&info, "\xF4\x8F\xBF\xBF");
   CHECK(al_ustr_get(us, 0) == 0x10ffff);
}

/* Test al_ustr_get on invalid sequences. */
void t22(void)
{
   ALLEGRO_USTR_INFO info;
   ALLEGRO_USTR *us;

   /* Empty string. */
   al_set_errno(0);
   CHECK(al_ustr_get(al_ustr_empty_string(), 0) < 0);
   CHECK(al_get_errno() == ERANGE);

   /* 5-byte sequence. */
   us = al_ref_cstr(&info, "\xf8\x88\x80\x80\x80");
   al_set_errno(0);
   CHECK(al_ustr_get(us, 0) < 0);
   CHECK(al_get_errno() == EILSEQ);

   /* Start in trail byte. */
   us = al_ref_cstr(&info, "ð");
   al_set_errno(0);
   CHECK(al_ustr_get(us, 1) < 0);
   CHECK(al_get_errno() == EILSEQ);

   /* Truncated 3-byte sequence. */
   us = al_ref_cstr(&info, "\xEF\xBF");
   al_set_errno(0);
   CHECK(al_ustr_get(us, 0) < 0);
   CHECK(al_get_errno() == EILSEQ);
}

/* Test al_ustr_get on invalid sequences (part 2). */
/* Get more ideas for tests from
 * http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt
 */
void t23(void)
{
   ALLEGRO_USTR_INFO info;
   ALLEGRO_USTR *us;

   /* Examples of an overlong ASCII character */
   us = al_ref_cstr(&info, "\xc0\xaf");
   CHECK(al_ustr_get(us, 0) < 0);
   us = al_ref_cstr(&info, "\xe0\x80\xaf");
   CHECK(al_ustr_get(us, 0) < 0);
   us = al_ref_cstr(&info, "\xf0\x80\x80\xaf");
   CHECK(al_ustr_get(us, 0) < 0);
   us = al_ref_cstr(&info, "\xf8\x80\x80\x80\xaf");
   CHECK(al_ustr_get(us, 0) < 0);
   us = al_ref_cstr(&info, "\xfc\x80\x80\x80\x80\xaf");
   CHECK(al_ustr_get(us, 0) < 0);

   /* Maximum overlong sequences */
   us = al_ref_cstr(&info, "\xc1\xbf");
   CHECK(al_ustr_get(us, 0) < 0);

   us = al_ref_cstr(&info, "\xe0\x9f\xbf");
   CHECK(al_ustr_get(us, 0) < 0);

   us = al_ref_cstr(&info, "\xf0\x8f\xbf\xbf");
   CHECK(al_ustr_get(us, 0) < 0);

   us = al_ref_cstr(&info, "\xf8\x87\xbf\xbf\xbf");
   CHECK(al_ustr_get(us, 0) < 0);

   us = al_ref_cstr(&info, "\xfc\x83\xbf\xbf\xbf\xbf");
   CHECK(al_ustr_get(us, 0) < 0);

   /* Overlong representation of the NUL character */
   us = al_ref_cstr(&info, "\xc0\x80");
   CHECK(al_ustr_get(us, 0) < 0);

   us = al_ref_cstr(&info, "\xe0\x80\x80");
   CHECK(al_ustr_get(us, 0) < 0);

   us = al_ref_cstr(&info, "\xf0\x80\x80");
   CHECK(al_ustr_get(us, 0) < 0);

   us = al_ref_cstr(&info, "\xf8\x80\x80\x80");
   CHECK(al_ustr_get(us, 0) < 0);

   us = al_ref_cstr(&info, "\xfc\x80\x80\x80\x80");
   CHECK(al_ustr_get(us, 0) < 0);
}

/* Test al_ustr_next. */
void t24(void)
{
   const char str[] = "a\0þ€\xf4\x8f\xbf\xbf";
   ALLEGRO_USTR_INFO info;
   ALLEGRO_USTR *us = al_ref_buffer(&info, str, sizeof(str) - 1);
   int pos = 0;

   CHECK(al_ustr_next(us, &pos));   /* a */
   CHECK(pos == 1);

   CHECK(al_ustr_next(us, &pos));   /* NUL */
   CHECK(pos == 2);

   CHECK(al_ustr_next(us, &pos));   /* þ */
   CHECK(pos == 4);

   CHECK(al_ustr_next(us, &pos));   /* € */
   CHECK(pos == 7);

   CHECK(al_ustr_next(us, &pos));   /* U+10FFFF */
   CHECK(pos == 11);

   CHECK(! al_ustr_next(us, &pos)); /* end */
   CHECK(pos == 11);
}

/* Test al_ustr_next with invalid input. */
void t25(void)
{
   const char str[] = "þ\xf4\x8f\xbf.";
   ALLEGRO_USTR_INFO info;
   ALLEGRO_USTR *us = al_ref_buffer(&info, str, sizeof(str) - 1);
   int pos;

   /* Starting in middle of a sequence. */
   pos = 1;
   CHECK(al_ustr_next(us, &pos));
   CHECK(pos == 2);

   /* Unexpected end of 4-byte sequence. */
   CHECK(al_ustr_next(us, &pos));
   CHECK(pos == 5);
}

/* Test al_ustr_prev. */
void t26(void)
{
   ALLEGRO_USTR *us = al_ustr_new("aþ€\xf4\x8f\xbf\xbf");
   int pos = al_ustr_size(us);

   CHECK(al_ustr_prev(us, &pos));   /* U+10FFFF */
   CHECK(pos == 6);

   CHECK(al_ustr_prev(us, &pos));   /* € */
   CHECK(pos == 3);

   CHECK(al_ustr_prev(us, &pos));   /* þ */
   CHECK(pos == 1);

   CHECK(al_ustr_prev(us, &pos));   /* a */
   CHECK(pos == 0);

   CHECK(!al_ustr_prev(us, &pos));  /* begin */
   CHECK(pos == 0);

   al_ustr_free(us);
}

/* Test al_ustr_length. */
void t27(void)
{
   ALLEGRO_USTR *us = al_ustr_new("aþ€\xf4\x8f\xbf\xbf");

   CHECK(0 == al_ustr_length(al_ustr_empty_string()));
   CHECK(4 == al_ustr_length(us));

   al_ustr_free(us);
}

/* Test al_ustr_offset. */
void t28(void)
{
   ALLEGRO_USTR *us = al_ustr_new("aþ€\xf4\x8f\xbf\xbf");

   CHECK(al_ustr_offset(us, 0) == 0);
   CHECK(al_ustr_offset(us, 1) == 1);
   CHECK(al_ustr_offset(us, 2) == 3);
   CHECK(al_ustr_offset(us, 3) == 6);
   CHECK(al_ustr_offset(us, 4) == 10);
   CHECK(al_ustr_offset(us, 5) == 10);

   CHECK(al_ustr_offset(us, -1) == 6);
   CHECK(al_ustr_offset(us, -2) == 3);
   CHECK(al_ustr_offset(us, -3) == 1);
   CHECK(al_ustr_offset(us, -4) == 0);
   CHECK(al_ustr_offset(us, -5) == 0);

   al_ustr_free(us);
}

/* Test al_ustr_get_next. */
void t29(void)
{
   ALLEGRO_USTR *us = al_ustr_new("aþ€");
   int pos;

   pos = 0;
   CHECK(al_ustr_get_next(us, &pos) == 'a');
   CHECK(al_ustr_get_next(us, &pos) == L'þ');
   CHECK(al_ustr_get_next(us, &pos) == L'€');
   CHECK(al_ustr_get_next(us, &pos) == -1);
   CHECK(pos == (int) al_ustr_size(us));

   /* Start in the middle of þ. */
   pos = 2;
   CHECK(al_ustr_get_next(us, &pos) == -2);
   CHECK(al_ustr_get_next(us, &pos) == L'€');

   al_ustr_free(us);
}

/* Test al_ustr_prev_get. */
void t30(void)
{
   ALLEGRO_USTR *us = al_ustr_new("aþ€");
   int pos;

   pos = al_ustr_size(us);
   CHECK(al_ustr_prev_get(us, &pos) == L'€');
   CHECK(al_ustr_prev_get(us, &pos) == L'þ');
   CHECK(al_ustr_prev_get(us, &pos) == 'a');
   CHECK(al_ustr_prev_get(us, &pos) == -1);

   /* Start in the middle of þ. */
   pos = 2;
   CHECK(al_ustr_prev_get(us, &pos) == L'þ');

   al_ustr_free(us);
}

/* Test al_ustr_find_chr. */
void t31(void)
{
   ALLEGRO_USTR *us = al_ustr_new("aábdðeéfghiíaábdðeéfghií");

   /* Find ASCII. */
   CHECK(al_ustr_find_chr(us, 0, 'e') == 7);
   CHECK(al_ustr_find_chr(us, 7, 'e') == 7);    /* start_pos is inclusive */
   CHECK(al_ustr_find_chr(us, 8, 'e') == 23);
   CHECK(al_ustr_find_chr(us, 0, '.') == -1);

   /* Find non-ASCII. */
   CHECK(al_ustr_find_chr(us, 0, L'ð') == 5);
   CHECK(al_ustr_find_chr(us, 5, L'ð') == 5);   /* start_pos is inclusive */
   CHECK(al_ustr_find_chr(us, 6, L'ð') == 21);
   CHECK(al_ustr_find_chr(us, 0, L'ƶ') == -1);

   al_ustr_free(us);
}

/* Test al_ustr_rfind_chr. */
void t32(void)
{
   ALLEGRO_USTR *us = al_ustr_new("aábdðeéfghiíaábdðeéfghií");
   int end = al_ustr_size(us);

   /* Find ASCII. */
   CHECK(al_ustr_rfind_chr(us, end, 'e') == 23);
   CHECK(al_ustr_rfind_chr(us, 23, 'e') == 7);        /* end_pos exclusive */
   CHECK(al_ustr_rfind_chr(us, end, '.') == -1);

   /* Find non-ASCII. */
   CHECK(al_ustr_rfind_chr(us, end, L'í') == 30);
   CHECK(al_ustr_rfind_chr(us, end - 1, L'í') == 14); /* end_pos exclusive */
   CHECK(al_ustr_rfind_chr(us, end, L'ƶ') == -1);

   al_ustr_free(us);
}

/* Test al_ustr_find_set, al_ustr_find_set_cstr. */
void t33(void)
{
   ALLEGRO_USTR *us = al_ustr_new("aábdðeéfghiíaábdðeéfghií");

   /* al_ustr_find_set_cstr is s simple wrapper for al_ustr_find_set
    * so we test using that.
    */

   /* Find ASCII. */
   CHECK(al_ustr_find_set_cstr(us, 0, "gfe") == 7);
   CHECK(al_ustr_find_set_cstr(us, 7, "gfe") == 7);  /* start_pos inclusive */
   CHECK(al_ustr_find_set_cstr(us, 0, "") == -1);
   CHECK(al_ustr_find_set_cstr(us, 0, "xyz") == -1);

   /* Find non-ASCII. */
   CHECK(al_ustr_find_set_cstr(us, 0, "éðf") == 5);
   CHECK(al_ustr_find_set_cstr(us, 5, "éðf") == 5);   /* start_pos inclusive */
   CHECK(al_ustr_find_set_cstr(us, 0, "ẋỹƶ") == -1);

   al_ustr_free(us);
}

/* Test al_ustr_find_set, al_ustr_find_set_cstr (invalid values).  */
void t34(void)
{
   ALLEGRO_USTR *us = al_ustr_new("a\x80ábdðeéfghií");

   /* Invalid byte sequence in search string. */
   CHECK(al_ustr_find_set_cstr(us, 0, "gfe") == 8);

   /* Invalid byte sequence in accept set. */
   CHECK(al_ustr_find_set_cstr(us, 0, "é\x80ðf") == 6);

   al_ustr_free(us);
}

/* Test al_ustr_find_cset, al_ustr_find_cset_cstr. */
void t35(void)
{
   ALLEGRO_USTR *us;

   /* al_ustr_find_cset_cstr is s simple wrapper for al_ustr_find_cset
    * so we test using that.
    */

   /* Find ASCII. */
   us = al_ustr_new("alphabetagamma");
   CHECK(al_ustr_find_cset_cstr(us, 0, "alphbet") == 9);
   CHECK(al_ustr_find_cset_cstr(us, 9, "alphbet") == 9);
   CHECK(al_ustr_find_cset_cstr(us, 0, "") == -1);
   CHECK(al_ustr_find_cset_cstr(us, 0, "alphbetgm") == -1);
   al_ustr_free(us);

   /* Find non-ASCII. */
   us = al_ustr_new("αλφαβεταγαμμα");
   CHECK(al_ustr_find_cset_cstr(us, 0, "αλφβετ") == 16);
   CHECK(al_ustr_find_cset_cstr(us, 16, "αλφβετ") == 16);
   CHECK(al_ustr_find_cset_cstr(us, 0, "αλφβετγμ") == -1);
   al_ustr_free(us);
}

/* Test al_ustr_find_cset, al_ustr_find_set_cstr (invalid values).  */
void t36(void)
{
   ALLEGRO_USTR *us = al_ustr_new("a\x80ábdðeéfghií");

   /* Invalid byte sequence in search string. */
   CHECK(al_ustr_find_cset_cstr(us, 0, "aábd") == 6);

   /* Invalid byte sequence in reject set. */
   CHECK(al_ustr_find_cset_cstr(us, 0, "a\x80ábd") == 6);

   al_ustr_free(us);
}

/* Test al_ustr_find_str, al_ustr_find_cstr. */
void t37(void)
{
   ALLEGRO_USTR *us = al_ustr_new("aábdðeéfghiíaábdðeéfghií");

   /* al_ustr_find_cstr is s simple wrapper for al_ustr_find_str
    * so we test using that.
    */

   CHECK(al_ustr_find_cstr(us, 0, "") == 0);
   CHECK(al_ustr_find_cstr(us, 10, "") == 10);
   CHECK(al_ustr_find_cstr(us, 0, "ábd") == 1);
   CHECK(al_ustr_find_cstr(us, 10, "ábd") == 17);
   CHECK(al_ustr_find_cstr(us, 0, "ábz") == -1);

   al_ustr_free(us);
}

/* Test al_ustr_rfind_str, al_ustr_rfind_cstr. */
void t38(void)
{
   ALLEGRO_USTR *us = al_ustr_new("aábdðeéfghiíaábdðeéfghií");
   int end = al_ustr_size(us);

   /* al_ustr_find_cstr is s simple wrapper for al_ustr_find_str
    * so we test using that.
    */

   CHECK(al_ustr_rfind_cstr(us, 0, "") == 0);
   CHECK(al_ustr_rfind_cstr(us, 1, "") == 1);
   CHECK(al_ustr_rfind_cstr(us, end, "hií") == end - 4);
   CHECK(al_ustr_rfind_cstr(us, end - 1, "hií") == 12);
   CHECK(al_ustr_rfind_cstr(us, end, "ábz") == -1);

   al_ustr_free(us);
}

/* Test al_ustr_new_from_buffer, al_cstr_dup. */
void t39(void)
{
   const char s1[] = "Корабът ми на въздушна възглавница\0е пълен със змиорки";
   ALLEGRO_USTR *us;
   char *s2;

   us = al_ustr_new_from_buffer(s1, sizeof(s1) - 1); /* missing NUL term. */
   s2 = al_cstr_dup(us);
   al_ustr_free(us);

   CHECK(0 == strcmp(s1, s2));
   CHECK(0 == memcmp(s1, s2, sizeof(s1))); /* including NUL terminator */

   free(s2); /* should be al_free eventually */
}

/* Test al_ustr_assign, al_ustr_assign_cstr. */
void t40(void)
{
   ALLEGRO_USTR *us1 = al_ustr_new("我隻氣墊船裝滿晒鱔");
   ALLEGRO_USTR *us2 = al_ustr_new("Τὸ χόβερκράφτ μου εἶναι γεμᾶτο χέλια");

   CHECK(al_ustr_assign(us1, us2));
   CHECK(0 == strcmp(al_cstr(us1), "Τὸ χόβερκράφτ μου εἶναι γεμᾶτο χέλια"));

   CHECK(al_ustr_assign_cstr(us1, "私のホバークラフトは鰻でいっぱいです"));
   CHECK(54 == al_ustr_size(us1));

   al_ustr_free(us1);
   al_ustr_free(us2);
}

/* Test al_ustr_assign_cstr. */
void t41(void)
{
   ALLEGRO_USTR *us1 = al_ustr_new("Моја лебдилица је пуна јегуља");
   ALLEGRO_USTR *us2 = al_ustr_new("");

   CHECK(al_ustr_assign_substr(us2, us1, 9, 27));
   CHECK(0 == strcmp(al_cstr(us2), "лебдилица"));

   /* Start > End */
   CHECK(al_ustr_assign_substr(us2, us1, 9, 0));
   CHECK(0 == strcmp(al_cstr(us2), ""));

   /* Start, end out of bounds */
   CHECK(al_ustr_assign_substr(us2, us1, -INT_MAX, INT_MAX));
   CHECK(0 == strcmp(al_cstr(us2), "Моја лебдилица је пуна јегуља"));

   al_ustr_free(us1);
   al_ustr_free(us2);
}

/* Test al_ustr_set_chr. */
void t42(void)
{
   ALLEGRO_USTR *us = al_ustr_new("abcdef");

   /* Same size (ASCII). */
   CHECK(al_ustr_set_chr(us, 1, 'B') == 1);
   CHECK(0 == strcmp(al_cstr(us), "aBcdef"));
   CHECK(6 == al_ustr_size(us));

   /* Enlarge to 2-bytes. */
   CHECK(al_ustr_set_chr(us, 1, L'β') == 2);
   CHECK(0 == strcmp(al_cstr(us), "aβcdef"));
   CHECK(7 == al_ustr_size(us));

   /* Enlarge to 3-bytes. */
   CHECK(al_ustr_set_chr(us, 5, L'ᴈ') == 3);
   CHECK(0 == strcmp(al_cstr(us), "aβcdᴈf"));
   CHECK(9 == al_ustr_size(us));

   /* Reduce to 2-bytes. */
   CHECK(al_ustr_set_chr(us, 5, L'ə') == 2);
   CHECK(0 == strcmp(al_cstr(us), "aβcdəf"));
   CHECK(8 == al_ustr_size(us));

   /* Set at end of string. */
   CHECK(al_ustr_set_chr(us, al_ustr_size(us), L'ῷ') == 3);
   CHECK(0 == strcmp(al_cstr(us), "aβcdəfῷ"));
   CHECK(11 == al_ustr_size(us));

   /* Set past end of string. */
   CHECK(al_ustr_set_chr(us, al_ustr_size(us) + 2, L'⁑') == 3);
   CHECK(0 == memcmp(al_cstr(us), "aβcdəfῷ\0\0⁑", 16));
   CHECK(16 == al_ustr_size(us));

   /* Set before start of string (not allowed). */
   CHECK(al_ustr_set_chr(us, -1, L'⁑') == 0);
   CHECK(16 == al_ustr_size(us));

   al_ustr_free(us);
}

/* Test al_ustr_remove_chr. */
void t43(void)
{
   ALLEGRO_USTR *us = al_ustr_new("«aβῷ»");

   CHECK(al_ustr_remove_chr(us, 2));
   CHECK(0 == strcmp(al_cstr(us), "«βῷ»"));

   CHECK(al_ustr_remove_chr(us, 2));
   CHECK(0 == strcmp(al_cstr(us), "«ῷ»"));

   CHECK(al_ustr_remove_chr(us, 2));
   CHECK(0 == strcmp(al_cstr(us), "«»"));

   /* Not at beginning of code point. */
   CHECK(! al_ustr_remove_chr(us, 1));

   /* Out of bounds. */
   CHECK(! al_ustr_remove_chr(us, -1));
   CHECK(! al_ustr_remove_chr(us, al_ustr_size(us)));

   al_ustr_free(us);
}

/* Test al_ustr_replace_range. */
void t44(void)
{
   ALLEGRO_USTR *us1 = al_ustr_new("Šis kungs par visu samaksās");
   ALLEGRO_USTR *us2 = al_ustr_new("ī kundze");

   CHECK(al_ustr_replace_range(us1, 2, 10, us2));
   CHECK(0 == strcmp(al_cstr(us1), "Šī kundze par visu samaksās"));

   /* Insert into itself. */
   CHECK(al_ustr_replace_range(us1, 5, 11, us1));
   CHECK(0 == strcmp(al_cstr(us1),
         "Šī Šī kundze par visu samaksās par visu samaksās"));

   al_ustr_free(us1);
   al_ustr_free(us2);
}

/* Test al_ustr_replace_range (part 2). */
void t45(void)
{
   ALLEGRO_USTR *us1 = al_ustr_new("abcdef");
   ALLEGRO_USTR *us2 = al_ustr_new("ABCDEF");

   /* Start1 < 0 [not allowed] */
   CHECK(! al_ustr_replace_range(us1, -1, 1, us2));

   /* Start1 > end(us1) [padded] */
   CHECK(al_ustr_replace_range(us1, 8, 100, us2));
   CHECK(0 == memcmp(al_cstr(us1), "abcdef\0\0ABCDEF", 15));

   /* Start1 > end1 [not allowed] */
   CHECK(! al_ustr_replace_range(us1, 8, 1, us2));
   CHECK(0 == memcmp(al_cstr(us1), "abcdef\0\0ABCDEF", 15));

   al_ustr_free(us1);
   al_ustr_free(us2);
}

extern bool call_vappendf(ALLEGRO_USTR *us, const char *fmt, ...);

/* Test al_ustr_newf, al_ustr_appendf, al_ustr_vappendf. */
void t46(void)
{
   ALLEGRO_USTR *us;

   us = al_ustr_newf("%s %c %02d", "hõljuk", 'c', 42);
   CHECK(0 == strcmp(al_cstr(us), "hõljuk c 42"));

   CHECK(al_ustr_appendf(us, " %s", "Luftchüssiboot"));
   CHECK(0 == strcmp(al_cstr(us), "hõljuk c 42 Luftchüssiboot"));

   CHECK(call_vappendf(us, " %s", "χόβερκράφτ"));
   CHECK(0 == strcmp(al_cstr(us), "hõljuk c 42 Luftchüssiboot χόβερκράφτ"));

   al_ustr_free(us);
   
   // FIXME: This crashes with revision 11638
   us = al_ustr_new("");
   call_vappendf(us, "%s", "test");
   al_ustr_free(us);
}

bool call_vappendf(ALLEGRO_USTR *us, const char *fmt, ...)
{
   va_list ap;
   bool rc;

   va_start(ap, fmt);
   rc = al_ustr_vappendf(us, fmt, ap);
   va_end(ap);
   return rc;
}

/* Test al_ustr_compare, al_ustr_ncompare. */
void t47(void)
{
   ALLEGRO_USTR_INFO i1;
   ALLEGRO_USTR_INFO i2;

   CHECK(al_ustr_compare(
            al_ref_cstr(&i1, "Thú mỏ vịt"),
            al_ref_cstr(&i2, "Thú mỏ vịt")) == 0);

   CHECK(al_ustr_compare(
            al_ref_cstr(&i1, "Thú mỏ vị"),
            al_ref_cstr(&i2, "Thú mỏ vịt")) < 0);

   CHECK(al_ustr_compare(
            al_ref_cstr(&i1, "Thú mỏ vịt"),
            al_ref_cstr(&i2, "Thú mỏ vit")) > 0);

   CHECK(al_ustr_ncompare(
            al_ref_cstr(&i1, "Thú mỏ vịt"),
            al_ref_cstr(&i2, "Thú mỏ vit"), 8) == 0);

   CHECK(al_ustr_ncompare(
            al_ref_cstr(&i1, "Thú mỏ vịt"),
            al_ref_cstr(&i2, "Thú mỏ vit"), 9) > 0);

   CHECK(al_ustr_ncompare(
            al_ref_cstr(&i1, "Thú mỏ vịt"),
            al_ref_cstr(&i2, "platypus"), 0) == 0);
}

/* Test al_ustr_has_prefix, al_ustr_has_suffix. */
void t48(void)
{
   ALLEGRO_USTR_INFO i1;
   ALLEGRO_USTR *us1 = al_ref_cstr(&i1, "Thú mỏ vịt");

   /* The _cstr versions are simple wrappers around the real functions so its
    * okay to test them only.
    */

   CHECK(al_ustr_has_prefix_cstr(us1, ""));
   CHECK(al_ustr_has_prefix_cstr(us1, "Thú"));
   CHECK(! al_ustr_has_prefix_cstr(us1, "Thú mỏ vịt."));

   CHECK(al_ustr_has_suffix_cstr(us1, ""));
   CHECK(al_ustr_has_suffix_cstr(us1, "vịt"));
   CHECK(! al_ustr_has_suffix_cstr(us1, "Thú mỏ vịt."));
}

/* Test al_ustr_find_replace, al_ustr_find_replace_cstr. */
void t49(void)
{
   ALLEGRO_USTR *us;
   ALLEGRO_USTR_INFO findi;
   ALLEGRO_USTR_INFO repli;
   ALLEGRO_USTR *find;
   ALLEGRO_USTR *repl;

   us = al_ustr_new("aábdðeéfghiíaábdðeéfghií");
   find = al_ref_cstr(&findi, "ðeéf");
   repl = al_ref_cstr(&repli, "deef");

   CHECK(al_ustr_find_replace(us, 0, find, repl));
   CHECK(0 == strcmp(al_cstr(us), "aábddeefghiíaábddeefghií"));

   find = al_ref_cstr(&findi, "aá");
   repl = al_ref_cstr(&repli, "AÁ");

   CHECK(al_ustr_find_replace(us, 14, find, repl));
   CHECK(0 == strcmp(al_cstr(us), "aábddeefghiíAÁbddeefghií"));

   CHECK(al_ustr_find_replace_cstr(us, 0, "dd", "đ"));
   CHECK(0 == strcmp(al_cstr(us), "aábđeefghiíAÁbđeefghií"));

   /* Not allowed */
   find = al_ustr_empty_string();
   CHECK(! al_ustr_find_replace(us, 0, find, repl));
   CHECK(0 == strcmp(al_cstr(us), "aábđeefghiíAÁbđeefghií"));

   al_ustr_free(us);
}

/*---------------------------------------------------------------------------*/

const test_t all_tests[] =
{
   NULL, t1, t2, t3, t4, t5, t6, t7, t8, t9,
   t10, t11, t12, t13, t14, t15, t16, t17, t18, t19,
   t20, t21, t22, t23, t24, t25, t26, t27, t28, t29,
   t30, t31, t32, t33, t34, t35, t36, t37, t38, t39,
   t40, t41, t42, t43, t44, t45, t46, t47, t48, t49
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
