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
 *      Emulation for libc routines that may be missing on some platforms.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"

#ifndef ALLEGRO_MPW
   #include <sys/stat.h>
#endif


#ifdef ALLEGRO_NO_STRLWR

/* _alemu_strlwr:
 *  Convert all upper case characters in string to lower case.
 */
char *_alemu_strlwr(char *string)
{
   char *p;

   for (p=string; *p; p++)
      *p = utolower(*p);

   return string;
}

#endif



#ifdef ALLEGRO_NO_STRUPR

/* _alemu_strupr:
 *  Convert all lower case characters in string to upper case.
 */
char *_alemu_strupr(char *string)
{
   char *p;

   for (p=string; *p; p++)
      *p = utoupper(*p);

   return string;
}

#endif



#ifdef ALLEGRO_NO_STRICMP

/* _alemu_stricmp:
 *  Case-insensitive comparison of strings.
 */
int _alemu_stricmp(AL_CONST char *s1, AL_CONST char *s2)
{
   int c1, c2;

   do {
      c1 = utolower(*(s1++));
      c2 = utolower(*(s2++));
   } while ((c1) && (c1 == c2));

   return c1 - c2;
}

#endif



#ifdef ALLEGRO_NO_MEMCMP

/* _alemu_memcmp:
 *  Comparison of two memory blocks.
 */
int _alemu_memcmp(AL_CONST void *s1, AL_CONST void *s2, size_t num)
{
   size_t i;

   for (i=0; i<num; i++)
      if (((unsigned char *)s1)[i] != ((unsigned char *)s2)[i])
	 return ((((unsigned char *)s1)[i] < ((unsigned char *)s2)[i]) ? -1 : 1);

   return 0;
}

#endif



#ifdef ALLEGRO_NO_FINDFIRST

#ifdef HAVE_DIRENT_H
   #include <sys/types.h>
   #include <dirent.h>
   #define NAMLEN(dirent) (strlen((dirent)->d_name))
#else
   #define dirent direct
   #define NAMLEN(dirent) ((dirent)->d_namlen)
   #ifdef HAVE_SYS_NDIR_H
      #include <sys/ndir.h>
   #endif
   #ifdef HAVE_SYS_DIR_H
      #include <sys/dir.h>
   #endif
   #ifdef HAVE_NDIR_H
      #include <ndir.h>
   #endif
#endif

#ifdef TIME_WITH_SYS_TIME
   #include <sys/time.h>
   #include <time.h>
#else
   #ifdef HAVE_SYS_TIME_H
      #include <sys/time.h>
   #else
      #include <time.h>
   #endif
#endif



/* ff_get_filename:
 *  When passed a completely specified file path, this returns a pointer
 *  to the filename portion.
 */
static char *ff_get_filename(AL_CONST char *path)
{
   char *p = (char*)path + strlen(path);

   while ((p > path) && (*(p - 1) != '/'))
      p--;

   return p;
}



/* ff_put_backslash:
 *  If the last character of the filename is not a /, this routine will
 *  concatenate a / on to it.
 */
static void ff_put_backslash(char *filename, int size)
{
   int len = strlen(filename);

   if ((len > 0) && (len < (size - 1)) && (filename[len - 1] != '/')) {
      filename[len] = '/';
      filename[len + 1] = 0;
   }
}



#define FF_MAXPATHLEN 1024


struct ff_info
{
   int attrib;
   char dirname[FF_MAXPATHLEN];
   char pattern[FF_MAXPATHLEN];
   char tempname[FF_MAXPATHLEN];
   char filename[FF_MAXPATHLEN];
   DIR *dir;
   struct dirent *entry;
   struct stat stat;
};


static int ff_match(AL_CONST char *s1, AL_CONST char *s2);



/* _alemu_findfirst:
 *  find first file matching pattern, which has no attribs
 *  that are not present in attrib.
 */
int _alemu_findfirst(AL_CONST char *pattern, struct ffblk *ffblk, int attrib)
{
   struct ff_info *ff_info;
   int ret;

   /* allocate ff_info structure */
   ff_info = malloc(sizeof(struct ff_info));

   if (!ff_info)
      return (errno = ENOMEM);

   ffblk->ff_info = ff_info;

   /* initialize ff_info structure */
   ff_info->attrib = attrib;

   ff_info->dirname[0] = 0;
   strncat(ff_info->dirname, pattern, sizeof(ff_info->dirname) - 1);
   *ff_get_filename(ff_info->dirname) = 0;
   if (ff_info->dirname[0] == 0)
      strcpy(ff_info->dirname, "./");

   ff_info->pattern[0] = 0;
   strncat(ff_info->pattern, ff_get_filename(pattern), sizeof(ff_info->pattern) - 1);

   /* nasty bodge, but gives better compatibility with DOS programs */
   if (strcmp(ff_info->pattern, "*.*") == 0)
      strcpy(ff_info->pattern, "*");

   /* open directory */
   ff_info->dir = opendir(ff_info->dirname);

   if (ff_info->dir == 0) {
      free(ff_info);
      return ((errno == 0) ? ENOENT : errno);
   }

   ret = _alemu_findnext(ffblk);

   if (ret) {
      closedir(ff_info->dir);
      free(ff_info);
   }

   return ret;
}



/* _alemu_findnext:
 *  Find next file matching pattern.
 */
int _alemu_findnext(struct ffblk *ffblk)
{
   struct ff_info *ff_info = ffblk->ff_info;
   struct tm *mtime;

   while (1) {
      /* read directory entry */
      ff_info->entry = readdir(ff_info->dir);
      if (ff_info->entry == 0)
	 return ((errno == 0) ? ENOENT : errno);

      /* try to match file name with pattern */
      ff_info->tempname[0] = 0;
      if (NAMLEN(ff_info->entry) >= sizeof(ff_info->tempname))
	 strncat(ff_info->tempname, ff_info->entry->d_name, sizeof(ff_info->tempname) - 1);
      else
	 strncat(ff_info->tempname, ff_info->entry->d_name, NAMLEN(ff_info->entry));
      if (!ff_match(ff_info->tempname, ff_info->pattern))
	 continue;

      strcpy(ff_info->filename, ff_info->dirname);
      ff_put_backslash(ff_info->filename, sizeof(ff_info->filename));
      strncat(ff_info->filename, ff_info->tempname,
	      sizeof(ff_info->filename) - strlen(ff_info->filename) - 1);

      /* get file statistics */
      if (stat(ff_info->filename, &(ff_info->stat)))
	 continue;

      ffblk->ff_attrib = 0;
      if ((ff_info->stat.st_mode & S_IRUSR) == 0)
	 ffblk->ff_attrib |= FA_RDONLY;
      if (S_ISDIR(ff_info->stat.st_mode))
	 ffblk->ff_attrib |= FA_DIREC;
      if ((ff_info->tempname[0] == '.')
	  && ((ff_info->tempname[1] != '.')
	      || (ff_info->tempname[2] != 0)))
	 ffblk->ff_attrib |= FA_HIDDEN;

      /* have any attributes, not listed in attrib? */
      if ((ffblk->ff_attrib & ~ff_info->attrib) != 0)
	 continue;

      mtime = gmtime(&(ff_info->stat.st_mtime));
      ffblk->ff_ftime = ((mtime->tm_hour << 11) | (mtime->tm_min << 5)
			 | (mtime->tm_sec >> 1));
      ffblk->ff_fdate = (((mtime->tm_year - 1980) << 9) | (mtime->tm_mon << 5)
			 | (mtime->tm_mday));

      ffblk->ff_fsize = ff_info->stat.st_size;

      ffblk->ff_name[0] = 0;
      strncat(ffblk->ff_name, ff_info->tempname, sizeof(ffblk->ff_name) - 1);

      return 0;
   }
}



/* _alemu_findclose:
 *  Shuts down a findfirst/findnext sequence.
 */
void _alemu_findclose(struct ffblk *ffblk)
{
   struct ff_info *ff_info = ffblk->ff_info;

   if (ff_info) {
      if (ff_info->dir != 0)
	 closedir(ff_info->dir);

      free(ff_info);
   }
}



#define FF_MATCH_TRY 0
#define FF_MATCH_ONE 1
#define FF_MATCH_ANY 2


struct ff_match_data
{
   int type;
   AL_CONST char *s1;
   AL_CONST char *s2;
};



/* ff_match:
 *  Match two strings ('*' matches any number of characters,
 *  '?' matches any character).
 */
static int ff_match(AL_CONST char *s1, AL_CONST char *s2)
{
   static int size = 0;
   static struct ff_match_data *data = 0;
   AL_CONST char *s1end = s1 + strlen(s1);
   int index, c1, c2;

   /* allocate larger working area if necessary */
   if ((data != 0) && (size < strlen(s2))) {
      free(data);
      data = 0;
   }
   if (data == 0) {
      size = strlen(s2);
      data = malloc(sizeof(struct ff_match_data) * size * 2 + 1);
      if (data == 0)
	 return 0;
   }

   index = 0;
   data[0].s1 = s1;
   data[0].s2 = s2;
   data[0].type = FF_MATCH_TRY;

   while (index >= 0) {
      s1 = data[index].s1;
      s2 = data[index].s2;
      c1 = *s1;
      c2 = *s2;

      switch (data[index].type) {

      case FF_MATCH_TRY:
	 if (c2 == 0) {
	    /* pattern exhausted */
	    if (c1 == 0)
	       return 1;
	    else
	       index--;
	 }
	 else if (c1 == 0) {
	    /* string exhausted */
	    while (*s2 == '*')
	       s2++;
	    if (*s2 == 0)
	       return 1;
	    else
	       index--;
	 }
	 else if (c2 == '*') {
	    /* try to match the rest of pattern with empty string */
	    data[index++].type = FF_MATCH_ANY;
	    data[index].s1 = s1end;
	    data[index].s2 = s2 + 1;
	    data[index].type = FF_MATCH_TRY;
	 }
	 else if ((c2 == '?') || (c1 == c2)) {
	    /* try to match the rest */
	    data[index++].type = FF_MATCH_ONE;
	    data[index].s1 = s1 + 1;
	    data[index].s2 = s2 + 1;
	    data[index].type = FF_MATCH_TRY;
	 }
	 else
	    index--;
	 break;

      case FF_MATCH_ONE:
	 /* the rest of string did not match, try earlier */
	 index--;
	 break;

      case FF_MATCH_ANY:
	 /* rest of string did not match, try add more chars to string tail */
	 if (--data[index + 1].s1 >= s1) {
	    data[index + 1].type = FF_MATCH_TRY;
	    index++;
	 }
	 else
	    index--;
	 break;

      default:
	 /* this is a bird? This is a plane? No it's a bug!!! */
	 return 0;
      }
   }

   return 0;
}

#endif

