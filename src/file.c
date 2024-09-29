/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      File I/O routines.
 *
 *      See LICENSE.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_file.h"


/* Function: al_fopen
 */
A5O_FILE *al_fopen(const char *path, const char *mode)
{
   return al_fopen_interface(al_get_new_file_interface(), path, mode);
}


/* Function: al_fopen_interface
 */
A5O_FILE *al_fopen_interface(const A5O_FILE_INTERFACE *drv,
   const char *path, const char *mode)
{
   A5O_FILE *f = NULL;

   ASSERT(drv);
   ASSERT(path);
   ASSERT(mode);

   if (drv->fi_fopen) {
      f = al_malloc(sizeof(*f));
      if (!f) {
         al_set_errno(ENOMEM);
      }
      else {
         f->vtable = drv;
         f->userdata = drv->fi_fopen(path, mode);
         f->ungetc_len = 0;
         if (!f->userdata) {
            al_free(f);
            f = NULL;
         }
      }
   }

   return f;
}


/* Function: al_create_file_handle
 */
A5O_FILE *al_create_file_handle(const A5O_FILE_INTERFACE *drv,
   void *userdata)
{
   A5O_FILE *f;

   ASSERT(drv);

   f = al_malloc(sizeof(*f));
   if (!f) {
      al_set_errno(ENOMEM);
   }
   else {
      f->vtable = drv;
      f->userdata = userdata;
      f->ungetc_len = 0;
   }

   return f;
}


/* Function: al_fclose
 */
bool al_fclose(A5O_FILE *f)
{
   if (f) {
      bool ret = f->vtable->fi_fclose(f);
      al_free(f);
      return ret;
   }

   al_set_errno(EINVAL);
   return false;
}


/* Function: al_fread
 */
size_t al_fread(A5O_FILE *f, void *ptr, size_t size)
{
   ASSERT(f);
   ASSERT(ptr || size == 0);

   if (f->ungetc_len) {
      int bytes_ungetc = 0;
      unsigned char *cptr = ptr;

      while (f->ungetc_len > 0 && size > 0) {
         *cptr++ = f->ungetc[--f->ungetc_len];
         ++bytes_ungetc;
         --size;
      }

      return bytes_ungetc + f->vtable->fi_fread(f, cptr, size);
   }
   else {
      return f->vtable->fi_fread(f, ptr, size);
   }
}


/* Function: al_fwrite
 */
size_t al_fwrite(A5O_FILE *f, const void *ptr, size_t size)
{
   ASSERT(f);
   ASSERT(ptr || size == 0);

   f->ungetc_len = 0;
   return f->vtable->fi_fwrite(f, ptr, size);
}


/* Function: al_fflush
 */
bool al_fflush(A5O_FILE *f)
{
   ASSERT(f);

   return f->vtable->fi_fflush(f);
}


/* Function: al_ftell
 */
int64_t al_ftell(A5O_FILE *f)
{
   ASSERT(f);

   return f->vtable->fi_ftell(f) - f->ungetc_len;
}


/* Function: al_fseek
 */
bool al_fseek(A5O_FILE *f, int64_t offset, int whence)
{
   ASSERT(f);
   /* offset can be negative */
   ASSERT(
      whence == A5O_SEEK_SET ||
      whence == A5O_SEEK_CUR ||
      whence == A5O_SEEK_END
   );

   if (f->ungetc_len) {
      if (whence == A5O_SEEK_CUR) {
         offset -= f->ungetc_len;
      }
      f->ungetc_len = 0;
   }

   return f->vtable->fi_fseek(f, offset, whence);
}


/* Function: al_feof
 */
bool al_feof(A5O_FILE *f)
{
   ASSERT(f);

   return f->ungetc_len == 0 && f->vtable->fi_feof(f);
}


/* Function: al_ferror
 */
int al_ferror(A5O_FILE *f)
{
   ASSERT(f);

   return f->vtable->fi_ferror(f);
}


/* Function: al_ferrmsg
 */
const char *al_ferrmsg(A5O_FILE *f)
{
   const char *msg;

   ASSERT(f);
   msg = f->vtable->fi_ferrmsg(f);
   ASSERT(msg);
   return msg;
}


/* Function: al_fclearerr
 */
void al_fclearerr(A5O_FILE *f)
{
   ASSERT(f);

   f->vtable->fi_fclearerr(f);
}


/* Function: al_fgetc
 */
int al_fgetc(A5O_FILE *f)
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
int al_fputc(A5O_FILE *f, int c)
{
   uint8_t b = (c & 0xff);
   ASSERT(f);

   if (al_fwrite(f, &b, 1) != 1) {
      return EOF;
   }

   return b;
}


/* Function: al_fread16le
 */
int16_t al_fread16le(A5O_FILE *f)
{
   unsigned char b[2];
   ASSERT(f);

   if (al_fread(f, b, 2) == 2) {
      return (((int16_t)b[1] << 8) | (int16_t)b[0]);
   }

   return EOF;
}


/* Function: al_fread32le
 */
int32_t al_fread32le(A5O_FILE *f)
{
   unsigned char b[4];
   ASSERT(f);

   if (al_fread(f, b, 4) == 4) {
      return (((int32_t)b[3] << 24) | ((int32_t)b[2] << 16) |
              ((int32_t)b[1] << 8) | (int32_t)b[0]);
   }

   return EOF;
}


/* Function: al_fwrite16le
 */
size_t al_fwrite16le(A5O_FILE *f, int16_t w)
{
   uint8_t b1, b2;
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
size_t al_fwrite32le(A5O_FILE *f, int32_t l)
{
   uint8_t b1, b2, b3, b4;
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
int16_t al_fread16be(A5O_FILE *f)
{
   unsigned char b[2];
   ASSERT(f);

   if (al_fread(f, b, 2) == 2) {
      return (((int16_t)b[0] << 8) | (int16_t)b[1]);
   }

   return EOF;
}


/* Function: al_fread32be
 */
int32_t al_fread32be(A5O_FILE *f)
{
   unsigned char b[4];
   ASSERT(f);

   if (al_fread(f, b, 4) == 4) {
      return (((int32_t)b[0] << 24) | ((int32_t)b[1] << 16) |
              ((int32_t)b[2] << 8) | (int32_t)b[3]);
   }

   return EOF;
}


/* Function: al_fwrite16be
 */
size_t al_fwrite16be(A5O_FILE *f, int16_t w)
{
   uint8_t b1, b2;
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
size_t al_fwrite32be(A5O_FILE *f, int32_t l)
{
   uint8_t b1, b2, b3, b4;
   ASSERT(f);

   b1 = ((l & 0xFF000000L) >> 24);
   b2 = ((l & 0x00FF0000L) >> 16);
   b3 = ((l & 0x0000FF00L) >> 8);
   b4 = l & 0x00FF;

   if (al_fputc(f, b1) == b1) {
      if (al_fputc(f, b2) == b2) {
         if (al_fputc(f, b3) == b3) {
            if (al_fputc(f, b4) == b4) {
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


/* Function: al_fgets
 */
char *al_fgets(A5O_FILE *f, char * const buf, size_t max)
{
   char *p = buf;
   int c;
   ASSERT(f);
   ASSERT(buf);

   /* Handle silly cases. */
   if (max == 0) {
      return NULL;
   }
   if (max == 1) {
      *buf = '\0';
      return buf;
   }

   /* Return NULL if already at end of file. */
   if ((c = al_fgetc(f)) == EOF) {
      return NULL;
   }

   /* Fill buffer until empty, or we reach a newline or EOF or error. */
   do {
      *p++ = c;
      max--;
      if (max == 1 || c == '\n')
         break;
      c = al_fgetc(f);
   } while (c != EOF);

   /* Return NULL on error. */
   if (c == EOF && al_ferror(f)) {
      return NULL;
   }

   /* Add null terminator. */
   ASSERT(max >= 1);
   *p = '\0';

   return buf;
}


/* Function: al_fget_ustr
 */
A5O_USTR *al_fget_ustr(A5O_FILE *f)
{
   A5O_USTR *us;
   char buf[128];

   if (!al_fgets(f, buf, sizeof(buf))) {
      return NULL;
   }

   us = al_ustr_new("");

   do {
      al_ustr_append_cstr(us, buf);
      if (al_ustr_has_suffix_cstr(us, "\n"))
         break;
   } while (al_fgets(f, buf, sizeof(buf)));

   return us;
}


/* Function: al_fputs
 */
int al_fputs(A5O_FILE *f, char const *p)
{
   size_t n;
   ASSERT(f);
   ASSERT(p);

   n = strlen(p);
   if (al_fwrite(f, p, n) != n) {
      return EOF;
   }

   return n;
}


/* Function: al_fungetc
 */
int al_fungetc(A5O_FILE *f, int c)
{
   ASSERT(f != NULL);

   if (f->vtable->fi_fungetc) {
      return f->vtable->fi_fungetc(f, c);
   }
   else {
      /* If the interface does not provide an implementation for ungetc,
       * then a default one will be used. (Note that if the interface does
       * implement it, then this ungetc buffer will never be filled, and all
       * other references to it within this file will always be ignored.)
       */
      if (f->ungetc_len == A5O_UNGETC_SIZE) {
         return EOF;
      }

      f->ungetc[f->ungetc_len++] = (unsigned char) c;

      return c;
   }
}


/* Function: al_fsize
 */
int64_t al_fsize(A5O_FILE *f)
{
   ASSERT(f != NULL);

   return f->vtable->fi_fsize(f);
}


/* Function: al_get_file_userdata
 */
void *al_get_file_userdata(A5O_FILE *f)
{
   ASSERT(f != NULL);

   return f->userdata;
}


/* Function: al_vfprintf
 */
int al_vfprintf(A5O_FILE *pfile, const char *format, va_list args)
{
   int rv = -1;
   A5O_USTR *ustr = 0;
   size_t size = 0;
   bool success;

   if (pfile != 0 && format != 0)
   {
      ustr = al_ustr_new("");
      if (ustr)
      {
         success = al_ustr_vappendf(ustr, format, args);
         if (success)
         {
            size = al_ustr_size(ustr);
            if (size > 0)
            {
               rv = al_fwrite(pfile, (const void*)(al_cstr(ustr)), size);
               if (rv != (int)size) {
                  rv = -1;
               }
            }
         }
         al_ustr_free(ustr);
      }
   }
   return rv;
}


/* Function: al_fprintf
 */
int al_fprintf(A5O_FILE *pfile, const char *format, ...)
{
   int rv = -1;
   va_list args;

   if (pfile != 0 && format != 0)
   {
      va_start(args, format);
      rv = al_vfprintf(pfile, format, args);
      va_end(args);
   }
   return rv;
}


/* vim: set sts=3 sw=3 et: */
