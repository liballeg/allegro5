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
 *      Grabber datafile -> C converter for the Allegro library.
 *
 *      By Laurence Withers.
 *
 *      Christer Sandberg made it work with ISO C90 compilers and
 *      fixed a couple of other problems.
 *
 *      See readme.txt for copyright information.
 */



/* Additional notes:
 *
 *  - All C symbols are converted to lowercase, and all macro names to
 *    uppercase.
 *
 *  - Packfile passwords are not supported since the converted data will
 *    be stored unencrypted in any case.
 *
 *  - The include guard symbol is generated from the current date and
 *    time alone.
 */



/* system includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>



/* Allegro includes */
#define ALLEGRO_USE_CONSOLE
#include "allegro.h"
#include "allegro/internal/aintern.h"



/* out_of_memory()
 *
 *  A macro which prints an error message stating out of memory and
 *  aborts.
 */
#define out_of_memory() \
    do { \
        fprintf(stderr, "out of memory\n"); \
        abort(); \
    } while(0)



/* enum lineformat
 *
 *  Specifies which lineformat to use (DOS, Mac, Unix).
 */
enum lineformat {
   lineformat_default,
   lineformat_mac,
   lineformat_dos,
   lineformat_unix
};



/* struct dat2c
 *
 *  This structure holds all the information and data required to carry
 *  out a conversion.
 */
struct dat2c {
   const char *fname_dat;	/* datafile filename */
   const char *fname_c;		/* C source output filename */
   const char *fname_h;		/* C header output filename */

   enum lineformat lformat;	/* which line ending format to use */

   int global_symbols;		/* nonzero if objects should be global */
   int convert_compiled_sprites;	/* CSPRITE -> BITMAP conversion */

   const char *prefix;		/* macro name prefix */
   const char *datafile_name;	/* datafile object name */

   DATAFILE *dat;		/* the loaded datafile */
   FILE *fp_c;			/* the C output file descriptor */
   FILE *fp_h;			/* the C header file descriptor */
};

struct dat2c *malloc_dat2c(void);
void free_dat2c(struct dat2c *);



/* do_conversion()
 *
 *  Given a `struct dat2c' object, this function actually carries out the
 *  conversion into C source code. Returns 0 on success.
 */
int do_conversion(struct dat2c *);



/* struct dat2c_converter
 *
 *  Represents a conversion for a single object type.
 */
struct dat2c_converter {
   int type;			/* from DAT_ID */
   int (*cvt) (struct dat2c *, DATAFILE *, const char *);
   /* the dat2c object, a pointer to the DATAFILE
      element (not array) to export, the name to use;
      return 0 on success */
};



/* invalid_macro_name()
 *
 *  Returns 0 if the macro name is valid, else non zero.
 */
static int invalid_macro_name(const char *n)
{
   const char *m = 0;
   if (*n != '_' && !isalpha((int)*n))
      return 1;
   for (m = n + 1; *m; m++)
      if (*m != '_' && !isalnum((int)*m))
	 return 1;
   return 0;
}



/* interpret_commandline()
 *
 *  Reads options on the commandline, filling out `dat2c' appropriately.
 *  Returns 0 on success.
 */
static INLINE int is_arg(const char *arg, const char *t1, const char *t2)
{
   return !(strcmp(arg, t1) && strcmp(arg, t2));
}

static int
interpret_commandline(struct dat2c *dat2c, int argc, char **argv)
{
   int i = 0;

   for (i = 1; i < argc; i++) {

      /* --cfile: C source file output */
      if (is_arg(argv[i], "-o", "--cfile")) {
	 if (++i == argc) {
	    fprintf(stderr, "Error: no filename after %s", argv[i - 1]);
	    return 1;
	 }
	 if (dat2c->fname_c) {
	    fprintf(stderr, "Error: C source file specified twice ("
		    "as `%s' and `%s').\n", dat2c->fname_c, argv[i]);
	    return 1;
	 }
	 dat2c->fname_c = argv[i];
	 continue;
      }

      /* --hfile: C header file output */
      if (is_arg(argv[i], "-h", "--hfile")) {
	 if (++i == argc) {
	    fprintf(stderr, "Error: no filename after %s", argv[i - 1]);
	    return 1;
	 }
	 if (dat2c->fname_h) {
	    fprintf(stderr, "Error: C header file specified twice ("
		    "as `%s' and `%s').\n", dat2c->fname_h, argv[i]);
	    return 1;
	 }
	 dat2c->fname_h = argv[i];
	 continue;
      }

      /* --crlf, -C: DOS line endings */
      if (is_arg(argv[i], "-C", "--crlf")) {
	 if (dat2c->lformat != lineformat_default) {
	    fprintf(stderr, "Error: line format specified twice.\n");
	    return 1;
	 }
	 dat2c->lformat = lineformat_dos;
	 continue;
      }

      /* --mac, -M: mac line endings */
      if (is_arg(argv[i], "-M", "--mac")) {
	 if (dat2c->lformat != lineformat_default) {
	    fprintf(stderr, "Error: line format specified twice.\n");
	    return 1;
	 }
	 dat2c->lformat = lineformat_mac;
	 continue;
      }

      /* --unix, -U: Unix line endings */
      if (is_arg(argv[i], "-U", "--unix")) {
	 if (dat2c->lformat != lineformat_default) {
	    fprintf(stderr, "Error: line format specified twice.\n");
	    return 1;
	 }
	 dat2c->lformat = lineformat_unix;
	 continue;
      }

      /* --prefix, -p: macro name prefix */
      if (is_arg(argv[i], "-p", "--prefix")) {
	 if (++i == argc) {
	    fprintf(stderr, "Error: no name after %s", argv[i - 1]);
	    return 1;
	 }
	 if (dat2c->prefix) {
	    fprintf(stderr, "Error: prefix specified twice (as `%s' "
		    "and `%s').\n", dat2c->prefix, argv[i]);
	    return 1;
	 }
	 if (invalid_macro_name(argv[i])) {
	    fprintf(stderr, "Error: `%s' is not a valid macro "
		    "prefix.\n", argv[i]);
	    return 1;
	 }
	 dat2c->prefix = argv[i];
	 continue;
      }

      /* --datafile-name, -n: datafile object name */
      if (is_arg(argv[i], "-n", "--datafile-name")) {
	 if (++i == argc) {
	    fprintf(stderr, "Error: no name after %s", argv[i - 1]);
	    return 1;
	 }
	 if (dat2c->datafile_name) {
	    fprintf(stderr, "Error: datafile name specified twice ("
		    "as `%s' and `%s').\n", dat2c->datafile_name, argv[i]);
	    return 1;
	 }
	 if (invalid_macro_name(argv[i])) {
	    fprintf(stderr, "Error: `%s' is not a valid C "
		    "identifier.\n", argv[i]);
	    return 1;
	 }
	 dat2c->datafile_name = argv[i];
	 continue;
      }

      /* --global, -g: globally visible objects */
      if (is_arg(argv[i], "-g", "--global")) {
	 if (dat2c->global_symbols) {
	    fprintf(stderr, "Error: global symbols specified " "twice.\n");
	    return 1;
	 }
	 dat2c->global_symbols = 1;
	 continue;
      }

      /* --convert-compiled-sprites, -S: convert compiled sprite
         objects to BITMAPs */
      if (is_arg(argv[i], "-S", "--convert-compiled-sprites")) {
	 if (dat2c->convert_compiled_sprites) {
	    fprintf(stderr, "Error: conversion of compiled sprites "
		    "specified twice.\n");
	    return 1;
	 }
	 dat2c->convert_compiled_sprites = 1;
	 continue;
      }


      /* .dat file input */
      if (dat2c->fname_dat) {
	 fprintf(stderr, "Error: datafile specified twice (as "
		 "`%s' and `%s').\n", dat2c->fname_dat, argv[i]);
	 return 1;
      }
      dat2c->fname_dat = argv[i];
   }

   /* sanity checking */
   if (!dat2c->fname_dat) {
      fprintf(stderr, "Error: datafile is not specified.\n");
      return 1;
   }

   /* all is in order */
   return 0;
}



/* prepare_dat2c()
 *
 *  Prepares a `dat2c' that has been filled out from the commandline.
 *  This involves opening files, etc. Returns 0 on success.
 */
static int prepare_dat2c(struct dat2c *dat2c)
{
   dat2c->dat = load_datafile(dat2c->fname_dat);
   if (!dat2c->dat) {
      fprintf(stderr, "Error loading datafile `%s'.\n", dat2c->fname_dat);
      return 1;
   }

   if (dat2c->fname_c) {
      dat2c->fp_c = fopen(dat2c->fname_c, "wb");
      if (!dat2c->fp_c) {
	 fprintf(stderr, "Error writing C source file to `%s'.\n",
		 dat2c->fname_c);
	 return 1;
      }
   }
   else {
      dat2c->fp_c = stdout;
      dat2c->fname_c = "(stdout)";
   }

   if (dat2c->fname_h) {
      dat2c->fp_h = fopen(dat2c->fname_h, "wb");
      if (!dat2c->fp_h) {
	 fprintf(stderr, "Error writing C header file to `%s'.\n",
		 dat2c->fname_h);
	 return 1;
      }
   }

   if (dat2c->lformat == lineformat_default)
#if (defined ALLEGRO_UNIX || defined ALLEGRO_QNX || defined ALLEGRO_BEOS || defined ALLEGRO_HAIKU || defined ALLEGRO_MACOSX)
      dat2c->lformat = lineformat_unix;
#elif (defined ALLEGRO_WINDOWS || defined ALLEGRO_DOS)
      dat2c->lformat = lineformat_dos;
#elif defined ALLEGRO_MPW
      dat2c->lformat = lineformat_mac;
#else
#error platform not supported
#endif

      return 0;
}



/* show_usage()
 *
 *  Shows the usage information.
 */
static void show_usage(void)
{
   printf("\nDatafile -> C conversion utility for Allegro "
	  ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR "\n");
   printf("By Laurence Withers, " ALLEGRO_DATE_STR "\n\n");
   printf("Usage:\n"
	  "  dat2c [options] inputfile.dat \n\n"
	  "Options:\n"
	  "  -o --cfile <filename>\n"
	  "    Specifies output filename (default stdout)\n"
	  "  -h --hfile <filename>\n"
	  "    Specifies header filename (default none)\n"
	  "  -p --prefix <C identifier>\n"
	  "    Specifies declaration prefix (default none)\n"
	  "  -C --crlf, -U --unix, -M --mac\n"
	  "    Specifies line ending (default: the platform default)\n"
	  "  -g --global\n"
	  "    Makes each object globally visible (default don't)\n"
	  "  -S --convert-compiled-sprites\n"
	  "    Converts COMPILED_SPRITEs to BITMAPs (default abort)\n"
	  "  -n --datafile-name <C identifier>\n"
	  "    Gives a name for the datafile (default 'data')\n");
}



/* setup_allegro()
 *
 *  Sets up Allegro, color conversion, etc. Returns 0 on success.
 */
static int setup_allegro(void)
{
   if (install_allegro(SYSTEM_NONE, &errno, atexit)) {
      fprintf(stderr, "Allegro failed to initialise.\n");
      return 1;
   }

   set_color_conversion(COLORCONV_NONE);

   /* _compile_sprites is an internal Allegro symbol. Setting it to 0
      will cause load_datafile() to not compile sprites, and to load
      them as bitmaps instead. */
   _compile_sprites = 0;

   return 0;
}



/* main()
 */
static int truecolor = FALSE;

int main(int argc, char *argv[])
{
   struct dat2c *dat2c = malloc_dat2c();
   int result = 0;

   if (argc == 1 || interpret_commandline(dat2c, argc, argv)) {
      show_usage();
      free_dat2c(dat2c);
      return 1;
   }

   if (setup_allegro() || prepare_dat2c(dat2c)) {
      free_dat2c(dat2c);
      return 1;
   }

   if (dat2c->fname_c)
      printf("Converting %s to %s...\n", dat2c->fname_dat, dat2c->fname_c);

   result = do_conversion(dat2c);
   free_dat2c(dat2c);

#ifdef ALLEGRO_USE_CONSTRUCTOR

   if (truecolor) {
      printf
	  ("\nI noticed some truecolor images, so you must call fixup_datafile()\n");
      printf("before using this data! (after setting a video mode).\n");
   }

#else

   printf
       ("\nI don't know how to do constructor functions on this platform, so you must\n");
   printf
       ("call fixup_datafile() before using this data! (after setting a video mode).\n");

#endif

   return result;
}

END_OF_MAIN()
/* malloc_dat2c()
 *
 *  Allocates the space required for a struct dat2c, aborting the program
 *  if the allocation fails. Fills all fields with 0, and returns a
 *  pointer to the newly allocated object.
 */
struct dat2c *malloc_dat2c(void)
{
   struct dat2c *d = malloc(sizeof(struct dat2c));
   if (!d)
      out_of_memory();
   memset(d, 0, sizeof(struct dat2c));
   return d;
}



/* free_dat2c()
 *
 *  Frees any data held by a dat2c object, and the object itself.
 */
void free_dat2c(struct dat2c *dat2c)
{
   /* close files */
   if (dat2c->dat)
      unload_datafile(dat2c->dat);
   if (dat2c->fp_c)
      fclose(dat2c->fp_c);
   if (dat2c->fp_h)
      fclose(dat2c->fp_h);

   /* free object */
   free(dat2c);
}



/* cwrite()
 *
 *  A function in the style of fprintf, this uses $seq$ as its escape
 *  sequence, and recognises the following sequences:
 *
 *   $$               Single '$' character (no parameter)
 *   $n$              Newline (no parameter)
 *
 *   $int$            Integer (parameter 'int')
 *   $uint$           Unsigned integer (parameter 'unsigned int')
 *   $long$           Long (paramter 'long')
 *   $ulong$          Unsigned long (parameter 'unsigned long')
 *   $int hex$        Integer (parameter 'int'), in hex notation (no 0x).
 *
 *   $dat_id$         Writes an Allegro DAT_ID number in human-readable
 *                      format, if possible (parameter 'int').
 *
 *   $string$         Write a verbatim string (parameter 'char*')
 *   $string c$       Write a string, but escape it as a C string literal
 *                      (parameter 'char*')
 *   $string upper$   Write a string, but convert it to uppercase.
 *                      (parameter 'char*')
 *   $string lower$   Write a string, but convert it to lowercase.
 *                      (parameter 'char*')
 *
 *   $data$           Write a block of memory as an escaped string
 *                      literal (parameter 'unsigned int', for size,
 *                      'unsigned int', for offset in spaces, and
 *                      'unsigned char*', for data).
 *
 *  Both $string c$ and $data$ enclose their data in double quotes.
 */
static void _cwrite_esc_char(FILE * fp, int ch)
{
   putc('\\', fp);
   switch (ch) {
      case 0:
	 putc('0', fp);
	 break;

      case 7:
	 putc('a', fp);
	 break;

      case 8:
	 putc('b', fp);
	 break;

      case 9:
	 putc('t', fp);
	 break;

      case 10:
	 putc('n', fp);
	 break;

      case 11:
	 putc('v', fp);
	 break;

      case 12:
	 putc('f', fp);
	 break;

      case 13:
	 putc('r', fp);
	 break;

      default:
	 fprintf(fp, "x%x", ch);
	 break;
   }
}

static void _cwrite_esc(FILE * fp, const unsigned char *data, int amt)
{
   const unsigned char *rd = 0;
   int was_num_escape = 0, was_trigraph_char = 0;

   putc('"', fp);
   for (rd = data; amt; amt--) {
      switch (*rd) {
	 case 0:
	    was_num_escape = 2;

	 case 7:		/* \a or bell */
	 case 8:		/* backspace */
	 case 9:		/* tab */
	 case 10:		/* LF */
	 case 11:		/* vertical tab */
	 case 12:		/* form feed */
	 case 13:		/* CR */
	    _cwrite_esc_char(fp, *rd);
	    break;

	 case '"':		/* char needs to be escaped */
	 case '\\':
	    putc('\\', fp);
	    putc(*rd, fp);
	    break;

	 case '?':
	    if (was_trigraph_char)
	       putc('\\', fp);
	    was_trigraph_char = 2;
	    putc('?', fp);
	    break;

	 default:
	    if (*rd < 32 || *rd >= 127) {
	       _cwrite_esc_char(fp, *rd);
	       was_num_escape = 2;
	    }
	    else {
	       if (was_num_escape && isxdigit(*rd)) {
		  /* quick fix to stop digits from being
		   * interpreted as hex/octal escapes */
		  putc('"', fp);
		  putc('"', fp);
	       }
	       putc(*rd, fp);
	    }
      }
      if (was_num_escape)
	 was_num_escape--;
      if (was_trigraph_char)
	 was_trigraph_char--;
      rd++;
   }
   putc('"', fp);
}

enum dat2c_file { C, H };

static void
cwrite(struct dat2c *dat2c, enum dat2c_file x, const char *fmt, ...)
{
   FILE *fp = 0;
   const char *rd = 0;
   const char *start = 0;
   va_list va;

   va_start(va, fmt);

   switch (x) {
      case C:
	 fp = dat2c->fp_c;
	 break;
      case H:
	 fp = dat2c->fp_h;
	 break;
   }
   if (!fp || !fmt)
      return;

   start = rd = fmt;
   while (*rd) {
      /* handle escapes */
      if (*rd == '$') {
	 if (!strncmp(rd, "$$", 2)) {
	    /* single '$' character */

	    putc('$', fp);

	 }
	 else if (!strncmp(rd, "$n$", 3)) {
	    /* newline sequence */

	    switch (dat2c->lformat) {
	       case lineformat_dos:
		  putc(13, fp);
		  putc(10, fp);
		  break;

	       case lineformat_mac:
		  putc(13, fp);
		  break;

	       default:
		  putc(10, fp);
		  break;
	    }

	 }
	 else if (!strncmp(rd, "$int$", 5)) {
	    /* plain integer */

	    int i = va_arg(va, int);
	    fprintf(fp, "%d", i);

	 }
	 else if (!strncmp(rd, "$uint$", 6)) {
	    /* unsigned integer */

	    unsigned int u = va_arg(va, unsigned int);
	    fprintf(fp, "%u", u);

	 }
	 else if (!strncmp(rd, "$long$", 6)) {
	    /* plain long integer */

	    long l = va_arg(va, long);
	    fprintf(fp, "%ld", l);

	 }
	 else if (!strncmp(rd, "$ulong$", 7)) {
	    /* unsigned long integer */

	    unsigned long ul = va_arg(va, unsigned long);
	    fprintf(fp, "%lu", ul);

	 }
	 else if (!strncmp(rd, "$int hex$", 9)) {
	    /* hexadecimal integer */

	    int i = va_arg(va, int);
	    fprintf(fp, "%x", i);

	 }
	 else if (!strncmp(rd, "$dat_id$", 8)) {
	    /* Allegro DAT_ID() */

	    int dat_id = va_arg(va, int);
	    int c[4];
	    int i = 0;

	    c[0] = (dat_id >> 24) & 255;
	    c[1] = (dat_id >> 16) & 255;
	    c[2] = (dat_id >> 8) & 255;
	    c[3] = dat_id & 255;

	    fputs("DAT_ID('", fp);
	    for (i = 0; i < 4; i++) {
	       if (c[i] == '\'' || c[i] == '\\')
		  putc('\\', fp);
	       if (c[i] < 32)
		  _cwrite_esc_char(fp, c[i]);
	       else
		  putc(c[i], fp);

	       if (i == 3) {
		  fputs("')", fp);
	       }
	       else {
		  fputs("', '", fp);
	       }
	    }

	 }
	 else if (!strncmp(rd, "$string$", 8)) {
	    /* verbatim string */

	    const char *s = va_arg(va, const char *);
	    fputs(s, fp);

	 }
	 else if (!strncmp(rd, "$string c$", 10)) {
	    /* escaped C string */

	    const unsigned char *s = va_arg(va,
					    const unsigned char *);
	    _cwrite_esc(fp, s, strlen((const char *)s));

	 }
	 else if (!strncmp(rd, "$string lower$", 14)) {
	    /* lowercase verbatim string */

	    const char *s = va_arg(va, const char *);
	    const char *p = 0;
	    for (p = s; *p; p++)
	       putc(tolower(*p), fp);

	 }
	 else if (!strncmp(rd, "$string upper$", 14)) {
	    /* uppercase verbatim string */

	    const char *s = va_arg(va, const char *);
	    const char *p = 0;
	    for (p = s; *p; p++)
	       putc(toupper(*p), fp);

	 }
	 else if (!strncmp(rd, "$data$", 6)) {
	    /* data block */

	    unsigned int size, offset, remaining;
	    const unsigned char *data = 0;

	    size = va_arg(va, unsigned int);
	    offset = va_arg(va, unsigned int);
	    data = va_arg(va, const unsigned char *);

	    remaining = size;
	    while (1) {
	       int sp;

	       if (remaining <= 50) {
		  _cwrite_esc(fp, data, remaining);
		  break;
	       }

	       _cwrite_esc(fp, data, 50);
	       remaining -= 50;
	       data += 50;

	       switch (dat2c->lformat) {
		  case lineformat_dos:
		     putc(13, fp);
		     putc(10, fp);
		     break;

		  case lineformat_mac:
		     putc(13, fp);
		     break;

		  default:
		     putc(10, fp);
		     break;
	       }
	       for (sp = offset; sp; sp--)
		  putc(' ', fp);
	    }

	 }
	 else {
	    /* error */

	    fprintf(stderr, "Error in format string `%s'.\n", fmt);
	    abort();
	 }

	 /* advance past terminating $ symbol */
	 for (rd++; *rd != '$'; rd++) ;
	 rd++;
      }

      /* write out non-escape stuff to disk */
      start = rd;
      for (; *rd && *rd != '$'; rd++) ;
      if (rd - start)
	 fwrite(start, 1, rd - start, fp);
   }

   va_end(va);
}



/* get_c_identifier()
 *
 *  Given a datafile object, gets a valid C identifier. We first try to
 *  compose this from the name, but if this is no use then we compose it
 *  from the type. In addition, we cache all the created names in an
 *  associative array, so calling the function twice with the same
 *  pointer always returns the same string.
 *
 *  Call clear_c_identifiers() to clear the cache. Returns a pointer to
 *  the allocated name. Always succeeds (calls out_of_memory()).
 */
struct cident_entry {
   char *name;
   DATAFILE *obj;
   struct cident_entry *next;
};

static struct cident_entry *first_cident = 0;

static const char *new_cident(const char *prefix, const char *name,
			      DATAFILE *obj)
{
   struct cident_entry *c = malloc(sizeof(struct cident_entry));
   if (!c)
      out_of_memory();

   c->name = malloc(strlen(prefix) + strlen(name) + 2);
   if (!c->name)
      out_of_memory();
   sprintf(c->name, "%s%s%s", prefix, *prefix ? "_" : "", name);
   c->obj = obj;
   c->next = first_cident;
   first_cident = c;

   return c->name;
}

static const char *get_c_identifier(const char *prefix, DATAFILE *obj)
{
   static unsigned int tmp_name_count = 0;
   struct cident_entry *iter = 0;
   const char *prop_name = 0;

   /* first, check the cache */
   for (iter = first_cident; iter; iter = iter->next)
      if (iter->obj == obj)
	 return iter->name;

   /* Create a new name: first, check if the datafile object name is a
      valid C identifier; if not, create a new name based on the object
      type and the number of previous names */
   prop_name = get_datafile_property(obj, DAT_ID('N', 'A', 'M', 'E'));

   if (prop_name && strlen(prop_name)) {
      int i;
      for (i = 1; prop_name[i]; i++) {
	 if (!isalnum((int)prop_name[i]) && prop_name[i] != '_')
	    break;
      }
      if ((*prop_name == '_' || isalpha((int)*prop_name)) && !prop_name[i]) {
	 return new_cident(prefix, prop_name, obj);
      }
   }

   /* property name not valid, so generate one */
   {
      char tmp_name[16];	/* TYPE_32bitnum */
      int c1 = (obj->type >> 24) & 255;
      int c2 = (obj->type >> 16) & 255;
      int c3 = (obj->type >> 8) & 255;
      int c4 = obj->type & 255;

      if ((isalpha(c1) || c1 == '_') &&
	  (isalnum(c2) || c2 == '_') &&
	  (isalnum(c3) || c3 == '_') &&
	  (isalnum(c4) || c4 == '_' || c4 == ' ')) {
	 tmp_name[0] = c1;
	 tmp_name[1] = c2;
	 tmp_name[2] = c3;
	 tmp_name[3] = c4 == ' ' ? '_' : c4;
      }
      else {
	 tmp_name[0] = 'O';
	 tmp_name[1] = 'B';
	 tmp_name[2] = 'J';
	 tmp_name[3] = '_';
      }

      sprintf(tmp_name + (tmp_name[3] == '_' ? 3 : 4),
	      "_%u", tmp_name_count++);

      return new_cident(prefix, tmp_name, obj);
   }

   return 0;			/* to stop compiler warning */
}



static void clear_c_identifiers(void)
{
   struct cident_entry *i = 0, *next = 0;
   for (i = first_cident; i; i = next) {
      next = i->next;
      free(i->name);
      free(i);
   }
   first_cident = 0;
}



/* make_single_cident()
 *
 *  Creates a valid cident from a string. The returned
 *  pointer is dynamically allocated and must be freed.
 */
static char *make_single_cident(const char *src)
{
   char *cident, *p;
   int c;

   p = cident = malloc(strlen(src) + 1);
   if (!cident)
      out_of_memory();

   if ((c = *src++)) {
      if (isalpha(c))
	 *p++ = toupper(c);
      else
	 *p++ = '_';

      while ((c = *src++)) {
	 if (isalnum(c))
	    *p++ = toupper(c);
	 else
	    *p++ = '_';
      }
   }

   *p = 0;			/* terminating null character */

   return cident;
}



/* write_header_start()
 *
 *  Writes the start of the header file.
 */
static void write_header_start(struct dat2c *dat2c)
{
   char *dat2c_include_guard = make_single_cident(dat2c->fname_h);

   /* comment at top of file */
   cwrite(dat2c, H, "/* $string$$n$"
	  " * $n$"
	  " *  Converted datafile header for $string$ .$n$"
	  " *  See $string$ for definitions.$n$"
	  " *  Do not hand edit.$n$"
	  " */$n$"
	  "$n$"
	  "#ifndef ALLEGRO_H$n$"
	  "#error You must include allegro.h$n$"
	  "#endif$n$"
	  "$n$"
	  "#ifndef $string$$n$"
	  "#define $string$$n$"
	  "$n$"
	  "#ifdef __cplusplus$n$"
	  "extern \"C\" {$n$"
	  "#endif$n$"
	  "$n$$n$$n$",
	  dat2c->fname_h,
	  dat2c->fname_dat,
	  dat2c->fname_c, dat2c_include_guard, dat2c_include_guard);

   free(dat2c_include_guard);
}



/* write_header_end()
 *
 *  Writes the end of the header file.
 */
static void write_header_end(struct dat2c *dat2c)
{
   /* end c++ / include guard */
   cwrite(dat2c, H, "#ifdef __cplusplus$n$"
	  "}$n$"
	  "#endif$n$"
	  "$n$"
	  "#endif /* include guard */$n$"
	  "$n$" "/* end of $string$ */$n$" "$n$$n$$n$", dat2c->fname_h);
}



/* write_source_start()
 *
 *  Writes the start of the source file.
 */
static void write_source_start(struct dat2c *dat2c)
{
   /* comment at top of file */
   cwrite(dat2c, C, "/* $string$$n$"
	  " * $n$"
	  " *  Source for data encoded from $string$ .$n$"
	  " *  Do not hand edit.$n$"
	  " */$n$"
	  "$n$"
	  "#include <allegro.h>$n$"
	  "#include <allegro/internal/aintern.h>$n$"
	  "$n$$n$$n$", dat2c->fname_c, dat2c->fname_dat);
}



/* write_source_end()
 *
 *  Writes the end of the source file.
 */
static void write_source_end(struct dat2c *dat2c, const char *dat_name)
{
   /* emit a constructor */
   cwrite(dat2c, C, "#ifdef ALLEGRO_USE_CONSTRUCTOR$n$"
	  "CONSTRUCTOR_FUNCTION("
	  "static void _construct_me(void));$n$"
	  "static void _construct_me(void)$n$"
	  "{$n$"
	  "    _construct_datafile($string lower$);$n$"
	  "}$n$" "#endif$n$" "$n$$n$$n$", dat_name);

   /* comment at bottom of file */
   cwrite(dat2c, C, "/* end of $string$ */$n$" "$n$$n$$n$",
	  dat2c->fname_c);
}



/* cvt_FILE()
 *
 *  Writes a nested datafile object.
 */
static int write_datafile(struct dat2c *, DATAFILE *, const char *,
			  const char *);
static int cvt_FILE(struct dat2c *dat2c, DATAFILE *dat, const char *name)
{
   /* the header declaration is actually carried out in write_datafile()
      automatically. */

   /* write out the nested datafile */
   return write_datafile(dat2c, dat->dat, name, name);
}



/* cvt_BITMAP, cvt_BITMAP_aux
 *
 *  Writes a BITMAP object.
 */
static int
cvt_BITMAP_aux(struct dat2c *dat2c, BITMAP *bmp,
	       const char *name, int export)
{
   int bpp = bitmap_color_depth(bmp);
   int bypp = (bpp + 7) / 8;
   int y = 0;

   if (bpp > 8)
      truecolor = TRUE;

   /* declare this in the header if we are exporting objects */
   if (export) {
      cwrite(dat2c, H, "extern BITMAP $string lower$;$n$$n$", name);
   }

   /* write out lines data (we actually write it as one big chunk) */
   cwrite(dat2c, C, "static unsigned char $string lower$_lines[] = $n$",
	  name);

   for (y = 0; y < bmp->h; y++) {
      cwrite(dat2c, C, "$data$$n$", bmp->w * bypp, 4, bmp->line[y]);
   }

   cwrite(dat2c, C, ";$n$$n$");

   /* write out BITMAP structure */

   cwrite(dat2c, C, "$string$struct { int w, h; int clip; int cl, cr, ct, cb;\n" "                GFX_VTABLE *vtable; void *write_bank; void *read_bank;\n" "                void *dat; unsigned long id; void *extra;\n" "                int x_ofs; int y_ofs; int seg; unsigned char *line[$int$]; } $string lower$ = {$n$" "    $int$, $int$, /* width, height */$n$" "    0, 0, 0, 0, 0, /* clip */$n$" "    (GFX_VTABLE *)$int$, /* bpp */$n$" "    0, 0, /* write_bank, read_bank */$n$" "    $string lower$_lines, /* data */$n$" "    0, 0, 0, 0, 0,$n$" "    { /* line[] array */$n$", export ? "" : "static ", bmp->h + 1,	/* trailing 0 for convenience */
	  name, bmp->w, bmp->h, bpp, name);

   /* write list of lines */
   for (y = 0; y < bmp->h; y++) {
      cwrite(dat2c, C, "      $string lower$_lines + $int$,$n$",
	     name, y * bmp->w * bypp);
   }

   /* close BITMAP structure */
   cwrite(dat2c, C, "      0 }$n$" "};$n$");

   return 0;
}

static int cvt_BITMAP(struct dat2c *dat2c, DATAFILE *dat, const char *name)
{
   int result = 0;
   result = cvt_BITMAP_aux(dat2c, dat->dat, name, dat2c->global_symbols);
   cwrite(dat2c, C, "$n$$n$$n$");
   return result;
}



/* cvt_DATA()
 *
 *  Converts some binary data.
 */
static int cvt_DATA(struct dat2c *dat2c, DATAFILE *dat, const char *name)
{
   /* declare this in the header if we are exporting objects */
   if (dat2c->global_symbols) {
      cwrite(dat2c, H, "extern const unsigned char $string lower$"
	     "[$long$];$n$$n$", name, dat->size);
   }

   /* write out the object definition in the source file */
   cwrite(dat2c, C, "$string$const unsigned char $string lower$"
	  "[$long$] = $n$"
	  "    $data$;$n$"
	  "$n$$n$$n$",
	  dat2c->global_symbols ? "" : "static ",
	  name, dat->size, dat->size, 4, dat->dat);

   return 0;
}



/* cvt_FONT()
 *
 *  Writes a FONT (either a color font or a mono font).
 */
static int cvt_FONT_mono(struct dat2c *dat2c, FONT *f, const char *name)
{
   struct FONT_MONO_DATA *iter = 0;
   int data_count = 0;

   /* iterate through each range, exporting them in turn */
   for (iter = f->data; iter; iter = iter->next) {
      int glyph = 0;

      /* export all the glyphs */
      for (glyph = iter->begin; glyph < iter->end; glyph++) {
	 FONT_GLYPH *fg = iter->glyphs[glyph - iter->begin];

	 cwrite(dat2c, C,
		"static struct { short w,h; char data[$int$]; } $string lower$_glyph"
		"$int$ = {$n$" "    $int$, $int$, /* width, height */$n$"
		"    $data$$n$" "};$n$$n$", fg->h * ((fg->w + 7) / 8),
		name, glyph, fg->w, fg->h, fg->h * ((fg->w + 7) / 8), 4,
		fg->dat);
      }

      /* glyph list */
      cwrite(dat2c, C, "static FONT_GLYPH* $string lower$_range$int$_"
	     "glyphs[] = {$n$", name, data_count);

      for (glyph = iter->begin; glyph < iter->end; glyph++) {
	 cwrite(dat2c, C,
		"    (FONT_GLYPH *)&$string lower$_glyph$int$,$n$", name,
		glyph);
      }

      cwrite(dat2c, C, "    0$n$};$n$$n$");

      /* write the FONT_MONO_DATA entry */
      cwrite(dat2c, C, "static FONT_MONO_DATA $string lower$_range"
	     "$int$ = {$n$"
	     "    $int$, $int$, /* begin, end */$n$"
	     "    $string lower$_range$int$_glyphs,$n$",
	     name, data_count, iter->begin, iter->end, name, data_count);

      if (data_count) {
	 cwrite(dat2c, C, "    &$string lower$_range$int$$n$",
		name, (data_count - 1));
      }
      else {
	 cwrite(dat2c, C, "    0$n$");
      }

      cwrite(dat2c, C, "};$n$$n$");

      data_count++;
   }

   return data_count;
}

static int cvt_FONT_color(struct dat2c *dat2c, FONT *f, const char *name)
{
   struct FONT_COLOR_DATA *iter = 0;
   int data_count = 0, result = 0;

   char *name_buf = malloc(strlen(name) + 18);
   if (!name_buf)
      out_of_memory();

   /* iterate through each range, exporting them in turn */
   for (iter = f->data; iter; iter = iter->next) {
      int glyph = 0;

      /* export all the glyphs */
      for (glyph = iter->begin; glyph < iter->end; glyph++) {
	 BITMAP *fg = iter->bitmaps[glyph - iter->begin];

	 sprintf(name_buf, "%s_glyph%d", name, glyph);

	 result = cvt_BITMAP_aux(dat2c, fg, name_buf, 0);
	 if (result) {
	    free(name_buf);
	    return 1;
	 }
	 cwrite(dat2c, C, "$n$");
      }

      /* glyph list */
      cwrite(dat2c, C, "static BITMAP* $string lower$_range$int$"
	     "_glyphs[] = {$n$", name, data_count);

      for (glyph = iter->begin; glyph < iter->end; glyph++) {
	 cwrite(dat2c, C, "    (BITMAP *)&$string lower$_glyph$int$,$n$",
		name, glyph);
      }

      cwrite(dat2c, C, "    0$n$};$n$$n$");

      /* write the FONT_COLOR_DATA entry */
      cwrite(dat2c, C, "static FONT_COLOR_DATA $string lower$_range"
	     "$int$ = {$n$"
	     "    $int$, $int$, /* begin, end */$n$"
	     "    $string lower$_range$int$_glyphs,$n$",
	     name, data_count, iter->begin, iter->end, name, data_count);

      if (data_count) {
	 cwrite(dat2c, C, "    &$string lower$_range$int$$n$",
		name, (data_count - 1));
      }
      else {
	 cwrite(dat2c, C, "    0$n$");
      }

      cwrite(dat2c, C, "};$n$$n$");

      data_count++;
   }

   free(name_buf);
   return data_count;
}

static int cvt_FONT(struct dat2c *dat2c, DATAFILE *dat, const char *name)
{
   FONT *f = dat->dat;
   int num_ranges = 0;

   /* declare this in the header if we are exporting objects */
   if (dat2c->global_symbols) {
      cwrite(dat2c, H, "extern FONT $string lower$;$n$", name);
   }

   if (f->vtable == font_vtable_mono) {
      num_ranges = cvt_FONT_mono(dat2c, f, name);
   }
   else if (f->vtable == font_vtable_color) {
      num_ranges = cvt_FONT_color(dat2c, f, name);
   }
   else {
      fprintf(stderr, "Font %s has an unrecognised vtable - cannot "
	      "export it.\n", name);
      return 1;
   }

   cwrite(dat2c, C, "$string$FONT $string lower$ = {$n$"
	  "    &$string lower$_range$int$,$n$"
	  "    $int$, /* height */$n$"
	  "    (FONT_VTABLE *)$int$ /* color flag */$n$"
	  "};$n$"
	  "$n$$n$$n$",
	  dat2c->global_symbols ? "" : "static ",
	  name,
	  name, (num_ranges - 1),
	  f->height, f->vtable == font_vtable_color ? 1 : 0);

   return 0;
}



/* cvt_SAMPLE
 *
 *  Writes a sound sample.
 */
static int cvt_SAMPLE(struct dat2c *dat2c, DATAFILE *dat, const char *name)
{
   SAMPLE *spl = dat->dat;

   /* declare this in the header if we are exporting objects */
   if (dat2c->global_symbols) {
      cwrite(dat2c, H, "extern SAMPLE $string lower$;$n$$n$", name);
   }

   cwrite(dat2c, C, "$string$SAMPLE $string lower$ = {$n$"
	  "    $int$, $int$, $int$, 0,"
	  " /* bits, stereo, freq */$n$"
	  "    $ulong$, /* length (in samples) */$n$"
	  "    $ulong$, $ulong$, $ulong$,"
	  " /* loop start/end */$n$"
	  "    $data$$n$"
	  "};$n$"
	  "$n$$n$$n$",
	  dat2c->global_symbols ? "" : "static ",
	  name,
	  spl->bits, spl->stereo, spl->freq,
	  spl->len,
	  spl->loop_start, spl->loop_end, spl->param,
	  spl->len * (spl->bits / 8) * (spl->stereo ? 2 : 1), 4,
	  spl->data);

   return 0;
}



/* cvt_MIDI
 *
 *  Writes a MIDI stream.
 */
static int cvt_MIDI(struct dat2c *dat2c, DATAFILE *dat, const char *name)
{
   MIDI *midi = dat->dat;
   int i = 0;

   /* declare this in the header if we are exporting objects */
   if (dat2c->global_symbols) {
      cwrite(dat2c, H, "extern MIDI $string lower$;$n$$n$", name);
   }

   cwrite(dat2c, C, "$string$MIDI $string lower$ = {$n$"
	  "    $int$, /* divisions */$n$"
	  "    { /* tracks follow */$n$",
	  dat2c->global_symbols ? "" : "static ", name, midi->divisions);

   for (i = 0; i < MIDI_TRACKS; i++) {
      if (midi->track[i].data) {
	 cwrite(dat2c, C, "        { /* track $int$ */$n$"
		"          $data$,$n$"
		"          $int$$n$"
		"        }",
		i,
		midi->track[i].len, 10, midi->track[i].data,
		midi->track[i].len);
      }
      else {
	 cwrite(dat2c, C, "        { 0, 0 }");
      }
      if (i < (MIDI_TRACKS - 1))
	 cwrite(dat2c, C, ",$n$");
      else
	 cwrite(dat2c, C, "$n$");
   }

   cwrite(dat2c, C, "    } /* end of tracks */$n$" "};$n$" "$n$$n$$n$");

   return 0;
}



/* cvt_RLE_SPRITE
 *
 *  Writes an RLE sprite.
 */
static int
cvt_RLE_SPRITE(struct dat2c *dat2c, DATAFILE *dat, const char *name)
{
   RLE_SPRITE *rle = dat->dat;
   int bpp = rle->color_depth;

   if (bpp > 8)
      truecolor = TRUE;

   /* declare this in the header if we are exporting objects */
   if (dat2c->global_symbols) {
      cwrite(dat2c, H, "extern RLE_SPRITE $string lower$;$n$$n$", name);
   }

   cwrite(dat2c, C,
	  "$string$struct { int w, h; int color_depth; int size; char data[$int$]; } $string lower$ = {$n$"
	  "    $int$, $int$, /* width, height */$n$"
	  "    $int$, /* color depth */$n$"
	  "    $int$, /* compressed size (bytes) */$n$" "    $data$$n$"
	  "};$n$" "$n$$n$$n$", dat2c->global_symbols ? "" : "static ",
	  rle->size, name, rle->w, rle->h, bpp, rle->size, rle->size, 4,
	  rle->dat);

   return 0;
}



/* cvt_CSPRITE
 *
 *  We have encountered a compiled sprite. Unless the user has passed a
 *  commandline option to convert this to a bitmap, we need to print an
 *  error message and then abort.
 */
static int
cvt_CSPRITE(struct dat2c *dat2c, DATAFILE *dat, const char *name)
{
   if (!dat2c->convert_compiled_sprites) {
      fprintf(stderr, "Error: encountered a compiled sprite. Please\n"
	      "see documentation for more information.\n");
      return 1;
   }

   /* sneakily convert to a bitmap and continue on our way */
   dat->type = DAT_BITMAP;
   return cvt_BITMAP(dat2c, dat, name);
}



/* cvt_PALETTE
 *
 *  Writes a palette.
 */
static int
cvt_PALETTE(struct dat2c *dat2c, DATAFILE *dat, const char *name)
{
   RGB *pal = dat->dat;
   int i = 0;

   /* declare this in the header if we are exporting objects */
   if (dat2c->global_symbols) {
      cwrite(dat2c, H, "extern PALETTE $string lower$;$n$$n$", name);
   }

   cwrite(dat2c, C, "$string$PALETTE $string lower$ = {$n$",
	  dat2c->global_symbols ? "" : "static ", name);

   for (i = 0; i < 255; i++) {
      cwrite(dat2c, C, "    { $int$, $int$, $int$, 0 },"
	     " /* $int$ */$n$", pal[i].r, pal[i].g, pal[i].b, i);
   }
   cwrite(dat2c, C, "    { $int$, $int$, $int$, 0 }"
	  " /* $int$ */$n$"
	  "};$n$" "$n$$n$$n$", pal[i].r, pal[i].g, pal[i].b, i);

   return 0;
}



/* dat2c_converters[]
 *
 *  This array lists all the available converters.
 */
static struct dat2c_converter dat2c_converters[] = {
   {DAT_FILE, cvt_FILE},
   {DAT_PROPERTY, 0},		/* this notation means 'skip' */
   {DAT_ID('i', 'n', 'f', 'o'), 0},
   {DAT_FONT, cvt_FONT},
   {DAT_SAMPLE, cvt_SAMPLE},
   {DAT_MIDI, cvt_MIDI},
   {DAT_BITMAP, cvt_BITMAP},
   {DAT_RLE_SPRITE, cvt_RLE_SPRITE},
   {DAT_C_SPRITE, cvt_CSPRITE},
   {DAT_XC_SPRITE, cvt_CSPRITE},
   {DAT_PALETTE, cvt_PALETTE},
   {DAT_END, cvt_DATA}		/* special case: use for unknown */
};



/* write_datafile()
 *
 *  Writes a datafile to the C source file. The prefix must always be
 *  specified, though it may be an empty string. The datafile given in
 *  the `dat' parameter overrides the datafile in the `dat2c' structure.
 *
 *  Returns 0 on success.
 */
static int
write_datafile(struct dat2c *dat2c, DATAFILE *dat,
	       const char *prefix, const char *datafile_name)
{
   int i = 0, array_count = 0;

   /* declare the datafile in the header */
   cwrite(dat2c, H, "extern DATAFILE $string lower$[];$n$$n$",
	  datafile_name);

   /* write datafile objects and headers */
   for (i = 0; dat[i].type != DAT_END; i++) {
      int j;
      const char *obj_name = 0;

      /* Here we write out each structure, eg. a BITMAP, by calling a
       * function with the relevant data. In addition, we write a
       * #define preprocessor symbol in the header corresponding to the
       * object's index in the DATAFILE array. Finally, if the object
       * has any properties, then we write those as well.
       */

      /* select the object writer, skipping DAT_PROPERTY and DAT_INFO.
         Unknown objects are dumped as blocks of binary data. */
      for (j = 0; dat2c_converters[j].type != DAT_END; j++)
	 if (dat2c_converters[j].type == dat[i].type)
	    break;

      /* skip DAT_PROPERTY/DAT_INFO */
      if (!dat2c_converters[j].cvt)
	 continue;

      /* generate a name */
      obj_name = get_c_identifier(prefix, dat + i);

      /* write out a #define to the header */
      cwrite(dat2c, H, "#define $string upper$ $int$$n$",
	     obj_name, array_count);

      /* write the object's properties */
      if (dat[i].prop) {
	 int pi = 0;

	 cwrite(dat2c, C, "static DATAFILE_PROPERTY $string lower$"
		"_prop[] = {$n$", obj_name);

	 for (pi = 0; dat[i].prop[pi].type != DAT_END; pi++) {
	    cwrite(dat2c, C, "    { $data$, $dat_id$ },$n$",
		   strlen(dat[i].prop[pi].dat), 6, dat[i].prop[pi].dat,
		   dat[i].prop[pi].type);
	 }

	 cwrite(dat2c, C, "    { 0, DAT_END }$n$" "};$n$$n$");
      }

      /* write the object */
      dat2c_converters[j].cvt(dat2c, dat + i, obj_name);

      /* increment true object count */
      array_count++;
   }

   /* write count of objects to the header */
   cwrite(dat2c, H, "#define $string upper$$string$COUNT $int$$n$"
	  "$n$$n$$n$", prefix, *prefix ? "_" : "", array_count);

   /* write DATAFILE structure */
   cwrite(dat2c, C, "DATAFILE $string lower$[] = {$n$", datafile_name);

   for (i = 0; dat[i].type != DAT_END; i++) {
      /* here we write out each field of the DATAFILE array, pointing
         to the previously written structures. */
      int j = 0;
      const char *obj_name = get_c_identifier(prefix, dat + i);

      /* skip DAT_PROPERTY/DAT_INFO */
      for (j = 0; dat2c_converters[j].type != DAT_END; j++)
	 if (dat2c_converters[j].type == dat[i].type)
	    break;
      if (!dat2c_converters[j].cvt)
	 continue;

      cwrite(dat2c, C, "    { &$string lower$, $dat_id$, $long$, ",
	     obj_name, dat[i].type, dat[i].size);

      if (dat[i].prop)
	 cwrite(dat2c, C, "$string lower$_prop },$n$", obj_name);
      else
	 cwrite(dat2c, C, "0 },$n$");
   }
   cwrite(dat2c, C, "    { 0, DAT_END, 0, 0 }$n$" "};$n$" "$n$$n$$n$");

   return 0;
}



/* do_conversion()
 */
int do_conversion(struct dat2c *dat2c)
{
   int result = 0;
   char *prefixed_name = 0;

   prefixed_name = malloc(5 +
      (dat2c->prefix ? (signed)strlen(dat2c->prefix) : 0) +
      (dat2c->datafile_name ? (signed)strlen(dat2c->datafile_name) : 0));
   if (!prefixed_name)
      out_of_memory();
   sprintf(prefixed_name, "%s%s%s",
	   dat2c->prefix ? dat2c->prefix : "",
	   dat2c->prefix ? "_" : "",
	   dat2c->datafile_name ? dat2c->datafile_name : "data");

   if (dat2c->fname_h)
      write_header_start(dat2c);
   write_source_start(dat2c);

   result = write_datafile(dat2c, dat2c->dat,
			   dat2c->prefix ? dat2c->prefix : "",
			   prefixed_name);

   write_header_end(dat2c);
   write_source_end(dat2c, prefixed_name);

   free(prefixed_name);
   clear_c_identifiers();

   return result;
}
