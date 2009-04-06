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
 *      Unicode support routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_UNICODE__H
#define ALLEGRO_UNICODE__H

#include "allegro5/base.h"

#ifdef __cplusplus
   extern "C" {
#endif

#define U_ASCII         AL_ID('A','S','C','8')
#define U_ASCII_CP      AL_ID('A','S','C','P')
#define U_UNICODE       AL_ID('U','N','I','C')
#define U_UTF8          AL_ID('U','T','F','8')

AL_FUNC(void, set_ucodepage, (AL_CONST unsigned short *table, AL_CONST unsigned short *extras));

AL_FUNC(int, uwidth_max, (int type));

AL_FUNCPTR(int, ugetc, (AL_CONST char *s));
AL_FUNCPTR(int, ugetx, (char **s));
AL_FUNCPTR(int, ugetxc, (AL_CONST char **s));
AL_FUNCPTR(int, usetc, (char *s, int c));
AL_FUNCPTR(int, uwidth, (AL_CONST char *s));
AL_FUNCPTR(int, ucwidth, (int c));
AL_FUNCPTR(int, uisok, (int c));
AL_FUNC(int, uoffset, (AL_CONST char *s, int idx));
AL_FUNC(int, ugetat, (AL_CONST char *s, int idx));
AL_FUNC(int, usetat, (char *s, int idx, int c));
AL_FUNC(int, uinsert, (char *s, int idx, int c));
AL_FUNC(int, uremove, (char *s, int idx));
AL_FUNC(int, uisspace, (int c));
AL_FUNC(int, uisdigit, (int c));
AL_FUNC(int, ustrsize, (AL_CONST char *s));
AL_FUNC(int, ustrsizez, (AL_CONST char *s));
AL_FUNC(char *, _ustrdup, (AL_CONST char *src, AL_METHOD(void *, malloc_func, (size_t))));
AL_FUNC(char *, ustrzcpy, (char *dest, int size, AL_CONST char *src));
AL_FUNC(char *, ustrchr, (AL_CONST char *s, int c));


#ifndef ustrdup
   #ifdef FORTIFY
      #define ustrdup(src)            _ustrdup(src, Fortify_malloc)
   #else
      #define ustrdup(src)            _ustrdup(src, malloc)
   #endif
#endif

#define ustrcpy(dest, src)            ustrzcpy(dest, INT_MAX, src)
#define ustrcat(dest, src)            ustrzcat(dest, INT_MAX, src)
#define ustrncpy(dest, src, n)        ustrzncpy(dest, INT_MAX, src, n)
#define ustrncat(dest, src, n)        ustrzncat(dest, INT_MAX, src, n)

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_UNICODE__H */


