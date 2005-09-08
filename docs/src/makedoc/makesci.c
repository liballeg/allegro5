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
 *      By Bobby Ferris.
 *
 *      See readme.txt for copyright information.
 *
 *      See allegro/docs/src/makedoc/format.txt for a brief description of
 *      the source of _tx files.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "makesci.h"
#include "makemisc.h"

/* _file_size:
 * Returns the size of the file specified.
 */
static int _file_size(const char *filename)
{
  int ret = 0;
  FILE *f = fopen(filename, "r");
  if(f) {
    if(fseek(f, 0L, SEEK_END) == 0)
      ret = ftell(f);
    fclose(f);
  }
  return ret;
}



/* _ignore_line:
 * Tells if it is supposed to ignore the line, of if it contains data for the
 * API file.
 */
static int _ignore_line(char *x)
{
  return strincmp(x, "typedef") == 0 ||
         strincmp(x, "extern")  == 0 ||
         strincmp(x, "struct")  == 0 ||
         strincmp(x, "example") == 0 ||
         strincmp(x, "drivers") == 0 ||
         strincmp(x, "#define") == 0;
}



/* _convert:
 * Converts filename into apifilename.
 * Returns: 0 if converted successfully
 *          1 if an error occurred.
 */
static int _convert(const char *filename, const char *apifilename)
{
  FILE *tx_file, *api_file;
  int i, fs, offset;
  char tmp[1024];
  char *buf;

  tx_file = fopen(filename, "r");
  if(!tx_file)
    return 1;
  fs = _file_size(filename);
  buf = malloc(fs+1);
  fread(buf, sizeof(char), fs, tx_file); /* read in the file */
  fclose(tx_file);

  api_file = fopen(apifilename, "w");
  if (!api_file) {
    free(buf);
    return 1;
  }
  
  for(i = 0; i < fs; ++i)
  {
    if(buf[i] == '@' && buf[i+1] == '\\')
    {
      int index;
      memset(tmp, '\0', sizeof(tmp));
      for(; buf[i] == '@' || buf[i] == '\\' || buf[i] == ' '; ++i)
        ;
      for(offset = i; buf[i] != '@'; ++i)
        tmp[i-offset] = buf[i];
      tmp[i-offset-1] = '\0';
      ++i;
      if(!_ignore_line(tmp))
      {
        int done = 0;
        int will_be_done = 0;
        memset(tmp, '\0', sizeof(tmp));
        for(offset = i; buf[i] != '\n'; ++i)
          tmp[i-offset] = buf[i];
        index = i-offset;
        ++i;
        while(!done)
        {
          if(buf[i] == '@' && buf[i+1] == '@')
            will_be_done = 1;
          for(; buf[i] == '@' || buf[i] == '\\' || buf[i] == ' '; ++i)
            ;
          --i;
          for(offset = i; buf[i] != '\n'; ++i)
            tmp[(i - offset)+index] = buf[i];
          index = (i - offset)+index;
          ++i;
          if(will_be_done)
            done=1;
        }
        fprintf(api_file, "%s\n", tmp);
      }
      else
      {
        int done = 0;
        int will_be_done = 0;
        memset(tmp, '\0', sizeof(tmp));
        for(offset = i; buf[i] != '\n'; ++i)
          ;
        ++i;
        while(!done)
        {
          if(buf[i] == '@' && buf[i+1] == '@')
            will_be_done = 1;
          for(; buf[i] == '@' || buf[i] == '\\' || buf[i] == ' '; ++i)
            ;
          --i;
          for(offset = i; buf[i] != '\n'; ++i)
            ;
          ++i;
          if(will_be_done)
            done=1;
        }
      }
    }
    else if(buf[i] == '@' && buf[i+1] == '@')
    {
      memset(tmp, '\0', sizeof(tmp));
      for(; buf[i] == '@'; ++i)
        ;
      for(offset = i; buf[i] != '@'; ++i)
        tmp[i-offset] = buf[i];
      tmp[i-offset-1] = '\0';
      ++i;
      if(!_ignore_line(tmp))
      {
        memset(tmp, '\0', sizeof(tmp));
        for(offset = i; buf[i] != '\n'; ++i)
          tmp[i-offset] = buf[i];
        fprintf(api_file, "%s\n", tmp);
        ++i;
      }
    }
  }
  
  fclose(api_file);
  free(buf);
  return 0;
}



/* write_scite:
 * Entry point to the function which translates makedoc's format
 * to correct SciTE API output.
 */
int write_scite(char *filename, char *src)
{
   /*printf("Writing %s (SciTE API File)\n", filename);*/
   return _convert(src, filename);
}
