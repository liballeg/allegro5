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
 *      Makedoc's SciTE API output routines.
 *
 *      By Shawn Hargreaves.
 *
 *      Made by Bobby Ferris.
 *
 *      See readme.txt for copyright information.
 *
 *      See allegro/docs/src/makedoc/format.txt for a brief description of
 *      the source of _tx files.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "makesci.h"
#include "makemisc.h"

#ifndef bool
 #define bool int
#endif

#ifndef true
 #define true 1
 #define false 0
#endif

/* file_size:
 * returns the size of the file specified
 */
static int _file_size(const char *filename)
{
  int ret;
  FILE *f = fopen(filename, "r");
  for(ret = 0; !feof(f); ++ret, fgetc(f))
    ;
  fclose(f);
  return ret;
}



/* baf_strtolower:
 * makes a string all lower case
 */
static char *baf_strtolower(char *str)
{
  int i;
  char *ret = malloc(strlen(str) + 1);
  for(i = 0; i < strlen(str); ++i)
  {
    ret[i] = mytolower(str[i]);
  }
  ret[strlen(str)] = '\0';
  return ret;
}



/* ignore_line:
 * tells if it is supposed to ignore the line, of if it contains data for the API file
 */
static bool _ignore_line(char *x)
{
  char *lowerx;
  lowerx = baf_strtolower(x);
  if(strncmp(lowerx, "typedef", 7) == 0)
  {
    free(lowerx);
    return true;
  }
  if(strncmp(lowerx, "extern", 6) == 0)
  {
    free(lowerx);
    return true;
  }
  if(strncmp(lowerx, "struct", 6) == 0)
  {
    free(lowerx);
    return true;
  }
  if(strncmp(lowerx, "example", 7) == 0)
  {
    free(lowerx);
    return true;
  }
  if(strncmp(lowerx, "drivers", 7) == 0)
  {
    free(lowerx);
    return true;
  }
  free(lowerx);
  return false;
}



/* convert:
 * converts filename into apifilename
 */
static void _convert(const char *filename, const char *apifilename)
{
  FILE *tx_file, *api_file;
  int i, fs = _file_size(filename), offset;
  char tmp[1024];
  char *buf = malloc(fs+1);
  memset((void *)buf, '\0', fs);
  tx_file = fopen(filename, "r");
  fread((void *)buf, sizeof(char), fs, tx_file); // read in the file
  fclose(tx_file);
  
  api_file = fopen(apifilename, "w");
  
  for(i = 0; i < fs; ++i)
  {
    if(buf[i] == '@' && buf[i+1] == '\\')
    {
      int index;
      memset((void *)tmp, '\0', 1024);
      for(; buf[i] == '@' || buf[i] == '\\' || buf[i] == ' '; ++i)
        ;
      for(offset = i; buf[i] != '@'; ++i)
        tmp[i-offset] = buf[i];
      tmp[i-offset-1] = '\0';
      ++i;
      if(!_ignore_line(tmp))
      {
        memset((void *)tmp, '\0', 1024);
        for(offset = i; buf[i] != '\n'; ++i)
          tmp[i-offset] = buf[i];
        index = i-offset;
        ++i;
        bool done = false;
        bool will_be_done = false;
        while(!done)
        {
          if(buf[i] == '@' && buf[i+1] == '@')
            will_be_done = true;
          for(; buf[i] == '@' || buf[i] == '\\' || buf[i] == ' '; ++i)
            ;
          --i;
          for(offset = i; buf[i] != '\n'; ++i)
            tmp[(i - offset)+index] = buf[i];
          index = (i - offset)+index;
          ++i;
          if(will_be_done)
            done=true;
        }
        fprintf(api_file, "%s\n", tmp);
      }
      else
      {
        memset((void *)tmp, '\0', 1024);
        for(offset = i; buf[i] != '\n'; ++i)
          ;
        ++i;
        bool done = false;
        bool will_be_done = false;
        while(!done)
        {
          if(buf[i] == '@' && buf[i+1] == '@')
            will_be_done = true;
          for(; buf[i] == '@' || buf[i] == '\\' || buf[i] == ' '; ++i)
            ;
          --i;
          for(offset = i; buf[i] != '\n'; ++i)
            ;
          ++i;
          if(will_be_done)
            done=true;
        }
      }
    }
    else if(buf[i] == '@' && buf[i+1] == '@')
    {
      memset((void *)tmp, '\0', 1024);
      for(; buf[i] == '@'; ++i)
        ;
      for(offset = i; buf[i] != '@'; ++i)
        tmp[i-offset] = buf[i];
      tmp[i-offset-1] = '\0';
      ++i;
      if(!_ignore_line(tmp))
      {
        memset((void *)tmp, '\0', 1024);
        for(offset = i; buf[i] != '\n'; ++i)
          tmp[i-offset] = buf[i];
        fprintf(api_file, "%s\n", tmp);
        ++i;
      }
    }
  }
  
  fclose(api_file);
  free(buf);
}

/* write_api:
 * Entry point to the function which translates makedoc's format
 * to correct SciTE API output.
 */
int write_scite(char *filename, char *src)
{
   printf("Writing %s (SciTE API File)\n", filename);
   _convert(src, filename);
   return 0;
}
