#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_file.h"


/* Function: al_fopen
 */
ALLEGRO_FILE *al_fopen(const char *path, const char *mode)
{
   const ALLEGRO_FILE_INTERFACE *drv = al_get_new_file_interface();
   ASSERT(path);
   ASSERT(mode);
   ASSERT(drv);

   return drv->fi_fopen(path, mode);
}


/* Function: al_fclose
 */
void al_fclose(ALLEGRO_FILE *f)
{
   if (f) {
      f->vtable->fi_fclose(f);
   }
}


/* Function: al_fread
 */
size_t al_fread(ALLEGRO_FILE *f, void *ptr, size_t size)
{
   ASSERT(f);
   ASSERT(ptr);

   return f->vtable->fi_fread(f, ptr, size);
}


/* Function: al_fwrite
 */
size_t al_fwrite(ALLEGRO_FILE *f, const void *ptr, size_t size)
{
   ASSERT(f);
   ASSERT(ptr);

   return f->vtable->fi_fwrite(f, ptr, size);
}


/* Function: al_fflush
 */
bool al_fflush(ALLEGRO_FILE *f)
{
   ASSERT(f);

   return f->vtable->fi_fflush(f);
}


/* Function: al_ftell
 */
int64_t al_ftell(ALLEGRO_FILE *f)
{
   ASSERT(f);

   return f->vtable->fi_ftell(f);
}


/* Function: al_fseek
 */
bool al_fseek(ALLEGRO_FILE *f, int64_t offset, int whence)
{
   ASSERT(f);
   ASSERT(offset >= 0);
   ASSERT(
      whence == ALLEGRO_SEEK_SET ||
      whence == ALLEGRO_SEEK_CUR ||
      whence == ALLEGRO_SEEK_END
   );

   return f->vtable->fi_fseek(f, offset, whence);
}


/* Function: al_feof
 */
bool al_feof(ALLEGRO_FILE *f)
{
   ASSERT(f);

   return f->vtable->fi_feof(f);
}


/* Function: al_ferror
 */
bool al_ferror(ALLEGRO_FILE *f)
{
   ASSERT(f);

   return f->vtable->fi_ferror(f);
}


/* Function: al_fgetc
 */
int al_fgetc(ALLEGRO_FILE *f)
{
   uint8_t c;
   ASSERT(f);

   if (al_fread(f, &c, 1) != 1) {
      return EOF;
   }

   return c;
}


/* Function: al_fputc
 */
int al_fputc(ALLEGRO_FILE *f, int c)
{
   ASSERT(f);

   if (al_fwrite(f, &c, 1) != 1) {
      return EOF;
   }

   return c;
}


/* Function: al_fread16le
 */
int16_t al_fread16le(ALLEGRO_FILE *f)
{
   int b1, b2;
   ASSERT(f);

   if ((b1 = al_fgetc(f)) != EOF)
      if ((b2 = al_fgetc(f)) != EOF)
         return ((b2 << 8) | b1);

   return EOF;
}


/* Function: al_fread32le
 */
int32_t al_fread32le(ALLEGRO_FILE *f, bool *ret_success)
{
   int b1, b2, b3, b4;
   ASSERT(f);

   if ((b1 = al_fgetc(f)) != EOF) {
      if ((b2 = al_fgetc(f)) != EOF) {
         if ((b3 = al_fgetc(f)) != EOF) {
            if ((b4 = al_fgetc(f)) != EOF) {
               if (ret_success)
                  *ret_success = true;
               return (((int32_t)b4 << 24) | ((int32_t)b3 << 16) |
                       ((int32_t)b2 << 8) | (int32_t)b1);
            }
         }
      }
   }

   if (ret_success)
      *ret_success = false;
   return EOF;
}


/* Function: al_fwrite16le
 */
size_t al_fwrite16le(ALLEGRO_FILE *f, int16_t w)
{
   int8_t b1, b2;
   ASSERT(f);

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (al_fputc(f, b2) == b2) {
      if (al_fputc(f, b1) == b1) {
         return 2;
      }
      return 1;
   }
   return 0;
}


/* Function: al_fwrite32le
 */
size_t al_fwrite32le(ALLEGRO_FILE *f, int32_t l)
{
   int8_t b1, b2, b3, b4;
   ASSERT(f);

   b1 = ((l & 0xFF000000L) >> 24);
   b2 = ((l & 0x00FF0000L) >> 16);
   b3 = ((l & 0x0000FF00L) >> 8);
   b4 = l & 0x00FF;

   if (al_fputc(f, b4) == b4) {
      if (al_fputc(f, b3) == b3) {
         if (al_fputc(f, b2) == b2) {
            if (al_fputc(f, b1) == b1) {
               return 4;
            }
            return 3;
         }
         return 2;
      }
      return 1;
   }
   return 0;
}


/* Function: al_fread16be
 */
int16_t al_fread16be(ALLEGRO_FILE *f)
{
   int b1, b2;
   ASSERT(f);

   if ((b1 = al_fgetc(f)) != EOF)
      if ((b2 = al_fgetc(f)) != EOF)
         return ((b1 << 8) | b2);

   return EOF;
}


/* Function: al_fread32be
 */
int32_t al_fread32be(ALLEGRO_FILE *f, bool *ret_success)
{
   int b1, b2, b3, b4;
   ASSERT(f);

   if ((b1 = al_fgetc(f)) != EOF) {
      if ((b2 = al_fgetc(f)) != EOF) {
         if ((b3 = al_fgetc(f)) != EOF) {
            if ((b4 = al_fgetc(f)) != EOF) {
               if (ret_success)
                  *ret_success = true;
               return (((int32_t)b1 << 24) | ((int32_t)b2 << 16) |
                       ((int32_t)b3 << 8) | (int32_t)b4);
            }
         }
      }
   }

   if (ret_success)
      *ret_success = false;
   return EOF;
}


/* Function: al_fwrite16be
 */
size_t al_fwrite16be(ALLEGRO_FILE *f, int16_t w)
{
   int b1, b2;
   ASSERT(f);

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (al_fputc(f, b1) == b1) {
      if (al_fputc(f, b2) == b2) {
         return 2;
      }
      return 1;
   }
   return 0;
}


/* Function: al_fwrite32be
 */
size_t al_fwrite32be(ALLEGRO_FILE *f, int32_t l)
{
   int b1, b2, b3, b4;
   ASSERT(f);

   b1 = ((l & 0xFF000000L) >> 24);
   b2 = ((l & 0x00FF0000L) >> 16);
   b3 = ((l & 0x0000FF00L) >> 8);
   b4 = l & 0x00FF;

   if (al_fputc(f, b1)==b1) {
      if (al_fputc(f, b2)==b2) {
         if (al_fputc(f, b3)==b3) {
            if (al_fputc(f, b4)==b4)
               return 4;
            return 3;
         }
         return 2;
      }
      return 1;
   }
   return 0;
}


/* Function: al_fgets
 */
char *al_fgets(ALLEGRO_FILE *f, char *p, size_t max)
{
   int c = 0;
   ALLEGRO_USTR *u;
   ASSERT(f);

   al_set_errno(0);

   if ((c = al_fgetc(f)) == EOF) {
      return NULL;
   }

   u = al_ustr_new("");

   do {
      char c2[] = " ";
      if (c == '\r' || c == '\n') {
         if (c == '\r') {
            /* eat the following \n, if any */
            c = al_fgetc(f);
            if ((c != '\n') && (c != EOF))
               al_fungetc(f, c);
         }
         break;
      }

      /* write the character */
      c2[0] = c;
      al_ustr_append_cstr(u, c2);
   } while ((c = al_fgetc(f)) != EOF);

   _al_sane_strncpy(p, al_cstr(u), max);
   al_ustr_free(u);

   if (c == '\0' || al_get_errno())
      return NULL;

   return p;
}


/* Function: al_fputs
 */
int al_fputs(ALLEGRO_FILE *f, char const *p)
{
   char const *s = p;
   ASSERT(f);
   ASSERT(p);

   al_set_errno(0);

   while (*s) {
      #if (defined ALLEGRO_DOS) || (defined ALLEGRO_WINDOWS)
         if (*s == '\n')
            al_fputc(f, '\r');
      #endif

      al_fputc(f, *s);
      s++;
   }

   if (al_get_errno())
      return -1;
   else
      return 0;
}


/* Function: al_fungetc
 */
int al_fungetc(ALLEGRO_FILE *f, int c)
{
   ASSERT(f != NULL);

   return f->vtable->fi_fungetc(f, c);
}


/* Function: al_fsize
 */
int64_t al_fsize(ALLEGRO_FILE *f)
{
   ASSERT(f != NULL);

   return f->vtable->fi_fsize(f);
}


/* vim: set sts=3 sw=3 et: */
