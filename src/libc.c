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
 *      Henrik Stokseth added _al_sane_realloc() and _al_sane_strncpy() functions.
 *
 *      _al_srand() and _al_rand() functions based on code by Paul Pridham.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include <string.h>
#include <ctype.h>


static int _al_rand_seed = 0;


/* _al_stricmp:
 *  Case-insensitive comparison of 7-bit ASCII strings.
 */
int _al_stricmp(const char *s1, const char *s2)
{
   int c1, c2;
   ASSERT(s1);
   ASSERT(s2);

   do {
      c1 = tolower(*(s1++));
      c2 = tolower(*(s2++));
   } while ((c1) && (c1 == c2));

   return c1 - c2;
}


/* _al_sane_realloc:
 *  al_realloc() substitution with guaranteed behaviour.
 */
void *_al_sane_realloc(void *ptr, size_t size)
{
   void *tmp_ptr;

   tmp_ptr = NULL;

   if (ptr && size) {
      tmp_ptr = al_realloc(ptr, size);
      if (!tmp_ptr && ptr) al_free(ptr);
   }
   else if (!size) {
      tmp_ptr = NULL;
      if (ptr) al_free(ptr);
   }
   else if (!ptr) {
      tmp_ptr = al_malloc(size);
   }
   
   return tmp_ptr;
}



/* _al_sane_strncpy:
 *  strncpy() substitution which properly null terminates a string.
 */
char *_al_sane_strncpy(char *dest, const char *src, size_t n)
{
   if (n <= 0)
      return dest;
   dest[0] = '\0';
   strncat(dest, src, n - 1);
   
   return dest;
}



/* _al_srand:
 *  Seed initialization routine for rand() replacement.
 */
void _al_srand(int seed)
{
   _al_rand_seed = seed;
}



/* _al_rand:
 *  Simple rand() replacement with guaranteed randomness in the lower 16 bits.
 */
int _al_rand(void)
{
   _al_rand_seed = (_al_rand_seed + 1) * 1103515245 + 12345;
   return ((_al_rand_seed >> 16) & _AL_RAND_MAX);
}
