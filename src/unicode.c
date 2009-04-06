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
 *      String handling functions (UTF-8, Unicode, ASCII, etc).
 *
 *      By Shawn Hargreaves.
 *
 *      Case conversion functions improved by Ole Laursen.
 *
 *      ustrrchr() and usprintf() improvements by Sven Sandberg.
 *
 *      Peter Cech added some non-ASCII characters to uissspace().
 *
 *      size-aware (aka 'z') functions added by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include <limits.h>
#include <string.h>
#include <stdio.h>

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_memory.h"

#define MAX _ALLEGRO_MAX


/* ascii_getc:
 *  Reads a character from an ASCII string.
 */
static int ascii_getc(AL_CONST char *s)
{
   return *((unsigned char *)s);
}



/* ascii_getx:
 *  Reads a character from an ASCII string, advancing the pointer position.
 */
static int ascii_getx(char **s)
{
   return *((unsigned char *)((*s)++));
}



/* ascii_setc:
 *  Sets a character in an ASCII string.
 */
static int ascii_setc(char *s, int c)
{
   *s = c;
   return 1;
}



/* ascii_width:
 *  Returns the width of an ASCII character.
 */
static int ascii_width(AL_CONST char *s)
{
   (void)s;
   return 1;
}



/* ascii_cwidth:
 *  Returns the width of an ASCII character.
 */
static int ascii_cwidth(int c)
{
   (void)c;
   return 1;
}



/* ascii_isok:
 *  Checks whether this character can be encoded in 8-bit ASCII format.
 */
static int ascii_isok(int c)
{
   return ((c >= 0) && (c <= 255));
}



/* lookup table for implementing 8-bit codepage modes */
static unsigned short _codepage_table[] =
{
   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
   0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
   0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
   0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
   0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
   0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
   0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
   0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
   0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 
   0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 
   0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 
   0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 
   0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 
   0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 
   0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 
   0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E,
};



/* this default table reduces Latin-1 and Extended-A characters to 7 bits */
static unsigned short _codepage_extras[] =
{
   0xA1, '!',  0xA2, 'c',  0xA3, '#',  0xB5, 'u',  0xBF, '?',  0xC0, 'A', 
   0xC1, 'A',  0xC2, 'A',  0xC3, 'A',  0xC4, 'A',  0xC5, 'A',  0xC6, 'A', 
   0xC7, 'C',  0xC8, 'E',  0xC9, 'E',  0xCA, 'E',  0xCB, 'E',  0xCC, 'I', 
   0xCD, 'I',  0xCE, 'I',  0xCF, 'I',  0xD0, 'D',  0xD1, 'N',  0xD2, 'O', 
   0xD3, 'O',  0xD4, 'O',  0xD5, 'O',  0xD6, 'O',  0xD7, 'X',  0xD8, '0', 
   0xD9, 'U',  0xDA, 'U',  0xDB, 'U',  0xDC, 'U',  0xDD, 'Y',  0xDE, 'P', 
   0xDF, 'S',  0xE0, 'a',  0xE1, 'a',  0xE2, 'a',  0xE3, 'a',  0xE4, 'a', 
   0xE5, 'a',  0xE6, 'a',  0xE7, 'c',  0xE8, 'e',  0xE9, 'e',  0xEA, 'e', 
   0xEB, 'e',  0xEC, 'i',  0xED, 'i',  0xEE, 'i',  0xEF, 'i',  0xF0, 'o', 
   0xF1, 'n',  0xF2, 'o',  0xF3, 'o',  0xF4, 'o',  0xF5, 'o',  0xF6, 'o', 
   0xF8, 'o',  0xF9, 'u',  0xFA, 'u',  0xFB, 'u',  0xFC, 'u',  0xFD, 'y', 
   0xFE, 'p',  0xFF, 'y',

   0x100, 'A',  0x101, 'a',  0x102, 'A',  0x103, 'a',  0x104, 'A',
   0x105, 'a',  0x106, 'C',  0x107, 'c',  0x108, 'C',  0x109, 'c',
   0x10a, 'C',  0x10b, 'c',  0x10c, 'C',  0x10d, 'c',  0x10e, 'D',
   0x10f, 'd',  0x110, 'D',  0x111, 'd',  0x112, 'E',  0x113, 'e',
   0x114, 'E',  0x115, 'e',  0x116, 'E',  0x117, 'e',  0x118, 'E',
   0x119, 'e',  0x11a, 'E',  0x11b, 'e',  0x11c, 'G',  0x11d, 'g',
   0x11e, 'G',  0x11f, 'g',  0x120, 'G',  0x121, 'g',  0x122, 'G',
   0x123, 'g',  0x124, 'H',  0x125, 'h',  0x126, 'H',  0x127, 'h',
   0x128, 'I',  0x129, 'i',  0x12a, 'I',  0x12b, 'i',  0x12c, 'I',
   0x12d, 'i',  0x12e, 'I',  0x12f, 'i',  0x130, 'I',  0x131, 'i',
   0x134, 'J',  0x135, 'j',  0x136, 'K',  0x137, 'k',  0x138, 'K',
   0x139, 'L',  0x13a, 'l',  0x13b, 'L',  0x13c, 'l',  0x13d, 'L',
   0x13e, 'l',  0x13f, 'L',  0x140, 'l',  0x141, 'L',  0x142, 'l',
   0x143, 'N',  0x144, 'n',  0x145, 'N',  0x146, 'n',  0x147, 'N',
   0x148, 'n',  0x149, 'n',  0x14a, 'N',  0x14b, 'n',  0x14c, 'O',
   0x14d, 'o',  0x14e, 'O',  0x14f, 'o',  0x150, 'O',  0x151, 'o',
   0x154, 'R',  0x155, 'r',  0x156, 'R',  0x157, 'r',  0x158, 'R',
   0x159, 'r',  0x15a, 'S',  0x15b, 's',  0x15c, 'S',  0x15d, 's',
   0x15e, 'S',  0x15f, 's',  0x160, 'S',  0x161, 's',  0x162, 'T',
   0x163, 't',  0x164, 'T',  0x165, 't',  0x166, 'T',  0x167, 't',
   0x168, 'U',  0x169, 'u',  0x16a, 'U',  0x16b, 'u',  0x16c, 'U',
   0x16d, 'u',  0x16e, 'U',  0x16f, 'u',  0x170, 'U',  0x171, 'u',
   0x172, 'U',  0x173, 'u',  0x174, 'W',  0x175, 'w',  0x176, 'Y',
   0x177, 'y',  0x178, 'y',  0x179, 'Z',  0x17a, 'z',  0x17b, 'Z',
   0x17c, 'z',  0x17d, 'Z',  0x17e, 'z',  0
};



/* access via pointers so they can be changed by the user */
static AL_CONST unsigned short *codepage_table = _codepage_table;
static AL_CONST unsigned short *codepage_extras = _codepage_extras;



/* ascii_cp_getc:
 *  Reads a character from an ASCII codepage string.
 */
static int ascii_cp_getc(AL_CONST char *s)
{
   return codepage_table[*((unsigned char *)s)];
}



/* ascii_cp_getx:
 *  Reads from an ASCII codepage string, advancing pointer position.
 */
static int ascii_cp_getx(char **s)
{
   return codepage_table[*((unsigned char *)((*s)++))];
}



/* ascii_cp_setc:
 *  Sets a character in an ASCII codepage string.
 */
static int ascii_cp_setc(char *s, int c)
{
   int i;

   for (i=0; i<256; i++) {
      if (codepage_table[i] == c) {
	 *s = i;
	 return 1;
      }
   }

   if (codepage_extras) {
      for (i=0; codepage_extras[i]; i+=2) {
	 if (codepage_extras[i] == c) {
	    *s = codepage_extras[i+1];
	    return 1;
	 }
      } 
   }

   *s = '^';
   return 1;
}



/* ascii_cp_isok:
 *  Checks whether this character can be encoded in ASCII codepage format.
 */
static int ascii_cp_isok(int c)
{
   int i;

   for (i=0; i<256; i++) {
      if (codepage_table[i] == c)
	 return true;
   }

   if (codepage_extras) {
      for (i=0; codepage_extras[i]; i+=2) {
	 if (codepage_extras[i] == c)
	    return true;
      } 
   }

   return false;
}



/* unicode_getc:
 *  Reads a character from a Unicode string.
 */
static int unicode_getc(AL_CONST char *s)
{
   return *((unsigned short *)s);
}



/* unicode_getx:
 *  Reads a character from a Unicode string, advancing the pointer position.
 */
static int unicode_getx(char **s)
{
   int c = *((unsigned short *)(*s));
   (*s) += sizeof(unsigned short);
   return c;
}



/* unicode_setc:
 *  Sets a character in a Unicode string.
 */
static int unicode_setc(char *s, int c)
{
   *((unsigned short *)s) = c;
   return sizeof(unsigned short);
}



/* unicode_width:
 *  Returns the width of a Unicode character.
 */
static int unicode_width(AL_CONST char *s)
{
   (void)s;
   return sizeof(unsigned short);
}



/* unicode_cwidth:
 *  Returns the width of a Unicode character.
 */
static int unicode_cwidth(int c)
{
   (void)c;
   return sizeof(unsigned short);
}



/* unicode_isok:
 *  Checks whether this character can be encoded in 16-bit Unicode format.
 */
static int unicode_isok(int c)
{
   return ((c >= 0) && (c <= 65535));
}



/* utf8_getc:
 *  Reads a character from a UTF-8 string.
 */
static int utf8_getc(AL_CONST char *s)
{
   int c = *((unsigned char *)(s++));
   int n, t;

   if (c & 0x80) {
      n = 1;
      while (c & (0x80>>n))
	 n++;

      c &= (1<<(8-n))-1;

      while (--n > 0) {
	 t = *((unsigned char *)(s++));

	 if ((!(t&0x80)) || (t&0x40))
	    return '^';

	 c = (c<<6) | (t&0x3F);
      }
   }

   return c;
}



/* utf8_getx:
 *  Reads a character from a UTF-8 string, advancing the pointer position.
 */
static int utf8_getx(char **s)
{
   int c = *((unsigned char *)((*s)++));
   int n, t;

   if (c & 0x80) {
      n = 1;
      while (c & (0x80>>n))
	 n++;

      c &= (1<<(8-n))-1;

      while (--n > 0) {
	 t = *((unsigned char *)((*s)++));

	 if ((!(t&0x80)) || (t&0x40)) {
	    (*s)--;
	    return '^';
	 }

	 c = (c<<6) | (t&0x3F);
      }
   }

   return c;
}



/* utf8_setc:
 *  Sets a character in a UTF-8 string.
 */
static int utf8_setc(char *s, int c)
{
   int size, bits, b, i;

   if (c < 128) {
      *s = c;
      return 1;
   }

   bits = 7;
   while (c >= (1<<bits))
      bits++;

   size = 2;
   b = 11;

   while (b < bits) {
      size++;
      b += 5;
   }

   b -= (7-size);
   s[0] = c>>b;

   for (i=0; i<size; i++)
      s[0] |= (0x80>>i);

   for (i=1; i<size; i++) {
      b -= 6;
      s[i] = 0x80 | ((c>>b)&0x3F);
   }

   return size;
}



/* utf8_width:
 *  Returns the width of a UTF-8 character.
 */
static int utf8_width(AL_CONST char *s)
{
   int c = *((unsigned char *)s);
   int n = 1;

   if (c & 0x80) {
      while (c & (0x80>>n))
	 n++;
   }

   return n;
}



/* utf8_cwidth:
 *  Returns the width of a UTF-8 character.
 */
static int utf8_cwidth(int c)
{
   int size, bits, b;

   if (c < 128)
      return 1;

   bits = 7;
   while (c >= (1<<bits))
      bits++;

   size = 2;
   b = 11;

   while (b < bits) {
      size++;
      b += 5;
   }

   return size;
}



/* utf8_isok:
 *  Checks whether this character can be encoded in UTF-8 format.
 */
static int utf8_isok(int c)
{
   (void)c;
   return true;
}



/* string format table, to allow user expansion with other encodings */
static UTYPE_INFO utypes[] =
{
   { U_ASCII,    ascii_getc,    ascii_getx,    ascii_setc,    ascii_width,   ascii_cwidth,   ascii_isok,    1    },
   { U_UTF8,     utf8_getc,     utf8_getx,     utf8_setc,     utf8_width,    utf8_cwidth,    utf8_isok,     4     },
   { U_UNICODE,  unicode_getc,  unicode_getx,  unicode_setc,  unicode_width, unicode_cwidth, unicode_isok,  2  },
   { U_ASCII_CP, ascii_cp_getc, ascii_cp_getx, ascii_cp_setc, ascii_width,   ascii_cwidth,   ascii_cp_isok, 1 },
   { 0,          NULL,          NULL,          NULL,          NULL,          NULL,           NULL,          0                     },
   { 0,          NULL,          NULL,          NULL,          NULL,          NULL,           NULL,          0                     },
   { 0,          NULL,          NULL,          NULL,          NULL,          NULL,           NULL,          0                     },
   { 0,          NULL,          NULL,          NULL,          NULL,          NULL,           NULL,          0                     }
};



/* current format information and worker routines */
static int utype = U_UTF8;

/* ugetc: */
int (*ugetc)(AL_CONST char *s) = utf8_getc;
/* ugetxc: */
int (*ugetx)(char **s) = utf8_getx;
/* ugetxc: */
int (*ugetxc)(AL_CONST char** s) = (int (*)(AL_CONST char**)) utf8_getx;
/* usetc: */
int (*usetc)(char *s, int c) = utf8_setc;
/* uwidth: */
int (*uwidth)(AL_CONST char *s) = utf8_width;
/* ucwidth: */
int (*ucwidth)(int c) = utf8_cwidth;
/* uisok: */
int (*uisok)(int c) = utf8_isok;



/* _find_utype:
 *  Helper for locating a string type description.
 */
UTYPE_INFO *_find_utype(int type)
{
   int i;

   if (type == U_UTF8)
      type = utype;

   for (i=0; i<(int)(sizeof(utypes)/sizeof(UTYPE_INFO)); i++)
      if (utypes[i].id == type)
	 return &utypes[i];

   return NULL;
}



/* set_ucodepage:
 *  Sets lookup table data for the codepage conversion functions.
 */
void set_ucodepage(AL_CONST unsigned short *table, AL_CONST unsigned short *extras)
{
   ASSERT(table);
   codepage_table = table;
   codepage_extras = extras;
}



/* ustrlen:
 *  Unicode-aware version of the ANSI strlen() function.
 */
int ustrlen(AL_CONST char *s)
{
   int c = 0;
   ASSERT(s);

   while (ugetxc(&s))
      c++;

   return c;
}



/* uoffset:
 *  Returns the offset in bytes from the start of the string to the 
 *  character at the specified index. If the index is negative, counts
 *  backward from the end of the string (-1 returns an offset to the
 *  last character).
 */
int uoffset(AL_CONST char *s, int index)
{
   AL_CONST char *orig = s;
   AL_CONST char *last;
   ASSERT(s);

   if (index < 0)
      index += ustrlen(s);

   while (index-- > 0) {
      last = s;
      if (!ugetxc(&s)) {
	 s = last;
	 break;
      }
   }

   return (long)s - (long)orig;
}



/* ugetat:
 *  Returns the character from the specified index within the string.
 */
int ugetat(AL_CONST char *s, int index)
{
   ASSERT(s);
   return ugetc(s + uoffset(s, index));
}



/* usetat:
 *  Modifies the character at the specified index within the string,
 *  handling adjustments for variable width data. Returns how far the
 *  rest of the string was moved.
 */
int usetat(char *s, int index, int c)
{
   int oldw, neww;
   ASSERT(s);

   s += uoffset(s, index);

   oldw = uwidth(s);
   neww = ucwidth(c);

   if (oldw != neww)
      memmove(s+neww, s+oldw, ustrsizez(s+oldw));

   usetc(s, c);

   return neww-oldw;
}



/* uinsert:
 *  Inserts a character at the specified index within a string, sliding
 *  following data along to make room. Returns how far the data was moved.
 */
int uinsert(char *s, int index, int c)
{
   int w = ucwidth(c);
   ASSERT(s);

   s += uoffset(s, index);
   memmove(s+w, s, ustrsizez(s));
   usetc(s, c);

   return w;
}



/* uremove:
 *  Removes the character at the specified index within the string, sliding
 *  following data back to make room. Returns how far the data was moved.
 */
int uremove(char *s, int index)
{
   int w;
   ASSERT(s);

   s += uoffset(s, index);
   w = uwidth(s);
   memmove(s, s+w, ustrsizez(s+w));

   return -w;
}



/* uwidth_max:
 *  Returns the largest possible size of a character in the specified
 *  encoding format, in bytes.
 */
int uwidth_max(int type)
{
   UTYPE_INFO *info;

   info = _find_utype(type);
   if (!info)
      return 0;

   return info->u_width_max;
}



/* uisspace:
 *  Unicode-aware version of the ANSI isspace() function.
 */
int uisspace(int c)
{
   return ((c == ' ') || (c == '\t') || (c == '\r') || 
	   (c == '\n') || (c == '\f') || (c == '\v') ||
	   (c == 0x1680) || ((c >= 0x2000) && (c <= 0x200A)) ||
	   (c == 0x2028) || (c == 0x202f) || (c == 0x3000));
}



/* uisdigit:
 *  Unicode-aware version of the ANSI isdigit() function.
 */
int uisdigit(int c)
{
   return ((c >= '0') && (c <= '9'));
}



/* ustrdup:
 *  Returns a newly allocated copy of the src string, which must later be
 *  freed by the caller.
 */
char *_ustrdup(AL_CONST char *src, AL_METHOD(void *, malloc_func, (size_t)))
{
   char *s;
   int size;
   ASSERT(src);
   ASSERT(malloc_func);

   size = ustrsizez(src);

   s = malloc_func(size);
   if (s)
      ustrzcpy(s, size, src);
   else
      al_set_errno(ENOMEM);

   return s;
}



/* ustrsize:
 *  Returns the size of the specified string in bytes, not including the
 *  trailing zero.
 */
int ustrsize(AL_CONST char *s)
{
   AL_CONST char *orig = s;
   AL_CONST char *last;
   ASSERT(s);

   do {
      last = s;
   } while (ugetxc(&s) != 0);

   return (long)last - (long)orig;
}



/* ustrsizez:
 *  Returns the size of the specified string in bytes, including the
 *  trailing zero.
 */
int ustrsizez(AL_CONST char *s)
{
   AL_CONST char *orig = s;
   ASSERT(s);

   do {
   } while (ugetxc(&s) != 0);

   return (long)s - (long)orig;
}



/* ustrzcpy:
 *  Enhanced Unicode-aware version of the ANSI strcpy() function
 *  that can handle the size (in bytes) of the destination string.
 *  The raw Unicode-aware version of ANSI strcpy() is defined as:
 *   #define ustrcpy(dest, src) ustrzcpy(dest, INT_MAX, src)
 */
char *ustrzcpy(char *dest, int size, AL_CONST char *src)
{
   int pos = 0;
   int c;
   ASSERT(dest);
   ASSERT(src);
   ASSERT(size > 0);

   size -= ucwidth(0);
   ASSERT(size >= 0);

   while ((c = ugetxc(&src)) != 0) {
      size -= ucwidth(c);
      if (size < 0)
         break;

      pos += usetc(dest+pos, c);
   }

   usetc(dest+pos, 0);

   return dest;
}



/* ustrcmp:
 *  Unicode-aware version of the ANSI strcmp() function.
 */
int ustrcmp(AL_CONST char *s1, AL_CONST char *s2)
{
   int c1, c2;
   ASSERT(s1);
   ASSERT(s2);

   for (;;) {
      c1 = ugetxc(&s1);
      c2 = ugetxc(&s2);

      if (c1 != c2)
	 return c1 - c2;

      if (!c1)
	 return 0;
   }
}



/* ustrchr:
 *  Unicode-aware version of the ANSI strchr() function.
 */
char *ustrchr(AL_CONST char *s, int c)
{
   int d;
   ASSERT(s);

   while ((d = ugetc(s)) != 0) {
      if (c == d)
	 return (char *)s;

      s += uwidth(s);
   }

   if (!c)
      return (char *)s;

   return NULL;
}


