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
 *      File I/O and LZSS compression routines.
 *
 *      By Shawn Hargreaves.
 *
 *      _pack_fdopen() and related modifications by Annie Testes.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"

#ifndef ALLEGRO_MPW
   #include <sys/stat.h>
#endif

#ifdef ALLEGRO_UNIX
   #include <pwd.h>                 /* for tilde expansion */
#endif



#define N            4096           /* 4k buffers for LZ compression */
#define F            18             /* upper limit for LZ match length */
#define THRESHOLD    2              /* LZ encode string into pos and length
				       if match size is greater than this */


typedef struct PACK_DATA            /* stuff for doing LZ compression */
{
   int state;                       /* where have we got to in the pack? */
   int i, c, len, r, s;
   int last_match_length, code_buf_ptr;
   unsigned char mask;
   char code_buf[17];
   int match_position;
   int match_length;
   int lson[N+1];                   /* left children, */
   int rson[N+257];                 /* right children, */
   int dad[N+1];                    /* and parents, = binary search trees */
   unsigned char text_buf[N+F-1];   /* ring buffer, with F-1 extra bytes
				       for string comparison */
} PACK_DATA;


typedef struct UNPACK_DATA          /* for reading LZ files */
{
   int state;                       /* where have we got to? */
   int i, j, k, r, c;
   int flags;
   unsigned char text_buf[N+F-1];   /* ring buffer, with F-1 extra bytes
				       for string comparison */
} UNPACK_DATA;


static int refill_buffer(PACKFILE *f);
static int flush_buffer(PACKFILE *f, int last);
static void pack_inittree(PACK_DATA *dat);
static void pack_insertnode(int r, PACK_DATA *dat);
static void pack_deletenode(int p, PACK_DATA *dat);
static int pack_write(PACKFILE *file, PACK_DATA *dat, int size, unsigned char *buf, int last);
static int pack_read(PACKFILE *file, UNPACK_DATA *dat, int s, unsigned char *buf);

static char the_password[256] = EMPTY_STRING;

int _packfile_filesize = 0;
int _packfile_datasize = 0;

int _packfile_type = 0;


#define FA_DAT_FLAGS  (FA_RDONLY | FA_ARCH)



/* fix_filename_case:
 *  Converts filename to upper case.
 */
char *fix_filename_case(char *filename)
{
   if (!ALLEGRO_LFN)
      ustrupr(filename);

   return filename;
}



/* fix_filename_slashes:
 *  Converts '/' or '\' to system specific path separator.
 */
char *fix_filename_slashes(char *filename)
{
   int pos, c;

   for (pos=0; ugetc(filename+pos); pos+=uwidth(filename+pos)) {
      c = ugetc(filename+pos);
      if ((c == '/') || (c == '\\'))
	 usetat(filename+pos, 0, OTHER_PATH_SEPARATOR);
   }

   return filename;
}



/* fix_filename_path:
 *  Canonicalizes path.
 */
char *fix_filename_path(char *dest, AL_CONST char *path, int size)
{
   int saved_errno = errno;
   char buf[1024], buf2[1024];
   char *p;
   int pos = 0;
   int drive = -1;
   int c1, i;

   #if (DEVICE_SEPARATOR != 0) && (DEVICE_SEPARATOR != '\0')

      /* check whether we have a drive letter */
      c1 = utolower(ugetc(path));
      if ((c1 >= 'a') && (c1 <= 'z')) {
	 int c2 = ugetat(path, 1);
	 if (c2 == DEVICE_SEPARATOR) {
	    drive = c1 - 'a';
	    path += uwidth(path);
	    path += uwidth(path);
	 }
      }

      /* if not, use the current drive */
      if (drive < 0)
	 drive = _al_getdrive();

      pos += usetc(buf+pos, drive+'a');
      pos += usetc(buf+pos, DEVICE_SEPARATOR);

   #endif

   #ifdef ALLEGRO_UNIX

      /* if the path starts with ~ then it's relative to a home directory */
      if ((ugetc(path) == '~')) {
	 AL_CONST char *tail = path + uwidth(path); /* could be the username */
	 char *home = NULL;                /* their home directory */

	 if (ugetc(tail) == '/' || !ugetc(tail)) {
	    /* easy */
	    home = getenv("HOME");
	    if (home)
	       home = strdup(home);
	 }
	 else {
	    /* harder */
	    char *username = (char *)tail, *ascii_username, *ch;
	    int userlen;
	    struct passwd *pwd;

	    /* find the end of the username */
	    tail = ustrchr(username, '/');
	    if (!tail)
	       tail = ustrchr(username, '\0');

	    /* this ought to be the ASCII length, but I can't see a Unicode
	     * function to return the difference in characters between two
	     * pointers. This code is safe on the assumption that ASCII is
	     * the most efficient encoding, but wasteful of memory */
	    userlen = tail - username + ucwidth('\0');
	    ascii_username = malloc(userlen);

	    if (ascii_username) {
	       /* convert the username to ASCII, find the password entry,
		* and copy their home directory. */
	       do_uconvert(username, U_CURRENT, ascii_username, U_ASCII, userlen);

	       if ((ch = strchr(ascii_username, '/')))
		  *ch = '\0';

	       setpwent();

	       while (((pwd = getpwent()) != NULL) && 
		      (strcmp(pwd->pw_name, ascii_username) != 0))
		  ;

	       free(ascii_username);

	       if (pwd)
		  home = strdup(pwd->pw_dir);

	       endpwent();
	    }
	 }

	 /* If we got a home directory, prepend it to the path. Otherwise
	  * we leave the path alone, like bash but not tcsh; bash is better
	  * anyway. :)
	  */
	 if (home) {
	    do_uconvert(home, U_ASCII, buf+pos, U_CURRENT, sizeof(buf)-pos);
	    free(home);
	    pos = ustrsize(buf);
	    path = tail;
	    goto no_relativisation;
	 }
      }

   #endif   /* Unix */

   /* if the path is relative, make it absolute */
   if ((ugetc(path) != '/') && (ugetc(path) != OTHER_PATH_SEPARATOR) && (ugetc(path) != '#')) {
      _al_getdcwd(drive, buf2, sizeof(buf2) - ucwidth(OTHER_PATH_SEPARATOR));
      put_backslash(buf2);

      p = buf2;
      if ((utolower(p[0]) >= 'a') && (utolower(p[0]) <= 'z') && (p[1] == DEVICE_SEPARATOR))
	 p += 2;

      ustrzcpy(buf+pos, sizeof(buf)-pos, p);
      pos = ustrsize(buf);
   }

 #ifdef ALLEGRO_UNIX
   no_relativisation:
 #endif

   /* add our path, and clean it up a bit */
   ustrzcpy(buf+pos, sizeof(buf)-pos, path);

   fix_filename_case(buf);
   fix_filename_slashes(buf);

   /* remove duplicate slashes */
   pos = usetc(buf2, OTHER_PATH_SEPARATOR);
   pos += usetc(buf2+pos, OTHER_PATH_SEPARATOR);
   usetc(buf2+pos, 0);

   while ((p = ustrstr(buf, buf2)) != NULL)
      uremove(p, 0);

   /* remove /./ patterns */
   pos = usetc(buf2, OTHER_PATH_SEPARATOR);
   pos += usetc(buf2+pos, '.');
   pos += usetc(buf2+pos, OTHER_PATH_SEPARATOR);
   usetc(buf2+pos, 0);

   while ((p = ustrstr(buf, buf2)) != NULL) {
      uremove(p, 0);
      uremove(p, 0);
   }

   /* collapse /../ patterns */
   pos = usetc(buf2, OTHER_PATH_SEPARATOR);
   pos += usetc(buf2+pos, '.');
   pos += usetc(buf2+pos, '.');
   pos += usetc(buf2+pos, OTHER_PATH_SEPARATOR);
   usetc(buf2+pos, 0);

   while ((p = ustrstr(buf, buf2)) != NULL) {
      for (i=0; buf+uoffset(buf, i) < p; i++)
	 ;

      while (--i > 0) {
	 c1 = ugetat(buf, i);

	 if (c1 == OTHER_PATH_SEPARATOR)
	    break;

	 if (c1 == DEVICE_SEPARATOR) {
	    i++;
	    break;
	 }
      }

      if (i < 0)
	 i = 0;

      p += ustrsize(buf2);
      memmove(buf+uoffset(buf, i+1), p, ustrsizez(p));
   }

   /* all done! */
   ustrzcpy(dest, size, buf);

   errno = saved_errno;

   return dest;
}



/* replace_filename:
 *  Replaces filename in path with different one.
 *  It does not append '/' to the path.
 */
char *replace_filename(char *dest, AL_CONST char *path, AL_CONST char *filename, int size)
{
   char tmp[1024];
   int pos, c;

   pos = ustrlen(path);

   while (pos>0) {
      c = ugetat(path, pos-1);
      if ((c == '/') || (c == OTHER_PATH_SEPARATOR) || (c == DEVICE_SEPARATOR) || (c == '#'))
	 break;
      pos--;
   }

   ustrzncpy(tmp, sizeof(tmp), path, pos);
   ustrzcat(tmp, sizeof(tmp), filename);

   ustrzcpy(dest, size, tmp);

   return dest;
}



/* replace_extension:
 *  Replaces extension in filename with different one.
 *  It appends '.' if it is not present in the filename.
 */
char *replace_extension(char *dest, AL_CONST char *filename, AL_CONST char *ext, int size)
{
   char tmp[1024], tmp2[16];
   int pos, end, c;

   pos = end = ustrlen(filename);

   while (pos>0) {
      c = ugetat(filename, pos-1);
      if ((c == '.') || (c == '/') || (c == OTHER_PATH_SEPARATOR) || (c == DEVICE_SEPARATOR) || (c == '#'))
	 break;
      pos--;
   }

   if (ugetat(filename, pos-1) == '.')
      end = pos-1;

   ustrzncpy(tmp, sizeof(tmp), filename, end);
   ustrzcat(tmp, sizeof(tmp), uconvert_ascii(".", tmp2));
   ustrzcat(tmp, sizeof(tmp), ext);

   ustrzcpy(dest, size, tmp);

   return dest;
}



/* append_filename:
 *  Append filename to path, adding separator if necessary.
 */
char *append_filename(char *dest, AL_CONST char *path, AL_CONST char *filename, int size)
{
   char tmp[1024];
   int pos, c;

   ustrzcpy(tmp, sizeof(tmp), path);
   pos = ustrlen(tmp);

   if ((pos > 0) && (uoffset(tmp, pos) < ((int)sizeof(tmp) - ucwidth(OTHER_PATH_SEPARATOR) - ucwidth(0)))) {
      c = ugetat(tmp, pos-1);

      if ((c != '/') && (c != OTHER_PATH_SEPARATOR) && (c != DEVICE_SEPARATOR) && (c != '#')) {
	 pos = uoffset(tmp, pos);
	 pos += usetc(tmp+pos, OTHER_PATH_SEPARATOR);
	 usetc(tmp+pos, 0);
      }
   }

   ustrzcat(tmp, sizeof(tmp), filename);

   ustrzcpy(dest, size, tmp);

   return dest;
}



/* get_filename:
 *  When passed a completely specified file path, this returns a pointer
 *  to the filename portion. Both '\' and '/' are recognized as directory
 *  separators.
 */
char *get_filename(AL_CONST char *path)
{
   int pos, c;

   pos = ustrlen(path);

   while (pos>0) {
      c = ugetat(path, pos-1);
      if ((c == '/') || (c == OTHER_PATH_SEPARATOR) || (c == DEVICE_SEPARATOR) || (c == '#'))
	 break;
      pos--;
   }

   return (char *)path + uoffset(path, pos);
}



/* get_extension:
 *  When passed a complete filename (with or without path information)
 *  this returns a pointer to the file extension.
 */
char *get_extension(AL_CONST char *filename)
{
   int pos, c;

   pos = ustrlen(filename);

   while (pos>0) {
      c = ugetat(filename, pos-1);
      if ((c == '.') || (c == '/') || (c == OTHER_PATH_SEPARATOR) || (c == DEVICE_SEPARATOR) || (c == '#'))
	 break;
      pos--;
   }

   if ((pos>0) && (ugetat(filename, pos-1) == '.'))
      return (char *)filename + uoffset(filename, pos);

   return (char *)filename + ustrsize(filename);
}



/* put_backslash:
 *  If the last character of the filename is not a \, /, or #, or a device
 *  separator (eg. : under DOS), this routine will concatenate a \ or / on
 *  to it (depending on platform).
 */
void put_backslash(char *filename)
{
   int c;

   if (ugetc(filename)) {
      c = ugetat(filename, -1);

      if ((c == '/') || (c == OTHER_PATH_SEPARATOR) || (c == DEVICE_SEPARATOR) || (c == '#'))
	 return;
   }

   filename += ustrsize(filename);
   filename += usetc(filename, OTHER_PATH_SEPARATOR);
   usetc(filename, 0);
}



/* pack_fopen_exe_file:
 *  Helper to handle opening files that have been appended to the end of
 *  the program executable.
 */
static PACKFILE *pack_fopen_exe_file(void)
{
   PACKFILE *f;
   char exe_name[1024];
   long size;

   /* open the file */
   get_executable_name(exe_name, sizeof(exe_name));

   if (!ugetc(get_filename(exe_name))) {
      *allegro_errno = ENOENT;
      return NULL;
   }

   f = pack_fopen(exe_name, F_READ);
   if (!f)
      return NULL;

   /* seek to the end and check for the magic number */
   pack_fseek(f, f->todo-8);

   if (pack_mgetl(f) != F_EXE_MAGIC) {
      pack_fclose(f);
      *allegro_errno = ENOTDIR;
      return NULL;
   }

   size = pack_mgetl(f);

   /* rewind */
   pack_fclose(f);
   f = pack_fopen(exe_name, F_READ);
   if (!f)
      return NULL;

   /* seek to the start of the appended data */
   pack_fseek(f, f->todo-size);

   f = pack_fopen_chunk(f, FALSE);

   if (f)
      f->flags |= PACKFILE_FLAG_EXEDAT;

   return f;
}



/* pack_fopen_datafile_object:
 *  Recursive helper to handle opening member objects from datafiles, 
 *  given a fake filename in the form 'object_name[/nestedobject]'.
 */
static PACKFILE *pack_fopen_datafile_object(PACKFILE *f, AL_CONST char *objname)
{
   char buf[512];   /* text is read into buf as UTF-8 */
   char tmp[512*4]; /* this should be enough even when expanding to UCS-4 */
   char name[512];
   int use_next = FALSE;
   int recurse = FALSE;
   int type, size, pos, c;

   /* split up the object name */
   pos = 0;

   while ((c = ugetxc(&objname)) != 0) {
      if ((c == '#') || (c == '/') || (c == OTHER_PATH_SEPARATOR)) {
	 recurse = TRUE;
	 break;
      }
      pos += usetc(name+pos, c);
   }

   usetc(name+pos, 0);

   pack_mgetl(f);

   /* search for the requested object */
   while (!pack_feof(f)) {
      type = pack_mgetl(f);

      if (type == DAT_PROPERTY) {
	 type = pack_mgetl(f);
	 size = pack_mgetl(f);
	 if (type == DAT_NAME) {
	    /* examine name property */
	    pack_fread(buf, size, f);
	    buf[size] = 0;
	    if (ustricmp(uconvert(buf, U_UTF8, tmp, U_CURRENT, sizeof tmp), name) == 0)
	       use_next = TRUE;
	 }
	 else {
	    /* skip property */
	    pack_fseek(f, size);
	 }
      }
      else {
	 if (use_next) {
	    /* found it! */
	    if (recurse) {
	       if (type == DAT_FILE)
		  return pack_fopen_datafile_object(pack_fopen_chunk(f, FALSE), objname);
	       else
		  break;
	    }
	    else {
	       _packfile_type = type;
	       return pack_fopen_chunk(f, FALSE);
	    }
	 }
	 else {
	    /* skip unwanted object */
	    size = pack_mgetl(f);
	    pack_fseek(f, size+4);
	 }
      }
   }

   /* oh dear, the object isn't there... */
   pack_fclose(f);
   *allegro_errno = ENOENT;
   return NULL; 
}



/* pack_fopen_special_file:
 *  Helper to handle opening psuedo-files, ie. datafile objects and data
 *  that has been appended to the end of the executable.
 */
static PACKFILE *pack_fopen_special_file(AL_CONST char *filename, AL_CONST char *mode)
{
   char fname[1024], objname[512], tmp[16];
   PACKFILE *f;
   char *p;
   int c;

   /* special files are read-only */
   while ((c = *(mode++)) != 0) {
      if ((c == 'w') || (c == 'W')) {
	 *allegro_errno = EROFS;
	 return NULL;
      }
   }

   if (ustrcmp(filename, uconvert_ascii("#", tmp)) == 0) {
      /* read appended executable data */
      return pack_fopen_exe_file();
   }
   else {
      if (ugetc(filename) == '#') {
	 /* read object from an appended datafile */
	 ustrzcpy(fname,  sizeof(fname), uconvert_ascii("#", tmp));
	 ustrzcpy(objname, sizeof(objname), filename+uwidth(filename));
      }
      else {
	 /* read object from a regular datafile */
	 ustrzcpy(fname,  sizeof(fname), filename);
	 p = ustrchr(fname, '#');
	 usetat(p, 0, 0);
	 ustrzcpy(objname, sizeof(objname), p+uwidth(p));
      }

      /* open the file */
      f = pack_fopen(fname, F_READ_PACKED);
      if (!f)
	 return NULL;

      if (pack_mgetl(f) != DAT_MAGIC) {
	 pack_fclose(f);
	 *allegro_errno = ENOTDIR;
	 return NULL;
      }

      /* find the required object */
      return pack_fopen_datafile_object(f, objname);
   }
}



/* file_exists:
 *  Checks whether a file matching the given name and attributes exists,
 *  returning non zero if it does. The file attribute may contain any of
 *  the FA_* constants from dir.h. If aret is not null, it will be set 
 *  to the attributes of the matching file. If an error occurs the system 
 *  error code will be stored in errno.
 */
int file_exists(AL_CONST char *filename, int attrib, int *aret)
{
   struct al_ffblk info;
   ASSERT(filename);

   if (ustrchr(filename, '#')) {
      PACKFILE *f = pack_fopen_special_file(filename, F_READ);
      if (f) {
	 pack_fclose(f);
	 if (aret)
	    *aret = FA_DAT_FLAGS;
	 return ((attrib & FA_DAT_FLAGS) == FA_DAT_FLAGS) ? TRUE : FALSE;
      }
      else
	 return FALSE;
   }

   if (!_al_file_isok(filename))
      return FALSE;

   if (al_findfirst(filename, &info, attrib) != 0) {
      /* no entry is not an error for file_exists() */
      if (*allegro_errno == ENOENT)
         errno = *allegro_errno = 0;

      return FALSE;
   }

   al_findclose(&info);

   if (aret)
      *aret = info.attrib;

   return TRUE;
}



/* exists:
 *  Shortcut version of file_exists().
 */
int exists(AL_CONST char *filename)
{
   ASSERT(filename);
   return file_exists(filename, FA_ARCH | FA_RDONLY, NULL);
}



/* file_size:
 *  Returns the size of a file, in bytes.
 *  If the file does not exist or an error occurs, it will return zero
 *  and store the system error code in errno.
 */
long file_size(AL_CONST char *filename)
{
   if (ustrchr(filename, '#')) {
      PACKFILE *f = pack_fopen_special_file(filename, F_READ);
      if (f) {
	 long ret = f->todo;
	 pack_fclose(f);
	 return ret;
      }
      else
	 return 0;
   }

   if (!_al_file_isok(filename))
      return 0;

   return _al_file_size(filename);
}



/* file_time:
 *  Returns a file time-stamp.
 *  If the file does not exist or an error occurs, it will return zero
 *  and store the system error code in errno.
 */
time_t file_time(AL_CONST char *filename)
{
   if (ustrchr(filename, '#')) {
      *allegro_errno = EPERM;
      return 0;
   }

   if (!_al_file_isok(filename))
      return 0;

   return _al_file_time(filename);
}



/* delete_file:
 *  Removes a file from the disk.
 */
int delete_file(AL_CONST char *filename)
{
   char tmp[1024];

   *allegro_errno = 0;

   if (ustrchr(filename, '#')) {
      *allegro_errno = EROFS;
      return *allegro_errno;
   }

   if (!_al_file_isok(filename))
      return *allegro_errno;

   unlink(uconvert_toascii(filename, tmp));
   *allegro_errno = errno;

   return *allegro_errno;
}



/* for_each_file:
 *  Finds all the files on the disk which match the given wildcard
 *  specification and file attributes, and executes callback() once for
 *  each. callback() will be passed three arguments, the first a string
 *  which contains the completed filename, the second being the attributes
 *  of the file, and the third an int which is simply a copy of param (you
 *  can use this for whatever you like). If an error occurs an error code
 *  will be stored in errno, and callback() can cause for_each_file() to
 *  abort by setting errno itself. Returns the number of successful calls
 *  made to callback(). The file attribute may contain any of the FA_* 
 *  flags from dir.h.
 */
int for_each_file(AL_CONST char *name, int attrib, void (*callback)(AL_CONST char *filename, int attrib, int param), int param)
{
   char buf[1024];
   struct al_ffblk info;
   int c = 0;

   if (ustrchr(name, '#')) {
      *allegro_errno = ENOTDIR;
      return 0;
   }

   if (!_al_file_isok(name))
      return 0;

   if (al_findfirst(name, &info, attrib) != 0) {
      /* no entry is not an error for for_each_file() */
      if (*allegro_errno == ENOENT)
         errno = *allegro_errno = 0;

      return 0;
   }

   errno = *allegro_errno = 0;
   do {
      replace_filename(buf, name, info.name, sizeof(buf));
      (*callback)(buf, info.attrib, param);

      if (*allegro_errno)
	 break;

      c++;
   } while (al_findnext(&info) == 0);

   al_findclose(&info);

   /* no entry is not an error for for_each_file() */
   if (*allegro_errno == ENOENT)
      errno = *allegro_errno = 0;

   return c;
}



/* find_resource:
 *  Tries lots of different places that a resource file might live.
 */
static int find_resource(char *dest, AL_CONST char *path, AL_CONST char *name, AL_CONST char *datafile, AL_CONST char *objectname, AL_CONST char *subdir, int size)
{
   char _name[128], _objectname[128], hash[8];
   char tmp[16];
   int i;

   /* convert from name.ext to name_ext (datafile object name format) */
   ustrzcpy(_name, sizeof(_name), name);

   for (i=0; i<ustrlen(_name); i++) {
      if (ugetat(_name, i) == '.')
	 usetat(_name, i, '_');
   }

   if (objectname) {
      ustrzcpy(_objectname, sizeof(_objectname), objectname);

      for (i=0; i<ustrlen(_objectname); i++) {
	 if (ugetat(_objectname, i) == '.')
	    usetat(_objectname, i, '_');
      }
   }
   else
      usetc(_objectname, 0);

   usetc(hash+usetc(hash, '#'), 0);

   /* try path/name */ 
   if (ugetc(name)) {
      ustrzcpy(dest, size, path);
      ustrzcat(dest, size, name);

      if (file_exists(dest, FA_RDONLY | FA_ARCH, NULL))
	 return 0;
   }

   /* try path#_name */
   if ((ustrchr(path, '#')) && (ugetc(name))) {
      ustrzcpy(dest, size, path);
      ustrzcat(dest, size, _name);

      if (file_exists(dest, FA_RDONLY | FA_ARCH, NULL))
	 return 0;
   }

   /* try path/name#_objectname */
   if ((ustricmp(get_extension(name), uconvert_ascii("dat", tmp)) == 0) && (objectname)) {
      ustrzcpy(dest, size, path);
      ustrzcat(dest, size, name);
      ustrzcat(dest, size, hash);
      ustrzcat(dest, size, _objectname);

      if (file_exists(dest, FA_RDONLY | FA_ARCH, NULL))
	 return 0;
   }

   /* try path/datafile#_name */
   if ((datafile) && (ugetc(name))) {
      ustrzcpy(dest, size, path);
      ustrzcat(dest, size, datafile);
      ustrzcat(dest, size, hash);
      ustrzcat(dest, size, _name);

      if (file_exists(dest, FA_RDONLY | FA_ARCH, NULL))
	 return 0;
   }

   /* try path/datafile#_objectname */
   if ((datafile) && (objectname)) {
      ustrzcpy(dest, size, path);
      ustrzcat(dest, size, datafile);
      ustrzcat(dest, size, hash);
      ustrzcat(dest, size, _objectname);

      if (file_exists(dest, FA_RDONLY | FA_ARCH, NULL))
	 return 0;
   }

   /* try path/objectname */ 
   if (objectname) {
      ustrzcpy(dest, size, path);
      ustrzcat(dest, size, objectname);

      if (file_exists(dest, FA_RDONLY | FA_ARCH, NULL))
	 return 0;
   }

   /* try path#_objectname */
   if ((ustrchr(path, '#')) && (objectname)) {
      ustrzcpy(dest, size, path);
      ustrzcat(dest, size, _objectname);

      if (file_exists(dest, FA_RDONLY | FA_ARCH, NULL))
	 return 0;
   }

   /* try path/subdir/objectname */
   if ((subdir) && (objectname)) {
      ustrzcpy(dest, size - ucwidth(OTHER_PATH_SEPARATOR), path);
      ustrzcat(dest, size - ucwidth(OTHER_PATH_SEPARATOR), subdir);
      put_backslash(dest);
      ustrzcat(dest, size, objectname);

      if (file_exists(dest, FA_RDONLY | FA_ARCH, NULL))
	 return 0;
   }

   return -1;
}



/* find_allegro_resource:
 *  Searches for a support file, eg. allegro.cfg or language.dat. Passed
 *  a resource string describing what you are looking for, along with
 *  extra optional information such as the default extension, what datafile
 *  to look inside, what the datafile object name is likely to be, any
 *  special environment variable to check, and any subdirectory that you
 *  would like to check as well as the default location, this function
 *  looks in a hell of a lot of different places :-) Returns zero on
 *  success, and stores a full path to the file (at most size bytes) in
 *  the dest parameter.
 */
int find_allegro_resource(char *dest, AL_CONST char *resource, AL_CONST char *ext, AL_CONST char *datafile, AL_CONST char *objectname, AL_CONST char *envvar, AL_CONST char *subdir, int size)
{
   int (*sys_find_resource)(char *, AL_CONST char *, int);
   char rname[128], path[1024], tmp[128];
   char *s;
   int i, c;

   /* if the resource is a path with no filename, look in that location */
   if ((resource) && (ugetc(resource)) && (!ugetc(get_filename(resource))))
      return find_resource(dest, resource, empty_string, datafile, objectname, subdir, size);

   /* if we have a path+filename, just use it directly */
   if ((resource) && (ustrpbrk(resource, uconvert_ascii("\\/#", tmp)))) {
      if (file_exists(resource, FA_RDONLY | FA_ARCH, NULL)) {
	 ustrzcpy(dest, size, resource);

	 /* if the resource is a datafile, try looking inside it */
	 if ((ustricmp(get_extension(dest), uconvert_ascii("dat", tmp)) == 0) && (objectname)) {
	    ustrzcat(dest, size, uconvert_ascii("#", tmp));

	    for (i=0; i<ustrlen(objectname); i++) {
	       c = ugetat(objectname, i);
	       if (c == '.')
		  c = '_';
	       if (ustrsizez(dest)+ucwidth(c) <= size)
		  uinsert(dest, ustrlen(dest), c);
	    }

	    if (!file_exists(dest, FA_RDONLY | FA_ARCH, NULL))
	       return -1;
	 }

	 return 0;
      }
      else
	 return -1;
   }

   /* clean up the resource name, adding any default extension */
   if (resource) {
      ustrzcpy(rname, sizeof(rname), resource);

      if (ext) {
	 s = get_extension(rname);
	 if (!ugetc(s))
	    ustrzcat(rname, sizeof(rname), ext);
      }
   }
   else
      usetc(rname, 0);

   /* try looking in the same directory as the program */
   get_executable_name(path, sizeof(path));
   usetc(get_filename(path), 0);

   if (find_resource(dest, path, rname, datafile, objectname, subdir, size) == 0)
      return 0;

   /* try the ALLEGRO environment variable */
   s = getenv("ALLEGRO");

   if (s) {
      do_uconvert(s, U_ASCII, path, U_CURRENT, sizeof(path)-ucwidth(OTHER_PATH_SEPARATOR));
      put_backslash(path);

      if (find_resource(dest, path, rname, datafile, objectname, subdir, size) == 0)
	 return 0; 
   }

   /* try any extra environment variable that the parameters say to use */
   if (envvar) {
      s = getenv(uconvert_toascii(envvar, tmp));

      if (s) {
	 do_uconvert(s, U_ASCII, path, U_CURRENT, sizeof(path)-ucwidth(OTHER_PATH_SEPARATOR));
	 put_backslash(path);

	 if (find_resource(dest, path, rname, datafile, objectname, subdir, size) == 0)
	    return 0; 
      }
   }

   /* ask the system driver */
   if (system_driver)
      sys_find_resource = system_driver->find_resource;
   else
      sys_find_resource = NULL;

   if (sys_find_resource) {
      if ((ugetc(rname)) && (sys_find_resource(dest, (char *)rname, size) == 0))
	 return 0;

      if ((datafile) && ((ugetc(rname)) || (objectname)) && (sys_find_resource(path, (char *)datafile, sizeof(path)) == 0)) {
	 if (!ugetc(rname))
	    ustrzcpy(rname, sizeof(rname), objectname);

	 for (i=0; i<ustrlen(rname); i++) {
	    if (ugetat(rname, i) == '.')
	       usetat(rname, i, '_');
	 }

	 ustrzcat(path, sizeof(path), uconvert_ascii("#", tmp));
	 ustrzcat(path, sizeof(path), rname);

	 if (file_exists(path, FA_RDONLY | FA_ARCH, NULL)) {
	    ustrzcpy(dest, size, path);
	    return 0;
	 }
      }
   }

   /* argh, all that work, and still no biscuit */ 
   return -1;
}



/* packfile_password:
 *  Sets the password to be used by all future read/write operations.
 */
void packfile_password(AL_CONST char *password)
{
   int i = 0;
   int c;

   if (password) {
      while ((c = ugetxc(&password)) != 0) {
	 the_password[i++] = c;
	 if (i >= (int)sizeof(the_password)-1)
	    break;
      }
   }

   the_password[i] = 0;
}



/* encrypt_id:
 *  Helper for encrypting magic numbers, using the current password.
 */
static long encrypt_id(long x, int new_format)
{
   long mask = 0;
   int i, pos;

   if (the_password[0]) {
      for (i=0; the_password[i]; i++)
	 mask ^= ((long)the_password[i] << ((i&3) * 8));

      for (i=0, pos=0; i<4; i++) {
	 mask ^= (long)the_password[pos++] << (24-i*8);
	 if (!the_password[pos])
	    pos = 0;
      }

      if (new_format)
	 mask ^= 42;
   }

   return x ^ mask;
}



/* clone_password:
 *  Sets up a local password string for use by this packfile.
 */
static int clone_password(PACKFILE *f)
{
   if (the_password[0]) {
      if ((f->passdata = malloc(strlen(the_password)+1)) == NULL) {
	 *allegro_errno = ENOMEM;
	 return FALSE;
      }
      strcpy(f->passdata, the_password);
   }
   else
      f->passdata = NULL;

   f->passpos = f->passdata;

   return TRUE;
}



/* create_packfile:
 *  Helper function for creating a PACKFILE structure.
 */
static PACKFILE *create_packfile(void)
{
   PACKFILE *f;

   if ((f = malloc(sizeof(PACKFILE))) == NULL) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   f->buf_pos = f->buf;
   f->flags = 0;
   f->buf_size = 0;
   f->filename = NULL;
   f->passdata = NULL;
   f->passpos = NULL;
   f->parent = NULL;
   f->pack_data = NULL;
   f->todo = 0;

   return f;
}



/* create_pack_data:
 *  Helper function for creating a PACK_DATA structure.
 */
static PACK_DATA *create_pack_data(void)
{
   PACK_DATA *dat;
   int c;

   if ((dat = malloc(sizeof(PACK_DATA))) == NULL) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   for (c=0; c < N - F; c++)
      dat->text_buf[c] = 0;

   dat->state = 0;

   return dat;
}



/* create_unpack_data:
 *  Helper function for creating an UNPACK_DATA structure.
 */
static UNPACK_DATA *create_unpack_data(void)
{
   UNPACK_DATA *dat;
   int c;

   if ((dat = malloc(sizeof(UNPACK_DATA))) == NULL) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   for (c=0; c < N - F; c++)
      dat->text_buf[c] = 0;

   dat->state = 0;

   return dat;
}



/* free_packfile:
 *  Helper function for freeing the PACKFILE struct.
 */
static void free_packfile(PACKFILE *f)
{
   if (f) {
      if (f->pack_data)
         free(f->pack_data);

      if (f->passdata)
         free(f->passdata);

      free(f);
   }
}



/* _pack_fdopen:
 *  Converts the given file descriptor into a PACKFILE. The mode can have
 *  the same values as for pack_fopen() and must be compatible with the
 *  mode of the file descriptor. Unlike the libc fdopen(), pack_fdopen()
 *  is unable to convert an already partially read or written file (i.e.
 *  the file offset must be 0).
 *  On success, it returns a pointer to a file structure, and on error it
 *  returns NULL and stores an error code in errno. An attempt to read
 *  a normal file in packed mode will cause errno to be set to EDOM.
 */
PACKFILE *_pack_fdopen(int fd, AL_CONST char *mode)
{
   PACKFILE *f, *f2;
   long header = FALSE;
   int c;

   if ((f = create_packfile()) == NULL)
      return NULL;

   while ((c = *(mode++)) != 0) {
      switch (c) {
	 case 'r': case 'R': f->flags &= ~PACKFILE_FLAG_WRITE; break;
	 case 'w': case 'W': f->flags |= PACKFILE_FLAG_WRITE; break;
	 case 'p': case 'P': f->flags |= PACKFILE_FLAG_PACK; break;
	 case '!': f->flags &= ~PACKFILE_FLAG_PACK; header = TRUE; break;
      }
   }

   if (f->flags & PACKFILE_FLAG_WRITE) {
      if (f->flags & PACKFILE_FLAG_PACK) {
	 /* write a packed file */
	 f->pack_data = create_pack_data();

	 if (!f->pack_data) {
	    free_packfile(f);
	    return NULL;
	 }

	 if ((f->parent = _pack_fdopen(fd, F_WRITE)) == NULL) {
	    free_packfile(f);
	    return NULL;
	 }

	 pack_mputl(encrypt_id(F_PACK_MAGIC, TRUE), f->parent);

	 f->todo = 4;
      }
      else {
	 /* write a 'real' file */
	 if (!clone_password(f)) {
	    free_packfile(f);
	    return NULL;
	 }

         f->hndl = fd;
	 f->todo = 0;

	 errno = 0;

	 if (header)
	    pack_mputl(encrypt_id(F_NOPACK_MAGIC, TRUE), f);
      }
   }
   else { 
      if (f->flags & PACKFILE_FLAG_PACK) {
	 /* read a packed file */
         f->pack_data = create_unpack_data();

         if (!f->pack_data) {
	    free_packfile(f);
	    return NULL;
	 }

         if ((f->parent = _pack_fdopen(fd, F_READ)) == NULL) {
	    free_packfile(f);
	    return NULL;
	 }

	 header = pack_mgetl(f->parent);

	 if ((f->parent->passpos) &&
	     ((header == encrypt_id(F_PACK_MAGIC, FALSE)) ||
	      (header == encrypt_id(F_NOPACK_MAGIC, FALSE)))) {

	    /* duplicate the file descriptor */
	    int fd2 = dup(fd);

	    if (fd2<0) {
	       pack_fclose(f->parent);
	       free_packfile(f);
	       *allegro_errno = errno;
	       return NULL;
            }

	    /* close the parent file (logically, not physically) */
	    pack_fclose(f->parent);

	    /* backward compatibility mode */
	    if (!clone_password(f)) {
	       free_packfile(f);
	       return NULL;
	    }

	    f->flags |= PACKFILE_FLAG_OLD_CRYPT;

	    /* re-open the parent file */
	    lseek(fd2, 0, SEEK_SET);

	    if ((f->parent = _pack_fdopen(fd2, F_READ)) == NULL) {
	       free_packfile(f);
	       return NULL;
	    }

	    f->parent->flags |= PACKFILE_FLAG_OLD_CRYPT;

	    pack_mgetl(f->parent);

	    if (header == encrypt_id(F_PACK_MAGIC, FALSE))
	       header = encrypt_id(F_PACK_MAGIC, TRUE);
	    else
	       header = encrypt_id(F_NOPACK_MAGIC, TRUE);
	 }

	 if (header == encrypt_id(F_PACK_MAGIC, TRUE)) {
	    f->todo = LONG_MAX;
	 }
	 else if (header == encrypt_id(F_NOPACK_MAGIC, TRUE)) {
	    f2 = f->parent;
 	    free_packfile(f);
	    return f2;
	 }
	 else {
	    pack_fclose(f->parent);
	    free_packfile(f);
	    if (*allegro_errno == 0)
	       *allegro_errno = EDOM;
	    return NULL;
	 }
      }
      else {
	 /* read a 'real' file */
	 f->todo = lseek(fd, 0, SEEK_END);  /* size of the file */
	 if (f->todo < 0) {
	    *allegro_errno = errno;
	    free_packfile(f);
	    return NULL;
         }

         lseek(fd, 0, SEEK_SET);

	 if (*allegro_errno) {
            free_packfile(f);
	    return NULL;
	 }

	 if (!clone_password(f)) {
            free_packfile(f);
	    return NULL;
	 }

         f->hndl = fd;
      }
   }

   return f;
}



/* pack_fopen:
 *  Opens a file according to mode, which may contain any of the flags:
 *  'r': open file for reading.
 *  'w': open file for writing, overwriting any existing data.
 *  'p': open file in 'packed' mode. Data will be compressed as it is
 *       written to the file, and automatically uncompressed during read
 *       operations. Files created in this mode will produce garbage if
 *       they are read without this flag being set.
 *  '!': open file for writing in normal, unpacked mode, but add the value
 *       F_NOPACK_MAGIC to the start of the file, so that it can be opened
 *       in packed mode and Allegro will automatically detect that the
 *       data does not need to be decompressed.
 *
 *  Instead of these flags, one of the constants F_READ, F_WRITE,
 *  F_READ_PACKED, F_WRITE_PACKED or F_WRITE_NOPACK may be used as the second 
 *  argument to fopen().
 *
 *  On success, fopen() returns a pointer to a file structure, and on error
 *  it returns NULL and stores an error code in errno. An attempt to read a 
 *  normal file in packed mode will cause errno to be set to EDOM.
 */
PACKFILE *pack_fopen(AL_CONST char *filename, AL_CONST char *mode)
{
   char tmp[1024];
   int fd;

   _packfile_type = 0;

   if (ustrchr(filename, '#'))
      return pack_fopen_special_file(filename, mode);

   if (!_al_file_isok(filename))
      return NULL;

   errno = *allegro_errno = 0;

#ifndef ALLEGRO_MPW
   if (strpbrk(mode, "wW"))  /* write mode? */
      fd = open(uconvert_toascii(filename, tmp), O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
   else
      fd = open(uconvert_toascii(filename, tmp), O_RDONLY | O_BINARY, S_IRUSR | S_IWUSR);
#else
   if (strpbrk(mode, "wW"))  /* write mode? */
      fd = _al_open(uconvert_toascii(filename, tmp), O_WRONLY | O_BINARY | O_CREAT | O_TRUNC);
   else
      fd = _al_open(uconvert_toascii(filename, tmp), O_RDONLY | O_BINARY);
#endif

   if (fd < 0) {
      *allegro_errno = errno;
      return NULL;
   }

   return _pack_fdopen(fd, mode);
}



/* pack_fclose:
 *  Closes a file after it has been read or written.
 *  Returns zero on success. On error it returns an error code which is
 *  also stored in errno. This function can fail only when writing to
 *  files: if the file was opened in read mode it will always succeed.
 */
int pack_fclose(PACKFILE *f)
{
   if (f) {
      if (f->flags & PACKFILE_FLAG_WRITE) {
	 if (f->flags & PACKFILE_FLAG_CHUNK) {
            f = pack_fclose_chunk(f);
            if (!f)
               return *allegro_errno;

            return pack_fclose(f);
         }

	 flush_buffer(f, TRUE);
      }

      if (f->parent)
	 pack_fclose(f->parent);
      else
	 close(f->hndl);

      free_packfile(f);
      *allegro_errno = errno;
      return *allegro_errno;
   }

   return 0;
}



/* pack_fopen_chunk: 
 *  Opens a sub-chunk of the specified file, for reading or writing depending
 *  on the type of the file. The returned file pointer describes the sub
 *  chunk, and replaces the original file, which will no longer be valid.
 *  When writing to a chunk file, data is sent to the original file, but
 *  is prefixed with two length counts (32 bit, big-endian). For uncompressed 
 *  chunks these will both be set to the length of the data in the chunk.
 *  For compressed chunks, created by setting the pack flag, the first will
 *  contain the raw size of the chunk, and the second will be the negative
 *  size of the uncompressed data. When reading chunks, the pack flag is
 *  ignored, and the compression type is detected from the sign of the
 *  second size value. The file structure used to read chunks checks the
 *  chunk size, and will return EOF if you try to read past the end of
 *  the chunk. If you don't read all of the chunk data, when you call
 *  pack_fclose_chunk(), the parent file will advance past the unused data.
 *  When you have finished reading or writing a chunk, you should call
 *  pack_fclose_chunk() to return to your original file.
 */
PACKFILE *pack_fopen_chunk(PACKFILE *f, int pack)
{
   PACKFILE *chunk;
   char tmp[1024];
   char *name;

   if (f->flags & PACKFILE_FLAG_WRITE) {

      /* write a sub-chunk */ 
      int tmp_fd = -1;

      /* the file is open in read/write mode, even if the pack file
       * seems to be in write only mode
       */
      #ifdef HAVE_MKSTEMP

         char tmp_name[] = "XXXXXX";
         tmp_fd = mkstemp(tmp_name);

      #else

         /* note: since the filename creation and the opening are not
          * an atomic operation, this is not secure
          */
         char *tmp_name = tmpnam(NULL);
         if (tmp_name) {
#ifndef ALLEGRO_MPW
            tmp_fd = open(tmp_name, O_RDWR | O_BINARY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
#else
            tmp_fd = _al_open(tmp_name, O_RDWR | O_BINARY | O_CREAT | O_EXCL);
#endif
         }

      #endif

      if (tmp_fd < 0)
         return NULL;

      name = uconvert_ascii(tmp_name, tmp);
      chunk = _pack_fdopen(tmp_fd, (pack ? F_WRITE_PACKED : F_WRITE_NOPACK));

      if (chunk) {
         chunk->filename = ustrdup(name);

	 if (pack)
	    chunk->parent->parent = f;
	 else
	    chunk->parent = f;

	 chunk->flags |= PACKFILE_FLAG_CHUNK;
      }
   }
   else {
      /* read a sub-chunk */
      _packfile_filesize = pack_mgetl(f);
      _packfile_datasize = pack_mgetl(f);

      if ((chunk = create_packfile()) == NULL)
         return NULL;

      chunk->flags = PACKFILE_FLAG_CHUNK;
      chunk->parent = f;

      if (f->flags & PACKFILE_FLAG_OLD_CRYPT) {
	 /* backward compatibility mode */
	 if (f->passdata) {
	    if ((chunk->passdata = malloc(strlen(f->passdata)+1)) == NULL) {
	       *allegro_errno = ENOMEM;
	       free(chunk);
	       return NULL;
	    }
	    strcpy(chunk->passdata, f->passdata);
	    chunk->passpos = chunk->passdata + (long)f->passpos - (long)f->passdata;
	    f->passpos = f->passdata;
	 }
	 chunk->flags |= PACKFILE_FLAG_OLD_CRYPT;
      }

      if (_packfile_datasize < 0) {
	 /* read a packed chunk */
         chunk->pack_data = create_unpack_data();

	 if (!chunk->pack_data) {
            free_packfile(chunk);
	    return NULL;
	 }

	 _packfile_datasize = -_packfile_datasize;
	 chunk->todo = _packfile_datasize;
	 chunk->flags |= PACKFILE_FLAG_PACK;
      }
      else {
	 /* read an uncompressed chunk */
	 chunk->todo = _packfile_datasize;
      }
   }

   return chunk;
}



/* pack_fclose_chunk:
 *  Call after reading or writing a sub-chunk. This closes the chunk file,
 *  and returns a pointer to the original file structure (the one you
 *  passed to pack_fopen_chunk()), to allow you to read or write data 
 *  after the chunk. If an error occurs, returns NULL and sets errno.
 */
PACKFILE *pack_fclose_chunk(PACKFILE *f)
{
   PACKFILE *parent = f->parent;
   PACKFILE *tmp;
   char *name = f->filename;
   int header;

   if (f->flags & PACKFILE_FLAG_WRITE) {
      /* finish writing a chunk */

      int hndl;

      /* duplicate the file descriptor to create a readable pack file,
       * the file descriptor must have been opened in read/write mode
       */
      if (f->flags & PACKFILE_FLAG_PACK)
         hndl = dup(f->parent->hndl);
      else
         hndl = dup(f->hndl);

      if (hndl<0) {
         *allegro_errno = errno;
         return NULL;
      }

      _packfile_datasize = f->todo + f->buf_size - 4;

      if (f->flags & PACKFILE_FLAG_PACK) {
	 parent = parent->parent;
	 f->parent->parent = NULL;
      }
      else
	 f->parent = NULL;

      /* close the writeable temp file, it isn't physically closed
       * because the descriptor has been duplicated
       */
      f->flags &= ~PACKFILE_FLAG_CHUNK;
      pack_fclose(f);

      lseek(hndl, 0, SEEK_SET);

      /* create a readable pack file */
      tmp = _pack_fdopen(hndl, F_READ);
      if (!tmp)
         return NULL;

      _packfile_filesize = tmp->todo - 4;

      header = pack_mgetl(tmp);

      pack_mputl(_packfile_filesize, parent);

      if (header == encrypt_id(F_PACK_MAGIC, TRUE))
	 pack_mputl(-_packfile_datasize, parent);
      else
	 pack_mputl(_packfile_datasize, parent);

      while (!pack_feof(tmp))
	 pack_putc(pack_getc(tmp), parent);

      pack_fclose(tmp);

      delete_file(name);
      free(name);
   }
   else {
      /* finish reading a chunk */
      while (f->todo > 0)
	 pack_getc(f);

      if ((f->passpos) && (f->flags & PACKFILE_FLAG_OLD_CRYPT))
	 parent->passpos = parent->passdata + (long)f->passpos - (long)f->passdata;

      free_packfile(f);
   }

   return parent;
}



/* pack_fseek:
 *  Like the stdio fseek() function, but only supports forward seeks 
 *  relative to the current file position.
 */
int pack_fseek(PACKFILE *f, int offset)
{
   int i;

   if (f->flags & PACKFILE_FLAG_WRITE)
      return -1;

   *allegro_errno = 0;

   /* skip forward through the buffer */
   if (f->buf_size > 0) {
      i = MIN(offset, f->buf_size);
      f->buf_size -= i;
      f->buf_pos += i;
      offset -= i;
      if ((f->buf_size <= 0) && (f->todo <= 0))
	 f->flags |= PACKFILE_FLAG_EOF;
   }

   /* need to seek some more? */
   if (offset > 0) {
      i = MIN(offset, f->todo);

      if ((f->flags & PACKFILE_FLAG_PACK) || (f->passpos)) {
	 /* for compressed files, we just have to read through the data */
	 while (i > 0) {
	    pack_getc(f);
	    i--;
	 }
      }
      else {
	 if (f->parent) {
	    /* pass the seek request on to the parent file */
	    pack_fseek(f->parent, i);
	 }
	 else {
	    /* do a real seek */
	    lseek(f->hndl, i, SEEK_CUR);
	 }
	 f->todo -= i;
	 if (f->todo <= 0)
	    f->flags |= PACKFILE_FLAG_EOF;
      }
   }

   return *allegro_errno;
}



/* pack_igetw:
 *  Reads a 16 bit word from a file, using intel byte ordering.
 */
int pack_igetw(PACKFILE *f)
{
   int b1, b2;

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
	 return ((b2 << 8) | b1);

   return EOF;
}



/* pack_igetl:
 *  Reads a 32 bit long from a file, using intel byte ordering.
 */
long pack_igetl(PACKFILE *f)
{
   int b1, b2, b3, b4;

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
	 if ((b3 = pack_getc(f)) != EOF)
	    if ((b4 = pack_getc(f)) != EOF)
	       return (((long)b4 << 24) | ((long)b3 << 16) |
		       ((long)b2 << 8) | (long)b1);

   return EOF;
}



/* pack_iputw:
 *  Writes a 16 bit int to a file, using intel byte ordering.
 */
int pack_iputw(int w, PACKFILE *f)
{
   int b1, b2;

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (pack_putc(b2,f)==b2)
      if (pack_putc(b1,f)==b1)
	 return w;

   return EOF;
}



/* pack_iputw:
 *  Writes a 32 bit long to a file, using intel byte ordering.
 */
long pack_iputl(long l, PACKFILE *f)
{
   int b1, b2, b3, b4;

   b1 = (int)((l & 0xFF000000L) >> 24);
   b2 = (int)((l & 0x00FF0000L) >> 16);
   b3 = (int)((l & 0x0000FF00L) >> 8);
   b4 = (int)l & 0x00FF;

   if (pack_putc(b4,f)==b4)
      if (pack_putc(b3,f)==b3)
	 if (pack_putc(b2,f)==b2)
	    if (pack_putc(b1,f)==b1)
	       return l;

   return EOF;
}



/* pack_mgetw:
 *  Reads a 16 bit int from a file, using motorola byte-ordering.
 */
int pack_mgetw(PACKFILE *f)
{
   int b1, b2;

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
	 return ((b1 << 8) | b2);

   return EOF;
}



/* pack_mgetl:
 *  Reads a 32 bit long from a file, using motorola byte-ordering.
 */
long pack_mgetl(PACKFILE *f)
{
   int b1, b2, b3, b4;

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
	 if ((b3 = pack_getc(f)) != EOF)
	    if ((b4 = pack_getc(f)) != EOF)
	       return (((long)b1 << 24) | ((long)b2 << 16) |
		       ((long)b3 << 8) | (long)b4);

   return EOF;
}



/* pack_mputw:
 *  Writes a 16 bit int to a file, using motorola byte-ordering.
 */
int pack_mputw(int w, PACKFILE *f)
{
   int b1, b2;

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (pack_putc(b1,f)==b1)
      if (pack_putc(b2,f)==b2)
	 return w;

   return EOF;
}



/* pack_mputl:
 *  Writes a 32 bit long to a file, using motorola byte-ordering.
 */
long pack_mputl(long l, PACKFILE *f)
{
   int b1, b2, b3, b4;

   b1 = (int)((l & 0xFF000000L) >> 24);
   b2 = (int)((l & 0x00FF0000L) >> 16);
   b3 = (int)((l & 0x0000FF00L) >> 8);
   b4 = (int)l & 0x00FF;

   if (pack_putc(b1,f)==b1)
      if (pack_putc(b2,f)==b2)
	 if (pack_putc(b3,f)==b3)
	    if (pack_putc(b4,f)==b4)
	       return l;

   return EOF;
}



/* pack_fread:
 *  Reads n bytes from f and stores them at memory location p. Returns the 
 *  number of items read, which will be less than n if EOF is reached or an 
 *  error occurs. Error codes are stored in errno.
 */
long pack_fread(void *p, long n, PACKFILE *f)
{
   unsigned char *cp = (unsigned char *)p;
   long c;
   int i;

   for (c=0; c<n; c++) {
      if (--(f->buf_size) > 0)
	 *(cp++) = *(f->buf_pos++);
      else {
	 i = _sort_out_getc(f);
	 if (i == EOF)
	    return c;
	 else
	    *(cp++) = i;
      }
   }

   return n;
}



/* pack_fwrite:
 *  Writes n bytes to the file f from memory location p. Returns the number 
 *  of items written, which will be less than n if an error occurs. Error 
 *  codes are stored in errno.
 */
long pack_fwrite(AL_CONST void *p, long n, PACKFILE *f)
{
   AL_CONST unsigned char *cp = (AL_CONST unsigned char *)p;
   long c;

   for (c=0; c<n; c++) {
      if (++(f->buf_size) >= F_BUF_SIZE) {
	 if (_sort_out_putc(*cp,f) != *cp)
	    return c;
	 cp++;
      }
      else
	 *(f->buf_pos++)=*(cp++);
   }

   return n;
}



/* pack_ungetc:
 *  Puts a character back in the file's input buffer.  Added by gfoot for
 *  use in the fgets function; maybe it should be in the API.  It only works
 *  for characters just fetched by pack_getc.
 */
static void pack_ungetc (int ch, PACKFILE *f)
{
   *(--f->buf_pos) = (unsigned char) ch;
   f->buf_size++;
   f->flags &= ~PACKFILE_FLAG_EOF;
}



/* pack_fgets:
 *  Reads a line from a text file, storing it at location p. Stops when a
 *  linefeed is encountered, or max bytes have been read. Returns a pointer
 *  to where it stored the text, or NULL on error. The end of line is
 *  handled by detecting optional '\r' characters optionally followed 
 *  by '\n' characters. This supports CR-LF (DOS/Windows), LF (Unix), and
 *  CR (Mac) formats.
 */
char *pack_fgets(char *p, int max, PACKFILE *f)
{
   char *pmax;
   int c;

   *allegro_errno = 0;

   if (pack_feof(f)) {
      if (ucwidth(0) <= max) usetc (p,0);
      return NULL;
   }

   pmax = p+max - ucwidth(0);

   while ((c = pack_getc (f)) != EOF) {

      if (c == '\r' || c == '\n') {
	 /* Technically we should check there's space in the buffer, and if so,
	  * add a \n.  But pack_fgets has never done this. */
	 if (c == '\r') {
	    /* eat the following \n, if any */
	    if ((c = pack_getc (f)) != '\n') pack_ungetc (c, f);
	 }
	 break;
      }

      /* is there room in the buffer? */
      if (ucwidth(c) > pmax - p) {
	 pack_ungetc (c, f);
	 c = '\0';
	 break;
      }

      /* write the character */
      p += usetc (p, c);
   }

   /* terminate the string */
   usetc (p, 0);

   if (c == '\0' || *allegro_errno)
      return NULL;

   return p;
}



/* pack_fputs:
 *  Writes a string to a text file, returning zero on success, -1 on error.
 */
int pack_fputs(AL_CONST char *p, PACKFILE *f)
{
   char *buf, *s;
   int bufsize;

   *allegro_errno = 0;

   bufsize = uconvert_size(p, U_CURRENT, U_UTF8);
   buf = malloc(bufsize);
   if (!buf)
      return -1;

   s = uconvert(p, U_CURRENT, buf, U_UTF8, bufsize);

   while (*s) {
      #if (defined ALLEGRO_DOS) || (defined ALLEGRO_WINDOWS)
	 if (*s == '\n')
	    pack_putc('\r', f);
      #endif

      pack_putc(*s, f);
      s++;
   }

   free(buf);

   if (*allegro_errno)
      return -1;
   else
      return 0;
}



/* _sort_out_getc:
 *  Helper function for the pack_getc() macro.
 */
int _sort_out_getc(PACKFILE *f)
{
   if (f->buf_size == 0) {
      if (f->todo <= 0)
	 f->flags |= PACKFILE_FLAG_EOF;
      return *(f->buf_pos++);
   }
   return refill_buffer(f);
}



/* refill_buffer:
 *  Refills the read buffer. The file must have been opened in read mode,
 *  and the buffer must be empty.
 */
static int refill_buffer(PACKFILE *f)
{
   int i, sz, done, offset;

   if ((f->flags & PACKFILE_FLAG_EOF) || (f->todo <= 0)) {
      f->flags |= PACKFILE_FLAG_EOF;
      return EOF;
   }

   if (f->parent) {
      if (f->flags & PACKFILE_FLAG_PACK) {
	 f->buf_size = pack_read(f->parent, (UNPACK_DATA *)f->pack_data, MIN(F_BUF_SIZE, f->todo), f->buf);
      }
      else {
	 f->buf_size = pack_fread(f->buf, MIN(F_BUF_SIZE, f->todo), f->parent);
      } 
      if (f->parent->flags & PACKFILE_FLAG_EOF)
	 f->todo = 0;
      if (f->parent->flags & PACKFILE_FLAG_ERROR)
	 goto err;
   }
   else {
      f->buf_size = MIN(F_BUF_SIZE, f->todo);

      offset = lseek(f->hndl, 0, SEEK_CUR);
      done = 0;

      errno = 0;
      sz = read(f->hndl, f->buf, f->buf_size);

      while (sz+done < f->buf_size) {
	 if ((sz < 0) && ((errno != EINTR) && (errno != EAGAIN)))
	    goto err;

	 if (sz > 0)
	    done += sz;

	 lseek(f->hndl, offset+done, SEEK_SET);
         errno = 0;
	 sz = read(f->hndl, f->buf+done, f->buf_size-done);
      }
      *allegro_errno = 0;

      if (errno == EINTR)
	 errno = 0;

      if ((f->passpos) && (!(f->flags & PACKFILE_FLAG_OLD_CRYPT))) {
	 for (i=0; i<f->buf_size; i++) {
	    f->buf[i] ^= *(f->passpos++);
	    if (!*f->passpos)
	       f->passpos = f->passdata;
	 }
      }
   }

   f->todo -= f->buf_size;
   f->buf_pos = f->buf;
   f->buf_size--;
   if (f->buf_size <= 0)
      if (f->todo <= 0)
	 f->flags |= PACKFILE_FLAG_EOF;

   return *(f->buf_pos++);

   err:
   *allegro_errno = EFAULT;
   f->flags |= PACKFILE_FLAG_ERROR;
   return EOF;
}



/* _sort_out_putc:
 *  Helper function for the pack_putc() macro.
 */
int _sort_out_putc(int c, PACKFILE *f)
{
   f->buf_size--;

   if (flush_buffer(f, FALSE))
      return EOF;

   f->buf_size++;
   return (*(f->buf_pos++)=c);
}



/* flush_buffer:
 * flushes a file buffer to the disk. The file must be open in write mode.
 */
static int flush_buffer(PACKFILE *f, int last)
{
   int i, sz, done, offset;

   if (f->buf_size > 0) {
      if (f->flags & PACKFILE_FLAG_PACK) {
	 if (pack_write(f->parent, (PACK_DATA *)f->pack_data, f->buf_size, f->buf, last))
	    goto err;
      }
      else {
	 if ((f->passpos) && (!(f->flags & PACKFILE_FLAG_OLD_CRYPT))) {
	    for (i=0; i<f->buf_size; i++) {
	       f->buf[i] ^= *(f->passpos++);
	       if (!*f->passpos)
		  f->passpos = f->passdata;
	    }
	 }

	 offset = lseek(f->hndl, 0, SEEK_CUR);
	 done = 0;

	 errno = 0;
	 sz = write(f->hndl, f->buf, f->buf_size);

	 while (sz+done < f->buf_size) {
	    if ((sz < 0) && ((errno != EINTR) && (errno != EAGAIN)))
	       goto err;

	    if (sz > 0)
	       done += sz;

	    lseek(f->hndl, offset+done, SEEK_SET);
	    errno = 0;
	    sz = write(f->hndl, f->buf+done, f->buf_size-done);
	 }
	 *allegro_errno = 0;
      }
      f->todo += f->buf_size;
   }
   f->buf_pos = f->buf;
   f->buf_size = 0;
   return 0;

   err:
   *allegro_errno = EFAULT;
   f->flags |= PACKFILE_FLAG_ERROR;
   return EOF;
}



/***************************************************
 ************ LZSS compression routines ************
 ***************************************************

   This compression algorithm is based on the ideas of Lempel and Ziv,
   with the modifications suggested by Storer and Szymanski. The algorithm 
   is based on the use of a ring buffer, which initially contains zeros. 
   We read several characters from the file into the buffer, and then 
   search the buffer for the longest string that matches the characters 
   just read, and output the length and position of the match in the buffer.

   With a buffer size of 4096 bytes, the position can be encoded in 12
   bits. If we represent the match length in four bits, the <position,
   length> pair is two bytes long. If the longest match is no more than
   two characters, then we send just one character without encoding, and
   restart the process with the next letter. We must send one extra bit
   each time to tell the decoder whether we are sending a <position,
   length> pair or an unencoded character, and these flags are stored as
   an eight bit mask every eight items.

   This implementation uses binary trees to speed up the search for the
   longest match.

   Original code by Haruhiko Okumura, 4/6/1989.
   12-2-404 Green Heights, 580 Nagasawa, Yokosuka 239, Japan.

   Modified for use in the Allegro filesystem by Shawn Hargreaves.

   Use, distribute, and modify this code freely.
*/



/* pack_inittree:
 *  For i = 0 to N-1, rson[i] and lson[i] will be the right and left 
 *  children of node i. These nodes need not be initialized. Also, dad[i] 
 *  is the parent of node i. These are initialized to N, which stands for 
 *  'not used.' For i = 0 to 255, rson[N+i+1] is the root of the tree for 
 *  strings that begin with character i. These are initialized to N. Note 
 *  there are 256 trees.
 */
static void pack_inittree(PACK_DATA *dat)
{
   int i;

   for (i=N+1; i<=N+256; i++)
      dat->rson[i] = N;

   for (i=0; i<N; i++)
      dat->dad[i] = N;
}



/* pack_insertnode:
 *  Inserts a string of length F, text_buf[r..r+F-1], into one of the trees 
 *  (text_buf[r]'th tree) and returns the longest-match position and length 
 *  via match_position and match_length. If match_length = F, then removes 
 *  the old node in favor of the new one, because the old one will be 
 *  deleted sooner. Note r plays double role, as tree node and position in 
 *  the buffer. 
 */
static void pack_insertnode(int r, PACK_DATA *dat)
{
   int i, p, cmp;
   unsigned char *key;
   unsigned char *text_buf = dat->text_buf;

   cmp = 1;
   key = &text_buf[r];
   p = N + 1 + key[0];
   dat->rson[r] = dat->lson[r] = N;
   dat->match_length = 0;

   for (;;) {

      if (cmp >= 0) {
	 if (dat->rson[p] != N)
	    p = dat->rson[p];
	 else {
	    dat->rson[p] = r;
	    dat->dad[r] = p;
	    return;
	 }
      }
      else {
	 if (dat->lson[p] != N)
	    p = dat->lson[p];
	 else {
	    dat->lson[p] = r;
	    dat->dad[r] = p;
	    return;
	 }
      }

      for (i = 1; i < F; i++)
	 if ((cmp = key[i] - text_buf[p + i]) != 0)
	    break;

      if (i > dat->match_length) {
	 dat->match_position = p;
	 if ((dat->match_length = i) >= F)
	    break;
      }
   }

   dat->dad[r] = dat->dad[p];
   dat->lson[r] = dat->lson[p];
   dat->rson[r] = dat->rson[p];
   dat->dad[dat->lson[p]] = r;
   dat->dad[dat->rson[p]] = r;
   if (dat->rson[dat->dad[p]] == p)
      dat->rson[dat->dad[p]] = r;
   else
      dat->lson[dat->dad[p]] = r;
   dat->dad[p] = N;                 /* remove p */
}



/* pack_deletenode:
 *  Removes a node from a tree.
 */
static void pack_deletenode(int p, PACK_DATA *dat)
{
   int q;

   if (dat->dad[p] == N)
      return;     /* not in tree */

   if (dat->rson[p] == N)
      q = dat->lson[p];
   else
      if (dat->lson[p] == N)
	 q = dat->rson[p];
      else {
	 q = dat->lson[p];
	 if (dat->rson[q] != N) {
	    do {
	       q = dat->rson[q];
	    } while (dat->rson[q] != N);
	    dat->rson[dat->dad[q]] = dat->lson[q];
	    dat->dad[dat->lson[q]] = dat->dad[q];
	    dat->lson[q] = dat->lson[p];
	    dat->dad[dat->lson[p]] = q;
	 }
	 dat->rson[q] = dat->rson[p];
	 dat->dad[dat->rson[p]] = q;
      }

   dat->dad[q] = dat->dad[p];
   if (dat->rson[dat->dad[p]] == p)
      dat->rson[dat->dad[p]] = q;
   else
      dat->lson[dat->dad[p]] = q;

   dat->dad[p] = N;
}



/* pack_write:
 *  Called by flush_buffer(). Packs size bytes from buf, using the pack 
 *  information contained in dat. Returns 0 on success.
 */
static int pack_write(PACKFILE *file, PACK_DATA *dat, int size, unsigned char *buf, int last)
{
   int i = dat->i;
   int c = dat->c;
   int len = dat->len;
   int r = dat->r;
   int s = dat->s;
   int last_match_length = dat->last_match_length;
   int code_buf_ptr = dat->code_buf_ptr;
   unsigned char mask = dat->mask;
   int ret = 0;

   if (dat->state==2)
      goto pos2;
   else
      if (dat->state==1)
	 goto pos1;

   dat->code_buf[0] = 0;
      /* code_buf[1..16] saves eight units of code, and code_buf[0] works
	 as eight flags, "1" representing that the unit is an unencoded
	 letter (1 byte), "0" a position-and-length pair (2 bytes).
	 Thus, eight units require at most 16 bytes of code. */

   code_buf_ptr = mask = 1;

   s = 0;
   r = N - F;
   pack_inittree(dat);

   for (len=0; (len < F) && (size > 0); len++) {
      dat->text_buf[r+len] = *(buf++);
      if (--size == 0) {
	 if (!last) {
	    dat->state = 1;
	    goto getout;
	 }
      }
      pos1:
	 ; 
   }

   if (len == 0)
      goto getout;

   for (i=1; i <= F; i++)
      pack_insertnode(r-i,dat);
	    /* Insert the F strings, each of which begins with one or
	       more 'space' characters. Note the order in which these
	       strings are inserted. This way, degenerate trees will be
	       less likely to occur. */

   pack_insertnode(r,dat);
	    /* Finally, insert the whole string just read. match_length
	       and match_position are set. */

   do {
      if (dat->match_length > len)
	 dat->match_length = len;  /* match_length may be long near the end */

      if (dat->match_length <= THRESHOLD) {
	 dat->match_length = 1;  /* not long enough match: send one byte */
	 dat->code_buf[0] |= mask;    /* 'send one byte' flag */
	 dat->code_buf[code_buf_ptr++] = dat->text_buf[r]; /* send uncoded */
      }
      else {
	 /* send position and length pair. Note match_length > THRESHOLD */
	 dat->code_buf[code_buf_ptr++] = (unsigned char) dat->match_position;
	 dat->code_buf[code_buf_ptr++] = (unsigned char)
				     (((dat->match_position >> 4) & 0xF0) |
				      (dat->match_length - (THRESHOLD + 1)));
      }

      if ((mask <<= 1) == 0) {                  /* shift mask left one bit */
	 if ((file->passpos) && (file->flags & PACKFILE_FLAG_OLD_CRYPT)) {
	    dat->code_buf[0] ^= *file->passpos;
	    file->passpos++;
	    if (!*file->passpos)
	       file->passpos = file->passdata;
	 };

	 for (i=0; i<code_buf_ptr; i++)         /* send at most 8 units of */
	    pack_putc(dat->code_buf[i], file);  /* code together */

	 if (pack_ferror(file)) {
	    ret = EOF;
	    goto getout;
	 }
	 dat->code_buf[0] = 0;
	 code_buf_ptr = mask = 1;
      }

      last_match_length = dat->match_length;

      for (i=0; (i < last_match_length) && (size > 0); i++) {
	 c = *(buf++);
	 if (--size == 0) {
	    if (!last) {
	       dat->state = 2;
	       goto getout;
	    }
	 }
	 pos2:
	 pack_deletenode(s,dat);    /* delete old strings and */
	 dat->text_buf[s] = c;      /* read new bytes */
	 if (s < F-1)
	    dat->text_buf[s+N] = c; /* if the position is near the end of
				       buffer, extend the buffer to make
				       string comparison easier */
	 s = (s+1) & (N-1);
	 r = (r+1) & (N-1);         /* since this is a ring buffer,
				       increment the position modulo N */

	 pack_insertnode(r,dat);    /* register the string in
				       text_buf[r..r+F-1] */
      }

      while (i++ < last_match_length) {   /* after the end of text, */
	 pack_deletenode(s,dat);          /* no need to read, but */
	 s = (s+1) & (N-1);               /* buffer may not be empty */
	 r = (r+1) & (N-1);
	 if (--len)
	    pack_insertnode(r,dat); 
      }

   } while (len > 0);   /* until length of string to be processed is zero */

   if (code_buf_ptr > 1) {         /* send remaining code */
      if ((file->passpos) && (file->flags & PACKFILE_FLAG_OLD_CRYPT)) {
	 dat->code_buf[0] ^= *file->passpos;
	 file->passpos++;
	 if (!*file->passpos)
	    file->passpos = file->passdata;
      };

      for (i=0; i<code_buf_ptr; i++) {
	 pack_putc(dat->code_buf[i], file);
	 if (pack_ferror(file)) {
	    ret = EOF;
	    goto getout;
	 }
      }
   }

   dat->state = 0;

   getout:

   dat->i = i;
   dat->c = c;
   dat->len = len;
   dat->r = r;
   dat->s = s;
   dat->last_match_length = last_match_length;
   dat->code_buf_ptr = code_buf_ptr;
   dat->mask = mask;

   return ret;
}



/* pack_read:
 *  Called by refill_buffer(). Unpacks from dat into buf, until either
 *  EOF is reached or s bytes have been extracted. Returns the number of
 *  bytes added to the buffer
 */
static int pack_read(PACKFILE *file, UNPACK_DATA *dat, int s, unsigned char *buf)
{
   int i = dat->i;
   int j = dat->j;
   int k = dat->k;
   int r = dat->r;
   int c = dat->c;
   unsigned int flags = dat->flags;
   int size = 0;

   if (dat->state==2)
      goto pos2;
   else
      if (dat->state==1)
	 goto pos1;

   r = N-F;
   flags = 0;

   for (;;) {
      if (((flags >>= 1) & 256) == 0) {
	 if ((c = pack_getc(file)) == EOF)
	    break;

	 if ((file->passpos) && (file->flags & PACKFILE_FLAG_OLD_CRYPT)) {
	    c ^= *file->passpos;
	    file->passpos++;
	    if (!*file->passpos)
	       file->passpos = file->passdata;
	 };

	 flags = c | 0xFF00;        /* uses higher byte to count eight */
      }

      if (flags & 1) {
	 if ((c = pack_getc(file)) == EOF)
	    break;
	 dat->text_buf[r++] = c;
	 r &= (N - 1);
	 *(buf++) = c;
	 if (++size >= s) {
	    dat->state = 1;
	    goto getout;
	 }
	 pos1:
	    ; 
      }
      else {
	 if ((i = pack_getc(file)) == EOF)
	    break;
	 if ((j = pack_getc(file)) == EOF)
	    break;
	 i |= ((j & 0xF0) << 4);
	 j = (j & 0x0F) + THRESHOLD;
	 for (k=0; k <= j; k++) {
	    c = dat->text_buf[(i + k) & (N - 1)];
	    dat->text_buf[r++] = c;
	    r &= (N - 1);
	    *(buf++) = c;
	    if (++size >= s) {
	       dat->state = 2;
	       goto getout;
	    }
	    pos2:
	       ; 
	 }
      }
   }

   dat->state = 0;

   getout:

   dat->i = i;
   dat->j = j;
   dat->k = k;
   dat->r = r;
   dat->c = c;
   dat->flags = flags;

   return size;
}


