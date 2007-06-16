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
 *      Utility for appending data onto your program executable.
 *      This information can be read by passing the special "#" filename
 *      to the packfile functions.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#define ALLEGRO_USE_CONSOLE

#include <stdio.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"

#ifdef ALLEGRO_UNIX
#include <sys/stat.h>
#include <unistd.h>
#endif



void usage(void)
{
   printf("\nExecutable data appendation utility for Allegro " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR);
   printf("\nBy Shawn Hargreaves, " ALLEGRO_DATE_STR "\n\n");
 #if (defined ALLEGRO_DOS) || (defined ALLEGRO_WINDOWS)
   printf("Usage: exedat [options] program.exe [filename]\n\n");
 #else
   printf("Usage: exedat [options] program [filename]\n\n");
 #endif
   printf("Options:\n");
   printf("\t'-a' use the Allegro packfile format\n");
   printf("\t'-b' use raw binary format\n");
   printf("\t'-c' compress the appended data\n");
   printf("\t'-d' remove any currently appended data\n");
   printf("\t'-x' extract appended data into the output filename\n");
   printf("\t'-007 password' sets the datafile password\n");
}



static int err = 0;

char *opt_filename = NULL;
char *opt_dataname = NULL;
char *opt_password = NULL;

int opt_allegro = FALSE;
int opt_binary = FALSE;
int opt_compress = FALSE;
int opt_delete = FALSE;
int opt_extract = FALSE;

int stat_hasdata = FALSE;
int stat_exe_size = 0;
int stat_compressed_size = 0;
int stat_uncompressed_size = 0;

#ifdef ALLEGRO_UNIX
struct stat stat_stat;
#endif



void get_stats(void)
{
   PACKFILE *f;

 #ifdef ALLEGRO_UNIX
   if (stat(opt_filename, &stat_stat)) {
      fprintf(stderr, "Error statting %s\n", opt_filename);
      err = 1;
      return;
   }
 #endif

   f = pack_fopen(opt_filename, F_READ);
   if (!f) {
      fprintf(stderr, "Error opening %s\n", opt_filename);
      err = 1;
      return;
   }

   stat_exe_size = f->normal.todo;

   pack_fseek(f, f->normal.todo-8);

   if (pack_mgetl(f) != F_EXE_MAGIC) {
      stat_hasdata = FALSE;
      stat_compressed_size = 0;
      stat_uncompressed_size = 0;
      pack_fclose(f);
      return;
   }

   stat_hasdata = TRUE;
   stat_compressed_size = pack_mgetl(f) - 16;

   pack_fclose(f);
   f = pack_fopen(opt_filename, F_READ);
   if (!f) {
      fprintf(stderr, "Error reading %s\n", opt_filename);
      err = 1;
      return;
   }

   stat_exe_size = f->normal.todo - stat_compressed_size - 16;

   pack_fseek(f, stat_exe_size);

   f = pack_fopen_chunk(f, FALSE);
   if (!f) {
      fprintf(stderr, "Error reading %s\n", opt_filename);
      err = 1;
      return;
   }

   stat_uncompressed_size = f->normal.todo;

   pack_fclose(f);
}



int rename_file(char *oldname, char *newname)
{
   PACKFILE *oldfile, *newfile;
   int c;

   oldfile = pack_fopen(oldname, F_READ);
   if (!oldfile)
      return -1;

   newfile = pack_fopen(newname, F_WRITE);
   if (!newfile) {
      pack_fclose(oldfile);
      return -1;
   }

   c = pack_getc(oldfile);

   while (c != EOF) {
      pack_putc(c, newfile);
      c = pack_getc(oldfile);
   } 

   pack_fclose(oldfile);
   pack_fclose(newfile);

   delete_file(oldname); 

   return 0;
}



void update_file(char *filename, char *dataname)
{
   PACKFILE *f, *ef, *df = NULL;
   char *tmpname;
   int c;

   #ifdef ALLEGRO_HAVE_MKSTEMP

      char tmp_buf[] = "XXXXXX";
      char tmp[512];
      int tmp_fd;

      tmp_fd = mkstemp(tmp_buf);
      close(tmp_fd);
      tmpname = uconvert_ascii(tmp_buf, tmp);

   #else

      tmpname = tmpnam(NULL);

   #endif

   get_stats();

   if (!err) {
      if ((!stat_hasdata) && (!dataname)) {
	 fprintf(stderr, "%s has no appended data: cannot remove it\n", filename);
	 err = 1;
	 return;
      }

      rename_file(filename, tmpname);

      ef = pack_fopen(tmpname, F_READ);
      if (!ef) {
	 delete_file(tmpname);
	 fprintf(stderr, "Error reading temporary file\n");
	 err = 1;
	 return;
      }

      f = pack_fopen(filename, F_WRITE);
      if (!f) {
	 pack_fclose(ef);
	 rename_file(tmpname, filename);
	 fprintf(stderr, "Error writing to %s\n", filename);
	 err = 1;
	 return;
      }

   #ifdef ALLEGRO_UNIX
      chmod(filename, stat_stat.st_mode);
   #endif

      for (c=0; c<stat_exe_size; c++)
	 pack_putc(pack_getc(ef), f);

      pack_fclose(ef);
      delete_file(tmpname);

      if (dataname) {
	 if (stat_hasdata)
	    printf("replacing %d bytes of currently appended data\n", stat_uncompressed_size);

	 if (opt_allegro) {
	    printf("reading %s as Allegro format compressed data\n", dataname);
	    df = pack_fopen(dataname, F_READ_PACKED);
	 }
	 else if (opt_binary) {
	    printf("reading %s as raw binary data\n", dataname);
	    df = pack_fopen(dataname, F_READ);
	 }
	 else {
	    printf("autodetecting format of %s: ", dataname);
	    df = pack_fopen(dataname, F_READ_PACKED);
	    if (df) {
	       printf("Allegro compressed data\n");
	    }
	    else {
	       df = pack_fopen(dataname, F_READ);
	       if (df)
		  printf("raw binary data\n");
	       else
		  printf("\n"); 
	    }
	 }

	 if (!df) {
	    pack_fclose(f);
	    fprintf(stderr, "Error opening %s\n", dataname);
	    err = 1;
	    return;
	 }

	 f = pack_fopen_chunk(f, opt_compress);

	 while ((c = pack_getc(df)) != EOF)
	    pack_putc(c, f);

	 f = pack_fclose_chunk(f);

	 pack_mputl(F_EXE_MAGIC, f);
	 pack_mputl(_packfile_filesize+16, f);

	 printf("%s has been appended onto %s\n"
		"original executable size: %d bytes (%dk)\n"
		"appended data size: %d bytes (%dk)\n"
		"compressed into: %d bytes (%dk)\n"
		"ratio: %d%%\n",
		dataname, filename, 
		stat_exe_size, stat_exe_size/1024, 
		_packfile_datasize, _packfile_datasize/1024, 
		_packfile_filesize, _packfile_filesize/1024, 
		(_packfile_datasize) ? _packfile_filesize*100/_packfile_datasize : 0);
      }
      else {
	 printf("%d bytes of appended data removed from %s\n", stat_uncompressed_size, filename);
      }

      pack_fclose(f);
      pack_fclose(df);
   }
}



void extract_data(void)
{
   PACKFILE *f, *df;
   int c;

   get_stats();

   if (!err) {
      if (!stat_hasdata) {
	 fprintf(stderr, "%s has no appended data: cannot extract it\n", opt_filename);
	 err = 1;
	 return;
      }

      f = pack_fopen(opt_filename, F_READ);
      if (!f) {
	 fprintf(stderr, "Error reading %s\n", opt_filename);
	 err = 1;
	 return;
      }

      pack_fseek(f, stat_exe_size);
      f = pack_fopen_chunk(f, FALSE);
      if (!f) {
	 fprintf(stderr, "Error reading %s\n", opt_filename);
	 err = 1;
	 return;
      }

      if (opt_allegro)
	 df = pack_fopen(opt_dataname, (opt_compress ? F_WRITE_PACKED : F_WRITE_NOPACK));
      else
	 df = pack_fopen(opt_dataname, F_WRITE);

      if (!df) {
	 pack_fclose(f);
	 fprintf(stderr, "Error writing %s\n", opt_dataname);
	 err = 1;
	 return;
      }

      while ((c = pack_getc(f)) != EOF)
	 pack_putc(c, df);

      printf("%d bytes extracted from %s into %s\n", stat_uncompressed_size, opt_filename, opt_dataname);

      pack_fclose(f);
      pack_fclose(df);
   }
}



void show_stats(void)
{
   get_stats();

   if (!err) {
      if (stat_hasdata) {
	 printf("%s has appended data\n"
		"original executable size: %d bytes (%dk)\n"
		"appended data size: %d bytes (%dk)\n"
		"compressed into: %d bytes (%dk)\n"
		"ratio: %d%%\n",
		opt_filename, 
		stat_exe_size, stat_exe_size/1024, 
		stat_uncompressed_size, stat_uncompressed_size/1024, 
		stat_compressed_size, stat_compressed_size/1024, 
		(stat_uncompressed_size) ? stat_compressed_size*100/stat_uncompressed_size : 0);
      }
      else {
	 printf("%s has no appended data\n"
		"executable size: %d bytes (%dk)\n",
		opt_filename, stat_exe_size, stat_exe_size/1024);
      }
   }
}



int main(int argc, char *argv[])
{
   int c;

   if (install_allegro(SYSTEM_NONE, &errno, atexit) != 0)
      return 1;

   for (c=1; c<argc; c++) {
      if (argv[c][0] == '-') {
	 switch (utolower(argv[c][1])) {

	    case 'a':
	       opt_allegro = TRUE; 
	       break;

	    case 'b':
	       opt_binary = TRUE;
	       break;

	    case 'c':
	       opt_compress = TRUE;
	       break;

	    case 'd':
	       opt_delete = TRUE;
	       break;

	    case 'x':
	       opt_extract = TRUE;
	       break;

	    case '0':
	       if ((opt_password) || (c >= argc-1)) {
		  usage();
		  return 1;
	       }
	       opt_password = argv[++c];
	       break;

	    default:
	       printf("Unknown option '%s'\n", argv[c]);
	       return 1;
	 }
      }
      else {
	 if (!opt_filename)
	    opt_filename = argv[c];
	 else if (!opt_dataname)
	    opt_dataname = argv[c];
	 else {
	    usage();
	    return 1;
	 }
      }
   }

   if ((!opt_filename) || 
       ((opt_allegro) && (opt_binary)) || 
       ((opt_delete) && (opt_extract))) {
      usage();
      return 1;
   }

   if (opt_password)
      packfile_password(opt_password);

   if (opt_delete) {
      if (opt_dataname) {
	 usage();
	 return 1;
      }
      update_file(opt_filename, NULL);
   }
   else if (opt_extract) {
      if (!opt_dataname) {
	 usage();
	 return 1;
      }
      if ((!opt_allegro) && (!opt_binary)) {
	 printf("The -x option must be used together with -a or -b\n");
	 return 1;
      }
      extract_data();
   }
   else {
      if (opt_dataname)
	 update_file(opt_filename, opt_dataname);
      else
	 show_stats();
   }

   return err;
}

END_OF_MAIN()
