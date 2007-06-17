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
 *      Datafile archiving utility for the Allegro library.
 *
 *      By Shawn Hargreaves.
 *
 *      Nathan Smith added the recursive handling of directories.
 *
 *      See readme.txt for copyright information.
 */


#define ALLEGRO_USE_CONSOLE

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "allegro.h"
#include "datedit.h"

#if ((defined ALLEGRO_DOS) || (defined ALLEGRO_WINDOWS)) && (!defined SCAN_DEPEND)
   #define DAT_HAVE_CONIO_H
   #include <conio.h>
#endif



static int err = 0;
static int changed = 0;

static int opt_command = 0;
static int opt_compression = -1;
static int opt_strip = -1;
static int opt_sort = -1;
static int opt_relf = FALSE;
static int opt_verbose = FALSE;
static int opt_keepnames = FALSE;
static int opt_colordepth = -1;
static int opt_gridx = -1;
static int opt_gridy = -1;
static int opt_gridw = -1;
static int opt_gridh = -1;
static char *opt_datafilename = NULL;
static char *opt_outputname = NULL;
static char *opt_headername = NULL;
static char *opt_dependencyfile = NULL;
static char *opt_objecttype = NULL;
static char *opt_prefixstring = NULL;
static char *opt_password = NULL;
static char *opt_palette = NULL;

static int attrib_ok = FA_ARCH|FA_RDONLY;

static char canonical_datafilename[1024];

static char **opt_proplist = NULL;
static int opt_numprops = 0;

static char **opt_fixedproplist = NULL;
static int opt_numfixedprops = 0;

static char **opt_namelist = NULL;
static int opt_numnames = 0;

static int *opt_usedname = NULL;



static void allocate(int n)
{
   static int max = 0;
   if (n <= max)
      return;
   opt_proplist = realloc(opt_proplist, n * sizeof *opt_proplist);
   opt_fixedproplist = realloc(opt_fixedproplist, n * sizeof *opt_fixedproplist);
   opt_namelist = realloc(opt_namelist, n * sizeof *opt_namelist);
   opt_usedname = realloc(opt_usedname, n * sizeof *opt_usedname);
   max = n;
}



/* display help on the command syntax */
static void usage(void)
{
   printf("\nDatafile archiving utility for Allegro " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR "\n");
   printf("By Shawn Hargreaves, " ALLEGRO_DATE_STR "\n\n");
   printf("Usage: dat [options] filename.dat [names]\n\n");
   printf("Options:\n");
   printf("\t'-a' adds the named files to the datafile\n");
   printf("\t'-bpp colordepth' grabs bitmaps in the specified format\n");
   printf("\t'-c0' no compression\n");
   printf("\t'-c1' compress objects individually\n");
   printf("\t'-c2' global compression on the entire datafile\n");
   printf("\t'-d' deletes the named objects from the datafile\n");
   printf("\t'-dither' dithers when reducing color depths\n");
   printf("\t'-e' extracts the named objects from the datafile\n");
   printf("\t'-f' store references to original files as relative filenames\n");
   printf("\t'-g x y w h' grabs bitmap data from a specific grid location\n");
   printf("\t'-h outputfile.h' sets the output header file\n");
   printf("\t'-k' keeps the original filenames when grabbing objects\n");
   printf("\t'-l' lists the contents of the datafile\n");
   printf("\t'-m dependencyfile' outputs makefile dependencies\n");
   printf("\t'-n0' no sort: list the objects in the order they were added\n");
   printf("\t'-n1' sort the objects of the datafile alphabetically by name\n");
   printf("\t'-o output' sets the output file or directory when extracting data\n");
   printf("\t'-p prefixstring' sets the prefix for the output header file\n");
   printf("\t'-pal objectname' specifies which palette to use\n");
   printf("\t'-r' recursively adds directories as nested datafiles\n");
   printf("\t'-s0' no strip: save everything\n");
   printf("\t'-s1' strip grabber specific information from the file\n");
   printf("\t'-s2' strip all object properties and names from the file\n");
   printf("\t'-s-PROP' do not strip object property PROP from the file\n");
   printf("\t'-t type' sets the object type when adding files\n");
   printf("\t'-transparency' preserves transparency through color conversion\n");
   printf("\t'-u' updates the contents of the datafile\n");
   printf("\t'-v' selects verbose mode\n");
   printf("\t'-w' always updates the entire contents of the datafile\n");
   printf("\t'-x' alias for -e\n");
   printf("\t'-007 password' sets the file encryption key\n");
   printf("\t'PROP=value' sets object properties\n");
}



/* callback for outputting messages */
void datedit_msg(AL_CONST char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   printf("%s\n", buf);
}



/* callback for starting a 2-part message output */
void datedit_startmsg(AL_CONST char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   printf("%s", buf);
   fflush(stdout);
}



/* callback for ending a 2-part message output */
void datedit_endmsg(AL_CONST char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   printf("%s\n", buf);
}



/* callback for printing errors */
void datedit_error(AL_CONST char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   fprintf(stderr, "%s\n", buf);

   err = 1;
}



/* callback for asking questions */
int datedit_ask(AL_CONST char *fmt, ...)
{
   va_list args;
   char buf[1024];
   int c;

   static int all = FALSE;

   if (all)
      return 'y';

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   printf("%s? (y/n/a/q) ", buf);
   fflush(stdout);

   for (;;) {
      #ifdef DAT_HAVE_CONIO_H

	 /* raw keyboard input for platforms that have conio functions */
	 c = getch();
	 if ((c == 0) || (c == 0xE0))
	    getch();

      #else

	 /* stdio version for other systems */
	 fflush(stdin);
	 c = getchar();

      #endif

      switch (c) {

	 case 'y':
	 case 'Y':
	    #ifdef DAT_HAVE_CONIO_H
	       printf("%c\n", c);
	    #endif
	    return 'y';

	 case 'n':
	 case 'N':
	    #ifdef DAT_HAVE_CONIO_H
	       printf("%c\n", c);
	    #endif
	    return 'n';

	 case 'a':
	 case 'A':
	    #ifdef DAT_HAVE_CONIO_H
	       printf("%c\n", c);
	    #endif
	    all = TRUE;
	    return 'y';

	 case 'q':
	 case 'Q':
	    #ifdef DAT_HAVE_CONIO_H
	       printf("%c\n", c);
	    #endif
	    return 27;

	 case 27:
	    #ifdef DAT_HAVE_CONIO_H
	       printf("\n");
	    #endif
	    return 27;
      }
   }
}



/* callback for the datedit functions to show a list of options */
/* Returns -1 if canceled */
int datedit_select(AL_CONST char *(*list_getter)(int index, int *list_size), AL_CONST char *fmt, ...)
{
   va_list args;
   char buf[1024];
   int c, num;

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   /* If there is only one choice, select it automatically. If the list is */
   /*  empty, return immediately. */
   list_getter(-1, &num);
   if (num<=1) return num-1;
   
   printf("%s:\n", buf);
   for (c=0; c<num; c++) {
      printf("% 2d %s\n", c, list_getter(c, NULL));
   }
   printf("% 2d %s\n", -1, "Cancel");
   fflush(stdout);
   fscanf(stdin, "%d", &c);
   if (c>num)
      c = num-1;
   return c;
}



/* checks if a string is one of the names specified on the command line */
static int is_name(AL_CONST char *name)
{
   char str1[256], str2[256];
   int i, e;

   for (i=0; i<opt_numnames; i++) {
      if ((stricmp(name, opt_namelist[i]) == 0) ||
	  (strcmp(opt_namelist[i], "*") == 0)) {
	 opt_usedname[i] = TRUE;
	 return TRUE;
      }
      else {
	 for (e=0; e<(int)strlen(opt_namelist[i]); e++) {
	    if (opt_namelist[i][e] == '*') {
	       strncpy(str1, opt_namelist[i], e);
	       str1[e] = 0;
	       strncpy(str2, name, e);
	       str2[e] = 0;
	       if (strcmp(str1, str2) == 0) {
		  opt_usedname[i] = TRUE;
		  return TRUE;
	       }
	    }
	 }
      }
   }

   return FALSE;
}



/* does a view operation */
static void do_view(AL_CONST DATAFILE *dat, AL_CONST char *parentname)
{
   int i, j;
   AL_CONST char *name;
   DATAFILE_PROPERTY *prop;
   char tmp[256];

   for (i=0; dat[i].type != DAT_END; i++) {
      name = get_datafile_property(dat+i, DAT_NAME);
      strcpy(tmp, parentname);
      strcat(tmp, name);

      if ((opt_numnames <= 0) || (is_name(tmp))) {
	 if (opt_verbose)
	    printf("\n");

	 printf("- %c%c%c%c - %s%-*s - %s\n", 
		  (dat[i].type>>24)&0xFF, (dat[i].type>>16)&0xFF,
		  (dat[i].type>>8)&0xFF, dat[i].type&0xFF,
		  parentname, 28-(int)strlen(parentname), name, 
		  datedit_desc(dat+i));

	 if (opt_verbose) {
	    prop = dat[i].prop;
	    if (prop) {
	       for (j=0; prop[j].type != DAT_END; j++) {
		  printf("  . %c%c%c%c '%s'\n", 
			   (prop[j].type>>24)&0xFF, (prop[j].type>>16)&0xFF,
			   (prop[j].type>>8)&0xFF, prop[j].type&0xFF,
			   prop[j].dat);
	       }
	    }
	 }
      }

      if (dat[i].type == DAT_FILE) {
	 strcat(tmp, "/");
	 do_view((DATAFILE *)dat[i].dat, tmp);
      }
   }
}



/* does an export operation */
static void do_export(DATAFILE *dat, char *parentname)
{
   int i;
   AL_CONST char *name;
   char tmp[256];

   for (i=0; dat[i].type != DAT_END; i++) {
      name = get_datafile_property(dat+i, DAT_NAME);
      strcpy(tmp, parentname);
      strcat(tmp, name);

      if (is_name(tmp)) {
	 if (!datedit_export(dat+i, opt_outputname)) {
	    err = 1;
	    return;
	 }
      }
      else {
	 if (dat[i].type == DAT_FILE) {
	    strcat(tmp, "/");
	    do_export((DATAFILE *)dat[i].dat, tmp);
	    if (err)
	       return;
	 }
      }
   }
}



/* deletes objects from the datafile */
static void do_delete(DATAFILE **dat, char *parentname)
{
   int i;
   AL_CONST char *name;
   char tmp[256];

   for (i=0; (*dat)[i].type != DAT_END; i++) {
      name = get_datafile_property((*dat)+i, DAT_NAME);
      strcpy(tmp, parentname);
      strcat(tmp, name);

      if (is_name(tmp)) {
	 printf("Deleting %s\n", tmp);
	 *dat = datedit_delete(*dat, i);
	 changed = TRUE;
	 i--;
      }
      else {
	 if ((*dat)[i].type == DAT_FILE) {
	    DATAFILE *dattmp = (*dat)[i].dat;
	    strcat(tmp, "/");
	    do_delete(&dattmp, tmp);
	    (*dat)[i].dat = dattmp;
	 }
      }
   }
}



/* open a sub-datafile in the parent datafile */
static DATAFILE *open_sub_datafile(DATAFILE *parent, AL_CONST char *name)
{
   DATAFILE *dat;
   int c;

   /* Return the datafile if it already exists. */
   for (c=0; parent[c].type != DAT_END; c++) {
      if ((parent[c].type == DAT_FILE) && (stricmp(name, get_datafile_property(parent+c, DAT_NAME)) == 0))
	 return parent[c].dat;
   }

   /* Otherwise create a new datafile. */
   printf("Creating sub-datafile %s\n", name);

   dat = malloc(sizeof(DATAFILE));
   dat->dat = NULL;
   dat->type = DAT_END;
   dat->size = 0;
   dat->prop = NULL;

   return dat;
}



/* close a sub-datafile from the parent datafile */
static DATAFILE *close_sub_datafile(DATAFILE *parent, AL_CONST char *name, DATAFILE *dat)
{
   int c;

   /* Adjust the datafile if it already exists. */
   for (c=0; parent[c].type != DAT_END; c++) {
      if ((parent[c].type == DAT_FILE) && (stricmp(name, get_datafile_property(parent+c, DAT_NAME)) == 0)) {
	 parent[c].dat = dat;
	 return parent;
      }
   }

   /* Otherwise insert the datafile. */
   return datedit_insert(parent, NULL, name, DAT_FILE, dat, 0);
}



/* adds a file to the archive */
static int do_add_file(AL_CONST char *filename, int attrib, void *param)
{
   DATEDIT_GRAB_PARAMETERS params;
   DATAFILE **dat = param;
   DATAFILE *d;
   char fname[256];
   char name[256];
   int c;

   canonicalize_filename(fname, filename, sizeof(fname));   

   strcpy(name, get_filename(fname));

   if ((strcmp(name, ".") == 0) || (strcmp(name, "..") == 0))
      return 0;

   if (!opt_keepnames) {
      strupr(name);

      for (c=0; name[c]; c++)
	 if (name[c] == '.')
	    name[c] = '_';
   }

   if (attrib & FA_DIREC) {
      DATAFILE *sub_dat = open_sub_datafile(*dat, name);

      if (!sub_dat) {
	 fprintf(stderr, "Error: failed to create sub-datafile %s\n", name);
	 err = 1;
	 return -1;
      }

      append_filename(fname, fname, "*", sizeof(fname));

      if (for_each_file_ex(fname, 0, ~attrib_ok, do_add_file, &sub_dat) <= 0) {
	 fprintf(stderr, "Error: failed to add objects to %s\n", name);
	 err = 1;
	 return -1;
      }

      *dat = close_sub_datafile(*dat, name, sub_dat);
      changed = TRUE;
      return 0;
   }

   params.datafile = canonical_datafilename;
   params.filename = fname;
   params.name = name;
   params.type = datedit_clean_typename(opt_objecttype);
   params.x = opt_gridx;
   params.y = opt_gridy;
   params.w = opt_gridw;
   params.h = opt_gridh;
   params.colordepth = opt_colordepth;
   params.relative = opt_relf;

   for (c=0; (*dat)[c].type != DAT_END; c++) {
      if (stricmp(name, get_datafile_property(*dat+c, DAT_NAME)) == 0) {
	 printf("Replacing %s -> %s\n", fname, name);
	 if (!datedit_grabreplace(*dat+c, &params)) {
	    err = 1;
	    return -1;
	 }
	 else {
	    changed = TRUE;
	    return 0;
	 }
      }
   }

   printf("Inserting %s -> %s\n", fname, name);
   d = datedit_grabnew(*dat, &params);
   if (!d) {
      err = 1;
      return -1;
   }
   else {
      *dat = d;
      changed = TRUE;
      return 0;
   }
}



/* does an update operation */
static void do_update(DATAFILE *dat, int force, char *parentname)
{
   int i;
   AL_CONST char *name;
   char tmp[256];

   for (i=0; dat[i].type != DAT_END; i++) {
      name = get_datafile_property(dat+i, DAT_NAME);
      strcpy(tmp, parentname);
      strcat(tmp, name);

      if (dat[i].type == DAT_FILE) {
	 strcat(tmp, "/");
	 do_update((DATAFILE *)dat[i].dat, force, tmp);
	 if (err)
	    return;
      }
      else if ((opt_numnames <= 0) || (is_name(tmp))) {
	 if (!datedit_update(dat+i, canonical_datafilename, force, opt_verbose, &changed)) {
	    err = 1;
	    return;
	 }
      }
   }
}



/* changes object properties */
static void do_set_props(DATAFILE *dat, char *parentname)
{
   int i, j;
   AL_CONST char *name;
   char tmp[256];
   char propname[256], *propvalue;
   int type;

   for (i=0; dat[i].type != DAT_END; i++) {
      name = get_datafile_property(dat+i, DAT_NAME);
      strcpy(tmp, parentname);
      strcat(tmp, name);

      if (is_name(tmp)) {
	 for (j=0; j<opt_numprops; j++) {
	    strcpy(propname, opt_proplist[j]);
	    propvalue = strchr(propname, '=');
	    if (propvalue) {
	       *propvalue = 0;
	       propvalue++;
	    }
	    else
	       propvalue = "";

	    type = datedit_clean_typename(propname);

	    if (opt_verbose) {
	       if (*propvalue) {
		  printf("%s: setting property %c%c%c%c = '%s'\n", 
			 tmp, type>>24, (type>>16)&0xFF, 
			 (type>>8)&0xFF, type&0xFF, propvalue);
	       }
	       else {
		  printf("%s: clearing property %c%c%c%c\n", 
			 tmp, type>>24, (type>>16)&0xFF, 
			 (type>>8)&0xFF, type&0xFF);
	       }
	    } 

	    datedit_set_property(dat+i, type, propvalue);
	    changed = TRUE;
	 }
      }

      if (dat[i].type == DAT_FILE) {
	 strcat(tmp, "/");
	 do_set_props((DATAFILE *)dat[i].dat, tmp);
      }
   }
}



/* selects a specific palette */
static void do_setpal(DATAFILE **dat, char *parentname)
{
   int i;
   AL_CONST char *name;
   char tmp[256];

   for (i=0; (*dat)[i].type != DAT_END; i++) {
      name = get_datafile_property((*dat)+i, DAT_NAME);
      strcpy(tmp, parentname);
      strcat(tmp, name);

      if (stricmp(tmp, opt_palette) == 0) {
	 if ((*dat)[i].type != DAT_PALETTE) {
	    printf("Error: %s is not a palette object\n", tmp);
	    err = 1;
	    return;
	 }
	 printf("Using palette %s\n", tmp);
	 memcpy(datedit_current_palette, (*dat)[i].dat, sizeof(PALETTE));
	 select_palette(datedit_current_palette);
	 return;
      }
      else {
	 if ((*dat)[i].type == DAT_FILE) {
	    DATAFILE *dattmp = (*dat)[i].dat;
	    strcat(tmp, "/");
	    do_setpal(&dattmp, tmp);
	    (*dat)[i].dat = dattmp;
	 }
      }
   }

   printf("Error: %s not found\n", opt_palette);
   err = 1;
}



/* recursive helper for writing out datafile dependencies */
static void save_dependencies(DATAFILE *dat, FILE *f, int *depth)
{
   AL_CONST char *orig;
   int c, i;
   int hasspace;

   for (c=0; dat[c].type != DAT_END; c++) {
      orig = get_datafile_property(dat+c, DAT_ORIG);

      if ((orig) && (orig[0])) {
	 if (*depth + strlen(orig) > 56) {
	    fprintf(f, " \\\n\t\t");
	    *depth = 0;
	 }
	 else {
	    fprintf(f, " ");
	    (*depth)++;
	 }

	 if (strchr(orig, ' ')) {
	    hasspace = TRUE;
	    fputc('"', f);
	    (*depth) += 2;
	 }
	 else
	    hasspace = FALSE;

	 for (i=0; orig[i]; i++) {
	    if (orig[i] == '\\')
	       fputc('/', f);
	    else
	       fputc(orig[i], f);
	    (*depth)++;
	 }

	 if (hasspace)
	    fputc('"', f);
      }

      if (dat[c].type == DAT_FILE)
	 save_dependencies((DATAFILE *)dat[c].dat, f, depth);
   }
}



/* writes out a makefile dependency rule */
static int do_save_dependencies(DATAFILE *dat, char *srcname, char *depname)
{
   char *pretty_name;
   char tm[80];
   time_t now;
   FILE *f;
   int c;

   pretty_name = datedit_pretty_name(srcname, "dat", FALSE);
   datedit_msg("Writing makefile dependencies into %s", depname);

   f = fopen(depname, "w");
   if (f) {
      time(&now);
      strcpy(tm, asctime(localtime(&now)));
      for (c=0; tm[c]; c++)
	 if ((tm[c] == '\r') || (tm[c] == '\n'))
	    tm[c] = 0;

      fprintf(f, "# Allegro datafile make dependencies, produced by dat v" ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR "\n");
      fprintf(f, "# Datafile: %s\n", pretty_name);
      fprintf(f, "# Date: %s\n", tm);
      fprintf(f, "# Do not hand edit!\n\n");

      if (opt_headername)
	 fprintf(f, "%s %s :", pretty_name, opt_headername);
      else
	 fprintf(f, "%s :", pretty_name);

      c = 0xFF;
      save_dependencies(dat, f, &c);

      fprintf(f, " \\\n\n\tdat -u %s", pretty_name);

      if (opt_headername)
	 fprintf(f, " -h %s", opt_headername);

      if (opt_password)
	 fprintf(f, " -007 %s", opt_password);

      fprintf(f, "\n");
      fclose(f);
   }
   else {
      datedit_error("Error writing %s", depname);
      return FALSE;
   }

   return TRUE;
}



int main(int argc, char *argv[])
{
   int c, colorconv_mode = 0;
   int *opt_fixed_prop = NULL;
   DATAFILE *datafile = NULL;

   if (install_allegro(SYSTEM_NONE, &errno, atexit) != 0)
      return 1;
   datedit_init();

   for (c=0; c<PAL_SIZE; c++)
      datedit_current_palette[c] = desktop_palette[c];

   for (c=1; c<argc; c++) {
      if (argv[c][0] == '-') {

	 switch (utolower(argv[c][1])) {

	    case 'd':
	       if (stricmp(argv[c]+2, "ither") == 0) {
		  colorconv_mode |= COLORCONV_DITHER;
		  break;
	       }
	       /* fall through */

	    case 'a':
	    case 'e':
	    case 'l':
	    case 'u':
	    case 'w':
	    case 'x':
	       if (opt_command) {
		  usage();
		  return 1;
	       }
	       opt_command = utolower(argv[c][1]);
	       break;

	    case 'b':
	       if ((opt_colordepth > 0) || (c >= argc-1)) {
		  usage();
		  return 1;
	       }
	       opt_colordepth = atoi(argv[++c]);
	       if ((opt_colordepth != 8) && (opt_colordepth != 15) &&
		   (opt_colordepth != 16) && (opt_colordepth != 24) &&
		   (opt_colordepth != 32)) {
		  usage();
		  return 1;
	       }
	       break;

	    case 'c':
	       if ((opt_compression >= 0) || 
		   (argv[c][2] < '0') || (argv[c][2] > '2')) {
		  usage();
		  return 1;
	       }
	       opt_compression = argv[c][2] - '0'; 
	       break;

	    case 'f':
	       opt_relf = TRUE;
	       break;

	    case 'g':
	       if ((opt_gridx > 0) || (c >= argc-4)) {
		  usage();
		  return 1;
	       }
	       opt_gridx = atoi(argv[++c]);
	       opt_gridy = atoi(argv[++c]);
	       opt_gridw = atoi(argv[++c]);
	       opt_gridh = atoi(argv[++c]);
	       if ((opt_gridx <  0) || (opt_gridy <  0) ||
		   (opt_gridw <= 0) || (opt_gridh <= 0)) {
		  usage();
		  return 1;
	       }
	       break;

	    case 'h':
	       if ((opt_headername) || (c >= argc-1)) {
		  usage();
		  return 1;
	       }
	       opt_headername = argv[++c];
	       break;

	    case 'k':
	       opt_keepnames = TRUE;
	       break;

	    case 'm':
	       if ((opt_dependencyfile) || (c >= argc-1)) {
		  usage();
		  return 1;
	       }
	       opt_dependencyfile = argv[++c];
	       break;

	    case 'n':
	       if ((opt_sort >= 0) ||
		   (argv[c][2] < '0') || (argv[c][2] > '1')) {
		  usage();
		  return 1;
	       }
	       opt_sort = argv[c][2] - '0';
	       break;

	    case 'o':
	       if ((opt_outputname) || (c >= argc-1)) {
		  usage();
		  return 1;
	       }
	       opt_outputname = argv[++c];
	       break;

	    case 'p':
	       if ((utolower(argv[c][2]) == 'a') && 
		   (utolower(argv[c][3]) == 'l') &&
		   (argv[c][4] == 0)) {
		  if ((opt_palette) || (c >= argc-1)) {
		     usage();
		     return 1;
		  }
		  opt_palette = argv[++c];
	       }
	       else {
		  if ((opt_prefixstring) || (c >= argc-1)) {
		     usage();
		     return 1;
		  }
		  opt_prefixstring = argv[++c];
	       }
	       break;

	    case 'r':
	       attrib_ok |= FA_DIREC;
	       break;

	    case 's':
	       if (argv[c][2] == '-') {
		  allocate(opt_numfixedprops + 1);
		  opt_fixedproplist[opt_numfixedprops++] = argv[c]+3;
	       }
	       else {
		  if ((opt_strip >= 0) || 
		      (argv[c][2] < '0') || (argv[c][2] > '2')) {
		     usage();
		     return 1;
	          }
	          opt_strip = argv[c][2] - '0';
	       }
	       break;

	    case 't':
	       if (stricmp(argv[c]+2, "ransparency") == 0) {
		  colorconv_mode |= COLORCONV_KEEP_TRANS;
		  break;
	       }

	       if ((opt_objecttype) || (c >= argc-1)) {
		  usage();
		  return 1;
	       }
	       opt_objecttype = argv[++c];
	       break;

	    case 'v':
	       opt_verbose = TRUE;
	       break;

	    case '0':
	       if ((opt_password) || (c >= argc-1)) {
		  usage();
		  return 1;
	       }
	       opt_password = argv[++c];
	       break;

	    case '-':
	       if (stricmp(argv[c]+2, "help") == 0) {
	          usage();
	          return 1;
	       }
	       break;

	    case '?':
	       usage();
	       return 1;

	    default:
	       printf("Unknown option '%s'\n", argv[c]);
	       return 1;
	 }
      }
      else {
	 if (strchr(argv[c], '=')) {
	    allocate(opt_numprops + 1);
	    opt_proplist[opt_numprops++] = argv[c];
	 }
	 else {
	    if (!opt_datafilename)
	       opt_datafilename = argv[c];
	    else {
	       allocate(opt_numnames + 1);
	       opt_namelist[opt_numnames] = argv[c];
	       opt_usedname[opt_numnames] = FALSE;
	       opt_numnames++;
	    }
	 }
      }
   }

   if ((!opt_datafilename) || 
       ((!opt_command) && 
	(opt_compression < 0) && 
	(opt_strip < 0) && 
	(opt_sort < 0) &&
	(!opt_numprops) &&
	(!opt_headername) &&
	(!opt_dependencyfile))) {
      usage();
      return 1;
   }

   canonicalize_filename(canonical_datafilename, opt_datafilename, sizeof(canonical_datafilename));

   if (colorconv_mode)
      set_color_conversion(colorconv_mode);
   else
      set_color_conversion(COLORCONV_NONE);

   datafile = datedit_load_datafile(opt_datafilename, FALSE, opt_password);

   if (datafile) {

      if (opt_palette)
	 do_setpal(&datafile, "");

      if (!err) {
	 switch (opt_command) {

	    case 'a':
	       if (!opt_numnames) {
		  printf("No files specified for addition\n");
		  err = 1;
	       }
	       else {
		  for (c=0; c<opt_numnames; c++) {
		     if (for_each_file_ex(opt_namelist[c], 0, ~attrib_ok, do_add_file, &datafile) <= 0) {
			fprintf(stderr, "Error: %s not found\n", opt_namelist[c]);
			err = 1;
			break;
		     }
		     else
			opt_usedname[c] = TRUE;
		  }
	       }
	       break;

	    case 'd':
	       if (!opt_numnames) {
		  printf("No objects specified for deletion\n");
		  err = 1;
	       }
	       else
		  do_delete(&datafile, "");
	       break;

	    case 'e':
	    case 'x':
	       if (!opt_numnames) {
		  printf("No objects specified: use '*' to extract everything\n");
		  err = 1;
	       }
	       else
		  do_export(datafile, "");
	       break;

	    case 'l':
	       do_view(datafile, "");
	       break;

	    case 'u':
	       do_update(datafile, FALSE, "");
	       break;

	    case 'w':
	       do_update(datafile, TRUE, "");
	       break;
	 }
      }

      if (!err) {
	 if (opt_command) {
	    for (c=0; c<opt_numnames; c++) {
	       if (!opt_usedname[c]) {
		  fprintf(stderr, "Error: %s not found\n", opt_namelist[c]);
		  err = 1;
	       }
	    }
	 }

	 if (opt_numprops > 0) {
	    if (!opt_numnames) {
	       printf("No objects specified for setting properties\n");
	       err = 1;
	    }
	    else {
	       for (c=0; c<opt_numnames; c++)
		  opt_usedname[c] = FALSE;

	       do_set_props(datafile, "");

	       for (c=0; c<opt_numnames; c++) {
		  if (!opt_usedname[c]) {
		     fprintf(stderr, "Error: %s not found\n", opt_namelist[c]);
		     err = 1;
		  }
	       }
	    }
	 }

	 if (opt_numfixedprops > 0) {
	    if (opt_strip < 0) {
	       printf("Error: no strip mode\n");
	       err = 1;
	    }
	    else {
	       opt_fixed_prop = malloc((opt_numfixedprops+1)*sizeof(int));

	       for (c=0; c<opt_numfixedprops; c++)
		  opt_fixed_prop[c] = datedit_clean_typename(opt_fixedproplist[c]);

	       opt_fixed_prop[c] = 0;  /* end of array delimiter */
	    }
	 }
      }

      if ((!err) && ((changed) || (opt_compression >= 0) || (opt_strip >= 0) || (opt_sort >= 0))) {
	 DATEDIT_SAVE_DATAFILE_OPTIONS options;

	 options.pack = opt_compression;
	 options.strip = opt_strip;
	 options.sort = opt_sort;
	 options.verbose = opt_verbose;
	 options.write_msg = TRUE;
	 options.backup = FALSE;

	 if (!datedit_save_datafile(datafile, opt_datafilename, opt_fixed_prop, &options, opt_password))
	    err = 1;
      }

      if ((!err) && (opt_headername))
	 if (!datedit_save_header(datafile, opt_datafilename, opt_headername, "dat", opt_prefixstring, opt_verbose))
	    err = 1;

      if ((!err) && (opt_dependencyfile))
	 if (!do_save_dependencies(datafile, opt_datafilename, opt_dependencyfile))
	    err = 1;

      if (opt_fixed_prop)
	 free(opt_fixed_prop);

      unload_datafile(datafile);
   }

   return err;
}

END_OF_MAIN()
