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
	 return TRUE;
   }

   if (codepage_extras) {
      for (i=0; codepage_extras[i]; i+=2) {
	 if (codepage_extras[i] == c)
	    return TRUE;
      } 
   }

   return FALSE;
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
   return TRUE;
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



/* need_uconvert:
 *  Decides whether a conversion is required to make this string be in the
 *  new type. No conversion will be needed if both types are the same, or
 *  when going from ASCII <-> UTF8 where the data is 7-bit clean.
 */
int need_uconvert(AL_CONST char *s, int type, int newtype)
{
   int c;
   ASSERT(s);
   
   if (type == U_UTF8)
      type = utype;

   if (newtype == U_UTF8)
      newtype = utype;

   if (type == newtype)
      return FALSE;

   if (((type == U_ASCII) || (type == U_UTF8)) && ((newtype == U_ASCII) || (newtype == U_UTF8))) {
      do {
	 c = *((unsigned char *)(s++));
	 if (!c)
	    return FALSE;
      } while (c <= 127);
   }

   return TRUE;
}



/* uconvert_size:
 *  Returns the number of bytes this string will occupy when converted to
 *  the specified type.
 */
int uconvert_size(AL_CONST char *s, int type, int newtype)
{
   UTYPE_INFO *info, *outfo;
   int size = 0;
   int c;
   ASSERT(s);

   info = _find_utype(type);
   if (!info)
      return 0;

   outfo = _find_utype(newtype);
   if (!outfo)
      return 0;

   size = 0;

   while ((c = info->u_getx((char **)&s)) != 0)
      size += outfo->u_cwidth(c);

   return size + outfo->u_cwidth(0);
}



/* do_uconvert:
 *  Converts a string from one format to another.
 */
void do_uconvert(AL_CONST char *s, int type, char *buf, int newtype, int size)
{
   UTYPE_INFO *info, *outfo;
   int pos = 0;
   int c;
   ASSERT(s);
   ASSERT(buf);
   ASSERT(size > 0);

   info = _find_utype(type);
   if (!info)
      return;

   outfo = _find_utype(newtype);
   if (!outfo)
      return;

   size -= outfo->u_cwidth(0);
   ASSERT(size >= 0);

   while ((c = info->u_getx((char**)&s)) != 0) {
      if (!outfo->u_isok(c))
	 c = '^';

      size -= outfo->u_cwidth(c);
      if (size < 0)
	 break;

      pos += outfo->u_setc(buf+pos, c);
   }

   outfo->u_setc(buf+pos, 0);
}



/* uconvert:
 *  Higher level version of do_uconvert(). This routine is intelligent
 *  enough to just return a copy of the input string if no conversion
 *  is required, and to use an internal static buffer if no user buffer
 *  is provided.
 */
char *uconvert(AL_CONST char *s, int type, char *buf, int newtype, int size)
{
   static char static_buf[1024];
   ASSERT(s);
   ASSERT(size >= 0);

   if (!need_uconvert(s, type, newtype))
      return (char *)s;

   if (!buf) {
      buf = static_buf;
      size = sizeof(static_buf);
   }

   do_uconvert(s, type, buf, newtype, size);
   return buf;
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



/* ustrzcat:
 *  Enhanced Unicode-aware version of the ANSI strcat() function
 *  that can handle the size (in bytes) of the destination string.
 *  The raw Unicode-aware version of ANSI strcat() is defined as:
 *   #define ustrcat(dest, src) ustrzcat(dest, INT_MAX, src)
 */
char *ustrzcat(char *dest, int size, AL_CONST char *src)
{
   int pos;
   int c;
   ASSERT(dest);
   ASSERT(src);
   ASSERT(size > 0);

   pos = ustrsize(dest);
   size -= pos + ucwidth(0);
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



/* ustrzncpy:
 *  Enhanced Unicode-aware version of the ANSI strncpy() function
 *  that can handle the size (in bytes) of the destination string.
 *  The raw Unicode-aware version of ANSI strncpy() is defined as:
 *   #define ustrncpy(dest, src, n) ustrzncpy(dest, INT_MAX, src, n)
 */
char *ustrzncpy(char *dest, int size, AL_CONST char *src, int n)
{
   int pos = 0, len = 0;
   int ansi_oddness = FALSE;
   int c;
   ASSERT(dest);
   ASSERT(src);
   ASSERT(size > 0);
   ASSERT(n >= 0);

   /* detect raw ustrncpy() call */
   if (size == INT_MAX)
      ansi_oddness = TRUE;

   size -= ucwidth(0);
   ASSERT(size >= 0);

   /* copy at most n characters */
   while (((c = ugetxc(&src)) != 0) && (len < n)) {
      size -= ucwidth(c);
      if (size<0)
         break;

      pos += usetc(dest+pos, c);
      len++;
   }

   /* pad with NULL characters */
   while (len < n) {
      size -= ucwidth(0);
      if (size < 0)
         break;

      pos += usetc(dest+pos, 0);
      len++;
   }

   /* ANSI C doesn't append the terminating NULL character */
   if (!ansi_oddness)
      usetc(dest+pos, 0);

   return dest;
}



/* ustrzncat:
 *  Enhanced Unicode-aware version of the ANSI strncat() function
 *  that can handle the size (in bytes) of the destination string.
 *  The raw Unicode-aware version of ANSI strncat() is defined as:
 *   #define ustrncat(dest, src, n) ustrzncat(dest, INT_MAX, src, n)
 */
char *ustrzncat(char *dest, int size, AL_CONST char *src, int n)
{
   int pos, len = 0;
   int c;
   ASSERT(dest);
   ASSERT(src);
   ASSERT(size >= 0);
   ASSERT(n >= 0);

   pos = ustrsize(dest);
   size -= pos + ucwidth(0);

   while (((c = ugetxc(&src)) != 0) && (len < n)) {
      size -= ucwidth(c);
      if (size<0)
         break;

      pos += usetc(dest+pos, c);
      len++;
   }

   usetc(dest+pos, 0);

   return dest;
}



/* ustrncmp:
 *  Unicode-aware version of the ANSI strncmp() function.
 */
int ustrncmp(AL_CONST char *s1, AL_CONST char *s2, int n)
{
   int c1, c2;
   ASSERT(s1);
   ASSERT(s2);

   if (n <= 0)
      return 0;

   for (;;) {
      c1 = ugetxc(&s1);
      c2 = ugetxc(&s2);

      if (c1 != c2)
	 return c1 - c2;

      if ((!c1) || (--n <= 0))
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



/* ustrrchr:
 *  Unicode-aware version of the ANSI strrchr() function.
 */
char *ustrrchr(AL_CONST char *s, int c)
{
   AL_CONST char *last_match = NULL;
   int c1, pos = 0;
   ASSERT(s);

   for (c1=ugetc(s); c1; c1=ugetc(s+pos)) {
      if (c1 == c)
	 last_match = s + pos;

      pos += ucwidth(c1);
   }

   return (char *)last_match;
}



/* ustrstr:
 *  Unicode-aware version of the ANSI strstr() function.
 */
char *ustrstr(AL_CONST char *s1, AL_CONST char *s2)
{
   int len;
   ASSERT(s1);
   ASSERT(s2);

   len = ustrlen(s2);
   while (ugetc(s1)) {
      if (ustrncmp(s1, s2, len) == 0)
	 return (char *)s1;

      s1 += uwidth(s1);
   }

   return NULL;
}



/* ustrpbrk:
 *  Unicode-aware version of the ANSI strpbrk() function.
 */
char *ustrpbrk(AL_CONST char *s, AL_CONST char *set)
{
   AL_CONST char *setp;
   int c, d;
   ASSERT(s);
   ASSERT(set);

   while ((c = ugetc(s)) != 0) {
      setp = set;

      while ((d = ugetxc(&setp)) != 0) {
	 if (c == d)
	    return (char *)s;
      }

      s += uwidth(s);
   }

   return NULL;
}



/* ustrtok_r:
 *  Unicode-aware version of the strtok_r() function.
 */
char *ustrtok_r(char *s, AL_CONST char *set, char **last)
{
   char *prev_str, *tok;
   AL_CONST char *setp;
   int c, sc;

   ASSERT(last);

   if (!s) {
      s = *last;

      if (!s)
	 return NULL;
   }

   skip_leading_delimiters:

   prev_str = s;
   c = ugetx(&s);

   setp = set;

   while ((sc = ugetxc(&setp)) != 0) {
      if (c == sc)
	 goto skip_leading_delimiters;
   }

   if (!c) {
      *last = NULL;
      return NULL;
   }

   tok = prev_str;

   for (;;) {
      prev_str = s;
      c = ugetx(&s);

      setp = set;

      do {
	 sc = ugetxc(&setp);
	 if (sc == c) {
	    if (!c) {
	       *last = NULL;
	       return tok;
	    }
	    else {
	       s += usetat(prev_str, 0, 0);
	       *last = s;
	       return tok;
	    }
	 }
      } while (sc);
   }
}



/* ustrerror:
 *  Fetch the error description from the OS and convert to Unicode.
 */
AL_CONST char *ustrerror(int err)
{
   AL_CONST char *s = strerror(err);
   static char tmp[1024];
   return uconvert_ascii(s, tmp);
}



/*******************************************************************/
/***             Unicode-aware sprintf() functions               ***/
/*******************************************************************/


/* information about the current format conversion mode */
typedef struct SPRINT_INFO
{
   int flags;
   int field_width;
   int precision;
   int num_special;
} SPRINT_INFO;



#define SPRINT_FLAG_LEFT_JUSTIFY             1
#define SPRINT_FLAG_FORCE_PLUS_SIGN          2
#define SPRINT_FLAG_FORCE_SPACE              4
#define SPRINT_FLAG_ALTERNATE_CONVERSION     8
#define SPRINT_FLAG_PAD_ZERO                 16
#define SPRINT_FLAG_SHORT_INT                32
#define SPRINT_FLAG_LONG_INT                 64
#define SPRINT_FLAG_LONG_DOUBLE              128
#define SPRINT_FLAG_LONG_LONG                256



/* decoded string argument type */
typedef struct STRING_ARG
{
   char *data;
   int size;  /* in bytes without the terminating '\0' */
   struct STRING_ARG *next;
} STRING_ARG;



/* LONGEST:
 *  64-bit integers on platforms that support it, 32-bit otherwise.
 */
#ifdef LONG_LONG
   #define LONGEST LONG_LONG
#else
   #define LONGEST long
#endif



/* va_int:
 *  Helper for reading an integer from the varargs list.
 */
#ifdef LONG_LONG

   #define va_int(args, flags)               \
   (                                         \
      ((flags) & SPRINT_FLAG_SHORT_INT) ?    \
	 va_arg(args, signed int)            \
      :                                      \
      (((flags) & SPRINT_FLAG_LONG_LONG) ?   \
	 va_arg(args, signed LONG_LONG)      \
      :                                      \
      (((flags) & SPRINT_FLAG_LONG_INT) ?    \
	 va_arg(args, signed long int)       \
      :                                      \
	 va_arg(args, signed int)))          \
   )

#else

   #define va_int(args, flags)               \
   (                                         \
      ((flags) & SPRINT_FLAG_SHORT_INT) ?    \
	 va_arg(args, signed int)            \
      :                                      \
      (((flags) & SPRINT_FLAG_LONG_INT) ?    \
	 va_arg(args, signed long int)       \
      :                                      \
	 va_arg(args, signed int))           \
   )

#endif



/* va_uint:
 *  Helper for reading an unsigned integer from the varargs list.
 */
#ifdef LONG_LONG

   #define va_uint(args, flags)              \
   (                                         \
      ((flags) & SPRINT_FLAG_SHORT_INT) ?    \
	 va_arg(args, unsigned int)          \
      :                                      \
      (((flags) & SPRINT_FLAG_LONG_LONG) ?   \
	 va_arg(args, unsigned LONG_LONG)    \
      :                                      \
      (((flags) & SPRINT_FLAG_LONG_INT) ?    \
	 va_arg(args, unsigned long int)     \
      :                                      \
	 va_arg(args, unsigned int)))        \
   )

#else

   #define va_uint(args, flags)              \
   (                                         \
      ((flags) & SPRINT_FLAG_SHORT_INT) ?    \
	 va_arg(args, unsigned int)          \
      :                                      \
      (((flags) & SPRINT_FLAG_LONG_INT) ?    \
	 va_arg(args, unsigned long int)     \
      :                                      \
	 va_arg(args, unsigned int))         \
   )

#endif



/* sprint_char:
 *  Helper for formatting (!) a character.
 */
static int sprint_char(STRING_ARG *string_arg, SPRINT_INFO *info, long val)
{
   int pos = 0;

   /* 1 character max for... a character */
   string_arg->data = _AL_MALLOC((MAX(1, info->field_width) * uwidth_max(U_UTF8)
                                                   + ucwidth(0)) * sizeof(char));

   pos += usetc(string_arg->data, val);

   string_arg->size = pos;
   usetc(string_arg->data+pos, 0);

   return 1;
}



/* sprint_i:
 *  Worker function for formatting integers.
 */
static int sprint_i(STRING_ARG *string_arg, unsigned LONGEST val, int precision)
{
   char tmp[24];  /* for 64-bit integers */
   int i = 0, pos = string_arg->size;
   int len;

   do {
      tmp[i++] = val % 10;
      val /= 10;
   } while (val);

   for (len=i; len<precision; len++)
      pos += usetc(string_arg->data+pos, '0');

   while (i > 0)
      pos += usetc(string_arg->data+pos, tmp[--i] + '0');

   string_arg->size = pos;
   usetc(string_arg->data+pos, 0);

   return len;
}



/* sprint_plus_sign:
 *  Helper to add a plus sign or space in front of a number.
 */
#define sprint_plus_sign(len)                              \
{                                                          \
   if (info->flags & SPRINT_FLAG_FORCE_PLUS_SIGN) {        \
      pos += usetc(string_arg->data+pos, '+');             \
      len++;                                               \
   }                                                       \
   else if (info->flags & SPRINT_FLAG_FORCE_SPACE) {       \
      pos += usetc(string_arg->data+pos, ' ');             \
      len++;                                               \
   }                                                       \
}



/* sprint_int:
 *  Helper for formatting a signed integer.
 */
static int sprint_int(STRING_ARG *string_arg, SPRINT_INFO *info, LONGEST val)
{
   int pos = 0, len = 0;

   /* 24 characters max for a 64-bit integer */
   string_arg->data = _AL_MALLOC((MAX(24, info->field_width) * uwidth_max(U_UTF8)
                                                    + ucwidth(0)) * sizeof(char));

   if (val < 0) {
      val = -val;
      pos += usetc(string_arg->data+pos, '-');
      len++;
   }
   else
      sprint_plus_sign(len);

   info->num_special = len;

   string_arg->size = pos;

   return sprint_i(string_arg, val, info->precision) +  info->num_special;
}



/* sprint_unsigned:
 *  Helper for formatting an unsigned integer.
 */
static int sprint_unsigned(STRING_ARG *string_arg, SPRINT_INFO *info, unsigned LONGEST val)
{
   int pos = 0;

   /* 24 characters max for a 64-bit integer */
   string_arg->data = _AL_MALLOC((MAX(24, info->field_width) * uwidth_max(U_UTF8)
                                                    + ucwidth(0)) * sizeof(char));

   sprint_plus_sign(info->num_special);

   string_arg->size = pos;

   return sprint_i(string_arg, val, info->precision) + info->num_special;
}



/* sprint_hex:
 *  Helper for formatting a hex integer.
 */
static int sprint_hex(STRING_ARG *string_arg, SPRINT_INFO *info, int caps, unsigned LONGEST val)
{
   static char hex_digit_caps[] = "0123456789ABCDEF";
   static char hex_digit[] = "0123456789abcdef";

   char tmp[24];  /* for 64-bit integers */
   char *table;
   int pos = 0, i = 0;
   int len;

   /* 24 characters max for a 64-bit integer */
   string_arg->data = _AL_MALLOC((MAX(24, info->field_width) * uwidth_max(U_UTF8)
                                                    + ucwidth(0)) * sizeof(char));

   sprint_plus_sign(info->num_special);

   if (info->flags & SPRINT_FLAG_ALTERNATE_CONVERSION) {
      pos += usetc(string_arg->data+pos, '0');
      pos += usetc(string_arg->data+pos, 'x');
      info->num_special += 2;
   }

   do {
      tmp[i++] = val & 15;
      val >>= 4;
   } while (val);

   for (len=i; len<info->precision; len++)
      pos += usetc(string_arg->data+pos, '0');

   if (caps)
      table = hex_digit_caps;
   else
      table = hex_digit;

   while (i > 0)
      pos += usetc(string_arg->data+pos, table[(int)tmp[--i]]);

   string_arg->size = pos;
   usetc(string_arg->data+pos, 0);

   return len + info->num_special;
}



/* sprint_octal:
 *  Helper for formatting an octal integer.
 */
static int sprint_octal(STRING_ARG *string_arg, SPRINT_INFO *info, unsigned LONGEST val)
{
   char tmp[24];  /* for 64-bit integers */
   int pos = 0, i = 0;
   int len;

   /* 24 characters max for a 64-bit integer */
   string_arg->data = _AL_MALLOC((MAX(24, info->field_width) * uwidth_max(U_UTF8)
                                                    + ucwidth(0)) * sizeof(char));

   sprint_plus_sign(info->num_special);

   if (info->flags & SPRINT_FLAG_ALTERNATE_CONVERSION) {
      pos += usetc(string_arg->data+pos, '0');
      info->num_special++;
   }

   do {
      tmp[i++] = val & 7;
      val >>= 3;
   } while (val);

   for (len=i; len<info->precision; len++)
      pos += usetc(string_arg->data+pos, '0');

   while (i > 0)
      pos += usetc(string_arg->data+pos, tmp[--i] + '0');

   string_arg->size = pos;
   usetc(string_arg->data+pos, 0);

   return len + info->num_special;
}



/* sprint_float:
 *  Helper for formatting a float (piggyback on the libc implementation).
 */
static int sprint_float(STRING_ARG *string_arg, SPRINT_INFO *info, double val, int conversion)
{
   char format[256], tmp[256];
   int len = 0, size;

   format[len++] = '%';

   if (info->flags & SPRINT_FLAG_LEFT_JUSTIFY)
      format[len++] = '-';

   if (info->flags & SPRINT_FLAG_FORCE_PLUS_SIGN)
      format[len++] = '+';

   if (info->flags & SPRINT_FLAG_FORCE_SPACE)
      format[len++] = ' ';

   if (info->flags & SPRINT_FLAG_ALTERNATE_CONVERSION)
      format[len++] = '#';

   if (info->flags & SPRINT_FLAG_PAD_ZERO)
      format[len++] = '0';

   if (info->field_width > 0)
      len += sprintf(format+len, "%d", info->field_width);

   if (info->precision >= 0)
      len += sprintf(format+len, ".%d", info->precision);

   format[len++] = conversion;
   format[len] = 0;

   len = sprintf(tmp, format, val);
   size = len * uwidth_max(U_UTF8) + ucwidth(0);

   string_arg->data = _AL_MALLOC(size * sizeof(char));

   do_uconvert(tmp, U_ASCII, string_arg->data, U_UTF8, size);

   info->field_width = 0;

   string_arg->size = ustrsize(string_arg->data);

   return len;
}



/* sprint_string:
 *  Helper for formatting a string.
 */
static int sprint_string(STRING_ARG *string_arg, SPRINT_INFO *info, AL_CONST char *s)
{
   int pos = 0, len = 0;
   int c;

   string_arg->data = _AL_MALLOC((MAX(ustrlen(s), info->field_width) * uwidth_max(U_UTF8)
                                                            + ucwidth(0)) * sizeof(char));

   while ((c = ugetxc(&s)) != 0) {
      if ((info->precision >= 0) && (len >= info->precision))
	 break;

      pos += usetc(string_arg->data+pos, c);
      len++;
   }

   string_arg->size = pos;
   usetc(string_arg->data+pos, 0);

   return len;
}



/* decode_format_string:
 *  Worker function for decoding the format string (with those pretty '%' characters)
 */
static int decode_format_string(char *buf, STRING_ARG *string_arg, AL_CONST char *format, va_list args)
{
   SPRINT_INFO info;
   int *pstr_pos;
   int done, slen, c, i, pos;
   int shift, shiftbytes, shiftfiller;
   int len = 0;

   while ((c = ugetxc(&format)) != 0) {

      if (c == '%') {
	 if ((c = ugetc(format)) == '%') {
	    /* percent sign escape */
	    format += uwidth(format);
            buf += usetc(buf, '%');
            buf += usetc(buf, '%');
            len++;
	 }
	 else {
	    /* format specifier */
	    #define NEXT_C()                 \
	    {                                \
	       format += uwidth(format);     \
	       c = ugetc(format);            \
	    }

	    /* set default conversion flags */
	    info.flags = 0;
	    info.field_width = 0;
	    info.precision = -1;
	    info.num_special = 0;

	    /* check for conversion flags */
	    done = FALSE;

	    do {
	       switch (c) {

		  case '-':
		     info.flags |= SPRINT_FLAG_LEFT_JUSTIFY;
		     NEXT_C();
		     break;

		  case '+':
		     info.flags |= SPRINT_FLAG_FORCE_PLUS_SIGN;
		     NEXT_C();
		     break;

		  case ' ':
		     info.flags |= SPRINT_FLAG_FORCE_SPACE;
		     NEXT_C();
		     break;

		  case '#':
		     info.flags |= SPRINT_FLAG_ALTERNATE_CONVERSION;
		     NEXT_C();
		     break;

		  case '0':
		     info.flags |= SPRINT_FLAG_PAD_ZERO;
		     NEXT_C();
		     break;

		  default:
		     done = TRUE;
		     break;
	       }

	    } while (!done);

	    /* check for a field width specifier */
	    if (c == '*') {
	       NEXT_C();
	       info.field_width = va_arg(args, int);
	       if (info.field_width < 0) {
		  info.flags |= SPRINT_FLAG_LEFT_JUSTIFY;
		  info.field_width = -info.field_width;
	       }
	    }
	    else if ((c >= '0') && (c <= '9')) {
	       info.field_width = 0;
	       do {
		  info.field_width *= 10;
		  info.field_width += c - '0';
		  NEXT_C();
	       } while ((c >= '0') && (c <= '9'));
	    }

	    /* check for a precision specifier */
	    if (c == '.')
	       NEXT_C();

	    if (c == '*') {
	       NEXT_C();
	       info.precision = va_arg(args, int);
	       if (info.precision < 0)
		  info.precision = 0;
	    }
	    else if ((c >= '0') && (c <= '9')) {
	       info.precision = 0;
	       do {
		  info.precision *= 10;
		  info.precision += c - '0';
		  NEXT_C();
	       } while ((c >= '0') && (c <= '9'));
	    }

	    /* check for size qualifiers */
	    done = FALSE;

	    do {
	       switch (c) {

		  case 'h':
		     info.flags |= SPRINT_FLAG_SHORT_INT;
		     NEXT_C();
		     break;

		  case 'l':
		     if (info.flags & SPRINT_FLAG_LONG_INT)
			info.flags |= SPRINT_FLAG_LONG_LONG;
		     else
			info.flags |= SPRINT_FLAG_LONG_INT;
		     NEXT_C();
		     break;

		  case 'L':
		     info.flags |= (SPRINT_FLAG_LONG_DOUBLE | SPRINT_FLAG_LONG_LONG);
		     NEXT_C();
		     break;

		  default:
		     done = TRUE;
		     break;
	       }

	    } while (!done);

	    /* format the data */
	    switch (c) {

	       case 'c':
		  /* character */
		  slen = sprint_char(string_arg, &info, va_arg(args, int));
		  NEXT_C();
		  break;

	       case 'd':
	       case 'i':
		  /* signed integer */
		  slen = sprint_int(string_arg, &info, va_int(args, info.flags));
		  NEXT_C();
		  break;

	       case 'D':
		  /* signed long integer */
		  slen = sprint_int(string_arg, &info, va_int(args, info.flags | SPRINT_FLAG_LONG_INT));
		  NEXT_C();
		  break;

	       case 'e':
	       case 'E':
	       case 'f':
	       case 'g':
	       case 'G':
		  /* double */
		  if (info.flags & SPRINT_FLAG_LONG_DOUBLE)
		     slen = sprint_float(string_arg, &info, va_arg(args, long double), c);
		  else
		     slen = sprint_float(string_arg, &info, va_arg(args, double), c);
		  NEXT_C();
		  break;

	       case 'n':
		  /* store current string position */
		  pstr_pos = va_arg(args, int *);
		  *pstr_pos = len;
		  slen = -1;
		  NEXT_C();
		  break;

	       case 'o':
		  /* unsigned octal integer */
		  slen = sprint_octal(string_arg, &info, va_uint(args, info.flags));
		  NEXT_C();
		  break;

	       case 'p':
		  /* pointer */
		  slen = sprint_hex(string_arg, &info, FALSE, (unsigned long)(va_arg(args, void *)));
		  NEXT_C();
		  break;

	       case 's':
		  /* string */
		  slen = sprint_string(string_arg, &info, va_arg(args, char *));
		  NEXT_C();
		  break;

	       case 'u':
		  /* unsigned integer */
		  slen = sprint_unsigned(string_arg, &info, va_uint(args, info.flags));
		  NEXT_C();
		  break;

	       case 'U':
		  /* unsigned long integer */
		  slen = sprint_unsigned(string_arg, &info, va_uint(args, info.flags | SPRINT_FLAG_LONG_INT));
		  NEXT_C();
		  break;

	       case 'x':
	       case 'X':
		  /* unsigned hex integer */
		  slen = sprint_hex(string_arg, &info, (c == 'X'), va_uint(args, info.flags));
		  NEXT_C();
		  break;

	       default:
		  /* weird shit... */
		  slen = -1;
		  break;
	    }

            if (slen >= 0) {
               if (slen < info.field_width) {
                  if (info.flags & SPRINT_FLAG_LEFT_JUSTIFY) {
                     /* left align the result */
                     pos = string_arg->size;
                     while (slen < info.field_width) {
                        pos += usetc(string_arg->data+pos, ' ');
                        slen++;
                     }

                     string_arg->size = pos;
                     usetc(string_arg->data+pos, 0);
                  }
                  else {
                     /* right align the result */
                     shift = info.field_width - slen;

                     if (shift > 0) {
                        pos = 0;

                        if (info.flags & SPRINT_FLAG_PAD_ZERO) {
                           shiftfiller = '0';

                           for (i=0; i<info.num_special; i++)
                              pos += uwidth(string_arg->data+pos);
                        }
                        else
                           shiftfiller = ' ';

                        shiftbytes = shift * ucwidth(shiftfiller);
                        memmove(string_arg->data+pos+shiftbytes, string_arg->data+pos, string_arg->size-pos+ucwidth(0));

                        string_arg->size += shiftbytes;
                        slen += shift;

                        for (i=0; i<shift; i++)
                           pos += usetc(string_arg->data+pos, shiftfiller);
                     }
                  }
               }

               buf += usetc(buf, '%');
               buf += usetc(buf, 's');
               len += slen;

               /* allocate next item */
               string_arg->next = _AL_MALLOC(sizeof(STRING_ARG));
               string_arg = string_arg->next;
               string_arg->next = NULL;
            }
	 }
      }
      else {
	 /* normal character */
	 buf += usetc(buf, c);
	 len++;
      }
   }

   usetc(buf, 0);

   return len;
}



/* uvszprintf:
 *  Enhanced Unicode-aware version of the ANSI vsprintf() function
 *  than can handle the size (in bytes) of the destination buffer.
 *  The raw Unicode-aware version of ANSI vsprintf() is defined as:
 *   #define uvsprintf(buf, format, args) uvszprintf(buf, INT_MAX, format, args)
 */
int uvszprintf(char *buf, int size, AL_CONST char *format, va_list args)
{
   char *decoded_format, *df;
   STRING_ARG *string_args, *iter_arg;
   int c, len;
   ASSERT(buf);
   ASSERT(size >= 0);
   ASSERT(format);

   /* decoding can only lower the length of the format string */
   df = decoded_format = _AL_MALLOC_ATOMIC(ustrsizez(format) * sizeof(char));

   /* allocate first item */
   string_args = _AL_MALLOC(sizeof(STRING_ARG));
   string_args->next = NULL;

   /* 1st pass: decode */
   len = decode_format_string(decoded_format, string_args, format, args);

   size -= ucwidth(0);
   iter_arg = string_args;

   /* 2nd pass: concatenate */
   while ((c = ugetx(&decoded_format)) != 0) {

      if (c == '%') {
	 if ((c = ugetx(&decoded_format)) == '%') {
	    /* percent sign escape */
            size -= ucwidth('%');
            if (size<0)
               break;
	    buf += usetc(buf, '%');
	 }
	 else if (c == 's') {
            /* string argument */
            ustrzcpy(buf, size+ucwidth(0), iter_arg->data);
            buf += iter_arg->size;
            size -= iter_arg->size;
            if (size<0) {
               buf += size;
               break;
            }
            iter_arg = iter_arg->next;
         }
      }
      else {
	 /* normal character */
         size -= ucwidth(c);
         if (size<0)
            break;
	 buf += usetc(buf, c);
      }
   }

   usetc(buf, 0);

   /* free allocated resources */
   while (string_args->next) {
      _AL_FREE(string_args->data);
      iter_arg = string_args;
      string_args = string_args->next;
      _AL_FREE(iter_arg);
   }
   _AL_FREE(string_args);
   _AL_FREE(df);  /* alias for decoded_format */

   return len;
}



/* uszprintf:
 *  Enhanced Unicode-aware version of the ANSI sprintf() function
 *  that can handle the size (in bytes) of the destination buffer.
 */
int uszprintf(char *buf, int size, AL_CONST char *format, ...)
{
   int ret;
   va_list ap;
   ASSERT(buf);
   ASSERT(size >= 0);
   ASSERT(format);

   va_start(ap, format);
   ret = uvszprintf(buf, size, format, ap);
   va_end(ap);

   return ret;
}

