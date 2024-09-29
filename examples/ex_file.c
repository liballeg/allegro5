#include <allegro5/allegro.h>

#include "common.c"

static int error = 0;

#define SEP() \
   log_printf("\n# line %d\n", __LINE__)

#define CHECK(_expr, _type, _expect)                                        \
   do {                                                                     \
      _type res = (_expr);                                                  \
      if (res != (_expect)) {                                               \
         log_printf("FAIL %s\n", #_expr);                                   \
         error++;                                                           \
      } else {                                                              \
         log_printf("OK   %s\n", #_expr);                                   \
      }                                                                     \
   } while (0)

static void read_test(void)
{
   A5O_FILE *f;
   uint8_t bs[8];
   int32_t sz;

   f = al_fopen("data/allegro.pcx", "rb");
   if (!f) {
      error++;
      return;
   }

   /* Test: reading bytes advances position. */
   SEP();
   CHECK(al_ftell(f), int, 0);
   CHECK(al_fgetc(f), int, 0xa);
   CHECK(al_fgetc(f), int, 0x5);
   CHECK(al_ftell(f), int64_t, 2);

   /* Test: ungetc moves position back. */
   SEP();
   CHECK(al_fungetc(f, 0x55), bool, true);
   CHECK(al_ftell(f), int64_t, 1);

   /* Test: read buffer. */
   SEP();
   CHECK(al_fread(f, bs, sizeof(bs)), size_t, sizeof(bs));
   CHECK(bs[0], uint8_t, 0x55); /* pushback */
   CHECK(bs[1], uint8_t, 0x01);
   CHECK(bs[2], uint8_t, 0x08);
   CHECK(bs[3], uint8_t, 0x00);
   CHECK(bs[4], uint8_t, 0x00);
   CHECK(bs[5], uint8_t, 0x00);
   CHECK(bs[6], uint8_t, 0x00);
   CHECK(bs[7], uint8_t, 0x3f);
   CHECK(al_ftell(f), int64_t, 9);

   /* Test: seek absolute. */
   SEP();
   CHECK(al_fseek(f, 13, A5O_SEEK_SET), bool, true);
   CHECK(al_ftell(f), int64_t, 13);
   CHECK(al_fgetc(f), int, 0x02);

   /* Test: seek nowhere. */
   SEP();
   CHECK(al_fseek(f, 0, A5O_SEEK_CUR), bool, true);
   CHECK(al_ftell(f), int64_t, 14);
   CHECK(al_fgetc(f), int, 0xe0);

   /* Test: seek nowhere with pushback. */
   SEP();
   CHECK(al_fungetc(f, 0x55), bool, true);
   CHECK(al_fseek(f, 0, A5O_SEEK_CUR), bool, true);
   CHECK(al_ftell(f), int64_t, 14);
   CHECK(al_fgetc(f), int, 0xe0);

   /* Test: seek relative backwards. */
   SEP();
   CHECK(al_fseek(f, -3, A5O_SEEK_CUR), bool, true);
   CHECK(al_ftell(f), int64_t, 12);
   CHECK(al_fgetc(f), int, 0x80);

   /* Test: seek backwards with pushback. */
   SEP();
   CHECK(al_ftell(f), int64_t, 13);
   CHECK(al_fungetc(f, 0x66),  bool, true);
   CHECK(al_ftell(f), int64_t, 12);
   CHECK(al_fseek(f, -2, A5O_SEEK_CUR), bool, true);
   CHECK(al_ftell(f), int64_t, 10);
   CHECK(al_fgetc(f), int, 0xc7);

   /* Test: seek relative to end. */
   SEP();
   CHECK(al_fseek(f, 0, A5O_SEEK_END), bool, true);
   CHECK(al_feof(f), bool, false);
   CHECK(al_ftell(f), int64_t, 0xab06);

   /* Test: read past EOF. */
   SEP();
   CHECK(al_fgetc(f), int, -1);
   CHECK(al_feof(f), bool, true);
   CHECK(al_ferror(f), int, 0);

   /* Test: seek clears EOF indicator. */
   SEP();
   CHECK(al_fseek(f, 0, A5O_SEEK_END), bool, true);
   CHECK(al_feof(f), bool, false);
   CHECK(al_ftell(f), int64_t, 0xab06);

   /* Test: seek backwards from end. */
   SEP();
   CHECK(al_fseek(f, -20, A5O_SEEK_END), bool, true);
   CHECK(al_ftell(f), int64_t, 0xaaf2);

   /* Test: seek forwards from end. */
   SEP();
   CHECK(al_fseek(f, 20, A5O_SEEK_END), bool, true);
   CHECK(al_ftell(f), int64_t, 0xab1a);
   CHECK(al_fgetc(f), int, -1);
   CHECK(al_feof(f), bool, true);

   /* Test: get file size if possible. */
   SEP();
   CHECK(sz = al_fsize(f), int64_t, sz);
   if (sz != -1)
      CHECK(sz, int64_t, 0xab06);

   /* Test: close. */
   SEP();
   CHECK(al_fclose(f), bool, true);
}

int main(int argc, char *argv[])
{
   (void)argc;
   (void)argv;

   if (!al_init())
      return 1;
   init_platform_specific();
   open_log_monospace();

   read_test();

   close_log(true);

   if (error) {
      exit(EXIT_FAILURE);
   }

   return 0;
}

/* vim: set sts=3 sw=3 et: */
