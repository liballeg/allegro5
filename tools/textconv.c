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
 *      Unicode text format conversion utility.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#define ALLEGRO_USE_CONSOLE

#include <stdio.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



char *in_name = NULL;
char *out_name = NULL;

int in_format = 0;
int out_format = 0;

int in_flip = FALSE;
int out_flip = FALSE;

UTYPE_INFO *in_type = NULL;
UTYPE_INFO *out_type = NULL;

int flag_c = FALSE;
int flag_hex = FALSE;
int flag_dos = FALSE;
int flag_unix = FALSE;
int flag_verbose = FALSE;
int flag_watermark = FALSE;

char *in_data = NULL;
int in_size = 0;
int in_pos = 0;

char *cp_table = NULL;



void usage(void)
{
   printf("\nUnicode text format conversion utility for Allegro " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR);
   printf("\nBy Shawn Hargreaves, " ALLEGRO_DATE_STR "\n\n");
   printf("Usage: 'textconv [options] -i{type} [inputfile] -o{type} [outputfile]'\n");
   printf("\n");
   printf("Character encoding {type} codes:\n");
   printf("\ta - 8 bit ASCII (Latin-1 codepage)\n");
   printf("\tb - 7 bit bare ASCII or codepage loaded via -f\n");
   printf("\tu - UTF-8 encoding\n");
   printf("\tw - 16 bit Unicode (current machine endianness)\n");
   printf("\tW - 16 bit Unicode (flip endianness)\n");
   printf("\n");
   printf("Option flags:\n");
   printf("\t-f{file} = load alternate codepage file\n");
   printf("\t-c = output data in C string format (default is binary)\n");
   printf("\t-h = output data in hex format (default is binary)\n");
   printf("\t-d = output line breaks in DOS (CR/LF) format\n");
   printf("\t-u = output line breaks in Unix (LF) format\n");
   printf("\t-w = add an endianness watermark to Unicode output files\n");
   printf("\t-v = verbose output\n");
   printf("\n");
   printf("If filenames are not specified, stdin and stdout will be used\n");
}



UTYPE_INFO *get_format_info(int type, char **desc, int *flip)
{
   switch (type) {

      case 'a': 
	 *desc = "ASCII (ISO Latin-1)";
	 *flip = FALSE;
	 return _find_utype(U_ASCII);

      case 'b': 
	 *desc = "ASCII (bare 7 bits)";
	 *flip = FALSE;
	 return _find_utype(U_ASCII_CP);

      case 'u': 
	 *desc = "UTF-8 encoding";
	 *flip = FALSE;
	 return _find_utype(U_UTF8);

      case 'w': 
	 *desc = "Unicode (default endianness)";
	 *flip = FALSE;
	 return _find_utype(U_UNICODE);

      case 'W': 
	 *desc = "Unicode (flip endianness)";
	 *flip = TRUE;
	 return _find_utype(U_UNICODE);
   }

   return NULL;
}



int read_input_file(void)
{
   FILE *f;
   int alloc, c; 

   if (in_name) {
      f = fopen(in_name, "rb");
      if (!f)
	 return 1;
   }
   else
      f = stdin;

   alloc = 0;

   while ((c = fgetc(f)) != EOF) {
      if (in_size >= alloc) {
	 alloc = in_size+1024;
	 in_data = realloc(in_data, alloc);
	 if (!in_data) {
	    fclose(f);
	    return 1;
	 }
      }

      in_data[in_size++] = c;
   }

   if (in_name)
      fclose(f);

   return 0;
}



int get_input(void)
{
   int c;

   if (in_pos >= in_size)
      return EOF;

   c = in_type->u_getc(in_data + in_pos);
   in_pos += in_type->u_width(in_data + in_pos);

   if (in_pos > in_size)
      return EOF;

   if (in_flip)
      c = ((c&0xFF)<<8) | ((c&0xFF00)>>8);

   return c;
}



void write_output(FILE *f, int c)
{
   static int warned = FALSE;
   static int count = 0;
   static int sstate = -1;
   unsigned char buf[16];
   int size, i;

   if (!out_type->u_isok(c)) {
      if (!warned) {
	 fprintf(stderr, "Warning: lossy reduction to output format\n");
	 warned = TRUE;
      }
      c = '^';
   }

   if (out_flip)
      c = ((c&0xFF)<<8) | ((c&0xFF00)>>8);

   size = out_type->u_setc((char *)buf, c);

   for (i=0; i<size; i++) {
      if (flag_c) {
	 /* output in C-string format */
	 switch (buf[i]) {

	    case '\r': 
	       fputs("\\r", f); 
	       sstate = 0;
	       break;

	    case '\n':
	       fputs("\\n", f);
	       sstate = 0;
	       break;

	    case '\t': 
	       fputs("\\t", f); 
	       sstate = 0;
	       break;

	    case '\"': 
	       fputs("\\\"", f); 
	       sstate = 0;
	       break;

	    case '\\': 
	       fputs("\\\\", f); 
	       sstate = 0;
	       break;

	    default:
	       if ((buf[i] >= 32) && (buf[i] < 127)) {
		  if (sstate == 1)
		     fputs("\" \"", f);

		  putc(buf[i], f);
		  sstate = 0;
	       }
	       else {
		  fprintf(f, "\\x%02X", buf[i]);
		  sstate = 1;
	       }
	       break;
	 }
      }
      else if (flag_hex) {
	 /* output in hex format */
	 if (count&7)
	    putc(' ', f);

	 fprintf(f, "0x%02X,", buf[i]);

	 if ((count&7) == 7)
	    putc('\n', f);
      }
      else {
	 /* output in binary format */
	 putc(buf[i], f);
      }

      count++;
   }
}



int load_codepage(char *filename)
{
   FILE *f;
   int alloc, c, size;

   f = fopen(filename, "rb");

   if (!f)
      return -1;

   /* format of codepage file:
      table = 256 * unicode value (unsigned short)
      extras = X * [unicode value (unsigned short), codepage value (unsigned short)]
      if there are any, file must end with bytes 0,0
    */

   alloc = size = 0;

   while ((c = fgetc(f)) != EOF) {
      if (size >= alloc) {
	 alloc = size+1024;
	 cp_table = realloc(cp_table, alloc);
	 if (!cp_table) {
	    fclose(f);
	    return 1;
	 }
      }

      cp_table[size++] = c;
   }

   set_ucodepage((unsigned short *)cp_table, (size > 256 * (int)sizeof(unsigned short)) ? (unsigned short *)cp_table + 256 : NULL);

   fclose(f);

   return 0;
}



int main(int argc, char *argv[])
{
   FILE *out_file;
   char *in_desc, *out_desc;
   int was_cr = FALSE;
   int err = 0;
   int c;

   if (install_allegro(SYSTEM_NONE, &errno, atexit) != 0)
      return 1;

   for (c=1; c<argc; c++) {
      if (argv[c][0] == '-') {
	 switch (argv[c][1]) {

	    case 'i':
	       if (in_format) {
		  usage();
		  return 1;
	       }
	       in_format = argv[c][2];
	       if ((c<argc-1) && (argv[c+1][0] != '-'))
		  in_name = argv[++c];
	       break;

	    case 'o':
	       if (out_format) {
		  usage();
		  return 1;
	       }
	       out_format = argv[c][2];
	       if ((c<argc-1) && (argv[c+1][0] != '-'))
		  out_name = argv[++c];
	       break;

	    case 'f':
	       if (load_codepage(argv[c]+2) != 0) {
		  fprintf(stderr, "Error reading %s\n", argv[c]+2);
		  return 1;
	       }
	       break;

	    case 'c':
	       flag_c = TRUE;
	       break;

	    case 'h':
	       flag_hex = TRUE;
	       break;

	    case 'd':
	       flag_dos = TRUE;
	       break;

	    case 'u':
	       flag_unix = TRUE;
	       break;

	    case 'v':
	       flag_verbose = TRUE;
	       break;

	    case 'w':
	       flag_watermark = TRUE;
	       break;

	    default:
	       fprintf(stderr, "Unknown option '%s'\n", argv[c]);
	       return 1;
	 }
      }
      else {
	 usage();
	 return 1;
      }
   }

   if ((!in_format) || (!out_format)) {
      usage();
      return 1;
   }

   in_type = get_format_info(in_format, &in_desc, &in_flip);

   if (!in_type) {
      fprintf(stderr, "Unknown input format %c\n", in_format);
      return 1;
   }

   out_type = get_format_info(out_format, &out_desc, &out_flip);

   if (!out_type) {
      fprintf(stderr, "Unknown output format %c\n", out_format);
      return 1;
   }

   if (((flag_dos) && (flag_unix)) || ((flag_c) && (flag_hex))) {
      fprintf(stderr, "Invalid option combination\n");
      return 1;
   }

   if (flag_verbose) {
      printf("Reading %s as %s\n", (in_name) ? in_name : "{stdin}", in_desc);
      printf("Writing %s as %s\n", (out_name) ? out_name : "{stdout}", out_desc);

      if (flag_c)
	 printf("Output in C-language string format\n");
      else if (flag_hex)
	 printf("Output in hexadecimal format\n");
   }

   if (read_input_file() != 0) {
      fprintf(stderr, "Error reading %s\n", in_name);
      return 1;
   }

   if (out_name) {
      out_file = fopen(out_name, ((flag_c) || (flag_hex)) ? "wt" : "wb");
      if (!out_file) {
	 fprintf(stderr, "Error writing %s\n", out_name);
	 err = 1;
	 goto getout;
      }
   }
   else
      out_file = stdout;

   if (flag_c)
      putc('"', out_file);

   if (flag_watermark) {
      if (flag_verbose)
	 printf("Adding Unicode endianness watermark\n");
      write_output(out_file, 0xFEFF);
   }

   while ((c = get_input()) != EOF) {

      switch (c) {

	 case '\n':
	    if ((flag_dos) && (!was_cr))
	       write_output(out_file, '\r');
	    was_cr = FALSE;
	    break;

	 case '\r':
	    was_cr = TRUE;
	    if (flag_unix)
	       continue;
	    break;

	 case 0xFEFF:
	    if (flag_verbose)
	       printf("Endianness watermark ok\n");
	    was_cr = FALSE;
	    continue;

	 case 0xFFFE:
	    if (flag_verbose)
	       printf("Bad endianness watermark! Toggling input format...\n");
	    in_flip = !in_flip;
	    was_cr = FALSE;
	    continue;

	 default:
	    was_cr = FALSE;
	    break;
      }

      write_output(out_file, c);
   }

   if (flag_c)
      putc('"', out_file);

   if ((flag_c) || (flag_hex))
      putc('\n', out_file);

   getout:

   if (in_data)
      free(in_data);

   if (cp_table)
      free(cp_table);

   if ((out_name) && (out_file))
      fclose(out_file);

   return err;
}

END_OF_MAIN()
