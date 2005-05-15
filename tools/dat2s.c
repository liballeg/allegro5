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
 *      Grabber datafile -> asm converter for the Allegro library.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#define ALLEGRO_USE_CONSOLE

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "datedit.h"


/* unused callbacks for datedit.c */
void datedit_msg(AL_CONST char *fmt, ...) { }
void datedit_startmsg(AL_CONST char *fmt, ...) { }
void datedit_endmsg(AL_CONST char *fmt, ...) { }
void datedit_error(AL_CONST char *fmt, ...) { }
int datedit_ask(AL_CONST char *fmt, ...) { return 0; }
int datedit_select(AL_CONST char *(*list_getter)(int index, int *list_size), AL_CONST char *fmt, ...) { return 0; }


/* this program is not portable! */
#ifdef ALLEGRO_I386


#ifndef ALLEGRO_ASM_PREFIX
   #define ALLEGRO_ASM_PREFIX    ""
#endif


static int err = 0;
static int truecolor = FALSE;
static int convert_compiled_sprites = FALSE;

static char prefix[80] = "";

static char *infilename = NULL;
static char *outfilename = NULL;
static char *outfilenameheader = NULL;
static char *dataname = NULL;
static char default_dataname[]="data";
static char *password = NULL;

static DATAFILE *data = NULL;

static FILE *outfile = NULL;
static FILE *outfileheader = NULL;

static void output_object(DATAFILE *object, char *name);



static void usage(void)
{
   printf("\nDatafile -> asm conversion utility for Allegro " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR "\n");
   printf("By Shawn Hargreaves, " ALLEGRO_DATE_STR "\n\n");
   printf("Usage: dat2s [options] inputfile.dat\n\n");
   printf("Options:\n");
   printf("\t'-o outputfile.s' sets the output file (default stdout)\n");
   printf("\t'-h outputfile.h' sets the output header file (default none)\n");
   printf("\t'-p prefix' sets the object name prefix string (default none)\n");
   printf("\t'-S' converts COMPILED_SPRITEs to BITMAPs (default abort)\n");
   printf("\t'-n name' gives a name for the datafile (default 'data')\n");
   printf("\t'-007 password' sets the datafile password\n");
}



static void write_data(unsigned char *data, int size)
{
   int c;

   for (c=0; c<size; c++) {
      if ((c & 7) == 0)
	 fprintf(outfile, "\t.byte ");

      fprintf(outfile, "0x%02X", data[c]);

      if ((c<size-1) && ((c & 7) != 7))
	 fprintf(outfile, ", ");
      else
	 fprintf(outfile, "\n");
   }
}



static void output_data(unsigned char *data, int size, char *name, char *type, int global)
{
   fprintf(outfile, "# %s (%d bytes)\n", type, size);

   if (global)
      fprintf(outfile, ".globl " ALLEGRO_ASM_PREFIX "%s%s\n", prefix, name);

   fprintf(outfile, ".balign 4\n" ALLEGRO_ASM_PREFIX "%s%s:\n", prefix, name);

   write_data(data, size);

   fprintf(outfile, "\n");
}



static void output_bitmap(BITMAP *bmp, char *name, int global)
{
   int bpp = bitmap_color_depth(bmp);
   int bypp = (bpp + 7) / 8;
   char buf[160];
   int c;

   if (bpp > 8)
      truecolor = TRUE;

   strcpy(buf, name);
   strcat(buf, "_data");

   output_data(bmp->line[0], bmp->w*bmp->h*bypp, buf, "bitmap data", FALSE);

   fprintf(outfile, "# bitmap\n");

   if (global)
      fprintf(outfile, ".globl " ALLEGRO_ASM_PREFIX "%s%s\n", prefix, name);

   fprintf(outfile, ".balign 4\n" ALLEGRO_ASM_PREFIX "%s%s:\n", prefix, name);

   fprintf(outfile, "\t.long %-16d# w\n", bmp->w);
   fprintf(outfile, "\t.long %-16d# h\n", bmp->h);
   fprintf(outfile, "\t.long %-16d# clip\n", -1);
   fprintf(outfile, "\t.long %-16d# cl\n", 0);
   fprintf(outfile, "\t.long %-16d# cr\n", bmp->w);
   fprintf(outfile, "\t.long %-16d# ct\n", 0);
   fprintf(outfile, "\t.long %-16d# cb\n", bmp->h);
   fprintf(outfile, "\t.long %-16d# bpp\n", bpp);
   fprintf(outfile, "\t.long %-16d# write_bank\n", 0);
   fprintf(outfile, "\t.long %-16d# read_bank\n", 0);
   fprintf(outfile, "\t.long " ALLEGRO_ASM_PREFIX "%s%s_data\n", prefix, name);
   fprintf(outfile, "\t.long %-16d# bitmap_id\n", 0);
   fprintf(outfile, "\t.long %-16d# extra\n", 0);
   fprintf(outfile, "\t.long %-16d# x_ofs\n", 0);
   fprintf(outfile, "\t.long %-16d# y_ofs\n", 0);
   fprintf(outfile, "\t.long %-16d# seg\n", 0);

   for (c=0; c<bmp->h; c++)
      fprintf(outfile, "\t.long " ALLEGRO_ASM_PREFIX "%s%s_data + %d\n", prefix, name, bmp->w*c*bypp);

   fprintf(outfile, "\n");
}



static void output_sample(SAMPLE *spl, char *name)
{
   char buf[160];

   strcpy(buf, name);
   strcat(buf, "_data");

   output_data(spl->data, spl->len * ((spl->bits==8) ? 1 : sizeof(short)) * ((spl->stereo) ? 2 : 1), buf, "waveform data", FALSE);

   fprintf(outfile, "# sample\n.globl " ALLEGRO_ASM_PREFIX "%s%s\n", prefix, name);
   fprintf(outfile, ".balign 4\n" ALLEGRO_ASM_PREFIX "%s%s:\n", prefix, name);
   fprintf(outfile, "\t.long %-16d# bits\n", spl->bits);
   fprintf(outfile, "\t.long %-16d# stereo\n", spl->stereo);
   fprintf(outfile, "\t.long %-16d# freq\n", spl->freq);
   fprintf(outfile, "\t.long %-16d# priority\n", spl->priority);
   fprintf(outfile, "\t.long %-16ld# length\n", spl->len);
   fprintf(outfile, "\t.long %-16ld# loop_start\n", spl->loop_start);
   fprintf(outfile, "\t.long %-16ld# loop_end\n", spl->loop_end);
   fprintf(outfile, "\t.long %-16ld# param\n", spl->param);
   fprintf(outfile, "\t.long " ALLEGRO_ASM_PREFIX "%s%s_data\n\n", prefix, name);
}



static void output_midi(MIDI *midi, char *name)
{
   char buf[160];
   int c;

   for (c=0; c<MIDI_TRACKS; c++) {
      if (midi->track[c].data) {
	 sprintf(buf, "%s_track_%d", name, c);
	 output_data(midi->track[c].data, midi->track[c].len, buf, "midi track", FALSE);
      }
   }

   fprintf(outfile, "# midi file\n.globl " ALLEGRO_ASM_PREFIX "%s%s\n", prefix, name);
   fprintf(outfile, ".balign 4\n" ALLEGRO_ASM_PREFIX "%s%s:\n", prefix, name);
   fprintf(outfile, "\t.long %-16d# divisions\n", midi->divisions);

   for (c=0; c<MIDI_TRACKS; c++)
      if (midi->track[c].data)
	 fprintf(outfile, "\t.long " ALLEGRO_ASM_PREFIX "%s%s_track_%d, %d\n", prefix, name, c, midi->track[c].len);
      else
	 fprintf(outfile, "\t.long 0, 0\n");

   fprintf(outfile, "\n");
}



static void output_font_color(FONT_COLOR_DATA *cf, char *name, int depth)
{
   char buf[1000], goodname[1000];
   int ch;

   if (cf->next) output_font_color(cf->next, name, depth + 1);

   if (depth > 0) 
      sprintf(goodname, "%s_r%d", name, depth + 1);
   else 
      strcpy(goodname, name);

   for (ch = cf->begin; ch < cf->end; ch++) {
      sprintf(buf, "%s_char_%04X", goodname, ch);
      output_bitmap(cf->bitmaps[ch - cf->begin], buf, FALSE);
   }

   fprintf(outfile, "# glyph list\n");
   fprintf(outfile, ".balign 4\n" ALLEGRO_ASM_PREFIX "%s%s_glyphs:\n", prefix, goodname);

   for (ch = cf->begin; ch < cf->end; ch++)
      fprintf(outfile, "\t.long " ALLEGRO_ASM_PREFIX "%s%s_char_%04X\n", prefix, goodname, ch);
   fprintf(outfile, "\n# FONT_COLOR_DATA\n");

   fprintf(outfile, ".balign 4\n" ALLEGRO_ASM_PREFIX "%s%s_data:\n", prefix, goodname);
   fprintf(outfile, "\t.long 0x%04X%10c# begin\n", cf->begin, ' ');
   fprintf(outfile, "\t.long 0x%04X%10c# end\n", cf->end, ' ');
   fprintf(outfile, "\t.long " ALLEGRO_ASM_PREFIX "%s%s_glyphs\n", prefix, goodname);
   if (cf->next) 
      fprintf(outfile, "\t.long " ALLEGRO_ASM_PREFIX "%s%s_r%d_data\n", prefix, name, depth + 2);
   else
      fprintf(outfile, "\t.long %-16d#next\n", 0);
   fprintf(outfile, "\n");
}



static void output_font_mono(FONT_MONO_DATA *mf, char *name, int depth)
{
   char buf[1000], goodname[1000];
   int ch;

   if (mf->next) output_font_mono(mf->next, name, depth + 1);

   if (depth > 0) 
      sprintf(goodname, "%s_r%d", name, depth + 1);
   else
      strcpy(goodname, name);

   for (ch = mf->begin; ch < mf->end; ch++) {
      FONT_GLYPH *g = mf->glyphs[ch - mf->begin];

      sprintf(buf, "%s_char_%04X", goodname, ch);
      fprintf(outfile, "# glyph\n");
      fprintf(outfile, ".balign 4\n" ALLEGRO_ASM_PREFIX "%s%s:\n", prefix, buf);
      fprintf(outfile, "\t.short %-15d# w\n", g->w);
      fprintf(outfile, "\t.short %-15d# h\n", g->h);
      
      write_data(g->dat, ((g->w + 7) / 8) * g->h);
      fprintf(outfile, "\n");
   }

   fprintf(outfile, "# glyph list\n");
   fprintf(outfile, ".balign 4\n" ALLEGRO_ASM_PREFIX "%s%s_glyphs:\n", prefix, goodname);

   for (ch = mf->begin; ch < mf->end; ch++)
      fprintf(outfile, "\t.long " ALLEGRO_ASM_PREFIX "%s%s_char_%04X\n", prefix, goodname, ch);
   fprintf(outfile, "\n# FONT_COLOR_DATA\n");

   fprintf(outfile, ".balign 4\n" ALLEGRO_ASM_PREFIX "%s%s_data:\n", prefix, goodname);
   fprintf(outfile, "\t.long 0x%04X%10c# begin\n", mf->begin, ' ');
   fprintf(outfile, "\t.long 0x%04X%10c# end\n", mf->end, ' ');
   fprintf(outfile, "\t.long " ALLEGRO_ASM_PREFIX "%s%s_glyphs\n", prefix, goodname);
   if (mf->next) 
      fprintf(outfile, "\t.long " ALLEGRO_ASM_PREFIX "%s%s_r%d_data\n", prefix, name, depth + 2);
   else
      fprintf(outfile, "\t.long %-16d#next\n", 0);
   fprintf(outfile, "\n");
}



static void output_font(FONT *f, char *name, int depth)
{
   int color_flag = (f->vtable == font_vtable_color ? 1 : 0);

   if (color_flag)
      output_font_color(f->data, name, 0);
   else
      output_font_mono(f->data, name, 0);

   fprintf(outfile, "# font\n");
   fprintf(outfile, ".globl " ALLEGRO_ASM_PREFIX "%s%s\n", prefix, name);
   fprintf(outfile, ".balign 4\n" ALLEGRO_ASM_PREFIX "%s%s:\n", prefix, name);
   fprintf(outfile, "\t.long " ALLEGRO_ASM_PREFIX "%s%s_data\n", prefix, name);
   fprintf(outfile, "\t.long %-16d# height\n", f->height);
   fprintf(outfile, "\t.long %-16d# color flag\n", color_flag);
   fprintf(outfile, "\n");
}



static void output_rle_sprite(RLE_SPRITE *sprite, char *name)
{
   int bpp = sprite->color_depth;

   if (bpp > 8)
      truecolor = TRUE;

   fprintf(outfile, "# RLE sprite\n.globl " ALLEGRO_ASM_PREFIX "%s%s\n", prefix, name);
   fprintf(outfile, ".balign 4\n" ALLEGRO_ASM_PREFIX "%s%s:\n", prefix, name);
   fprintf(outfile, "\t.long %-16d# w\n", sprite->w);
   fprintf(outfile, "\t.long %-16d# h\n", sprite->h);
   fprintf(outfile, "\t.long %-16d# color depth\n", bpp);
   fprintf(outfile, "\t.long %-16d# size\n", sprite->size);

   write_data((unsigned char *)sprite->dat, sprite->size);

   fprintf(outfile, "\n");
}



static void get_object_name(char *buf, char *name, DATAFILE *dat, int root)
{
   if (!root) {
      strcpy(buf, name);
      strcat(buf, "_");
   }
   else
      buf[0] = 0;

   strcat(buf, get_datafile_property(dat, DAT_NAME));
   strlwr(buf);
}



static void output_datafile(DATAFILE *dat, char *name, int root)
{
   char buf[160];
   int c;

   for (c=0; (dat[c].type != DAT_END) && (!err); c++) {
      get_object_name(buf, name, dat+c, root);
      output_object(dat+c, buf);
   }

   fprintf(outfile, "# datafile\n.globl " ALLEGRO_ASM_PREFIX "%s%s\n", prefix, name);
   fprintf(outfile, ".balign 4\n" ALLEGRO_ASM_PREFIX "%s%s:\n", prefix, name);

   if (outfileheader)
      fprintf(outfileheader, "extern DATAFILE %s%s[];\n", prefix, name);

   for (c=0; dat[c].type != DAT_END; c++) {
      get_object_name(buf, name, dat+c, root);
      fprintf(outfile, "\t.long " ALLEGRO_ASM_PREFIX "%s%s\n", prefix, buf);
      fprintf(outfile, "\t.long %-16d# %c%c%c%c\n", dat[c].type, (dat[c].type>>24) & 0xFF, (dat[c].type>>16) & 0xFF, (dat[c].type>>8) & 0xFF, dat[c].type & 0xFF);
      fprintf(outfile, "\t.long %-16ld# size\n", dat[c].size);
      fprintf(outfile, "\t.long %-16d# properties\n", 0);
   }

   fprintf(outfile, "\t.long 0\n\t.long -1\n\t.long 0\n\t.long 0\n\n");
}



static void output_object(DATAFILE *object, char *name)
{
   char buf[160];
   int i;

   switch (object->type) {

      case DAT_FONT:
	 if (outfileheader)
	    fprintf(outfileheader, "extern FONT %s%s;\n", prefix, name);

	 output_font((FONT *)object->dat, name, 0);
	 break;

      case DAT_BITMAP:
	 if (outfileheader)
	    fprintf(outfileheader, "extern BITMAP %s%s;\n", prefix, name);

	 output_bitmap((BITMAP *)object->dat, name, TRUE);
	 break;

      case DAT_PALETTE:
	 if (outfileheader)
	    fprintf(outfileheader, "extern PALETTE %s%s;\n", prefix, name);

	 output_data(object->dat, sizeof(PALETTE), name, "palette", TRUE);
	 break;

      case DAT_SAMPLE:
	 if (outfileheader)
	    fprintf(outfileheader, "extern SAMPLE %s%s;\n", prefix, name);

	 output_sample((SAMPLE *)object->dat, name);
	 break;

      case DAT_MIDI:
	 if (outfileheader)
	    fprintf(outfileheader, "extern MIDI %s%s;\n", prefix, name);

	 output_midi((MIDI *)object->dat, name);
	 break;

      case DAT_PATCH:
	 printf("Compiled GUS patch objects are not supported!\n");
	 break;

      case DAT_RLE_SPRITE:
	 if (outfileheader)
	    fprintf(outfileheader, "extern RLE_SPRITE %s%s;\n", prefix, name);

	 output_rle_sprite((RLE_SPRITE *)object->dat, name);
	 break;

      case DAT_FLI:
	 if (outfileheader)
	    fprintf(outfileheader, "extern unsigned char %s%s[];\n", prefix, name);

	 output_data(object->dat, object->size, name, "FLI/FLC animation", TRUE);
	 break;

      case DAT_C_SPRITE:
      case DAT_XC_SPRITE:
	 if (convert_compiled_sprites) {
	    object->type = DAT_BITMAP;
	    output_object(object, name);
	 }
	 else {
	    fprintf(stderr, "Error: encountered a compiled sprite (%s). Please\n"
	                    "see documentation for more information.\n", name);
	    err = 1;
	 }
	 break;

      case DAT_FILE:
	 output_datafile((DATAFILE *)object->dat, name, FALSE);
	 break;

      default:
	 for (i=0; datedit_object_info[i]->type != DAT_END; i++) {
	    if ((datedit_object_info[i]->type == object->type) && (datedit_object_info[i]->dat2s)) {
	       strcpy(buf, prefix);
	       strcat(buf, name);
	       datedit_object_info[i]->dat2s(object, buf, outfile, outfileheader);
	       return;
	    }
	 }

	 if (outfileheader)
	    fprintf(outfileheader, "extern unsigned char %s%s[];\n", prefix, name);

	 output_data(object->dat, object->size, name, "binary data", TRUE);
	 break;
   }
}



int main(int argc, char *argv[])
{
   int c;
   char tm[80];
   time_t now;

   if (install_allegro(SYSTEM_NONE, &errno, atexit) != 0)
      return 1;
   datedit_init();

   time(&now);
   strcpy(tm, asctime(localtime(&now)));
   for (c=0; tm[c]; c++)
      if ((tm[c] == '\r') || (tm[c] == '\n'))
	 tm[c] = 0;

   for (c=1; c<argc; c++) {
      if (stricmp(argv[c], "-o") == 0) {
	 if ((outfilename) || (c >= argc-1)) {
	    usage();
	    return 1;
	 }
	 outfilename = argv[++c];
      }
      else if (stricmp(argv[c], "-h") == 0) {
	 if ((outfilenameheader) || (c >= argc-1)) {
	    usage();
	    return 1;
	 }
	 outfilenameheader = argv[++c];
      }
      else if (stricmp(argv[c], "-p") == 0) {
	 if ((prefix[0]) || (c >= argc-1)) {
	    usage();
	    return 1;
	 }
	 strcpy(prefix, argv[++c]);
      }
      else if (stricmp(argv[c], "-n") == 0) {
	 if ((dataname) || (c >= argc-1)) {
	    usage();
	    return 1;
	 }
	 dataname = argv[++c];
      }
      else if (stricmp(argv[c], "-007") == 0) {
	 if ((password) || (c >= argc-1)) {
	    usage();
	    return 1;
	 }
	 password = argv[++c];
      }
      else if (stricmp(argv[c], "-S") == 0) {
	 if (convert_compiled_sprites) {
	    usage();
	    return 1;
	 }
	 convert_compiled_sprites = TRUE;
      }
      else {
	 if ((argv[c][0] == '-') || (infilename)) {
	    usage();
	    return 1;
	 }
	 infilename = argv[c];
      }
   }

   if (!infilename) {
      usage();
      return 1;
   }

   if ((prefix[0]) && (prefix[strlen(prefix)-1] != '_'))
      strcat(prefix, "_");

   if (!dataname)
      dataname = default_dataname;

   set_color_conversion(COLORCONV_NONE);

   data = datedit_load_datafile(infilename, FALSE, password);
   if (!data) {
      fprintf(stderr, "Error reading %s\n", infilename);
      err = 1; 
      goto ohshit;
   }

   if (outfilename) {
      outfile = fopen(outfilename, "w");
      if (!outfile) {
	 fprintf(stderr, "Error writing %s\n", outfilename);
	 err = 1; 
	 goto ohshit;
      }
   }
   else
      outfile = stdout;

   fprintf(outfile, "/* Compiled Allegro data file, produced by dat2s v" ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR " */\n");
   fprintf(outfile, "/* Input file: %s */\n", infilename);
   fprintf(outfile, "/* Date: %s */\n", tm);
   fprintf(outfile, "/* Do not hand edit! */\n\n.data\n\n");

   if (outfilenameheader) {
      outfileheader = fopen(outfilenameheader, "w");
      if (!outfileheader) {
	 fprintf(stderr, "Error writing %s\n", outfilenameheader);
	 err = 1; 
	 goto ohshit;
      }
      fprintf(outfileheader, "/* Allegro data file definitions, produced by dat2s v" ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR " */\n");
      fprintf(outfileheader, "/* Input file: %s */\n", infilename);
      fprintf(outfileheader, "/* Date: %s */\n", tm);
      fprintf(outfileheader, "/* Do not hand edit! */\n\n");
   }

   if (outfilename)
      printf("Converting %s to %s...\n", infilename, outfilename);

   output_datafile(data, dataname, TRUE);

   #ifdef ALLEGRO_USE_CONSTRUCTOR

      fprintf(outfile, ".text\n");
      fprintf(outfile, ".balign 4\n");
      fprintf(outfile, "construct_me:\n");
      fprintf(outfile, "\tpushl %%ebp\n");
      fprintf(outfile, "\tmovl %%esp, %%ebp\n");
      fprintf(outfile, "\tpushl $" ALLEGRO_ASM_PREFIX "%s%s\n", prefix, dataname);
      fprintf(outfile, "\tcall " ALLEGRO_ASM_PREFIX "_construct_datafile\n");
      fprintf(outfile, "\taddl $4, %%esp\n");
      fprintf(outfile, "\tleave\n");
      fprintf(outfile, "\tret\n\n");
      #ifdef ALLEGRO_DJGPP      
      fprintf(outfile, ".section .ctor\n");
      #else
      fprintf(outfile, ".section .ctors\n");
      #endif
      fprintf(outfile, "\t.long construct_me\n");

   #endif

   if ((outfile && ferror(outfile)) || (outfileheader && ferror(outfileheader)))
      err = 1;

   ohshit:

   if ((outfile) && (outfile != stdout))
      fclose(outfile);

   if (outfileheader)
      fclose(outfileheader);

   if (data)
      unload_datafile(data);

   if (err) {
      if (outfilename)
	 delete_file(outfilename);

      if (outfilenameheader)
	 delete_file(outfilenameheader);
   }
   else {
      #ifdef ALLEGRO_USE_CONSTRUCTOR

	 if (truecolor) {
	    printf("\nI noticed some truecolor images, so you must call fixup_datafile()\n");
	    printf("before using this data! (after setting a video mode).\n");
	 }

      #else

	 printf("\nI don't know how to do constructor functions on this platform, so you must\n");
	 printf("call fixup_datafile() before using this data! (after setting a video mode).\n");

      #endif
   }

   return err;
}


#else       /* ifdef ALLEGRO_I386 */


int main(void)
{
   allegro_init();
   allegro_message("Sorry, the DAT2S program only works on x86 processors.\n");
   return 1;
}


#endif

END_OF_MAIN()
